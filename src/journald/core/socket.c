/*
 * Copyright © 2015 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <stddef.h>
#include <sys/socket.h>

#include "socket.h"


int socket_set_sndbuf(int fd, int len)
{
	if (!setsockopt(fd, SOL_SOCKET, SO_SNDBUFFORCE, &len, sizeof(len)))
		return 0;

	if (!setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &len, sizeof(len)))
		return 0;

	return -1;
}
