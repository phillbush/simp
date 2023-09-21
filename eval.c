#include <limits.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "simp.h"

#define ERROR_AUXILIARY   "invalid use of auxiliary syntax: "
#define ERROR_DIVZERO     "division by zero"
#define ERROR_EMPTY       "empty operation"
#define ERROR_ILLMACRO    "ill-formed syntactical form"
#define ERROR_MAP         "map over vectors of different sizes"
#define ERROR_MEMORY      "allocation error"
#define ERROR_NARGS       "wrong number of arguments"
#define ERROR_NIL         "expected non-nil vector"
#define ERROR_NOTBYTE     "expected byte; got "
#define ERROR_NOTENV      "expected environment; got "
#define ERROR_NOTFIT      "source object do not fit destination"
#define ERROR_NOTINT      "expected integer; got "
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
#define ERROR_VARMACRO    "macro used as variable: "
#define ERROR_VOID        "expression evaluated to nothing; expected value"

#define MACRO_SPECIALS                                              \
	/* SYMBOL               ENUM            NARGS   VARIADIC */ \
	X("do",                 BLTIN_DO,       0,      true       )\
	X("if",                 BLTIN_IF,       2,      true       )\
	X("let",                BLTIN_LET,      1,      true       )

#define PROCEDURE_SPECIALS                                          \
	/* SYMBOL               ENUM            NARGS   VARIADIC */ \
	X("apply",              BLTIN_APPLY,    2,      true       )\
	X("eval",               BLTIN_EVAL,     2,      false      )

#define MACRO_ROUTINES                                              \
	/* SYMBOL               FUNCTION        NARGS   VARIADIC */ \
	X("and",                f_and,          0,      true       )\
	X("define",             f_define,       2,      false      )\
	X("defmacro",           f_defmacro,     2,      true       )\
	X("defmacro...",        f_vardefmacro,  3,      true       )\
	X("defun",              f_defun,        2,      true       )\
	X("defun...",           f_vardefun,     3,      true       )\
	X("false",              f_false,        0,      false      )\
	X("lambda",             f_lambda,       1,      true       )\
	X("lambda...",          f_varlambda,    2,      true       )\
	X("or",                 f_or,           0,      true       )\
	X("quote",              f_quote,        1,      false      )\
	X("quasiquote",         f_quasiquote,   1,      false      )\
	X("redefine",           f_redefine,     2,      false      )\
	X("true",               f_true,         0,      false      )

#define PROCEDURE_ROUTINES                                          \
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
	X("current-environment",f_envcur,       0,      false      )\
	X("copy!",              f_vectorcpy,    2,      false      )\
	X("display",            f_display,      1,      true       )\
	X("empty?",             f_emptyp,       1,      false      )\
	X("environment",        f_envnew,       0,      true       )\
	X("environment?",       f_envp,         1,      false      )\
	X("eof?",               f_eofp,         1,      false      )\
	X("equiv?",             f_vectoreqv,    0,      true       )\
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
	X("vector?",            f_vectorp,      1,      false      )\
	X("write",              f_write,        1,      true       )

#define AUXILIARY_SYNTAX                                            \
	/* SYMBOL               ENUM                             */ \
	X("splice",             AUX_SPLICE                         )\
	X("unquote",            AUX_UNQUOTE                        )

enum {
#define X(s, e) e,
	AUXILIARY_SYNTAX
#undef  X
	NAUXILIARIES
};

typedef struct Eval {
	Simp ctx;
	Simp env;
	Simp aux[NAUXILIARIES];
	Simp iport, oport, eport;
	jmp_buf jmp;
} Eval;

struct Builtin {
	/* data for builtin procedure or builtin macro */
	unsigned char *name;
	enum {
		BLTIN_ROUTINE,
#define X(s, e, a, v) e,
		PROCEDURE_SPECIALS
		MACRO_SPECIALS
#undef  X
	} type;
	bool variadic;
	SimpSiz nargs;
	SimpSiz namelen;

	/* function is only used if type is BLTIN_ROUTINE; null otherwise */
	void (*fun)(Eval *, Simp *, Simp, Simp, Simp, Simp);
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
memerror(Eval *eval)
{
	error(eval, simp_nil(), simp_void(), simp_void(), ERROR_MEMORY);
}

static bool
syntaxget(Simp *macro, Simp env, Simp sym)
{
	Simp bind, var;

	for (; !simp_isnulenv(env); env = simp_getenvparent(env)) {
		for (bind = simp_getenvsynframe(env);
		     !simp_isnil(bind);
		     bind = simp_getnextbind(bind)) {
			var = simp_getbindvariable(bind);
			if (!simp_issame(var, sym))
				continue;
			if (macro != NULL)
				*macro = simp_getbindvalue(bind);
			return true;
		}
	}
	return false;
}

static Simp
envget(Eval *eval, Simp expr, Simp mainenv, Simp sym)
{
	enum { SYNTAX_ENV, NORMAL_ENV, LAST_ENV } i;
	Simp bind, var, env;
	const char *errstr = ERROR_VARMACRO;

	for (i = 0; i < LAST_ENV; i++) {
		for (env = mainenv;
		     !simp_isnulenv(env);
		     env = simp_getenvparent(env)) {
			if (i == SYNTAX_ENV)
				bind = simp_getenvsynframe(env);
			else
				bind = simp_getenvframe(env);
			for (;
			     !simp_isnil(bind);
			     bind = simp_getnextbind(bind)) {
				var = simp_getbindvariable(bind);
				if (!simp_issame(var, sym))
					continue;
				if (i == SYNTAX_ENV)
					goto syntax_error;
				return simp_getbindvalue(bind);
			}
		}
	}
	errstr = ERROR_UNBOUND;
syntax_error:
	error(eval, expr, simp_void(), sym, errstr);
	abort();
}

static void
envset(Eval *eval, Simp expr, Simp env, Simp var, Simp val, bool syntax)
{
	if (!syntax && syntaxget(NULL, env, var))
		error(eval, expr, simp_void(), var, ERROR_VARMACRO);
	for (; !simp_isnulenv(env); env = simp_getenvparent(env))
		if (simp_envredefine(env, var, val, syntax))
			return;
	error(eval, expr, simp_void(), var, ERROR_UNBOUND);
}

static void
envdef(Eval *eval, Simp expr, Simp env, Simp var, Simp val, bool syntax)
{
	if (!syntax && syntaxget(NULL, env, var))
		error(eval, expr, simp_void(), var, ERROR_VARMACRO);
	if (simp_envredefine(env, var, val, syntax))
		return;
	if (!simp_envdefine(eval->ctx, env, var, val, syntax))
		memerror(eval);
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
f_abs(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp obj;

	(void)env;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isnum(obj))
		error(eval, expr, self, obj, ERROR_NOTNUM);
	if (!simp_arithabs(eval->ctx, ret, obj))
		memerror(eval);
}

static void
f_add(Eval *eval, Simp *sum, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp obj;

	(void)env;
	nargs = simp_getsize(args);
	if (!simp_makesignum(eval->ctx, sum, 0))
		memerror(eval);
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			error(eval, expr, self, obj, ERROR_NOTNUM);
		if (!simp_arithadd(eval->ctx, sum, *sum, obj))
			memerror(eval);
	}
}

static void
f_and(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;

	nargs = simp_getsize(args);
	*ret = simp_true();
	for (i = 0; i < nargs; i++) {
		*ret = simp_getvectormemb(args, i);
		*ret = simp_eval(eval, *ret, env);
		if (simp_isvoid(*ret))
			error(eval, expr, self, simp_void(), ERROR_VOID);
		if (simp_isfalse(*ret)) {
			break;
		}
	}
}

static void
f_auxiliary(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)env;
	(void)ret;
	(void)args;
	error(eval, expr, simp_void(), self, ERROR_AUXILIARY);
}

static void
f_bytep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)expr;
	(void)self;
	(void)env;
	typepred(args, ret, simp_isbyte);
}

static void
f_booleanp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)expr;
	(void)self;
	(void)env;
	typepred(args, ret, simp_isbool);
}

static void
f_car(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz size;
	Simp obj;

	(void)eval;
	(void)env;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		error(eval, expr, self, obj, ERROR_NOTINT);
	size = simp_getsize(obj);
	if (size < 1)
		error(eval, expr, self, simp_nil(), ERROR_NIL);
	*ret = simp_getvectormemb(obj, 0);
}

static void
f_cdr(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz size;
	Simp obj;

	(void)eval;
	(void)env;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		error(eval, expr, self, obj, ERROR_NOTVECTOR);
	size = simp_getsize(obj);
	if (size < 1)
		error(eval, expr, self, simp_nil(), ERROR_NIL);
	*ret = simp_slicevector(obj, 1, size - 1);
}

static void
f_stdin(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)ret;
	(void)self;
	(void)expr;
	(void)args;
	(void)env;
	*ret = eval->iport;
}

static void
f_stdout(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)ret;
	(void)self;
	(void)expr;
	(void)args;
	(void)env;
	*ret = eval->oport;
}

static void
f_stderr(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)ret;
	(void)self;
	(void)expr;
	(void)args;
	(void)env;
	*ret = eval->eport;
}

static void
f_define(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp var, val;

	(void)ret;
	var = simp_getvectormemb(args, 0);
	val = simp_getvectormemb(args, 1);
	if (!simp_issymbol(var))
		error(eval, expr, self, var, ERROR_NOTSYM);
	val = simp_eval(eval, val, env);
	envdef(eval, expr, env, var, val, false);
}

static void
f_defmacro(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp var, val, body;
	SimpSiz nargs, i;

	(void)ret;
	nargs = simp_getsize(args);
	var = simp_getvectormemb(args, 0);
	body = simp_getvectormemb(args, nargs - 1);
	if (!simp_issymbol(var))
		error(eval, expr, self, var, ERROR_NOTSYM);
	nargs -= 2;
	args = simp_slicevector(args, 1, nargs);
	for (i = 0; i < nargs; i++) {
		val = simp_getvectormemb(args, i);
		if (!simp_issymbol(val)) {
			error(eval, expr, self, val, ERROR_NOTSYM);
		}
	}
	if (!simp_makeclosure(eval->ctx, &val, expr, env, args, simp_nil(), body))
		memerror(eval);
	envdef(eval, expr, env, var, val, true);
}

static void
f_defun(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp var, val, body;
	SimpSiz nargs, i;

	(void)ret;
	nargs = simp_getsize(args);
	var = simp_getvectormemb(args, 0);
	body = simp_getvectormemb(args, nargs - 1);
	if (!simp_issymbol(var))
		error(eval, expr, self, var, ERROR_NOTSYM);
	nargs -= 2;
	args = simp_slicevector(args, 1, nargs);
	for (i = 0; i < nargs; i++) {
		val = simp_getvectormemb(args, i);
		if (!simp_issymbol(val)) {
			error(eval, expr, self, val, ERROR_NOTSYM);
		}
	}
	if (!simp_makeclosure(eval->ctx, &val, expr, env, args, simp_nil(), body))
		memerror(eval);
	envdef(eval, expr, env, var, val, false);
}

static void
f_display(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp obj, port;

	(void)env;
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
f_divide(Eval *eval, Simp *ratio, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp obj;

	(void)env;
	nargs = simp_getsize(args);
	if (!simp_makesignum(eval->ctx, ratio, 1))
		memerror(eval);
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			error(eval, expr, self, obj, ERROR_NOTNUM);
		if (nargs > 1 && i == 0) {
			*ratio = obj;
		} else if (!simp_arithzero(obj)) {
			if (!simp_arithdiv(
				eval->ctx,
				ratio,
				*ratio,
				obj
			)) {
				memerror(eval);
			}
		} else {
			error(eval, expr, self, simp_void(), ERROR_DIVZERO);
		}
	}
}

static void
f_emptyp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)expr;
	(void)self;
	(void)env;
	typepred(args, ret, simp_isempty);
}

static void
f_envcur(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)expr;
	(void)self;
	(void)args;
	*ret = env;
}

static void
f_envp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)expr;
	(void)self;
	(void)env;
	typepred(args, ret, simp_isenvironment);
}

static void
f_envnew(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
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
f_eofp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_iseof);
}

static void
f_equal(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	(void)env;
	nargs = simp_getsize(args);
	*ret = simp_true();
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_arithcmp(prev, next) != 0) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_false(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	(void)args;
	*ret = simp_false();
}

static void
f_falsep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_isfalse);
}

static void
f_foreach(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp prod, obj;
	SimpSiz i, j, n, size, nargs;

	(void)env;
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
f_foreachstring(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp prod, obj;
	SimpSiz i, j, n, size, nargs;
	unsigned char byte;

	(void)env;
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
f_ge(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)env;
	(void)eval;
	nargs = simp_getsize(args);
	*ret = simp_true();
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_arithcmp(prev, next) < 0) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_gt(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)env;
	(void)eval;
	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_arithcmp(prev, next) <= 0) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_lambda(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp body, var;

	nargs = simp_getsize(args);
	body = simp_getvectormemb(args, nargs - 1);
	args = simp_slicevector(args, 0, --nargs);
	for (i = 0; i < nargs; i++) {
		var = simp_getvectormemb(args, i);
		if (!simp_issymbol(var)) {
			error(eval, expr, self, var, ERROR_NOTSYM);
		}
	}
	if (!simp_makeclosure(eval->ctx, ret, expr, env, args, simp_nil(), body))
		memerror(eval);
}

static void
f_le(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	(void)env;
	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_arithcmp(prev, next) > 0) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_lt(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	(void)env;
	*ret = simp_true();
	nargs = simp_getsize(args);
	for (i = 0; i < nargs; i++, prev = next) {
		next = simp_getvectormemb(args, i);
		if (!simp_isnum(next))
			error(eval, expr, self, next, ERROR_NOTNUM);
		if (i == 0)
			continue;
		if (simp_arithcmp(prev, next) >= 0) {
			*ret = simp_false();
			break;
		}
	}
}

static void
f_makestring(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpInt size;
	Simp obj;

	(void)env;
	obj = simp_getvectormemb(args, 0);
	if (!simp_issignum(obj))
		error(eval, expr, self, obj, ERROR_NOTINT);
	size = simp_getsignum(obj);
	if (size < 0)
		error(eval, expr, self, obj, ERROR_RANGE);
	if (!simp_makestring(eval->ctx, ret, NULL, size))
		memerror(eval);
}

static void
f_makevector(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpInt size;
	Simp obj;

	(void)env;
	obj = simp_getvectormemb(args, 0);
	if (!simp_issignum(obj))
		error(eval, expr, self, obj, ERROR_NOTINT);
	size = simp_getsignum(obj);
	if (size < 0)
		error(eval, expr, self, obj, ERROR_RANGE);
	if (!simp_makevector(eval->ctx, ret, size))
		memerror(eval);
}

static void
f_map(Eval *eval, Simp *vector, Simp self, Simp expr, Simp env, Simp args)
{
	Simp prod, obj;
	SimpSiz i, j, n, size, nargs;

	(void)env;
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
f_mapstring(Eval *eval, Simp *string, Simp self, Simp expr, Simp env, Simp args)
{
	Simp newexpr, prod, obj;
	SimpSiz i, j, n, size, nargs;
	unsigned char byte;

	(void)env;
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
f_member(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp newexpr, pred, ref, obj, vector;
	SimpSiz i, size;

	(void)env;
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
f_multiply(Eval *eval, Simp *prod, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp obj;

	(void)env;
	nargs = simp_getsize(args);
	if (!simp_makesignum(eval->ctx, prod, 1))
		memerror(eval);
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			error(eval, expr, self, obj, ERROR_NOTINT);
		if (!simp_arithmul(eval->ctx, prod, *prod, obj))
			memerror(eval);
	}
}

static void
f_newline(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp port;

	(void)ret;
	(void)env;
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
f_not(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp obj;

	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	*ret = simp_false();
	obj = simp_getvectormemb(args, 0);
	if (simp_isfalse(obj))
		*ret = simp_true();
}

static void
f_nullp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_isnil);
}

static void
f_numberp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_issignum);
}

static void
f_or(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;

	nargs = simp_getsize(args);
	*ret = simp_true();
	for (i = 0; i < nargs; i++) {
		*ret = simp_getvectormemb(args, i);
		*ret = simp_eval(eval, *ret, env);
		if (simp_isvoid(*ret))
			error(eval, expr, self, simp_void(), ERROR_VOID);
		if (simp_istrue(*ret)) {
			break;
		}
	}
}

static void
f_portp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_isport);
}

static void
f_procedurep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_isprocedure);
}

static void
f_quote(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	*ret = simp_getvectormemb(args, 0);
}

static void
f_quasiquote(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp vect, fst, obj, oldvect;
	SimpSiz size, n, i, j;

	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	vect = simp_getvectormemb(args, 0);
	if (!simp_isvector(vect) || simp_isnil(vect)) {
		*ret = vect;
		return;
	}
	fst = simp_getvectormemb(vect, 0);
	size = simp_getsize(vect);
	if (simp_issame(fst, eval->aux[AUX_UNQUOTE])) {
		if (size != 2)
			error(eval, expr, self, simp_void(), ERROR_NARGS);
		obj = simp_getvectormemb(vect, 1);
		*ret = simp_eval(eval, obj, env);
		return;
	}
	if (!simp_makevector(eval->ctx, ret, size))
		memerror(eval);
	for (i = j = 0; i < size; j++) {
		obj = simp_getvectormemb(vect, j);
		if (simp_isvector(obj) && (n = simp_getsize(obj)) > 0 &&
		    simp_issame((fst = simp_getvectormemb(obj, 0)), eval->aux[AUX_SPLICE])) {
			if (n != 2)
				error(eval, expr, fst, simp_void(), ERROR_NARGS);
			args = simp_slicevector(obj, 1, 1);
			f_quasiquote(eval, &obj, self, expr, env, args);
			if (!simp_isvector(obj))
				error(eval, expr, fst, obj, ERROR_NOTVECTOR);
			n = simp_getsize(obj);
			oldvect = *ret;
			size--;
			if (!simp_makevector(eval->ctx, ret, size + n))
				memerror(eval);
			simp_cpyvector(*ret, oldvect);
			simp_cpyvector(simp_slicevector(*ret, i, n), obj);
			size += n;
			i += n;
		} else {
			args = simp_slicevector(vect, j, 1);
			f_quasiquote(eval, &obj, self, expr, env, args);
			simp_setvector(*ret, i++, obj);
		}
	}
}

static void
f_read(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp port;

	(void)env;
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
f_redefine(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp var, val;

	(void)ret;
	var = simp_getvectormemb(args, 0);
	val = simp_getvectormemb(args, 1);
	if (!simp_issymbol(var))
		error(eval, expr, self, var, ERROR_NOTSYM);
	val = simp_eval(eval, val, env);
	envset(eval, expr, env, var, val, false);
}

static void
f_remainder(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpInt d;
	Simp a, b;

	(void)env;
	a = simp_getvectormemb(args, 0);
	b = simp_getvectormemb(args, 1);
	if (!simp_issignum(a))
		error(eval, expr, self, a, ERROR_NOTINT);
	if (!simp_issignum(b))
		error(eval, expr, self, b, ERROR_NOTINT);
	d = simp_getsignum(b);
	if (d == 0)
		error(eval, expr, self, simp_void(), ERROR_DIVZERO);
	d = simp_getsignum(a) % d;
	if (!simp_makesignum(eval->ctx, ret, d))
		memerror(eval);
}

static void
f_samep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
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
f_slicevector(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp vector, obj;
	SimpSiz nargs, from, size, capacity;

	(void)eval;
	(void)env;
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
		if (!simp_issignum(obj)) {
			error(eval, expr, self, obj, ERROR_NOTINT);
		}
		from = simp_getsignum(obj);
		if (from < 0 || from > capacity) {
			error(eval, expr, self, obj, ERROR_RANGE);
		}
	}
	size = capacity - from;
	if (nargs > 2) {
		obj = simp_getvectormemb(args, 2);
		if (!simp_issignum(obj)) {
			error(eval, expr, self, obj, ERROR_NOTINT);
		}
		size = simp_getsignum(obj);
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
f_slicestring(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp string, obj;
	SimpSiz nargs, from, size, capacity;

	(void)eval;
	(void)env;
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
		if (!simp_issignum(obj)) {
			error(eval, expr, self, obj, ERROR_NOTINT);
		}
		from = simp_getsignum(obj);
		if (from < 0 || from > capacity) {
			error(eval, expr, self, obj, ERROR_RANGE);
		}
	}
	if (nargs > 2) {
		obj = simp_getvectormemb(args, 2);
		if (!simp_issignum(obj)) {
			error(eval, expr, self, obj, ERROR_NOTINT);
		}
		size = simp_getsignum(obj);
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
f_subtract(Eval *eval, Simp *diff, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp obj;

	(void)env;
	nargs = simp_getsize(args);
	if (!simp_makesignum(eval->ctx, diff, 0))
		memerror(eval);
	for (i = 0; i < nargs; i++) {
		obj = simp_getvectormemb(args, i);
		if (!simp_isnum(obj))
			error(eval, expr, self, obj, ERROR_NOTNUM);
		if (nargs == 1 || i > 0) {
			if (!simp_arithdiff(eval->ctx, diff, *diff, obj)) {
				memerror(eval);
			}
		} else {
			*diff = obj;
		}
	}
}

static void
f_string(Eval *eval, Simp *string, Simp self, Simp expr, Simp env, Simp args)
{
	Simp obj;
	SimpSiz nargs, i;

	(void)env;
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
f_stringcat(Eval *eval, Simp *string, Simp self, Simp expr, Simp env, Simp args)
{
	Simp obj;
	SimpSiz nargs, size, n, i;

	(void)env;
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
f_stringcpy(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp dst, src;
	SimpSiz dstsiz, srcsiz;

	(void)ret;
	(void)env;
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
f_stringdup(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp obj;
	SimpSiz len;

	(void)env;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isstring(obj))
		error(eval, expr, self, obj, ERROR_NOTSTRING);
	len = simp_getsize(obj);
	if (!simp_makestring(eval->ctx, ret, simp_getstring(obj), len))
		memerror(eval);
}

static void
f_stringge(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	*ret = simp_true();
	(void)env;
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
f_stringgt(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)env;
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
f_stringle(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)env;
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
f_stringlt(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp next, prev;

	(void)env;
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
f_stringlen(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpInt size;
	Simp obj;

	(void)env;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isstring(obj))
		error(eval, expr, self, obj, ERROR_NOTSTRING);
	size = simp_getsize(obj);
	if (!simp_makesignum(eval->ctx, ret, size))
		memerror(eval);
}

static void
f_stringref(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz size;
	SimpInt pos;
	Simp a, b;
	unsigned u;

	(void)env;
	a = simp_getvectormemb(args, 0);
	b = simp_getvectormemb(args, 1);
	if (!simp_isstring(a))
		error(eval, expr, self, a, ERROR_NOTSTRING);
	if (!simp_issignum(b))
		error(eval, expr, self, b, ERROR_NOTINT);
	size = simp_getsize(a);
	pos = simp_getsignum(b);
	if (pos < 0 || pos >= (SimpInt)size)
		error(eval, expr, self, b, ERROR_RANGE);
	u = simp_getstringmemb(a, pos);
	if (!simp_makebyte(eval->ctx, ret, u))
		memerror(eval);
}

static void
f_stringp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_isstring);
}

static void
f_stringvector(Eval *eval, Simp *vector, Simp self, Simp expr, Simp env, Simp args)
{
	Simp str, byte;
	SimpSiz i, size;
	unsigned char u;

	(void)env;
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
f_stringset(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp str, pos, val;
	SimpSiz size;
	SimpInt n;
	unsigned char u;

	(void)ret;
	(void)env;
	str = simp_getvectormemb(args, 0);
	pos = simp_getvectormemb(args, 1);
	val = simp_getvectormemb(args, 2);
	if (!simp_isstring(str))
		error(eval, expr, self, str, ERROR_NOTSTRING);
	if (!simp_issignum(pos))
		error(eval, expr, self, pos, ERROR_NOTINT);
	if (!simp_isbyte(val))
		error(eval, expr, self, val, ERROR_NOTBYTE);
	size = simp_getsize(str);
	n = simp_getsignum(pos);
	u = simp_getbyte(val);
	if (n < 0 || n >= (SimpInt)size)
		error(eval, expr, self, pos, ERROR_RANGE);
	simp_setstring(str, n, u);
}

static void
f_true(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	(void)args;
	*ret = simp_true();
}

static void
f_truep(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_istrue);
}

static void
f_symbolp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_issymbol);
}

static void
f_vardefmacro(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp var, val, body, extra;
	SimpSiz nargs, i;

	(void)ret;
	nargs = simp_getsize(args);
	var = simp_getvectormemb(args, 0);
	body = simp_getvectormemb(args, nargs - 1);
	extra = simp_getvectormemb(args, nargs - 2);
	if (!simp_issymbol(var))
		error(eval, expr, self, var, ERROR_NOTSYM);
	args = simp_slicevector(args, 1, nargs - 3);
	nargs -= 2;
	for (i = 0; i < nargs; i++) {
		val = simp_getvectormemb(args, i);
		if (!simp_issymbol(val)) {
			error(eval, expr, self, val, ERROR_NOTSYM);
		}
	}
	if (!simp_makeclosure(eval->ctx, &val, expr, env, args, extra, body))
		memerror(eval);
	envdef(eval, expr, env, var, val, true);
}

static void
f_vardefun(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp var, val, body, extra;
	SimpSiz nargs, i;

	(void)ret;
	nargs = simp_getsize(args);
	var = simp_getvectormemb(args, 0);
	body = simp_getvectormemb(args, nargs - 1);
	extra = simp_getvectormemb(args, nargs - 2);
	if (!simp_issymbol(var))
		error(eval, expr, self, var, ERROR_NOTSYM);
	args = simp_slicevector(args, 1, nargs - 3);
	nargs -= 2;
	for (i = 0; i < nargs; i++) {
		val = simp_getvectormemb(args, i);
		if (!simp_issymbol(val)) {
			error(eval, expr, self, val, ERROR_NOTSYM);
		}
	}
	if (!simp_makeclosure(eval->ctx, &val, expr, env, args, extra, body))
		memerror(eval);
	envdef(eval, expr, env, var, val, false);
}

static void
f_varlambda(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz nargs, i;
	Simp extra, body, var;

	nargs = simp_getsize(args);
	body = simp_getvectormemb(args, nargs - 1);
	extra = simp_getvectormemb(args, nargs - 2);
	args = simp_slicevector(args, 0, nargs - 2);
	nargs--;
	for (i = 0; i < nargs; i++) {
		var = simp_getvectormemb(args, i);
		if (!simp_issymbol(var)) {
			error(eval, expr, self, var, ERROR_NOTSYM);
		}
	}
	if (!simp_makeclosure(eval->ctx, ret, expr, env, args, extra, body))
		memerror(eval);
}

static void
f_vector(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)ret;
	(void)env;
	*ret = args;
}

static void
f_vectorcat(Eval *eval, Simp *vector, Simp self, Simp expr, Simp env, Simp args)
{
	Simp obj;
	SimpSiz nargs, size, n, i;

	(void)env;
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
f_vectorcpy(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp dst, src;
	SimpSiz dstsiz, srcsiz;

	(void)ret;
	(void)env;
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
f_vectordup(Eval *eval, Simp *dst, Simp self, Simp expr, Simp env, Simp args)
{
	Simp src;
	SimpSiz i, len;

	(void)env;
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
f_vectoreqv(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp a, b;
	Simp next, prev;
	SimpSiz nargs, newsize, oldsize, i, j;

	(void)eval;
	(void)env;
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
f_vectorp(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	(void)eval;
	(void)self;
	(void)expr;
	(void)env;
	typepred(args, ret, simp_isvector);
}

static void
f_vectorset(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp vector, pos, val;
	SimpSiz size;
	SimpInt n;

	(void)ret;
	(void)env;
	vector = simp_getvectormemb(args, 0);
	pos = simp_getvectormemb(args, 1);
	val = simp_getvectormemb(args, 2);
	if (!simp_isvector(vector))
		error(eval, expr, self, vector, ERROR_NOTVECTOR);
	if (!simp_issignum(pos))
		error(eval, expr, self, pos, ERROR_NOTINT);
	size = simp_getsize(vector);
	n = simp_getsignum(pos);
	if (n < 0 || n >= (SimpInt)size)
		error(eval, expr, self, pos, ERROR_RANGE);
	simp_setvector(vector, n, val);
}

static void
f_vectorlen(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpInt size;
	Simp obj;

	(void)env;
	obj = simp_getvectormemb(args, 0);
	if (!simp_isvector(obj))
		error(eval, expr, self, obj, ERROR_NOTVECTOR);
	size = simp_getsize(obj);
	if (!simp_makesignum(eval->ctx, ret, size))
		memerror(eval);
}

static void
f_vectorref(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz size;
	SimpInt pos;
	Simp a, b;

	(void)eval;
	(void)env;
	a = simp_getvectormemb(args, 0);
	b = simp_getvectormemb(args, 1);
	if (!simp_isvector(a))
		error(eval, expr, self, a, ERROR_NOTVECTOR);
	if (!simp_issignum(b))
		error(eval, expr, self, b, ERROR_NOTINT);
	size = simp_getsize(a);
	pos = simp_getsignum(b);
	if (pos < 0 || pos >= (SimpInt)size)
		error(eval, expr, self, b, ERROR_RANGE);
	*ret = simp_getvectormemb(a, pos);
}

static void
f_vectorrev(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz i, n, size;
	Simp obj, beg, end;

	(void)eval;
	(void)env;
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
f_vectorrevnew(Eval *eval, Simp *vector, Simp self, Simp expr, Simp env, Simp args)
{
	SimpSiz i, n, size;
	Simp obj, beg, end;

	(void)env;
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
f_write(Eval *eval, Simp *ret, Simp self, Simp expr, Simp env, Simp args)
{
	Simp obj, port;

	(void)ret;
	(void)env;
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

bool
simp_environmentnew(Simp ctx, Simp *env)
{
	static Builtin funcs[] = {
#define X(s, p, a, v) { \
	.type = BLTIN_ROUTINE, \
	.name = (unsigned char *)s, \
	.fun = &p, \
	.nargs = a, \
	.variadic = v, \
	.namelen = sizeof(s)-1 },
		PROCEDURE_ROUTINES
#undef  X

#define X(s, e, a, v) { \
	.type = e, \
	.name = (unsigned char *)s, \
	.fun = NULL, \
	.nargs = a, \
	.variadic = v, \
	.namelen = sizeof(s)-1 },
		PROCEDURE_SPECIALS
#undef  X
	};
	static Builtin macros[] = {
#define X(s, p, a, v) { \
	.type = BLTIN_ROUTINE, \
	.name = (unsigned char *)s, \
	.fun = &p, \
	.nargs = a, \
	.variadic = v, \
	.namelen = sizeof(s)-1 },
		MACRO_ROUTINES
#undef  X

#define X(s, e, a, v) { \
	.type = e, \
	.name = (unsigned char *)s, \
	.fun = NULL, \
	.nargs = a, \
	.variadic = v, \
	.namelen = sizeof(s)-1 },
		MACRO_SPECIALS
#undef  X

#define X(s, e) { \
	.type = BLTIN_ROUTINE, \
	.name = (unsigned char *)s, \
	.fun = &f_auxiliary, \
	.nargs = 0, \
	.variadic = true, \
	.namelen = sizeof(s)-1 },
		AUXILIARY_SYNTAX
#undef  X
	};
	Simp var, val;
	SimpSiz i;

	if (!simp_makeenvironment(ctx, env, simp_nulenv()))
		return false;
	for (i = 0; i < LEN(macros); i++) {
		/* fill initial environment with builtin macros */
		if (!simp_makesymbol(ctx, &var, macros[i].name, macros[i].namelen))
			return false;
		if (!simp_makebuiltin(ctx, &val, &macros[i]))
			return false;
		if (!simp_envdefine(ctx, *env, var, val, true))
			return false;
	}
	for (i = 0; i < LEN(funcs); i++) {
		/* fill initial environment with builtin procedures */
		if (!simp_makesymbol(ctx, &var, funcs[i].name, funcs[i].namelen))
			return false;
		if (!simp_makebuiltin(ctx, &val, &funcs[i]))
			return false;
		if (!simp_envdefine(ctx, *env, var, val, false))
			return false;
	}
	return true;
}

static Simp
simp_eval(Eval *eval, Simp expr, Simp env)
{
	Builtin *bltin;
	Simp operator, operands, arguments, extraargs, extraparams;
	Simp proc, macro, params, var, val;
	SimpSiz noperands, nparams, narguments, nextraargs, i;
	bool ismacro;

loop:
	ismacro = false;
	if (simp_issymbol(expr))   /* expression is variable */
		return envget(eval, expr, env, expr);
	if (!simp_isvector(expr))  /* expression is self-evaluating */
		return expr;
	if ((noperands = simp_getsize(expr)) == 0)
		error(eval, expr, simp_void(), simp_void(), ERROR_EMPTY);
	noperands--;
	operator = simp_getvectormemb(expr, 0);
	operands = simp_slicevector(expr, 1, noperands);
	extraargs = simp_nil();
	nextraargs = 0;
	ismacro = syntaxget(&macro, env, operator);
	if (ismacro) {
		/* operator is a macro; do not evaluate operands */
		proc = macro;
		arguments = operands;
		narguments = noperands;
	} else {
		/* operator is a not a macro; evaluate the operands */
		proc = simp_eval(eval, operator, env);
apply:
		if (simp_isvoid(proc))
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
	}
	if (simp_isclosure(proc)) {
		expr = simp_getclosurebody(proc);
		if (!ismacro &&
		    !simp_makeenvironment(eval->ctx, &env, simp_getclosureenv(proc)))
			memerror(eval);
		params = simp_getclosureparam(proc);
		nparams = simp_getsize(params);
		extraparams = simp_getclosurevarargs(proc);
		if (narguments < nparams)
			error(eval, expr, operator, simp_void(), ERROR_NARGS);
		if (narguments > nparams && simp_isnil(extraparams))
			error(eval, expr, operator, simp_void(), ERROR_NARGS);
		for (i = 0; i < nparams; i++) {
			var = simp_getvectormemb(params, i);
			val = simp_getvectormemb(arguments, i);
			envdef(eval, expr, env, var, val, false);
		}
		if (simp_issymbol(extraparams)) {
			val = simp_slicevector(arguments, i, narguments - i);
			envdef(eval, expr, env, extraparams, val, false);
		}
		if (ismacro)
			expr = simp_eval(eval, expr, env);
		goto loop;
	}
	if (!simp_isbuiltin(proc))
		error(eval, expr, simp_void(), operator, ERROR_NOTPROC);
	bltin = simp_getbuiltin(proc);
	if (bltin->variadic && narguments < bltin->nargs)
		error(eval, expr, operator, simp_void(), ERROR_NARGS);
	if (!bltin->variadic && narguments != bltin->nargs)
		error(eval, expr, operator, simp_void(), ERROR_NARGS);
	switch (bltin->type) {
	case BLTIN_APPLY:
		extraargs = simp_getvectormemb(arguments, narguments - 1);
		if (!simp_isvector(extraargs))
			error(eval, expr, operator, extraargs, ERROR_NOTVECTOR);
		proc = simp_getvectormemb(arguments, 0);
		nextraargs = simp_getsize(extraargs);
		operands = simp_slicevector(arguments, 1, narguments - 2);
		noperands = narguments - 2;
		goto apply;
	case BLTIN_DO:
		/* (do EXPRESSION ...) */
		if (noperands == 0)
			return simp_void();
		for (i = 0; i + 1 < noperands; i++) {
			val = simp_getvectormemb(operands, i);
			val = simp_eval(eval, val, env);
		}
		expr = simp_getvectormemb(operands, i);
		goto loop;
	case BLTIN_EVAL:
		expr = simp_getvectormemb(arguments, 0);
		env = simp_getvectormemb(arguments, 1);
		goto loop;
	case BLTIN_IF:
		/* (if [COND THEN]... [ELSE]) */
		if (noperands < 2)
			error(eval, expr, operator, simp_void(), ERROR_ILLMACRO);
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
	case BLTIN_LET:
		if (noperands % 2 == 0)
			error(eval, expr, operator, simp_void(), ERROR_ILLMACRO);
		if (!simp_makeenvironment(eval->ctx, &env, env))
			memerror(eval);
		for (i = 0; i + 1 < noperands; i += 2) {
			var = simp_getvectormemb(operands, i);
			if (!simp_issymbol(var))
				error(eval, expr, operator, var, ERROR_NOTSYM);
			val = simp_getvectormemb(operands, i + 1);
			val = simp_eval(eval, val, env);
			envdef(eval, expr, env, var, val, false);
		}
		expr = simp_getvectormemb(operands, noperands - 1);
		goto loop;
	case BLTIN_ROUTINE:
		val = simp_void();
		if (!simp_makesymbol(eval->ctx, &var, bltin->name, bltin->namelen))
			memerror(eval);
		(*bltin->fun)(eval, &val, var, expr, env, arguments);
		return val;
	}
	/* UNREACHABLE */
	abort();
}

bool
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
	bool retval = false;

#define X(s, e) if(!simp_makesymbol(ctx, &eval.aux[e], (unsigned char *)s, sizeof(s)-1)) goto error;
	AUXILIARY_SYNTAX
#undef  X
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
	retval = true;
error:
	simp_gc(ctx, gcignore, LEN(gcignore));
	return retval;
}
