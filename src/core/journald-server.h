/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

/***
  This file is part of systemd.

  Copyright 2011 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <inttypes.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "core/server.h"
#include "journal-file.h"
#include "hashmap.h"
#include "util.h"
#include "journald-rate-limit.h"


typedef enum ServerState
{
	SERVER_RUNNING,
	SERVER_EXITING,
	SERVER_FINISHED
} ServerState;

typedef enum Storage {
        STORAGE_AUTO,
        STORAGE_VOLATILE,
        STORAGE_PERSISTENT,
        STORAGE_NONE,
        _STORAGE_MAX,
        _STORAGE_INVALID = -1
} Storage;

typedef struct Server {
        ServerState state;
        server_t server;

        JournalFile *runtime_journal;
        JournalFile *system_journal;
        Hashmap *user_journals;

        uint64_t seqnum;

        char *buffer;
        size_t buffer_size;

        JournalRateLimit *rate_limit;
        usec_t sync_interval_usec;
        usec_t rate_limit_interval;
        unsigned rate_limit_burst;

        JournalMetrics runtime_metrics;
        JournalMetrics system_metrics;

        bool compress;

        bool forward_to_syslog;
        bool forward_to_console;

        uint64_t cached_available_space;
        usec_t cached_available_space_timestamp;

        uint64_t var_available_timestamp;

        usec_t max_retention_usec;
        usec_t max_file_usec;
        usec_t oldest_file_usec;

        char *tty_path;

        int max_level_store;
        int max_level_syslog;
        int max_level_kmsg;
        int max_level_console;

        Storage storage;

        MMapCache *mmap;

        bool dev_kmsg_readable;

        uint64_t sync_seqnum;
        usec_t sync_time;
} Server;

#define N_IOVEC_META_FIELDS 20
#define N_IOVEC_KERNEL_FIELDS 64
#define N_IOVEC_OBJECT_FIELDS 11

int dispatch_message_real(struct iovec *iovec, struct ucred *ucred);
int dispatch_message(Server *s, struct iovec *iovec, struct timeval *tv);
void server_dispatch_message(Server *s, struct iovec *iovec, unsigned n, unsigned m, struct ucred *ucred, struct timeval *tv, int priority);
void server_driver_message(Server *s, const char *format, ...) _printf_(2,3);

/* gperf lookup function */
const struct ConfigPerfItem* journald_gperf_lookup(const char *key, size_t length);

int config_parse_storage(const char *filename, unsigned line, const char *rvalue, void *data);

const char *storage_to_string(Storage s) _const_;
Storage storage_from_string(const char *s) _pure_;

void server_fix_perms(Server *s, JournalFile *f);
bool shall_try_append_again(JournalFile *f, int r);
int server_init(Server *s);
void server_done(Server *s);
void server_sync(Server *s);
void server_vacuum(Server *s);
void server_rotate(Server *s);
int server_schedule_sync(Server *s, int priority);
int server_flush_to_var(Server *s);
int process_datagram(int fd, uint32_t events, void *userdata);
