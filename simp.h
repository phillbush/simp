#include <stdbool.h>

#define LEN(a)          (sizeof(a) / sizeof((a)[0]))
#define FLAG(f, b)      (((f) & (b)) == (b))
#define MASK(f, m, b)   (((f) & (m)) == (b))
#define RETURN_FAILURE  (-1)
#define RETURN_SUCCESS  0
#define NOTHING         (-1)

#define EXCEPTIONS                                                       \
	X(ERROR_ARGS,           "wrong number of arguments"             )\
	X(ERROR_EMPTY,          "empty operation"                       )\
	X(ERROR_ENVIRON,        "symbol for environment not supplied"   )\
	X(ERROR_UNBOUND,        "unbound variable"                      )\
	X(ERROR_OPERATOR,       "operator is not a procedure"           )\
	X(ERROR_OUTOFRANGE,     "out of range"                          )\
	X(ERROR_ILLEXPR,        "ill expression"                        )\
	X(ERROR_ILLTYPE,        "improper type"                         )\
	X(ERROR_NOTSYM,         "not a symbol"                          )\
	X(ERROR_STREAM,         "stream error"                          )\
	X(ERROR_RANGE,          "out of range"                          )\
	X(ERROR_MEMORY,         "allocation error"                      )

#define OPERATIONS                                       \
	X(OP_DEFINE,            "define"                )\
	X(OP_EVAL,              "eval"                  )\
	X(OP_IF,                "if"                    )\
	X(OP_MACRO,             "macro"                 )\
	X(OP_LAMBDA,            "lambda"                )\
	X(OP_SET,               "set!"                  )\
	X(OP_WRAP,              "wrap"                  )

#define BUILTINS                                                          \
	X(F_BOOLEANP,     "boolean?",            f_booleanp,        1, 1 )\
	X(F_BYTEP,        "byte?",               f_bytep,           1, 1 )\
	X(F_CURIPORT,     "current-input-port",  f_curiport,        0, 0 )\
	X(F_CUROPORT,     "current-output-port", f_curoport,        0, 0 )\
	X(F_CUREPORT,     "current-error-port",  f_cureport,        0, 0 )\
	X(F_DISPLAY,      "display",             f_display,         1, 2 )\
	X(F_EQUAL,        "=",                   f_equal,           2, 2 )\
	X(F_FALSE,        "false",               f_false,           0, 0 )\
	X(F_FALSEP,       "falsep",              f_falsep,          1, 1 )\
	X(F_GT,           ">",                   f_gt,              2, 2 )\
	X(F_LT,           "<",                   f_lt,              2, 2 )\
	X(F_MAKESTRING,   "make-string",         f_makestring,      1, 2 )\
	X(F_MAKEVECTOR,   "make-vector",         f_makevector,      1, 2 )\
	X(F_MAKEENV,      "make-environment",    f_makeenvironment, 0, 1 )\
	X(F_NEWLINE,      "newline",             f_newline,         0, 1 )\
	X(F_NULLP,        "null?",               f_nullp,           1, 1 )\
	X(F_PORTP,        "port?",               f_portp,           1, 1 )\
	X(F_SAMEP,        "same?",               f_samep,           2, 2 )\
	X(F_STRINGCMP,    "string-compare",      f_stringcmp,       2, 2 )\
	X(F_STRINGLEN,    "string-length",       f_stringlen,       1, 1 )\
	X(F_STRINGREF,    "string-ref",          f_stringref,       2, 2 )\
	X(F_STRINGP,      "string?",             f_stringp,         1, 1 )\
	X(F_STRINGSET,    "string-set!",         f_stringset,       3, 3 )\
	X(F_STRINGVECTOR, "string->vector",      f_stringvector,    1, 1 )\
	X(F_SYMBOLP,      "symbol?",             f_symbolp,         1, 1 )\
	X(F_TRUE,         "true",                f_true,            0, 0 )\
	X(F_TRUEP,        "true?",               f_truep,           1, 1 )\
	X(F_VECTORREF,    "vector-ref",          f_vectorref,       2, 2 )\
	X(F_VECTORLEN,    "vector-length",       f_vectorlen,       1, 1 )\
	X(F_VECTORSET,    "vector-set!",         f_vectorset,       3, 3 )\
	X(F_VOID,         "void",                f_void,            0, 0 )\
	X(F_WRITE,        "write",               f_write,           1, 2 )

#define VARARGS                                          \
	X(F_ADD,      "+",                   f_add      )\
	X(F_DIVIDE,   "/",                   f_divide   )\
	X(F_MULTIPLY, "*",                   f_multiply )\
	X(F_SUBTRACT, "-",                   f_subtract )\
	X(F_VECTOR  , "vector",              f_vector )

#define TYPES                                              \
	/* Object type        Is vector   Is allocated   */\
	X(TYPE_APPLICATIVE,   true,       true            )\
	X(TYPE_BUILTIN,       false,      false           )\
	X(TYPE_VARARGS,       false,      false           )\
	X(TYPE_BYTE,          false,      false           )\
	X(TYPE_ENVIRONMENT,   true,       true            )\
	X(TYPE_EOF,           false,      false           )\
	X(TYPE_EXCEPTION,     false,      false           )\
	X(TYPE_FALSE,         false,      false           )\
	X(TYPE_OPERATIVE,     true,       true            )\
	X(TYPE_BINDING,       true,       true            )\
	X(TYPE_FORM,          false,      false           )\
	X(TYPE_PORT,          false,      false           )\
	X(TYPE_REAL,          false,      false           )\
	X(TYPE_SIGNUM,        false,      false           )\
	X(TYPE_STRING,        false,      true            )\
	X(TYPE_SYMBOL,        false,      true            )\
	X(TYPE_TRUE,          false,      false           )\
	X(TYPE_VECTOR,        true,       true            )\
	X(TYPE_VOID,          false,      false           )

typedef struct GC               GC;
typedef struct Vector           Vector;
typedef struct Simp             Simp;
typedef unsigned long long      SimpSiz;
typedef long long               SimpInt;

enum Exceptions {
#define X(n, s) n,
	EXCEPTIONS
	NEXCEPTIONS
#undef  X
};

enum Operations {
#define X(n, s) n,
	OPERATIONS
	NOPERATIONS
#undef  X
};

enum Builtins {
#define X(n, s, p, min, max) n,
	BUILTINS
	NBUILTINS
#undef  X
};

enum Varargs {
#define X(n, s, p) n,
	VARARGS
	NVARARGS
#undef  X
};

struct Simp {
	union {
		SimpInt         num;
		double          real;
		Vector         *vector;
		void           *string;
		unsigned char  *errmsg;
		unsigned char   byte;
		void           *port;
		enum Operations form;
		enum Builtins   builtin;
		enum Varargs    varargs;
	} u;
	enum Type {
#define X(n, v, h) n,
	TYPES
#undef  X
	} type;
};

/* error handling */
unsigned char *simp_errorstr(int exception);

/* data constant utils */
Simp    simp_nil(void);
Simp    simp_empty(void);
Simp    simp_eof(void);
Simp    simp_false(void);
Simp    simp_true(void);
Simp    simp_void(void);

/* data type accessors */
enum Operations simp_getform(Simp ctx, Simp obj);
enum Builtins simp_getbuiltin(Simp ctx, Simp obj);
enum Varargs simp_getvarargs(Simp ctx, Simp obj);
unsigned char simp_getbyte(Simp ctx, Simp obj);
SimpInt simp_getnum(Simp ctx, Simp obj);
void   *simp_getport(Simp ctx, Simp obj);
double  simp_getreal(Simp ctx, Simp obj);
SimpSiz   simp_getsize(Simp ctx, Simp obj);
unsigned char *simp_getstring(Simp ctx, Simp obj);
unsigned char *simp_getsymbol(Simp ctx, Simp obj);
unsigned char *simp_getexception(Simp ctx, Simp obj);
Simp    simp_getvectormemb(Simp ctx, Simp obj, SimpSiz pos);
enum Type simp_gettype(Simp ctx, Simp obj);
Simp   *simp_getvector(Simp ctx, Simp obj);
Simp    simp_getapplicativeenv(Simp ctx, Simp obj);
Simp    simp_getapplicativeparam(Simp ctx, Simp obj);
Simp    simp_getapplicativebody(Simp ctx, Simp obj);
Simp    simp_getoperativeenv(Simp ctx, Simp obj);
Simp    simp_getoperativeparam(Simp ctx, Simp obj);
Simp    simp_getoperativebody(Simp ctx, Simp obj);
Vector *simp_getgcmemory(Simp ctx, Simp obj);

/* data type predicates */
bool    simp_isapplicative(Simp ctx, Simp obj);
bool    simp_isbool(Simp ctx, Simp obj);
bool    simp_isbuiltin(Simp ctx, Simp obj);
bool    simp_isvarargs(Simp ctx, Simp obj);
bool    simp_isbyte(Simp ctx, Simp obj);
bool    simp_isempty(Simp ctx, Simp obj);
bool    simp_isenvironment(Simp ctx, Simp obj);
bool    simp_iseof(Simp ctx, Simp obj);
bool    simp_isexception(Simp ctx, Simp obj);
bool    simp_isfalse(Simp ctx, Simp obj);
bool    simp_isform(Simp ctx, Simp obj);
bool    simp_isnum(Simp ctx, Simp obj);
bool    simp_isnil(Simp ctx, Simp obj);
bool    simp_isoperative(Simp ctx, Simp obj);
bool    simp_ispair(Simp ctx, Simp obj);
bool    simp_ispair(Simp ctx, Simp obj);
bool    simp_isport(Simp ctx, Simp obj);
bool    simp_isreal(Simp ctx, Simp obj);
bool    simp_isstring(Simp ctx, Simp obj);
bool    simp_issymbol(Simp ctx, Simp obj);
bool    simp_istrue(Simp ctx, Simp obj);
bool    simp_isvector(Simp ctx, Simp obj);
bool    simp_isvoid(Simp ctx, Simp obj);

/* data type checkers */
bool    simp_issame(Simp ctx, Simp a, Simp b);

/* data type mutators */
Simp    simp_setstring(Simp ctx, Simp obj, Simp pos, Simp val);
Simp    simp_setvector(Simp ctx, Simp obj, Simp pos, Simp val);

/* data type constructors */
Simp    simp_makebyte(Simp ctx, unsigned char byte);
Simp    simp_makeform(Simp ctx, int);
Simp    simp_makebuiltin(Simp ctx, int);
Simp    simp_makevarargs(Simp ctx, int);
Simp    simp_makeapplicative(Simp ctx, Simp env, Simp param, Simp body);
Simp    simp_makeexception(Simp ctx, int n);
Simp    simp_makeenvironment(Simp ctx, Simp parent);
Simp    simp_makenum(Simp ctx, SimpInt n);
Simp    simp_makeoperative(Simp ctx, Simp env, Simp param, Simp body);
Simp    simp_makeport(Simp ctx, void *p);
Simp    simp_makereal(Simp ctx, double x);
Simp    simp_makestring(Simp ctx, unsigned char *src, SimpSiz size);
Simp    simp_makesymbol(Simp ctx, unsigned char *src, SimpSiz size);
Simp    simp_makevector(Simp ctx, SimpSiz size, Simp fill);

/* context operations */
Simp    simp_contextenvironment(Simp ctx);
Simp    simp_contextsymtab(Simp ctx);
Simp    simp_contextiport(Simp ctx);
Simp    simp_contextoport(Simp ctx);
Simp    simp_contexteport(Simp ctx);
Simp    simp_contextnew(void);

/* environment operations */
Simp    simp_envget(Simp ctx, Simp env, Simp sym);
Simp    simp_envdef(Simp ctx, Simp env, Simp var, Simp val);
Simp    simp_envset(Simp ctx, Simp env, Simp var, Simp val);

/* port operations */
Simp    simp_openstream(Simp ctx, void *p, char *mode);
Simp    simp_openstring(Simp ctx, unsigned char *p, SimpSiz len, char *mode);
int     simp_porteof(Simp ctx, Simp obj);
int     simp_porterr(Simp ctx, Simp obj);
int     simp_readbyte(Simp ctx, Simp port);
int     simp_peekbyte(Simp ctx, Simp port);
void    simp_unreadbyte(Simp ctx, Simp obj, int c);
Simp    simp_printf(Simp ctx, Simp obj, const char *fmt, ...);

/* eval */
Simp    simp_read(Simp ctx, Simp port);
Simp    simp_write(Simp ctx, Simp port, Simp obj);
Simp    simp_display(Simp ctx, Simp port, Simp obj);
Simp    simp_eval(Simp ctx, Simp expr, Simp env);

/* gc */
Vector *simp_gcnewarray(Simp ctx, SimpSiz nmembs, SimpSiz membsiz);
void   *simp_gcgetdata(Vector *vector);
SimpSiz simp_gcgetlength(Vector *vector);
void    simp_gc(Simp ctx);
void    simp_gcfree(Simp ctx);
