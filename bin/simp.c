#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "simp.h"

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

static simp_context
init(int argc, char *argv[])
{
	simp_context ctx;

	(void)argc;
	(void)argv;
	if ((ctx = simp_init()) == NULL)
		errx(1, "could not initialize");
	simp_interactive(ctx, stdin, stdout, stderr);
	return ctx;
}

static simp_cell
xread(simp_context ctx)
{
	printf("> ");
	fflush(stdout);
	return simp_read(ctx);
}

static simp_cell
eval(simp_context ctx, simp_cell cell)
{
	return simp_eval(ctx, cell);
}

static int
print(simp_context ctx, simp_cell cell)
{
	if (simp_eof_p(ctx, cell)) {
		printf("\n");
		return 0;
	}
	(void)simp_write(ctx, cell);
	printf("\n");
	fflush(stdout);
	return 1;
}

static int
clean(simp_context ctx)
{
	(void)ctx;
	// TODO
	return 0;
}

int
main(int argc, char *argv[])
{
	simp_context ctx;

	progname = *argv;
	ctx = init(argc, argv);
	while (print(ctx, eval(ctx, xread(ctx))))
		;
	return clean(ctx);
}
