/*
 * Copyright © 2015-2016 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_EPOLLFD_H_
#define _JOURNALD_EPOLLFD_H_

#include <stdint.h>
#include <sys/signalfd.h>


typedef int (*event_cb)(int fd, uint32_t events, void* data);

typedef int (*signal_cb)(const struct signalfd_siginfo* si, void* data);

typedef struct event_t event_t;

typedef struct epollfd_t epollfd_t;


int epollfd_create(epollfd_t** pepoll);
void epollfd_close(epollfd_t** pepoll);

int epollfd_add(epollfd_t* epoll, int fd, uint32_t events, event_cb callback, void* data);
int epollfd_del(epollfd_t* epoll, int fd);

int epollfd_signal_add(epollfd_t* epoll, int signo, signal_cb callback, void* data);
int epollfd_signal_setup(epollfd_t* epoll);

int epollfd_run(epollfd_t* epoll);

#endif	/* _JOURNALD_EPOLLFD_H_ */
