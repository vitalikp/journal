/*
 * Copyright © 2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "core/syslog.h"
#include "utils.h"
#include "log.h"


#define DEVLOG "/dev/log"

void syslog_run(server_t *s)
{
	struct stat st = {};

	if (!lstat(DEVLOG, &st) || errno != ENOENT)
		return;

	if (symlink(JOURNAL_RUNDEVLOG, DEVLOG) < 0)
	{
		log_error("Failed to create symlink “%s”: %m", DEVLOG);
		return;
	}

	log_info("Created socket symlink from “%s” to “%s”.", JOURNAL_RUNDEVLOG, DEVLOG);
}
