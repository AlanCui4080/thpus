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
    main.h: generic profile
*/
#ifndef _MAIN_H_
#define _MAIN_H_

#include<errno.h>
#include<unistd.h>

//name in "Server:" and console output
#define SERVER_NAME "Thpus"
#define SERVER_COMPLIER_INFO __DATE__" "__TIME__
//version number control
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_NAME "Meow!"
//old-fashined version string
#define VERSION_SALT "Thpus (Experied MACRO)"

//simple list base class
struct list_head
{
    struct list_head* next;
    struct list_head* prev;
};
//push src back to the dst
#define list_put(dst,src) \
    ((struct list_head*)(dst))->next = (struct list_head*)(src); \
    ((struct list_head*)(src))->prev = (struct list_head*)(dst);
//init a list_head based class
#define list_init(v) \
    ((struct list_head*)(v))->next = NULL; \
    ((struct list_head*)(v))->prev = NULL;
//the next node
#define list_next(v) \
    ((typeof(v))(((struct list_head*)(v))->next))
//the previous node
#define list_prev(v) \
    ((typeof(v))(((struct list_head*)(v))->prev))
enum{
    LOG_DEBUG = 1,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FAULT,
};

//someting wired and junk.....
#define main_fastfail() \
    shutdown(main_fd,SHUT_RDWR); \
    close(main_fd); \
    wait(NULL); \
    _exit(EXIT_FAILURE)

//gen a log as levels 
#define slog(l,m,...) _slog(l,m,__FILE__,__LINE__,__func__,## __VA_ARGS__)

//socket fd of session
extern int session_fd;
//
extern int _slog(int level, const char *msg,
    const char* file,unsigned int line,const char* func,...);
//FASTFAIL as a session
extern void session_fastfail();
#endif //_MAIN_H_