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
	CLOSURE_EXPRESSIONS,
	CLOSURE_SIZE
};

static unsigned char *errortab[NEXCEPTIONS] = {
#define X(n, s) [n] = (unsigned char *)s,
	EXCEPTIONS
#undef  X
};

static int
simp_isbinding(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_BINDING;
}

static Simp
simp_nulbind(void)
{
	return (Simp){
		.type = TYPE_BINDING,
		.u.vector = NULL,
	};
}

static Simp
simp_nulenv(void)
{
	return (Simp){
		.type = TYPE_ENVIRONMENT,
		.u.vector = NULL,
	};
}

static Simp
simp_makebind(Simp ctx, Simp sym, Simp val, Simp frame)
{
	Simp bind;

	if (!simp_issymbol(ctx, sym))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_isbinding(ctx, frame))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	bind = simp_makevector(ctx, BINDING_SIZE);
	if (simp_isexception(ctx, bind))
		return bind;
	simp_setvector(ctx, bind, BINDING_VARIABLE, sym);
	simp_setvector(ctx, bind, BINDING_VALUE, val);
	simp_setvector(ctx, bind, BINDING_NEXT, frame);
	bind.type = TYPE_BINDING;
	return bind;
}

static int
simp_isnulenv(Simp ctx, Simp obj)
{
	return simp_isenvironment(ctx, obj) && obj.u.vector == NULL;
}

static int
simp_isnulbind(Simp ctx, Simp obj)
{
	return simp_isbinding(ctx, obj) && obj.u.vector == NULL;
}

Simp *
simp_getvector(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.vector;
}

static Simp *
simp_getclosure(Simp ctx, Simp obj)
{
	return simp_getvector(ctx, obj);
}

static Simp *
simp_getenvironment(Simp ctx, Simp obj)
{
	return simp_getvector(ctx, obj);
}

static Simp *
simp_getbind(Simp ctx, Simp obj)
{
	return simp_getvector(ctx, obj);
}

static Simp
simp_getenvframe(Simp ctx, Simp obj)
{
	return simp_getenvironment(ctx, obj)[ENVIRONMENT_FRAME];
}

static Simp
simp_getenvparent(Simp ctx, Simp obj)
{
	return simp_getenvironment(ctx, obj)[ENVIRONMENT_PARENT];
}

static Simp
simp_getbindvariable(Simp ctx, Simp obj)
{
	return simp_getbind(ctx, obj)[BINDING_VARIABLE];
}

static Simp
simp_getbindvalue(Simp ctx, Simp obj)
{
	return simp_getbind(ctx, obj)[BINDING_VALUE];
}

static Simp
simp_getbindnext(Simp ctx, Simp obj)
{
	return simp_getbind(ctx, obj)[BINDING_NEXT];
}

enum Type
simp_gettype(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.type;
}

Simp
simp_envget(Simp ctx, Simp env, Simp sym)
{
	Simp bind, var;

	if (!simp_issymbol(ctx, sym))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	for (; !simp_isnulenv(ctx, env); env = simp_getenvparent(ctx, env)) {
		for (bind = simp_getenvframe(ctx, env);
		     !simp_isnulbind(ctx, bind);
		     bind = simp_getbindnext(ctx, bind)) {
			var = simp_getbindvariable(ctx, bind);
			if (simp_issame(ctx, var, sym)) {
				return simp_getbindvalue(ctx, bind);
			}
		}
	}
	return simp_makeexception(ctx, ERROR_UNBOUND);
}

bool
xenvset(Simp ctx, Simp env, Simp var, Simp val)
{
	Simp bind, sym;

	for (bind = simp_getenvframe(ctx, env);
	     !simp_isnulbind(ctx, bind);
	     bind = simp_getbindnext(ctx, bind)) {
		sym = simp_getbindvariable(ctx, bind);
		if (simp_issame(ctx, var, sym)) {
			simp_setvector(ctx, bind, BINDING_VALUE, val);
			return true;
		}
	}
	return false;
}

Simp
simp_envset(Simp ctx, Simp env, Simp var, Simp val)
{
	if (!simp_isenvironment(ctx, env))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_issymbol(ctx, var))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (xenvset(ctx, env, var, val))
		return var;
	return simp_makeexception(ctx, ERROR_UNBOUND);
}

Simp
simp_envdef(Simp ctx, Simp env, Simp var, Simp val)
{
	Simp frame, bind;

	if (!simp_isenvironment(ctx, env))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_issymbol(ctx, var))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (xenvset(ctx, env, var, val))
		return var;
	frame = simp_getenvframe(ctx, env);
	bind = simp_makebind(ctx, var, val, frame);
	if (simp_isexception(ctx, bind))
		return bind;
	simp_setvector(ctx, env, ENVIRONMENT_FRAME, bind);
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
	return simp_makevector(ctx, SYMTAB_SIZE);
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

	ctx = simp_makevector(simp_nil(), NCONTEXTS);
	membs = simp_getvector(ctx, ctx);
	for (i = 0; i < NCONTEXTS; i++) {
		obj = (*init[i])(ctx);
		if (simp_isexception(ctx, obj))
			return obj;
		membs[i] = obj;
	}
	obj = simp_initbuiltins(ctx);
	if (simp_isexception(ctx, obj))
		return obj;
	return ctx;
}

Simp
simp_contextsymtab(Simp ctx)
{
	return simp_getvectormemb(ctx, ctx, CONTEXT_SYMTAB);
}

Simp
simp_contextforms(Simp ctx)
{
	return simp_getvectormemb(ctx, ctx, CONTEXT_FORMS);
}

Simp
simp_contextenvironment(Simp ctx)
{
	return simp_getvectormemb(ctx, ctx, CONTEXT_ENVIRONMENT);
}

Simp
simp_contextports(Simp ctx)
{
	return simp_getvectormemb(ctx, ctx, CONTEXT_PORTS);
}

Simp
simp_empty(void)
{
	return (Simp){
		.type = TYPE_STRING,
		.u.vector = NULL,
		.nmembers = 0,
		.capacity = 0,
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
simp_getbuiltin(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.builtin;
}

unsigned char
simp_getbyte(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.byte;
}

Simp
simp_getclosureenv(Simp ctx, Simp obj)
{
	enum Type type = simp_gettype(ctx, obj);

	if (type != TYPE_CLOSURE)
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getclosure(ctx, obj)[CLOSURE_ENVIRONMENT];
}

Simp
simp_getclosureparam(Simp ctx, Simp obj)
{
	enum Type type = simp_gettype(ctx, obj);

	if (type != TYPE_CLOSURE)
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getclosure(ctx, obj)[CLOSURE_PARAMETERS];
}

Simp
simp_getclosurebody(Simp ctx, Simp obj)
{
	enum Type type = simp_gettype(ctx, obj);

	if (type != TYPE_CLOSURE)
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getclosure(ctx, obj)[CLOSURE_EXPRESSIONS];
}

unsigned char *
simp_getexception(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.errmsg;
}

SimpInt
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

SimpSiz
simp_getcapacity(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.capacity;
}

SimpSiz
simp_getsize(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.nmembers;
}

unsigned char *
simp_getstring(Simp ctx, Simp obj)
{
	(void)ctx;
	if (simp_isempty(ctx, obj))
		return NULL;
	return obj.u.string;
}

unsigned char *
simp_getsymbol(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.string;
}

Simp
simp_getvectormemb(Simp ctx, Simp obj, SimpSiz pos)
{
	return simp_getvector(ctx, obj)[pos];
}

unsigned char
simp_getstringmemb(Simp ctx, Simp obj, SimpSiz pos)
{
	return simp_getstring(ctx, obj)[pos];
}

unsigned char *
simp_errorstr(int exception)
{
	if (exception < 0 || exception >= NEXCEPTIONS)
		return (unsigned char *)"unknown error";
	return errortab[exception];
}

bool
simp_isbool(Simp ctx, Simp obj)
{
	enum Type t = simp_gettype(ctx, obj);

	(void)ctx;
	return t == TYPE_TRUE || t == TYPE_FALSE;
}

bool
simp_isbuiltin(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_BUILTIN;
}

bool
simp_isclosure(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_CLOSURE;
}

bool
simp_isbyte(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_BYTE;
}

bool
simp_isempty(Simp ctx, Simp obj)
{
	return simp_isstring(ctx, obj) && obj.u.string == NULL;
}

bool
simp_isenvironment(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_ENVIRONMENT;
}

bool
simp_iseof(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_EOF;
}

bool
simp_isexception(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_EXCEPTION;
}

bool
simp_isfalse(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_FALSE;
}

bool
simp_isnil(Simp ctx, Simp obj)
{
	return simp_isvector(ctx, obj) && obj.u.vector == NULL;
}

bool
simp_isnum(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_SIGNUM;
}

bool
simp_isport(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_PORT;
}

bool
simp_isreal(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_REAL;
}

bool
simp_issame(Simp ctx, Simp a, Simp b)
{
	enum Type typea = simp_gettype(ctx, a);
	enum Type typeb = simp_gettype(ctx, b);

	if (typea != typeb)
		return false;
	switch (typea) {
	case TYPE_CLOSURE:
	case TYPE_VECTOR:
	case TYPE_BINDING:
	case TYPE_ENVIRONMENT:
		return simp_getvector(ctx, a) == simp_getvector(ctx, b);
	case TYPE_SIGNUM:
		return simp_getnum(ctx, a) == simp_getnum(ctx, b);
	case TYPE_REAL:
		return simp_getreal(ctx, a) == simp_getreal(ctx, b);
	case TYPE_PORT:
		return simp_getport(ctx, a) == simp_getport(ctx, b);
	case TYPE_BYTE:
		return simp_getbyte(ctx, a) == simp_getbyte(ctx, b);
	case TYPE_SYMBOL:
		return simp_getsymbol(ctx, a) == simp_getsymbol(ctx, b);
	case TYPE_STRING:
		return simp_getstring(ctx, a) == simp_getstring(ctx, b);
	case TYPE_EXCEPTION:
		return simp_getexception(ctx, a) == simp_getexception(ctx, b);
	case TYPE_BUILTIN:
		return simp_getbuiltin(ctx, a) == simp_getbuiltin(ctx, b);
	case TYPE_EOF:
	case TYPE_TRUE:
	case TYPE_FALSE:
		return true;
	}
	return false;
}

bool
simp_isstring(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_STRING;
}

bool
simp_issymbol(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_SYMBOL;
}

bool
simp_istrue(Simp ctx, Simp obj)
{
	return !simp_isfalse(ctx, obj);
}

bool
simp_isvector(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_VECTOR;
}

void
simp_setstring(Simp ctx, Simp obj, SimpSiz pos, unsigned char u)
{
	simp_getstring(ctx, obj)[pos] = u;
}

void
simp_setvector(Simp ctx, Simp obj, SimpSiz pos, Simp val)
{
	simp_getvector(ctx, obj)[pos] = val;
}

Simp
simp_slicevector(Simp ctx, Simp obj, SimpSiz from, SimpSiz size)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_VECTOR,
		.u.vector = obj.u.vector + from,
		.capacity = obj.capacity - from,
		.nmembers = size
	};
}

Simp
simp_slicestring(Simp ctx, Simp obj, SimpSiz from, SimpSiz size)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_STRING,
		.u.string = obj.u.string + from,
		.capacity = obj.capacity - from,
		.nmembers = size
	};
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

	if (!simp_isenvironment(ctx, parent))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	env = simp_makevector(ctx, ENVIRONMENT_SIZE);
	if (simp_isexception(ctx, env))
		return env;
	simp_setvector(ctx, env, ENVIRONMENT_PARENT, parent);
	simp_setvector(ctx, env, ENVIRONMENT_FRAME, simp_nulbind());
	env.type = TYPE_ENVIRONMENT;
	return env;
}

Simp
simp_makeexception(Simp ctx, int n)
{
	(void)ctx;
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
simp_makeclosure(Simp ctx, Simp env, Simp params, Simp body)
{
	Simp lambda;
	SimpSiz nparams, i;

	if (!simp_isenvironment(ctx, env))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	if (simp_isvector(ctx, params)) {
		nparams = simp_getsize(ctx, params);
		for (i = 0; i < nparams; i++) {
			if (!simp_issymbol(ctx, simp_getvectormemb(ctx, params, i))) {
				return simp_makeexception(ctx, ERROR_ILLEXPR);
			}
		}
	} else if (!simp_issymbol(ctx, params)) {
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	}
	lambda = simp_makevector(ctx, CLOSURE_SIZE);
	if (simp_isexception(ctx, lambda))
		return lambda;
	simp_setvector(ctx, lambda, CLOSURE_ENVIRONMENT, env);
	simp_setvector(ctx, lambda, CLOSURE_PARAMETERS, params);
	simp_setvector(ctx, lambda, CLOSURE_EXPRESSIONS, body);
	lambda.type = TYPE_CLOSURE;
	return lambda;
}

Simp
simp_makeport(Simp ctx, void *p)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_PORT,
		.nmembers = 1,
		.capacity = 1,
		.u.port = p,
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
simp_makestring(Simp ctx, unsigned char *src, SimpSiz size)
{
	unsigned char *dst = NULL;

	if (size < 0)
		return simp_makeexception(ctx, ERROR_RANGE);
	if (size == 0)
		return simp_empty();
	dst = simp_gcnewarray(ctx, size, 1);
	if (dst == NULL)
		return simp_makeexception(ctx, ERROR_MEMORY);
	if (src != NULL)
		memcpy(dst, src, size);
	return (Simp){
		.type = TYPE_STRING,
		.capacity = size,
		.nmembers = size,
		.u.string = dst,
	};
}

Simp
simp_makesymbol(Simp ctx, unsigned char *src, SimpSiz size)
{
	Simp list, prev, pair, symtab, sym;
	SimpSiz i, bucket, len;
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
	for (pair = list; !simp_isnil(ctx, pair); pair = simp_getvectormemb(ctx, pair, 1)) {
		sym = simp_getvectormemb(ctx, pair, 0);
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
	pair = simp_makevector(ctx, 2);
	if (simp_isexception(ctx, pair))
		return pair;
	simp_setvector(ctx, pair, 0, sym);
	if (simp_isnil(ctx, prev))
		simp_setvector(ctx, symtab, bucket, pair);
	else
		simp_setvector(ctx, prev, 1, pair);
	return sym;
}

Simp
simp_makevector(Simp ctx, SimpSiz size)
{
	SimpSiz i;
	Simp *data;

	if (size < 0)
		return simp_makeexception(ctx, ERROR_RANGE);
	if (size == 0)
		return simp_nil();
	data = simp_gcnewarray(ctx, size, sizeof(Simp));
	if (data == NULL)
		return simp_makeexception(ctx, ERROR_MEMORY);
	for (i = 0; i < size; i++)
		data[i] = simp_nil();
	return (Simp){
		.type = TYPE_VECTOR,
		.u.vector = data,
		.capacity = size,
		.nmembers = size,
	};
}

Simp
simp_nil(void)
{
	return (Simp){
		.type = TYPE_VECTOR,
		.capacity = 0,
		.nmembers = 0,
		.u.vector = NULL,
	};
}

Simp
simp_void(void)
{
	return simp_nil();
}

void *
simp_getgcmemory(Simp ctx, Simp obj)
{
	enum Type type;
	static bool isvector[] = {
#define X(n, v, h) [n] = v,
		TYPES
#undef  X
	};

	(void)ctx;
	type = simp_gettype(ctx, obj);
	if (isvector[type]) {
		if (obj.u.vector == NULL)
			return NULL;
		return &obj.u.vector[obj.capacity];
	}
	switch (simp_gettype(ctx, obj)) {
	case TYPE_STRING:
	case TYPE_SYMBOL:
		if (obj.u.string == NULL)
			return NULL;
		return &obj.u.string[obj.capacity];
	case TYPE_PORT:
		if (obj.u.port == NULL)
			return NULL;
		return &obj.u.port[1];
	default:
		return 0;
	}
}
