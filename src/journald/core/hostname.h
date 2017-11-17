/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_HOSTNAME_H_
#define _JOURNALD_HOSTNAME_H_

#include "server.h"


int hostname_open(server_t* s);
int hostname_read(int fd, char* hostname);

#endif	/* _JOURNALD_HOSTNAME_H_ */
