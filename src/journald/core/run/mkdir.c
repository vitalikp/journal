/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <sys/stat.h>

#include "core/run.h"
#include "log.h"


int run_mkdir(void)
{
	struct stat st;

	if (!stat(JOURNAL_RUNDIR, &st))
	{
		if (!S_ISDIR(st.st_mode))
		{
			log_error("Failed to start: '%s' is not directory", JOURNAL_RUNDIR);
			return -1;
		}

		return 0;
	}

	if (errno != ENOENT)
	{
		log_error("Failed to start: cannot stat '%s' directory (%m)", JOURNAL_RUNDIR);
		return -1;
	}

	if (!mkdir(JOURNAL_RUNDIR, 0755))
		return 0;

	log_error("Failed to create '%s' directory: %m", JOURNAL_RUNDIR);

	return -1;
}
