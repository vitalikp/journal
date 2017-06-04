/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "core/kmsg.h"
#include "utils.h"


static void decode_prefix(msg_t *msg, size_t len)
{
	uint8_t *p = msg->data;
	uint64_t data = 0;
	uint8_t pos = 0;
	size_t i = 0;


	while (i < len)
	{
		if (p[i++] != ',')
			continue;

		p[i-1] = '\0';

		if (pos < 3)
		{
			data = 0;
			if (parse_uint(p, &data) < 0)
				return;

			switch (pos)
			{
				case 0:
					if (data > 7)
						continue;

					msg->pri = data;
					break;
				case 1:
					msg->seqnum = data;
					break;
				case 2:
					msg->ts += data;
					break;

				default:
					return;
			}

			p += i;
			i = 0;
			pos++;
		}
	}
}

void kmsg_decode(msg_t *msg, size_t size)
{
	uint8_t *p = msg->data;
	size_t len;

	p = msg->data;

	while (*p)
	{
		if (*p++ != ';')
			continue;

		len = p - msg->data;

		decode_prefix(msg, len);

		str_copy(msg->data, p, msg->size);
	}
}