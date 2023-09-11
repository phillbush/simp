#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "simp.h"

static void
usage(void)
{
	(void)fprintf(stderr, "usage: simp [-i] [-e string | -p string | file]");
}

int
main(int argc, char *argv[])
{
	enum { MODE_INTERACTIVE, MODE_STRING, MODE_PRINT, MODE_SCRIPT } mode;
	FILE *fp;
	Simp ctx, env, iport, oport, eport, port;
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

	/* first, create context (holds symbol table and garbage context) */
	ctx = simp_contextnew();
	if (simp_isexception(ctx))
		errx(EXIT_FAILURE, "could not create context");

	/* then, create standard input/output/error ports */
	iport = simp_openstream(ctx, "<stdin>", stdin, "r");
	if (simp_isexception(iport))
		errx(EXIT_FAILURE, "could not open input port");
	oport = simp_openstream(ctx, "<stdout>", stdout, "w");
	if (simp_isexception(oport))
		errx(EXIT_FAILURE, "could not open output port");
	eport = simp_openstream(ctx, "<stderr>", stderr, "w");
	if (simp_isexception(eport))
		errx(EXIT_FAILURE, "could not open error port");

	/* finally, create environment (holds variable bindings) */
	env = simp_environmentnew(ctx);
	if (simp_isexception(env))
		errx(EXIT_FAILURE, "could not create environment");

	switch (mode) {
	case MODE_STRING:
	case MODE_PRINT:
		port = simp_openstring(ctx, (unsigned char *)expr, strlen(expr), "r");
		if (simp_isexception(port))
			goto error;
		retval = simp_repl(
			ctx, env, port,
			iport, oport, eport,
			mode == MODE_PRINT ? SIMP_ECHO : 0
		);
		if (retval == EXIT_SUCCESS && iflag)
			goto interactive;
		break;
	case MODE_SCRIPT:
		if (argv[0][0] == '-' && argv[0][1] == '\0') {
			fp = stdin;
			port = iport;
			iflag = 0;
		} else if ((fp = fopen(argv[0], "r")) != NULL) {
			port = simp_openstream(ctx, argv[0], fp, "r");
		} else {
			goto error;
		}
		if (simp_isexception(port))
			goto error;
		retval = simp_repl(ctx, env, port, iport, oport, eport, 0);
		if (fp != stdin)
			(void)fclose(fp);
		if (retval == EXIT_SUCCESS && iflag)
			goto interactive;
		break;
	case MODE_INTERACTIVE:
interactive:
		retval = simp_repl(
			ctx, env, iport,
			iport, oport, eport,
			SIMP_INTERACTIVE
		);
		break;
	}
error:
	simp_gcfree(ctx);
	return retval;
}
