/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>
#include <grp.h>

#include "run.h"
#include "log.h"


int run_group(const char *group)
{
	struct group *gr;

	if (!group)
		group = "journal";

	gr = getgrnam(group);
	if (!gr)
	{
		log_warning("No such group '%s'!", group);

		return 0;
	}

	if (chown(JOURNAL_RUNDIR, -1, gr->gr_gid) < 0)
	{
		log_error("Unable to chgrp run directory to %d (%s): %m", gr->gr_gid, group);

		return -1;
	}

	if (setgid(gr->gr_gid) < 0)
	{
		log_error("Unable to setgid to %d (%s): %m", gr->gr_gid, group);

		return -1;
	}

	return 0;
}