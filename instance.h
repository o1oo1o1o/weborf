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
@author Salvo Rinaldi <salvin@anche.no>
 */

#ifndef WEBORF_INSTANCE_H
#define WEBORF_INSTANCE_H

#include "types.h"
#include "buffered_reader.h"

#ifdef WEBDAV
#include "webdav.h"
#endif


#define NO_ACTION -120

void inetd();
void *instance(void *);
int send_err(connection_t *connection_prop,int err,char* descr);
int send_http_header(connection_t * connection_prop);
int delete_file(connection_t* connection_prop);
int read_file(connection_t* connection_prop,buffered_read_t* read_b);
void prepare_get_file(connection_t* connection_prop);
#endif
