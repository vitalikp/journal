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
 * hashlittle2: return 2 32-bit hash values
 *
 * This is identical to hashlittle(), except it returns two 32-bit hash
 * values instead of just one.  This is good enough for hash table
 * lookup with 2^^64 buckets, or if you want a second hash if you're not
 * happy with the first, or if you want a probably-unique 64-bit ID for
 * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 */
void hashlittle2(
  const void *key,       /* the key to hash */
  size_t      length,    /* length of the key */
  uint32_t   *pc,        /* IN: primary initval, OUT: primary hash */
  uint32_t   *pb)        /* IN: secondary initval, OUT: secondary hash */
{
  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + *pc;
  c += *pb;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
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
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticeably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#else /* make valgrind happy */

    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;	/* fall through */	// no break
    case 10: c+=((uint32_t)k8[9])<<8;	/* fall through */	// no break
    case 9 : c+=k8[8];					/* fall through */	// no break
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;	/* fall through */	// no break
    case 6 : b+=((uint32_t)k8[5])<<8;	/* fall through */	// no break
    case 5 : b+=k8[4];					/* fall through */	// no break
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;	/* fall through */	// no break
    case 2 : a+=((uint32_t)k8[1])<<8;	/* fall through */	// no break
    case 1 : a+=k8[0]; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;		/* fall through */	// no break
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];						/* fall through */	// no break
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;		/* fall through */	// no break
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];						/* fall through */	// no break
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;		/* fall through */	// no break
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;	/* fall through */	// no break
    case 11: c+=((uint32_t)k[10])<<16;	/* fall through */	// no break
    case 10: c+=((uint32_t)k[9])<<8;	/* fall through */	// no break
    case 9 : c+=k[8];					/* fall through */	// no break
    case 8 : b+=((uint32_t)k[7])<<24;	/* fall through */	// no break
    case 7 : b+=((uint32_t)k[6])<<16;	/* fall through */	// no break
    case 6 : b+=((uint32_t)k[5])<<8;	/* fall through */	// no break
    case 5 : b+=k[4];					/* fall through */	// no break
    case 4 : a+=((uint32_t)k[3])<<24;	/* fall through */	// no break
    case 3 : a+=((uint32_t)k[2])<<16;	/* fall through */	// no break
    case 2 : a+=((uint32_t)k[1])<<8;	/* fall through */	// no break
    case 1 : a+=k[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }
  }

  final(a,b,c);
  *pc=c; *pb=b;
}

#ifdef TESTS
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>



void test_empty(void)
{
	uint32_t hash1, hash2;

	hash1 = 0;
	hash2 = 0;
	hashlittle2("", 0, &hash1, &hash2);
	assert(hash1 == 0xdeadbeef);
	assert(hash2 == 0xdeadbeef);

	hash1 = 0;
	hash2 = 0xdeadbeef;
	hashlittle2("", 0, &hash1, &hash2);
	assert(hash1 == 0xbd5b7dde);
	assert(hash2 == 0xdeadbeef);

	hash1 = 0xdeadbeef;
	hash2 = 0xdeadbeef;
	hashlittle2("", 0, &hash1, &hash2);
	assert(hash1 == 0x9c093ccd);
	assert(hash2 == 0xbd5b7dde);
}

void test_hash(void)
{
	uint32_t hash1, hash2;
	const uint8_t value[] = "Four score and seven years ago";

	hash1 = 0;
	hash2 = 0;
	hashlittle2(value, 30, &hash1, &hash2);
	assert(hash1 == 0x17770551);
	assert(hash2 == 0xce7226e6);

	hash1 = 0;
	hash2 = 1;
	hashlittle2(value, 30, &hash1, &hash2);
	assert(hash1 == 0xe3607cae);
	assert(hash2 == 0xbd371de4);

	hash1 = 1;
	hash2 = 0;
	hashlittle2(value, 30, &hash1, &hash2);
	assert(hash1 == 0xcd628161);
	assert(hash2 == 0x6cbea4b3);
}

void test_hashword_hash(void)
{
	uint32_t hash1, hash2;
	const uint8_t value[] = "hashword value ... hashword value ...";

	hash1 = 17;
	hash2 = 0;
	hashlittle2(value, 32, &hash1, &hash2);
	assert(hash1 == 0x5f00134c);
	assert(hash2 == 0x6fcf0c30);

	hash1 = 17;
	hash2 = 0;
	hashlittle2(value, 28, &hash1, &hash2);
	assert(hash1 == 0xd872b6d5);
	assert(hash2 == 0xeb8c224e);

	hash1 = 17;
	hash2 = 0;
	hashlittle2(value, 12, &hash1, &hash2);
	assert(hash1 == 0x1f99cd19);
	assert(hash2 == 0x9da41c7e);
}

int main()
{
	test_empty();

	test_hash();

	test_hashword_hash();

	return EXIT_SUCCESS;
}

#endif
