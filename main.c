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
    main.c
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "response.h"
#include "request.h"

//the file description of session socket
int session_fd = 0;
//the file description which used by main proc
int main_fd = 0;

//session quit
void session_fastfail()
{
    slog(LOG_DEBUG, "quiting.");

    if(!session_fd)
        main_fastfail();

    slog(LOG_DEBUG, "trying to shutdown fd:%d", session_fd);
    if (shutdown(session_fd, SHUT_RDWR))
        slog(LOG_ERROR, "shutdown failed:%s", strerror(errno));

    slog(LOG_DEBUG, "trying to close fd:%d", session_fd);
    if (close(session_fd))
        slog(LOG_ERROR, "close failed:%s", strerror(errno));

    slog(LOG_DEBUG, "ready to _exit()");
    _exit(EXIT_FAILURE);
}

//log (internal) use macro log directly plz
int _slog(int level, const char *msg,
          const char *file, unsigned int line, const char *func, ...)
{
//if not debug, donnot to print debug level infomation
#ifndef DEBUG
    if (level == LOG_DEBUG)
        return 0;
#endif
    //format the string 
    va_list va;
    va_start(va, func);
    char buf_log[256] = {0};
    vsprintf(buf_log, msg, va);
//if debug and the program thrown a error then report a source
#ifdef DEBUG
    switch (level)
    {
    case LOG_DEBUG:
    case LOG_INFO:
        break;
    default:
        fprintf(stderr, "in ");
        fprintf(stderr, func);
        fprintf(stderr, "(%s:%d)", file, line);
        fprintf(stderr, ": \n");
        break;
    }
#endif
    //accoring to level, choose output and char
    switch (level)
    {
    case LOG_DEBUG:
        printf("[D] Session %d:", getpid());
        printf(buf_log);
        printf("\n");
        break;
    case LOG_INFO:
        printf("[I] Session %d:", getpid());
        printf(buf_log);
        printf("\n");
        break;
    case LOG_WARNING:
        fprintf(stderr, "[W] Session %d:", getpid());
        fprintf(stderr, buf_log);
        fprintf(stderr, "\n");
        break;
    case LOG_ERROR:
        fprintf(stderr, "[E] Session %d:", getpid());
        fprintf(stderr, buf_log);
        fprintf(stderr, "\n");
        break;
    case LOG_FAULT:
        fprintf(stderr, "[!] Session %d:", getpid());
        fprintf(stderr, buf_log);
        fprintf(stderr, "\n");
        session_fastfail();
        break;
    default:
        break;
    }
}
void session_main(int fd)
{
    char buf_req[1024] = {0};
    char buf_info[1024] = {0};
    struct request_header req;
    struct response res;

    if (recv(fd, buf_req, 1024, 0) < 0)
    {
        slog(LOG_FAULT, "request header recv() failed:%s", strerror(errno));
    }
    analyse_request_startline(&req, buf_req);
    analyse_request_line(&req, buf_req);
    build_response(&res, &req);
    do_response(&res);
    return;
}
//to catch SIGINT and close socket safely
void sig_catch()
{
    slog(LOG_WARNING, "Cought ^C. Exiting.");
    main_fastfail();
}
int main()
{
    signal(SIGINT, sig_catch);

    struct sockaddr_in main_addr =
        {
            .sin_family = AF_INET,
            .sin_port = htons(8888),
            .sin_addr.s_addr = inet_addr("0.0.0.0")};

    printf("Thpus WebServer\n");
    printf("Version:" VERSION_SALT "\n\n");

    main_fd = socket(AF_INET, SOCK_STREAM, 0);
    slog(LOG_DEBUG, "creating main listening socket fd:%d", main_fd);
    if (main_fd < 0)
        slog(LOG_FAULT, "create main lisening socket failed:%s", strerror(errno));

    slog(LOG_DEBUG, "trying to bind main lisening socket to localhost:8888");
    if (bind(main_fd, (struct sockaddr *)&main_addr, sizeof(struct sockaddr_in)))
        slog(LOG_FAULT, "bind failed:%s", strerror(errno));

    slog(LOG_DEBUG, "starting listening ");
    if (listen(main_fd, 16))
        slog(LOG_FAULT, "start listening failed:%s", strerror(errno));

    slog(LOG_INFO, "ready");
    while (1)
    {
        struct sockaddr_in session_addr;
        int session_addr_size = sizeof(struct sockaddr_in);
        int rtn_fork = 0;
        int failback = 0;

        session_fd = accept(main_fd,
                            (struct sockaddr *)&session_addr,
                            &session_addr_size);
    failback:
        rtn_fork = fork();
        slog(LOG_DEBUG, "forked sub_session pid:%d", rtn_fork);
        if (rtn_fork < 0)
        {
            slog(LOG_ERROR, "fork sub_session failed:%s", strerror(errno));
            if (failback < 5)
            {
                slog(LOG_INFO, "trying to failback:%d(times)", failback);
                failback++;
                goto failback;
            }
        }

        if (rtn_fork == 0)
        {
            //print source address and port
            slog(LOG_INFO, "%u.%u.%u.%u:%u", 
                (ntohl(session_addr.sin_addr.s_addr) & 0xff000000) >> 24,
                (ntohl(session_addr.sin_addr.s_addr) & 0x00ff0000) >> 16,
                (ntohl(session_addr.sin_addr.s_addr) & 0x0000ff00) >> 8,
                (ntohl(session_addr.sin_addr.s_addr) & 0x000000ff),
                (unsigned)ntohs(session_addr.sin_port));
            session_main(session_fd);
            session_fastfail();
        }
        slog(LOG_DEBUG, "send a non blocked wait4");
        if (waitpid((pid_t)-1, NULL, WNOHANG) < 0)
            slog(LOG_WARNING, "wait4 failed:%s, that may cause a perfermance problem",
                 strerror(errno));
    }
}
