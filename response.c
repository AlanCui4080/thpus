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
    response.c:
*/
#include "response.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void hook_void(struct response *res, const struct request_header *req);
void hook_get(struct response *res, const struct request_header *req);
void hook_head(struct response *res, const struct request_header *req);
void hook_options(struct response *res, const struct request_header *req);
void hook_trace(struct response *res, const struct request_header *req);

const void (*method_hook[METHOD_NR])(struct response*, const struct request_header*) =\
    {
        hook_void, //CONNECT
        hook_void, //DELETE
        hook_get, //GET
        hook_head, //HEAD
        hook_options, //OPTIONS
        hook_void, //PATCH
        hook_void, //POST
        hook_void, //PUT
        hook_trace, //TRACE
        hook_void, //Unknow
    };
int do_response(struct response *res)
{
    slog(LOG_DEBUG, "responsee DID");
    char buf_version[HTTP_VERSION_MAX_LEN] = {0};
    char buf_code[4] = {0};

    char buf_feature[SERVER_FEATURE_MAX_LEN] = {0};
    char buf_header[SERVER_HEADER_MAX_LEN] = {0};

    size_t res_size;

    char *buf_res;

    FILE *fp;

    // version
    strcpy(buf_version, HTTP_VERSION[res->http_version]);

    // code
    sprintf(buf_code, "%d", res->response_code);

    // feature lines
    if (res->flag & SERVER_ACCEPT_RANGE)
        strcat(buf_feature, FEATURE_ACCEPT_RANGE_TEXT);

    if (res->flag & SERVER_KEEP_ALIVE)
        strcat(buf_feature, FEATURE_KEEP_ALIVE_TEXT);
    else
        strcat(buf_feature, FEATURE_CLOSED_TEXT);

    if (res->flag & SERVER_CONTENT_LEN)
        sprintf(buf_feature + strlen(buf_feature), FEATURE_CONTENT_LEN "%llu\r\n", res->payload_size);

    if (res->flag & SERVER_SHOW_NAME)
        sprintf(buf_feature + strlen(buf_feature), "Server: " SERVER_NAME ":%d.%d-%s\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_NAME);

    if (res->flag & SERVER_SHOW_ALLOW)
        strcat(buf_feature, FEATURE_ALLOW_TEXT);

    strcat(buf_feature, "\r\n");

    strcpy(buf_header, buf_version);
    strcat(buf_header, " ");
    strcat(buf_header, buf_code);
    strcat(buf_header, "\r\n");

    strcat(buf_header, buf_feature);
    strcat(buf_header, "\r\n\r\n");

    res_size = strlen(buf_header) + res->payload_size;
    buf_res = malloc(res_size + 4);
    memset(buf_res, 0, res_size);

    memcpy(buf_res, buf_header, strlen(buf_header) + 1);

    slog(LOG_DEBUG, "buf_header \n%s", buf_header);

    if (!(res->flag & SERVER_HAVE_PAYLOAD))
        goto end;

    if (res->flag & SERVER_PAYLOAD_LOADED)
    {
        strcat(buf_res,res->payload_path);
        goto end;
    }

    slog(LOG_DEBUG, "trying to open:%s", res->payload_path);
    fp = fopen(res->payload_path, "rb");
    if (fp == NULL)
        slog(LOG_FAULT, "open failed:%s", strerror(errno));

    slog(LOG_DEBUG, "trying to read");

    fread(buf_res + strlen(buf_header), res->payload_size, 1, fp);
    if (ferror(fp))
        slog(LOG_FAULT, "read failed:%s", strerror(errno));

    slog(LOG_DEBUG, "trying to close");
    if (fclose(fp))
        slog(LOG_ERROR, "close failed:%s, that may cause a perfermance problem", strerror(errno));
    slog(LOG_DEBUG, "file operating end, ready to send");
end:
    if (send(session_fd, buf_res, res_size, 0) < 0)
        slog(LOG_FAULT, "send failed:%s", strerror(errno));

    return 0;
}
int build_response(struct response *res, const struct request_header *req)
{
    slog(LOG_DEBUG, "req_version: %d", req->http_version);
    slog(LOG_DEBUG, "req_method: %d", req->method);

    if (req->http_version != HTTP_11)
    {
        res->response_code = HTTP_VERSION_NOT_SUPPPORT;
        res->http_version = HTTP_11;
        return 0;
    }
    else
        res->http_version = req->http_version;

    method_hook[req->method](res,req);
    
    return 0;
}
void hook_void(struct response *res, const struct request_header *req)
{
    res->response_code = HTTP_METHOD_NOT_ALLOWED;
    return ; 
}
void hook_trace(struct response *res, const struct request_header *req)
{
    const struct request_line *lp = &(req->root_line);
    slog(LOG_WARNING,"got a TRACE request, that may cause a serious SECURITY problem");
    res->payload_size = 0;
    res->flag = SERVER_TRACE_DEFAULT;

    lp = list_next(lp);
    while (strcmp(lp->subject,"X-__TRACE_INFO"))
    {
        slog(LOG_DEBUG,"lp next. lp.subj=%s",lp->subject);
        lp = list_next(lp);
    }
    slog(LOG_DEBUG,"lp.subj=%s",lp->subject);
    slog(LOG_DEBUG,"lp.va=%s",lp->value);
    res->payload_path = lp->value;
    res->payload_size = strlen(lp->value);
    return;
}
void hook_head(struct response *res, const struct request_header *req)
{
    hook_get(res,req);
    res->payload_size = 0;
    res->flag &= ~SERVER_HAVE_PAYLOAD;
}
void hook_options(struct response *res, const struct request_header *req)
{
    res->response_code = HTTP_OK;
    res->payload_size = 0;
    res->flag = SERVER_OPTIONS_DEFAULT;
    return;
}
void hook_get(struct response *res, const struct request_header *req)
{
    FILE *fp;

    slog(LOG_DEBUG, "res->flag: %llu", res->flag);
    slog(LOG_DEBUG, "res->http_version: %d", res->http_version);
    slog(LOG_DEBUG, "url:%s", req->url);

    slog(LOG_DEBUG, "trying to open:%s", req->url);

    res->flag = SERVER_GET_DEFAULT;

    fp = fopen(req->url + 1, "rb");
    if (fp == NULL)
    {
        slog(LOG_DEBUG, "fp==NULL %llu", fp);
        switch (errno)
        {
        case EACCES:
            res->response_code = HTTP_FORBIDDEN;
            break;
        case ENOENT:
            res->response_code = HTTP_NOT_FOUND;
            break;
        default:
            res->response_code = HTTP_INTERNAL_ERR;
            break;
        }
        slog(LOG_DEBUG, "res->response_code: %d", res->response_code);
        res->payload_size = 0;
        return ;
    }
    res->response_code = HTTP_OK;
    slog(LOG_DEBUG, "res->response_code: %d", res->response_code);

    slog(LOG_DEBUG, "trying to seek");
    if (fseek(fp, 0, SEEK_END) < 0)
        slog(LOG_FAULT, "cannot move fp:%s", strerror(errno));

    slog(LOG_DEBUG, "trying to tell");
    res->payload_size = ftell(fp);
    if (res->payload_size < 0)
        slog(LOG_FAULT, "cannot get fp:%s", strerror(errno));

    slog(LOG_DEBUG, "trying to close");
    if (fclose(fp))
        slog(LOG_ERROR, "close failed:%s, that may cause a perfermance problem", strerror(errno));
    slog(LOG_DEBUG, "go ahead");

    res->payload_path = req->url + 1;
    
    res->flag |= SERVER_HAVE_PAYLOAD;

    return ;
}