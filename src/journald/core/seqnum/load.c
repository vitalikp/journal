/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <fcntl.h>
#include <unistd.h>

#include "core/seqnum.h"
#include "log.h"


int seqnum_load(const char *fn, uint64_t *pseqnum)
{
	int fd;
	int res;

	fd = open(fn, O_RDONLY|O_CLOEXEC|O_NOCTTY|O_NOFOLLOW, 0);
	if (fd < 0)
	{
		if (errno == ENOENT)
			return 0;

		log_error("Failed to open “%s” file, ignoring: %m", fn);
		return -1;
	}

	res = read(fd, pseqnum, sizeof(uint64_t));
	if (res < 0)
		log_error("Failed to read “%s” file, ignoring: %m", fn);

	close(fd);

	return res;
}