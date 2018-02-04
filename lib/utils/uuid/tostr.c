/*
 * Copyright © 2015-2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "uuid.h"


API char* journal_uuid_to_str(uuid_t id, char str[33])
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
JOURNAL_API(uuid_to_str)