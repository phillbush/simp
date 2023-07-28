#include <string.h>

#include "simp.h"

#define MAXARGS  4

/* 0th element is form, 1th is operator, arguments begin at pos 2 */
#define ARGS_POS 2

#define FORMS                                            \
	X(FORM_APPLY,           "apply"                 )\
	X(FORM_DEFINE,          "define"                )\
	X(FORM_EVAL,            "eval"                  )\
	X(FORM_IF,              "if"                    )\
	X(FORM_LAMBDA,          "lambda"                )\
	X(FORM_QUOTE,           "quote"                 )\
	X(FORM_SET,             "set!"                  )

enum Forms {
#define X(n, s) n,
	FORMS
#undef  X
};

static Simp
typepred(Simp ctx, Simp args[], SimpSiz nargs, bool (*pred)(Simp, Simp))
{
	if (nargs != 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	return (*pred)(ctx, args[0]) ? simp_true() : simp_false();
}

static Simp
f_add(Simp ctx, Simp expr, Simp env)
{
	SimpSiz i, noperands;
	SimpInt num = 0;
	Simp obj;

	noperands = simp_getsize(ctx, expr);
	for (i = ARGS_POS; i < noperands; i++) {
		obj = simp_getvectormemb(ctx, expr, i);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		num += simp_getnum(ctx, obj);
	}
	return simp_makenum(ctx, num);
}

static Simp
f_bytep(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	return typepred(ctx, args, nargs, simp_isbyte);
}

static Simp
f_booleanp(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	return typepred(ctx, args, nargs, simp_isbool);
}

static Simp
f_curiport(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env, (void)args, (void)nargs;
	if (nargs != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_contextiport(ctx);
}

static Simp
f_curoport(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env, (void)args, (void)nargs;
	if (nargs != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_contextoport(ctx);
}

static Simp
f_cureport(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env, (void)args, (void)nargs;
	if (nargs != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_contexteport(ctx);
}

static Simp
f_display(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	if (nargs > 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (nargs == 2)
		return simp_display(ctx, args[1], args[0]);
	return simp_display(ctx, simp_contextoport(ctx), args[0]);
}

static Simp
f_divide(Simp ctx, Simp expr, Simp env)
{
	SimpSiz i, noperands;
	SimpInt num = 1;
	Simp obj;
	bool gotop = false;

	if ((noperands = simp_getsize(ctx, expr)) < 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = ARGS_POS; i < noperands; i++) {
		obj = simp_getvectormemb(ctx, expr, i);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		if (gotop)
			num /= simp_getnum(ctx, obj);
		else
			num = simp_getnum(ctx, obj);
		gotop = true;
	}
	if (noperands == 1)
		return simp_makenum(ctx, 1 / num);
	return simp_makenum(ctx, num);
}

static Simp
f_equal(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	if (nargs != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (simp_getnum(ctx, args[0]) == simp_getnum(ctx, args[1]))
		return simp_true();
	return simp_false();
}

static Simp
f_false(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	(void)args;
	if (nargs != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_false();
}

static Simp
f_falsep(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	return typepred(ctx, args, nargs, simp_isfalse);
}

static Simp
f_gt(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	if (nargs != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (simp_getnum(ctx, args[0]) > simp_getnum(ctx, args[1]))
		return simp_true();
	return simp_false();
}

static Simp
f_lt(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	if (nargs != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (simp_getnum(ctx, args[0]) < simp_getnum(ctx, args[1]))
		return simp_true();
	return simp_false();
}

static Simp
f_makeenvironment(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	if (nargs > 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (nargs == 1)
		return simp_makeenvironment(ctx, args[0]);
	return simp_makeenvironment(ctx, env);
}

static Simp
f_makestring(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	SimpInt i, size;
	Simp obj;
	unsigned char byte;
	unsigned char *string;

	(void)env;
	if (nargs > 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (!simp_isnum(ctx, args[0]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (nargs == 2) {
		if (!simp_isbyte(ctx, args[1]))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		byte = simp_getbyte(ctx, args[1]);
	} else {
		byte = '\0';
	}
	size = simp_getnum(ctx, args[0]);
	if (size < 0)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	obj = simp_makestring(ctx, NULL, size);
	if (simp_isexception(ctx, obj))
		return obj;
	string = simp_getstring(ctx, obj);
	for (i = 0; i < size; i++)
		string[i] = byte;
	return obj;
}

static Simp
f_makevector(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	SimpInt size;
	Simp fill;

	(void)env;
	if (nargs > 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (!simp_isnum(ctx, args[0]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (nargs == 2) {
		fill = args[1];
	} else {
		fill = simp_nil();
	}
	size = simp_getnum(ctx, args[0]);
	if (size < 0)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	return simp_makevector(ctx, size);
}

static Simp
f_multiply(Simp ctx, Simp expr, Simp env)
{
	SimpSiz i, noperands;
	SimpInt num = 1;
	Simp obj;

	noperands = simp_getsize(ctx, expr);
	for (i = ARGS_POS; i < noperands; i++) {
		obj = simp_getvectormemb(ctx, expr, i);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		num *= simp_getnum(ctx, obj);
	}
	return simp_makenum(ctx, num);
}

static Simp
f_newline(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	if (nargs > 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (nargs == 1)
		return simp_printf(ctx, args[0], "\n");
	return simp_printf(ctx, simp_contextoport(ctx), "\n");
}

static Simp
f_nullp(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	return typepred(ctx, args, nargs, simp_isnil);
}

static Simp
f_portp(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	return typepred(ctx, args, nargs, simp_isport);
}

static Simp
f_samep(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	if (nargs != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_issame(ctx, args[0], args[1]) ? simp_true() : simp_false();
}

static Simp
f_subtract(Simp ctx, Simp expr, Simp env)
{
	SimpSiz i, noperands;
	SimpInt num = 0;
	Simp obj;
	bool gotop = false;

	if ((noperands = simp_getsize(ctx, expr)) < 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = ARGS_POS; i < noperands; i++) {
		obj = simp_getvectormemb(ctx, expr, i);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		if (gotop)
			num -= simp_getnum(ctx, obj);
		else
			num = simp_getnum(ctx, obj);
		gotop = true;
	}
	if (noperands == 1)
		return simp_makenum(ctx, -num);
	return simp_makenum(ctx, num);
}

static Simp
f_stringcmp(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	SimpSiz size0, size1;
	int cmp;

	(void)env;
	if (nargs != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (!simp_isstring(ctx, args[0]) || !simp_isstring(ctx, args[1]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size0 = simp_getsize(ctx, args[0]);
	size1 = simp_getsize(ctx, args[1]);
	cmp = memcmp(
		simp_getstring(ctx, args[0]),
		simp_getstring(ctx, args[1]),
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
f_stringlen(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	SimpInt size;

	(void)env;
	if (nargs != 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (!simp_isstring(ctx, args[0]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, args[0]);
	return simp_makenum(ctx, size);
}

static Simp
f_stringref(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	SimpSiz size;
	SimpInt pos;
	unsigned char byte;

	(void)env;
	if (nargs != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (!simp_isstring(ctx, args[0]) || !simp_isnum(ctx, args[1]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, args[0]);
	pos = simp_getnum(ctx, args[1]);
	if (pos < 0 || pos >= (SimpInt)size)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	byte = simp_getstring(ctx, args[0])[pos];
	return simp_makebyte(ctx, byte);
}

static Simp
f_stringp(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	return typepred(ctx, args, nargs, simp_isstring);
}

static Simp
f_stringvector(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	SimpSiz i, size;
	Simp vector, byte;
	unsigned char *string;

	(void)env;
	if (nargs != 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (!simp_isstring(ctx, args[0]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, args[0]);
	vector = simp_makevector(ctx, size);
	if (simp_isexception(ctx, vector))
		return vector;
	string = simp_getstring(ctx, args[0]);
	for (i = 0; i < size; i++) {
		byte = simp_makebyte(ctx, string[i]);
		if (simp_isexception(ctx, byte))
			return byte;
		simp_getvector(ctx, vector)[i] = byte;
	}
	return vector;
}

Simp
f_stringset(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	Simp val;

	(void)env;
	if (nargs != 3)
		return simp_makeexception(ctx, ERROR_ARGS);
	val = simp_setstring(ctx, args[0], args[1], args[2]);
	if (simp_isexception(ctx, val))
		return val;
	return simp_void();
}

static Simp
f_symbolp(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	return typepred(ctx, args, nargs, simp_issymbol);
}

Simp
f_true(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env, (void)args, (void)nargs;
	if (nargs != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_true();
}

static Simp
f_truep(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	return typepred(ctx, args, nargs, simp_istrue);
}

Simp
f_vectorset(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	Simp val;

	(void)env;
	if (nargs != 3)
		return simp_makeexception(ctx, ERROR_ARGS);
	val = simp_setvector(ctx, args[0], args[1], args[2]);
	if (simp_isexception(ctx, val))
		return val;
	return simp_void();
}

static Simp
f_vectorlen(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	SimpInt size;

	(void)env;
	if (nargs != 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (!simp_isvector(ctx, args[0]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, args[0]);
	return simp_makenum(ctx, size);
}

static Simp
f_vectorref(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	SimpSiz size;
	SimpInt pos;

	(void)env;
	if (nargs != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (!simp_isvector(ctx, args[0]) || !simp_isnum(ctx, args[1]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	size = simp_getsize(ctx, args[0]);
	pos = simp_getnum(ctx, args[1]);
	if (pos < 0 || pos >= (SimpInt)size)
		return simp_makeexception(ctx, ERROR_OUTOFRANGE);
	return simp_getvector(ctx, args[0])[pos];
}

Simp
f_write(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	if (nargs > 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (nargs == 2)
		return simp_display(ctx, args[1], args[0]);
	return simp_write(ctx, simp_contextoport(ctx), args[0]);
}

static Simp
f_vector(Simp ctx, Simp expr, Simp env)
{
	Simp vector, obj;
	SimpSiz i, nobjs;

	(void)env;
	if ((nobjs = simp_getsize(ctx, expr)) < 2)
		return simp_makeexception(ctx, -1);
	nobjs -= 2;
	vector = simp_makevector(ctx, nobjs);
	if (simp_isexception(ctx, vector))
		return vector;
	for (i = 0; i < nobjs; i++) {
		obj = simp_getvectormemb(ctx, expr, i + ARGS_POS);
		obj = simp_eval(ctx, obj, env);
		if (simp_isexception(ctx, obj))
			return obj;
		simp_getvector(ctx, vector)[i] = obj;
	}
	return vector;
}

static Simp
lambda(Simp ctx, Simp expr, Simp env)
{
	if (simp_getsize(ctx, expr) != 3)   /* (lambda (args) body) */
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_makeclosure(
		ctx, env,
		simp_getvectormemb(ctx, expr, 1),
		simp_getvectormemb(ctx, expr, 2)
	);
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

static Simp
varargs(Simp ctx, Simp expr, Simp env, enum Varargs varg)
{
	Simp (*fun[NVARARGS])(Simp, Simp, Simp) = {
#define X(n, s, p) [n] = p,
		VARARGS
#undef  X
	};

	return (*fun[varg])(ctx, expr, env);
}

static Simp
builtin(Simp ctx, Simp expr, Simp env, enum Builtins bltin, SimpSiz nargs)
{
	Simp args[MAXARGS];
	SimpSiz i;
	static struct {
		int min, max;
		Simp (*fun)(Simp, Simp, Simp[], SimpSiz);
	} builtins[NBUILTINS] = {
#define X(n, s, p, i, j) [n] = { .fun = p, .min = i, .max = j },
			BUILTINS
#undef  X
	};

	if (nargs < (SimpSiz)builtins[bltin].min)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (nargs > (SimpSiz)builtins[bltin].max)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = 0; i < nargs; i++) {
		args[i] = simp_getvectormemb(ctx, expr, i + ARGS_POS);
		args[i] = simp_eval(ctx, args[i], env);
		if (simp_isexception(ctx, args[i])) {
			return args[i];
		}
	}
	return (*builtins[bltin].fun)(ctx, env, args, nargs);
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
		simp_getvector(ctx, forms)[i] = sym;
	}
	return forms;
}

Simp
simp_eval(Simp ctx, Simp expr, Simp env)
{
	Simp *forms;
	Simp form, operator, body, params, cloenv, var, val;
	SimpSiz nobjects, nparams, noprnds, i;

	forms = simp_getvector(ctx, simp_contextforms(ctx));
loop:
	if (simp_issymbol(ctx, expr))   /* expression is variable */
		return simp_envget(ctx, env, expr);
	if (!simp_isvector(ctx, expr))  /* expression is self-evaluating object */
		return expr;
	if ((nobjects = simp_getsize(ctx, expr)) == 0)
		return simp_makeexception(ctx, ERROR_EMPTY);
	form = simp_getvectormemb(ctx, expr, 0);
	if (!simp_issymbol(ctx, form))
		return simp_makeexception(ctx, ERROR_ILLSYNTAX);
	if (simp_issame(ctx, form, forms[FORM_APPLY])) {
		if (nobjects < 2)                       /* (apply OP ...) */
			return simp_makeexception(ctx, ERROR_ILLFORM);
		operator = simp_getvectormemb(ctx, expr, 1);
		operator = simp_eval(ctx, operator, env);
		if (simp_isvarargs(ctx, operator)) {
			return varargs(
				ctx,
				expr,
				env,
				simp_getvarargs(ctx, operator)
			);
		}
		if (simp_isbuiltin(ctx, operator)) {
			return builtin(
				ctx,
				expr,
				env,
				simp_getbuiltin(ctx, operator),
				nobjects - 1
			);
		}
		if (!simp_isclosure(ctx, operator))
			return simp_makeexception(ctx, ERROR_OPERATOR);
		body = simp_getclosurebody(ctx, operator);
		params = simp_getclosureparam(ctx, operator);
		cloenv = simp_getclosureenv(ctx, operator);
		noprnds = nobjects - 1;
		nparams = simp_getsize(ctx, params);
		if (noprnds != nparams)
			return simp_makeexception(ctx, ERROR_ARGS);
		cloenv = simp_makeenvironment(ctx, cloenv);
		if (simp_isexception(ctx, cloenv))
			return cloenv;
		for (i = 0; i < nparams; i++) {
			var = simp_getvectormemb(ctx, params, i);
			if (!simp_issymbol(ctx, var))
				return simp_makeexception(ctx, ERROR_ILLEXPR);
			val = simp_getvectormemb(ctx, expr, i + ARGS_POS);
			val = simp_eval(ctx, val, env);
			if (simp_isexception(ctx, val))
				return val;
			simp_envdef(ctx, cloenv, var, val);
		}
		expr = body;
		env = cloenv;
		goto loop;
	} else if (simp_issame(ctx, form, forms[FORM_DEFINE])) {
		return define(ctx, expr, env);
	} else if (simp_issame(ctx, form, forms[FORM_IF])) {
		if (nobjects > 4 || nobjects < 3)       /* (if COND THEN ELSE) */
			return simp_makeexception(ctx, ERROR_ILLFORM);
		val = simp_getvectormemb(ctx, expr, 1);
		val = simp_eval(ctx, val, env);
		if (simp_istrue(ctx, val)) {
			expr = simp_getvectormemb(ctx, expr, 2);
		} else if (nobjects == 4) {
			expr = simp_getvectormemb(ctx, expr, 3);
		} else {
			return simp_void();
		}
		goto loop;
	} else if (simp_issame(ctx, form, forms[FORM_EVAL])) {
		if (nobjects == 2) {                    /* (eval EXPR) */
			expr = simp_getvectormemb(ctx, expr, 1);
		} else if (nobjects == 3) {             /* (eval EXPR ENV) */
			env = simp_getvectormemb(ctx, expr, 2);
			expr = simp_getvectormemb(ctx, expr, 1);
		} else {
			return simp_makeexception(ctx, ERROR_ILLFORM);
		}
		goto loop;
	} else if (simp_issame(ctx, form, forms[FORM_LAMBDA])) {
		return lambda(ctx, expr, env);
	} else if (simp_issame(ctx, form, forms[FORM_QUOTE])) {
		if (nobjects != 2)                      /* (quote OBJ) */
			return simp_makeexception(ctx, ERROR_ILLFORM);
		return simp_getvectormemb(ctx, expr, 1);
	} else if (simp_issame(ctx, form, forms[FORM_SET])) {
		return set(ctx, expr, env);
	}
	return simp_makeexception(ctx, ERROR_UNKSYNTAX);
}
