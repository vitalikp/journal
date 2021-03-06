/*
 * Copyright © 2017-2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>

#include "core/syslog.h"


void syslog_close(server_t *s)
{
	if (s->syslog_fd < 0)
		return;

	close(s->syslog_fd);
	s->syslog_fd = -1;

	unlink(JOURNAL_RUNDEVLOG);
}
