/*
 * Copyright © 2015 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "uuid.h"


static uint8_t chartohex(char c)
{
	if (c < '0' || c > 'f')
		return -1;

	if (c >= 'a')
		return c - 'a' + 10;

	if (c <= '9')
		return c - '0';

	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return -1;
}

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

		a = chartohex(in[i++]);
		if (a < 0)
			return -1;

		b = chartohex(in[i++]);
		if (a < 0)
			return -1;

		id->bytes[n++] = (a << 4) | b;
	}

	return 0;
}
