/*
 * Copyright © 2017-2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>
#include <limits.h>

#include "core/server.h"
#include "core/run.h"
#include "core/syslog.h"
#include "core/seqnum.h"
#include "core/hostname.h"
#include "log.h"


int server_start(server_t *s)
{
	if (!getuid())
	{
		uid_t uid = 0;

		if (run_mkdir(JOURNAL_RUNDIR) < 0)
		{
			log_error("Failed to create '%s' directory: %m", JOURNAL_RUNDIR);
			return -1;
		}

		if (run_mkdir(JOURNAL_LOGDIR) < 0)
			log_warning("Failed to create '%s' directory: %m", JOURNAL_LOGDIR);

		syslog_run(s);

		if (run_group(s->rungroup) < 0)
			return -1;

		if (run_user(s->runuser, &uid) < 0)
			return -1;
	}

	if (epollfd_create(&s->epoll) < 0)
	{
		log_error("Failed to create event loop: %m");
		return -1;
	}

	s->msg = msg_new(LINE_MAX);
	if (!s->msg)
		return -1;

	seqnum_load(JOURNAL_RUNDIR "/kernel-seqnum", &s->kseqnum);

	if (hostname_open(s) < 0)
		return -1;

	boot_get_id(&s->boot_id);

	return 0;
}