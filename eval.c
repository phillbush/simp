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
operate(Simp ctx, Simp macro, Simp args, Simp env)
{
	Simp cloenv, param, body, var, val, expr;

	cloenv = simp_getoperativeenv(ctx, macro);
	if (!simp_isenvironment(ctx, cloenv))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	cloenv = simp_makeenvironment(ctx, cloenv);
	if (simp_isexception(ctx, cloenv))
		return cloenv;
	param = simp_getoperativeparam(ctx, macro);
	if (!simp_ispair(ctx, param))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	var = simp_car(ctx, param);
	if (!simp_issymbol(ctx, var))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	simp_envset(ctx, cloenv, var, env);
	param = simp_cdr(ctx, param);
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
		simp_envset(ctx, cloenv, var, val);
		param = simp_cdr(ctx, param);
		args = simp_cdr(ctx, args);
	}
	if (!simp_isnil(ctx, args))
		return simp_makeexception(ctx, ERROR_ARGS);
	expr = simp_nil();
	for (body = simp_getoperativebody(ctx, macro);
	     !simp_isnil(ctx, body); body = simp_cdr(ctx, body)) {
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
apply(Simp ctx, Simp lambda, Simp args, Simp env)
{
	Simp cloenv, param, body, var, val, expr;

	cloenv = simp_getapplicativeenv(ctx, lambda);
	if (!simp_isenvironment(ctx, cloenv))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	cloenv = simp_makeenvironment(ctx, cloenv);
	if (simp_isexception(ctx, cloenv))
		return cloenv;
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
		val = simp_eval(ctx, val, env);
		if (simp_isexception(ctx, val))
			return val;
		if (!simp_issymbol(ctx, var))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		simp_envset(ctx, cloenv, var, val);
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
	if (simp_isapplicative(ctx, operator))
		return apply(ctx, operator, operands, env);
	if (simp_isoperative(ctx, operator))
		return operate(ctx, operator, operands, env);
	return simp_makeexception(ctx, -1);
}

static Simp
getargs(Simp ctx, Simp operands, Simp env, Simp args[], SimpInt nargs)
{
	SimpInt i;

	for (i = 0; i < nargs; i++) {
		if (!simp_ispair(ctx, operands))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		args[i] = simp_car(ctx, operands);
		operands = simp_cdr(ctx, operands);
	}
	if (!simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	for (i = 0; i < nargs; i++) {
		args[i] = simp_eval(ctx, args[i], env);
		if (simp_isexception(ctx, args[i])) {
			return args[i];
		}
	}
	return simp_nil();
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
simp_opbooleanp(Simp ctx, Simp operands, Simp env)
{
	Simp arg, ret;

	ret = getargs(ctx, operands, env, &arg, 1);
	if (simp_isexception(ctx, ret))
		return ret;
	if (simp_isbool(ctx, arg))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_opcuriport(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (!simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	return simp_contextiport(ctx);
}

Simp
simp_opcuroport(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (!simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	return simp_contextoport(ctx);
}

Simp
simp_opcureport(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (!simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
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
simp_opequal(Simp ctx, Simp operands, Simp env)
{
	Simp args[2];
	Simp ret;

	ret = getargs(ctx, operands, env, args, 2);
	if (simp_isexception(ctx, ret))
		return ret;
	if (!simp_isnum(ctx, args[0]) || !simp_isnum(ctx, args[1]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (simp_getnum(ctx, args[0]) == simp_getnum(ctx, args[1]))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_opfalse(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (!simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	return simp_false();
}

Simp
simp_opgt(Simp ctx, Simp operands, Simp env)
{
	Simp args[2];
	Simp ret;

	ret = getargs(ctx, operands, env, args, 2);
	if (simp_isexception(ctx, ret))
		return ret;
	if (!simp_isnum(ctx, args[0]) || !simp_isnum(ctx, args[1]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (simp_getnum(ctx, args[0]) > simp_getnum(ctx, args[1]))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_opif(Simp ctx, Simp operands, Simp env)
{
	Simp test, exprs[2] = { simp_nil(), simp_nil() };
	int i = 0;

	if (!simp_ispair(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	test = simp_car(ctx, operands);
	operands = simp_cdr(ctx, operands);
	if (!simp_ispair(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	exprs[0] = simp_car(ctx, operands);
	operands = simp_cdr(ctx, operands);
	if (simp_ispair(ctx, operands)) {
		exprs[1] = simp_car(ctx, operands);
		operands = simp_cdr(ctx, operands);
	}
	if (!simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	test = simp_eval(ctx, test, env);
	if (simp_isexception(ctx, test))
		return test;
	if (simp_istrue(ctx, test))
		i = 0;
	else
		i = 1;
	return simp_eval(ctx, exprs[i], env);
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
simp_oplt(Simp ctx, Simp operands, Simp env)
{
	Simp args[2];
	Simp ret;

	ret = getargs(ctx, operands, env, args, 2);
	if (simp_isexception(ctx, ret))
		return ret;
	if (!simp_isnum(ctx, args[0]) || !simp_isnum(ctx, args[1]))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	if (simp_getnum(ctx, args[0]) < simp_getnum(ctx, args[1]))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_opmacro(Simp ctx, Simp operands, Simp env)
{
	Simp body, parameters;

	if (!simp_ispair(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	parameters = simp_car(ctx, operands);
	body = simp_cdr(ctx, operands);
	return simp_makeoperative(ctx, env, parameters, body);
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
simp_opnewline(Simp ctx, Simp operands, Simp env)
{
	Simp port;

	(void)env;
	if (simp_isnil(ctx, operands)) {
		port = simp_contextoport(ctx);
	} else if (simp_ispair(ctx, operands) &&
	           simp_isnil(ctx, simp_cdr(ctx, operands))) {
		port = simp_car(ctx, operands);
	} else {
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	}
	port = simp_eval(ctx, port, env);
	if (simp_isexception(ctx, port))
		return port;
	if (!simp_isport(ctx, port))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	simp_printf(ctx, port, "\n");
	return simp_void();
}

Simp
simp_opnullp(Simp ctx, Simp operands, Simp env)
{
	Simp arg, ret;

	ret = getargs(ctx, operands, env, &arg, 1);
	if (simp_isexception(ctx, ret))
		return ret;
	if (simp_isnil(ctx, arg))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_oppairp(Simp ctx, Simp operands, Simp env)
{
	Simp arg, ret;

	ret = getargs(ctx, operands, env, &arg, 1);
	if (simp_isexception(ctx, ret))
		return ret;
	if (simp_ispair(ctx, arg))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_opportp(Simp ctx, Simp operands, Simp env)
{
	Simp arg, ret;

	ret = getargs(ctx, operands, env, &arg, 1);
	if (simp_isexception(ctx, ret))
		return ret;
	if (simp_isport(ctx, arg))
		return simp_true();
	else
		return simp_false();
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
simp_opsamep(Simp ctx, Simp operands, Simp env)
{
	Simp args[2];
	Simp ret;

	ret = getargs(ctx, operands, env, args, 2);
	if (simp_isexception(ctx, ret))
		return ret;
	if (simp_issame(ctx, args[0], args[1]))
		return simp_true();
	else
		return simp_false();
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
simp_opsymbolp(Simp ctx, Simp operands, Simp env)
{
	Simp arg, ret;

	ret = getargs(ctx, operands, env, &arg, 1);
	if (simp_isexception(ctx, ret))
		return ret;
	if (simp_issymbol(ctx, arg))
		return simp_true();
	else
		return simp_false();
}

Simp
simp_optrue(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (!simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	return simp_true();
}

Simp
simp_opvoid(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (!simp_isnil(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	return simp_void();
}

Simp
simp_opwrite(Simp ctx, Simp operands, Simp env)
{
	Simp obj, port;

	(void)env;
	if (!simp_ispair(ctx, operands))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	obj = simp_car(ctx, operands);
	operands = simp_cdr(ctx, operands);
	if (simp_isnil(ctx, operands)) {
		port = simp_contextoport(ctx);
	} else if (simp_ispair(ctx, operands) &&
	           simp_isnil(ctx, simp_cdr(ctx, operands))) {
		port = simp_car(ctx, operands);
	} else {
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	}
	obj = simp_eval(ctx, obj, env);
	port = simp_eval(ctx, port, env);
	if (simp_isexception(ctx, obj))
		return obj;
	if (simp_isexception(ctx, port))
		return port;
	if (!simp_isport(ctx, port))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	simp_write(ctx, port, obj);
	return simp_void();
}

Simp
simp_eval(Simp ctx, Simp expr, Simp env)
{
	if (simp_issymbol(ctx, expr))           /* expression is a variable */
		return simp_envget(ctx, env, expr);
	if (!simp_isvector(ctx, expr))          /* expression is a self-evaluating object */
		return expr;
	return combine(ctx, expr, env);    /* expression is an operation */
}
