/*
 * Copyright © 2017-2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_SYSLOG_H_
#define _JOURNALD_SYSLOG_H_

#include "server.h"


int syslog_open(server_t *s, event_cb callback);
void syslog_close(server_t *s);

void syslog_run(server_t *s);

#endif	/* _JOURNALD_SYSLOG_H_ */
