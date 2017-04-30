/*
 * Copyright © 2015 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "uuid.h"


static void random_get_bytes(uuid_t* id, size_t bytes)
{
	int fd;

	fd = open("/dev/urandom", O_RDONLY|O_CLOEXEC);
	if (fd == -1)
		fd = open("/dev/random", O_RDONLY|O_NONBLOCK|O_CLOEXEC);

	unsigned char *cp = (unsigned char *) id->bytes;

	size_t n = bytes;
	if (fd >= 0)
	{
		uint8_t cnt = 0;
		size_t x;
		while (n > 0)
		{
			if (fd >= 0)
			{
				x = read(fd, cp, n);
				if (x <= 0)
				{
					if (cnt++ > 3)
						break;

					continue;
				}
			}
			else
			{
				*cp ^= (rand() >> 7) & 0xFF;
				x = 1;
			}

			n -= x;
			cp += x;
			cnt = 0;
		}

		if (fd >= 0)
			close(fd);
	}
}

void uuid_gen_rand(uuid_t* id)
{
	random_get_bytes(id, sizeof(id->bytes));

	/* Set UUID version to 4 --- truly random generation */
	id->bytes[6] = (id->bytes[6] & 0x0F) | 0x40;

	/* Set the UUID variant to DCE */
	id->bytes[8] = (id->bytes[8] & 0x3F) | 0x80;
}

char* uuid_to_str(uuid_t id, char str[33])
{
	static const char table[16] = "0123456789abcdef";
	uint8_t i = 0;

	while (i < 16)
	{
		str[i*2] = table[(id.bytes[i] >> 4) & 15];
		str[i*2+1] = table[(id.bytes[i] & 0xf) & 15];
		i++;
	}

	str[32] = '\0';

	return str;
}

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
