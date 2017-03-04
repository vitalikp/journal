/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "server.h"
#include "syslog.h"
#include "native.h"


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
