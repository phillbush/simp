#include <string.h>

#include "simp.h"

static Simp
typepred(Simp ctx, Simp operands, Simp env, bool (*pred)(Simp, Simp))
{
	Simp obj;

	if (simp_getsize(ctx, operands) != 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	obj = simp_eval(ctx, simp_getvectormemb(ctx, operands, 1), env);
	if (simp_isexception(ctx, obj))
		return obj;
	if ((*pred)(ctx, obj))
		return simp_true();
	return simp_false();
}

static Simp
operate(Simp ctx, Simp operator, Simp args, Simp env)
{
	SimpSiz i, nargs, nparams;
	Simp var, val;
	Simp body, cloenv, param;

	if (simp_isapplicative(ctx, operator)) {
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
	nargs = simp_getsize(ctx, args);
	nparams = simp_getsize(ctx, param);
	if (!simp_isenvironment(ctx, cloenv))
		return simp_makeexception(ctx, ERROR_ILLTYPE);
	cloenv = simp_makeenvironment(ctx, cloenv);
	if (simp_isexception(ctx, cloenv))
		return cloenv;
	if (simp_isoperative(ctx, operator) && nargs < 1)
		return simp_makeexception(ctx, ERROR_ENVIRON);
	for (i = 0; i < nparams; i++) {
		var = simp_getvectormemb(ctx, param, i);
		if (!simp_issymbol(ctx, var))
			return simp_makeexception(ctx, ERROR_ILLEXPR);
		// TODO: implement variable arguments
		if (simp_isapplicative(ctx, operator)) {
			val = simp_getvectormemb(ctx, args, i);
			val = simp_eval(ctx, val, env);
		} else if (i == 0) {
			val = env;
		} else {
			val = simp_getvectormemb(ctx, args, i - 1);
		}
		if (simp_isexception(ctx, val))
			return val;
		simp_envset(ctx, cloenv, var, val);
	}
	// TODO: implement multiple values
	val = simp_eval(ctx, body, cloenv);
	return val;
}

static Simp
combine(Simp ctx, Simp expr, Simp env)
{
	SimpSiz i, nobjs;
	Simp operator, operands;

	if (!simp_isvector(ctx, expr))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	nobjs = simp_getsize(ctx, expr);
	if (nobjs < 1)
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	operator = simp_getvectormemb(ctx, expr, 0);
	operator = simp_eval(ctx, operator, env);
	if (simp_isexception(ctx, operator))
		return operator;
	nobjs--;
	if (nobjs == 0)
		operands = simp_nil();
	else
		operands = simp_makevector(ctx, nobjs, simp_nil());
	for (i = 0; i < nobjs; i++) {
		simp_setvector(
			ctx,
			operands,
			i,
			simp_getvectormemb(ctx, expr, i + 1)
		);
	}
	if (simp_isbuiltin(ctx, operator))
		return (*simp_getbuiltin(ctx, operator))(ctx, operands, env);
	return operate(ctx, operator, operands, env);
}

Simp
simp_opadd(Simp ctx, Simp operands, Simp env)
{
	SimpSiz i, noperands;
	SimpInt num = 0;
	Simp obj;

	noperands = simp_getsize(ctx, operands);
	for (i = 0; i < noperands; i++) {
		obj = simp_getvectormemb(ctx, operands, i);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		num += simp_getnum(ctx, obj);
	}
	return simp_makenum(ctx, num);
}

Simp
simp_opbooleanp(Simp ctx, Simp operands, Simp env)
{
	return typepred(ctx, operands, env, simp_isbool);
}

Simp
simp_opcuriport(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (simp_getsize(ctx, operands) != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_contextiport(ctx);
}

Simp
simp_opcuroport(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (simp_getsize(ctx, operands) != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_contextoport(ctx);
}

Simp
simp_opcureport(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (simp_getsize(ctx, operands) != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_contexteport(ctx);
}

Simp
simp_opdefine(Simp ctx, Simp operands, Simp env)
{
	Simp symbol, value;

	if (simp_getsize(ctx, operands) != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	symbol = simp_getvectormemb(ctx, operands, 0);
	if (!simp_issymbol(ctx, symbol))
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	value = simp_getvectormemb(ctx, operands, 1);
	value = simp_eval(ctx, value, env);
	if (simp_isexception(ctx, value))
		return value;
	(void)simp_envset(ctx, env, symbol, value);
	return simp_void();
}

Simp
simp_opdisplay(Simp ctx, Simp operands, Simp env)
{
	Simp port, obj;
	SimpSiz noperands;

	noperands = simp_getsize(ctx, operands);
	if (noperands < 1 || noperands > 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	obj = simp_getvectormemb(ctx, operands, 0);
	obj = simp_eval(ctx, obj, env);
	if (simp_isexception(ctx, obj))
		return obj;
	if (noperands == 2)
		port = simp_getvectormemb(ctx, operands, 1);
	else
		port = simp_contextoport(ctx);
	return simp_display(ctx, obj, port);
}

Simp
simp_opdivide(Simp ctx, Simp operands, Simp env)
{
	SimpSiz i, noperands;
	SimpInt num = 1;
	Simp obj;
	bool gotop = false;

	if ((noperands = simp_getsize(ctx, operands)) < 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = 0; i < noperands; i++) {
		obj = simp_getvectormemb(ctx, operands, i);
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

Simp
simp_opequal(Simp ctx, Simp operands, Simp env)
{
	Simp objs[2];
	size_t i;

	if (simp_getsize(ctx, operands) != 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = 0; i < LEN(objs); i++) {
		objs[i] = simp_eval(ctx, simp_getvectormemb(ctx, operands, 1), env);
		if (simp_isexception(ctx, objs[i])) {
			return objs[i];
		}
	}
	if (simp_getnum(ctx, objs[0]) == simp_getnum(ctx, objs[1]))
		return simp_true();
	return simp_false();
}

Simp
simp_opfalse(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (simp_getsize(ctx, operands) != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_false();
}

Simp
simp_opgt(Simp ctx, Simp operands, Simp env)
{
	Simp objs[2];
	size_t i;

	if (simp_getsize(ctx, operands) != 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = 0; i < LEN(objs); i++) {
		objs[i] = simp_eval(ctx, simp_getvectormemb(ctx, operands, 1), env);
		if (simp_isexception(ctx, objs[i])) {
			return objs[i];
		}
	}
	if (simp_getnum(ctx, objs[0]) > simp_getnum(ctx, objs[1]))
		return simp_true();
	return simp_false();
}

Simp
simp_opif(Simp ctx, Simp operands, Simp env)
{
	Simp obj;
	SimpSiz noperands;

	noperands = simp_getsize(ctx, operands);
	if (noperands < 2 || noperands > 3)
		return simp_makeexception(ctx, ERROR_ARGS);
	obj = simp_eval(ctx, simp_getvectormemb(ctx, operands, 0), env);
	if (simp_isexception(ctx, obj))
		goto error;
	if (simp_istrue(ctx, obj))
		obj = simp_eval(ctx, simp_getvectormemb(ctx, operands, 1), env);
	else
		obj = simp_eval(ctx, simp_getvectormemb(ctx, operands, 2), env);
error:
	return obj;
}

Simp
simp_oplambda(Simp ctx, Simp operands, Simp env)
{
	if (simp_getsize(ctx, operands) != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_makeapplicative(
		ctx, env,
		simp_getvectormemb(ctx, operands, 0),
		simp_getvectormemb(ctx, operands, 1)
	);
}

Simp
simp_oplt(Simp ctx, Simp operands, Simp env)
{
	Simp objs[2];
	size_t i;

	if (simp_getsize(ctx, operands) != 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = 0; i < LEN(objs); i++) {
		objs[i] = simp_eval(ctx, simp_getvectormemb(ctx, operands, 1), env);
		if (simp_isexception(ctx, objs[i])) {
			return objs[i];
		}
	}
	if (simp_getnum(ctx, objs[0]) < simp_getnum(ctx, objs[1]))
		return simp_true();
	return simp_false();
}

Simp
simp_opmakeenvironment(Simp ctx, Simp operands, Simp env)
{
	if (simp_getsize(ctx, operands) != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_makeenvironment(ctx, env);
}

Simp
simp_opmacro(Simp ctx, Simp operands, Simp env)
{
	if (simp_getsize(ctx, operands) != 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_makeoperative(
		ctx, env,
		simp_getvectormemb(ctx, operands, 0),
		simp_getvectormemb(ctx, operands, 1)
	);
}

Simp
simp_opmultiply(Simp ctx, Simp operands, Simp env)
{
	SimpSiz i, noperands;
	SimpInt num = 1;
	Simp obj;

	noperands = simp_getsize(ctx, operands);
	for (i = 0; i < noperands; i++) {
		obj = simp_getvectormemb(ctx, operands, i);
		if (simp_isexception(ctx, obj))
			return obj;
		obj = simp_eval(ctx, obj, env);
		if (!simp_isnum(ctx, obj))
			return simp_makeexception(ctx, ERROR_ILLTYPE);
		num *= simp_getnum(ctx, obj);
	}
	return simp_makenum(ctx, num);
}

Simp
simp_opnewline(Simp ctx, Simp operands, Simp env)
{
	Simp port;

	(void)env;
	switch (simp_getsize(ctx, operands)) {
	case 0:  port = simp_contextoport(ctx);               break;
	case 1:  port = simp_getvectormemb(ctx, operands, 1); break;
	default: return simp_makeexception(ctx, ERROR_ARGS);
	}
	return simp_printf(ctx, port, "\n");
}

Simp
simp_opnullp(Simp ctx, Simp operands, Simp env)
{
	return typepred(ctx, operands, env, simp_isnil);
}

Simp
simp_opportp(Simp ctx, Simp operands, Simp env)
{
	return typepred(ctx, operands, env, simp_isport);
}

Simp
simp_opsamep(Simp ctx, Simp operands, Simp env)
{
	Simp objs[2];
	SimpSiz i;

	if (simp_getsize(ctx, operands) != 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = 0; i < LEN(objs); i++) {
		objs[i] = simp_eval(ctx, simp_getvectormemb(ctx, operands, 1), env);
		if (simp_isexception(ctx, objs[i])) {
			return objs[i];
		}
	}
	return simp_issame(ctx, objs[0], objs[1]) ? simp_true() : simp_false();
}

Simp
simp_opsubtract(Simp ctx, Simp operands, Simp env)
{
	SimpSiz i, noperands;
	SimpInt num = 0;
	Simp obj;
	bool gotop = false;

	if ((noperands = simp_getsize(ctx, operands)) < 1)
		return simp_makeexception(ctx, ERROR_ARGS);
	for (i = 0; i < noperands; i++) {
		obj = simp_getvectormemb(ctx, operands, i);
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

Simp
simp_opsymbolp(Simp ctx, Simp operands, Simp env)
{
	return typepred(ctx, operands, env, simp_issymbol);
}

Simp
simp_optrue(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (simp_getsize(ctx, operands) != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_true();
}

Simp
simp_opvoid(Simp ctx, Simp operands, Simp env)
{
	(void)env;
	if (simp_getsize(ctx, operands) != 0)
		return simp_makeexception(ctx, ERROR_ARGS);
	return simp_void();
}

Simp
simp_opwrite(Simp ctx, Simp operands, Simp env)
{
	Simp port, obj;
	SimpSiz noperands;

	noperands = simp_getsize(ctx, operands);
	if (noperands < 1 || noperands > 2)
		return simp_makeexception(ctx, ERROR_ARGS);
	obj = simp_getvectormemb(ctx, operands, 0);
	obj = simp_eval(ctx, obj, env);
	if (simp_isexception(ctx, obj))
		return obj;
	if (noperands == 2)
		port = simp_getvectormemb(ctx, operands, 1);
	else
		port = simp_contextoport(ctx);
	return simp_write(ctx, obj, port);
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
