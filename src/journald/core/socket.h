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


int socket_open(const char* path, int type);
int socket_set_sndbuf(int fd, int len);

ssize_t socket_sendmsg(int fd, const char *path, void *data, size_t size);

#endif	/* _JOURNALD_SOCKET_H_ */
