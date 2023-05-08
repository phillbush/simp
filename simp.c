#include "common.h"

int
main(void)
{
	Context *ctx;

	ctx = simp_contextnew();
	simp_repl(ctx);
}
