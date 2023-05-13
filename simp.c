#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "simp.h"

static void
usage(void)
{
	(void)fprintf(stderr, "usage: simp [-l] [-e string | file]");
}

static int
rel(Simp ctx, Simp iport)
{
	Simp eport, obj, env;

	env = simp_contextenvironment(ctx);
	eport = simp_contexteport(ctx);
	for (;;) {
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

static void
repl(Simp ctx)
{
	Simp iport, oport, eport, obj, env;

	env = simp_contextenvironment(ctx);
	iport = simp_contextiport(ctx);
	oport = simp_contextoport(ctx);
	eport = simp_contexteport(ctx);
	for (;;) {
		if (simp_porterr(ctx, iport))
			break;
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
		printf("\n");
	}
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	Simp ctx, port;
	int ch, retval = EXIT_SUCCESS;
	int lflag = 0;
	char *expr = NULL;

	while ((ch = getopt(argc, argv, "e:l")) != -1) switch (ch) {
	case 'e':
		expr = optarg;
		break;
	case 'l':
		lflag = 1;
		break;
	default:
		usage();
	}
	argc -= optind;
	argv += optind;
	ctx = simp_contextnew();
	if (simp_isexception(simp_nil(), ctx))
		errx(EXIT_FAILURE, "could not create context");
	if (expr != NULL) {
		port = simp_openstring(ctx, (unsigned char *)expr, strlen(expr), "r");
		if (simp_isexception(ctx, port))
			goto error;
		retval = rel(ctx, port);
		if (lflag) {
			repl(ctx);
		}
	} else if (argc > 0) {
		if ((fp = fopen(argv[0], "r")) == NULL)
			goto error;
		port = simp_openstream(ctx, fp, "r");
		if (simp_isexception(ctx, port))
			goto error;
		retval = rel(ctx, port);
		(void)fclose(fp);
		if (lflag) {
			repl(ctx);
		}
	} else {
		repl(ctx);
	}
	return retval;
error:
	return EXIT_FAILURE;
}
