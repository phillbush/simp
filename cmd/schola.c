#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "schola.h"

static char *progname;

static void
err(int exitval, const char *fmt, ...)
{
	extern char *progname;
	va_list ap;
	int sverrno;

	sverrno = errno;
	if (progname != NULL)
		(void)fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	va_end(ap);
	if (sverrno != 0)
		fprintf(stderr, "%s", strerror(sverrno));
	fprintf(stderr, "\n");
	exit(exitval);
}

static void
errx(int exitval, const char *fmt, ...)
{
	extern char *progname;
	va_list ap;

	if (progname != NULL)
		(void)fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	if (fmt != NULL)
		(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	exit(exitval);
}

static schola_context
init(int argc, char *argv[])
{
	schola_context ctx;

	(void)argc;
	(void)argv;
	if ((ctx = schola_init()) == NULL)
		errx(1, "could not initialize");
	schola_interactive(ctx, stdin, stdout, stderr);
	return ctx;
}

static schola_cell
xread(schola_context ctx)
{
	printf("> ");
	fflush(stdout);
	return schola_read(ctx);
}

static schola_cell
eval(schola_context ctx, schola_cell cell)
{
	return schola_eval(ctx, cell);
}

static int
print(schola_context ctx, schola_cell cell)
{
	if (schola_eof_p(ctx, cell)) {
		printf("\n");
		return 0;
	}
	(void)schola_write(ctx, cell);
	printf("\n");
	fflush(stdout);
	return 1;
}

static int
clean(schola_context ctx)
{
	(void)ctx;
	// TODO
	return 0;
}

int
main(int argc, char *argv[])
{
	schola_context ctx;

	progname = *argv;
	ctx = init(argc, argv);
	while (print(ctx, eval(ctx, xread(ctx))))
		;
	return clean(ctx);
}
