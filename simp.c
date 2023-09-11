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
	if (!simp_contextnew(&ctx))
		errx(EXIT_FAILURE, "could not create context");

	/* then, create standard input/output/error ports */
	if (!simp_openstream(ctx, &iport, "<stdin>", stdin, "r"))
		errx(EXIT_FAILURE, "could not open input port");
	if (!simp_openstream(ctx, &oport, "<stdout>", stdout, "w"))
		errx(EXIT_FAILURE, "could not open output port");
	if (!simp_openstream(ctx, &eport, "<stderr>", stderr, "w"))
		errx(EXIT_FAILURE, "could not open error port");

	/* finally, create environment (holds variable bindings) */
	if (!simp_environmentnew(ctx, &env))
		errx(EXIT_FAILURE, "could not create environment");

	switch (mode) {
	case MODE_STRING:
	case MODE_PRINT:
		if (!simp_openstring(ctx, &port, (unsigned char *)expr, strlen(expr), "r"))
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
			if (!simp_openstream(ctx, &port, argv[0], fp, "r"))
				goto error;
		} else {
			goto error;
		}
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
