/*
 * Copyright © 2015 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _BOOT_H_
#define _BOOT_H_

#include "uuid.h"


int journal_get_bootid(uuid_t* boot_id);

#endif	/* _BOOT_H_ */
