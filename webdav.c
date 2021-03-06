/*
Weborf
Copyright (C) 2009  Salvo "LtWorf" Tomaselli

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

#ifdef WEBDAV

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "webdav.h"
#include "http.h"
#include "instance.h"
#include "mime.h"
#include "myio.h"
#include "mystring.h"
#include "utils.h"
#include "cachedir.h"

typedef struct {
    bool getetag :1;
    bool getcontentlength :1;
    bool resourcetype :1;
    bool getlastmodified :1;
    bool getcontenttype :1;
    bool deep :1;
    unsigned type :2;
} t_dav_details;

typedef union {
    unsigned int int_version;
    t_dav_details dav_details;
} u_dav_details;


extern weborf_configuration_t weborf_conf;
extern pthread_key_t thread_key;            //key for pthread_setspecific


/**
This function will create a copy of the source URI into the
dest buffer %-escaping it.

dest_size specifies the size of the dest buffer
*/
static inline void escape_uri(char *source, char *dest, int dest_size) {
    int i;

    //dest_size must have at least 4 bytes to contain %00\0
    for (i=0; source[i]!=0 && dest_size>=4; i++) {

        //The or with the space changes it to lower case, so there is no need to compare with two ranges
        if (((source[i] | ' ')>='a' && (source[i] | ' ')<='z' ) || (source[i]>='-' && source[i]<='9')) {
            dest[0]=source[i];
            dest[1]='\0';
            dest++;
            dest_size--;
        } else {
            //Prints % followed by 2 digits hex code of the character
            sprintf(dest,"%%%02x",source[i]);
            dest+=3;
            dest_size-=3;
        }
    }
}


/**
This function will use the result param prop to return the required
props and the value of the Depth header

Properties are in the form <D:prop><prop1/><prop2/></D:prop>
If something unexpected happens during the xml parsing, ERR_NODATA
will be returned.

RFC-1518 requires that the xml must be validated and an error MUST
be returned to the client if the xml is not valid. This is not the case
in this funcion. It will accept many forms of invalid xml.

The original string post_param->data will be modified.

Returns 0 in case of success or the HTTP code error in case of failure
*/
static inline int get_props(connection_t* connection_prop,u_dav_details *props) {

    props->dav_details.deep = http_read_deep(connection_prop);

    char *sprops[MAXPROPCOUNT];   //List of pointers to properties

    if (connection_prop->post_data.len==0) {//No specific prop request, sending everything
        props->dav_details.getetag=true;
        props->dav_details.getcontentlength=true;
        props->dav_details.resourcetype=true;
        props->dav_details.getlastmodified=true;
        // props->getcontenttype=false; Commented because redoundant

        return 0;
    }

//Locates the starting prop tag
    char*post=connection_prop->post_data.data;
    char*data=strstr(post,"<D:prop ");
    if (data==NULL)
        data=strstr(post,"<D:prop>");
    if (data==NULL)
        data=strstr(post,"<prop ");
    if (data==NULL)
        data=strstr(post,"<prop>");

    if (data==NULL) {
        return HTTP_CODE_BAD_REQUEST;
    }
    data+=6; //Eliminates the 1st useless tag

    {
        //Locates the ending prop tag
        char*end=strstr(data,"</D:prop>");
        if (end==NULL)
            end=strstr(data,"</prop>");

        if (end==NULL) {
            return HTTP_CODE_BAD_REQUEST;
        }
        end[0]=0;
    }
    int i;
    char *temp, *p_temp;
    for (i=0; (sprops[i]=strstr(data,"<"))!=NULL; i++,data=temp+1) {
        if (i==MAXPROPCOUNT-1) {//Reached limit
            sprops[i]=NULL;
            break;
        }
        sprops[i]+=1; //Removes the < stuff

        //Removes the />
        temp=strstr(sprops[i],"/>");
        if (temp==NULL) return HTTP_CODE_BAD_REQUEST;
        temp[0]=0;

        //Removing if there are parameters to the node
        p_temp=strstr(sprops[i]," ");
        if (p_temp!=NULL) {
            p_temp[0]=0;
        }
    }


    for (i=0; sprops[i]!=NULL; i++) {
        if (strstr(sprops[i],"getetag")!=NULL) {
            props->dav_details.getetag=true;
        } else if (strstr(sprops[i],"getcontentlength")!=NULL) {
            props->dav_details.getcontentlength=true;
        } else if (strstr(sprops[i],"resourcetype")!=NULL) {
            props->dav_details.resourcetype=true;
        } else if (strstr(sprops[i],"getlastmodified")!=NULL) { //Sends Date
            props->dav_details.getlastmodified=true;
#ifdef SEND_MIMETYPES
        } else if(strstr(sprops[i],"getcontenttype")!=NULL) { //Sends MIME type
            props->dav_details.getcontenttype=true;
#endif
        }
    }
    return 0;
}

/**
This function sends a xml property to the client.
It can be called only by funcions aware of this xml, because it sends only partial xml.

props:
file:       file to open
filename:
parent:

If the file can't be opened in readonly mode, this function does nothing.

Return value: 0 in case there were no errors in opening the file. Do not expect
too much error checking from this function.
*/
static inline int printprops(connection_t *connection_prop,u_dav_details props,char* file,char*filename,bool parent,int sock) {
    int p_len;
    struct stat stat_s;

    int file_fd=open(file,O_RDONLY);
    if (file_fd==-1) return 1;
    fstat(file_fd, &stat_s);

    dprintf(sock,"<D:response>\n");

    {
        char escaped_filename[URI_LEN];
        escape_uri(filename,escaped_filename,URI_LEN);
        //Sends href of the resource
        if (parent) {
            dprintf(sock,"<D:href>%s</D:href>",escaped_filename);
        } else {
            char escaped_page[URI_LEN];
            escape_uri(connection_prop->page,escaped_page,URI_LEN);
            dprintf(sock,"<D:href>%s%s</D:href>",escaped_page,escaped_filename);
        }
    }

    dprintf(sock,"<D:propstat><D:prop>");

    //Writing properties

    if (props.dav_details.getetag) {
        dprintf(sock,"<D:getetag>%ld</D:getetag>\n",(long int)stat_s.st_mtime);
    }

    if (props.dav_details.getcontentlength) {
        dprintf(sock,"<D:getcontentlength>%llu</D:getcontentlength>\n",(long long unsigned int)stat_s.st_size);
    }

    if (props.dav_details.resourcetype) {//Directory or normal file
        char *out;
        if (S_ISDIR(stat_s.st_mode)) {
            out="<D:collection/>";
        } else {
            out=" ";
        }

        dprintf(sock,"<D:resourcetype>%s</D:resourcetype>\n",out);
    }

    if (props.dav_details.getlastmodified) { //Sends Date
        char buffer[URI_LEN];
        struct tm ts;
        localtime_r(&stat_s.st_mtime,&ts);
        p_len=strftime(buffer,URI_LEN, "<D:getlastmodified>%a, %d %b %Y %H:%M:%S GMT</D:getlastmodified>", &ts);
        write (sock,buffer,p_len);
    }


#ifdef SEND_MIMETYPES
    if(props.dav_details.getcontenttype) { //Sends MIME type
        thread_prop_t *thread_prop = pthread_getspecific(thread_key);

        const char* t=mime_get_fd(thread_prop->mime_token,file_fd,&stat_s);
        dprintf(sock,"<D:getcontenttype>%s</D:getcontenttype>\n",t);
    }
#endif


    dprintf(sock,"</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat>");

    dprintf(sock,"</D:response>");

    close(file_fd);
    return 0;
}

/**
This function serves a PROPFIND request.

Can serve both depth and non-depth requests. This funcion works only if
authentication is enabled.

RETURNS
true if an header was sent TODO
*/
void prepare_propfind(connection_t* connection_prop) {

    u_dav_details props= {0};
    props.dav_details.type=1; //I need to avoid the struct to be fully 0 in each case

    {
        //TODO this code is duplicated.. should be possible to collapse it in only one place

        //This redirects directory without ending / to directory with the ending /
        int stat_r=stat(connection_prop->strfile, &connection_prop->strfile_stat);

        if (stat_r!=0) {
            connection_prop->response.status_code = HTTP_CODE_PAGE_NOT_FOUND;
            connection_prop->status = STATUS_ERR;
            return;
        }

        if (S_ISDIR(connection_prop->strfile_stat.st_mode) && !endsWith(connection_prop->strfile,"/",connection_prop->strfile_len,1)) {//Putting the ending / and redirect
            http_append_header_str(connection_prop,"Location: %s/\r\n",connection_prop->page);
            connection_prop->response.status_code=HTTP_CODE_MOVED_PERMANENTLY;
            connection_prop->status = STATUS_ERR;
            return;
        }
    } // End redirection

    int result_fd;
    int retval=get_props(connection_prop,&props);//splitting props
    if (retval!=0) {
        connection_prop->response.status_code=retval;
        connection_prop->status = STATUS_ERR;
        return;
    }

    //Sets keep alive to false (have no clue about how big is the generated xml) and sends a multistatus header code
    connection_prop->response.keep_alive=false; //TODO no longer true
    connection_prop->response.status_code=WEBDAV_CODE_MULTISTATUS;
    http_append_header(connection_prop,"Content-Type: text/xml; charset=\"utf-8\"\r\n");


    //Check if exists in cache
    if (cache_is_enabled()) {
        if (cache_send_item(props.int_version,connection_prop))
            return;

        result_fd=cache_get_item_fd_wr(props.int_version,connection_prop);

    } else  {
        result_fd = myio_mktmp();
    }

    /////////////////////////////////GENERATES THE RESPONSE

    //Sends header of xml response
    dprintf(result_fd,"<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
    dprintf(result_fd,"<D:multistatus xmlns:D=\"DAV:\">");

    //sends props about the requested file
    printprops(connection_prop,props,connection_prop->strfile,connection_prop->page,true,result_fd);
    if (props.dav_details.deep && S_ISDIR(connection_prop->strfile_stat.st_mode)) {//Send children files
        DIR *dp = opendir(connection_prop->strfile); //Open dir
        //TODO check result
        char file[URI_LEN];
        struct dirent entry;
        struct dirent *result;
        int return_code;

        if (dp == NULL) {//Error, unable to send because header was already sent
            //FIXME
        }

        for (return_code=readdir_r(dp,&entry,&result); result!=NULL && return_code==0; return_code=readdir_r(dp,&entry,&result)) { //Cycles trough dir's elements
#ifdef HIDE_HIDDEN_FILES
            if (entry.d_name[0]=='.') //doesn't list hidden files
                continue;
#else
            //Avoids dir . and .. but not all hidden files
            if (entry.d_name[0]=='.' && (entry.d_name[1]==0 || (entry.d_name[1]=='.' && entry.d_name[2]==0)))
                continue;
#endif

            snprintf(file,URI_LEN,"%s%s", connection_prop->strfile, entry.d_name);

            //Sends details about a file
            printprops(connection_prop,props,file,entry.d_name,false,result_fd);
        }

        closedir(dp);
    }

    //ends multistatus
    dprintf(result_fd,"</D:multistatus>");


    ////////////////// ACTUALLY SENDING THE FILE
    lseek(result_fd, 0, SEEK_SET); //Reset the file so it can be read again

    if (connection_prop->strfile_fd!=-1) printf ("ERROR in webdav.c, file descriptor leak\n"); //TODO check correctness
    //close(connection_prop->strfile_fd);
    connection_prop->strfile_fd=result_fd;
    fstat(connection_prop->strfile_fd, &connection_prop->strfile_stat);
    prepare_get_file(connection_prop);

    return;
}

/**
This funcion should be named mkdir. But standards writers are weird people.

Returns 0 if the directory was created.
Never sends anything
*/
int mkcol(connection_t* connection_prop) {
    connection_prop->status = STATUS_ERR;

    int res=mkdir(connection_prop->strfile,S_IRWXU | S_IRWXG | S_IRWXO);

    if (res==0) {//Directory created
        connection_prop->response.status_code = HTTP_CODE_OK_CREATED;
        return 0;
    }

    //Error
    switch (errno) {
    case EACCES:
    case EFAULT:
    case ELOOP:
    case ENAMETOOLONG:
    case EEXIST:
    case ENOTDIR:
        connection_prop->response.status_code = HTTP_CODE_FORBIDDEN;
        break;
    case ENOMEM:
        connection_prop->response.status_code = HTTP_CODE_SERVICE_UNAVAILABLE;
        break;
    case ENOENT:
        connection_prop->response.status_code = HTTP_CODE_CONFLICT;
        break;
    case ENOSPC:
    case EROFS:
    case EPERM:
        connection_prop->response.status_code = HTTP_CODE_INSUFFICIENT_STORAGE;
        break;
    }

    return -1;
}

/**
Webdav method copy.
*/
void copy_move(connection_t* connection_prop) {
    struct stat f_prop; //File's property
    bool check_exists=false;
    int retval=0;
    bool exists;

    connection_prop->status = STATUS_ERR;

    char* host=malloc(3*PATH_LEN+12);
    if (host==NULL) {
        connection_prop->response.status_code = HTTP_CODE_SERVICE_UNAVAILABLE;
        return;
    }
    char* dest=host+PATH_LEN;
    char* overwrite=dest+PATH_LEN;
    char* destination=overwrite+2;

    //If the file has the same date, there is no need of sending it again
    bool host_b=get_param_value(connection_prop->http_param,"Host",host,PATH_LEN,strlen("Host"));
    bool dest_b=get_param_value(connection_prop->http_param,"Destination",dest,PATH_LEN,strlen("Destination"));
    bool overwrite_b=get_param_value(connection_prop->http_param,"Overwrite",overwrite,PATH_LEN,strlen("Overwrite"));

    if (host_b && dest_b == false) { //Some important header is missing
        connection_prop->response.status_code = HTTP_CODE_BAD_REQUEST;
        goto escape;
    }

    /*Sets if there is overwrite or not.
    ovewrite header is a boolean where F is false.
    */
    if (overwrite_b) {
        check_exists=(overwrite[0]!='F');
    }

    dest=strstr(dest,host);
    if (dest==NULL) {//Something is wrong here
        retval = connection_prop->response.status_code = HTTP_CODE_BAD_REQUEST;
        goto escape;
    }
    dest+=strlen(host);

    //Local path for destination file
    snprintf(destination,PATH_LEN,"%s%s",connection_prop->basedir,dest);

    if (strcmp(connection_prop->strfile,destination)==0) {//same
        retval = connection_prop->response.status_code = HTTP_CODE_FORBIDDEN;
        goto escape;
    }
    exists=(access(destination,R_OK)==0?true:false);

    //Checks if the file already exists
    if (check_exists && exists) {
        connection_prop->response.status_code = HTTP_CODE_PRECONDITION_FAILED;
        goto escape;
    }

    stat(connection_prop->strfile, &f_prop);
    if (S_ISDIR(f_prop.st_mode)) { //Directory
        if (connection_prop->request.method_id==COPY) {
            retval=dir_copy(connection_prop->strfile,destination);
        } else {//Move
            retval=dir_move(connection_prop->strfile,destination);
        }
    } else { //Normal file
        if (connection_prop->request.method_id==COPY) {
            retval=file_copy(connection_prop->strfile,destination);
        } else {//Move
            retval=file_move(connection_prop->strfile,destination);
        }
    }

    if (retval==0) {
        connection_prop->response.status_code = exists? HTTP_CODE_OK_NO_CONTENT:HTTP_CODE_OK_CREATED;
    }
escape:
    free(host);
    return;
}

#endif
