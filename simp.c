#include <stdio.h>

#include "simp.h"

int
main(void)
{
	Simp ctx, iport, oport, obj;

	ctx = simp_contextnew();
	iport = simp_openstream(ctx, stdin, "r");
	oport = simp_contextoport(ctx);
	for (;;) {
		obj = simp_read(ctx, iport);
		if (simp_porteof(ctx, iport))
			break;
		if (simp_porterr(ctx, iport))
			break;
		simp_write(ctx, oport, obj);
		printf("\n");
	}
}
