#include <string.h>

#include "simp.h"

static SimpInt
noperands(Simp ctx, Simp list)
{
	SimpInt n = 0;
	Simp obj;

	/* returns -1 if is not proper list */
	for (obj = list; !simp_isnil(ctx, obj); obj = simp_cdr(ctx, obj)) {
		if (!simp_ispair(ctx, obj))
			return -1;
		n++;
	}
	return n;
}

static Simp
apply(Simp ctx, Simp lambda, Simp args)
{
	Simp env, param, body, var, val, expr;

	env = simp_getapplicativeenv(ctx, lambda);
	if (!simp_isenvironment(ctx, env))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	env = simp_makeenvironment(ctx, env);
	if (simp_isexception(ctx, env))
		return env;
	param = simp_getapplicativeparam(ctx, lambda);
	while (!simp_isnil(ctx, param)) {
		if (!simp_ispair(ctx, param))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		if (simp_isnil(ctx, args))
			return simp_makeexception(ctx, ERROR_ARGS);
		if (!simp_ispair(ctx, args))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		var = simp_car(ctx, param);
		val = simp_car(ctx, args);
		if (!simp_issymbol(ctx, var))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		simp_envset(ctx, env, var, val);
		param = simp_cdr(ctx, param);
		args = simp_cdr(ctx, args);
	}
	if (!simp_isnil(ctx, args))
		return simp_makeexception(ctx, ERROR_ARGS);
	expr = simp_nil();
	for (body = simp_getapplicativebody(ctx, lambda);
	     !simp_isnil(ctx, body); body = simp_cdr(ctx, body)) {
		if (!simp_ispair(ctx, body))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		expr = simp_car(ctx, body);
		val = simp_eval(ctx, expr, env);
		if (simp_isexception(ctx, val))
			return val;
	}
	return val;
}

Simp
simp_opadd(Simp ctx, Simp operands, Simp env)
{
	SimpInt num = 0;
	Simp sum, arg, obj;

	if (noperands(ctx, operands) < 0)
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	for (arg = operands; !simp_isnil(ctx, arg); arg = simp_cdr(ctx, arg)) {
		obj = simp_car(ctx, arg);
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
simp_opdivide(Simp ctx, Simp operands, Simp env)
{
	SimpInt num = 1;
	SimpInt nops = 0;
	Simp rat, arg, obj;
	int gotop = FALSE;

	if ((nops = noperands(ctx, operands)) < 1)
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	for (arg = operands; !simp_isnil(ctx, arg); arg = simp_cdr(ctx, arg)) {
		obj = simp_car(ctx, arg);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		if (gotop || nops == 1)
			num /= simp_getnum(ctx, obj);
		else
			num = simp_getnum(ctx, obj);
		gotop = TRUE;
	}
	rat = simp_makenum(ctx, num);
	return rat;
}

Simp
simp_oplambda(Simp ctx, Simp operands, Simp env)
{
	Simp body, parameters;

	if (!simp_ispair(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	parameters = simp_car(ctx, operands);
	body = simp_cdr(ctx, operands);
	return simp_makeapplicative(ctx, env, parameters, body);
}

Simp
simp_oplet(Simp ctx, Simp operands, Simp env)
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
	return value;
error:
	return simp_makeexception(ctx, ERROR_ILLEXPR);
}

Simp
simp_opmultiply(Simp ctx, Simp operands, Simp env)
{
	SimpInt num = 1;
	Simp prod, arg, obj;

	if (noperands(ctx, operands) < 0)
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	for (arg = operands; !simp_isnil(ctx, arg); arg = simp_cdr(ctx, arg)) {
		obj = simp_car(ctx, arg);
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
simp_opquote(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (!simp_ispair(ctx, operands))
		goto error;
	if (!simp_isnil(ctx, simp_cdr(ctx, operands)))
		goto error;
	return simp_car(ctx, operands);
error:
	return simp_makeexception(ctx, ERROR_ILLEXPR);
}

Simp
simp_opsubtract(Simp ctx, Simp operands, Simp env)
{
	SimpInt num = 0;
	SimpInt nops = 0;
	Simp diff, arg, obj;
	int gotop = FALSE;

	if ((nops = noperands(ctx, operands)) < 1)
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	for (arg = operands; !simp_isnil(ctx, arg); arg = simp_cdr(ctx, arg)) {
		obj = simp_car(ctx, arg);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		if (gotop || nops == 1)
			num -= simp_getnum(ctx, obj);
		else
			num = simp_getnum(ctx, obj);
		gotop = TRUE;
	}
	diff = simp_makenum(ctx, num);
	return diff;
}

Simp
simp_operate(Simp ctx, Simp expr, Simp env)
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
	if (simp_isapplicative(ctx, operator))
		return apply(ctx, operator, operands);
	return simp_makeexception(ctx, -1);
}

Simp
simp_eval(Simp ctx, Simp expr, Simp env)
{
	if (simp_issymbol(ctx, expr))           /* expression is a variable */
		return simp_envget(ctx, env, expr);
	if (!simp_isvector(ctx, expr))          /* expression is a self-evaluating object */
		return expr;
	return simp_operate(ctx, expr, env);    /* expression is an operation */
}
