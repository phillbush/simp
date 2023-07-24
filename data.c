#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simp.h"

#define SYMTAB_SIZE     389
#define SYMTAB_MULT     37

enum {
	CONTEXT_IPORT,
	CONTEXT_OPORT,
	CONTEXT_EPORT,
	CONTEXT_SYMTAB,
	CONTEXT_ENVIRONMENT,
	NCONTEXTS
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

struct String {
	SimpSiz size;
	unsigned char *arr;
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
	bind = simp_makevector(ctx, BINDING_SIZE, simp_nil());
	if (simp_isexception(ctx, bind))
		return bind;
	simp_getvector(ctx, bind)[BINDING_VARIABLE] = sym;
	simp_getvector(ctx, bind)[BINDING_VALUE] = val;
	simp_getvector(ctx, bind)[BINDING_NEXT] = frame;
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
	if (obj.u.vector == NULL)
		return NULL;
	return obj.u.vector + 1;        /* +1 for the first element is the size */
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
			simp_getvector(ctx, bind)[BINDING_VALUE] = val;
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
	simp_getvector(ctx, env)[ENVIRONMENT_FRAME] = bind;
	return var;
}

Simp
simp_contextnew(void)
{
	Simp ctx, sym, val, obj;
	Simp membs[NCONTEXTS];
	SimpSiz i, j, len;
	unsigned char *operations[NOPERATIONS] = {
#define X(n, s) [n] = (unsigned char *)s,
		OPERATIONS
#undef  X
	};
	unsigned char *varargs[NVARARGS] = {
#define X(n, s, p) [n] = (unsigned char *)s,
		VARARGS
#undef  X
	};
	unsigned char *builtins[NBUILTINS] = {
#define X(n, s, p, min, max) [n] = (unsigned char *)s,
		BUILTINS
#undef  X
	};
	struct {
		Simp (*maker)(Simp, int);
		unsigned char **names;
		SimpSiz size;
	} funs[3] = {
		{ simp_makeform, operations, LEN(operations) },
		{ simp_makebuiltin, builtins, LEN(builtins) },
		{ simp_makevarargs, varargs, LEN(varargs) },
	};

	ctx = simp_makevector(simp_nil(), NCONTEXTS, simp_nil());
	membs[CONTEXT_IPORT] = simp_openstream(simp_nil(), stdin, "r");
	membs[CONTEXT_OPORT] = simp_openstream(simp_nil(), stdout, "w");
	membs[CONTEXT_EPORT] = simp_openstream(simp_nil(), stderr, "w");
	membs[CONTEXT_SYMTAB] = simp_makevector(simp_nil(), SYMTAB_SIZE, simp_nil());
	membs[CONTEXT_ENVIRONMENT] = simp_makeenvironment(simp_nil(), simp_nulenv());
	for (i = 0; i < NCONTEXTS; i++)
		simp_getvector(simp_nil(), ctx)[i] = membs[i];
	for (j = 0; j < LEN(funs); j++) {
		for (i = 0; i < funs[j].size; i++) {
			len = strlen((char *)funs[j].names[i]);
			sym = simp_makesymbol(ctx, funs[j].names[i], len);
			if (simp_isexception(ctx, sym))
				goto error;
			val = (*funs[j].maker)(ctx, i);
			if (simp_isexception(ctx, val))
				goto error;
			obj = simp_envdef(ctx, membs[CONTEXT_ENVIRONMENT], sym, val);
			if (simp_isexception(ctx, obj))
				goto error;
		}
	}
	return ctx;
error:
	return simp_makeexception(ctx, ERROR_MEMORY);
}

Simp
simp_contextenvironment(Simp ctx)
{
	return simp_getvectormemb(ctx, ctx, CONTEXT_ENVIRONMENT);
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
simp_empty(void)
{
	return (Simp){
		.type = TYPE_STRING,
		.u.vector = NULL,
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

enum Operations
simp_getform(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.form;
}

enum Builtins
simp_getbuiltin(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.builtin;
}

enum Varargs
simp_getvarargs(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.varargs;
}

unsigned char
simp_getbyte(Simp ctx, Simp obj)
{
	(void)ctx;
	return obj.u.byte;
}

Simp
simp_getapplicativeenv(Simp ctx, Simp obj)
{
	enum Type type = simp_gettype(ctx, obj);

	if (type != TYPE_APPLICATIVE)
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getclosure(ctx, obj)[CLOSURE_ENVIRONMENT];
}

Simp
simp_getapplicativeparam(Simp ctx, Simp obj)
{
	enum Type type = simp_gettype(ctx, obj);

	if (type != TYPE_APPLICATIVE)
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getclosure(ctx, obj)[CLOSURE_PARAMETERS];
}

Simp
simp_getapplicativebody(Simp ctx, Simp obj)
{
	enum Type type = simp_gettype(ctx, obj);

	if (type != TYPE_APPLICATIVE)
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getclosure(ctx, obj)[CLOSURE_EXPRESSIONS];
}

Simp
simp_getoperativeenv(Simp ctx, Simp obj)
{
	enum Type type = simp_gettype(ctx, obj);

	if (type != TYPE_OPERATIVE)
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getclosure(ctx, obj)[CLOSURE_ENVIRONMENT];
}

Simp
simp_getoperativeparam(Simp ctx, Simp obj)
{
	enum Type type = simp_gettype(ctx, obj);

	if (type != TYPE_OPERATIVE)
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getclosure(ctx, obj)[CLOSURE_PARAMETERS];
}

Simp
simp_getoperativebody(Simp ctx, Simp obj)
{
	enum Type type = simp_gettype(ctx, obj);

	if (type != TYPE_OPERATIVE)
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
simp_getsize(Simp ctx, Simp obj)
{
	(void)ctx;
	switch (simp_gettype(ctx, obj)) {
	case TYPE_VECTOR:
		if (obj.u.vector == NULL)
			return 0;
		return (SimpSiz)simp_getnum(ctx, obj.u.vector[0]);      /* first element is the size */
	case TYPE_STRING:
	case TYPE_SYMBOL:
	case TYPE_EXCEPTION:
		if (obj.u.string == NULL)
			return 0;
		return ((struct String *)obj.u.string)->size;
	default:
		return 0;
	}
}

unsigned char *
simp_getstring(Simp ctx, Simp obj)
{
	(void)ctx;
	if (simp_isempty(ctx, obj))
		return NULL;
	return ((struct String *)obj.u.string)->arr;
}

unsigned char *
simp_getsymbol(Simp ctx, Simp obj)
{
	(void)ctx;
	return ((struct String *)obj.u.string)->arr;
}

Simp
simp_getstringmemb(Simp ctx, Simp obj, SimpSiz pos)
{
	if (!simp_isstring(ctx, obj) || pos >= simp_getsize(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_makebyte(ctx, simp_getstring(ctx, obj)[pos]);
}

Simp
simp_getvectormemb(Simp ctx, Simp obj, SimpSiz pos)
{
	if (!simp_isvector(ctx, obj) || pos >= simp_getsize(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_getvector(ctx, obj)[pos];
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
simp_isvarargs(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_VARARGS;
}

bool
simp_isbuiltin(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_BUILTIN;
}

bool
simp_isform(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_FORM;
}

bool
simp_isapplicative(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_APPLICATIVE;
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
simp_isoperative(Simp ctx, Simp obj)
{
	(void)ctx;
	return simp_gettype(ctx, obj) == TYPE_OPERATIVE;
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
	case TYPE_APPLICATIVE:
	case TYPE_OPERATIVE:
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
	case TYPE_FORM:
		return simp_getform(ctx, a) == simp_getform(ctx, b);
	case TYPE_BUILTIN:
		return simp_getbuiltin(ctx, a) == simp_getbuiltin(ctx, b);
	case TYPE_VARARGS:
		return simp_getvarargs(ctx, a) == simp_getvarargs(ctx, b);
	case TYPE_EOF:
	case TYPE_TRUE:
	case TYPE_FALSE:
	case TYPE_VOID:
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

bool
simp_isvoid(Simp ctx, Simp obj)
{
	return simp_gettype(ctx, obj) == TYPE_VOID;
}

Simp
simp_setstring(Simp ctx, Simp obj, Simp pos, Simp val)
{
	SimpInt size, n;
	unsigned char u;

	if (!simp_isstring(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_isnum(ctx, pos))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_isbyte(ctx, val))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, obj);
	n = simp_getnum(ctx, pos);
	u = simp_getbyte(ctx, val);
	if (n < 0 || n >= size)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	simp_getstring(ctx, obj)[n] = u;
	return obj;
}

Simp
simp_setvector(Simp ctx, Simp obj, Simp pos, Simp val)
{
	SimpInt size, n;

	if (!simp_isvector(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_isnum(ctx, pos))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, obj);
	n = simp_getnum(ctx, pos);
	if (n < 0 || n >= size)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	simp_getvector(ctx, obj)[n] = val;
	return obj;
}

Simp
simp_true(void)
{
	return (Simp){ .type = TYPE_TRUE };
}

Simp
simp_makeform(Simp ctx, int form)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_FORM,
		.u.form = form,
	};
}

Simp
simp_makebuiltin(Simp ctx, int builtin)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_BUILTIN,
		.u.builtin = builtin,
	};
}

Simp
simp_makevarargs(Simp ctx, int varargs)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_VARARGS,
		.u.varargs = varargs,
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
	env = simp_makevector(ctx, ENVIRONMENT_SIZE, simp_nil());
	if (simp_isexception(ctx, env))
		return env;
	simp_getvector(ctx, env)[ENVIRONMENT_PARENT] = parent;
	simp_getvector(ctx, env)[ENVIRONMENT_FRAME] = simp_nulbind();
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

static Simp
makeclosure(Simp ctx, Simp env, Simp params, Simp body, enum Type type)
{
	Simp macro;
	SimpSiz nparams, i;

	if (!simp_isenvironment(ctx, env))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	if (!simp_isvector(ctx, params))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	nparams = simp_getsize(ctx, params);
	if (type == TYPE_OPERATIVE && nparams < 1)
		return simp_makeexception(ctx, ERROR_ENVIRON);
	for (i = 0; i < nparams; i++)
		if (!simp_issymbol(ctx, simp_getvectormemb(ctx, params, i)))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
	macro = simp_makevector(ctx, CLOSURE_SIZE, simp_nil());
	if (simp_isexception(ctx, macro))
		return macro;
	simp_getvector(ctx, macro)[CLOSURE_ENVIRONMENT] = env;
	simp_getvector(ctx, macro)[CLOSURE_PARAMETERS] = params;
	simp_getvector(ctx, macro)[CLOSURE_EXPRESSIONS] = body;
	macro.type = type;
	return macro;
}

Simp
simp_makeapplicative(Simp ctx, Simp env, Simp params, Simp body)
{
	return makeclosure(ctx, env, params, body, TYPE_APPLICATIVE);
}

Simp
simp_makeoperative(Simp ctx, Simp env, Simp params, Simp body)
{
	return makeclosure(ctx, env, params, body, TYPE_OPERATIVE);
}

Simp
simp_makeport(Simp ctx, void *p)
{
	(void)ctx;
	return (Simp){
		.type = TYPE_PORT,
		.u.port = (void *)p,
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
	struct String *p = NULL;
	unsigned char *dst = NULL;

	if (size < 0)
		return simp_makeexception(ctx, ERROR_RANGE);
	if (size == 0)
		return simp_empty();
	if ((p = malloc(sizeof(*p))) == NULL)
		goto error;
	if ((dst = calloc(size, 1)) == NULL)
		goto error;
	*p = (struct String){
		.size = size,
		.arr = dst,
	};
	if (src != NULL)
		memcpy(dst, src, size);
	return (Simp){
		.type = TYPE_STRING,
		.u.string = p,
	};
error:
	free(p);
	free(dst);
	return simp_makeexception(ctx, ERROR_MEMORY);
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
	pair = simp_makevector(ctx, 2, simp_nil());
	if (simp_isexception(ctx, pair))
		return pair;
	simp_getvector(ctx, pair)[0] = sym;
	if (simp_isnil(ctx, prev))
		simp_getvector(ctx, symtab)[bucket] = pair;
	else
		simp_getvector(ctx, prev)[1] = pair;
	return sym;
}

Simp
simp_makevector(Simp ctx, SimpSiz size, Simp fill)
{
	Simp *dst = NULL;
	SimpSiz i;

	if (size < 0)
		return simp_makeexception(ctx, ERROR_RANGE);
	if (size == 0)
		return simp_nil();
	if ((dst = calloc(size + 1, sizeof(*dst))) == NULL)
		return simp_makeexception(ctx, ERROR_MEMORY);
	dst[0] = simp_makenum(ctx, (SimpInt)size);
	for (i = 0; i < size; i++)
		dst[i + 1] = fill;
	return (Simp){
		.type = TYPE_VECTOR,
		.u.vector = dst,
	};
}

Simp
simp_nil(void)
{
	return (Simp){
		.type = TYPE_VECTOR,
		.u.vector = NULL,
	};
}

Simp
simp_void(void)
{
	return (Simp){ .type = TYPE_VOID };
}

Simp
simp_unwrap(Simp ctx, Simp obj)
{
	if (simp_isoperative(ctx, obj))
		return obj;
	if (!simp_isapplicative(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	obj.type = TYPE_OPERATIVE;
	return obj;
}

Simp
simp_wrap(Simp ctx, Simp obj)
{
	if (simp_isapplicative(ctx, obj))
		return obj;
	if (!simp_isoperative(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	obj.type = TYPE_APPLICATIVE;
	return obj;
}
