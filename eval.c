#include <limits.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "simp.h"

#define ERROR_DIVZERO     "division by zero"
#define ERROR_EMPTY       "empty operation"
#define ERROR_ILLFORM     "ill-formed syntactical form"
#define ERROR_MAP         "map over vectors of different sizes"
#define ERROR_MEMORY      "allocation error"
#define ERROR_NARGS       "wrong number of arguments"
#define ERROR_NIL         "expected non-nil vector"
#define ERROR_NOTBYTE     "expected byte; got "
#define ERROR_NOTENV      "expected environment; got "
#define ERROR_NOTFIT      "source object do not fit destination"
#define ERROR_NOTNUM      "expected number; got "
#define ERROR_NOTPORT     "expected device port; got "
#define ERROR_NOTPROC     "expected procedure; got "
#define ERROR_NOTSTRING   "expected string; got "
#define ERROR_NOTSYM      "expected symbol; got "
#define ERROR_NOTVECTOR   "expected vector; got "
#define ERROR_RANGE       "out of range: "
#define ERROR_READ        "read error"
#define ERROR_STREAM      "stream error"
#define ERROR_UNBOUND     "unbound variable: "
#define ERROR_VARFORM     "macro used as variable: "
#define ERROR_VOID        "expression evaluated to nothing; expected value"

#define FORMS                                            \
	X(FORM_AND,             "and"                   )\
	X(FORM_APPLY,           "apply"                 )\
	X(FORM_DEFINE,          "define"                )\
	X(FORM_DEFUN,           "defun"                 )\
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
	X("read",               f_read,         0,      true       )\
	X("remainder",          f_remainder,    2,      false      )\
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
	X("write",              f_write,        1,      true       )

enum {
	/*
	 * An environment frame is a linked-list of triplets containing
	 * a symbol of the variable, its value, and a pointer to the
	 * next triplet.
	 */
	BINDING_VARIABLE,
	BINDING_VALUE,
	BINDING_NEXT,
	BINDING_SIZE,
};

typedef enum Form {
#define X(n, s) n,
	FORMS
#undef  X
} Form;

typedef struct Eval {
	Simp ctx;
	Simp env;
	Simp iport, oport, eport;
	jmp_buf jmp;
} Eval;

struct Builtin {
	unsigned char *name;
	bool variadic;
	void (*fun)(Eval *, Simp *, Simp, Simp, Simp);
	SimpSiz nargs;
	SimpSiz namelen;
	Simp self;
};

static Simp simp_eval(Eval *eval, Simp expr, Simp env);

static void
error(Eval *eval, Simp expr, Simp sym, Simp obj, const char *errmsg)
{
	const char *filename;
	SimpSiz lineno;
	SimpSiz column;

	if (simp_getsource(expr, &filename, &lineno, &column)) {
		simp_printf(
			eval->eport,
			"%s:%llu:%llu: ",
			filename,
			lineno,
			column
		);
	}
	if (simp_issymbol(sym)) {
		simp_printf(eval->eport, "in ");
		simp_write(eval->eport, sym);
		simp_printf(eval->eport, ": ");
	}
	simp_printf(eval->eport, "%s", errmsg);
	if (!simp_isvoid(obj))
		simp_write(eval->eport, obj);
	simp_printf(eval->eport, "\n");
	longjmp(eval->jmp, 1);
	abort();
}

static void
typepred(Simp args, Simp *ret, bool (*pred)(Simp))
{
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	*ret = (*pred)(obj) ? simp_true() : simp_false();
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

static void
memerror(Eval *eval)
{
	error(eval, simp_nil(), simp_void(), simp_void(), ERROR_MEMORY);
}

static void
f_abs(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp obj;
	SimpInt num;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isnum(obj))
		error(eval, expr, self, obj, ERROR_NOTNUM);
	num = simp_getnum(obj);
	num = llabs(num);
	if (!simp_makenum(eval->ctx, ret, num))
		memerror(eval);
}

static void
f_add(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	SimpInt sum;
	Simp obj;

	nargs = simp_getsize(args);
	sum = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			error(eval, expr, self, obj, ERROR_NOTNUM);
		sum += simp_getnum(obj);
	}
	if (!simp_makenum(eval->ctx, ret, sum)) {
		memerror(eval);
	}
}

static void
f_bytep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)expr;
	(void)self;
	typepred(args, ret, simp_isbyte);
}

static void
f_booleanp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)expr;
	(void)self;
	typepred(args, ret, simp_isbool);
}

static void
f_car(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz size;
	Simp obj;

	(void)eval;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		error(eval, expr, self, obj, ERROR_NOTNUM);
	size = simp_getsize(obj);
	if (size < 1)
		error(eval, expr, self, simp_nil(), ERROR_NIL);
	*ret = simp_getvectormemb(obj, 0);
}

static void
f_cdr(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz size;
	Simp obj;

	(void)eval;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		error(eval, expr, self, obj, ERROR_NOTVECTOR);
	size = simp_getsize(obj);
	if (size < 1)
		error(eval, expr, self, simp_nil(), ERROR_NIL);
	*ret = simp_slicevector(obj, 1, size - 1);
}

static void
f_stdin(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)ret;
	(void)self;
	(void)expr;
	(void)args;
	*ret = eval->iport;
}

static void
f_stdout(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)ret;
	(void)self;
	(void)expr;
	(void)args;
	*ret = eval->oport;
}

static void
f_stderr(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)ret;
	(void)self;
	(void)expr;
	(void)args;
	*ret = eval->eport;
}

static void
f_display(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp obj, port;

	port = eval->oport;
	switch (simp_getsize(args)) {
	case 2:
		port = simp_getvectormemb(args, 1);
		/* FALLTHROUGH */
	case 1:
		obj = simp_getvectormemb(args, 0);
		break;
	default:
		error(eval, expr, self, simp_void(), ERROR_NARGS);
		abort();
	}
	if (!simp_isport(port))
		error(eval, expr, self, port, ERROR_NOTPORT);
	simp_display(port, obj);
	*ret = simp_void();
}

static void
f_divide(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	SimpInt ratio, d;
	Simp obj;

	nargs = simp_getsize(args);
	ratio = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			error(eval, expr, self, obj, ERROR_NOTNUM);
		d = simp_getnum(obj);
		if (nargs > 1 && i == 0) {
			ratio = d;
		} else if (d != 0) {
			ratio /= d;
		} else {
			error(eval, expr, self, simp_void(), ERROR_DIVZERO);
		}
	}
	if (!simp_makenum(eval->ctx, ret, ratio)) {
		memerror(eval);
	}
}

static void
f_emptyp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)expr;
	(void)self;
	typepred(args, ret, simp_isempty);
}

static void
f_envp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)expr;
	(void)self;
	typepred(args, ret, simp_isenvironment);
}

static void
f_envnew(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
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
		error(eval, expr, self, simp_void(), ERROR_NARGS);
	}
	if (!simp_isenvironment(env))
		error(eval, expr, self, env, ERROR_NOTENV);
	if (!simp_makeenvironment(eval->ctx, ret, env))
		memerror(eval);
}

static void
f_equal(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	nargs = simp_getsize(args);
	*ret = simp_true();
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_getnum(prev) != simp_getnum(next)) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_falsep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	typepred(args, ret, simp_isfalse);
}

static void
f_foreach(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp prod, obj;
	SimpSiz i, j, n, size, nargs;

	*ret = simp_void();
	nargs = simp_getsize(args);
	if (nargs < 2)
		error(eval, expr, self, simp_void(), ERROR_NARGS);
	prod = simp_getvectormemb(args, 0);
	if (!simp_isprocedure(prod))
		error(eval, expr, self, prod, ERROR_NOTPROC);
	size = 0;
	for (i = 1; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isvector(obj))
			error(eval, expr, self, obj, ERROR_NOTVECTOR);
		n = simp_getsize(obj);
		if (i == 1){
			size = n;
			continue;
		}
		if (n != size) {
			error(eval, expr, self, simp_void(), ERROR_MAP);
		}
	}
	if (!simp_makevector(eval->ctx, &expr, nargs))
		memerror(eval);
	simp_setvector(expr, 0, prod);
	for (i = 0; i < size; i++) {
		for (j = 1; j < nargs; j++) {
			obj = simp_getvectormemb(args, j);
			obj = simp_getvectormemb(obj, i);
			simp_setvector(expr, j, obj);
		}
		(void)simp_eval(eval, expr, simp_nulenv());
	}
}

static void
f_foreachstring(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp prod, obj;
	SimpSiz i, j, n, size, nargs;
	unsigned char byte;

	*ret = simp_void();
	nargs = simp_getsize(args);
	if (nargs < 2)
		error(eval, expr, self, simp_void(), ERROR_NARGS);
	prod = simp_getvectormemb(args, 0);
	if (!simp_isprocedure(prod))
		error(eval, expr, self, prod, ERROR_NOTPROC);
	size = 0;
	for (i = 1; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isstring(obj))
			error(eval, expr, self, obj, ERROR_NOTSTRING);
		n = simp_getsize(obj);
		if (i == 1){
			size = n;
			continue;
		}
		if (n != size) {
			error(eval, expr, self, simp_void(), ERROR_MAP);
		}
	}
	if (!simp_makevector(eval->ctx, &expr, nargs))
		memerror(eval);
	simp_setvector(expr, 0, prod);
	for (i = 0; i < size; i++) {
		for (j = 1; j < nargs; j++) {
			obj = simp_getvectormemb(args, j);
			byte = simp_getstringmemb(obj, i);
			if (!simp_makebyte(eval->ctx, &obj, byte))
				memerror(eval);
			simp_setvector(expr, j, obj);
		}
		(void)simp_eval(eval, expr, simp_nulenv());
	}
}

static void
f_ge(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	nargs = simp_getsize(args);
	*ret = simp_true();
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_getnum(prev) < simp_getnum(next)) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_gt(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_getnum(prev) <= simp_getnum(next)) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_le(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_getnum(prev) > simp_getnum(next)) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_lt(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_getnum(prev) >= simp_getnum(next)) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_makestring(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isnum(obj))
		error(eval, expr, self, obj, ERROR_NOTNUM);
	size = simp_getnum(obj);
	if (size < 0)
		error(eval, expr, self, obj, ERROR_RANGE);
	if (!simp_makestring(eval->ctx, ret, NULL, size))
		memerror(eval);
}

static void
f_makevector(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isnum(obj))
		error(eval, expr, self, obj, ERROR_NOTNUM);
	size = simp_getnum(obj);
	if (size < 0)
		error(eval, expr, self, obj, ERROR_RANGE);
	if (!simp_makevector(eval->ctx, ret, size))
		memerror(eval);
}

static void
f_map(Eval *eval, Simp *vector, Simp self, Simp expr, Simp args)
{
	Simp prod, obj;
	SimpSiz i, j, n, size, nargs;

	nargs = simp_getsize(args);
	if (nargs < 2)
		error(eval, expr, self, simp_void(), ERROR_NARGS);
	prod = simp_getvectormemb(args, 0);
	if (!simp_isprocedure(prod))
		error(eval, expr, self, prod, ERROR_NOTPROC);
	size = 0;
	for (i = 1; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isvector(obj))
			error(eval, expr, self, obj, ERROR_NOTVECTOR);
		n = simp_getsize(obj);
		if (i == 1){
			size = n;
			continue;
		}
		if (n != size) {
			error(eval, expr, self, simp_void(), ERROR_MAP);
		}
	}
	if (!simp_makevector(eval->ctx, &expr, nargs))
		memerror(eval);
	if (!simp_makevector(eval->ctx, vector, size))
		memerror(eval);
	simp_setvector(expr, 0, prod);
	for (i = 0; i < size; i++) {
		for (j = 1; j < nargs; j++) {
			obj = simp_getvectormemb(args, j);
			obj = simp_getvectormemb(obj, i);
			simp_setvector(expr, j, obj);
		}
		obj = simp_eval(eval, expr, simp_nulenv());
		simp_setvector(*vector, i, obj);
	}
}

static void
f_mapstring(Eval *eval, Simp *string, Simp self, Simp expr, Simp args)
{
	Simp newexpr, prod, obj;
	SimpSiz i, j, n, size, nargs;
	unsigned char byte;

	nargs = simp_getsize(args);
	if (nargs < 2)
		error(eval, expr, self, simp_void(), ERROR_NARGS);
	prod = simp_getvectormemb(args, 0);
	if (!simp_isprocedure(prod))
		error(eval, expr, self, prod, ERROR_NOTPROC);
	size = 0;
	for (i = 1; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isstring(obj))
			error(eval, expr, self, obj, ERROR_NOTSTRING);
		n = simp_getsize(obj);
		if (i == 1){
			size = n;
			continue;
		}
		if (n != size) {
			error(eval, expr, self, simp_void(), ERROR_MAP);
		}
	}
	if (!simp_makevector(eval->ctx, &newexpr, nargs))
		memerror(eval);
	if (!simp_makestring(eval->ctx, string, NULL, size))
		memerror(eval);
	simp_setvector(newexpr, 0, prod);
	for (i = 0; i < size; i++) {
		for (j = 1; j < nargs; j++) {
			obj = simp_getvectormemb(args, j);
			byte = simp_getstringmemb(obj, i);
			if (!simp_makebyte(eval->ctx, &obj, byte))
				memerror(eval);
			simp_setvector(newexpr, j, obj);
		}
		obj = simp_eval(eval, newexpr, simp_nulenv());
		if (!simp_isbyte(obj))
			error(eval, expr, self, obj, ERROR_NOTBYTE);
		simp_setstring(*string, i, simp_getbyte(obj));
	}
}

static void
f_member(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp newexpr, pred, ref, obj, vector;
	SimpSiz i, size;

	*ret = simp_false();
	pred = simp_getvectormemb(args, 0);
	ref = simp_getvectormemb(args, 1);
	vector = simp_getvectormemb(args, 2);
	if (!simp_isprocedure(pred))
		error(eval, expr, self, pred, ERROR_NOTPROC);
	if (!simp_isvector(vector))
		error(eval, expr, self, vector, ERROR_NOTVECTOR);
	size = simp_getsize(vector);
	if (!simp_makevector(eval->ctx, &newexpr, 3))
		memerror(eval);
	simp_setvector(newexpr, 0, pred);
	simp_setvector(newexpr, 1, ref);
	for (i = 0; i < size; i++) {
		obj = simp_getvectormemb(vector, i);
		simp_setvector(newexpr, 2, obj);
		obj = simp_eval(eval, newexpr, simp_nulenv());
		if (simp_istrue(obj)) {
			*ret = simp_slicevector(vector, i, size - i);
			break;
		}
	}
}

static void
f_multiply(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	SimpInt prod;
	Simp obj;

	nargs = simp_getsize(args);
	prod = 1;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			error(eval, expr, self, obj, ERROR_NOTNUM);
		prod *= simp_getnum(obj);
	}
	if (!simp_makenum(eval->ctx, ret, prod))
		memerror(eval);
}

static void
f_newline(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp port;

	(void)ret;
	port = eval->oport;
	switch (simp_getsize(args)) {
	case 1:
		port = simp_getvectormemb(args, 0);
		/* FALLTHROUGH */
	case 0:
		break;
	default:
		error(eval, expr, self, simp_void(), ERROR_NARGS);
		abort();
	}
	if (!simp_isport(port))
		error(eval, expr, self, port, ERROR_NOTPORT);
	(void)simp_printf(port, "\n");
}

static void
f_not(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp obj;

	(void)eval;
	(void)self;
	(void)expr;
	*ret = simp_false();
	obj = simp_getvectormemb(args, 0);
	if (simp_isfalse(obj))
		*ret = simp_true();
}

static void
f_nullp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	typepred(args, ret, simp_isnil);
}

static void
f_numberp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	typepred(args, ret, simp_isnum);
}

static void
f_portp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	typepred(args, ret, simp_isport);
}

static void
f_procedurep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	typepred(args, ret, simp_isprocedure);
}

static void
f_read(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp port;

	port = eval->iport;
	switch (simp_getsize(args)) {
	case 1:
		port = simp_getvectormemb(args, 0);
		/* FALLTHROUGH */
	case 0:
		break;
	default:
		error(eval, expr, self, simp_void(), ERROR_NARGS);
		abort();
	}
	if (!simp_isport(port))
		error(eval, expr, self, port, ERROR_NOTPORT);
	if (!simp_read(eval->ctx, ret, port))
		error(eval, expr, self, simp_void(), ERROR_READ);
}

static void
f_remainder(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpInt d;
	Simp a, b;

	a = simp_getvectormemb(args, 0);
	b = simp_getvectormemb(args, 1);
	if (!simp_isnum(a))
		error(eval, expr, self, a, ERROR_NOTNUM);
	if (!simp_isnum(b))
		error(eval, expr, self, b, ERROR_NOTNUM);
	d = simp_getnum(b);
	if (d == 0)
		error(eval, expr, self, simp_void(), ERROR_DIVZERO);
	d = simp_getnum(a) % d;
	if (!simp_makenum(eval->ctx, ret, d))
		memerror(eval);
}

static void
f_samep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	(void)self;
	(void)expr;
	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (i == 0)
			continue;
		if (!simp_issame(prev, next)) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_slicevector(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp vector, obj;
	SimpSiz nargs, from, size, capacity;

	(void)eval;
	nargs = simp_getsize(args);
	if (nargs < 1 || nargs > 3)
		error(eval, expr, self, simp_void(), ERROR_NARGS);
	vector = simp_getvectormemb(args, 0);
	if (!simp_isvector(vector))
		error(eval, expr, self, vector, ERROR_NOTVECTOR);
	from = 0;
	capacity = simp_getsize(vector);
	if (nargs > 1) {
		obj = simp_getvectormemb(args, 1);
		if (!simp_isnum(obj)) {
			error(eval, expr, self, obj, ERROR_NOTNUM);
		}
		from = simp_getnum(obj);
		if (from < 0 || from > capacity) {
			error(eval, expr, self, obj, ERROR_RANGE);
		}
	}
	size = capacity - from;
	if (nargs > 2) {
		obj = simp_getvectormemb(args, 2);
		if (!simp_isnum(obj)) {
			error(eval, expr, self, obj, ERROR_NOTNUM);
		}
		size = simp_getnum(obj);
		if (size < 0 || from + size > capacity) {
			error(eval, expr, self, obj, ERROR_RANGE);
		}
	}
	if (size == 0)
		*ret = simp_nil();
	else
		*ret = simp_slicevector(vector, from, size);
}

static void
f_slicestring(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp string, obj;
	SimpSiz nargs, from, size, capacity;

	(void)eval;
	nargs = simp_getsize(args);
	if (nargs < 1 || nargs > 3)
		error(eval, expr, self, simp_void(), ERROR_NARGS);
	string = simp_getvectormemb(args, 0);
	if (!simp_isstring(string))
		error(eval, expr, self, string, ERROR_NOTSTRING);
	from = 0;
	size = simp_getsize(string);
	capacity = simp_getsize(string);
	if (nargs > 1) {
		obj = simp_getvectormemb(args, 1);
		if (!simp_isnum(obj)) {
			error(eval, expr, self, obj, ERROR_NOTNUM);
		}
		from = simp_getnum(obj);
		if (from < 0 || from > capacity) {
			error(eval, expr, self, obj, ERROR_RANGE);
		}
	}
	if (nargs > 2) {
		obj = simp_getvectormemb(args, 2);
		if (!simp_isnum(obj)) {
			error(eval, expr, self, obj, ERROR_NOTNUM);
		}
		size = simp_getnum(obj);
		if (size < 0 || from + size > capacity) {
			error(eval, expr, self, obj, ERROR_RANGE);
		}
	}
	if (size == 0)
		*ret = simp_nil();
	else
		*ret = simp_slicestring(string, from, size);
}

static void
f_subtract(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	SimpInt diff;
	Simp obj;

	nargs = simp_getsize(args);
	diff = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			error(eval, expr, self, obj, ERROR_NOTNUM);
		if (nargs == 1 || i > 0) {
			diff -= simp_getnum(obj);
		} else {
			diff = simp_getnum(obj);
		}
	}
	if (!simp_makenum(eval->ctx, ret, diff))
		memerror(eval);
}

static void
f_string(Eval *eval, Simp *string, Simp self, Simp expr, Simp args)
{
	Simp obj;
	SimpSiz nargs, i;

	nargs = simp_getsize(args);
	if (!simp_makestring(eval->ctx, string, NULL, nargs))
		memerror(eval);
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isbyte(obj))
			error(eval, expr, self, obj, ERROR_NOTBYTE);
		simp_setstring(*string, i, simp_getbyte(obj));
	}
}

static void
f_stringcat(Eval *eval, Simp *string, Simp self, Simp expr, Simp args)
{
	Simp obj;
	SimpSiz nargs, size, n, i;

	nargs = simp_getsize(args);
	size = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isstring(obj))
			error(eval, expr, self, obj, ERROR_NOTSTRING);
		size += simp_getsize(obj);
	}
	if (!simp_makestring(eval->ctx, string, NULL, size))
		memerror(eval);
	for (size = i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		n = simp_getsize(obj);
		simp_cpystring(
			simp_slicestring(*string, size, n),
			obj
		);
		size += n;
	}
}

static void
f_stringcpy(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp dst, src;
	SimpSiz dstsiz, srcsiz;

	(void)ret;
	dst = simp_getvectormemb(args, 0);
	src = simp_getvectormemb(args, 1);
	if (!simp_isstring(dst))
		error(eval, expr, self, dst, ERROR_NOTSTRING);
	if (!simp_isstring(src))
		error(eval, expr, self, dst, ERROR_NOTSTRING);
	dstsiz = simp_getsize(dst);
	srcsiz = simp_getsize(src);
	if (srcsiz > dstsiz)
		error(eval, expr, self, simp_void(), ERROR_NOTFIT);
	simp_cpystring(dst, src);
}

static void
f_stringdup(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp obj;
	SimpSiz len;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isstring(obj))
		error(eval, expr, self, obj, ERROR_NOTSTRING);
	len = simp_getsize(obj);
	if (!simp_makestring(eval->ctx, ret, simp_getstring(obj), len))
		memerror(eval);
}

static void
f_stringge(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isstring(next))
			error(eval, expr, self, next, ERROR_NOTSTRING);
		if (i == 0)
			continue;
		if (stringcmp(prev,next) < 0) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_stringgt(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isstring(next))
			error(eval, expr, self, next, ERROR_NOTSTRING);
		if (i == 0)
			continue;
		if (stringcmp(prev,next) <= 0) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_stringle(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isstring(next))
			error(eval, expr, self, next, ERROR_NOTSTRING);
		if (i == 0)
			continue;
		if (stringcmp(prev,next) > 0) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_stringlt(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isstring(next))
			error(eval, expr, self, next, ERROR_NOTSTRING);
		if (i == 0)
			continue;
		if (stringcmp(prev,next) >= 0) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_stringlen(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isstring(obj))
		error(eval, expr, self, obj, ERROR_NOTSTRING);
	size = simp_getsize(obj);
	if (!simp_makenum(eval->ctx, ret, size))
		memerror(eval);
}

static void
f_stringref(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz size;
	SimpInt pos;
	Simp a, b;
	unsigned u;

	a = simp_getvectormemb(args, 0);
	b = simp_getvectormemb(args, 1);
	if (!simp_isstring(a))
		error(eval, expr, self, a, ERROR_NOTSTRING);
	if (!simp_isnum(b))
		error(eval, expr, self, b, ERROR_NOTNUM);
	size = simp_getsize(a);
	pos = simp_getnum(b);
	if (pos < 0 || pos >= (SimpInt)size)
		error(eval, expr, self, b, ERROR_RANGE);
	u = simp_getstringmemb(a, pos);
	if (!simp_makebyte(eval->ctx, ret, u))
		memerror(eval);
}

static void
f_stringp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	typepred(args, ret, simp_isstring);
}

static void
f_stringvector(Eval *eval, Simp *vector, Simp self, Simp expr, Simp args)
{
	Simp str, byte;
	SimpSiz i, size;
	unsigned char u;

	str = simp_getvectormemb(args, 0);
	if (!simp_isstring(str))
		error(eval, expr, self, str, ERROR_NOTSTRING);
	size = simp_getsize(str);
	if (!simp_makevector(eval->ctx, vector, size))
		memerror(eval);
	for (i = 0; i < size; i++) {
		u = simp_getstringmemb(str, i);
		if (!simp_makebyte(eval->ctx, &byte, u))
			memerror(eval);
		simp_setvector(*vector, i, byte);
	}
}

static void
f_stringset(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp str, pos, val;
	SimpSiz size;
	SimpInt n;
	unsigned char u;

	(void)ret;
	str = simp_getvectormemb(args, 0);
	pos = simp_getvectormemb(args, 1);
	val = simp_getvectormemb(args, 2);
	if (!simp_isstring(str))
		error(eval, expr, self, str, ERROR_NOTSTRING);
	if (!simp_isnum(pos))
		error(eval, expr, self, pos, ERROR_NOTNUM);
	if (!simp_isbyte(val))
		error(eval, expr, self, val, ERROR_NOTBYTE);
	size = simp_getsize(str);
	n = simp_getnum(pos);
	u = simp_getbyte(val);
	if (n < 0 || n >= (SimpInt)size)
		error(eval, expr, self, pos, ERROR_RANGE);
	simp_setstring(str, n, u);
}

static void
f_truep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	typepred(args, ret, simp_istrue);
}

static void
f_symbolp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	typepred(args, ret, simp_issymbol);
}

static void
f_vector(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)ret;
	*ret = args;
}

static void
f_vectorcat(Eval *eval, Simp *vector, Simp self, Simp expr, Simp args)
{
	Simp obj;
	SimpSiz nargs, size, n, i;

	nargs = simp_getsize(args);
	size = 0;
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isvector(obj))
			error(eval, expr, self, obj, ERROR_NOTVECTOR);
		size += simp_getsize(obj);
	}
	if (!simp_makevector(eval->ctx, vector, size))
		memerror(eval);
	for (size = i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		n = simp_getsize(obj);
		simp_cpyvector(
			simp_slicevector(*vector, size, n),
			obj
		);
		size += n;
	}
}

static void
f_vectorcpy(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp dst, src;
	SimpSiz dstsiz, srcsiz;

	(void)ret;
	dst = simp_getvectormemb(args, 0);
	src = simp_getvectormemb(args, 1);
	if (!simp_isvector(dst))
		error(eval, expr, self, dst, ERROR_NOTVECTOR);
	if (!simp_isvector(src))
		error(eval, expr, self, src, ERROR_NOTVECTOR);
	dstsiz = simp_getsize(dst);
	srcsiz = simp_getsize(src);
	if (srcsiz > dstsiz)
		error(eval, expr, self, simp_void(), ERROR_NOTFIT);
	simp_cpyvector(dst, src);
}

static void
f_vectordup(Eval *eval, Simp *dst, Simp self, Simp expr, Simp args)
{
	Simp src;
	SimpSiz i, len;

	src = simp_getvectormemb(args, 0);
	if (!simp_isvector(src))
		error(eval, expr, self, src, ERROR_NOTVECTOR);
	len = simp_getsize(src);
	if (!simp_makevector(eval->ctx, dst, len))
		memerror(eval);
	for (i = 0; i < len; i++) {
		simp_setvector(
			*dst,
			i,
			simp_getvectormemb(src, i)
		);
	}
}

static void
f_vectoreqv(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp a, b;
	Simp next, prev;
	SimpSiz nargs, newsize, oldsize, i, j;

	(void)eval;
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next, oldsize = newsize) {
		next = simp_getvectormemb(args, i);
		if (!simp_isvector(next))
			error(eval, expr, self, next, ERROR_NOTVECTOR);
		newsize = simp_getsize(next);
		if (i == 0)
			continue;
		if (oldsize != newsize) {
			*ret = simp_false();
			return;
		}
		for (j = 0; j < newsize; j++) {
			a = simp_getvectormemb(prev, j);
			b = simp_getvectormemb(next, j);
			if (!simp_issame(a, b)) {
				*ret = simp_false();
				return;
			}
		}
	}
	*ret = simp_true();
}

static void
f_vectorp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	typepred(args, ret, simp_isvector);
}

static void
f_vectorset(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp vector, pos, val;
	SimpSiz size;
	SimpInt n;

	(void)ret;
	vector = simp_getvectormemb(args, 0);
	pos = simp_getvectormemb(args, 1);
	val = simp_getvectormemb(args, 2);
	if (!simp_isvector(vector))
		error(eval, expr, self, vector, ERROR_NOTVECTOR);
	if (!simp_isnum(pos))
		error(eval, expr, self, pos, ERROR_NOTNUM);
	size = simp_getsize(vector);
	n = simp_getnum(pos);
	if (n < 0 || n >= (SimpInt)size)
		error(eval, expr, self, pos, ERROR_RANGE);
	simp_setvector(vector, n, val);
}

static void
f_vectorlen(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpInt size;
	Simp obj;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		error(eval, expr, self, obj, ERROR_NOTVECTOR);
	size = simp_getsize(obj);
	if (!simp_makenum(eval->ctx, ret, size))
		memerror(eval);
}

static void
f_vectorref(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz size;
	SimpInt pos;
	Simp a, b;

	(void)eval;
	a = simp_getvectormemb(args, 0);
	b = simp_getvectormemb(args, 1);
	if (!simp_isvector(a))
		error(eval, expr, self, a, ERROR_NOTVECTOR);
	if (!simp_isnum(b))
		error(eval, expr, self, b, ERROR_NOTNUM);
	size = simp_getsize(a);
	pos = simp_getnum(b);
	if (pos < 0 || pos >= (SimpInt)size)
		error(eval, expr, self, b, ERROR_RANGE);
	*ret = simp_getvectormemb(a, pos);
}

static void
f_vectorrev(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	SimpSiz i, n, size;
	Simp obj, beg, end;

	(void)eval;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		error(eval, expr, self, obj, ERROR_NOTVECTOR);
	size = simp_getsize(obj);
	for (i = 0; i < size / 2; i++) {
		n = size - i - 1;
		beg = simp_getvectormemb(obj, i);
		end = simp_getvectormemb(obj, n);
		simp_setvector(obj, i, end);
		simp_setvector(obj, n, beg);
	}
	*ret = obj;
}

static void
f_vectorrevnew(Eval *eval, Simp *vector, Simp self, Simp expr, Simp args)
{
	SimpSiz i, n, size;
	Simp obj, beg, end;

	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		error(eval, expr, self, obj, ERROR_NOTVECTOR);
	size = simp_getsize(obj);
	if (!simp_makevector(eval->ctx, vector, size))
		memerror(eval);
	for (i = 0; i < size / 2; i++) {
		n = size - i - 1;
		beg = simp_getvectormemb(obj, i);
		end = simp_getvectormemb(obj, n);
		simp_setvector(*vector, i, end);
		simp_setvector(*vector, n, beg);
	}
	if (size % 2 == 1) {
		obj = simp_getvectormemb(obj, i);
		simp_setvector(*vector, i, obj);
	}
}

static void
f_write(Eval *eval, Simp *ret, Simp self, Simp expr, Simp args)
{
	Simp obj, port;

	(void)ret;
	port = eval->oport;
	switch (simp_getsize(args)) {
	case 2:
		port = simp_getvectormemb(args, 1);
		/* FALLTHROUGH */
	case 1:
		obj = simp_getvectormemb(args, 0);
		break;
	default:
		error(eval, expr, self, simp_void(), ERROR_NARGS);
		abort();
	}
	if (!simp_isport(port))
		error(eval, expr, self, port, ERROR_NOTPORT);
	simp_write(port, obj);
}

static bool
isform(Simp obj, Form *form)
{
	SimpSiz i;
	int cmp;
	static struct {
		const char *name;
		size_t size;
	} forms[] = {
#define X(n, s) [n] = { .name = s, .size = sizeof(s)-1, },
		FORMS
#undef  X
	};

	if (!simp_issymbol(obj))
		return false;
	for (i = 0; i < LEN(forms); i++) {
		if (simp_getsize(obj) != forms[i].size)
			continue;
		cmp = memcmp(
			simp_getsymbol(obj),
			forms[i].name,
			forms[i].size
		);
		if (cmp == 0) {
			if (form != NULL)
				*form = i;
			return true;
		}
	}
	return false;
}

static Simp *
getbind(Simp obj)
{
	return simp_getvector(obj);
}

static Simp
getbindvariable(Simp obj)
{
	return getbind(obj)[BINDING_VARIABLE];
}

static Simp
getbindvalue(Simp obj)
{
	return getbind(obj)[BINDING_VALUE];
}

static Simp
getnextbind(Simp obj)
{
	return getbind(obj)[BINDING_NEXT];
}

static bool
xenvset(Simp env, Simp var, Simp val)
{
	Simp bind, sym;

	for (bind = simp_getenvframe(env);
	     !simp_isnil(bind);
	     bind = getnextbind(bind)) {
		sym = getbindvariable(bind);
		if (simp_issame(var, sym)) {
			simp_setvector(bind, BINDING_VALUE, val);
			return true;
		}
	}
	return false;
}

static Simp
envget(Eval *eval, Simp expr, Simp env, Simp sym)
{
	Simp bind, var;

	if (isform(sym, NULL))
		error(eval, expr, simp_void(), sym, ERROR_VARFORM);
	for (; !simp_isnulenv(env); env = simp_getenvparent(env)) {
		for (bind = simp_getenvframe(env);
		     !simp_isnil(bind);
		     bind = getnextbind(bind)) {
			var = getbindvariable(bind);
			if (simp_issame(var, sym)) {
				return getbindvalue(bind);
			}
		}
	}
	error(eval, expr, simp_void(), sym, ERROR_UNBOUND);
	abort();
}

static void
envset(Eval *eval, Simp expr, Simp env, Simp var, Simp val)
{
	if (isform(var, NULL))
		error(eval, expr, simp_void(), var, ERROR_VARFORM);
	if (!xenvset(env, var, val))
		error(eval, expr, simp_void(), var, ERROR_UNBOUND);
}

static void
envdef(Eval *eval, Simp expr, Simp env, Simp var, Simp val)
{
	Simp frame, bind;

	if (isform(var, NULL))
		error(eval, expr, simp_void(), var, ERROR_VARFORM);
	if (xenvset(env, var, val))
		return;
	frame = simp_getenvframe(env);
	if (!simp_makevector(eval->ctx, &bind, BINDING_SIZE))
		memerror(eval);
	simp_setvector(bind, BINDING_VARIABLE, var);
	simp_setvector(bind, BINDING_VALUE, val);
	simp_setvector(bind, BINDING_NEXT, frame);
	simp_setenvframe(env, bind);
}

static Builtin bltins[] = {
#define X(s, p, a, v) { \
.name = (unsigned char *)s, \
.fun = &p, \
.nargs = a, \
.variadic = v, \
.namelen = sizeof(s)-1 },
	BUILTINS
#undef  X
};

static bool
initialdef(Simp ctx, Simp env, Simp var, Simp val)
{
	Simp frame, bind;

	frame = simp_getenvframe(env);
	if (!simp_makevector(ctx, &bind, BINDING_SIZE))
		return false;
	simp_setvector(bind, BINDING_VARIABLE, var);
	simp_setvector(bind, BINDING_VALUE, val);
	simp_setvector(bind, BINDING_NEXT, frame);
	simp_setenvframe(env, bind);
	return true;
}

bool
simp_environmentnew(Simp ctx, Simp *env)
{
	Simp val;
	SimpSiz i;

	if (!simp_makeenvironment(ctx, env, simp_nulenv()))
		return false;
	for (i = 0; i < LEN(bltins); i++) {
		if (!simp_makesymbol(ctx, &bltins[i].self, bltins[i].name, bltins[i].namelen))
			return false;
		if (!simp_makebuiltin(ctx, &val, &bltins[i]))
			return false;
		if (!initialdef(ctx, *env, bltins[i].self, val))
			return false;
	}
	return true;
}

static Simp
simp_eval(Eval *eval, Simp expr, Simp env)
{
	Form form;
	Builtin *bltin;
	Simp operator, operands, arguments, extraargs, extraparams;
	Simp params, proc, body, var, val;
	SimpSiz noperands, nparams, narguments, nextraargs, i;

loop:
	if (simp_issymbol(expr))   /* expression is variable */
		return envget(eval, expr, env, expr);
	if (!simp_isvector(expr))  /* expression is self-evaluating */
		return expr;
	if ((noperands = simp_getsize(expr)) == 0)
		error(eval, expr, simp_void(), simp_void(), ERROR_EMPTY);
	noperands--;
	operator = simp_getvectormemb(expr, 0);
	operands = simp_slicevector(expr, 1, noperands);
	if (isform(operator, &form)) switch (form) {
	case FORM_APPLY:
		/* (apply PROC ARG ... ARGS) */
		if (noperands < 2)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		proc = simp_getvectormemb(operands, 0);
		extraargs = simp_getvectormemb(operands, noperands - 1);
		extraargs = simp_eval(eval, extraargs, env);
		if (simp_isvoid(extraargs))
			error(eval, expr, operator, simp_void(), ERROR_VOID);
		if (!simp_isvector(extraargs))
			error(eval, expr, operator, extraargs, ERROR_NOTVECTOR);
		nextraargs = simp_getsize(extraargs);
		operands = simp_slicevector(operands, 1, noperands - 2);
		noperands -= 2;
		operator = proc;
		goto apply;
	case FORM_AND:
		/* (and EXPRESSION ...) */
		val = simp_true();
		for (i = 0; i < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			val = simp_eval(eval, val, env);
			if (simp_isvoid(val))
				error(eval, expr, operator, simp_void(), ERROR_VOID);
			if (simp_isfalse(val)) {
				break;
			}
		}
		return val;
	case FORM_OR:
		/* (or EXPRESSION ...) */
		val = simp_false();
		for (i = 0; i < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			val = simp_eval(eval, val, env);
			if (simp_isvoid(val))
				error(eval, expr, operator, simp_void(), ERROR_VOID);
			if (simp_istrue(val)) {
				break;
			}
		}
		return val;
	case FORM_DEFINE:
		/* (define VAR VAL) */
		if (noperands != 2)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		var = simp_getvectormemb(expr, 1);
		if (!simp_issymbol(var))
			error(eval, expr, operator, var, ERROR_NOTSYM);
		val = simp_getvectormemb(expr, 2);
		val = simp_eval(eval, val, env);
		envdef(eval, expr, env, var, val);
		return simp_void();
	case FORM_DO:
		/* (do EXPRESSION ...) */
		if (noperands == 0)
			return simp_void();
		for (i = 0; i + 1 < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			val = simp_eval(eval, val, env);
		}
		expr = simp_getvectormemb(operands, i);
		goto loop;
	case FORM_IF:
		/* (if [COND THEN]... [ELSE]) */
		if (noperands < 2)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		for (i = 0; i + 1 < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			val = simp_eval(eval, val, env);
			if (simp_isvoid(val))
				error(eval, expr, operator, simp_void(), ERROR_VOID);
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
	case FORM_EVAL:
		/* (eval EXPRESSION ENVIRONMENT) */
		if (noperands != 2)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		expr = simp_eval(
			eval,
			simp_getvectormemb(operands, 0),
			env
		);
		if (simp_isvoid(expr))
			error(eval, expr, operator, simp_void(), ERROR_VOID);
		env = simp_eval(
			eval,
			simp_getvectormemb(operands, 1),
			env
		);
		if (simp_isvoid(env))
			error(eval, expr, operator, simp_void(), ERROR_VOID);
		goto loop;
	case FORM_DEFUN:
		/* (defun SYMBOL PARAMETER ... BODY) */
		if (noperands < 2)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		var = simp_getvectormemb(expr, 1);
		if (!simp_issymbol(var))
			error(eval, expr, operator, var, ERROR_NOTSYM);
		operands = simp_slicevector(operands, 1, --noperands);
		body = simp_getvectormemb(operands, noperands - 1);
		params = simp_slicevector(operands, 0, noperands - 1);
		for (i = 0; i + 1 < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			if (!simp_issymbol(val)) {
				error(eval, expr, operator, val, ERROR_NOTSYM);
			}
		}
		if (!simp_makeclosure(eval->ctx, &proc, expr, env, params, simp_nil(), body))
			memerror(eval);
		envdef(eval, expr, env, var, proc);
		return simp_void();
	case FORM_LAMBDA:
		/* (lambda PARAMETER ... BODY) */
		if (noperands < 1)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		body = simp_getvectormemb(operands, noperands - 1);
		params = simp_slicevector(operands, 0, noperands - 1);
		for (i = 0; i + 1 < noperands; i++) {
			var = simp_getvectormemb(operands, i);
			if (!simp_issymbol(var)) {
				error(eval, expr, operator, var, ERROR_NOTSYM);
			}
		}
		if (!simp_makeclosure(eval->ctx, &proc, expr, env, params, simp_nil(), body))
			memerror(eval);
		return proc;
	case FORM_VARLAMBDA:
		/* (lambda PARAMETER PARAMETER ... BODY) */
		if (noperands < 2)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		body = simp_getvectormemb(operands, noperands - 1);
		extraparams = simp_getvectormemb(operands, noperands - 2);
		params = simp_slicevector(operands, 0, noperands - 2);
		for (i = 0; i + 1 < noperands; i++) {
			var = simp_getvectormemb(operands, i);
			if (!simp_issymbol(var)) {
				error(eval, expr, operator, var, ERROR_NOTSYM);
			}
		}
		if (!simp_makeclosure(eval->ctx, &proc, expr, env, params, extraparams, body))
			memerror(eval);
		return proc;
	case FORM_QUOTE:
		/* (quote OBJ) */
		if (noperands != 1)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		return simp_getvectormemb(operands, 0);
	case FORM_REDEFINE:
		/* (redefine VAR VAL) */
		if (noperands != 2)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		var = simp_getvectormemb(expr, 1);
		if (!simp_issymbol(var))
			error(eval, expr, operator, var, ERROR_NOTSYM);
		val = simp_getvectormemb(expr, 2);
		val = simp_eval(eval, val, env);
		envset(eval, expr, env, var, val);
		return simp_void();
	case FORM_FALSE:
		/* (false) */
		if (noperands != 0)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		return simp_false();
	case FORM_TRUE:
		/* (true) */
		if (noperands != 0)
			error(eval, expr, operator, simp_void(), ERROR_ILLFORM);
		return simp_true();
	}
	extraargs = simp_nil();
	nextraargs = 0;
apply:
	/* procedure application */
	operator = simp_eval(eval, operator, env);
	if (simp_isvoid(operator))
		error(eval, expr, operator, simp_void(), ERROR_VOID);
	narguments = noperands + nextraargs;
	if (!simp_makevector(eval->ctx, &arguments, narguments))
		memerror(eval);
	for (i = 0; i < noperands; i++) {
		/* evaluate arguments */
		val = simp_getvectormemb(operands, i);
		val = simp_eval(eval, val, env);
		if (simp_isvoid(val))
			error(eval, expr, operator, simp_void(), ERROR_VOID);
		simp_setvector(arguments, i, val);
	}
	for (i = 0; i < nextraargs; i++) {
		/* evaluate extra arguments */
		val = simp_getvectormemb(extraargs, i);
		val = simp_eval(eval, val, env);
		if (simp_isvoid(val))
			error(eval, expr, operator, simp_void(), ERROR_VOID);
		simp_setvector(arguments, i + noperands, val);
	}
	if (simp_isbuiltin(operator)) {
		val = simp_void();
		bltin = simp_getbuiltin(operator);
		if (bltin->variadic && narguments < bltin->nargs)
			error(eval, expr, operator, simp_void(), ERROR_NARGS);
		if (!bltin->variadic && narguments != bltin->nargs)
			error(eval, expr, operator, simp_void(), ERROR_NARGS);
		(*bltin->fun)(eval, &val, bltin->self, expr, arguments);
		return val;
	}
	if (!simp_isclosure(operator))
		error(eval, expr, simp_void(), operator, ERROR_NOTPROC);
	expr = simp_getclosurebody(operator);
	if (!simp_makeenvironment(eval->ctx, &env, simp_getclosureenv(operator)))
		memerror(eval);
	params = simp_getclosureparam(operator);
	nparams = simp_getsize(params);
	extraparams = simp_getclosurevarargs(operator);
	if (narguments < nparams)
		error(eval, expr, operator, simp_void(), ERROR_NARGS);
	if (narguments > nparams && simp_isnil(extraparams))
		error(eval, expr, operator, simp_void(), ERROR_NARGS);
	for (i = 0; i < nparams; i++) {
		var = simp_getvectormemb(params, i);
		val = simp_getvectormemb(arguments, i);
		envdef(eval, expr, env, var, val);
	}
	if (simp_issymbol(extraparams)) {
		val = simp_slicevector(arguments, i, narguments - i);
		envdef(eval, expr, env, extraparams, val);
	}
	goto loop;
}

int
simp_repl(Simp ctx, Simp env, Simp rport, Simp iport, Simp oport, Simp eport, int mode)
{
	Simp obj;
	Simp gcignore[] = {
		env, rport, iport, oport, eport
	};
	Eval eval = {
		.ctx = ctx,
		.env = env,
		.iport = iport,
		.oport = oport,
		.eport = eport,
	};
	int retval = 1;

	if (setjmp(eval.jmp) && !FLAG(mode, SIMP_CONTINUE))
		goto error;
	for (;;) {
		simp_gc(ctx, gcignore, LEN(gcignore));
		if (simp_porterr(rport))
			goto error;
		if (mode & SIMP_PROMPT)
			simp_printf(oport, "> ");
		if (!simp_read(ctx, &obj, rport)) {
			simp_printf(
				eport,
				"%s:%llu:%llu: not a valid expression\n",
				simp_portfilename(eport),
				simp_portlineno(eport),
				simp_portcolumn(eport)
			);
			if (FLAG(mode, SIMP_CONTINUE))
				continue;
			goto error;
		}
		if (simp_iseof(obj))
			break;
		obj = simp_eval(&eval, obj, env);
		if ((mode & SIMP_ECHO) && !simp_isvoid(obj)) {
			simp_write(oport, obj);
			simp_printf(oport, "\n");
		}
	}
	retval = 0;
error:
	simp_gc(ctx, gcignore, LEN(gcignore));
	return retval;
}
