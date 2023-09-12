#include <stdlib.h>
#include <string.h>

#include "simp.h"

#define SYMTAB_SIZE     389
#define SYMTAB_MULT     37

enum {
	/*
	 * An environment is a pair of a pointer to its parent and a
	 * pointer to the environment's frame
	 */
	ENVIRONMENT_PARENT,
	ENVIRONMENT_FRAME,
	ENVIRONMENT_SIZE,
};

enum {
	CLOSURE_ENVIRONMENT,
	CLOSURE_PARAMETERS,
	CLOSURE_VARARGS,
	CLOSURE_EXPRESSIONS,
	CLOSURE_SIZE
};

struct Source {
	const char             *filename;
	SimpSiz                 lineno;
	SimpSiz                 column;
};

Simp *
simp_getvector(Simp obj)
{
	if (simp_isnil(obj))
		return NULL;
	return (Simp *)simp_getheapdata(obj.u.heap) + obj.start;
}

static Simp *
simp_getclosure(Simp obj)
{
	return simp_getvector(obj);
}

enum Type
simp_gettype(Simp obj)
{
	return obj.type;
}

void
simp_setenvframe(Simp env, Simp frame)
{
	simp_setvector(env, ENVIRONMENT_FRAME, frame);
}

bool
simp_contextnew(Simp *ctx)
{
	return simp_makevector(simp_nil(), ctx, SYMTAB_SIZE);
}

void
simp_cpystring(Simp dst, Simp src)
{
	(void)memcpy(
		simp_getstring(dst),
		simp_getstring(src),
		simp_getsize(src) * sizeof(unsigned char)
	);
}

void
simp_cpyvector(Simp dst, Simp src)
{
	(void)memcpy(
		simp_getvector(dst),
		simp_getvector(src),
		simp_getsize(src) * sizeof(Simp)
	);
}

Simp
simp_empty(void)
{
	return (Simp){
		.type = TYPE_STRING,
		.u.heap = NULL,
		.size = 0,
		.start = 0,
	};
}

Simp
simp_eof(void)
{
	return (Simp){ .type = TYPE_EOF };
}

Simp
simp_false(void)
{
	return (Simp){ .type = TYPE_FALSE };
}

Builtin *
simp_getbuiltin(Simp obj)
{
	return obj.u.builtin;
}

unsigned char
simp_getbyte(Simp obj)
{
	return obj.u.byte;
}

Simp
simp_getclosureenv(Simp obj)
{
	return simp_getclosure(obj)[CLOSURE_ENVIRONMENT];
}

Simp
simp_getclosureparam(Simp obj)
{
	return simp_getclosure(obj)[CLOSURE_PARAMETERS];
}

Simp
simp_getclosurevarargs(Simp obj)
{
	return simp_getclosure(obj)[CLOSURE_VARARGS];
}

Simp
simp_getclosurebody(Simp obj)
{
	return simp_getclosure(obj)[CLOSURE_EXPRESSIONS];
}

static Simp *
simp_getenvironment(Simp obj)
{
	return simp_getvector(obj);
}

Simp
simp_getenvframe(Simp obj)
{
	return simp_getenvironment(obj)[ENVIRONMENT_FRAME];
}

Simp
simp_getenvparent(Simp obj)
{
	return simp_getenvironment(obj)[ENVIRONMENT_PARENT];
}

SimpInt
simp_getsignum(Simp obj)
{
	return obj.u.num;
}

Port *
simp_getport(Simp obj)
{
	return (Port *)simp_getheapdata(obj.u.heap);
}

double
simp_getreal(Simp obj)
{
	return obj.u.real;
}

static SimpSiz
simp_getstart(Simp obj)
{
	return obj.start;
}

SimpSiz
simp_getsize(Simp obj)
{
	return obj.size;
}

unsigned char *
simp_getstring(Simp obj)
{
	if (simp_isempty(obj))
		return NULL;
	return (unsigned char *)simp_getheapdata(obj.u.heap) + obj.start;
}

unsigned char *
simp_getsymbol(Simp obj)
{
	return (unsigned char *)simp_getheapdata(obj.u.heap) + obj.start;
}

Simp
simp_getvectormemb(Simp obj, SimpSiz pos)
{
	return simp_getvector(obj)[pos];
}

unsigned char
simp_getstringmemb(Simp obj, SimpSiz pos)
{
	return simp_getstring(obj)[pos];
}

bool
simp_isbool(Simp obj)
{
	enum Type t = simp_gettype(obj);

	return t == TYPE_TRUE || t == TYPE_FALSE;
}

bool
simp_isbuiltin(Simp obj)
{
	return simp_gettype(obj) == TYPE_BUILTIN;
}

bool
simp_isclosure(Simp obj)
{
	return simp_gettype(obj) == TYPE_CLOSURE;
}

bool
simp_isbyte(Simp obj)
{
	return simp_gettype(obj) == TYPE_BYTE;
}

bool
simp_isempty(Simp obj)
{
	return simp_isstring(obj) && obj.u.heap == NULL;
}

bool
simp_isenvironment(Simp obj)
{
	return simp_gettype(obj) == TYPE_ENVIRONMENT;
}

bool
simp_iseof(Simp obj)
{
	return simp_gettype(obj) == TYPE_EOF;
}

bool
simp_isfalse(Simp obj)
{
	return simp_gettype(obj) == TYPE_FALSE;
}

bool
simp_isnil(Simp obj)
{
	if (!simp_isvector(obj))
		return false;
	if (simp_getgcmemory(obj) == NULL)
		return true;
	if (simp_getheapdata(obj.u.heap) == NULL)
		return true;
	return false;
}

bool
simp_isnulenv(Simp obj)
{
	return simp_isenvironment(obj) && obj.u.heap == NULL;
}

bool
simp_issignum(Simp obj)
{
	return simp_gettype(obj) == TYPE_SIGNUM;
}

bool
simp_isnum(Simp obj)
{
	return simp_issignum(obj) || simp_isreal(obj);
}

bool
simp_isport(Simp obj)
{
	return simp_gettype(obj) == TYPE_PORT;
}

bool
simp_isprocedure(Simp obj)
{
	return simp_isbuiltin(obj) || simp_isclosure(obj);
}

bool
simp_isreal(Simp obj)
{
	return simp_gettype(obj) == TYPE_REAL;
}

bool
simp_issame(Simp a, Simp b)
{
	enum Type typea = simp_gettype(a);
	enum Type typeb = simp_gettype(b);

	if (typea != typeb)
		return false;
	switch (typea) {
	case TYPE_CLOSURE:
	case TYPE_VECTOR:
	case TYPE_ENVIRONMENT:
		return simp_getvector(a) == simp_getvector(b) &&
			simp_getstart(a) == simp_getstart(b) &&
			simp_getsize(a) == simp_getsize(b);
	case TYPE_SIGNUM:
		return simp_getsignum(a) == simp_getsignum(b);
	case TYPE_REAL:
		return simp_getreal(a) == simp_getreal(b);
	case TYPE_PORT:
		return simp_getport(a) == simp_getport(b);
	case TYPE_BYTE:
		return simp_getbyte(a) == simp_getbyte(b);
	case TYPE_SYMBOL:
		return simp_getsymbol(a) == simp_getsymbol(b);
	case TYPE_STRING:
		return simp_getstring(a) == simp_getstring(b) &&
			simp_getstart(a) == simp_getstart(b) &&
			simp_getsize(a) == simp_getsize(b);
	case TYPE_BUILTIN:
		return simp_getbuiltin(a) == simp_getbuiltin(b);
	case TYPE_EOF:
	case TYPE_TRUE:
	case TYPE_VOID:
	case TYPE_FALSE:
		return true;
	}
	return false;
}

bool
simp_isstring(Simp obj)
{
	return simp_gettype(obj) == TYPE_STRING;
}

bool
simp_issymbol(Simp obj)
{
	return simp_gettype(obj) == TYPE_SYMBOL;
}

bool
simp_istrue(Simp obj)
{
	return !simp_isfalse(obj);
}

bool
simp_isvector(Simp obj)
{
	return simp_gettype(obj) == TYPE_VECTOR;
}

bool
simp_isvoid(Simp obj)
{
	return simp_gettype(obj) == TYPE_VOID;
}

void
simp_setstring(Simp obj, SimpSiz pos, unsigned char u)
{
	simp_getstring(obj)[pos] = u;
}

void
simp_setvector(Simp obj, SimpSiz pos, Simp val)
{
	simp_getvector(obj)[pos] = val;
}

Simp
simp_slicevector(Simp obj, SimpSiz from, SimpSiz size)
{
	obj.start += from;
	obj.size = size;
	return obj;
}

Simp
simp_slicestring(Simp obj, SimpSiz from, SimpSiz size)
{
	obj.start += from;
	obj.size = size;
	return obj;
}

Simp
simp_true(void)
{
	return (Simp){ .type = TYPE_TRUE };
}

bool
simp_makebuiltin(Simp ctx, Simp *ret, Builtin *builtin)
{
	(void)ctx;
	*ret = (Simp){
		.type = TYPE_BUILTIN,
		.u.builtin = builtin,
		.source = NULL,
	};
	return true;
}

bool
simp_makebyte(Simp ctx, Simp *ret, unsigned char byte)
{
	(void)ctx;
	*ret = (Simp){
		.type = TYPE_BYTE,
		.u.byte = byte,
		.source = NULL,
	};
	return true;
}

bool
simp_makeenvironment(Simp ctx, Simp *env, Simp parent)
{
	(void)ctx;
	if (!simp_makevector(ctx, env, ENVIRONMENT_SIZE))
		return false;
	simp_setvector(*env, ENVIRONMENT_PARENT, parent);
	simp_setvector(*env, ENVIRONMENT_FRAME, simp_nil());
	env->type = TYPE_ENVIRONMENT;
	return true;
}

bool
simp_makesignum(Simp ctx, Simp *ret, SimpInt n)
{
	(void)ctx;
	*ret = (Simp){
		.type = TYPE_SIGNUM,
		.u.num = n,
		.source = NULL,
	};
	return true;
}

bool
simp_makeclosure(Simp ctx, Simp *lambda, Simp src, Simp env, Simp params, Simp varargs, Simp body)
{
	const char *filename = NULL;
	SimpSiz lineno = 0;
	SimpSiz column = 0;

	if (!simp_makevector(ctx, lambda, CLOSURE_SIZE))
		return false;
	simp_setvector(*lambda, CLOSURE_ENVIRONMENT, env);
	simp_setvector(*lambda, CLOSURE_PARAMETERS, params);
	simp_setvector(*lambda, CLOSURE_VARARGS, varargs);
	simp_setvector(*lambda, CLOSURE_EXPRESSIONS, body);
	lambda->type = TYPE_CLOSURE;
	if (simp_getsource(src, &filename, &lineno, &column))
		return simp_setsource(ctx, lambda, filename, lineno, column);
	return true;
}

bool
simp_makeport(Simp ctx, Simp *ret, Heap *p)
{
	(void)ctx;
	*ret = (Simp){
		.type = TYPE_PORT,
		.size = 1,
		.start = 0,
		.source = NULL,
		.u.heap = p,
	};
	return true;
}

bool
simp_makereal(Simp ctx, Simp *ret, double x)
{
	(void)ctx;
	*ret = (Simp){
		.type = TYPE_REAL,
		.u.real = x,
		.source = NULL,
	};
	return true;
}

bool
simp_makestring(Simp ctx, Simp *ret, const unsigned char *src, SimpSiz size)
{
	Heap *heap;
	unsigned char *dst = NULL;

	*ret = simp_empty();
	if (size == 0)
		return true;
	heap = simp_gcnewobj(simp_getgcmemory(ctx), size, 0);
	if (heap == NULL)
		return false;
	dst = (unsigned char *)simp_getheapdata(heap);
	if (src != NULL)
		memcpy(dst, src, size);
	*ret = (Simp){
		.type = TYPE_STRING,
		.size = size,
		.start = 0,
		.u.heap = heap,
		.source = NULL,
	};
	return true;
}

bool
simp_makesymbol(Simp ctx, Simp *sym, const unsigned char *src, SimpSiz size)
{
	Simp list, prev, pair;
	SimpSiz i, bucket, len;
	unsigned char *dst;

	bucket = 0;
	for (i = 0; i < size; i++) {
		bucket *= SYMTAB_MULT;
		bucket += src[i];
	}
	bucket %= SYMTAB_SIZE;
	list = simp_getvectormemb(ctx, bucket);
	prev = simp_nil();
	for (pair = list; !simp_isnil(pair); pair = simp_getvectormemb(pair, 1)) {
		*sym = simp_getvectormemb(pair, 0);
		dst = simp_getstring(*sym);
		len = simp_getsize(*sym);
		if (len == size && memcmp(src, dst, size) == 0)
			return true;
		prev = pair;
	}
	if (!simp_makestring(ctx, sym, src, size))
		return false;
	sym->type = TYPE_SYMBOL;
	if (!simp_makevector(ctx, &pair, 2))
		return false;
	simp_setvector(pair, 0, *sym);
	if (simp_isnil(prev))
		simp_setvector(ctx, bucket, pair);
	else
		simp_setvector(prev, 1, pair);
	return true;
}

bool
simp_makevector(Simp ctx, Simp *ret, SimpSiz size)
{
	SimpSiz i;
	Simp *data;
	Heap *heap;

	*ret = simp_nil();
	if (size == 0)
		return true;
	heap = simp_gcnewobj(
		simp_getgcmemory(ctx),
		size * sizeof(Simp),
		size
	);
	if (heap == NULL)
		return false;
	data = simp_getheapdata(heap);
	for (i = 0; i < size; i++)
		data[i] = simp_nil();
	*ret = (Simp){
		.type = TYPE_VECTOR,
		.u.heap = heap,
		.size = size,
		.start = 0,
		.source = NULL,
	};
	return true;
}

Simp
simp_nil(void)
{
	return (Simp){
		.type = TYPE_VECTOR,
		.size = 0,
		.start = 0,
		.u.heap = NULL,
	};
}

Simp
simp_nulenv(void)
{
	return (Simp){
		.type = TYPE_ENVIRONMENT,
		.size = 0,
		.start = 0,
		.u.heap = NULL,
	};
}

Simp
simp_void(void)
{
	return (Simp){ .type = TYPE_VOID };
}

Heap *
simp_getgcmemory(Simp obj)
{
	return obj.u.heap;
}

bool
simp_setsource(Simp ctx, Simp *obj, const char *filename, SimpSiz lineno, SimpSiz column)
{
	struct Source *src;

	obj->source = simp_gcnewobj(simp_getgcmemory(ctx), sizeof(*src), 0);
	if (obj->source == NULL)
		return false;
	src = simp_getheapdata(obj->source);
	src->filename = filename;
	src->lineno = lineno;
	src->column = column;
	return true;
}

bool
simp_getsource(Simp obj, const char **filename, SimpSiz *lineno, SimpSiz *column)
{
	struct Source *src;

	if (obj.source == NULL)
		return false;
	if ((src = simp_getheapdata(obj.source)) == NULL)
		return false;
	if (filename != NULL)
		*filename = src->filename;
	if (lineno != NULL)
		*lineno = src->lineno;
	if (column != NULL)
		*column = src->column;
	return true;
}

Heap *
simp_getsourcep(Simp obj)
{
	return(obj.source);
}
