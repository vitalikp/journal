/*
 * Copyright © 2017 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#ifndef _UTILS_STR_H_
#define _UTILS_STR_H_

#include <stddef.h>
#include <stdint.h>


/**
 * str_empty:
 * @str: input string
 *
 * Returns: 1 if string str is empty, or 0 otherwise
 */
static inline int str_empty(const char *str)
{
	return (!str || *str == '\0');
}

/**
 * str_eq:
 * @str1: input string 1
 * @str2: input string 2
 *
 * Returns: 1 if string str is equal, or 0 otherwise
 */
static inline int str_eq(const char *str1, const char *str2)
{
	if (!str1 || !str2)
		return str1 == str2;

	do
	{
		if (*str1 != *str2++)
			return 0;

	} while (*str1++);

	return 1;
}

/**
 * str_caseeq:
 * @str1: input string 1
 * @str2: input string 2
 *
 * Returns: 1 if string str is equal without case sensitive, or 0 otherwise
 */
static inline int str_caseeq(const char *str1, const char *str2)
{
	char val;

	if (!str1 || !str2)
		return str1 == str2;

	do
	{
		val = *str2++;
		if (val > 0x60 && val < 0x7b)
			val -= 0x20;

		if (*str1 > 0x60 && *str1 < 0x7b)
			val += 0x20;

		if (*str1 != val)
			return 0;

	} while (*str1++);

	return 1;
}

/**
 * str_copy:
 * @dst: destination string
 * @src: source string
 * @size: maximum size of the destination string
 *
 * Copy source string src to destination dst with length size-1
 * and put zero at the end.
 *
 * Returns: length of destination string
 */
static inline size_t str_copy(char *dst, const char *src, size_t size)
{
	size_t len = size;

	while (*src && size > 1)
	{
		*dst++ = *src++;
		size--;
	}

	*dst = '\0';

	return len - size;
}

#endif	/* _UTILS_STR_H_ */