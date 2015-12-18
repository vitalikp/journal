/*
 * Copyright © 2015 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#if !defined (_JOURNAL_H_INSIDE_) && !defined (JOURNAL_COMPILATION)
#	error "Only <journal/journal.h> can be included directly."
#endif

#ifndef _JOURNAL_UUID_H_
#define _JOURNAL_UUID_H_

#include <stdint.h>


/*
 * RFC4122 http://tools.ietf.org/html/rfc4122
 */
typedef union uuid_t
{
	uint8_t		bytes[16];
	uint64_t	qwords[2];
} uuid_t;


int uuid_equal(const uuid_t id1, const uuid_t id2);
int uuid_is_null(const uuid_t id1);

void uuid_gen_rand(uuid_t* id);

char* uuid_to_str(uuid_t id, char str[33]);
int uuid_parse(const char* in, uuid_t* id);

#endif	/* _JOURNAL_UUID_H_ */
