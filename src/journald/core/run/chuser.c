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


int run_chuser(uid_t uid)
{
	if (setuid(uid) < 0)
		return -1;

	return 0;
}