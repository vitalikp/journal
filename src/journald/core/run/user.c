/*
 * Copyright © 2017-2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>
#include <pwd.h>

#include "core/run.h"
#include "utils.h"
#include "log.h"


int run_user(const char *user, uid_t *uid, gid_t *gid)
{
	struct passwd* pw;

	if (!user)
		user = "journal";

	pw = getpwnam(user);
	if (!pw)
	{
		log_warning("No such user '%s'!", user);

		return 0;
	}

	*uid = pw->pw_uid;
	*gid = pw->pw_gid;

	if (chown(JOURNAL_RUNDIR, pw->pw_uid, -1) < 0)
	{
		log_error("Unable to chown run directory to %d (%s): %m", pw->pw_uid, user);

		return -1;
	}

	if (!getgid())
	{
		if (setgid(pw->pw_gid) < 0)
		{
			log_error("Unable to setgid to %d: %m", pw->pw_gid);

			return -1;
		}
	}

	if (!str_empty(pw->pw_dir) && chdir(pw->pw_dir) < 0)
		log_warning("Unable change working directory to '%s' path: %m", pw->pw_dir);

	return 0;
}