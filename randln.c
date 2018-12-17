#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void die_with_error(const char *s)
{
	perror(s);
	exit(EXIT_FAILURE);
}

FILE *fopen_or_die(const char *p, const char *m)
{
	FILE *fp;
	
	/* "-" is a special form meaning stdin */
	fp = (strcmp("-", p) == 0)
		? freopen(NULL, m, stdin)
		: fopen(p, m);

	if (!fp)
		die_with_error(p);
	return fp;
}

/* output until \n or EOF */
void echoline(FILE *fp)
{
	int c;
	while ((c = getc(fp)) != EOF)
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
	while ((c = getc(fp)) != EOF && c != '\n')
		;

	/* probe for EOF */
	if ((c = getc(fp)) != EOF)
		ungetc(c, fp);
}

void via_fseek(const char* filename)
{
	FILE *fp = fopen_or_die(filename, "rb");
	long filesz, pos;
	int c;

	/* check that stream supports seeking */
	if (fseek(fp, 0, SEEK_END) != 0)
		die_with_error(NULL);
	if ((filesz = ftell(fp)) == -1)
		die_with_error(NULL);

	pos = (int)((double)rand() / ((double)RAND_MAX + 1) * filesz);

	if (fseek(fp, pos, SEEK_SET) != 0)
		die_with_error(NULL);
	eatline(fp);

	/* if we hit the end, wrap to start */
	if ((c = getc(fp)) == EOF)
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
			die_with_error(NULL);
		if (nlines >= number_allocated)
		{
			number_allocated *= 2;
			bookmarks = realloc(
				bookmarks, number_allocated * sizeof(fpos_t));
			if (bookmarks == NULL)
				die_with_error(NULL);
		}
		eatline(fp);
	} while (!feof(fp));

	line = round(((double)rand() / ((double)RAND_MAX + 1) * nlines));
	if (fsetpos(fp, &bookmarks[line]) != 0)
		die_with_error(NULL);

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
		die_with_error(NULL);
	do
	{
		if(nlines++ >= nextmark)
		{
			nextmark *= 2;
			if (fgetpos(fp, bm++) != 0)
				die_with_error(NULL);
		}
		eatline(fp);
	} while (!feof(fp));

	line = ((double)rand() / ((double)RAND_MAX + 1) * nlines);
	bm = &bookmarks[(size_t)floor(log(line)/log(2))];

	if (fsetpos(fp, bm) != 0)
		die_with_error(NULL);
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

void usage(const char *prog)
{
	printf("usage: %s [-m(f|b|e|p)] [-p(fff)] filename\n", prog);
}

int main(int argc, char **argv)
{
	const char *filename = NULL, *flag, *prog = argv[0];
	double poisson_probability = 1e-4;
	enum {
		VIA_FSEEK = 'f', VIA_BOOK    = 'b',
		VIA_EXP   = 'e', VIA_POISSON = 'p'
	} method = VIA_FSEEK;

	while (--argc)
	{
		flag = *++argv;
		if (flag[0] != '-')
			filename = flag;
		else
		{
			switch (flag[1])
			{
				case 'm':
					method = flag[2];
					break;
				case 'p':
					poisson_probability = strtod(flag+2, NULL);
					break;
				case '\0': /* "-" alone means stdin */
					filename = flag;
					break;
				case 'h':
					usage(prog);
					exit(EXIT_SUCCESS);
				default:
					fprintf(stderr, "Unknown flag: %c\n", flag[1]);
					exit(EXIT_FAILURE);
			}
		}
	}

	defensive_srand();

	if (filename == NULL)
	{
		fputs("Filename required\n", stderr);
		usage(prog);
		exit(EXIT_FAILURE);
	}

	switch (method)
	{
		case VIA_FSEEK:
			via_fseek(filename);
			break;
		case VIA_BOOK:
			via_bookmarks(filename);
			break;
		case VIA_EXP:
			via_expmarks(filename);
			break;
		case VIA_POISSON:
			via_poisson(poisson_probability, filename);
			break;
		default:
			fprintf(stderr, "Unknown method: %c\n", (char)method);
			usage(prog);
			exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
