/*
 * Copyright © 2017-2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_RUN_H_
#define _JOURNALD_RUN_H_

#include <sys/types.h>


int run_mkdir(const char *dn);
void run_group(const char *group, gid_t *gid);
void run_user(const char *user, uid_t *uid, gid_t *gid);
int run_chgroup(gid_t gid);
int run_chuser(uid_t uid);

#endif	/* _JOURNALD_SYSLOG_H_ */
