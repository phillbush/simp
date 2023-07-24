#include <string.h>

#include "simp.h"

#define MAXARGS 4

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
	for (i = 1; i < noperands; i++) {
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
	for (i = 1; i < noperands; i++) {
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
f_multiply(Simp ctx, Simp expr, Simp env)
{
	SimpSiz i, noperands;
	SimpInt num = 1;
	Simp obj;

	noperands = simp_getsize(ctx, expr);
	for (i = 1; i < noperands; i++) {
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
	for (i = 1; i < noperands; i++) {
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
f_stringp(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env;
	return typepred(ctx, args, nargs, simp_isstring);
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

Simp
f_void(Simp ctx, Simp env, Simp args[], SimpSiz nargs)
{
	(void)env, (void)args, (void)nargs;
	if (nargs != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_void();
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
	if ((nobjs = simp_getsize(ctx, expr)) < 1)
		return simp_makeexception(ctx, -1);
	nobjs--;
	vector = simp_makevector(ctx, nobjs, simp_nil());
	if (simp_isexception(ctx, vector))
		return vector;
	for (i = 0; i < nobjs; i++) {
		obj = simp_getvectormemb(ctx, expr, i + 1);
		obj = simp_eval(ctx, obj, env);
		if (simp_isexception(ctx, obj))
			return obj;
		simp_getvector(ctx, vector)[i] = obj;
	}
	return vector;
}

static Simp
macro(Simp ctx, Simp expr, Simp env)
{
	if (simp_getsize(ctx, expr) != 3)   /* (macro (args) body) */
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_makeoperative(
		ctx, env,
		simp_getvectormemb(ctx, expr, 1),
		simp_getvectormemb(ctx, expr, 2)
	);
}

static Simp
lambda(Simp ctx, Simp expr, Simp env)
{
	if (simp_getsize(ctx, expr) != 3)   /* (lambda (args) body) */
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_makeapplicative(
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
wrap(Simp ctx, Simp expr, Simp env)
{
	(void)env;
	if (simp_getsize(ctx, expr) != 2)       /* (wrap COMBINER) */
		return simp_makeexception(ctx, ERROR_ARGS);
	if (!simp_isoperative(ctx, expr))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_wrap(ctx, simp_getvectormemb(ctx, expr, 1));
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

	if (builtins[bltin].min < 0) {
		switch (bltin) {
		case F_ADD:
			return f_add(ctx, expr, env);
		case F_SUBTRACT:
			return f_subtract(ctx, expr, env);
		case F_MULTIPLY:
			return f_multiply(ctx, expr, env);
		case F_DIVIDE:
			return f_divide(ctx, expr, env);
		default:
			return simp_makeexception(ctx, -1);
		}
	}
	if (nargs < (SimpSiz)builtins[bltin].min)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (nargs > (SimpSiz)builtins[bltin].max)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = 0; i < nargs; i++) {
		args[i] = simp_getvectormemb(ctx, expr, i + 1);
		args[i] = simp_eval(ctx, args[i], env);
		if (simp_isexception(ctx, args[i])) {
			return args[i];
		}
	}
	return (*builtins[bltin].fun)(ctx, env, args, nargs);
}

Simp
simp_eval(Simp ctx, Simp expr, Simp env)
{
	Simp operator, body, params, cloenv, var, val;
	SimpSiz nobjects, nparams, noprnds, i;
	bool eval;

loop:
	if (simp_issymbol(ctx, expr))   /* expression is variable */
		return simp_envget(ctx, env, expr);
	if (!simp_isvector(ctx, expr))  /* expression is self-evaluating object */
		return expr;
	if ((nobjects = simp_getsize(ctx, expr)) == 0)
		return simp_makeexception(ctx, ERROR_EMPTY);
	operator = simp_getvectormemb(ctx, expr, 0);
	operator = simp_eval(ctx, operator, env);
	if (simp_isexception(ctx, operator))
		return operator;
	if (simp_isform(ctx, operator)) switch (simp_getform(ctx, operator)) {
	case OP_DEFINE:
		return define(ctx, expr, env);
	case OP_IF:
		if (nobjects > 4 || nobjects < 3)       /* (if COND THEN ELSE) */
			return simp_makeexception(ctx, ERROR_ARGS);
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
	case OP_EVAL:
		if (nobjects == 2) {                    /* (eval EXPR) */
			expr = simp_getvectormemb(ctx, expr, 1);
		} else if (nobjects == 3) {             /* (eval EXPR ENV) */
			env = simp_getvectormemb(ctx, expr, 2);
			expr = simp_getvectormemb(ctx, expr, 1);
		} else {
			return simp_makeexception(ctx, ERROR_ARGS);
		}
		goto loop;
	case OP_MACRO:
		return macro(ctx, expr, env);
	case OP_LAMBDA:
		return lambda(ctx, expr, env);
	case OP_SET:
		return set(ctx, expr, env);
	case OP_WRAP:
		return wrap(ctx, expr, env);
	default:
		return simp_makeexception(ctx, -1);
	}
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
	if (simp_isapplicative(ctx, operator)) {
		eval = true;
		body = simp_getapplicativebody(ctx, operator);
		params = simp_getapplicativeparam(ctx, operator);
		cloenv = simp_getapplicativeenv(ctx, operator);
		noprnds = nobjects - 1;
		nparams = simp_getsize(ctx, params);
		if (noprnds != nparams) {
			return simp_makeexception(ctx, ERROR_ARGS);
		}
	} else if (simp_isoperative(ctx, operator)) {
		eval = false;
		body = simp_getoperativebody(ctx, operator);
		params = simp_getoperativeparam(ctx, operator);
		cloenv = simp_getoperativeenv(ctx, operator);
		noprnds = nobjects - 1;
		nparams = simp_getsize(ctx, params);
		if (noprnds + 1 != nparams) {
			return simp_makeexception(ctx, ERROR_ARGS);
		}
	} else {
		return simp_makeexception(ctx, ERROR_OPERATOR);
	}
	cloenv = simp_makeenvironment(ctx, cloenv);
	if (simp_isexception(ctx, cloenv))
		return cloenv;
	for (i = 0; i < nparams; i++) {
		var = simp_getvectormemb(ctx, params, i);
		if (!simp_issymbol(ctx, var))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		if (eval) {
			val = simp_getvectormemb(ctx, expr, i + 1);
			val = simp_eval(ctx, val, env);
		} else if (i == 0) {
			val = env;
		} else {
			val = simp_getvectormemb(ctx, expr, i);
		}
		if (simp_isexception(ctx, val))
			return val;
		simp_envdef(ctx, cloenv, var, val);
	}
	expr = body;
	env = cloenv;
	goto loop;
}
