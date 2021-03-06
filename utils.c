/*
Weborf
Copyright (C) 2007  Salvo "LtWorf" Tomaselli

Weborf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@author Salvo "LtWorf" Tomaselli <tiposchi@tiscali.it>

 */
#include "options.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <netdb.h>
#include <dirent.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>

#include "mystring.h"
#include "utils.h"
#include "embedded_auth.h"

/**
This function reads the directory dir, putting inside the html string an html
page with links to all the files within the directory.

Buffer for html must be allocated by the calling function.
bufsize is the size of the buffer allocated for html
parent is true when the dir has a parent dir
*/
int list_dir(connection_t *connection_prop, char *html, unsigned int bufsize, bool parent) {
    int pagesize=0; //Written bytes on the page
    int maxsize = bufsize - 1; //String's max size
    int printf_s;
    char *color; //Depending on row count chooses a background color
    char *measure; //contains measure unit for file's size (B, KiB, MiB)
    int counter = 0;

    char path[INBUFFER]; //Buffer to contain element's absolute path

    struct dirent **namelist;
    counter = scandir(connection_prop->strfile, &namelist, 0, alphasort);


    if (counter <0) { //Open not succesfull
        return -1;
    }

    //Specific header table)
    pagesize=printf_s=snprintf(html,maxsize,"%s<table><tr><td></td><td>Name</td><td>Size</td><td>Last Modified</td></tr>",HTMLHEAD);
    maxsize-=printf_s;

    //Cycles trough dir's elements
    int i;
    struct tm ts;
    struct stat f_prop; //File's property
    char last_modified[URI_LEN];

    //Print link to parent directory, if there is any
    if (parent) {
        printf_s=snprintf(html+pagesize,maxsize,"<tr style=\"background-color: #DFDFDF;\"><td>d</td><td><a href=\"../\">../</a></td><td>-</td><td>-</td></tr>");
        maxsize-=printf_s;
        pagesize+=printf_s;
    }

    for (i=0; i<counter; i++) {
        //Skipping hidden files
        if (namelist[i]->d_name[0] == '.') {
            free(namelist[i]);
            continue;
        }

        snprintf(path, INBUFFER,"%s/%s", connection_prop->strfile, namelist[i]->d_name);

        //Stat on the entry

        stat(path, &f_prop);
        int f_mode = f_prop.st_mode; //Get's file's mode

        //get last modified
        localtime_r(&f_prop.st_mtime,&ts);
        strftime(last_modified,URI_LEN, "%a, %d %b %Y %H:%M:%S GMT", &ts);

        if (S_ISREG(f_mode)) { //Regular file

            //Table row for the file

            //Scaling the file's size
            unsigned long long int size = f_prop.st_size;
            if (size < 1024) {
                measure="B";
            } else if ((size = (size / 1024)) < 1024) {
                measure="KiB";
            } else if ((size = (size / 1024)) < 1024) {
                measure="MiB";
            } else {
                size = size / 1024;
                measure="GiB";
            }

            if (i % 2 == 0)
                color = "white";
            else
                color = "#EAEAEA";

            printf_s=snprintf(html+pagesize,maxsize,
                              "<tr style=\"background-color: %s;\"><td>f</td><td><a href=\"%s\">%s</a></td><td>%lld%s</td><td>%s</td></tr>\n",
                              color, namelist[i]->d_name, namelist[i]->d_name, (long long int)size, measure,last_modified);
            maxsize-=printf_s;
            pagesize+=printf_s;

        } else if (S_ISDIR(f_mode)) { //Directory entry
            //Table row for the dir
            printf_s=snprintf(html+pagesize,maxsize,
                              "<tr style=\"background-color: #DFDFDF;\"><td>d</td><td><a href=\"%s/\">%s/</a></td><td>-</td><td>%s</td></tr>\n",
                              namelist[i]->d_name, namelist[i]->d_name,last_modified);
            maxsize-=printf_s;
            pagesize+=printf_s;
        }

        free(namelist[i]);
    }

    free(namelist);

    printf_s=snprintf(html+pagesize,maxsize,"</table>%s",HTMLFOOT);
    pagesize+=printf_s;

    return pagesize;
}

/**
Prints version information
*/
void version() {
    printf(NAME " " VERSION "\n"
           "Copyright (C) 2007 Salvo 'LtWorf' Tomaselli.\n"
           "This is free software.  You may redistribute copies of it under the terms of\n"
           "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n"
           "There is NO WARRANTY, to the extent permitted by law.\n\n"

           "Written by Salvo 'LtWorf' Tomaselli and Salvo Rinaldi.\n"
           "Synchronized queue by Prof. Giuseppe Pappalardo.\n\n"
           PACKAGE_URL "\n");
    exit(0);
}

/**
 * Prints the capabilities/compile time options
 */
void capabilities() {
    printf (
        "name=" VERSION "\n"
        "version=" VERSION "\n"
        "signature=" SIGNATURE "\n"
        "port=" PORT "\n"
        "index=" INDEX "\n"
        "basedir=" BASEDIR "\n"
        "cgi-timeout=%d\n"

#ifdef IPV6
        "socket=IPv6\n"
#else
        "socket=IPv4\n"
#endif

#ifdef WEBDAV
        "webdav=true\n"
#else
        "webdav=false\n"
#endif

#ifdef SEND_MIMETYPES
        "mime=true\n"
#else
        "mime=false\n"
#endif

#ifdef EMBEDDED_AUTH
        "auth-embedded=true\n"
#else
        "auth-embedded=false\n"
#endif

#ifdef HAVE_INOTIFY_INIT
        "cache_correctness=true\n"
#else
        "cache_correctness=false\n"
#endif

#ifdef __COMPRESSION
        "compression=true\n"
#else
        "compression=false\n"
#endif

#ifdef SEND_LAST_MODIFIED_HEADER
        "last-modified=true\n"
#else
        "last-modified=false\n"
#endif

#ifdef __RANGE
        "range=true\n"
#else
        "range=false\n"
#endif
        ,SCRPT_TIMEOUT);

#ifdef MTIME_MAX_WATCH_DIRS
    printf("inotify-watch=%d\n",MTIME_MAX_WATCH_DIRS);
#endif
    exit(0);

}

/**
 * Prints command line help
 */
void help() {

    printf("\tUsage: weborf [OPTIONS]\n"
           "\tStart the weborf webserver\n\n"
#ifdef IPV6
           "\tCompiled for IPv6\n"
#else
           "\tCompiled for IPv4\n"
#endif

#ifdef WEBDAV
           "\tHas webdav support\n"
#endif

#ifdef SEND_MIMETYPES
           "\tHas MIME support\n"
#endif

#ifdef HAVE_INOTIFY_INIT
           "\tHas cache correctness support\n"
#endif

           "Default port is        %s\n"
           "Default base directory %s\n"
           "Signature used         %s\n\n", PORT,BASEDIR,SIGNATURE);

    printf("  -a, --auth         followed by absolute path of the program to handle authentication\n"
           "  -B  --capabilities shows the capabilities\n"
           "  -b, --basedir      followed by absolute path of basedir\n"
           "  -C, --cache        sets the directory to use for cache files\n"
           "  -c, --cgi          list of cgi files and binary to execute them comma-separated\n"
           "  -d                 run as a daemon\n"
           "  -f  --fastcache    makes cache less correct but faster (see manpage)\n"
           "  -h, --help         display this help and exit\n"
           "  -I, --index        list of index files, comma-separated\n"
           "  -i, --ip           followed by IP address to listen (dotted format)\n"
           "  -m, --mime         sends content type header to clients\n"
           "  -p, --port         followed by port number to listen\n"
           "  -T  --inetd        must be specified when using weborf with inetd or xinetd\n"
           "  -t  --tar          will send the directories as .tar.gz files\n"
           "  -u,                followed by a valid uid\n"
           "                     If started by root weborf will use this user to read files and execute scripts\n"
           "  -V, --virtual      list of virtualhosts in the form host=basedir, comma-separated\n"
           "  -v, --version      print program version\n"
           "  -x, --noexec       tells weborf to send each file instead of executing scripts\n\n"


           "Report bugs here https://bugs.launchpad.net/weborf\n"
           "or to " PACKAGE_BUGREPORT "\n");
    exit(0);
}

/**
Searching for easter eggs within the code isn't fair!
*/
void moo() {
    printf(" _____________________________________\n"
           "< Weborf ha i poteri della supermucca >\n"
           " -------------------------------------\n"
           "        \\   ^__^\n"
           "         \\  (oo)\\_______\n"
           "            (__)\\       )\\/\\\n"
           "                ||----w |\n"
           "                ||     ||\n");
    exit(0);
}

/**
 * This function prints the start disclaimer on stdout.
 * It wants the command line parameters
 * */
void print_start_disclaimer(char *argv[]) {
    printf(NAME "\n"
           "This program comes with ABSOLUTELY NO WARRANTY.\n"
           "This is free software, and you are welcome to redistribute it\n"
           "under certain conditions.\nFor details see the GPLv3 Licese.\n"
           "Run %s --help to see the options\n", argv[0]);
}

/**
 * Detaches the process from the shell,
 * it is re-implemented because it is not
 * included in POSIX
 *
 * It shouldn't be executed after launching
 * other threads. In that case the effects are
 * not specified.
 * */
void daemonize() {
    if (fork() == 0)
        signal(SIGHUP, SIG_IGN);
    else
        exit(0);
}

/**
 * This function generates a detached child that
 * it is not possible to wait.
 *
 * Return values as the fork (pid_t is a fake id, can't be used)
 **/
pid_t detached_fork() {
    pid_t f1 = fork();
    pid_t f2;

    if (f1==0) { //Child process
        f2=fork();
        if (f2!=0)
            exit(0);
        return 0;
    } else if (f1>0) { //Father process
        waitpid(f1,NULL,0);
        return 1;
    } else if (f1<0) {
        return f1;
    }
    return -1;
}

/**
This function retrieves the value of an http field within the header
http_param is the string containing the header
parameter is the searched parameter
buf is the buffer where copy the value
size, maximum size of the buffer
param_len =lenght of the parameter

Returns false if the parameter isn't found, or true otherwise
*/
bool get_param_value(char *http_param, char *parameter, char *buf, ssize_t size,ssize_t param_len) {
    char *val = strstr(http_param, parameter); //Locates the requested parameter information

    if (val == NULL) { //No such field
        return false;
    }

    /*
     * It is very important for this line to be here, for security reasons.
     * It moves the pointer forward, assuming "Field: Value\r\n"
     * If the field is malformed like "Field0\r\n" the subsequent strstr
     * will fail and the function will return false.
     * Moving this line after the next strstr would introduce a security
     * vulnerability.
     * The strstr will not cause a segfault because at this point the header
     * string must at least terminate with "\r\n\r", the last '\r' is changed to 0
     * so there is enough space to perform the operation
     * */
    val += param_len + 2; //Moves the begin of the string to exclude the name of the field

    char *field_end = strstr(val, "\r\n"); //Searches the end of the parameter
    if (field_end==NULL || (field_end - val + 1) >= size) {
        return false;
    }

    memcpy(buf, val, field_end - val);
    buf[field_end - val] = 0; //Ends the string within the destination buffer

    return true;
}


