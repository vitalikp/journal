/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>

#include "core/native.h"


void native_close(server_t *s)
{
	if (s->native_fd < 0)
		return;

	close(s->native_fd);
	s->native_fd = -1;

	unlink(JOURNAL_RUNDIR "/socket");
}
