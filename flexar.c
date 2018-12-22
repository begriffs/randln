#include "flexar.h"

#include <stdlib.h>

struct flexarray *flex_new(void)
{
	struct flexarray *a = malloc(sizeof(struct flexarray));
	const size_t initial = 64;

	if (!a)
		return NULL;
	/* always keep an extra item for NUL char */
	if (!(a->val = malloc(initial + 1)))
	{
		free(a);
		return NULL;
	}
	*a->val = '\0';
	a->len = 0;
	a->max = initial;
	return a;
}

char *flex_append(struct flexarray *a, char c)
{
	char *newbuf;

	if (a->len+1 >= a->max)
	{
		/* resize n+1 -> 2n+1 (= 2(n+1)-1) */
		if (!(newbuf = realloc(a->val, a->max*2 - 1)))
			return NULL;
		a->val = newbuf;
		a->max = a->max*2 - 1; 
	}
	a->val[a->len++] = c;
	/* write a NUL character ahead for better integration
	 * with standard C functions */
	a->val[a->len+1] = '\0';
	return a->val;
}

void flex_free(struct flexarray *a)
{
	if (a)
	{
		if (a->val)
			free(a->val);
		free(a);
	}
}

void flex_trunc(struct flexarray *a)
{
	*a->val = '\0';
	a->len = 0;
}
