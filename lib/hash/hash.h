/*
 * Copyright © 2016 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#if !defined (_JOURNAL_H_INSIDE_) && !defined (JOURNAL_COMPILATION)
#	error "Only <journal/journal.h> can be included directly."
#endif

#ifndef _JOURNAL_HASH_H_
#define _JOURNAL_HASH_H_

#include <inttypes.h>
#include <sys/types.h>

#include "lookup3.h"


static inline void hash64(const void* data, size_t len, uint64_t* hash)
{
	*hash = 0;
	hashlittle2(data, len, ((uint32_t*)hash)+1,((uint32_t*)hash));
}

#endif /* _JOURNAL_HASH_H_ */
