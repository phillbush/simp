#include <stdio.h>

#include "simp.h"

int
main(void)
{
	simpctx_t ctx;

	ctx = simp_init(stdin, stdout, stderr);
	simp_repl(ctx);
	simp_clean(ctx);
	return 0;
}
