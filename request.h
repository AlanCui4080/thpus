/*
    This file is a part of Thpus.
    Copyright (C) 2020-2022 AlanCui4080

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>
*/
/*
    request.h: 
*/
#ifndef _REQUEST_H_
#define _REQUEST_H_

#include"main.h"

//the longest url server can detect, also the size of buffer
#define HTTP_URL_MAX_LEN 256
//the longest method server can detect, also the size of buffer
#define HTTP_METHOD_MAX_LEN 16
//the longest http version server can detect, also the size of buffer
#define HTTP_VERSION_MAX_LEN 16
#define HTTP_STARTLINE_MAX_LEN (HTTP_METHOD_MAX_LEN+ \
     HTTP_URL_MAX_LEN+ \
     HTTP_VERSION_MAX_LEN+2)
//a line of http header
struct request_line
{
    struct list_head list;
    char* subject;
    char* value;
};
//header of request
struct request_header
{
    int method;
    char* url;
    int http_version;
    struct request_line root_line;
};
//method of http
enum {
    METHOD_CONNECT = 0,
    METHOD_DELETE,
    METHOD_GET,
    METHOD_HEAD,
    METHOD_OPTIONS,
    METHOD_PATCH,
    METHOD_POST,
    METHOD_PUT,
    METHOD_TRACE,
    METHOD_UNKNOW
};
#define METHOD_NR 10
//method of version
enum {
    HTTP_10 = 0,
    HTTP_11,
    HTTP_2,
    HTTP_UNKNOW
};
extern int analyse_request_startline(struct request_header* req,const char* header);
extern int analyse_request_line(const struct request_header* req,const char* header);

#define HTTP_METHOD_NR 9
extern const char* HTTP_METHOD[HTTP_METHOD_NR];

#define HTTP_VERSION_NR 3
extern const char* HTTP_VERSION[HTTP_VERSION_NR];
#endif //_REQUEST_H_