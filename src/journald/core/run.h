/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_RUN_H_
#define _JOURNALD_RUN_H_


int run_group(const char *group);
int run_user(const char *user);

#endif	/* _JOURNALD_SYSLOG_H_ */
