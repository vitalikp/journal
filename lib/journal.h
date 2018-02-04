/*
 * Copyright © 2015-2018 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNAL_H_
#define _JOURNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define JORNAL_ATTR_PRINTF(fmt, args) \
	__attribute__ ((format(printf, fmt, args)))

#define JORNAL_ATTR_SENTINEL \
	__attribute__((sentinel))


typedef union uuid uuid_t;


/**
 * journal utils
 */

char* journal_uuid_to_str(uuid_t id, char str[33]);

#include <sd-journal.h>

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _JOURNAL_H_ */
