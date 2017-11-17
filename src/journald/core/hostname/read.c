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
#include "utils.h"


int hostname_read(int fd, char* hostname)
{
	char host[HOST_NAME_MAX];
	int len;

	if (fd < 0 || !hostname)
		return -1;

	lseek(fd, 0, SEEK_SET);

	len = read(fd, host, HOST_NAME_MAX);
	if (len <= 0)
		return len;

	host[len-1] = '\0';

	if (str_eq("(none)", host))
	{
		hostname[0] = '\0';

		return 0;
	}

	str_copy(hostname, host, HOST_NAME_MAX);

	return len;
}
