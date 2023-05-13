#include <string.h>

#include "simp.h"

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
