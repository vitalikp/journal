/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNALD_SEQNUM_H_
#define _JOURNALD_SEQNUM_H_

#include <stdint.h>


int seqnum_load(const char *fn, uint64_t *pseqnum);
int seqnum_save(const char *fn, uint64_t *pseqnum);

#endif	/* _JOURNALD_SEQNUM_H_ */
