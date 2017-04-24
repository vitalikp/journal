/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>

#include "core/kmsg.h"
#include "core/seqnum.h"
#include "log.h"


void kmsg_close(server_t *s)
{
	if (s->kmsg_fd < 0)
		return;

	close(s->kmsg_fd);
	s->kmsg_fd = -1;
}
