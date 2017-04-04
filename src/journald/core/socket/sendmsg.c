/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>

#include "core/socket.h"


ssize_t socket_sendmsg(int fd, const char *path, void *data, size_t size)
{
	struct iovec iovec = { data, size };
	struct sockaddr_un sa;
	size_t len;
	ssize_t res;

	len = strlen(path);

	sa.sun_family = AF_UNIX;
	memcpy(sa.sun_path, path, len);
	len += sizeof(sa_family_t);

	struct msghdr msghdr =
	{
		.msg_iov = &iovec,
		.msg_iovlen = 1,
		.msg_name = &sa,
		.msg_namelen = len,
	};

	res = sendmsg(fd, &msghdr, MSG_NOSIGNAL);
	if (res < 0)
		return -1;

	return res;
}