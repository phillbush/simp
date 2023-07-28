#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "simp.h"

extern unsigned char _binary_stdlib_lisp_start[];
extern unsigned char _binary_stdlib_lisp_end[];

static void
usage(void)
{
	(void)fprintf(stderr, "usage: simp [-i] [-e string | -p string | file]");
}

static int
rel(Simp ctx, Simp iport)
{
	Simp eport, obj, env;

	env = simp_contextenvironment(ctx);
	eport = simp_contexteport(ctx);
	for (;;) {
		simp_gc(ctx);
		if (simp_porterr(ctx, iport))
			break;
		obj = simp_read(ctx, iport);
		if (simp_iseof(ctx, obj))
			break;
		if (simp_isexception(ctx, obj)) {
			simp_write(ctx, eport, obj);
			simp_printf(ctx, eport, "\n");
			return EXIT_FAILURE;
		}
		obj = simp_eval(ctx, obj, env);
		if (simp_isexception(ctx, obj)) {
			simp_write(ctx, eport, obj);
			simp_printf(ctx, eport, "\n");
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

static int
repl(Simp ctx, Simp iport, int prompt)
{
	Simp oport, eport, obj, env;

	env = simp_contextenvironment(ctx);
	oport = simp_contextoport(ctx);
	eport = simp_contexteport(ctx);
	for (;;) {
		simp_gc(ctx);
		if (simp_porterr(ctx, iport))
			return EXIT_FAILURE;
		if (prompt)
			simp_printf(ctx, oport, "> ");
		obj = simp_read(ctx, iport);
		if (simp_iseof(ctx, obj))
			break;
		if (simp_isexception(ctx, obj)) {
			simp_write(ctx, eport, obj);
			goto newline;
		}
		obj = simp_eval(ctx, obj, env);
		if (simp_isexception(ctx, obj)) {
			simp_write(ctx, eport, obj);
			goto newline;
		}
		simp_write(ctx, oport, obj);
newline:
		simp_printf(ctx, oport, "\n");
	}
	return EXIT_SUCCESS;
}

int
main(int argc, char *argv[])
{
	enum { MODE_INTERACTIVE, MODE_STRING, MODE_PRINT, MODE_SCRIPT } mode;
	FILE *fp;
	Simp ctx, iport, port;
	int ch, retval = EXIT_FAILURE;
	int iflag = 0;
	char *expr = NULL;

	mode = MODE_INTERACTIVE;
	while ((ch = getopt(argc, argv, "e:ip:")) != -1) switch (ch) {
	case 'e':
		mode = MODE_STRING;
		expr = optarg;
		break;
	case 'i':
		iflag = 1;
		break;
	case 'p':
		mode = MODE_PRINT;
		expr = optarg;
		break;
	default:
		usage();
	}
	argc -= optind;
	argv += optind;
	if (mode == MODE_INTERACTIVE && argc > 0)
		mode = MODE_SCRIPT;
	ctx = simp_contextnew();
	if (simp_isexception(simp_nil(), ctx))
		errx(EXIT_FAILURE, "could not create context");
	port = simp_openstring(
		ctx,
		_binary_stdlib_lisp_start,
		_binary_stdlib_lisp_end - _binary_stdlib_lisp_start,
		"r"
	);
	rel(ctx, port);
	iport = simp_contextiport(ctx);
	switch (mode) {
	case MODE_STRING:
		port = simp_openstring(ctx, (unsigned char *)expr, strlen(expr), "r");
		if (simp_isexception(ctx, port))
			goto error;
		rel(ctx, port);
		if (iflag)
			repl(ctx, iport, 1);
		break;
	case MODE_PRINT:
		port = simp_openstring(ctx, (unsigned char *)expr, strlen(expr), "r");
		if (simp_isexception(ctx, port))
			goto error;
		repl(ctx, port, 0);
		if (iflag)
			repl(ctx, iport, 1);
		break;
	case MODE_SCRIPT:
		if (argv[0][0] == '-' && argv[0][1] == '\0') {
			port = iport;
			iflag = 0;
		} else if ((fp = fopen(argv[0], "r")) != NULL) {
			port = simp_openstream(ctx, fp, "r");
		} else {
			goto error;
		}
		if (simp_isexception(ctx, port))
			goto error;
		rel(ctx, port);
		if (fp != stdin)
			(void)fclose(fp);
		if (iflag)
			repl(ctx, iport, 1);
		break;
	case MODE_INTERACTIVE:
		repl(ctx, iport, 1);
		break;
	}
	retval = EXIT_SUCCESS;
error:
	simp_gcfree(ctx);
	return retval;
}
