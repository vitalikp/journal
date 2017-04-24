/*
 * Copyright © 2015-2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_SOCKET_H_
#define _JOURNALD_SOCKET_H_

#include "msg.h"


int socket_open(const char* path, int type);
int socket_set_sndbuf(int fd, int len);

int socket_get_size(int fd, msg_t **pmsg);
ssize_t socket_sendmsg(int fd, const char *path, void *data, size_t size);
ssize_t socket_recvmsg(int fd, msg_t *msg);

#endif	/* _JOURNALD_SOCKET_H_ */
