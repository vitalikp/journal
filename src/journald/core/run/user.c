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


void run_user(server_t *s, uid_t *uid, gid_t *gid)
{
	struct passwd* pw;

	if (!s->runuser)
		s->runuser = "journal";

	pw = getpwnam(s->runuser);
	if (!pw)
	{
		log_warning("No such user '%s'!", s->runuser);

		return;
	}

	*uid = pw->pw_uid;
	*gid = pw->pw_gid;

	if (!str_empty(pw->pw_dir) && chdir(pw->pw_dir) < 0)
		log_warning("Unable change working directory to '%s' path: %m", pw->pw_dir);
}