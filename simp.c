#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "simp.h"

static void
repl(Simp ctx)
{
	Simp iport, oport, eport, obj, env;

	env = simp_contextenvironment(ctx);
	iport = simp_contextiport(ctx);
	oport = simp_contextoport(ctx);
	eport = simp_contexteport(ctx);
	for (;;) {
		obj = simp_read(ctx, iport);
		if (simp_porteof(ctx, iport))
			break;
		if (simp_porterr(ctx, iport))
			break;
		if (simp_isexception(ctx, obj)) {
			simp_write(ctx, eport, obj);
			goto newline;
		}
		obj = simp_eval(ctx, env, obj);
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
main(void)
{
	Simp ctx;

	ctx = simp_contextnew();
	if (simp_isexception(simp_nil(), ctx))
		errx(EXIT_FAILURE, "could not create context");
	repl(ctx);
	return EXIT_SUCCESS;
}
