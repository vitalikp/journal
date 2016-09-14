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
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "epollfd.h"
#include "socket.h"


static void test_create(void)
{
	epollfd_t* epoll;
	int res;

	res = epollfd_create(&epoll);
	assert(res == 0 && epoll);

	epollfd_close(&epoll);
	assert(epoll == NULL);
}

static int io_change(int fd, uint32_t events, int* val)
{
	*val = fd;

	return 0;
}

static void io_send(const char* path)
{
	int fd;

	struct sockaddr_un sa;

	size_t len = strlen(path);

	sa.sun_family = AF_UNIX;
	memcpy(sa.sun_path, path, len);
	len += sizeof(sa_family_t);

	fd = socket(AF_UNIX, SOCK_DGRAM|SOCK_CLOEXEC|SOCK_NONBLOCK, 0);
	if (fd < 0)
		return;

	if (connect(fd, &sa, len) < 0)
		return;

	write(fd, NULL, 0);

	close(fd);
}

static void test_io()
{
	const char* path = "/tmp/test_socket";

	epollfd_t* epoll;
	int res;
	int fd;

	res = epollfd_create(&epoll);
	assert(res == 0 && epoll);

	fd = socket_open(path, SOCK_DGRAM);
	assert(fd > 0);

	int val = 1;

	res = epollfd_add(epoll, fd, EPOLLIN, (event_cb)io_change, &val);
	assert(res == 0);

	pid_t pid;

	pid = fork();
	assert(pid >= 0);

	if (!pid)
	{
		io_send(path);
		exit(0);
	}

	res = epollfd_run(epoll);
	assert(res == 0);
	assert(fd == val);

	epollfd_close(&epoll);
	assert(epoll == NULL);

	res = unlink(path);
	assert(res == 0);
}

int main(int argc, char *argv[])
{
	test_create();

	test_io();

	return EXIT_SUCCESS;
}