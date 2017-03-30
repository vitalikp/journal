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

#include "core/syslog.h"
#include "core/socket.h"
#include "log.h"


int syslog_open(server_t *s, event_cb callback)
{
	s->syslog_fd = socket_open(JOURNAL_RUNDIR "/devlog", SOCK_DGRAM);
	if (s->syslog_fd < 0)
		return -1;

	/* set send buffer to 8M */
	if (socket_set_sndbuf(s->syslog_fd, 8<<20) < 0)
		log_warning("SO_SNDBUF(%s) failed: %m", JOURNAL_RUNDIR "/devlog");

	if (epollfd_add(s->epoll, s->syslog_fd, EPOLLIN, callback, s) < 0)
	{
		log_error("Failed to add syslog server fd to event loop: %m");
		return -1;
	}

	return 0;
}