/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>
#include <limits.h>

#include "core/server.h"
#include "core/run.h"
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

	s->msg = msg_new(LINE_MAX);
	if (!s->msg)
		return -1;

	return 0;
}