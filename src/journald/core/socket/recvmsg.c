/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/sockios.h>

#include "core/socket.h"


ssize_t socket_recvmsg(int fd, msg_t *msg)
{
	struct timespec ts = {};
	struct cmsghdr *cmsg;
	struct iovec iovec = { &msg->data, msg->size };
	ssize_t res;

	uint8_t buf[CMSG_SPACE(sizeof(struct ucred)) +
				CMSG_SPACE(sizeof(struct timeval))];

	struct msghdr msghdr =
	{
		.msg_iov = &iovec,
		.msg_iovlen = 1,
		.msg_control = &buf,
		.msg_controllen = sizeof(buf),
	};

	res = recvmsg(fd, &msghdr, MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
	if (res < 0)
		return -1;

	msg->ts = -1;
	msg->pid = 0;
	msg->uid = 0;
	msg->gid = 0;

	if (!clock_gettime(CLOCK_REALTIME, &ts))
		msg->ts = ts.tv_sec * CLOCKS_PER_SEC + ts.tv_nsec / 1000;

	msg_decode(msg, buf, msghdr.msg_controllen);

	return res;
}