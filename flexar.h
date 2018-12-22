#ifndef FLEXAR_H
#define FLEXAR_H

#include <stddef.h>

struct flexarray
{
	size_t len;
	size_t max;
	char *val;
};

struct flexarray *flex_new(void);
char *flex_append(struct flexarray *, char);
void flex_free(struct flexarray *);
void flex_trunc(struct flexarray *);

#endif
