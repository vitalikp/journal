/*
 * Copyright © 2015-2017 - Vitaliy Perevertun
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