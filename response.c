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

void phook_range(struct response *res, const struct request_header *req, const struct request_line *lp);

const void (*method_hook[METHOD_NR])(struct response *, const struct request_header *) =
    {
        hook_void,    // CONNECT
        hook_void,    // DELETE
        hook_get,     // GET
        hook_head,    // HEAD
        hook_options, // OPTIONS
        hook_void,    // PATCH
        hook_void,    // POST
        hook_void,    // PUT
        hook_trace,   // TRACE
        hook_void,    // Unknow
};
const struct headline_hook hline_hook[HLINE_NR] =
    {
        {"Range", phook_range},
};
int do_response(struct response *res)
{
    slog(LOG_DEBUG, "responsee DID");
    slog(LOG_DEBUG, "res->payload_size: %llu", res->payload_size);
    slog(LOG_DEBUG, "res->http_version: %s", HTTP_VERSION[res->http_version]);

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

    // get them together
    strcat(buf_feature, "\r\n");

    strcpy(buf_header, buf_version);
    strcat(buf_header, " ");
    strcat(buf_header, buf_code);
    strcat(buf_header, "\r\n");

    strcat(buf_header, buf_feature);

    //caculation for final buffer size
    res_size = strlen(buf_header) + res->payload_size;
    buf_res = malloc(res_size + 4);
    memset(buf_res, 0, res_size + 4);

    memcpy(buf_res, buf_header, strlen(buf_header) + 1);

    slog(LOG_DEBUG, "buf_version \n%s", buf_version);
    slog(LOG_DEBUG, "buf_code \n%s", buf_code);
    slog(LOG_DEBUG, "buf_header \n%s", buf_header);

    if (!(res->flag & SERVER_HAVE_PAYLOAD))
        goto end;
    //if payload_path actually points to a address instead of a filename
    if (res->flag & SERVER_PAYLOAD_LOADED)
    {
        strcat(buf_res, res->payload_path);
        goto end;
    }

    slog(LOG_DEBUG, "trying to open:%s", res->payload_path);
    fp = fopen(res->payload_path, "rb");
    if (fp == NULL)
        slog(LOG_FAULT, "open failed:%s", strerror(errno));

    slog(LOG_DEBUG, "trying to seek");
    if (fseek(fp, res->payload_start, SEEK_SET))
        slog(LOG_FAULT, "seek failed:%s,", strerror(errno));

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
    const struct request_line *lp = &(req->root_line);

    slog(LOG_DEBUG, "req_version: %d", req->http_version);
    slog(LOG_DEBUG, "req_method: %d", req->method);

    //version settings
    if (req->http_version != HTTP_11)
    {
        res->response_code = HTTP_VERSION_NOT_SUPPPORT;
        res->http_version = HTTP_11;
        return 0;
    }
    else
        res->http_version = req->http_version;
    //call method-specific function
    method_hook[req->method](res, req);
    //find and call hline-specific function
    slog(LOG_DEBUG, "res->payload_size: %llu", res->payload_size);
    lp = list_next(lp);
    while (lp != NULL)
    {
        for (size_t i = 0; i < HLINE_NR; i++)
        {
            slog(LOG_DEBUG, "lp next. lp.subj=%s", lp->subject);
            slog(LOG_DEBUG, "hline. hline.subj=%s", hline_hook[i].subject);
            if (!strcmp(lp->subject, hline_hook[i].subject))
            {
                slog(LOG_DEBUG, "got a fliter");
                hline_hook[i].func(res, req, lp);
            }
        }
        lp = list_next(lp);
    }
    slog(LOG_DEBUG, "no more hline fliter found");

    return 0;
}
//if the method is disabled
void hook_void(struct response *res, const struct request_header *req)
{
    res->flag = SERVER_SHOW_NAME;
    res->flag |= SERVER_SHOW_ALLOW;
    res->response_code = HTTP_METHOD_NOT_ALLOWED;
    res->payload_start = 0;
    res->payload_end = 0;
    res->payload_size = 0;
    return;
}
void hook_trace(struct response *res, const struct request_header *req)
{
    const struct request_line *lp = &(req->root_line);
    slog(LOG_WARNING, "got a TRACE request, that may cause a serious SECURITY problem");
    res->payload_size = 0;
    res->flag = SERVER_TRACE_DEFAULT;

    lp = list_next(lp);
    while (strcmp(lp->subject, "X-__TRACE_INFO"))
    {
        slog(LOG_DEBUG, "lp next. lp.subj=%s", lp->subject);
        lp = list_next(lp);
    }
    slog(LOG_DEBUG, "lp.subj=%s", lp->subject);
    slog(LOG_DEBUG, "lp.va=%s", lp->value);

    res->response_code = 200;
    res->payload_path = lp->value;
    res->payload_size = strlen(lp->value);
    res->payload_start = 0;
    res->payload_end = 0;
    return;
}
void hook_head(struct response *res, const struct request_header *req)
{
    slog(LOG_DEBUG, "hook_head");
    hook_get(res, req);
    res->flag &= ~SERVER_HAVE_PAYLOAD;
}
void hook_options(struct response *res, const struct request_header *req)
{
    slog(LOG_DEBUG, "hook_options");
    res->response_code = HTTP_OK;
    res->payload_size = 0;
    res->payload_start = 0;
    res->payload_end = 0;
    res->flag = SERVER_OPTIONS_DEFAULT;
    return;
}
void hook_get(struct response *res, const struct request_header *req)
{
    FILE *fp;
    res->flag = SERVER_GET_DEFAULT;

    slog(LOG_DEBUG, "res->flag: %llu", res->flag);
    slog(LOG_DEBUG, "res->http_version: %d", res->http_version);
    slog(LOG_DEBUG, "url:%s", req->url);

    slog(LOG_DEBUG, "trying to open:%s", req->url);

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
        return;
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
    res->payload_start = 0;
    res->payload_end = res->payload_size;

    res->flag |= SERVER_HAVE_PAYLOAD;

    return;
}
void phook_range(struct response *res, const struct request_header *req, const struct request_line *lp)
{
    /*e.g.
        Range: bytes=233-456
                    +   +
                    a   b
    */
    char *a, *b;
    size_t start, end;

    if (res->response_code != 200)
        return;

    a = strchr(lp->value, '=');
    *a = '\0';
    if (strcmp("bytes", lp->value))
        goto fail;

    b = strchr(a + 1, '-');
    *b = '\0';
    start = atoll(a + 1);

    if (*(b + 1) == '\r')
    {
        slog(LOG_DEBUG, "a type");
        slog(LOG_DEBUG, "a:%s", a);

        res->payload_start = start;
        res->payload_end = res->payload_size;
        res->payload_size = res->payload_end - res->payload_start;
        goto succeed;
    }
    else
    {
        slog(LOG_DEBUG, "ab type");
        slog(LOG_DEBUG, "a:%s", a);
        slog(LOG_DEBUG, "b:%s", b);

        end = atoll(b + 1);
        if (end > res->payload_size)
            goto fail;
        res->payload_start = start;
        res->payload_end = end;
        res->payload_size = res->payload_end - res->payload_start;
        goto succeed;
    }
    if (strchr(b, ',') != NULL)
        goto fail;

succeed:
    slog(LOG_DEBUG, "range start:%llu", start);
    slog(LOG_DEBUG, "range end:%llu", end);
    res->response_code = HTTP_PARTICAL;
    return;
fail:
    res->response_code = HTTP_RANGE_NOT_SATISFIED;
    res->flag &= ~SERVER_HAVE_PAYLOAD;
    return;
}