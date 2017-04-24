/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <sys/ioctl.h>
#include <linux/sockios.h>

#include "core/socket.h"


int socket_get_size(int fd, msg_t **pmsg)
{
	int sz;

	if (ioctl(fd, SIOCINQ, &sz) < 0)
		return -1;

	return msg_resize(pmsg, sz);
}
