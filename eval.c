#include <string.h>

#include "simp.h"

#define FORMS                                            \
	X(FORM_AND,             "and"                   )\
	X(FORM_APPLY,           "apply"                 )\
	X(FORM_APPLY2,          "!"                     )\
	X(FORM_DEFINE,          "define"                )\
	X(FORM_DO,              "do"                    )\
	X(FORM_EVAL,            "eval"                  )\
	X(FORM_ENV,             "env"                   )\
	X(FORM_FALSE,           "false"                 )\
	X(FORM_IF,              "if"                    )\
	X(FORM_LAMBDA,          "lambda"                )\
	X(FORM_OR,              "or"                    )\
	X(FORM_QUOTE,           "quote"                 )\
	X(FORM_SET,             "set!"                  )\
	X(FORM_TRUE,            "true"                  )\
	X(FORM_VECTOR,          "vector"                )

#define BUILTINS                                                       \
	X(F_BOOLEANP,     "boolean?",            f_booleanp,        1 )\
	X(F_BYTEP,        "byte?",               f_bytep,           1 )\
	X(F_CURIPORT,     "current-input-port",  f_curiport,        0 )\
	X(F_CUROPORT,     "current-output-port", f_curoport,        0 )\
	X(F_CUREPORT,     "current-error-port",  f_cureport,        0 )\
	X(F_DISPLAY,      "display",             f_display,         1 )\
	X(F_EQUAL,        "=",                   f_equal,           2 )\
	X(F_FALSEP,       "falsep",              f_falsep,          1 )\
	X(F_GT,           ">",                   f_gt,              2 )\
	X(F_LT,           "<",                   f_lt,              2 )\
	X(F_MAKESTRING,   "make-string",         f_makestring,      1 )\
	X(F_MAKEVECTOR,   "make-vector",         f_makevector,      1 )\
	X(F_MAKEENV,      "make-environment",    f_makeenvironment, 1 )\
	X(F_NEWLINE,      "newline",             f_newline,         0 )\
	X(F_NULLP,        "null?",               f_nullp,           1 )\
	X(F_PORTP,        "port?",               f_portp,           1 )\
	X(F_SAMEP,        "same?",               f_samep,           2 )\
	X(F_SLICESTRING,  "slice-string",        f_slicestring,     3 )\
	X(F_SLICEVECTOR,  "slice-vector",        f_slicevector,     3 )\
	X(F_STRINGCMP,    "string-compare",      f_stringcmp,       2 )\
	X(F_STRINGLEN,    "string-length",       f_stringlen,       1 )\
	X(F_STRINGREF,    "string-ref",          f_stringref,       2 )\
	X(F_STRINGP,      "string?",             f_stringp,         1 )\
	X(F_STRINGSET,    "string-set!",         f_stringset,       3 )\
	X(F_STRINGVECTOR, "string->vector",      f_stringvector,    1 )\
	X(F_SYMBOLP,      "symbol?",             f_symbolp,         1 )\
	X(F_TRUEP,        "true?",               f_truep,           1 )\
	X(F_VECTORREF,    "vector-ref",          f_vectorref,       2 )\
	X(F_VECTORLEN,    "vector-length",       f_vectorlen,       1 )\
	X(F_VECTORSET,    "vector-set!",         f_vectorset,       3 )\
	X(F_WRITE,        "write",               f_write,           2 )\
	X(F_ADD,          "+",                   f_add,             2 )\
	X(F_DIVIDE,       "/",                   f_divide,          2 )\
	X(F_MULTIPLY,     "*",                   f_multiply,        2 )\
	X(F_SUBTRACT,     "-",                   f_subtract,        2 )

enum Builtins {
#define X(n, s, p, a) n,
	BUILTINS
	NBUILTINS
#undef  X
};

enum Forms {
#define X(n, s) n,
	FORMS
#undef  X
};

struct Builtin {
	char *name;
	Simp (*fun)(Simp, Simp);
	SimpSiz nargs;
};

static Simp
typepred(Simp ctx, Simp args, bool (*pred)(Simp, Simp))
{
	Simp obj;

	obj = simp_getvectormemb(ctx, args, 0);
	return (*pred)(ctx, obj) ? simp_true() : simp_false();
}

static Simp
f_add(Simp ctx, Simp args)
{
	SimpInt n;
	Simp a, b;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isnum(ctx, a) || !simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	n = simp_getnum(ctx, a) + simp_getnum(ctx, b);
	return simp_makenum(ctx, n);
}

static Simp
f_bytep(Simp ctx, Simp args)
{
	return typepred(ctx, args, simp_isbyte);
}

static Simp
f_booleanp(Simp ctx, Simp args)
{
	return typepred(ctx, args, simp_isbool);
}

static Simp
f_curiport(Simp ctx, Simp args)
{
	(void)args;
	return simp_contextiport(ctx);
}

static Simp
f_curoport(Simp ctx, Simp args)
{
	(void)args;
	return simp_contextoport(ctx);
}

static Simp
f_cureport(Simp ctx, Simp args)
{
	(void)args;
	return simp_contexteport(ctx);
}

static Simp
f_display(Simp ctx, Simp args)
{
	Simp obj;

	obj = simp_getvectormemb(ctx, args, 0);
	return simp_display(ctx, simp_contextoport(ctx), obj);
}

static Simp
f_divide(Simp ctx, Simp args)
{
	SimpInt d;
	Simp a, b;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isnum(ctx, a) || !simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	d = simp_getnum(ctx, b);
	if (d == 0)
		return simp_makeexception(ctx, ERROR_DIVZERO);
	d = simp_getnum(ctx, a) / d;
	return simp_makenum(ctx, d);
}

static Simp
f_equal(Simp ctx, Simp args)
{
	Simp a, b;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isnum(ctx, a) || !simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (simp_getnum(ctx, a) == simp_getnum(ctx, b))
		return simp_true();
	return simp_false();
}

static Simp
f_falsep(Simp ctx, Simp args)
{
	return typepred(ctx, args, simp_isfalse);
}

static Simp
f_gt(Simp ctx, Simp args)
{
	Simp a, b;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isnum(ctx, a) || !simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (simp_getnum(ctx, a) > simp_getnum(ctx, b))
		return simp_true();
	return simp_false();
}

static Simp
f_lt(Simp ctx, Simp args)
{
	Simp a, b;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isnum(ctx, a) || !simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (simp_getnum(ctx, a) < simp_getnum(ctx, b))
		return simp_true();
	return simp_false();
}

static Simp
f_makeenvironment(Simp ctx, Simp args)
{
	Simp env;

	env = simp_getvectormemb(ctx, args, 0);
	return simp_makeenvironment(ctx, env);
}

static Simp
f_makestring(Simp ctx, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(ctx, args, 0);
	if (!simp_isnum(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getnum(ctx, obj);
	if (size < 0)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	return simp_makestring(ctx, NULL, size);
}

static Simp
f_makevector(Simp ctx, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(ctx, args, 0);
	if (!simp_isnum(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getnum(ctx, obj);
	if (size < 0)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	return simp_makevector(ctx, size);
}

static Simp
f_multiply(Simp ctx, Simp args)
{
	SimpInt n;
	Simp a, b;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isnum(ctx, a) || !simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	n = simp_getnum(ctx, a) * simp_getnum(ctx, b);
	return simp_makenum(ctx, n);
}

static Simp
f_newline(Simp ctx, Simp args)
{
	(void)args;
	return simp_printf(ctx, simp_contextoport(ctx), "\n");
}

static Simp
f_nullp(Simp ctx, Simp args)
{
	return typepred(ctx, args, simp_isnil);
}

static Simp
f_portp(Simp ctx, Simp args)
{
	return typepred(ctx, args, simp_isport);
}

static Simp
f_samep(Simp ctx, Simp args)
{
	Simp a, b;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	return simp_issame(ctx, a, b) ? simp_true() : simp_false();
}

static Simp
f_slicevector(Simp ctx, Simp args)
{
	Simp v, a, b;
	SimpSiz from, size, capacity;

	v = simp_getvectormemb(ctx, args, 0);
	a = simp_getvectormemb(ctx, args, 1);
	b = simp_getvectormemb(ctx, args, 2);
	if (!simp_isvector(ctx, v))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_isnum(ctx, a))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	from = simp_getnum(ctx, a);
	size = simp_getnum(ctx, b);
	capacity = simp_getcapacity(ctx, v);
	if (from < 0 || from > capacity)
		return simp_makeexception(ctx, ERROR_RANGE);
	if (size < 0 || from + size > capacity)
		return simp_makeexception(ctx, ERROR_RANGE);
	return simp_slicevector(ctx, v, from, size);
}

static Simp
f_slicestring(Simp ctx, Simp args)
{
	Simp v, a, b;
	SimpSiz from, size, capacity;

	v = simp_getvectormemb(ctx, args, 0);
	a = simp_getvectormemb(ctx, args, 1);
	b = simp_getvectormemb(ctx, args, 2);
	if (!simp_isstring(ctx, v))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_isnum(ctx, a))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (!simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	from = simp_getnum(ctx, a);
	size = simp_getnum(ctx, b);
	capacity = simp_getcapacity(ctx, v);
	if (from < 0 || from > capacity)
		return simp_makeexception(ctx, ERROR_RANGE);
	if (size < 0 || from + size > capacity)
		return simp_makeexception(ctx, ERROR_RANGE);
	return simp_slicestring(ctx, v, from, size);
}

static Simp
f_subtract(Simp ctx, Simp args)
{
	SimpInt n;
	Simp a, b;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isnum(ctx, a) || !simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	n = simp_getnum(ctx, a) - simp_getnum(ctx, b);
	return simp_makenum(ctx, n);
}

static Simp
f_stringcmp(Simp ctx, Simp args)
{
	SimpSiz size0, size1;
	Simp a, b;
	int cmp;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isstring(ctx, a) || !simp_isstring(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size0 = simp_getsize(ctx, a);
	size1 = simp_getsize(ctx, b);
	cmp = memcmp(
		simp_getstring(ctx, a),
		simp_getstring(ctx, b),
		size0 < size1 ? size0 : size1
	);
	if (cmp == 0 && size0 != size1)
		cmp = size0 < size1 ? -1 : +1;
	else if (cmp < 0)
		cmp = -1;
	else if (cmp > 0)
		cmp = +1;
	return simp_makenum(ctx, cmp);
}

static Simp
f_stringlen(Simp ctx, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(ctx, args, 0);
	if (!simp_isstring(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, obj);
	return simp_makenum(ctx, size);
}

static Simp
f_stringref(Simp ctx, Simp args)
{
	SimpSiz size;
	SimpInt pos;
	Simp a, b;
	unsigned u;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isstring(ctx, a) || !simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, a);
	pos = simp_getnum(ctx, b);
	if (pos < 0 || pos >= (SimpInt)size)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	u = simp_getstringmemb(ctx, a, pos);
	return simp_makebyte(ctx, u);
}

static Simp
f_stringp(Simp ctx, Simp args)
{
	return typepred(ctx, args, simp_isstring);
}

static Simp
f_stringvector(Simp ctx, Simp args)
{
	Simp vector, str, byte;
	SimpSiz i, size;
	unsigned char u;

	str = simp_getvectormemb(ctx, args, 0);
	if (!simp_isstring(ctx, str))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, str);
	vector = simp_makevector(ctx, size);
	if (simp_isexception(ctx, vector))
		return vector;
	for (i = 0; i < size; i++) {
		u = simp_getstringmemb(ctx, str, i);
		byte = simp_makebyte(ctx, u);
		if (simp_isexception(ctx, byte))
			return byte;
		simp_setvector(ctx, vector, i, byte);
	}
	return vector;
}

Simp
f_stringset(Simp ctx, Simp args)
{
	Simp str, pos, val;
	SimpSiz size;
	SimpInt n;
	unsigned char u;

	str = simp_getvectormemb(ctx, args, 0);
	pos = simp_getvectormemb(ctx, args, 1);
	val = simp_getvectormemb(ctx, args, 2);
	if (!simp_isstring(ctx, str) || !simp_isnum(ctx, pos) || !simp_isbyte(ctx, val))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, str);
	n = simp_getnum(ctx, pos);
	u = simp_getbyte(ctx, val);
	if (n < 0 || n >= (SimpInt)size)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	simp_setstring(ctx, str, n, u);
	return simp_void();
}

static Simp
f_symbolp(Simp ctx, Simp args)
{
	return typepred(ctx, args, simp_issymbol);
}

static Simp
f_truep(Simp ctx, Simp args)
{
	return typepred(ctx, args, simp_istrue);
}

Simp
f_vectorset(Simp ctx, Simp args)
{
	Simp str, pos, val;
	SimpSiz size;
	SimpInt n;

	str = simp_getvectormemb(ctx, args, 0);
	pos = simp_getvectormemb(ctx, args, 1);
	val = simp_getvectormemb(ctx, args, 2);
	if (!simp_isvector(ctx, str) || !simp_isnum(ctx, pos))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, str);
	n = simp_getnum(ctx, pos);
	if (n < 0 || n >= (SimpInt)size)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	simp_setvector(ctx, str, n, val);
	return simp_void();
}

static Simp
f_vectorlen(Simp ctx, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(ctx, args, 0);
	if (!simp_isvector(ctx, obj))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, obj);
	return simp_makenum(ctx, size);
}

static Simp
f_vectorref(Simp ctx, Simp args)
{
	SimpSiz size;
	SimpInt pos;
	Simp a, b;

	a = simp_getvectormemb(ctx, args, 0);
	b = simp_getvectormemb(ctx, args, 1);
	if (!simp_isvector(ctx, a) || !simp_isnum(ctx, b))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, a);
	pos = simp_getnum(ctx, b);
	if (pos < 0 || pos >= (SimpInt)size)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	return simp_getvectormemb(ctx, a, pos);
}

Simp
f_write(Simp ctx, Simp args)
{
	Simp obj, port;

	port = simp_getvectormemb(ctx, args, 0);
	if (!simp_isport(ctx, port))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	obj = simp_getvectormemb(ctx, args, 1);
	return simp_write(ctx, port, obj);
}

static Simp
defineorset(Simp ctx, Simp expr, Simp env, bool define)
{
	Simp symbol, value;

	if (simp_getsize(ctx, expr) != 3)       /* (define VAR VAL) */
		return simp_makeexception(ctx, ERROR_ARGS);
	symbol = simp_getvectormemb(ctx, expr, 1);
	if (!simp_issymbol(ctx, symbol))
		return simp_makeexception(ctx, ERROR_NOTSYM);
	value = simp_getvectormemb(ctx, expr, 2);
	value = simp_eval(ctx, value, env);
	if (simp_isexception(ctx, value))
		return value;
	if (define)
		value = simp_envdef(ctx, env, symbol, value);
	else
		value = simp_envset(ctx, env, symbol, value);
	if (simp_isexception(ctx, value))
		return value;
	return simp_void();
}

static Simp
set(Simp ctx, Simp expr, Simp env)
{
	return defineorset(ctx, expr, env, false);
}

static Simp
define(Simp ctx, Simp expr, Simp env)
{
	return defineorset(ctx, expr, env, true);
}

Simp
simp_initforms(Simp ctx)
{
	Simp forms, sym;
	SimpSiz i, len;
	unsigned char *formnames[] = {
#define X(n, s) [n] = (unsigned char *)s,
		FORMS
#undef  X
	};

	forms = simp_makevector(ctx, LEN(formnames));
	if (simp_isexception(ctx, forms))
		return forms;
	for (i = 0; i < LEN(formnames); i++) {
		len = strlen((char *)formnames[i]);
		sym = simp_makesymbol(ctx, formnames[i], len);
		if (simp_isexception(ctx, sym))
			return sym;
		simp_setvector(ctx, forms, i, sym);
	}
	return forms;
}

Simp
simp_initbuiltins(Simp ctx)
{
	Simp env, sym, ret, val;
	SimpSiz i, len;
	static Builtin builtins[] = {
#define X(n, s, p, a) [n] = { .name = s, .fun = &p, .nargs = a, },
		BUILTINS
#undef  X
	};

	env = simp_contextenvironment(ctx);
	for (i = 0; i < LEN(builtins); i++) {
		len = strlen(builtins[i].name);
		sym = simp_makesymbol(ctx, (unsigned char *)builtins[i].name, len);
		if (simp_isexception(ctx, sym))
			return sym;
		val = simp_makebuiltin(ctx, &builtins[i]);
		if (simp_isexception(ctx, val))
			return val;
		ret = simp_envdef(ctx, env, sym, val);
		if (simp_isexception(ctx, ret))
			return ret;
	}
	return simp_nil();
}

Simp
simp_eval(Simp ctx, Simp expr, Simp env)
{
	Builtin *bltin;
	Simp *forms;
	Simp vect, form, operator, args, body, params, cloenv, var, val;
	SimpSiz nobjects, nargs, nparams, i;

	forms = simp_getvector(ctx, simp_contextforms(ctx));
loop:
	if (simp_issymbol(ctx, expr))   /* expression is variable */
		return simp_envget(ctx, env, expr);
	if (!simp_isvector(ctx, expr))  /* expression is self-evaluating */
		return expr;
	if ((nobjects = simp_getsize(ctx, expr)) == 0)
		return simp_makeexception(ctx, ERROR_EMPTY);
	form = simp_getvectormemb(ctx, expr, 0);
	if (!simp_issymbol(ctx, form))
		return simp_makeexception(ctx, ERROR_ILLSYNTAX);
	if (simp_issame(ctx, form, forms[FORM_APPLY]) ||
	    simp_issame(ctx, form, forms[FORM_APPLY2])) {
		if (nobjects < 2)               /* (apply OP ...) */
			return simp_makeexception(ctx, ERROR_ILLFORM);
		nargs = nobjects - 2;
		operator = simp_getvectormemb(ctx, expr, 1);
		operator = simp_eval(ctx, operator, env);
		if (simp_isexception(ctx, operator))
			return operator;
		args = simp_makevector(ctx, nargs);
		if (simp_isexception(ctx, args))
			return args;
		for (i = 0; i < nargs; i++) {
			/* evaluate arguments */
			val = simp_getvectormemb(ctx, expr, i + 2);
			if (simp_isexception(ctx, val))
				return val;
			val = simp_eval(ctx, val, env);
			if (simp_isexception(ctx, val))
				return val;
			simp_setvector(ctx, args, i, val);
		}
		if (simp_isbuiltin(ctx, operator)) {
			bltin = simp_getbuiltin(ctx, operator);
			if (nargs != bltin->nargs)
				return simp_makeexception(ctx, ERROR_ARGS);
			return (*bltin->fun)(ctx, args);
		}
		if (!simp_isclosure(ctx, operator))
			return simp_makeexception(ctx, ERROR_OPERATOR);
		body = simp_getclosurebody(ctx, operator);
		params = simp_getclosureparam(ctx, operator);
		cloenv = simp_getclosureenv(ctx, operator);
		nparams = simp_getsize(ctx, params);
		if (nargs != nparams)
			return simp_makeexception(ctx, ERROR_ARGS);
		cloenv = simp_makeenvironment(ctx, cloenv);
		if (simp_isexception(ctx, cloenv))
			return cloenv;
		for (i = 0; i < nparams; i++) {
			/* fill environment */
			var = simp_getvectormemb(ctx, params, i);
			val = simp_getvectormemb(ctx, args, i);
			var = simp_envdef(ctx, cloenv, var, val);
			if (simp_isexception(ctx, var)) {
				return var;
			}
		}
		expr = body;
		env = cloenv;
		goto loop;
	} else if (simp_issame(ctx, form, forms[FORM_AND])) {
		val = simp_true();
		for (i = 1; i < nobjects; i++) {
			val = simp_getvectormemb(ctx, expr, i);
			val = simp_eval(ctx, val, env);
			if (simp_isexception(ctx, val))
				return val;
			if (simp_isfalse(ctx, val))
				return val;
		}
		return val;
	} else if (simp_issame(ctx, form, forms[FORM_OR])) {
		val = simp_false();
		for (i = 1; i < nobjects; i++) {
			val = simp_getvectormemb(ctx, expr, i);
			val = simp_eval(ctx, val, env);
			if (simp_isexception(ctx, val))
				return val;
			if (simp_istrue(ctx, val))
				return val;
		}
		return val;
	} else if (simp_issame(ctx, form, forms[FORM_DEFINE])) {
		return define(ctx, expr, env);
	} else if (simp_issame(ctx, form, forms[FORM_DO])) {
		if (nobjects < 2)               /* (do ...) */
			return simp_void();
		for (i = 1; i + 1 < nobjects; i++) {
			val = simp_getvectormemb(ctx, expr, i);
			(void)simp_eval(ctx, val, env);
		}
		expr = simp_getvectormemb(ctx, expr, i);
		goto loop;
	} else if (simp_issame(ctx, form, forms[FORM_IF])) {
		if (nobjects < 3)               /* (if COND THEN ELSE ...) */
			return simp_makeexception(ctx, ERROR_ILLFORM);
		for (i = 1; i < nobjects; i++) {
			if (i + 1 == nobjects) {
				expr = simp_getvectormemb(ctx, expr, i);
				goto loop;
			}
			val = simp_getvectormemb(ctx, expr, i);
			val = simp_eval(ctx, val, env);
			if (simp_isexception(ctx, val))
				return val;
			i++;
			if (simp_istrue(ctx, val)) {
				expr = simp_getvectormemb(ctx, expr, i);
				goto loop;
			}
		}
		return simp_void();
	} else if (simp_issame(ctx, form, forms[FORM_ENV])) {
		return env;
	} else if (simp_issame(ctx, form, forms[FORM_EVAL])) {
		if (nobjects != 3)              /* (eval EXPR ENV) */
			return simp_makeexception(ctx, ERROR_ILLFORM);
		env = simp_getvectormemb(ctx, expr, 2);
		expr = simp_getvectormemb(ctx, expr, 1);
		goto loop;
	} else if (simp_issame(ctx, form, forms[FORM_LAMBDA])) {
		if (nobjects < 2)               /* (lambda args ... body) */
			return simp_makeexception(ctx, ERROR_ARGS);
		params = simp_makevector(ctx, nobjects - 2);
		if (simp_isexception(ctx, params))
			return params;
		for (i = 0; i + 2 < nobjects; i++) {
			var = simp_getvectormemb(ctx, expr, i + 1);
			if (!simp_issymbol(ctx, var))
				return simp_makeexception(ctx, ERROR_ILLFORM);
			simp_setvector(ctx, params, i, var);
		}
		body = simp_getvectormemb(ctx, expr, nobjects - 1);
		return simp_makeclosure(ctx, env, params, body);
	} else if (simp_issame(ctx, form, forms[FORM_QUOTE])) {
		if (nobjects != 2)              /* (quote OBJ) */
			return simp_makeexception(ctx, ERROR_ILLFORM);
		return simp_getvectormemb(ctx, expr, 1);
	} else if (simp_issame(ctx, form, forms[FORM_SET])) {
		return set(ctx, expr, env);
	} else if (simp_issame(ctx, form, forms[FORM_FALSE])) {
		if (nobjects != 1)              /* (false) */
			return simp_makeexception(ctx, ERROR_ILLFORM);
		return simp_false();
	} else if (simp_issame(ctx, form, forms[FORM_TRUE])) {
		if (nobjects != 1)              /* (true) */
			return simp_makeexception(ctx, ERROR_ILLFORM);
		return simp_true();
	} else if (simp_issame(ctx, form, forms[FORM_VECTOR])) {
		vect = simp_makevector(ctx, nobjects - 1);
		if (simp_isexception(ctx, vect))
			return vect;
		for (i = 0; i + 1 < nobjects; i++) {
			/* evaluate arguments */
			val = simp_getvectormemb(ctx, expr, i + 1);
			if (simp_isexception(ctx, val))
				return val;
			val = simp_eval(ctx, val, env);
			if (simp_isexception(ctx, val))
				return val;
			simp_setvector(ctx, vect, i, val);
		}
		return vect;
	}
	return simp_makeexception(ctx, ERROR_UNKSYNTAX);
}
