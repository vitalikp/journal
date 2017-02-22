/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "server.h"
#include "log.h"


int server_start(server_t *s)
{
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
}

int server_run(server_t *s)
{
	return epollfd_run(s->epoll);
}
