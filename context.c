#include <stdlib.h>
#include <string.h>

#include "common.h"

struct Context {
	Atom    iport, oport, eport;
	Atom    symtab;
};

#define SYMTAB_SIZE     389
#define SYMTAB_MULT     37

Context *
simp_contextnew(void)
{
	Context *ctx;

	if ((ctx = malloc(sizeof(*ctx))) == NULL)
		return NULL;
	*ctx = (Context){
		.iport = simp_openiport(NULL),
		.oport = simp_openoport(NULL),
		.eport = simp_openeport(NULL),
		.symtab = simp_makevector(NULL, SYMTAB_SIZE, simp_nil()),
	};
	return ctx;
}

Atom
simp_contextintern(Context *ctx, Byte *src, Size size)
{
	Atom list, prev, pair, sym;
	Size i, bucket, len;
	Byte *dst;

	bucket = 0;
	for (i = 0; i < size; i++) {
		bucket *= SYMTAB_MULT;
		bucket += src[i];
	}
	bucket %= SYMTAB_SIZE;
	list = simp_getvectormemb(ctx, ctx->symtab, bucket);
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
		simp_setvector(ctx, ctx->symtab, bucket, pair);
	else
		simp_setcdr(ctx, prev, pair);
	return sym;
}

Atom
simp_contextiport(Context *ctx)
{
	return ctx->iport;
}

Atom
simp_contextoport(Context *ctx)
{
	return ctx->oport;
}

Atom
simp_contexteport(Context *ctx)
{
	return ctx->eport;
}
