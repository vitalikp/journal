/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

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

#include <unistd.h>

#include "journald-server.h"
#include "journald-kmsg.h"
#include "journald-syslog.h"

int main(int argc, char *argv[]) {
        Server server;
        int r;

        if (argc > 1) {
                log_error("This program does not take arguments.");
                return EXIT_FAILURE;
        }

        log_set_target(LOG_TARGET_SAFE);
        log_set_facility(LOG_SYSLOG);
        log_parse_environment();
        log_open();

        umask(0022);

        r = server_init(&server);
        if (r < 0)
                goto finish;

        server_vacuum(&server);
        server_flush_to_var(&server);
        server_flush_dev_kmsg(&server);

        log_debug("journald running as pid "PID_FMT, getpid());
        server_driver_message(&server, "Journal started");

        while (server.state != SERVER_FINISHED) {
                usec_t t = (usec_t) -1, n;

                n = now(CLOCK_REALTIME);

                if (server.max_retention_usec > 0 && server.oldest_file_usec > 0) {

                        /* The retention time is reached, so let's vacuum! */
                        if (server.oldest_file_usec + server.max_retention_usec < n) {
                                log_info("Retention time reached.");
                                server_rotate(&server);
                                server_vacuum(&server);
                                continue;
                        }

                        /* Calculate when to rotate the next time */
                        t = server.oldest_file_usec + server.max_retention_usec - n;
                }

                if (server.state == SERVER_EXITING)
                        server.state = SERVER_FINISHED;
                else {
                        r = sd_event_run(server.event, t);
                        if (r < 0) {
                                log_error("Failed to run event loop: %s", strerror(-r));
                                goto finish;
                        }
                }
        }

        log_debug("journald stopped as pid "PID_FMT, getpid());
        server_driver_message(&server, "Journal stopped");

finish:
        server_done(&server);

        return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
