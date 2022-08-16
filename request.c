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
    request.c: process with request
*/
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "request.h"
#include "main.h"
const char *HTTP_METHOD[HTTP_METHOD_NR] =
    {
        "CONNECT",
        "DELETE",
        "GET",
        "HEAD",
        "OPTIONS",
        "PATCH",
        "POST",
        "PUT",
        "TRACE"
    };

const char *HTTP_VERSION[HTTP_VERSION_NR] =
    {
        "HTTP/1.0",
        "HTTP/1.1",
        "HTTP/2"
    };
const char *seek_line_crlf(const char *src, size_t n)
{
    size_t p = 0;
back:
    while (src[p] != '\r' && p < n)
        p++;
    p++;
    if (src[p] != '\n')
        goto back;
    return &(src[p]);
}
int analyse_request_line(const struct request_header *req, const char *header)
{
    const char *p1, *p2;
    const struct request_line *lp = &(req->root_line);
    list_init(lp);
    p1 = seek_line_crlf(header, HTTP_STARTLINE_MAX_LEN);
    p1++;
    do
    {
        char *buf_line;
        p2 = seek_line_crlf(p1, HTTP_STARTLINE_MAX_LEN);

        buf_line = malloc((unsigned long long)p2 - (unsigned long long)p1 + 2);
        memcpy(buf_line, p1, (unsigned long long)p2 - (unsigned long long)p1);
        buf_line[(unsigned long long)p2 - (unsigned long long)p1 + 1] = '\0';

        slog(LOG_DEBUG, "analyze_raw:%s", buf_line);

        struct request_line *line = malloc(sizeof(struct request_line));
        list_init(line);

        line->value = memchr(buf_line, ':', p2 - p1 + 1) + 2;
        *(char *)memchr(buf_line, ':', p2 - p1 + 1) = '\0'; // cut off
        line->subject = buf_line;

        list_put(lp, line);

        lp = line;
        p1 = p2 + 1;

        slog(LOG_DEBUG, "analyze_subj:%s", line->subject);
        slog(LOG_DEBUG, "analyze_va:%s", line->value);
    } while (*p1 != '\r');
    if(req->method == METHOD_TRACE)
    {
        struct request_line* trace = malloc(sizeof(struct request_line));
        list_init(trace);

        trace->subject = "X-__TRACE_INFO";
        trace->value = malloc(strlen(header)+1);
        strcpy(trace->value,header);

        list_put(lp,trace);
        slog(LOG_DEBUG,"added X-__TRACE_INFO");
    }
    slog(LOG_DEBUG, "analyze_done");

    return 0;
}
// analyse the startline of request header and set basic info in req
int analyse_request_startline(struct request_header *req, const char *header)
{
    size_t i = 0;
    size_t l = 0;
    size_t j = 0;
    char buf_method[HTTP_METHOD_MAX_LEN] = {0};
    char buf_url[HTTP_URL_MAX_LEN] = {0};
    char buf_version[HTTP_VERSION_MAX_LEN] = {0};
    // method
    for (; header[i] != ' '; i++)
    {
        buf_method[i] = header[i];
        if (i >= HTTP_METHOD_MAX_LEN)
            slog(LOG_FAULT, "method too long");
    }
    i++;
    // URL
    l = i;
    for (; header[l] != ' '; l++)
    {
        buf_url[l - i] = header[l];
        if ((l - i) >= HTTP_URL_MAX_LEN)
            slog(LOG_FAULT, "url too long");
    }
    l++;
    // Version
    j = l;
    for (; header[j] != '\r'; j++)
    {
        buf_version[j - l] = header[j];
        if ((j - l) >= HTTP_VERSION_MAX_LEN)
            slog(LOG_FAULT, "version too long");
    }
    j += 2;
    if (strcmp(buf_url, "/"))
    {
        req->url = malloc(strlen(buf_url) + 1);
        strcpy(req->url, buf_url);
    }
    else
    {
        req->url = malloc(strlen("/index.html") + 1);
        strcpy(req->url, "/index.html");
    }

    for (size_t i = 0; i < HTTP_METHOD_NR; i++)
    {
        if (!strcmp(HTTP_METHOD[i], buf_method))
        {
            req->method = i;
            goto got_method;
        }
    }
    req->method = METHOD_UNKNOW;
    slog(LOG_WARNING, "method unknow");
got_method:
    for (size_t i = 0; i < HTTP_VERSION_NR; i++)
    {
        if (!strcmp(HTTP_VERSION[i], buf_version))
        {
            req->http_version = i;
            return 0;
        }
    }
    req->method = HTTP_UNKNOW;
    slog(LOG_WARNING, "version unknow");
}
