/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_SERVER_H_
#define _JOURNALD_SERVER_H_

#include "epollfd.h"
#include "msg.h"
#include "utils.h"


typedef struct server server_t;

struct server
{
	epollfd_t	*epoll;

	int			syslog_fd;
	int			native_fd;
	int			kmsg_fd;

	char		*runuser;
	char		*rungroup;

	/** kmsg sequence number */
	uint64_t	kseqnum;

	uuid_t		boot_id;
	char		hostname[HOST_NAME_MAX];

	msg_t		*msg;
};


int server_start(server_t *s);
void server_stop(server_t *s);

int server_run(server_t *s);

#endif	/* _JOURNALD_SERVER_H_ */