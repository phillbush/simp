#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "simp.h"

#define FORMS                                            \
	X(FORM_AND,             "and"                   )\
	X(FORM_APPLY,           "apply"                 )\
	X(FORM_DEFINE,          "define"                )\
	X(FORM_DO,              "do"                    )\
	X(FORM_EVAL,            "eval"                  )\
	X(FORM_FALSE,           "false"                 )\
	X(FORM_IF,              "if"                    )\
	X(FORM_LAMBDA,          "lambda"                )\
	X(FORM_OR,              "or"                    )\
	X(FORM_QUOTE,           "quote"                 )\
	X(FORM_REDEFINE,        "redefine"              )\
	X(FORM_TRUE,            "true"                  )\
	X(FORM_VARLAMBDA,       "varlambda"             )

#define BUILTINS                                                    \
	/* SYMBOL               FUNCTION        NARGS   VARIADIC */ \
	X("*",                  f_multiply,     0,      true       )\
	X("+",                  f_add,          0,      true       )\
	X("-",                  f_subtract,     1,      true       )\
	X("/",                  f_divide,       1,      true       )\
	X("<",                  f_lt,           0,      true       )\
	X("<=",                 f_le,           0,      true       )\
	X("=",                  f_equal,        1,      true       )\
	X(">",                  f_gt,           0,      true       )\
	X(">=",                 f_ge,           0,      true       )\
	X("abs",                f_abs,          1,      false      )\
	X("alloc",              f_makevector,   1,      false      )\
	X("boolean?",           f_booleanp,     1,      false      )\
	X("byte?",              f_bytep,        1,      false      )\
	X("car",                f_car,          1,      false      )\
	X("cdr",                f_cdr,          1,      false      )\
	X("clone",              f_vectordup,    0,      true       )\
	X("concat",             f_vectorcat,    0,      true       )\
	X("copy!",              f_vectorcpy,    2,      false      )\
	X("display",            f_display,      1,      true       )\
	X("empty?",             f_emptyp,       1,      false      )\
	X("environment",        f_envnew,       0,      true       )\
	X("environment?",       f_envp,         1,      false      )\
	X("equiv?" ,            f_vectoreqv,    0,      true       )\
	X("false?",             f_falsep,       1,      false      )\
	X("for-each",           f_foreach,      1,      true       )\
	X("get",                f_vectorref,    2,      false      )\
	X("length",             f_vectorlen,    1,      false      )\
	X("map",                f_map,          1,      true       )\
	X("member",             f_member,       3,      false      )\
	X("newline",            f_newline,      0,      true       )\
	X("not",                f_not,          1,      false      )\
	X("null?",              f_nullp,        1,      false      )\
	X("number?",            f_numberp,      1,      false      )\
	X("port?",              f_portp,        1,      false      )\
	X("procedure?",         f_procedurep,   1,      false      )\
	X("reverse",            f_vectorrevnew, 1,      false      )\
	X("reverse!",           f_vectorrev,    1,      false      )\
	X("same?",              f_samep,        1,      true       )\
	X("set!",               f_vectorset,    3,      false      )\
	X("slice",              f_slicevector,  1,      true       )\
	X("stderr",             f_stderr,       0,      false      )\
	X("stdin",              f_stdin,        0,      false      )\
	X("stdout",             f_stdout,       0,      false      )\
	X("string",             f_string,       0,      true       )\
	X("string-<=?",         f_stringle,     0,      true       )\
	X("string-<?",          f_stringlt,     0,      true       )\
	X("string->=?",         f_stringge,     0,      true       )\
	X("string->?",          f_stringgt,     0,      true       )\
	X("string->vector",     f_stringvector, 1,      false      )\
	X("string-alloc",       f_makestring,   1,      false      )\
	X("string-clone",       f_stringdup,    1,      false      )\
	X("string-concat",      f_stringcat,    0,      true       )\
	X("string-copy!",       f_stringcpy,    2,      false      )\
	X("string-get",         f_stringref,    2,      false      )\
	X("string-length",      f_stringlen,    1,      false      )\
	X("string-for-each",    f_foreachstring,1,      true       )\
	X("string-map",         f_mapstring,    1,      true       )\
	X("string-set!",        f_stringset,    3,      false      )\
	X("string?",            f_stringp,      1,      false      )\
	X("string-slice",       f_slicestring,  1,      true       )\
	X("symbol?",            f_symbolp,      1,      false      )\
	X("true?",              f_truep,        1,      false      )\
	X("vector",             f_vector,       0,      true       )\
	X("vector?",            f_vectorp,      0,      true       )\
	X("write",              f_write,        2,      false      )

enum Forms {
#define X(n, s) n,
	FORMS
#undef  X
};

struct Builtin {
	char *name;
	bool variadic;
	Simp (*fun)(Simp, Simp);
	SimpSiz nargs;
};

static Simp
typepred(Simp args, bool (*pred)(Simp))
{
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	return (*pred)(obj) ? simp_true() : simp_false();
}

static int
stringcmp(Simp a, Simp b)
{
	SimpSiz size0, size1;
	int cmp;

	size0 = simp_getsize(a);
	size1 = simp_getsize(b);
	cmp = memcmp(
		simp_getstring(a),
		simp_getstring(b),
		size0 < size1 ? size0 : size1
	);
	if (cmp == 0 && size0 != size1)
		return size0 < size1 ? -1 : +1;
	return cmp;
}

static Simp
f_abs(Simp ctx, Simp args)
{
	Simp obj;
	SimpInt num;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isnum(obj))
		return simp_exception(ERROR_ILLTYPE);
	num = simp_getnum(obj);
	num = llabs(num);
	return simp_makenum(ctx, num);
}

static Simp
f_add(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	SimpInt sum;
	Simp obj;

	nargs = simp_getsize(args);
	sum = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			return simp_exception(ERROR_ILLTYPE);
		sum += simp_getnum(obj);
	}
	return simp_makenum(ctx, sum);
}

static Simp
f_bytep(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isbyte);
}

static Simp
f_booleanp(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isbool);
}

static Simp
f_car(Simp ctx, Simp args)
{
	SimpSiz size;
	Simp obj;

	(void)ctx;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(obj);
	if (size < 1)
		return simp_exception(ERROR_OUTOFRANGE);
	return simp_getvectormemb(obj, 0);
}

static Simp
f_cdr(Simp ctx, Simp args)
{
	SimpSiz size;
	Simp obj;

	(void)ctx;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(obj);
	if (size < 1)
		return simp_exception(ERROR_OUTOFRANGE);
	return simp_slicevector(obj, 1, size - 1);
}

static Simp
f_stdin(Simp ctx, Simp args)
{
	(void)args;
	return simp_contextiport(ctx);
}

static Simp
f_stdout(Simp ctx, Simp args)
{
	(void)args;
	return simp_contextoport(ctx);
}

static Simp
f_stderr(Simp ctx, Simp args)
{
	(void)args;
	return simp_contexteport(ctx);
}

static Simp
f_display(Simp ctx, Simp args)
{
	Simp obj, port;

	port = simp_contextoport(ctx);
	switch (simp_getsize(args)) {
	case 2:
		port = simp_getvectormemb(args, 1);
		/* FALLTHROUGH */
	case 1:
		obj = simp_getvectormemb(args, 0);
		break;
	default:
		return simp_exception(ERROR_ARGS);
	}
	if (!simp_isport(port))
		return simp_exception(ERROR_ILLTYPE);
	(void)simp_display(port, obj);
	return simp_void();
}

static Simp
f_divide(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	SimpInt ratio;
	Simp obj;

	nargs = simp_getsize(args);
	ratio = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			return simp_exception(ERROR_ILLTYPE);
		if (nargs == 1 || i > 0) {
			ratio /= simp_getnum(obj);
		} else {
			ratio = simp_getnum(obj);
		}
	}
	return simp_makenum(ctx, ratio);
}

static Simp
f_emptyp(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isempty);
}

static Simp
f_envp(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isenvironment);
}

static Simp
f_envnew(Simp ctx, Simp args)
{
	Simp env;

	env = simp_nulenv();
	switch (simp_getsize(args)) {
	case 1:
		env = simp_getvectormemb(args, 0);
		/* FALLTHROUGH */
	case 0:
		break;
	default:
		return simp_exception(ERROR_ARGS);
	}
	if (!simp_isenvironment(env))
		return simp_exception(ERROR_ILLTYPE);
	return simp_makeenvironment(ctx, env);
}

static Simp
f_equal(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			return simp_exception(ERROR_ILLTYPE);
		if (i == 0)
			continue;
		if (simp_getnum(prev) != simp_getnum(next))
			return simp_false();
	}
	return simp_true();
}

static Simp
f_falsep(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isfalse);
}

static Simp
f_foreach(Simp ctx, Simp args)
{
	Simp expr, prod, obj;
	SimpSiz i, j, n, size, nargs;

	nargs = simp_getsize(args);
	if (nargs < 2)
		return simp_exception(ERROR_ARGS);
	prod = simp_getvectormemb(args, 0);
	if (!simp_isprocedure(prod))
		return simp_exception(ERROR_ILLTYPE);
	size = 0;
	for (i = 1; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isvector(obj))
			return simp_exception(ERROR_ILLTYPE);
		n = simp_getsize(obj);
		if (i == 1){
			size = n;
			continue;
		}
		if (n != size) {
			return simp_exception(ERROR_MAP);
		}
	}
	expr = simp_makevector(ctx, nargs);
	if (simp_isexception(expr))
		return expr;
	simp_setvector(expr, 0, prod);
	for (i = 0; i < size; i++) {
		for (j = 1; j < nargs; j++) {
			obj = simp_getvectormemb(args, j);
			obj = simp_getvectormemb(obj, i);
			simp_setvector(expr, j, obj);
		}
		obj = simp_eval(ctx, expr, simp_nulenv());
		if (simp_isexception(obj)) {
			return obj;
		}
	}
	return simp_void();
}

static Simp
f_foreachstring(Simp ctx, Simp args)
{
	Simp expr, prod, obj;
	SimpSiz i, j, n, size, nargs;
	unsigned char byte;

	nargs = simp_getsize(args);
	if (nargs < 2)
		return simp_exception(ERROR_ARGS);
	prod = simp_getvectormemb(args, 0);
	if (!simp_isprocedure(prod))
		return simp_exception(ERROR_ILLTYPE);
	size = 0;
	for (i = 1; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isstring(obj))
			return simp_exception(ERROR_ILLTYPE);
		n = simp_getsize(obj);
		if (i == 1){
			size = n;
			continue;
		}
		if (n != size) {
			return simp_exception(ERROR_MAP);
		}
	}
	expr = simp_makevector(ctx, nargs);
	if (simp_isexception(expr))
		return expr;
	simp_setvector(expr, 0, prod);
	for (i = 0; i < size; i++) {
		for (j = 1; j < nargs; j++) {
			obj = simp_getvectormemb(args, j);
			byte = simp_getstringmemb(obj, i);
			obj = simp_makebyte(ctx, byte);
			simp_setvector(expr, j, obj);
		}
		obj = simp_eval(ctx, expr, simp_nulenv());
		if (simp_isexception(obj)) {
			return obj;
		}
	}
	return simp_void();
}

static Simp
f_ge(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			return simp_exception(ERROR_ILLTYPE);
		if (i == 0)
			continue;
		if (simp_getnum(prev) < simp_getnum(next))
			return simp_false();
	}
	return simp_true();
}

static Simp
f_gt(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			return simp_exception(ERROR_ILLTYPE);
		if (i == 0)
			continue;
		if (simp_getnum(prev) <= simp_getnum(next))
			return simp_false();
	}
	return simp_true();
}

static Simp
f_le(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			return simp_exception(ERROR_ILLTYPE);
		if (i == 0)
			continue;
		if (simp_getnum(prev) > simp_getnum(next))
			return simp_false();
	}
	return simp_true();
}

static Simp
f_lt(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			return simp_exception(ERROR_ILLTYPE);
		if (i == 0)
			continue;
		if (simp_getnum(prev) >= simp_getnum(next))
			return simp_false();
	}
	return simp_true();
}

static Simp
f_makestring(Simp ctx, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isnum(obj))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getnum(obj);
	if (size < 0)
		return simp_exception(ERROR_OUTOFRANGE);
	return simp_makestring(ctx, NULL, size);
}

static Simp
f_makevector(Simp ctx, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isnum(obj))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getnum(obj);
	if (size < 0)
		return simp_exception(ERROR_OUTOFRANGE);
	return simp_makevector(ctx, size);
}

static Simp
f_map(Simp ctx, Simp args)
{
	Simp vector, expr, prod, obj;
	SimpSiz i, j, n, size, nargs;

	nargs = simp_getsize(args);
	if (nargs < 2)
		return simp_exception(ERROR_ARGS);
	prod = simp_getvectormemb(args, 0);
	if (!simp_isprocedure(prod))
		return simp_exception(ERROR_ILLTYPE);
	size = 0;
	for (i = 1; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isvector(obj))
			return simp_exception(ERROR_ILLTYPE);
		n = simp_getsize(obj);
		if (i == 1){
			size = n;
			continue;
		}
		if (n != size) {
			return simp_exception(ERROR_MAP);
		}
	}
	expr = simp_makevector(ctx, nargs);
	if (simp_isexception(expr))
		return expr;
	vector = simp_makevector(ctx, size);
	if (simp_isexception(vector))
		return vector;
	simp_setvector(expr, 0, prod);
	for (i = 0; i < size; i++) {
		for (j = 1; j < nargs; j++) {
			obj = simp_getvectormemb(args, j);
			obj = simp_getvectormemb(obj, i);
			simp_setvector(expr, j, obj);
		}
		obj = simp_eval(ctx, expr, simp_nulenv());
		if (simp_isexception(obj))
			return obj;
		simp_setvector(vector, i, obj);
	}
	return vector;
}

static Simp
f_mapstring(Simp ctx, Simp args)
{
	Simp string, expr, prod, obj;
	SimpSiz i, j, n, size, nargs;
	unsigned char byte;

	nargs = simp_getsize(args);
	if (nargs < 2)
		return simp_exception(ERROR_ARGS);
	prod = simp_getvectormemb(args, 0);
	if (!simp_isprocedure(prod))
		return simp_exception(ERROR_ILLTYPE);
	size = 0;
	for (i = 1; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isstring(obj))
			return simp_exception(ERROR_ILLTYPE);
		n = simp_getsize(obj);
		if (i == 1){
			size = n;
			continue;
		}
		if (n != size) {
			return simp_exception(ERROR_MAP);
		}
	}
	expr = simp_makevector(ctx, nargs);
	if (simp_isexception(expr))
		return expr;
	string = simp_makestring(ctx, NULL, size);
	if (simp_isexception(string))
		return string;
	simp_setvector(expr, 0, prod);
	for (i = 0; i < size; i++) {
		for (j = 1; j < nargs; j++) {
			obj = simp_getvectormemb(args, j);
			byte = simp_getstringmemb(obj, i);
			obj = simp_makebyte(ctx, byte);
			simp_setvector(expr, j, obj);
		}
		obj = simp_eval(ctx, expr, simp_nulenv());
		if (simp_isexception(obj))
			return obj;
		if (!simp_isbyte(obj))
			return simp_exception(ERROR_NOTBYTE);
		simp_setstring(string, i, simp_getbyte(obj));
	}
	return string;
}

static Simp
f_member(Simp ctx, Simp args)
{
	Simp expr, pred, ref, obj, vector;
	SimpSiz i, size;

	pred = simp_getvectormemb(args, 0);
	ref = simp_getvectormemb(args, 1);
	vector = simp_getvectormemb(args, 2);
	if (!simp_isprocedure(pred))
		return simp_exception(ERROR_ILLTYPE);
	if (!simp_isvector(vector))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(vector);
	expr = simp_makevector(ctx, 3);
	if (simp_isexception(expr))
		return expr;
	simp_setvector(expr, 0, pred);
	simp_setvector(expr, 1, ref);
	for (i = 0; i < size; i++) {
		obj = simp_getvectormemb(vector, i);
		simp_setvector(expr, 2, obj);
		obj = simp_eval(ctx, expr, simp_nulenv());
		if (simp_isexception(obj))
			return obj;
		if (simp_istrue(obj)) {
			return simp_slicevector(vector, i, size - i);
		}
	}
	return simp_false();
}

static Simp
f_multiply(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	SimpInt prod;
	Simp obj;

	nargs = simp_getsize(args);
	prod = 1;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			return simp_exception(ERROR_ILLTYPE);
		prod *= simp_getnum(obj);
	}
	return simp_makenum(ctx, prod);
}

Simp
f_newline(Simp ctx, Simp args)
{
	Simp port;

	port = simp_contextoport(ctx);
	switch (simp_getsize(args)) {
	case 1:
		port = simp_getvectormemb(args, 0);
		/* FALLTHROUGH */
	case 0:
		break;
	default:
		return simp_exception(ERROR_ARGS);
	}
	if (!simp_isport(port))
		return simp_exception(ERROR_ILLTYPE);
	(void)simp_printf(port, "\n");
	return simp_void();
}

static Simp
f_not(Simp ctx, Simp args)
{
	Simp obj;

	(void)ctx;
	obj = simp_getvectormemb(args, 0);
	if (simp_isfalse(obj))
		return simp_true();
	return simp_false();
}

static Simp
f_nullp(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isnil);
}

static Simp
f_numberp(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isnum);
}

static Simp
f_portp(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isport);
}

static Simp
f_procedurep(Simp ctx, Simp args)
{
	Simp obj;

	(void)ctx;
	obj = simp_getvectormemb(args, 0);
	if (simp_isprocedure(obj))
		return simp_true();
	return simp_false();
}

static Simp
f_samep(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (i == 0)
			continue;
		if (!simp_issame(prev, next))
			return simp_false();
	}
	return simp_true();
}

static Simp
f_slicevector(Simp ctx, Simp args)
{
	Simp vector, obj;
	SimpSiz nargs, from, size, capacity;

	(void)ctx;
	nargs = simp_getsize(args);
	if (nargs < 1 || nargs > 3)
		return simp_exception(ERROR_ARGS);
	vector = simp_getvectormemb(args, 0);
	if (!simp_isvector(vector))
		return simp_exception(ERROR_ILLTYPE);
	from = 0;
	capacity = simp_getsize(vector);
	if (nargs > 1) {
		obj = simp_getvectormemb(args, 1);
		if (!simp_isnum(obj)) {
			return simp_exception(ERROR_ILLTYPE);
		}
		from = simp_getnum(obj);
		if (from < 0 || from > capacity) {
			return simp_exception(ERROR_RANGE);
		}
	}
	size = capacity - from;
	if (nargs > 2) {
		obj = simp_getvectormemb(args, 1);
		if (!simp_isnum(obj)) {
			return simp_exception(ERROR_ILLTYPE);
		}
		size = simp_getnum(obj);
		if (size < 0 || from + size > capacity) {
			return simp_exception(ERROR_RANGE);
		}
	}
	if (size == 0)
		return simp_nil();
	return simp_slicevector(vector, from, size);
}

static Simp
f_slicestring(Simp ctx, Simp args)
{
	Simp string, obj;
	SimpSiz nargs, from, size, capacity;

	(void)ctx;
	nargs = simp_getsize(args);
	if (nargs < 1 || nargs > 3)
		return simp_exception(ERROR_ARGS);
	string = simp_getvectormemb(args, 0);
	if (!simp_isstring(string))
		return simp_exception(ERROR_ILLTYPE);
	from = 0;
	size = simp_getsize(string);
	capacity = simp_getcapacity(string);
	if (nargs > 1) {
		obj = simp_getvectormemb(args, 1);
		if (!simp_isnum(obj)) {
			return simp_exception(ERROR_ILLTYPE);
		}
		from = simp_getnum(obj);
		if (from < 0 || from > capacity) {
			return simp_exception(ERROR_RANGE);
		}
	}
	if (nargs > 2) {
		obj = simp_getvectormemb(args, 1);
		if (!simp_isnum(obj)) {
			return simp_exception(ERROR_ILLTYPE);
		}
		size = simp_getnum(obj);
		if (size < 0 || from + size > capacity) {
			return simp_exception(ERROR_RANGE);
		}
	}
	if (size == 0)
		return simp_nil();
	return simp_slicestring(string, from, size);
}

static Simp
f_subtract(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	SimpInt diff;
	Simp obj;

	nargs = simp_getsize(args);
	diff = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			return simp_exception(ERROR_ILLTYPE);
		if (nargs == 1 || i > 0) {
			diff -= simp_getnum(obj);
		} else {
			diff = simp_getnum(obj);
		}
	}
	return simp_makenum(ctx, diff);
}

static Simp
f_string(Simp ctx, Simp args)
{
	Simp string, obj;
	SimpSiz nargs, i;

	nargs = simp_getsize(args);
	string = simp_makestring(ctx, NULL, nargs);
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isbyte(obj))
			return simp_exception(ERROR_ILLTYPE);
		simp_setstring(string, i, simp_getbyte(obj));
	}
	return string;
}

static Simp
f_stringcat(Simp ctx, Simp args)
{
	Simp obj, string;
	SimpSiz nargs, size, n, i;

	nargs = simp_getsize(args);
	size = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isstring(obj))
			return simp_exception(ERROR_ILLTYPE);
		size += simp_getsize(obj);
	}
	string = simp_makestring(ctx, NULL, size);
	if (simp_isexception(string))
		return string;
	for (size = i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		n = simp_getsize(obj);
		simp_cpystring(
			simp_slicestring(string, size, n),
			obj
		);
		size += n;
	}
	return string;
}

static Simp
f_stringcpy(Simp ctx, Simp args)
{
	Simp dst, src;
	SimpSiz dstsiz, srcsiz;

	(void)ctx;
	dst = simp_getvectormemb(args, 0);
	src = simp_getvectormemb(args, 1);
	if (!simp_isstring(dst) || !simp_isstring(src))
		return simp_exception(ERROR_ILLTYPE);
	dstsiz = simp_getsize(dst);
	srcsiz = simp_getsize(src);
	if (srcsiz > dstsiz)
		return simp_exception(ERROR_OUTOFRANGE);
	simp_cpystring(dst, src);
	return simp_void();
}

static Simp
f_stringdup(Simp ctx, Simp args)
{
	Simp obj;
	SimpSiz len;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isstring(obj))
		return simp_exception(ERROR_ILLTYPE);
	len = simp_getsize(obj);
	return simp_makestring(ctx, simp_getstring(obj), len);
}

static Simp
f_stringge(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isstring(next))
			return simp_exception(ERROR_ILLTYPE);
		if (i == 0)
			continue;
		if (stringcmp(prev,next) < 0)
			return simp_false();
	}
	return simp_true();
}

static Simp
f_stringgt(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isstring(next))
			return simp_exception(ERROR_ILLTYPE);
		if (i == 0)
			continue;
		if (stringcmp(prev,next) <= 0)
			return simp_false();
	}
	return simp_true();
}

static Simp
f_stringle(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isstring(next))
			return simp_exception(ERROR_ILLTYPE);
		if (i == 0)
			continue;
		if (stringcmp(prev,next) > 0)
			return simp_false();
	}
	return simp_true();
}

static Simp
f_stringlt(Simp ctx, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isstring(next))
			return simp_exception(ERROR_ILLTYPE);
		if (i == 0)
			continue;
		if (stringcmp(prev,next) >= 0)
			return simp_false();
	}
	return simp_true();
}

static Simp
f_stringlen(Simp ctx, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isstring(obj))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(obj);
	return simp_makenum(ctx, size);
}

static Simp
f_stringref(Simp ctx, Simp args)
{
	SimpSiz size;
	SimpInt pos;
	Simp a, b;
	unsigned u;

	a = simp_getvectormemb(args, 0);
	b = simp_getvectormemb(args, 1);
	if (!simp_isstring(a) || !simp_isnum(b))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(a);
	pos = simp_getnum(b);
	if (pos < 0 || pos >= (SimpInt)size)
		return simp_exception(ERROR_OUTOFRANGE);
	u = simp_getstringmemb(a, pos);
	return simp_makebyte(ctx, u);
}

static Simp
f_stringp(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isstring);
}

static Simp
f_stringvector(Simp ctx, Simp args)
{
	Simp vector, str, byte;
	SimpSiz i, size;
	unsigned char u;

	str = simp_getvectormemb(args, 0);
	if (!simp_isstring(str))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(str);
	vector = simp_makevector(ctx, size);
	if (simp_isexception(vector))
		return vector;
	for (i = 0; i < size; i++) {
		u = simp_getstringmemb(str, i);
		byte = simp_makebyte(ctx, u);
		if (simp_isexception(byte))
			return byte;
		simp_setvector(vector, i, byte);
	}
	return vector;
}

static Simp
f_stringset(Simp ctx, Simp args)
{
	Simp str, pos, val;
	SimpSiz size;
	SimpInt n;
	unsigned char u;

	(void)ctx;
	str = simp_getvectormemb(args, 0);
	pos = simp_getvectormemb(args, 1);
	val = simp_getvectormemb(args, 2);
	if (!simp_isstring(str) || !simp_isnum(pos) || !simp_isbyte(val))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(str);
	n = simp_getnum(pos);
	u = simp_getbyte(val);
	if (n < 0 || n >= (SimpInt)size)
		return simp_exception(ERROR_OUTOFRANGE);
	simp_setstring(str, n, u);
	return simp_void();
}

static Simp
f_truep(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_istrue);
}

static Simp
f_symbolp(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_issymbol);
}

static Simp
f_vector(Simp ctx, Simp args)
{
	(void)ctx;
	return args;
}

static Simp
f_vectorcat(Simp ctx, Simp args)
{
	Simp obj, vector;
	SimpSiz nargs, size, n, i;

	nargs = simp_getsize(args);
	size = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isvector(obj))
			return simp_exception(ERROR_ILLTYPE);
		size += simp_getsize(obj);
	}
	vector = simp_makevector(ctx, size);
	if (simp_isexception(vector))
		return vector;
	for (size = i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		n = simp_getsize(obj);
		simp_cpyvector(
			simp_slicevector(vector, size, n),
			obj
		);
		size += n;
	}
	return vector;
}

static Simp
f_vectorcpy(Simp ctx, Simp args)
{
	Simp dst, src;
	SimpSiz dstsiz, srcsiz;

	(void)ctx;
	dst = simp_getvectormemb(args, 0);
	src = simp_getvectormemb(args, 1);
	if (!simp_isvector(dst) || !simp_isvector(src))
		return simp_exception(ERROR_ILLTYPE);
	dstsiz = simp_getsize(dst);
	srcsiz = simp_getsize(src);
	if (srcsiz > dstsiz)
		return simp_exception(ERROR_OUTOFRANGE);
	simp_cpyvector(dst, src);
	return simp_void();
}

static Simp
f_vectordup(Simp ctx, Simp args)
{
	Simp src, dst;
	SimpSiz i, len;

	src = simp_getvectormemb(args, 0);
	if (!simp_isvector(src))
		return simp_exception(ERROR_ILLTYPE);
	len = simp_getsize(src);
	dst = simp_makevector(ctx, len);
	if (simp_isexception(dst))
		return dst;
	for (i = 0; i < len; i++) {
		simp_setvector(
			dst,
			i,
			simp_getvectormemb(src, i)
		);
	}
	return dst;
}

static Simp
f_vectoreqv(Simp ctx, Simp args)
{
	Simp a, b;
	Simp next, prev;
	SimpSiz nargs, newsize, oldsize, i, j;

	(void)ctx;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next, oldsize = newsize) {
		next = simp_getvectormemb(args, i);
		if (!simp_isvector(next))
			return simp_exception(ERROR_ILLTYPE);
		newsize = simp_getsize(next);
		if (i == 0)
			continue;
		if (oldsize != newsize)
			return simp_false();
		for (j = 0; j < newsize; j++) {
			a = simp_getvectormemb(prev, j);
			b = simp_getvectormemb(next, j);
			if (!simp_issame(a, b)) {
				return simp_false();
			}
		}
	}
	return simp_true();
}

static Simp
f_vectorp(Simp ctx, Simp args)
{
	(void)ctx;
	return typepred(args, simp_isvector);
}

static Simp
f_vectorset(Simp ctx, Simp args)
{
	Simp str, pos, val;
	SimpSiz size;
	SimpInt n;

	(void)ctx;
	str = simp_getvectormemb(args, 0);
	pos = simp_getvectormemb(args, 1);
	val = simp_getvectormemb(args, 2);
	if (!simp_isvector(str) || !simp_isnum(pos))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(str);
	n = simp_getnum(pos);
	if (n < 0 || n >= (SimpInt)size)
		return simp_exception(ERROR_OUTOFRANGE);
	simp_setvector(str, n, val);
	return simp_void();
}

static Simp
f_vectorlen(Simp ctx, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(obj);
	return simp_makenum(ctx, size);
}

static Simp
f_vectorref(Simp ctx, Simp args)
{
	SimpSiz size;
	SimpInt pos;
	Simp a, b;

	(void)ctx;
	a = simp_getvectormemb(args, 0);
	b = simp_getvectormemb(args, 1);
	if (!simp_isvector(a) || !simp_isnum(b))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(a);
	pos = simp_getnum(b);
	if (pos < 0 || pos >= (SimpInt)size)
		return simp_exception(ERROR_OUTOFRANGE);
	return simp_getvectormemb(a, pos);
}

static Simp
f_vectorrev(Simp ctx, Simp args)
{
	SimpSiz i, n, size;
	Simp obj, beg, end;

	(void)ctx;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(obj);
	for (i = 0; i < size / 2; i++) {
		n = size - i - 1;
		beg = simp_getvectormemb(obj, i);
		end = simp_getvectormemb(obj, n);
		simp_setvector(obj, i, end);
		simp_setvector(obj, n, beg);
	}
	return obj;
}

static Simp
f_vectorrevnew(Simp ctx, Simp args)
{
	SimpSiz i, n, size;
	Simp vector, obj, beg, end;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		return simp_exception(ERROR_ILLTYPE);
	size = simp_getsize(obj);
	vector = simp_makevector(ctx, size);
	if (simp_isexception(vector))
		return vector;
	for (i = 0; i < size / 2; i++) {
		n = size - i - 1;
		beg = simp_getvectormemb(obj, i);
		end = simp_getvectormemb(obj, n);
		simp_setvector(vector, i, end);
		simp_setvector(vector, n, beg);
	}
	if (size % 2 == 1) {
		obj = simp_getvectormemb(obj, i);
		simp_setvector(vector, i, obj);
	}
	return vector;
}

Simp
f_write(Simp ctx, Simp args)
{
	Simp obj, port;

	port = simp_contextoport(ctx);
	switch (simp_getsize(args)) {
	case 2:
		port = simp_getvectormemb(args, 1);
		/* FALLTHROUGH */
	case 1:
		obj = simp_getvectormemb(args, 0);
		break;
	default:
		return simp_exception(ERROR_ARGS);
	}
	if (!simp_isport(port))
		return simp_exception(ERROR_ILLTYPE);
	(void)simp_write(port, obj);
	return simp_void();
}

static Simp
define(Simp ctx, Simp expr, Simp env, bool redefine)
{
	Simp symbol, value;

	if (simp_getsize(expr) != 3)       /* (define VAR VAL) */
		return simp_exception(ERROR_ARGS);
	symbol = simp_getvectormemb(expr, 1);
	if (!simp_issymbol(symbol))
		return simp_exception(ERROR_NOTSYM);
	value = simp_getvectormemb(expr, 2);
	value = simp_eval(ctx, value, env);
	if (simp_isexception(value))
		return value;
	if (redefine)
		value = simp_envset(ctx, env, symbol, value);
	else
		value = simp_envdef(ctx, env, symbol, value);
	if (simp_isexception(value))
		return value;
	return simp_void();
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
	if (simp_isexception(forms))
		return forms;
	for (i = 0; i < LEN(formnames); i++) {
		len = strlen((char *)formnames[i]);
		sym = simp_makesymbol(ctx, formnames[i], len);
		if (simp_isexception(sym))
			return sym;
		simp_setvector(forms, i, sym);
	}
	return forms;
}

Simp
simp_initbuiltins(Simp ctx)
{
	Simp env, sym, ret, val;
	SimpSiz i, len;
	static Builtin builtins[] = {
#define X(s, p, a, v) { .name = s, .fun = &p, .nargs = a, .variadic = v },
		BUILTINS
#undef  X
	};

	env = simp_contextenvironment(ctx);
	for (i = 0; i < LEN(builtins); i++) {
		len = strlen(builtins[i].name);
		sym = simp_makesymbol(ctx, (unsigned char *)builtins[i].name, len);
		if (simp_isexception(sym))
			return sym;
		val = simp_makebuiltin(ctx, &builtins[i]);
		if (simp_isexception(val))
			return val;
		ret = simp_envdef(ctx, env, sym, val);
		if (simp_isexception(ret))
			return ret;
	}
	return simp_nil();
}

Simp
simp_eval(Simp ctx, Simp expr, Simp env)
{
	Builtin *bltin;
	Simp *forms;
	Simp operator, operands, arguments, extraargs, extraparams;
	Simp params, var, val;
	SimpSiz noperands, nparams, narguments, nextraargs, i;

	forms = simp_getvector(simp_contextforms(ctx));
loop:
	if (simp_issymbol(expr))   /* expression is variable */
		return simp_envget(ctx, env, expr);
	if (!simp_isvector(expr))  /* expression is self-evaluating */
		return expr;
	if ((noperands = simp_getsize(expr)) == 0)
		return simp_exception(ERROR_EMPTY);
	noperands--;
	operator = simp_getvectormemb(expr, 0);
	operands = simp_slicevector(expr, 1, noperands);
	if (simp_issame(operator, forms[FORM_APPLY])) {
		/* (apply PROC ARG ... ARGS) */
		if (noperands < 2)
			return simp_exception(ERROR_ILLFORM);
		operator = simp_getvectormemb(operands, 0);
		extraargs = simp_getvectormemb(operands, noperands - 1);
		extraargs = simp_eval(ctx, extraargs, env);
		if (simp_isexception(extraargs))
			return extraargs;
		if (simp_isvoid(extraargs))
			return simp_exception(ERROR_VOID);
		if (!simp_isvector(extraargs))
			return simp_exception(ERROR_ILLTYPE);
		nextraargs = simp_getsize(extraargs);
		operands = simp_slicevector(operands, 1, noperands - 2);
		noperands -= 2;
		goto apply;
	} else if (simp_issame(operator, forms[FORM_AND])) {
		/* (and EXPRESSION ...) */
		val = simp_true();
		for (i = 0; i < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			val = simp_eval(ctx, val, env);
			if (simp_isexception(val))
				return val;
			if (simp_isvoid(val))
				return simp_exception(ERROR_VOID);
			if (simp_isfalse(val))
				return val;
		}
		return val;
	} else if (simp_issame(operator, forms[FORM_OR])) {
		/* (or EXPRESSION ...) */
		val = simp_false();
		for (i = 0; i < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			val = simp_eval(ctx, val, env);
			if (simp_isexception(val))
				return val;
			if (simp_isvoid(val))
				return simp_exception(ERROR_VOID);
			if (simp_istrue(val))
				return val;
		}
		return val;
	} else if (simp_issame(operator, forms[FORM_DEFINE])) {
		return define(ctx, expr, env, false);
	} else if (simp_issame(operator, forms[FORM_DO])) {
		/* (do EXPRESSION ...) */
		if (noperands == 0)
			return simp_void();
		for (i = 0; i + 1 < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			val = simp_eval(ctx, val, env);
			if (simp_isexception(val))
				return val;
		}
		expr = simp_getvectormemb(operands, i);
		goto loop;
	} else if (simp_issame(operator, forms[FORM_IF])) {
		/* (if [COND THEN]... [ELSE]) */
		if (noperands < 2)
			return simp_exception(ERROR_ILLFORM);
		for (i = 0; i + 1 < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			val = simp_eval(ctx, val, env);
			if (simp_isvoid(val))
				return simp_exception(ERROR_VOID);
			if (simp_isexception(val))
				return val;
			i++;
			if (simp_istrue(val)) {
				break;
			}
		}
		if (i < noperands) {
			expr = simp_getvectormemb(operands, i);
			goto loop;
		}
		return simp_void();
	} else if (simp_issame(operator, forms[FORM_EVAL])) {
		/* (eval EXPRESSION ENVIRONMENT) */
		if (noperands != 2)
			return simp_exception(ERROR_ILLFORM);
		expr = simp_eval(
			ctx,
			simp_getvectormemb(operands, 0),
			env
		);
		if (simp_isexception(expr))
			return expr;
		if (simp_isvoid(expr))
			return simp_exception(ERROR_VOID);
		env = simp_eval(
			ctx,
			simp_getvectormemb(operands, 1),
			env
		);
		if (simp_isexception(env))
			return env;
		if (simp_isvoid(env))
			return simp_exception(ERROR_VOID);
		goto loop;
	} else if (simp_issame(operator, forms[FORM_LAMBDA])) {
		/* (lambda PARAMETER ... BODY) */
		if (noperands < 1)
			return simp_exception(ERROR_ILLFORM);
		expr = simp_getvectormemb(operands, noperands - 1);
		params = simp_slicevector(operands, 0, noperands - 1);
		for (i = 0; i + 1 < noperands; i++) {
			var = simp_getvectormemb(operands, i);
			if (!simp_issymbol(var)) {
				return simp_exception(ERROR_ILLFORM);
			}
		}
		return simp_makeclosure(ctx, env, params, simp_nil(), expr);
	} else if (simp_issame(operator, forms[FORM_VARLAMBDA])) {
		/* (lambda PARAMETER PARAMETER ... BODY) */
		if (noperands < 2)
			return simp_exception(ERROR_ILLFORM);
		expr = simp_getvectormemb(operands, noperands - 1);
		extraparams = simp_getvectormemb(operands, noperands - 2);
		params = simp_slicevector(operands, 0, noperands - 2);
		for (i = 0; i + 1 < noperands; i++) {
			var = simp_getvectormemb(operands, i);
			if (!simp_issymbol(var)) {
				return simp_exception(ERROR_ILLFORM);
			}
		}
		return simp_makeclosure(ctx, env, params, extraparams, expr);
	} else if (simp_issame(operator, forms[FORM_QUOTE])) {
		/* (quote OBJ) */
		if (noperands != 1)
			return simp_exception(ERROR_ILLFORM);
		return simp_getvectormemb(operands, 0);
	} else if (simp_issame(operator, forms[FORM_REDEFINE])) {
		return define(ctx, expr, env, true);
	} else if (simp_issame(operator, forms[FORM_FALSE])) {
		/* (false) */
		if (noperands != 0)
			return simp_exception(ERROR_ILLFORM);
		return simp_false();
	} else if (simp_issame(operator, forms[FORM_TRUE])) {
		/* (true) */
		if (noperands != 0)
			return simp_exception(ERROR_ILLFORM);
		return simp_true();
	}
	extraargs = simp_nil();
	nextraargs = 0;
apply:
	/* procedure application */
	operator = simp_eval(ctx, operator, env);
	if (simp_isexception(operator))
		return operator;
	if (simp_isvoid(operator))
		return simp_exception(ERROR_VOID);
	narguments = noperands + nextraargs;
	arguments = simp_makevector(ctx, narguments);
	if (simp_isexception(arguments))
		return arguments;
	for (i = 0; i < noperands; i++) {
		/* evaluate arguments */
		val = simp_getvectormemb(operands, i);
		val = simp_eval(ctx, val, env);
		if (simp_isexception(val))
			return val;
		if (simp_isvoid(val))
			return simp_exception(ERROR_VOID);
		simp_setvector(arguments, i, val);
	}
	for (i = 0; i < nextraargs; i++) {
		/* evaluate extra arguments */
		val = simp_getvectormemb(extraargs, i);
		val = simp_eval(ctx, val, env);
		if (simp_isexception(val))
			return val;
		if (simp_isvoid(val))
			return simp_exception(ERROR_VOID);
		simp_setvector(arguments, i + noperands, val);
	}
	if (simp_isbuiltin(operator)) {
		bltin = simp_getbuiltin(operator);
		if (bltin->variadic && narguments < bltin->nargs)
			return simp_exception(ERROR_ARGS);
		if (!bltin->variadic && narguments != bltin->nargs)
			return simp_exception(ERROR_ARGS);
		return (*bltin->fun)(ctx, arguments);
	}
	if (!simp_isclosure(operator))
		return simp_exception(ERROR_OPERATOR);
	expr = simp_getclosurebody(operator);
	env = simp_getclosureenv(operator);
	env = simp_makeenvironment(ctx, env);
	if (simp_isexception(env))
		return env;
	params = simp_getclosureparam(operator);
	nparams = simp_getsize(params);
	extraparams = simp_getclosurevarargs(operator);
	if (narguments < nparams)
		return simp_exception(ERROR_ARGS);
	if (narguments > nparams && simp_isnil(extraparams))
		return simp_exception(ERROR_ARGS);
	for (i = 0; i < nparams; i++) {
		var = simp_getvectormemb(params, i);
		val = simp_getvectormemb(arguments, i);
		var = simp_envdef(ctx, env, var, val);
		if (simp_isexception(var)) {
			return var;
		}
	}
	if (simp_issymbol(extraparams)) {
		val = simp_slicevector(arguments, i, narguments - i);
		var = simp_envdef(ctx, env, extraparams, val);
		if (simp_isexception(var)) {
			return var;
		}
	}
	goto loop;
}

int
simp_repl(Simp ctx, Simp iport, int mode)
{
	Simp oport, eport, obj, env;
	int retval = -1;

	env = simp_contextenvironment(ctx);
	oport = simp_contextoport(ctx);
	eport = simp_contexteport(ctx);
	for (;;) {
		simp_gc(ctx, &iport, 1);
		if (simp_porterr(iport))
			goto error;
		if (mode & SIMP_PROMPT)
			simp_printf(oport, "> ");
		obj = simp_read(ctx, iport);
		if (simp_iseof(obj))
			break;
		if (!simp_isexception(obj))
			obj = simp_eval(ctx, obj, env);
		if (simp_isexception(obj)) {
			simp_write(eport, obj);
			simp_printf(eport, "\n");
			if (mode & SIMP_CONTINUE)
				continue;
			break;
		}
		if ((mode & SIMP_ECHO) && !simp_isvoid(obj)) {
			simp_write(oport, obj);
			simp_printf(oport, "\n");
		}
	}
	retval = 0;
error:
	simp_gc(ctx, &iport, 1);
	return retval;
}
