/*
 * Copyright © 2015 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "socket.h"
#include "log.h"
#include "selinux-util.h"


int socket_open(const char* path, int type)
{
	int fd;

	struct sockaddr_un sa;

	size_t len = strlen(path);

	sa.sun_family = AF_UNIX;
	memcpy(sa.sun_path, path, len);
	len += sizeof(sa_family_t);

	fd = socket(AF_UNIX, type|SOCK_CLOEXEC|SOCK_NONBLOCK, 0);
	if (fd < 0)
	{
		log_error("socket() failed: %m");
		return -1;
	}

	unlink(sa.sun_path);

	if (bind(fd, &sa, len) < 0)
	{
		log_error("bind(%s) failed: %m", sa.sun_path);
		return -1;
	}

	chmod(sa.sun_path, 0666);

	if (type == SOCK_STREAM)
	{
		if (listen(fd, SOMAXCONN) < 0)
		{
			log_error("listen(%s) failed: %m", sa.sun_path);
			return -1;
		}
	}

	int optval;

	optval = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)) < 0)
	{
		log_error("SO_PASSCRED failed: %m");
		return -1;
	}

#ifdef HAVE_SELINUX
	if (use_selinux())
	{
		optval = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_PASSSEC, &optval, sizeof(optval)) < 0)
				log_warning("SO_PASSSEC(%s) failed: %m", sa.sun_path);
	}
#endif

	optval = 8<<20;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &optval, sizeof(optval)) < 0)
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval)) < 0)
			log_warning("SO_RCVBUF(%s) failed: %m", sa.sun_path);

	if (type != SOCK_STREAM)
	{
		optval = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &optval, sizeof(optval)) < 0)
		{
			log_error("SO_TIMESTAMP(%s) failed: %m", sa.sun_path);
			return -1;
		}
	}

	return fd;
}
