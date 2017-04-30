/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal.
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNAL_UTILS_H_
#define _JOURNAL_UTILS_H_

#include <stdint.h>
#include <sys/types.h>

#include "utils/str.h"
#include "utils/uuid.h"


#define DEC(val) '0' + val
#define DECVAL(val) val - 10
#define HEXVAL(val) 'a' + DECVAL(val)


static inline char char_hex(uint8_t val)
{
	if (val < 10)
		return DEC(val);

	return HEXVAL(val);
}

static inline uint8_t hex_char(char c)
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

int parse_uint(const char *str, uint64_t *pval);

#endif	/* _JOURNAL_UTILS_H_ */