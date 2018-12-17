#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Calls srand() with a value that changes fairly dramatically even
 * if the system time has not advanced much since the last time
 * defensive_srand() was called.
 *
 * This is in case our program is executed more than once per second,
 * and to defend aganst crappy RNGs whose first rand() value doesn't
 * change much when the seed doesn't. (cough, OS X...)
 *
 * Internally it hashes tmpnam() to add to the seed, to avoid needing to
 * rely on POSIX things like the process PID.
 */
void defensive_srand(void)
{
	unsigned long hash = 5381;
	char *str = tmpnam(NULL);
	int c;

	if (str != NULL)
	{
		/* djb2 hashing */
		while ( (c = *str++) )
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	srand(hash + time(NULL));
}

void die_with_error(void)
{
	perror(NULL);
	exit(EXIT_FAILURE);
}

FILE *fopen_or_die(const char *p, const char *m)
{
	FILE *fp = fopen(p, m);
	if (!fp)
		die_with_error();
	return fp;
}

/* output until \n or EOF */
void echoline(FILE *fp)
{
	int c;
	while ((c = fgetc(fp)) != EOF)
	{
		putchar(c);
		if (c == '\n')
			break;
	}
}

/* advance to next full line, set EOF if last */
void eatline(FILE *fp)
{
	int c;
	while ((c = fgetc(fp)) != EOF && c != '\n')
		;

	/* probe for EOF */
	if ((c= fgetc(fp)) != EOF)
		ungetc(c, fp);
}

void via_fseek(const char* filename)
{
	FILE *fp = fopen_or_die(filename, "rb");
	long filesz, pos;
	int c;

	/* check that stream supports seeking */
	if (fseek(fp, 0, SEEK_END) != 0)
		die_with_error();
	if ((filesz = ftell(fp)) == -1)
		die_with_error();

	pos = (int)((double)rand() / ((double)RAND_MAX + 1) * filesz);

	if (fseek(fp, pos, SEEK_SET) != 0)
		die_with_error();
	eatline(fp);

	/* if we hit the end, wrap to start */
	if ((c = fgetc(fp)) == EOF)
		rewind(fp);
	else
		ungetc(c, fp);

	echoline(fp);
	fclose(fp);
}

void via_bookmarks(const char* filename)
{
	size_t number_allocated = 1, nlines = 0, line;
	fpos_t *bookmarks = malloc(sizeof(fpos_t));
	FILE *fp = fopen_or_die(filename, "r");

	/* scan whole file, set bookmarks */
	do
	{
		if (fgetpos(fp, &bookmarks[nlines++]) != 0)
			die_with_error();
		if (nlines >= number_allocated)
		{
			number_allocated *= 2;
			bookmarks = realloc(
				bookmarks, number_allocated * sizeof(fpos_t));
			if (bookmarks == NULL)
				die_with_error();
		}
		eatline(fp);
	} while (!feof(fp));

	line = round(((double)rand() / ((double)RAND_MAX + 1) * nlines));
	if (fsetpos(fp, &bookmarks[line]) != 0)
		die_with_error();

	echoline(fp);
	free(bookmarks);
	fclose(fp);
}

/* bookmark lines 1,2,4,8 ... */
void via_expmarks(const char* filename)
{
	/* up to 2^64 lines */
	fpos_t bookmarks[64], *bm = bookmarks;
	/* limited by this guy */
	unsigned long nlines = 0, nextmark = 1,
				  catchup, line;
	
	FILE *fp = fopen_or_die(filename, "r");

	if (fgetpos(fp, bm++) != 0)
		die_with_error();
	do
	{
		if(nlines++ >= nextmark)
		{
			nextmark *= 2;
			if (fgetpos(fp, bm++) != 0)
				die_with_error();
		}
		eatline(fp);
	} while (!feof(fp));

	line = ((double)rand() / ((double)RAND_MAX + 1) * nlines);
	bm = &bookmarks[(size_t)floor(log(line)/log(2))];

	if (fsetpos(fp, bm) != 0)
		die_with_error();
	catchup = exp2(bm - bookmarks);
	while (catchup++ < line)
		eatline(fp);
	echoline(fp);
	fclose(fp);
}

void via_poisson(double prob, const char *filename)
{
	int limbo;
	FILE *fp;

	assert(0 < prob && prob <= 1);

	fp = fopen_or_die(filename, "r");
	limbo = round(RAND_MAX * prob);

	while (rand() > limbo)
	{
		if (feof(fp))
		{
			/* try to rewind if stream supports it */
			if (fseek(fp, 0, SEEK_SET) != 0)
				break;
		}
		eatline(fp);
	}
	echoline(fp);
	fclose(fp);
}

int main(int argc, char **argv)
{
	(void)argc;

	defensive_srand();

	via_fseek(argv[1]);
	/* via_bookmarks(argv[1]); */
	/* via_expmarks(argv[1]); */
	/* via_poisson(0.000001, argv[1]); */

	return EXIT_SUCCESS;
}
