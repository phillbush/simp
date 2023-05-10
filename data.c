#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simp.h"

static unsigned char *errortab[NEXCEPTIONS] = {
#define X(n, s) [n] = (unsigned char *)s,
	EXCEPTIONS
#undef  X
};

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

Simp
simp_car(Simp ctx, Simp obj)
{
	if (!simp_ispair(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getvector(ctx, obj)[0];
}

Simp
simp_cdr(Simp ctx, Simp obj)
{
	if (!simp_ispair(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getvector(ctx, obj)[1];
}

Simp
simp_cons(Simp ctx, Simp a, Simp b)
{
	Simp pair;

	pair = simp_makevector(ctx, 2, simp_nil());
	if (simp_isexception(ctx, pair))
		return pair;
	simp_setcar(ctx, pair, a);
	simp_setcdr(ctx, pair, b);
	return pair;
}

Simp
simp_empty(void)
{
	return (Simp){
		.type = TYPE_STRING,
		.size = 0,
		.u.vector = NULL,
	};
}

unsigned char
simp_getbyte(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.byte;
}

unsigned char *
simp_getexception(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.string;
}

SSimp
simp_getnum(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.num;
}

void *
simp_getport(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.port;
}

double
simp_getreal(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.real;
}

SSimp
simp_getsize(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.size;
}

unsigned char *
simp_getstring(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.string;
}

unsigned char *
simp_getsymbol(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.string;
}

unsigned char
simp_getstringmemb(Simp ctx, Simp obj, SSimp pos)
{
	return simp_getstring(ctx, obj)[pos];
}

Simp *
simp_getvector(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.vector;
}

Simp
simp_getvectormemb(Simp ctx, Simp obj, SSimp pos)
{
	return simp_getvector(ctx, obj)[pos];
}

unsigned char *
simp_errorstr(int exception)
{
	if (exception < 0 || exception >= NEXCEPTIONS)
		return (unsigned char *)"unknown error";
	return errortab[exception];
}

int
simp_isbyte(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type == TYPE_BYTE;
}

int
simp_isexception(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type == TYPE_EXCEPTION;
}

int
simp_isnum(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type == TYPE_SIGNUM;
}

int
simp_isnil(Simp ctx, Simp obj)
{
	return simp_isvector(ctx, obj) && obj.size == 0;
}

int
simp_isnul(Simp ctx, Simp obj)
{
	return simp_isstring(ctx, obj) && obj.size == 0;
}

int
simp_ispair(Simp ctx, Simp obj)
{
	return simp_isvector(ctx, obj) && simp_getsize(ctx, obj) == 2;
}

int
simp_isport(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type == TYPE_PORT;
}

int
simp_isreal(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type == TYPE_REAL;
}

int
simp_isstring(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type == TYPE_STRING;
}

int
simp_issymbol(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type == TYPE_SYMBOL;
}

int
simp_isvector(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type == TYPE_VECTOR;
}

void
simp_setcar(Simp ctx, Simp obj, Simp val)
{
	simp_setvector(ctx, obj, 0, val);
}

void
simp_setcdr(Simp ctx, Simp obj, Simp val)
{
	simp_setvector(ctx, obj, 1, val);
}

void
simp_setstring(Simp ctx, Simp obj, SSimp pos, unsigned char val)
{
	(void)ctx;
	obj.u.string[pos] = val;
}

void
simp_setvector(Simp ctx, Simp obj, SSimp pos, Simp val)
{
	(void)ctx;
	obj.u.vector[pos] = val;
}

Simp
simp_makebyte(Simp ctx, unsigned char byte)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_BYTE,
		.size = 0,
		.u.byte = byte,
	};
}

Simp
simp_makeexception(Simp ctx, int n)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_EXCEPTION,
		.size = 0,
		.u.string = simp_errorstr(n),
	};
}

Simp
simp_makenum(Simp ctx, SSimp n)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_SIGNUM,
		.size = 0,
		.u.num = n,
	};
}

Simp
simp_makeport(Simp ctx, void *p)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_PORT,
		.size = 0,
		.u.port = (void *)p,
	};
}

Simp
simp_makereal(Simp ctx, double x)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_REAL,
		.size = 0,
		.u.real = x,
	};
}

Simp
simp_makestring(Simp ctx, unsigned char *src, SSimp size)
{
	unsigned char *dst = NULL;

	if (size > 0)
		if ((dst = calloc(size, 1)) == NULL)
			return simp_makeexception(ctx, ERROR_MEMORY);
	if (size > 0 && src != NULL)
		memcpy(dst, src, size);
	return (Simp){
		.type = TYPE_STRING,
		.size = size,
		.u.string = dst,
	};
}

Simp
simp_makesymbol(Simp ctx, unsigned char *src, SSimp size)
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
	if (simp_isexception(ctx, sym))
		return sym;
	sym.type = TYPE_SYMBOL;
	pair = simp_cons(ctx, sym, simp_nil());
	if (simp_isexception(ctx, pair))
		return pair;
	if (simp_isnil(ctx, prev))
		simp_setvector(ctx, symtab, bucket, pair);
	else
		simp_setcdr(ctx, prev, pair);
	return sym;
}

Simp
simp_makevector(Simp ctx, SSimp size, Simp fill)
{
	Simp *dst = NULL;
	SSimp i;

	if (size > 0)
		if ((dst = calloc(size, sizeof(*dst))) == NULL)
			return simp_makeexception(ctx, ERROR_MEMORY);
	for (i = 0; i < size; i++)
		dst[i] = fill;
	return (Simp){
		.type = TYPE_VECTOR,
		.size = size,
		.u.vector = dst,
	};
}

Simp
simp_nil(void)
{
	return (Simp){
		.type = TYPE_VECTOR,
		.size = 0,
		.u.vector = NULL,
	};
}
