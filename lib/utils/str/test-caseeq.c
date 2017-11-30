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

	res = str_caseeq(str1, str2);
	assert(res == 1);

	printf("test case equal string '%s': '%s'\n", str1, str2);
}

static void test_notequal(const char *str1, const char *str2)
{
	int res;

	res = str_caseeq(str1, str2);
	assert(!res);

	printf("test not case equal string '%s': '%s'\n", str1, str2);
}

int main(void)
{
	test_equal(NULL, NULL);
	test_equal("", "");
	test_equal("eq", "eq");
	test_equal("eq", "EQ");
	test_equal("eq", "Eq");
	test_equal("equal string", "equal string");
	test_equal("equal string", "EQUAL STRING");
	test_equal("EQUAL STRING", "equal string");
	test_equal("equal string", "Equal String");
	test_equal("Equal String", "equal string");
	test_equal("case equal string", "case equal string");
	test_equal("CaSe EqUaL sTrInG", "cAsE eQuAl StRiNg");
	test_equal("cAsE eQuAl StRiNg", "CaSe EqUaL sTrInG");

	// TODO add unit tests

	test_notequal(NULL, "");
	test_notequal("", NULL);
	test_notequal("string", "Not String");
	test_notequal("not string", "String");
	test_notequal("string 1", "String");
	test_notequal("string", "String 2");

	return EXIT_SUCCESS;
}