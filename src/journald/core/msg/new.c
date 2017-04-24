/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <stdlib.h>

#include "core/msg.h"


msg_t* msg_new(size_t size)
{
	msg_t *msg;

	msg = calloc(1, sizeof(msg_t) + size);
	if (!msg)
		return NULL;

	msg->size = size;

	return msg;
}