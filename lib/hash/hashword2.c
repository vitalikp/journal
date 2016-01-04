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
hashword2() -- same as hashword(), but take two seeds and return two
32-bit values.  pc and pb must both be nonnull, and *pc and *pb must
both be initialized with seeds.  If you pass in (*pb)==0, the output
(*pc) will be the same as the return value from hashword().
--------------------------------------------------------------------
*/
void hashword2 (
const uint32_t *k,                   /* the key, an array of uint32_t values */
size_t          length,               /* the length of the key, in uint32_ts */
uint32_t       *pc,                      /* IN: seed OUT: primary hash value */
uint32_t       *pb)               /* IN: more seed OUT: secondary hash value */
{
  uint32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)(length<<2)) + *pc;
  c += *pb;

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
  *pc=c; *pb=b;
}

#ifdef TESTS
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

int main()
{
	uint32_t hash1, hash2;
	const uint8_t value[] = "hashword value ... hashword value ...";

	hash1 = 17;
	hash2 = 0;
	hashword2((const uint32_t *)value, 8, &hash1, &hash2);
	assert(hash1 == 0x5f00134c);
	assert(hash2 == 0x6fcf0c30);

	hash1 = 17;
	hash2 = 0;
	hashword2((const uint32_t *)value, 7, &hash1, &hash2);
	assert(hash1 == 0xd872b6d5);
	assert(hash2 == 0xeb8c224e);

	hash1 = 17;
	hash2 = 0;
	hashword2((const uint32_t *)value, 3, &hash1, &hash2);
	assert(hash1 == 0x1f99cd19);
	assert(hash2 == 0x9da41c7e);

	return EXIT_SUCCESS;
}

#endif
