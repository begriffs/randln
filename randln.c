#include "flexar.h"
#include "rand.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

	if (fseek(fp, 0, SEEK_END) != 0)
		die_with_error(NULL);
	if ((filesz = ftell(fp)) == -1)
		die_with_error(NULL);

	pos = (int)((double)defensive_rand() / ((double)DEFENSIVE_RAND_MAX + 1) * filesz);

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

int via_replacement(const char *filename)
{
	FILE *fp = fopen_or_die(filename, "r");
	struct flexarray *line = flex_new();
	long double n = 1.0, rand01;
	int c;

	do
	{
		rand01 = ((long double)defensive_rand()) / DEFENSIVE_RAND_MAX;
		if (rand01 <= 1/n)
		{
			flex_trunc(line); /* overwrite line */
			while ((c = getc(fp)) != EOF && c != '\n')
				if (!flex_append(line, (char)c))
				{
					flex_free(line);
					return -1;
				}
		}
		else
			eatline(fp);
		n++;
	} while (!feof(fp));

	puts(line->val);
	flex_free(line);
	return 0;
}

int via_replacement_fpos(const char *filename)
{
	FILE *fp = fopen_or_die(filename, "r");
	fpos_t line;
	long double n = 1.0, rand01;

	do
	{
		rand01 = ((long double)defensive_rand()) / DEFENSIVE_RAND_MAX;
		if (rand01 <= 1/n)
			if (fgetpos(fp, &line) != 0)
				die_with_error(NULL);
		eatline(fp);
		n++;
	} while (!feof(fp));

	if (fsetpos(fp, &line) != 0)
		die_with_error(NULL);
	echoline(fp);
	return 0;
}

void via_poisson(double prob, const char *filename)
{
	FILE *fp;

	assert(0 < prob && prob <= 1);

	fp = fopen_or_die(filename, "r");
	while (defensive_rand() > DEFENSIVE_RAND_MAX * prob)
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
	printf("usage: %s [-m(f|r|R|p)] [-p(fff)] filename\n", prog);
}

int main(int argc, char **argv)
{
	const char *filename = NULL, *flag, *prog = argv[0];
	double poisson_probability = 1e-4;
	enum {
		VIA_FSEEK     = 'f', VIA_REPL = 'r',
		VIA_FPOS_REPL = 'R', VIA_POISSON = 'p'
	} method = VIA_FSEEK;

	defensive_srand(defensive_seed());

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
		case VIA_REPL:
			via_replacement(filename);
			break;
		case VIA_FPOS_REPL:
			via_replacement_fpos(filename);
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
