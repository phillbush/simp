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

	operator = simp_eval(ctx, simp_car(ctx, expr), env);
	if (simp_isexception(ctx, operator))
		return operator;
	operands = simp_cdr(ctx, expr);
	if (simp_isbuiltin(ctx, operator))
		return (*simp_getbuiltin(ctx, operator))(ctx, operands, env);
	return simp_makeexception(ctx, -1);
}

Simp
simp_eval(Simp ctx, Simp expr, Simp env)
{
	if (simp_issymbol(ctx, expr))           /* expression is a variable */
		return simp_envget(ctx, env, expr);
	if (!simp_isvector(ctx, expr))          /* expression is a self-evaluating object */
		return expr;
	if (!simp_ispair(ctx, expr))            /* expression is ill-formed */
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	return simp_operate(ctx, expr, env);    /* expressioin is an operation */
}
