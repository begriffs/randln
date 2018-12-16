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

	/* djb2 hashing */
	while ( (c = *str++) )
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	srand(hash + time(NULL));
}

/* output until \n or EOF */
void fputline(FILE *fp)
{
	int c;
	while ((c = fgetc(fp)) != EOF)
	{
		putchar(c);
		if (c == '\n')
			break;
	}
}

/* advance to next full line */
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
	FILE *fp = fopen(filename, "rb");
	long filesz, pos;
	int c;

	fseek(fp, 0, SEEK_END);
	filesz = ftell(fp);

	pos = (int)((double)rand() / ((double)RAND_MAX + 1) * filesz);

	fseek(fp, pos, SEEK_SET);
	eatline(fp);

	/* if we hit the end, wrap to start */
	if ((c = fgetc(fp)) == EOF)
		rewind(fp);
	else
		ungetc(c, fp);

	fputline(fp);
	fclose(fp);
}

void via_bookmarks(const char* filename)
{
	size_t nalloc = 1, nlines = 0, line;
	fpos_t *bookmarks = malloc(sizeof(fpos_t));
	FILE *fp = fopen(filename, "r");

	/* scan whole file, set bookmarks */
	do
	{
		fgetpos(fp, &bookmarks[nlines++]);
		if (nlines >= nalloc)
		{
			nalloc *= 2;
			bookmarks = realloc(
				bookmarks, nalloc * sizeof(fpos_t));
		}
		eatline(fp);
	} while (!feof(fp));

	line = ((double)rand() / ((double)RAND_MAX + 1) * nlines);
	fsetpos(fp, &bookmarks[line]);

	fputline(fp);
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
	
	FILE *fp = fopen(filename, "r");

	fgetpos(fp, bm++);
	do
	{
		if(nlines++ >= nextmark)
		{
			nextmark *= 2;
			fgetpos(fp, bm++);
		}
		eatline(fp);
	} while (!feof(fp));

	line = ((double)rand() / ((double)RAND_MAX + 1) * nlines);
	bm = &bookmarks[(size_t)floor(log(line)/log(2))];

	fsetpos(fp, bm);
	catchup = exp2(bm - bookmarks);
	while (catchup++ < line)
		eatline(fp);
	fputline(fp);
	fclose(fp);
}

int main(int argc, char **argv)
{
	(void)argc;

	defensive_srand();

	via_fseek(argv[1]);
	/* via_bookmarks(argv[1]); */
	/* via_expmarks(argv[1]); */

	return EXIT_SUCCESS;
}
