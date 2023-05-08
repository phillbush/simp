#include <stdlib.h>
#include <string.h>

#include "common.h"

Atom
simp_car(Context *ctx, Atom obj)
{
	return simp_getvector(ctx, obj)[0];
}

Atom
simp_cdr(Context *ctx, Atom obj)
{
	return simp_getvector(ctx, obj)[1];
}

Atom
simp_cons(Context *ctx, Atom a, Atom b)
{
	Atom pair;

	pair = simp_makevector(ctx, 2, simp_nil());
	simp_setcar(ctx, pair, a);
	simp_setcdr(ctx, pair, b);
	return pair;
}

Atom
simp_empty(void)
{
	return (Atom){
		.type = TYPE_STRING,
		.size = 0,
		.u.vector = NULL,
	};
}

Byte
simp_getbyte(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.u.byte;
}

Signum
simp_getnum(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.u.num;
}

Port *
simp_getport(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.u.port;
}

Real
simp_getreal(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.u.real;
}

Size
simp_getsize(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.size;
}

Byte *
simp_getstring(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.u.string;
}

Byte
simp_getstringmemb(Context *ctx, Atom obj, Size pos)
{
	return simp_getstring(ctx, obj)[pos];
}

Atom *
simp_getvector(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.u.vector;
}

Atom
simp_getvectormemb(Context *ctx, Atom obj, Size pos)
{
	return simp_getvector(ctx, obj)[pos];
}

Bool
simp_isbyte(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.type == TYPE_BYTE;
}

Bool
simp_isnum(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.type == TYPE_SIGNUM;
}

Bool
simp_isnil(Context *ctx, Atom obj)
{
	return simp_isvector(ctx, obj) && obj.size == 0;
}

Bool
simp_isnul(Context *ctx, Atom obj)
{
	return simp_isstring(ctx, obj) && obj.size == 0;
}

Bool
simp_isport(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.type == TYPE_PORT;
}

Bool
simp_isreal(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.type == TYPE_REAL;
}

Bool
simp_isstring(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.type == TYPE_STRING;
}

Bool
simp_issymbol(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.type == TYPE_SYMBOL;
}

Bool
simp_isvector(Context *ctx, Atom obj)
{
	(void)ctx;
	return obj.type == TYPE_VECTOR;
}

void
simp_setcar(Context *ctx, Atom obj, Atom val)
{
	simp_setvector(ctx, obj, 0, val);
}

void
simp_setcdr(Context *ctx, Atom obj, Atom val)
{
	simp_setvector(ctx, obj, 1, val);
}

void
simp_setstring(Context *ctx, Atom obj, Size pos, Byte val)
{
	(void)ctx;
	obj.u.string[pos] = val;
}

void
simp_setvector(Context *ctx, Atom obj, Size pos, Atom val)
{
	(void)ctx;
	obj.u.vector[pos] = val;
}

Atom
simp_makebyte(Context *ctx, Byte byte)
{
	(void)ctx;
	return (Atom){
		.type = TYPE_BYTE,
		.size = 0,
		.u.byte = byte,
	};
}

Atom
simp_makenum(Context *ctx, Signum n)
{
	(void)ctx;
	return (Atom){
		.type = TYPE_SIGNUM,
		.size = 0,
		.u.num = n,
	};
}

Atom
simp_makeport(Context *ctx, Port *p)
{
	(void)ctx;
	return (Atom){
		.type = TYPE_PORT,
		.size = 0,
		.u.port = (Port *)p,
	};
}

Atom
simp_makereal(Context *ctx, Real x)
{
	(void)ctx;
	return (Atom){
		.type = TYPE_REAL,
		.size = 0,
		.u.real = x,
	};
}

Atom
simp_makestring(Context *ctx, Byte *src, Size size)
{
	Byte *dst;

	(void)ctx;
	dst = calloc(size, 1);
	// TODO: check error
	if (src != NULL)
		memcpy(dst, src, size);
	return (Atom){
		.type = TYPE_STRING,
		.size = size,
		.u.string = dst,
	};
}

Atom
simp_makesymbol(Context *ctx, Byte *src, Size size)
{
	Atom obj;

	obj = simp_contextintern(ctx, src, size);
	obj.type = TYPE_SYMBOL;
	return obj;
}

Atom
simp_makevector(Context *ctx, Size size, Atom fill)
{
	Atom *dst;
	Size i;

	(void)ctx;
	dst = calloc(size, sizeof(*dst));
	// TODO: check error
	for (i = 0; i < size; i++)
		dst[i] = fill;
	return (Atom){
		.type = TYPE_VECTOR,
		.size = size,
		.u.vector = dst,
	};
}
