/*
 * lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 *
 * This file is part of lookup3.c
 * Original source file can be found at
 * http://www.burtleburtle.net/bob/c/lookup3.c
 *
 * See article here http://www.burtleburtle.net/bob/hash/doobs.html
 */

#include "lookup3.h"


/*
--------------------------------------------------------------------
 This works on all machines.  To be useful, it requires
 -- that the key be an array of uint32_t's, and
 -- that the length be the number of uint32_t's in the key

 The function hashword() is identical to hashlittle() on little-endian
 machines, and identical to hashbig() on big-endian machines,
 except that the length has to be measured in uint32_ts rather than in
 bytes.  hashlittle() is more complicated than hashword() only because
 hashlittle() has to dance around fitting the key bytes into registers.
--------------------------------------------------------------------
*/
uint32_t hashword(
const uint32_t *k,                   /* the key, an array of uint32_t values */
size_t          length,               /* the length of the key, in uint32_ts */
uint32_t        initval)         /* the previous hash, or an arbitrary value */
{
  uint32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + (((uint32_t)length)<<2) + initval;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch(length)                     /* all the case statements fall through */
  {
  case 3 : c+=k[2];
  case 2 : b+=k[1];
  case 1 : a+=k[0];
    final(a,b,c);
  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  return c;
}

#ifdef TESTS
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>


int main()
{
	uint32_t hash;
	const uint8_t value[] = "hashword value ... hashword value ...";

	hash = hashword((const uint32_t *)value, 8, 17);
	assert(hash == 0x5f00134c);

	hash = hashword((const uint32_t *)value, 7, 17);
	assert(hash == 0xd872b6d5);

	hash = hashword((const uint32_t *)value, 3, 17);
	assert(hash == 0x1f99cd19);

	return EXIT_SUCCESS;
}

#endif
