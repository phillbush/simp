#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simp.h"

enum {
	CONTEXT_IPORT,
	CONTEXT_OPORT,
	CONTEXT_EPORT,
	CONTEXT_SYMTAB,
	NCONTEXTS
};

#define SYMTAB_SIZE     389
#define SYMTAB_MULT     37

Simp
simp_contextnew(void)
{
	Simp ctx;
	Simp membs[NCONTEXTS];
	SSimp i;

	ctx = simp_makevector(simp_nil(), NCONTEXTS, simp_nil());
	membs[CONTEXT_IPORT] = simp_openstream(simp_nil(), stdin, "r");
	membs[CONTEXT_OPORT] = simp_openstream(simp_nil(), stdout, "w");
	membs[CONTEXT_EPORT] = simp_openstream(simp_nil(), stderr, "w");
	membs[CONTEXT_SYMTAB] = simp_makevector(simp_nil(), SYMTAB_SIZE, simp_nil());
	for (i = 0; i < NCONTEXTS; i++)
		simp_setvector(simp_nil(), ctx, i, membs[i]);
	return ctx;
}

Simp
simp_contextintern(Simp ctx, unsigned char *src, SSimp size)
{
	Simp list, prev, pair, symtab, sym;
	SSimp i, bucket, len;
	unsigned char *dst;

	symtab = simp_getvectormemb(ctx, ctx, CONTEXT_SYMTAB);
	bucket = 0;
	for (i = 0; i < size; i++) {
		bucket *= SYMTAB_MULT;
		bucket += src[i];
	}
	bucket %= SYMTAB_SIZE;
	list = simp_getvectormemb(ctx, symtab, bucket);
	prev = simp_nil();
	for (pair = list; !simp_isnil(ctx, pair); pair = simp_cdr(ctx, pair)) {
		sym = simp_car(ctx, pair);
		dst = simp_getstring(ctx, sym);
		len = simp_getsize(ctx, sym);
		if (len == size && memcmp(src, dst, size) == 0)
			return sym;
		prev = pair;
	}
	sym = simp_makestring(ctx, src, size);
	pair = simp_cons(ctx, sym, simp_nil());
	if (simp_isnil(ctx, prev))
		simp_setvector(ctx, symtab, bucket, pair);
	else
		simp_setcdr(ctx, prev, pair);
	return sym;
}

Simp
simp_contextiport(Simp ctx)
{
	return simp_getvectormemb(ctx, ctx, CONTEXT_IPORT);
}

Simp
simp_contextoport(Simp ctx)
{
	return simp_getvectormemb(ctx, ctx, CONTEXT_OPORT);
}

Simp
simp_contexteport(Simp ctx)
{
	return simp_getvectormemb(ctx, ctx, CONTEXT_EPORT);
}
