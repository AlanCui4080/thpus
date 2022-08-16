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
    response.h: 
*/
#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include"request.h"
#include<stddef.h>

#define SERVER_ACCEPT_RANGE 0x01
#define SERVER_KEEP_ALIVE 0x02
#define SERVER_CONTENT_LEN 0x04
#define SERVER_SHOW_NAME 0x08
#define SERVER_HAVE_PAYLOAD 0x10
#define SERVER_SHOW_ALLOW 0x20
#define SERVER_PAYLOAD_LOADED 0x40

#define SERVER_GET_DEFAULT (SERVER_ACCEPT_RANGE|SERVER_CONTENT_LEN|SERVER_SHOW_NAME)
#define SERVER_HEAD_DEFAULT (SERVER_SHOW_NAME|SERVER_ACCEPT_RANGE|SERVER_CONTENT_LEN)
#define SERVER_OPTIONS_DEFAULT (SERVER_SHOW_NAME|SERVER_SHOW_ALLOW)
#define SERVER_TRACE_DEFAULT (SERVER_HAVE_PAYLOAD|SERVER_PAYLOAD_LOADED|SERVER_CONTENT_LEN|SERVER_SHOW_NAME)

#define SERVER_FEATURE_MAX_LEN 512
#define SERVER_HEADER_MAX_LEN (HTTP_METHOD_MAX_LEN+HTTP_VERSION_MAX_LEN+8 \
    +SERVER_FEATURE_MAX_LEN)

#define FEATURE_ACCEPT_RANGE_TEXT "Accept-Ranges: bytes\r\n"
#define FEATURE_KEEP_ALIVE_TEXT "Connection: keep-alive\r\n"
#define FEATURE_CLOSED_TEXT "Connection: close\r\n"
#define FEATURE_ALLOW_TEXT "Allow: GET, OPTIONS, HEAD, TRACE\r\n"
#define FEATURE_CONTENT_LEN "Content-Length: "
#define FEATURE_SHOW_NAME "Server: "

#define HTTP_OK 200
#define HTTP_PARTICAL 206
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_RANGE_NOT_SATISFIED 416
#define HTTP_INTERNAL_ERR 500
#define HTTP_VERSION_NOT_SUPPPORT 505

struct response
{
    int http_version;
    int response_code;
    unsigned long long flag;
    char* payload_path;
    size_t payload_size;
    size_t payload_start;
    size_t payload_end;
};

#define HLINE_NR 1
struct headline_hook
{
    char* subject;
    void (*func)(struct response* res,const struct request_header* req,const struct request_line *lp);
};

extern int do_response(struct response* res);
extern int build_response(struct response* res,const struct request_header* req);

#endif //_RESPONSE_H_