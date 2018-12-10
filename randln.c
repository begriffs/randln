#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* in case program is executed more than once per second,
 * and to defend aganst crappy RNGs *cough OS X.*
 *
 * Uses tmpnam() as a source of extra randomness, to avoid
 * needing to rely on POSIX things like the process PID.
 */
void defensive_srand(void)
{
	unsigned long hash = 5381;
	char *str = tmpnam(NULL);
	int c;

	/* djb2 hashing */
	while ( (c = *str++) )
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	srand(hash + time(NULL));
}

int main(int argc, char **argv)
{
	FILE *fp = fopen(argv[1], "rb");
	long filesz, pos;
	int c;

	(void)argc;

	fseek(fp, 0, SEEK_END);
	filesz = ftell(fp);

	defensive_srand();
	pos = (int)((double)rand() / ((double)RAND_MAX + 1) * filesz);

	fprintf(stderr, "pos: %ld\n", pos);
	fseek(fp, pos, SEEK_SET);

	/* advance to next full line */
	while ((c = fgetc(fp)) != EOF && c != '\n')
		;

	/* if we hit the end, wrap to start */
	if ((c = fgetc(fp)) == EOF)
		rewind(fp);
	else
		ungetc(c, fp);

	while ((c = fgetc(fp)) != EOF)
	{
		putchar(c);
		if (c == '\n')
			break;
	}

	fclose(fp);

	return EXIT_SUCCESS;
}
