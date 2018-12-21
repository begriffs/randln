#include "rand.h"

#include <stdio.h>
#include <string.h>

/* We'll use the address of main() for entropy.
 * Assumption: main is declared to take args.
 */
extern int main(int, char **);

static unsigned long mix(unsigned long, unsigned long);
static unsigned long djb2hash(const unsigned char *);

/* Holds previous pseudo-random number in sequence.
 * Assumption: no multi-threaded access.
 */
static unsigned long g_rand_state = 0;

/*
 * Create a seed value value that changes fairly dramatically, even if
 * the epoch has not advanced much since the last time defensive_seed()
 * was called. (In case our program is executed more than once per
 * second.)
 *
 * Internally it pulls from two sources of entropy that don't require
 * any POSIX functions. The first source is a hash of tmpnam(), which
 * in many implementations consults the process ID or a higher precision
 * clock. The second is the address of main(), which is often fairly
 * unpredictable due to address space layout randomization (ASLR).
 */
unsigned long defensive_seed(void)
{
	/* two portable sources of entropy */
	unsigned long src_a, src_b;

	int (*p)(int, char**) = &main;
	unsigned char bytes[sizeof(p) + 1] = { 0 };

	/* use tmpnam not for creating a file, but for the entropy
	 * that it gets from getpid(2), arc4random(2), time(3), or
	 * glib's highres clock depending on the implementation */
	src_a = djb2hash((unsigned char*)tmpnam(NULL));

	/* the address of main() can also differ per-execution due
	 * to address space layout randomization (ASLR) */
	memcpy(bytes, (void*)&p, sizeof(p));
	src_b = djb2hash(bytes);

	return mix(src_a, src_b);
}

/*
 * Alternative to the garbage implementation of rand() in many
 * systems such as FreeBSD and OS X. Uses xorshift.
 *
 * Returns a value between 0 and DEFENSIVE_RAND_MAX, which on
 * many platforms is smaller than what unsigned long can hold.
 */
unsigned long defensive_rand()
{
	/*
	 * The long type should portably be at least 32 bits, and we
	 * mask the calculations to 32 bits in case long happens to be
	 * wider. The constants are tuned for a 32-bit type.
	 */
	unsigned long y = g_rand_state & DEFENSIVE_RAND_MAX;
	y ^= (y << 13) & DEFENSIVE_RAND_MAX;
	y ^= y >> 17;
	y ^= (y << 5) & DEFENSIVE_RAND_MAX;
	return (g_rand_state = y);
}

void defensive_srand(unsigned long s)
{
	g_rand_state = s;
}

unsigned long djb2hash(const unsigned char *str)
{
	unsigned long hash = 5381;
	int c;

	if (str)
		while ( (c = *str++) )
			hash = hash * 33 + c;
	return hash;
}

/* It's better to combine entropic values this way rather
 * than e.g. adding them together.
 */
unsigned long mix(unsigned long x, unsigned long y)
{
	unsigned long result = 0xca01f9dd*x - 0x4973f715*y;
	result ^= result >> 16;
	return result;
}
