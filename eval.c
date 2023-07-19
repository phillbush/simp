#include <string.h>

#include "simp.h"

typedef struct Argument {
	Simp    argument;
	bool    evaluate;
} Argument;

static Simp
evalargs(Simp ctx, Simp list, Simp env)
{
	Simp val, pair, prev, args;

	args = simp_nil();
	prev = simp_nil();
	for (; !simp_isnil(ctx, list); list = simp_cdr(ctx, list)) {
		if (!simp_ispair(ctx, list))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		val = simp_car(ctx, list);
		val = simp_eval(ctx, val, env);
		if (simp_isexception(ctx, val))
			return val;
		pair = simp_cons(ctx, val, simp_nil());
		if (simp_isexception(ctx, pair))
			return pair;
		if (!simp_isnil(pair, prev))
			simp_setcdr(ctx, prev, pair);
		else
			args = pair;
		prev = pair;
	}
	return args;
}

static Simp
operate(Simp ctx, Simp operator, Simp args, Simp env)
{
	Simp var, val, expr;
	Simp body, cloenv, param;

	if (simp_isapplicative(ctx, operator)) {
		args = evalargs(ctx, args, env);
		if (simp_isexception(ctx, args))
			return args;
		body = simp_getapplicativebody(ctx, operator);
		param = simp_getapplicativeparam(ctx, operator);
		cloenv = simp_getapplicativeenv(ctx, operator);
	} else if (simp_isoperative(ctx, operator)) {
		body = simp_getoperativebody(ctx, operator);
		param = simp_getoperativeparam(ctx, operator);
		cloenv = simp_getoperativeenv(ctx, operator);
	} else {
		return simp_makeexception(ctx, -1);
	}
	if (!simp_isenvironment(ctx, cloenv))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	cloenv = simp_makeenvironment(ctx, cloenv);
	if (simp_isexception(ctx, cloenv))
		return cloenv;
	if (simp_isoperative(ctx, operator)) {
		if (!simp_ispair(ctx, param))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		var = simp_car(ctx, param);
		if (!simp_issymbol(ctx, var))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		simp_envset(ctx, cloenv, var, env);
		param = simp_cdr(ctx, param);
	}
	while (!simp_isnil(ctx, param)) {
		if (simp_isnil(ctx, args))
			return simp_makeexception(ctx, ERROR_ARGS);
		if (!simp_ispair(ctx, args))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		var = simp_getvectormemb(ctx, param, 0);
		if (!simp_issymbol(ctx, var))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		if (simp_getsize(ctx, param) == 1)
			val = args;
		else
			val = simp_car(ctx, args);
		simp_envset(ctx, cloenv, var, val);
		if (simp_getsize(ctx, param) == 1)
			goto done;
		param = simp_cdr(ctx, param);
		args = simp_cdr(ctx, args);
	}
	if (!simp_isnil(ctx, args))
		return simp_makeexception(ctx, ERROR_ARGS);
done:
	expr = simp_nil();
	for (; !simp_isnil(ctx, body); body = simp_cdr(ctx, body)) {
		if (!simp_ispair(ctx, body))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		expr = simp_car(ctx, body);
		val = simp_eval(ctx, expr, cloenv);
		if (simp_isexception(ctx, val))
			return val;
	}
	return val;
}

static Simp
combine(Simp ctx, Simp expr, Simp env)
{
	Simp operator, operands;

	if (!simp_ispair(ctx, expr))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	operator = simp_eval(ctx, simp_car(ctx, expr), env);
	if (simp_isexception(ctx, operator))
		return operator;
	operands = simp_cdr(ctx, expr);
	if (simp_isbuiltin(ctx, operator))
		return (*simp_getbuiltin(ctx, operator))(ctx, operands, env);
	return operate(ctx, operator, operands, env);
}

static SimpInt
getargs(Simp ctx, Simp operands, Simp env, Argument args[], SimpInt min, SimpInt max, Simp *ret)
{
	SimpInt i, nargs;

	/*
	 * Return number of read arguments (between min and max,
	 * inclusive); or -1 if could not get the arguments or
	 * evaluate them.
	 *
	 * Fills args[] with the (evaluated) arguments.
	 */
	*ret = simp_makeexception(ctx, ERROR_ILLEXPR);
	for (i = 0; i < max; i++) {
		if (simp_isnil(ctx, operands))
			break;
		if (!simp_ispair(ctx, operands))
			return -1;
		args[i].argument = simp_car(ctx, operands);
		operands = simp_cdr(ctx, operands);
	}
	nargs = i;
	if (nargs < min || !simp_isnil(ctx, operands))
		return -1;
	for (i = 0; i < nargs; i++) {
		if (!args[i].evaluate)
			continue;
		args[i].argument = simp_eval(ctx, args[i].argument, env);
		if (simp_isexception(ctx, args[i].argument)) {
			*ret = args[i].argument;
			return -1;
		}
	}
	return nargs;
}

#define GETARGS(c, o, e, a, m, n)                           \
	do {                                                \
		Simp r;                                     \
		if(getargs((c),(o),(e),(a),(m),(n),&r) < 0) \
			return r;                           \
	} while(0)

Simp
simp_opadd(Simp ctx, Simp operands, Simp env)
{
	SimpInt num = 0;
	Simp sum, pair, obj;

	for (pair = operands; !simp_isnil(ctx, pair); pair = simp_cdr(ctx, pair)) {
		if (!simp_ispair(ctx, pair))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		obj = simp_car(ctx, pair);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		num += simp_getnum(ctx, obj);
	}
	sum = simp_makenum(ctx, num);
	return sum;
}

Simp
simp_opbooleanp(Simp ctx, Simp operands, Simp env)
{
	Argument arg = { simp_void(), true };

	GETARGS(ctx, operands, env, &arg, 1, 1);
	return simp_isbool(ctx, arg.argument) ? simp_true() : simp_false();
}

Simp
simp_opcar(Simp ctx, Simp operands, Simp env)
{
	Argument arg = { simp_void(), true };

	GETARGS(ctx, operands, env, &arg, 1, 1);
	return simp_car(ctx, arg.argument);
}

Simp
simp_opcdr(Simp ctx, Simp operands, Simp env)
{
	Argument arg = { simp_void(), true };

	GETARGS(ctx, operands, env, &arg, 1, 1);
	return simp_cdr(ctx, arg.argument);
}

Simp
simp_opcons(Simp ctx, Simp operands, Simp env)
{
	Argument args[2];

	args[0].evaluate = args[1].evaluate = true;
	GETARGS(ctx, operands, env, args, 2, 2);
	return simp_cons(ctx, args[0].argument, args[1].argument);
}

Simp
simp_opcuriport(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	GETARGS(ctx, operands, env, NULL, 0, 0);
	return simp_contextiport(ctx);
}

Simp
simp_opcuroport(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	GETARGS(ctx, operands, env, NULL, 0, 0);
	return simp_contextoport(ctx);
}

Simp
simp_opcureport(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	GETARGS(ctx, operands, env, NULL, 0, 0);
	return simp_contexteport(ctx);
}

Simp
simp_opdefine(Simp ctx, Simp operands, Simp env)
{
	Simp symbol, value;

	if (!simp_ispair(ctx, operands))
		goto error;
	symbol = simp_car(ctx, operands);
	if (!simp_issymbol(ctx, symbol))
		goto error;
	operands = simp_cdr(ctx, operands);
	if (!simp_ispair(ctx, operands))
		goto error;
	value = simp_car(ctx, operands);
	operands = simp_cdr(ctx, operands);
	if (!simp_isnil(ctx, operands))
		goto error;
	value = simp_eval(ctx, value, env);
	if (simp_isexception(ctx, value))
		return value;
	(void)simp_envset(ctx, env, symbol, value);
	return simp_void();
error:
	return simp_makeexception(ctx, ERROR_ILLEXPR);
}

Simp
simp_opdisplay(Simp ctx, Simp operands, Simp env)
{
	enum { ARG_OBJECT, ARG_PORT, NARGS };
	Argument args[NARGS] = {
		[ARG_OBJECT] = { simp_void(),            true },
		[ARG_PORT]   = { simp_contextoport(ctx), true },
	};

	GETARGS(ctx, operands, env, args, 1, 2);
	return simp_display(
		ctx,
		args[ARG_PORT].argument,
		args[ARG_OBJECT].argument
	);
}

Simp
simp_opdivide(Simp ctx, Simp operands, Simp env)
{
	SimpInt num = 1;
	SimpInt nops = 0;
	Simp pair, obj;
	bool gotop = false;

	for (pair = operands; !simp_isnil(ctx, pair); pair = simp_cdr(ctx, pair)) {
		if (!simp_ispair(ctx, pair))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		nops++;
		obj = simp_car(ctx, pair);
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
	if (nops == 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (nops == 1)
		return simp_makenum(ctx, 1 / num);
	return simp_makenum(ctx, num);
}

Simp
simp_opequal(Simp ctx, Simp operands, Simp env)
{
	Argument args[2];

	args[0].evaluate = args[1].evaluate = true;
	GETARGS(ctx, operands, env, args, 2, 2);
	if (!simp_isnum(ctx, args[0].argument) || !simp_isnum(ctx, args[1].argument))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (simp_getnum(ctx, args[0].argument) == simp_getnum(ctx, args[1].argument))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_opeval(Simp ctx, Simp operands, Simp env)
{
	Argument args[2];

	args[0].evaluate = args[1].evaluate = true;
	GETARGS(ctx, operands, env, args, 2, 2);
	if (!simp_isenvironment(ctx, args[1].argument))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	return simp_eval(ctx, args[0].argument, args[1].argument);
}

Simp
simp_opfalse(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	GETARGS(ctx, operands, env, NULL, 0, 0);
	return simp_false();
}

Simp
simp_opgt(Simp ctx, Simp operands, Simp env)
{
	Argument args[2];

	args[0].evaluate = args[1].evaluate = true;
	GETARGS(ctx, operands, env, args, 2, 2);
	if (!simp_isnum(ctx, args[0].argument) || !simp_isnum(ctx, args[1].argument))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (simp_getnum(ctx, args[0].argument) > simp_getnum(ctx, args[1].argument))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_opif(Simp ctx, Simp operands, Simp env)
{
	enum { COND, THEN, ELSE, LAST };
	Argument args[LAST];
	int n = ELSE;

	args[COND].evaluate = true;
	args[THEN].evaluate = args[ELSE].evaluate = false;
	GETARGS(ctx, operands, env, args, 2, 3);
	n = simp_istrue(ctx, args[COND].argument) ? THEN : ELSE;
	return simp_eval(ctx, args[n].argument, env);
}

Simp
simp_oplambda(Simp ctx, Simp operands, Simp env)
{
	Simp body, parameters;

	if (!simp_ispair(ctx, operands) && !simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	parameters = simp_car(ctx, operands);
	body = simp_cdr(ctx, operands);
	return simp_makeapplicative(ctx, env, parameters, body);
}

Simp
simp_oplt(Simp ctx, Simp operands, Simp env)
{
	Argument args[2];

	args[0].evaluate = args[1].evaluate = true;
	GETARGS(ctx, operands, env, args, 2, 2);
	if (!simp_isnum(ctx, args[0].argument) || !simp_isnum(ctx, args[1].argument))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (simp_getnum(ctx, args[0].argument) < simp_getnum(ctx, args[1].argument))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_opmakeenvironment(Simp ctx, Simp operands, Simp env)
{
	GETARGS(ctx, operands, env, NULL, 0, 0);
	return simp_makeenvironment(ctx, env);
}

Simp
simp_opmacro(Simp ctx, Simp operands, Simp env)
{
	Simp body, parameters;

	if (!simp_ispair(ctx, operands) && !simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	parameters = simp_car(ctx, operands);
	body = simp_cdr(ctx, operands);
	return simp_makeoperative(ctx, env, parameters, body);
}

Simp
simp_opmultiply(Simp ctx, Simp operands, Simp env)
{
	SimpInt num = 1;
	Simp prod, pair, obj;

	for (pair = operands; !simp_isnil(ctx, pair); pair = simp_cdr(ctx, pair)) {
		if (!simp_ispair(ctx, pair))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		obj = simp_car(ctx, pair);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		num *= simp_getnum(ctx, obj);
	}
	prod = simp_makenum(ctx, num);
	return prod;
}

Simp
simp_opnewline(Simp ctx, Simp operands, Simp env)
{
	Argument arg = {
		.argument = simp_contextoport(ctx),
		.evaluate = true,
	};

	GETARGS(ctx, operands, env, &arg, 0, 1);
	return simp_printf(ctx, arg.argument, "\n");
}

Simp
simp_opnullp(Simp ctx, Simp operands, Simp env)
{
	Argument arg = { simp_void(), true };

	GETARGS(ctx, operands, env, &arg, 1, 1);
	return simp_isnil(ctx, arg.argument) ? simp_true() : simp_false();
}

Simp
simp_oppairp(Simp ctx, Simp operands, Simp env)
{
	Argument arg = { simp_void(), true };

	GETARGS(ctx, operands, env, &arg, 1, 1);
	return simp_ispair(ctx, arg.argument) ? simp_true() : simp_false();
}

Simp
simp_opportp(Simp ctx, Simp operands, Simp env)
{
	Argument arg = { simp_void(), true };

	GETARGS(ctx, operands, env, &arg, 1, 1);
	return simp_isport(ctx, arg.argument) ? simp_true() : simp_false();
}

Simp
simp_opsamep(Simp ctx, Simp operands, Simp env)
{
	Argument args[2];

	args[0].evaluate = args[1].evaluate = true;
	GETARGS(ctx, operands, env, args, 2, 2);
	return simp_issame(ctx, args[0].argument, args[1].argument)
		? simp_true()
		: simp_false();
}

Simp
simp_opsubtract(Simp ctx, Simp operands, Simp env)
{
	SimpInt num = 0;
	SimpInt nops = 0;
	Simp pair, obj;
	bool gotop = false;

	for (pair = operands; !simp_isnil(ctx, pair); pair = simp_cdr(ctx, pair)) {
		if (!simp_ispair(ctx, pair))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		nops++;
		obj = simp_car(ctx, pair);
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
	if (nops == 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	if (nops == 1)
		return simp_makenum(ctx, -num);
	return simp_makenum(ctx, num);
}

Simp
simp_opsymbolp(Simp ctx, Simp operands, Simp env)
{
	Argument arg = { simp_void(), true };

	GETARGS(ctx, operands, env, &arg, 1, 1);
	return simp_issymbol(ctx, arg.argument) ? simp_true() : simp_false();
}

Simp
simp_optrue(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	GETARGS(ctx, operands, env, NULL, 0, 0);
	return simp_true();
}

Simp
simp_opvoid(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	GETARGS(ctx, operands, env, NULL, 0, 0);
	return simp_void();
}

Simp
simp_opwrite(Simp ctx, Simp operands, Simp env)
{
	enum { ARG_OBJECT, ARG_PORT, NARGS };
	Argument args[NARGS] = {
		[ARG_OBJECT] = { simp_void(),            true },
		[ARG_PORT]   = { simp_contextoport(ctx), true },
	};

	GETARGS(ctx, operands, env, args, 1, 2);
	return simp_write(
		ctx,
		args[ARG_PORT].argument,
		args[ARG_OBJECT].argument
	);
}

Simp
simp_eval(Simp ctx, Simp expr, Simp env)
{
	if (simp_issymbol(ctx, expr))   /* expression is variable */
		return simp_envget(ctx, env, expr);
	if (!simp_isvector(ctx, expr))  /* expression is self-evaluating object */
		return expr;
	return combine(ctx, expr, env); /* expression is operation */
}
