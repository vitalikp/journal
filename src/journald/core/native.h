/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_NATIVE_H_
#define _JOURNALD_NATIVE_H_

#include "server.h"


int native_open(server_t *s, event_cb callback);
void native_close(server_t *s);

#endif	/* _JOURNALD_NATIVE_H_ */
