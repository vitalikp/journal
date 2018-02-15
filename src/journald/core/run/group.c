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

#include "core/run.h"
#include "log.h"


int run_group(const char *group, gid_t *gid)
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

	*gid = gr->gr_gid;

	return 0;
}