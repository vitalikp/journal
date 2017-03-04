/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "server.h"


int server_run(server_t *s)
{
	return epollfd_run(s->epoll);
}
