/*
 * Copyright © 2015-2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "utils.h"


int uuid_parse(const char* in, uuid_t* id)
{
	uint8_t a, b;
	uint8_t i = 0, n = 0;

	while (n < 16)
	{
		if (i == 8 || i == 13 || i == 18 || i == 23)
		{
			if (in[i] == '-')
			{
				i++;
				continue;
			}
		}

		a = hex_char(in[i++]);
		if (a < 0)
			return -1;

		b = hex_char(in[i++]);
		if (a < 0)
			return -1;

		id->bytes[n++] = (a << 4) | b;
	}

	return 0;
}
