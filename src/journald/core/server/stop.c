/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "core/server.h"
#include "core/syslog.h"
#include "core/native.h"


void server_stop(server_t *s)
{
	epollfd_close(&s->epoll);

	syslog_close(s);
	native_close(s);
}