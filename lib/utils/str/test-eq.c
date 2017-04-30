/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal.
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "utils.h"


static void test_equal(const char *str1, const char *str2)
{
	int res;

	res = str_eq(str1, str2);
	assert(res == 1);

	printf("test equal string '%s': '%s'\n", str1, str2);
}

static void test_notequal(const char *str1, const char *str2)
{
	int res;

	res = str_eq(str1, str2);
	assert(!res);

	printf("test not equal string '%s': '%s'\n", str1, str2);
}

int main()
{
	test_equal(NULL, NULL);
	test_equal("", "");
	test_equal("eq", "eq");
	test_equal("equal string", "equal string");

	test_notequal(NULL, "");
	test_notequal("", NULL);
	test_notequal("string", "not string");
	test_notequal("not string", "string");
	test_notequal("string 1", "string");
	test_notequal("string", "string 2");

	return EXIT_SUCCESS;
}