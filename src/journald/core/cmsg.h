/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_CMSG_H_
#define _JOURNALD_CMSG_H_

#include "core/msg.h"


void cmsg_decode(msg_t *msg, uint8_t *buf, size_t size);

#endif	/* _JOURNALD_CMSG_H_ */
