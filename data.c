#include <stdlib.h>
#include <string.h>

#include "simp.h"

#define SYMTAB_SIZE     389
#define SYMTAB_MULT     37

#define CONTEXTS                                                      \
	/* symbol table and environment should be initialized first */\
	X(CONTEXT_SYMTAB,       simp_inithashtab                     )\
	X(CONTEXT_ENVIRONMENT,  simp_initenviron                     )\
	/* then the rest                                            */\
	X(CONTEXT_PORTS,        simp_initports                       )\
	X(CONTEXT_FORMS,        simp_initforms                       )\

enum {
#define X(n, f) n,
	CONTEXTS
	NCONTEXTS
#undef  X
};

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
	/*
	 * An environment frame is a linked-list of triplets containing
	 * a symbol of the variable, its value, and a pointer to the
	 * next triplet.
	 */
	BINDING_VARIABLE,
	BINDING_VALUE,
	BINDING_NEXT,
	BINDING_SIZE,
};

enum {
	CLOSURE_ENVIRONMENT,
	CLOSURE_PARAMETERS,
	CLOSURE_VARARGS,
	CLOSURE_EXPRESSIONS,
	CLOSURE_SIZE
};

static unsigned char *errortab[NEXCEPTIONS] = {
#define X(n, s) [n] = (unsigned char *)s,
	EXCEPTIONS
#undef  X
};

static int
simp_isbinding(Simp obj)
{
	return simp_gettype(obj) == TYPE_BINDING;
}

static int
simp_isnulenv(Simp obj)
{
	return simp_isenvironment(obj) && obj.u.heap == NULL;
}

static int
simp_isnulbind(Simp obj)
{
	return simp_isbinding(obj) && obj.u.heap == NULL;
}

static Simp
simp_nulbind(void)
{
	return (Simp){
		.type = TYPE_BINDING,
		.u.heap = NULL,
	};
}

static Simp
simp_makebind(Simp ctx, Simp sym, Simp val, Simp frame)
{
	Simp bind;

	bind = simp_makevector(ctx, NULL, 0, 0, BINDING_SIZE);
	if (simp_isexception(bind))
		return bind;
	simp_setvector(bind, BINDING_VARIABLE, sym);
	simp_setvector(bind, BINDING_VALUE, val);
	simp_setvector(bind, BINDING_NEXT, frame);
	bind.type = TYPE_BINDING;
	return bind;
}

Simp *
simp_getvector(Simp obj)
{
	return (Simp *)simp_getheapdata(obj.u.heap) + obj.start;
}

static Simp *
simp_getclosure(Simp obj)
{
	return simp_getvector(obj);
}

static Simp *
simp_getenvironment(Simp obj)
{
	return simp_getvector(obj);
}

static Simp *
simp_getbind(Simp obj)
{
	return simp_getvector(obj);
}

static Simp
simp_getenvframe(Simp obj)
{
	return simp_getenvironment(obj)[ENVIRONMENT_FRAME];
}

static Simp
simp_getenvparent(Simp obj)
{
	return simp_getenvironment(obj)[ENVIRONMENT_PARENT];
}

static Simp
simp_getbindvariable(Simp obj)
{
	return simp_getbind(obj)[BINDING_VARIABLE];
}

static Simp
simp_getbindvalue(Simp obj)
{
	return simp_getbind(obj)[BINDING_VALUE];
}

static Simp
simp_getbindnext(Simp obj)
{
	return simp_getbind(obj)[BINDING_NEXT];
}

enum Type
simp_gettype(Simp obj)
{
	return obj.type;
}

static bool
isform(Simp ctx, Simp obj)
{
	Simp forms;
	SimpSiz i, nforms;

	forms = simp_contextforms(ctx);
	nforms = simp_getsize(forms);
	for (i = 0; i < nforms; i++)
		if (simp_issame(obj, simp_getvectormemb(forms, i)))
			return true;
	return false;
}

static bool
xenvset(Simp env, Simp var, Simp val)
{
	Simp bind, sym;

	for (bind = simp_getenvframe(env);
	     !simp_isnulbind(bind);
	     bind = simp_getbindnext(bind)) {
		sym = simp_getbindvariable(bind);
		if (simp_issame(var, sym)) {
			simp_setvector(bind, BINDING_VALUE, val);
			return true;
		}
	}
	return false;
}

Simp
simp_envget(Simp ctx, Simp env, Simp sym)
{
	Simp bind, var;

	if (isform(ctx, sym))
		return simp_exception(ERROR_VARFORM);
	for (; !simp_isnulenv(env); env = simp_getenvparent(env)) {
		for (bind = simp_getenvframe(env);
		     !simp_isnulbind(bind);
		     bind = simp_getbindnext(bind)) {
			var = simp_getbindvariable(bind);
			if (simp_issame(var, sym)) {
				return simp_getbindvalue(bind);
			}
		}
	}
	return simp_exception(ERROR_UNBOUND);
}

Simp
simp_envset(Simp ctx, Simp env, Simp var, Simp val)
{
	(void)ctx;
	if (isform(ctx, var))
		return simp_exception(ERROR_VARFORM);
	if (xenvset(env, var, val))
		return var;
	return simp_exception(ERROR_UNBOUND);
}

Simp
simp_envdef(Simp ctx, Simp env, Simp var, Simp val)
{
	Simp frame, bind;

	if (isform(ctx, var))
		return simp_exception(ERROR_VARFORM);
	if (xenvset(env, var, val))
		return var;
	frame = simp_getenvframe(env);
	bind = simp_makebind(ctx, var, val, frame);
	if (simp_isexception(bind))
		return bind;
	simp_setvector(env, ENVIRONMENT_FRAME, bind);
	return var;
}

static Simp
simp_initenviron(Simp ctx)
{
	return simp_makeenvironment(ctx, simp_nulenv());
}

static Simp
simp_inithashtab(Simp ctx)
{
	return simp_makevector(ctx, NULL, 0, 0, SYMTAB_SIZE);
}

Simp
simp_contextnew(void)
{
	Simp ctx, obj;
	Simp *membs;
	SimpSiz i;
	Simp (*init[NCONTEXTS])(Simp) = {
#define X(n, f) [n] = f,
	CONTEXTS
#undef  X
	};

	ctx = simp_makevector(simp_nil(), NULL, 0, 0, NCONTEXTS);
	membs = simp_getvector(ctx);
	for (i = 0; i < NCONTEXTS; i++) {
		obj = (*init[i])(ctx);
		if (simp_isexception(obj))
			return obj;
		membs[i] = obj;
	}
	obj = simp_initbuiltins(ctx);
	if (simp_isexception(obj))
		return obj;
	return ctx;
}

Simp
simp_contextsymtab(Simp ctx)
{
	return simp_getvectormemb(ctx, CONTEXT_SYMTAB);
}

Simp
simp_contextforms(Simp ctx)
{
	return simp_getvectormemb(ctx, CONTEXT_FORMS);
}

Simp
simp_contextenvironment(Simp ctx)
{
	return simp_getvectormemb(ctx, CONTEXT_ENVIRONMENT);
}

Simp
simp_contextports(Simp ctx)
{
	return simp_getvectormemb(ctx, CONTEXT_PORTS);
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

unsigned char *
simp_getexception(Simp obj)
{
	return obj.u.errmsg;
}

SimpInt
simp_getnum(Simp obj)
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

SimpSiz
simp_getsize(Simp obj)
{
	return obj.size;
}

unsigned char *
simp_getstring(Simp obj)
{
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

unsigned char *
simp_errorstr(int exception)
{
	if (exception < 0 || exception >= NEXCEPTIONS)
		return (unsigned char *)"unknown error";
	return errortab[exception];
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
simp_isexception(Simp obj)
{
	return simp_gettype(obj) == TYPE_EXCEPTION;
}

bool
simp_isfalse(Simp obj)
{
	return simp_gettype(obj) == TYPE_FALSE;
}

bool
simp_isnil(Simp obj)
{
	return simp_isvector(obj) && obj.u.heap == NULL;
}

bool
simp_isnum(Simp obj)
{
	return simp_gettype(obj) == TYPE_SIGNUM;
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
	case TYPE_BINDING:
	case TYPE_ENVIRONMENT:
		return simp_getvector(a) == simp_getvector(b);
	case TYPE_SIGNUM:
		return simp_getnum(a) == simp_getnum(b);
	case TYPE_REAL:
		return simp_getreal(a) == simp_getreal(b);
	case TYPE_PORT:
		return simp_getport(a) == simp_getport(b);
	case TYPE_BYTE:
		return simp_getbyte(a) == simp_getbyte(b);
	case TYPE_SYMBOL:
		return simp_getsymbol(a) == simp_getsymbol(b);
	case TYPE_STRING:
		return simp_getstring(a) == simp_getstring(b);
	case TYPE_EXCEPTION:
		return simp_getexception(a) == simp_getexception(b);
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
	obj.start = from;
	obj.size = size;
	return obj;
}

Simp
simp_slicestring(Simp obj, SimpSiz from, SimpSiz size)
{
	obj.start = from;
	obj.size = size;
	return obj;
}

Simp
simp_true(void)
{
	return (Simp){ .type = TYPE_TRUE };
}

Simp
simp_makebuiltin(Simp ctx, Builtin *builtin)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_BUILTIN,
		.u.builtin = builtin,
	};
}

Simp
simp_makebyte(Simp ctx, unsigned char byte)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_BYTE,
		.u.byte = byte,
	};
}

Simp
simp_makeenvironment(Simp ctx, Simp parent)
{
	Simp env;

	(void)ctx;
	env = simp_makevector(ctx, NULL, 0, 0, ENVIRONMENT_SIZE);
	if (simp_isexception(env))
		return env;
	simp_setvector(env, ENVIRONMENT_PARENT, parent);
	simp_setvector(env, ENVIRONMENT_FRAME, simp_nulbind());
	env.type = TYPE_ENVIRONMENT;
	return env;
}

Simp
simp_exception(int n)
{
	return (Simp){
		.type = TYPE_EXCEPTION,
		.u.errmsg = simp_errorstr(n),
	};
}

Simp
simp_makenum(Simp ctx, SimpInt n)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_SIGNUM,
		.u.num = n,
	};
}

Simp
simp_makeclosure(Simp ctx, Simp env, Simp params, Simp varargs, Simp body)
{
	Simp lambda;

	lambda = simp_makevector(ctx, NULL, 0, 0, CLOSURE_SIZE);
	if (simp_isexception(lambda))
		return lambda;
	simp_setvector(lambda, CLOSURE_ENVIRONMENT, env);
	simp_setvector(lambda, CLOSURE_PARAMETERS, params);
	simp_setvector(lambda, CLOSURE_VARARGS, varargs);
	simp_setvector(lambda, CLOSURE_EXPRESSIONS, body);
	lambda.type = TYPE_CLOSURE;
	return lambda;
}

Simp
simp_makeport(Simp ctx, Heap *p)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_PORT,
		.size = 1,
		.start = 0,
		.u.heap = p,
	};
}

Simp
simp_makereal(Simp ctx, double x)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_REAL,
		.u.real = x,
	};
}

Simp
simp_makestring(Simp ctx, const unsigned char *src, SimpSiz size)
{
	Heap *heap;
	unsigned char *dst = NULL;

	if (size == 0)
		return simp_empty();
	heap = simp_gcnewobj(ctx, size, 0, NULL, 0, 0);
	if (heap == NULL)
		return simp_exception(ERROR_MEMORY);
	dst = (unsigned char *)simp_getheapdata(heap);
	if (src != NULL)
		memcpy(dst, src, size);
	return (Simp){
		.type = TYPE_STRING,
		.size = size,
		.start = 0,
		.u.heap = heap,
	};
}

Simp
simp_makesymbol(Simp ctx, const unsigned char *src, SimpSiz size)
{
	Simp list, prev, pair, symtab, sym;
	SimpSiz i, bucket, len;
	unsigned char *dst;

	symtab = simp_getvectormemb(ctx, CONTEXT_SYMTAB);
	bucket = 0;
	for (i = 0; i < size; i++) {
		bucket *= SYMTAB_MULT;
		bucket += src[i];
	}
	bucket %= SYMTAB_SIZE;
	list = simp_getvectormemb(symtab, bucket);
	prev = simp_nil();
	for (pair = list; !simp_isnil(pair); pair = simp_getvectormemb(pair, 1)) {
		sym = simp_getvectormemb(pair, 0);
		dst = simp_getstring(sym);
		len = simp_getsize(sym);
		if (len == size && memcmp(src, dst, size) == 0)
			return sym;
		prev = pair;
	}
	sym = simp_makestring(ctx, src, size);
	if (simp_isexception(sym))
		return sym;
	sym.type = TYPE_SYMBOL;
	pair = simp_makevector(ctx, NULL, 0, 0, 2);
	if (simp_isexception(pair))
		return pair;
	simp_setvector(pair, 0, sym);
	if (simp_isnil(prev))
		simp_setvector(symtab, bucket, pair);
	else
		simp_setvector(prev, 1, pair);
	return sym;
}

Simp
simp_makevector(Simp ctx, const char *filename, SimpSiz lineno, SimpSiz column, SimpSiz size)
{
	SimpSiz i;
	Simp *data;
	Heap *heap;

	if (size == 0)
		return simp_nil();
	heap = simp_gcnewobj(ctx, size * sizeof(Simp), size, filename, lineno, column);
	if (heap == NULL)
		return simp_exception(ERROR_MEMORY);
	data = simp_getheapdata(heap);
	for (i = 0; i < size; i++)
		data[i] = simp_nil();
	return (Simp){
		.type = TYPE_VECTOR,
		.u.heap = heap,
		.size = size,
		.start = 0,
	};
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

Simp    simp_makesource(Simp ctx, const char *file, SimpSiz lineno, SimpSiz column);
const char *simp_sourcefilename(Simp obj);
SimpSiz     simp_sourcelineno(Simp obj);
SimpSiz     simp_sourcecolumn(Simp obj);
