/*
 * Copyright © 2015-2016 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/epoll.h>

#include "epollfd.h"


#define EPOLL_QUEUE_MAX 512U

struct event_t
{
	int			fd;
	event_cb	callback;
	void*		data;

	event_t*	next;
};

typedef struct sigdata_t sigdata_t;

struct sigdata_t
{
	signal_cb	callback;
	void*		data;
};

struct epollfd_t
{
	/* epoll fd */
	int			fd;

	/* signals */
	sigset_t	sigset;
	sigset_t	oldsigset;
	sigdata_t	signals[_NSIG];

	/* epoll events */
	event_t*	events;
};


int epollfd_create(epollfd_t** pepoll)
{
	int fd;

	fd = epoll_create1(EPOLL_CLOEXEC);
	if (fd < 0)
		return -1;

	epollfd_t* epoll;

	epoll = calloc(1, sizeof(epollfd_t));
	if (!epoll)
	{
		close(fd);
		return -1;
	}

	epoll->fd = fd;

	*pepoll = epoll;

	return 0;
}

static void epollfd_free(epollfd_t* epoll)
{
	event_t* event = epoll->events;
	event_t* curr;
	while (event)
	{
		curr = event;
		event = event->next;

		epollfd_del(epoll, curr->fd);

		free(curr);
	}
}

void epollfd_close(epollfd_t** pepoll)
{
	epollfd_t* epoll = *pepoll;

	if (epoll)
	{
		if (!sigisemptyset(&epoll->oldsigset))
			sigprocmask(SIG_SETMASK, &epoll->oldsigset, NULL);

		epollfd_free(epoll);

		if (epoll->fd > 0)
			close(epoll->fd);

		*pepoll = NULL;
		free(epoll);
	}
}

int epollfd_add(epollfd_t* epoll, int fd, uint32_t events, event_cb callback, void* data)
{
	event_t* event = malloc(sizeof(event_t));
	if (!event)
		return -1;

	event->fd = fd;
	event->callback = callback;
	event->data = data;
	event->next = epoll->events;

	struct epoll_event ev = { events, event };

	if (epoll_ctl(epoll->fd, EPOLL_CTL_ADD, fd, &ev) < 0)
	{
		free(event);
		return -1;
	}

	epoll->events = event;

	return 0;
}

int epollfd_del(epollfd_t* epoll, int fd)
{
	if (epoll_ctl(epoll->fd, EPOLL_CTL_DEL, fd, NULL) < 0)
		return -1;

	return 0;
}

int epollfd_signal_add(epollfd_t* epoll, int signo, signal_cb callback, void* data)
{
	if (sigaddset(&epoll->sigset, signo) < 0)
		return -1;

	epoll->signals[signo].callback = callback;
	epoll->signals[signo].data = data;

	return 0;
}

static int signal_process(int fd, uint32_t events, epollfd_t* epoll)
{
	struct signalfd_siginfo si;
	ssize_t sz;

	sz = read(fd, &si, sizeof(si));
	if (sz < 0)
	{
		if (errno == EAGAIN || errno == EINTR)
			return 0;

		return -1;
	}

	if (sz != sizeof(si))
	{
		errno = EIO;
		return -1;
	}

	sigdata_t* signal = &epoll->signals[si.ssi_signo];
	if (!signal->callback)
	{
		errno = EIO;
		return -1;
	}

	if (signal->callback(&si, signal->data) < 0)
		return -1;

	return 0;
}

int epollfd_signal_setup(epollfd_t* epoll)
{
	if (sigprocmask(SIG_SETMASK, &epoll->sigset, &epoll->oldsigset) < 0)
		return -1;

	int fd = -1;

	fd = signalfd(fd, &epoll->sigset, SFD_NONBLOCK|SFD_CLOEXEC);
	if (fd < 0)
		return -1;

	if (epollfd_add(epoll, fd, EPOLLIN, (event_cb)signal_process, epoll) < 0)
	{
		close(fd);
		return -1;
	}

	return 0;
}

static int event_process(event_t* event, uint32_t events)
{
	if (!event->callback)
		return 0;

	return event->callback(event->fd, events, event->data);
}

int epollfd_run(epollfd_t* epoll)
{
	static struct epoll_event ev_queue[EPOLL_QUEUE_MAX];

	int num;

	num = epoll_wait(epoll->fd, ev_queue, EPOLL_QUEUE_MAX, -1);
	if (num < 0)
	{
		if (errno == EAGAIN || errno == EINTR)
			return 0;

		return -1;
	}

	uint32_t events;
	event_t* event;

	int i = 0;
	while (i < num)
	{
		event = ev_queue[i].data.ptr;
		events = ev_queue[i].events;

		if (event)
		{
			if (event_process(event, events) < 0)
				return -1;
		}

		i++;
	}

	return 0;
}
