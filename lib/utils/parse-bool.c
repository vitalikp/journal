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


int parse_bool(const char *str, bool *pval)
{
	if (str_empty(str) || !pval)
	{
		errno = EINVAL;
		return -1;
	}

	if (!str[1])
	{
		switch (str[0])
		{
			case '1':
			case 'y':
			case 'Y':
				*pval = true;
				break;

			case '0':
			case 'n':
			case 'N':
				*pval = false;
				break;

			default:
				errno = EINVAL;
				return -1;
		}

		return 0;
	}

	if (str_caseeq(str, "yes") || str_caseeq(str, "true") || str_caseeq(str, "on"))
	{
		*pval = true;
		return 0;
	}

	if (str_caseeq(str, "no") || str_caseeq(str, "false") || str_caseeq(str, "off"))
	{
		*pval = false;
		return 0;
	}

	errno = EINVAL;
	return -1;
}


#ifdef TESTS
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <assert.h>


static void test_null(const char *str)
{
	int res;

	res = parse_bool(str, NULL);
	assert(res < 0);

	printf("test null parse bool '%s'\n", str);
}

static void test_fail(const char *str)
{
	bool val = true;

	int res;

	res = parse_bool(str, &val);
	assert(res < 0);
	assert(val);

	printf("test fail parse bool '%s': %m\n", str);
}

static void test_parse(const char *str, bool rval)
{
	bool val = !rval;

	int res;

	res = parse_bool(str, &val);
	assert(!res);
	assert(val == rval);

	printf("test parse bool '%s': %d\n", str, val);
}

int main()
{
	test_null(NULL);
	test_null("1");

	test_fail("1a");
	test_fail("b1");
	test_fail("a");

	test_parse("1", true);
	test_parse("0", false);
	test_parse("Y", true);
	test_parse("y", true);
	test_parse("N", false);
	test_parse("n", false);
	test_parse("yes", true);
	test_parse("Yes", true);
	test_parse("true", true);
	test_parse("True", true);
	test_parse("on", true);
	test_parse("ON", true);
	test_parse("On", true);
	test_parse("oN", true);
	test_parse("no", false);
	test_parse("No", false);
	test_parse("false", false);
	test_parse("False", false);
	test_parse("off", false);
	test_parse("OFF", false);
	test_parse("Off", false);

	return EXIT_SUCCESS;
}

#endif // TESTS