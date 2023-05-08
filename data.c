#include <stdlib.h>
#include <string.h>

#include "simp.h"

Simp
simp_car(Simp ctx, Simp obj)
{
	return simp_getvector(ctx, obj)[0];
}

Simp
simp_cdr(Simp ctx, Simp obj)
{
	return simp_getvector(ctx, obj)[1];
}

Simp
simp_cons(Simp ctx, Simp a, Simp b)
{
	Simp pair;

	pair = simp_makevector(ctx, 2, simp_nil());
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

int
simp_isbyte(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type == TYPE_BYTE;
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
	unsigned char *dst;

	(void)ctx;
	dst = calloc(size, 1);
	// TODO: check error
	if (src != NULL)
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
	Simp obj;

	obj = simp_contextintern(ctx, src, size);
	obj.type = TYPE_SYMBOL;
	return obj;
}

Simp
simp_makevector(Simp ctx, SSimp size, Simp fill)
{
	Simp *dst;
	SSimp i;

	(void)ctx;
	dst = calloc(size, sizeof(*dst));
	// TODO: check error
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
