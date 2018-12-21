#ifndef RAND_H
#define RAND_H

#define DEFENSIVE_RAND_MAX 0xffffffffUL

void defensive_srand(unsigned long);
unsigned long defensive_seed(void);
unsigned long defensive_rand();

#endif
