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


static void test_empty(const char *str)
{
	int res;

	res = str_empty(str);
	assert(res == 1);

	printf("test empty string '%s'\n", str);
}

static void test_notempty(const char *str)
{
	int res;

	res = str_empty(str);
	assert(!res);

	printf("test not empty string '%s'\n", str);
}

int main()
{
	test_empty(NULL);
	test_empty("");

	test_notempty("string");
	test_notempty("test not empty");

	return EXIT_SUCCESS;
}