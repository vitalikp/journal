/*
 * Copyright © 2018 - Vitaliy Perevertun
 *
 * This file is part of journal.
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _JOURNAL_MACROS_H_
#define _JOURNAL_MACROS_H_


#define ALIAS(name) __attribute__((weak,alias(#name)))

#define API __attribute__ ((visibility("default")))
#define PRIVATE_API __attribute__ ((visibility("hidden")))

#define JOURNAL_API(func) extern __typeof (func) func ALIAS(journal_ ## func) PRIVATE_API;

#endif	/* _JOURNAL_MACROS_H_ */