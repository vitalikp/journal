/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_MSG_H_
#define _JOURNALD_MSG_H_

#include <stdint.h>
#include <sys/time.h>
#include <sys/socket.h>


typedef struct msg
{
	uint8_t		pri;
	uint64_t	seqnum;
	uint64_t	ts;
	pid_t		pid;
	uid_t		uid;
	gid_t		gid;

	size_t		size;
	uint8_t		data[0];
} msg_t;


msg_t* msg_new(size_t size);
int msg_resize(msg_t **pmsg, size_t size);

void msg_decode(msg_t *msg, uint8_t *buf, size_t size);

#endif	/* _JOURNALD_MSG_H_ */
