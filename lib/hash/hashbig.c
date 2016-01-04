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
 * hashbig():
 * This is the same as hashword() on big-endian machines.  It is different
 * from hashlittle() on all machines.  hashbig() takes advantage of
 * big-endian byte ordering.
 */
uint32_t hashbig( const void *key, size_t length, uint32_t initval)
{
  uint32_t a,b,c;
  union { const void *ptr; size_t i; } u; /* to cast key to (size_t) happily */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_BIG_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */
    const uint8_t  *k8;

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /*
     * "k[2]<<8" actually reads beyond the end of the string, but
     * then shifts out the part it's not allowed to read.  Because the
     * string is aligned, the illegal read is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff00; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff0000; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff000000; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff00; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff0000; a+=k[0]; break;
    case 5 : b+=k[1]&0xff000000; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff00; break;
    case 2 : a+=k[0]&0xffff0000; break;
    case 1 : a+=k[0]&0xff000000; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }

#else  /* make valgrind happy */

    k8 = (const uint8_t *)k;
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<8;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<16;  /* fall through */
    case 9 : c+=((uint32_t)k8[8])<<24;  /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<8;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<16;  /* fall through */
    case 5 : b+=((uint32_t)k8[4])<<24;  /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<8;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<16;  /* fall through */
    case 1 : a+=((uint32_t)k8[0])<<24; break;
    case 0 : return c;
    }

#endif /* !VALGRIND */

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += ((uint32_t)k[0])<<24;
      a += ((uint32_t)k[1])<<16;
      a += ((uint32_t)k[2])<<8;
      a += ((uint32_t)k[3]);
      b += ((uint32_t)k[4])<<24;
      b += ((uint32_t)k[5])<<16;
      b += ((uint32_t)k[6])<<8;
      b += ((uint32_t)k[7]);
      c += ((uint32_t)k[8])<<24;
      c += ((uint32_t)k[9])<<16;
      c += ((uint32_t)k[10])<<8;
      c += ((uint32_t)k[11]);
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=k[11];
    case 11: c+=((uint32_t)k[10])<<8;
    case 10: c+=((uint32_t)k[9])<<16;
    case 9 : c+=((uint32_t)k[8])<<24;
    case 8 : b+=k[7];
    case 7 : b+=((uint32_t)k[6])<<8;
    case 6 : b+=((uint32_t)k[5])<<16;
    case 5 : b+=((uint32_t)k[4])<<24;
    case 4 : a+=k[3];
    case 3 : a+=((uint32_t)k[2])<<8;
    case 2 : a+=((uint32_t)k[1])<<16;
    case 1 : a+=((uint32_t)k[0])<<24;
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}

#ifdef TESTS
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

int main()
{
	uint32_t hash;
	const uint8_t value[] = "hashbig value ... hashbig value ...";

	hash = hashbig(value, 8, 17);
	assert(hash == 0x65bb5600);

	hash = hashbig(value, 7, 17);
	assert(hash == 0x6c6d2a07);

	hash = hashbig(value, 3, 17);
	assert(hash == 0x194a449a);

	return EXIT_SUCCESS;
}

#endif
