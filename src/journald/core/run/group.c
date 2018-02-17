/*
 * Copyright © 2017-2018 - Vitaliy Perevertun
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


void run_group(server_t *s, gid_t *gid)
{
	struct group *gr;

	if (!s->rungroup)
		s->rungroup = "journal";

	gr = getgrnam(s->rungroup);
	if (!gr)
	{
		log_warning("No such group '%s'!", s->rungroup);

		return;
	}

	*gid = gr->gr_gid;
}