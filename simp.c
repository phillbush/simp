#include "simp.h"

int
main(void)
{
	Simp ctx;

	ctx = simp_contextnew();
	simp_repl(ctx);
}
