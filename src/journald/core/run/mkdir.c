/*
 * Copyright © 2017-2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <sys/stat.h>

#include "core/run.h"
#include "log.h"


int run_mkdir(const char *dn)
{
	struct stat st;

	if (!stat(dn, &st))
	{
		if (!S_ISDIR(st.st_mode))
		{
			log_error("Failed to start: '%s' is not directory", dn);
			return -1;
		}

		return 0;
	}

	if (errno != ENOENT)
	{
		log_error("Failed to start: cannot stat '%s' directory (%m)", dn);
		return -1;
	}

	if (mkdir(dn, 0755) < 0)
	{
		log_error("Failed to create '%s' directory: %m", dn);
		return -1;
	}

	return 0;
}
