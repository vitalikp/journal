/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>

#include "server.h"
#include "run.h"
#include "syslog.h"
#include "native.h"
#include "log.h"


int server_start(server_t *s)
{
	if (!getuid())
	{
		if (run_mkdir() < 0)
			return -1;

		if (run_group(s->rungroup) < 0)
			return -1;

		if (run_user(s->runuser) < 0)
			return -1;
	}

	if (epollfd_create(&s->epoll) < 0)
	{
		log_error("Failed to create event loop: %m");
		return -1;
	}

	return 0;
}

void server_stop(server_t *s)
{
	epollfd_close(&s->epoll);

	syslog_close(s);
	native_close(s);
}

int server_run(server_t *s)
{
	return epollfd_run(s->epoll);
}
