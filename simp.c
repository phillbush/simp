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
	if (simp_isexception(ctx))
		errx(EXIT_FAILURE, "could not create context");
	iport = simp_contextiport(ctx);
	switch (mode) {
	case MODE_STRING:
		port = simp_openstring(ctx, (unsigned char *)expr, strlen(expr), "r");
		if (simp_isexception(port))
			goto error;
		simp_repl(ctx, port, 0);
		if (iflag)
			simp_repl(ctx, iport, SIMP_INTERACTIVE);
		break;
	case MODE_PRINT:
		port = simp_openstring(ctx, (unsigned char *)expr, strlen(expr), "r");
		if (simp_isexception(port))
			goto error;
		simp_repl(ctx, port, SIMP_ECHO);
		if (iflag)
			simp_repl(ctx, iport, SIMP_INTERACTIVE);
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
		simp_repl(ctx, port, 0);
		if (fp != stdin)
			(void)fclose(fp);
		if (iflag)
			simp_repl(ctx, iport, SIMP_INTERACTIVE);
		break;
	case MODE_INTERACTIVE:
		simp_repl(ctx, iport, SIMP_INTERACTIVE);
		break;
	}
	retval = EXIT_SUCCESS;
error:
	simp_gcfree(ctx);
	return retval;
}
