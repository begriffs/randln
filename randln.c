#include "flexar.h"
#include "rand.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

void die_perror(const char *s)
{
    perror(s);
    exit(EXIT_FAILURE);
}

void die_error(const char *s)
{
	fputs(s, stderr);
	exit(EXIT_FAILURE);
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

void via_fseek(FILE *fp)
{
	long filesz, pos;
	int c;

	if (fseek(fp, 0, SEEK_END) != 0)
		die_perror(NULL);
	if ((filesz = ftell(fp)) == -1)
		die_perror(NULL);

	pos = (int)((double)defensive_rand() / ((double)DEFENSIVE_RAND_MAX + 1) * filesz);

	if (fseek(fp, pos, SEEK_SET) != 0)
		die_perror(NULL);
	eatline(fp);

	/* if we hit the end, wrap to start */
	if ((c = getc(fp)) == EOF)
		rewind(fp);
	else
		ungetc(c, fp);

	echoline(fp);
}

void via_replacement(FILE *fp)
{
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
					die_error("Failed to realloc line buffer");
				}
			/* probe for EOF */
			if ((c = getc(fp)) != EOF)
				ungetc(c, fp);
		}
		else
			eatline(fp);
		n++;
	} while (!feof(fp));

	puts(line->val);
	flex_free(line);
}

void via_replacement_fpos(FILE *fp)
{
	fpos_t line;
	long double n = 1.0, rand01;

	do
	{
		rand01 = ((long double)defensive_rand()) / DEFENSIVE_RAND_MAX;
		if (rand01 <= 1/n)
			if (fgetpos(fp, &line) != 0)
				die_perror(NULL);
		eatline(fp);
		n++;
	} while (!feof(fp));

	if (fsetpos(fp, &line) != 0)
		die_perror(NULL);
	echoline(fp);
}

void via_poisson(double prob, FILE *fp)
{
	assert(0 < prob && prob <= 1);

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
	FILE *fp;

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

	fp = (filename == NULL || strcmp("-", filename) == 0)
		? stdin
		: fopen(filename,
			method == VIA_FSEEK ? "rb" : "r"
		  );
	if (!fp)
	{
		perror(filename);
		exit(EXIT_FAILURE);
	}

	switch (method)
	{
		case VIA_FSEEK:
			via_fseek(fp);
			break;
		case VIA_REPL:
			via_replacement(fp);
			break;
		case VIA_FPOS_REPL:
			via_replacement_fpos(fp);
			break;
		case VIA_POISSON:
			via_poisson(poisson_probability, fp);
			break;
		default:
			fprintf(stderr, "Unknown method: %c\n", (char)method);
			usage(prog);
			exit(EXIT_FAILURE);
	}
	fclose(fp);

	return EXIT_SUCCESS;
}
