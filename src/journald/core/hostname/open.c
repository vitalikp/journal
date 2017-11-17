/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>
#include <fcntl.h>

#include "core/hostname.h"
#include "log.h"
#include "utils.h"


static int server_hostname_io_change(int fd, server_t* s)
{
	hostname_read(fd, s->hostname);

	return 0;
}

int hostname_open(server_t* s)
{
	int fd;

	fd = open("/proc/sys/kernel/hostname", O_RDONLY|O_CLOEXEC|O_NDELAY|O_NOCTTY);
	if (fd < 0)
	{
		log_error("Failed to open /proc/sys/kernel/hostname: %m");
		return -1;
	}

	if (epollfd_add(s->epoll, fd, 0, (event_cb)server_hostname_io_change, s) < 0)
	{
		close(fd);
		return -1;
	}

	hostname_read(fd, s->hostname);

	return 0;
}