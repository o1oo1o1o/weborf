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
@author Salvo Rinaldi <salvin@bluebottle.com>
 */

#ifndef WEBORF_INSTANCE_H
#define WEBORF_INSTANCE_H

#define _GNU_SOURCE


#ifndef O_LARGEFILE //Needed to compile on Mac, where this doesn't exist
#define O_LARGEFILE 0
#endif

#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>	//To use syslog
#include "options.h"
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>	//Adds boolean type
#include <string.h>


#define INVALID -1
#define GET 0
#define POST 1

#define ERR_NONAUTH -5
#define ERR_SOCKWRITE -4
#define ERR_NOMEM -3
#define ERR_FILENOTFOUND -2
#define ERR_BRKPIPE -1

typedef struct {
    ssize_t len; //length of the string
    char * data;//Pointer to string
} string_t;

int writeDir(int sock, char* page,char* real_basedir);
void * instance(void *);
int sendPage(int sock,char * page,char * http_param,int method_id,char * method,char* ip_addr);
int writeFile(int sock,char * strfile,char *http_param);
#ifdef __COMPRESSION
int writeCompressedFile(int sock, char*strfile,unsigned int size);
#endif
int execPage(int sock, char * file,char*strfile, char * params,char * executor,char * http_param,string_t* post_param,char * method,char* ip_addr,char* real_basedir);
int send_err(int sock,int err,char* descr,char* ip_addr);
int send_http_header(int sock,unsigned int size,char* headers);
int send_http_header_code(int sock,int code, unsigned int size,char* headers);
int send_http_header_scode(int sock,char* code, unsigned int size,char* headers);
int send_http_header_code_content(int sock,int code, unsigned int size,char* headers,bool content);
void piperr();
void modURL(char* url);
int request_auth(int sock,char* descr);
int check_auth(int sock, char* http_param, char * method, char * page, char * ip_addr);
string_t read_post_data(int sock,char* http_param,int method_id);
char* get_basedir(char* http_param);

#endif

