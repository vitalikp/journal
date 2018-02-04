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

#define JORNAL_ATTR_PRINTF(fmt, args) \
	__attribute__ ((format(printf, fmt, args)))

#define _JOURNAL_H_INSIDE_

#include <sd-journal.h>

#undef _JOURNAL_H_INSIDE_

#endif /* _JOURNAL_H_ */
