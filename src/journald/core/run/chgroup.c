/*
 * Copyright © 2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <unistd.h>

#include "core/run.h"


int run_chgroup(gid_t gid)
{
	if (setgid(gid) < 0)
		return -1;

	return 0;
}