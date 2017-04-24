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


int seqnum_save(const char *fn, uint64_t *pseqnum)
{
	int fd;
	int res;

	fd = open(fn, O_RDWR|O_CREAT|O_CLOEXEC|O_NOCTTY|O_NOFOLLOW, 0644);
	if (fd < 0)
	{
		log_error("Failed to open “%s”, ignoring: %m", fn);
		return -1;
	}

	res = write(fd, pseqnum, sizeof(uint64_t));
	if (res < 0)
		log_error("Failed to write “%s” file, ignoring: %m", fn);

	close(fd);

	return 0;
}