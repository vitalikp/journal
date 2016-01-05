/*
 * Copyright © 2016 - Vitaliy Perevertun
 *
 * This file is part of journal
 *
 * This file is licensed under the MIT license.
 * See the file LICENSE.
 */

#include "hash.h"


#ifdef TESTS
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

int main()
{
	uint64_t hash;
	const uint8_t value[] = "hash value ... hash value ... ";

	hash64(value, 30, &hash);
	assert(hash == 0x34436c8269eb4c47);

	return EXIT_SUCCESS;
}

#endif
