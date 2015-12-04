/*
 * Copyright © 2015 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include "boot.h"


int journal_get_bootid(uuid_t* boot_id)
{
	int fd = -1;
	char buf[37];
	ssize_t sz;

	fd = open("/proc/sys/kernel/random/boot_id", O_RDONLY|O_CLOEXEC|O_NOCTTY);
	if (fd < 0)
		return -1;

	sz = read(fd, buf, 36);

	close(fd);

	if (sz != 36)
	{
		errno = EIO;
		return -1;
	}

	buf[36] = '\0';

	if (uuid_parse(buf, boot_id))
	{
		errno = EIO;
		return -1;
	}

	return 0;
}
