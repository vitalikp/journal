/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <time.h>
#include <sys/socket.h>

#include "core/msg.h"


void msg_decode(msg_t *msg, uint8_t *buf, size_t size)
{
	struct cmsghdr *cmsg;
	size_t off;

	off = 0;
	while (off < size)
	{
		cmsg = (struct cmsghdr*)&buf[off];
		if (cmsg->cmsg_level != SOL_SOCKET)
			continue;

		switch (cmsg->cmsg_type)
		{
			case SO_TIMESTAMP:
				if (cmsg->cmsg_len == CMSG_LEN(sizeof(struct timeval)))
				{
					struct timeval *tv;

					tv = (struct timeval*) CMSG_DATA(cmsg);
					if (tv)
						msg->ts = tv->tv_sec * CLOCKS_PER_SEC + tv->tv_usec;
				}
				break;
			case SCM_CREDENTIALS:
				if (cmsg->cmsg_len == CMSG_LEN(sizeof(struct ucred)))
				{
					struct ucred *ucred;

					ucred = (struct ucred*) CMSG_DATA(cmsg);
					if (ucred)
					{
						msg->pid = ucred->pid;
						msg->uid = ucred->uid;
						msg->gid = ucred->gid;
					}
				}
				break;

		}

		off += CMSG_ALIGN(cmsg->cmsg_len);
	}
}