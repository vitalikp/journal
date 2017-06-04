/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>

#include "private.h"


ssize_t kmsg_read(int fd, msg_t *msg)
{
	ssize_t sz;

	sz = read(fd, msg->data, msg->size);
	if (sz < 0)
		return -1;

	msg->data[sz-1] = '\0';

	return sz;
}