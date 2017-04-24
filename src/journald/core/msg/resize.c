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


int msg_resize(msg_t **pmsg, size_t size)
{
	msg_t *msg = *pmsg;

	if (msg && msg->size > size)
		return 0;

	msg = realloc(msg, sizeof(msg_t) + size);
	if (!msg)
		return -1;

	msg->size = size;
	*pmsg = msg;

	return 0;
}