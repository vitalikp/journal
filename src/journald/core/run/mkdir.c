/*
 * Copyright © 2017-2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <errno.h>
#include <sys/stat.h>

#include "core/run.h"


int run_mkdir(const char *dn)
{
	struct stat st;

	if (!stat(dn, &st))
	{
		if (!S_ISDIR(st.st_mode))
		{
			errno = ENOTDIR;
			return -1;
		}

		return 0;
	}

	if (errno != ENOENT)
		return -1;

	if (mkdir(dn, 0755) < 0)
		return -1;

	return 0;
}
