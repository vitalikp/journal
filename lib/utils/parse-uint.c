/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal.
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <errno.h>

#include "utils.h"


int parse_uint(const char *str, uint64_t *pval)
{
	if (!str || !pval)
	{
		errno = EINVAL;
		return -1;
	}

	uint64_t val = 0;
	char *p = (char*)str;
	while (*p)
	{
		if (*p < '0' || *p > '9')
		{
			errno = EINVAL;
			return -1;
		}

		if (val > 0)
			val *= 10;
		val += *p - '0';

		p++;
	}

	*pval = val;

	return 0;
}


#ifdef TESTS
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <assert.h>


static void test_null(const char *str)
{
	int res;

	res = parse_uint(str, NULL);
	assert(res < 0);

	printf("test null parse uint '%s'\n", str);
}

static void test_fail(const char *str)
{
	uint64_t val = -1;

	int res;

	res = parse_uint(str, &val);
	assert(res < 0);
	assert(val == -1);

	printf("test fail parse uint '%s': %m\n", str);
}

static void test_parse(const char *str, uint64_t rval)
{
	uint64_t val = -1;

	int res;

	res = parse_uint(str, &val);
	assert(!res);
	assert(val == rval);

	printf("test parse uint '%s': %"PRIu64"\n", str, val);
}

int main()
{
	test_null(NULL);
	test_null("123");

	test_fail("123a");
	test_fail("b321");

	test_parse("3", 3);
	test_parse("8", 8);
	test_parse("57", 57);
	test_parse("123", 123);

	return EXIT_SUCCESS;
}

#endif // TESTS