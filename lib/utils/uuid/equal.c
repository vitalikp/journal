/*
 * Copyright © 2015-2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "uuid.h"


int uuid_equal(const uuid_t id1, const uuid_t id2)
{
	if (id1.qwords[0] != id2.qwords[0])
		return 0;

	if (id1.qwords[1] != id2.qwords[1])
		return 0;

	return 1;
}