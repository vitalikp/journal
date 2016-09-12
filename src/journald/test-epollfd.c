/*
 * Copyright © 2016 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <stdlib.h>
#include <assert.h>

#include "epollfd.h"


static void test_create(void)
{
	epollfd_t* epoll;
	int res;

	res = epollfd_create(&epoll);
	assert(res == 0 && epoll);

	epollfd_close(&epoll);
	assert(epoll == NULL);
}

int main(int argc, char *argv[])
{
	test_create();

	return EXIT_SUCCESS;
}