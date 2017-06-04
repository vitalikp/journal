/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _KMSG_PRIVATE_H_
#define _KMSG_PRIVATE_H_

#include "core/kmsg.h"


ssize_t kmsg_read(int fd, msg_t *msg);

#endif	/* _KMSG_PRIVATE_H_ */