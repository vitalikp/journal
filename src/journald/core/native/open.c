/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <sys/epoll.h>
#include <sys/socket.h>

#include "core/native.h"
#include "core/socket.h"
#include "log.h"


int native_open(server_t *s, event_cb callback)
{
	s->native_fd = socket_open(JOURNAL_RUNDIR "/socket", SOCK_DGRAM);
	if (s->native_fd < 0)
		return -1;

	if (epollfd_add(s->epoll, s->native_fd, EPOLLIN, callback, s) < 0)
	{
		log_error("Failed to add native server fd to event loop: %m");
		return -1;
	}

	return 0;
}