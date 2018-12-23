#ifndef RAND_H
#define RAND_H

#include <limits.h>

#define DEFENSIVE_RAND_MAX ULONG_MAX

void defensive_srand(unsigned long);
unsigned long defensive_seed(void);
unsigned long defensive_rand();

#endif
