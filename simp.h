#include <stdbool.h>

#define LEN(a)          (sizeof(a) / sizeof((a)[0]))
#define FLAG(f, b)      (((f) & (b)) == (b))
#define MASK(f, m, b)   (((f) & (m)) == (b))
#define RETURN_FAILURE  (-1)
#define RETURN_SUCCESS  0
#define NOTHING         (-1)

#define EXCEPTIONS                                               \
	X(ERROR_ARGS,    "wrong number of arguments"            )\
	X(ERROR_ENVIRON, "symbol for environment not supplied"  )\
	X(ERROR_UNBOUND, "unbound variable"                     )\
	X(ERROR_ILLEXPR, "ill expression"                       )\
	X(ERROR_ILLTYPE, "improper type"                        )\
	X(ERROR_STREAM,  "stream error"                         )\
	X(ERROR_RANGE,   "out of range"                         )\
	X(ERROR_MEMORY,  "allocation error"                     )

#define OPERATIONS                                                       \
	X(OP_ADD,       "+",                    simp_opadd              )\
	X(OP_BOOLEANP,  "boolean?",             simp_opbooleanp         )\
	X(OP_CAR,       "car",                  simp_opcar              )\
	X(OP_CDR,       "cdr",                  simp_opcdr              )\
	X(OP_CONS,      "cons",                 simp_opcons             )\
	X(OP_CURIPORT,  "current-input-port",   simp_opcuriport         )\
	X(OP_CUROPORT,  "current-output-port",  simp_opcuroport         )\
	X(OP_CUREPORT,  "current-error-port",   simp_opcureport         )\
	X(OP_DEFINE,    "define",               simp_opdefine           )\
	X(OP_DISPLAY,   "display",              simp_opdisplay          )\
	X(OP_DIVIDE,    "/",                    simp_opdivide           )\
	X(OP_EQUAL,     "=",                    simp_opequal            )\
	X(OP_EVAL,      "eval",                 simp_opeval             )\
	X(OP_FALSE,     "false",                simp_opfalse            )\
	X(OP_GT,        ">",                    simp_opgt               )\
	X(OP_IF,        "if",                   simp_opif               )\
	X(OP_LAMBDA,    "lambda",               simp_oplambda           )\
	X(OP_LT,        "<",                    simp_oplt               )\
	X(OP_MACRO,     "macro",                simp_opmacro            )\
	X(OP_MAKEENV,   "make-environment",     simp_opmakeenvironment  )\
	X(OP_MULTIPLY,  "*",                    simp_opmultiply         )\
	X(OP_NEWLINE,   "newline",              simp_opnewline          )\
	X(OP_NULLP,     "null?",                simp_opnullp            )\
	X(OP_PAIRP,     "pair?",                simp_oppairp            )\
	X(OP_PORTP,     "port?",                simp_opportp            )\
	X(OP_SAMEP,     "same?",                simp_opsamep            )\
	X(OP_SUBTRACT,  "-",                    simp_opsubtract         )\
	X(OP_SYMBOLP,   "symbol?",              simp_opsymbolp          )\
	X(OP_TRUE,      "true",                 simp_optrue             )\
	X(OP_VOID,      "void",                 simp_opvoid             )\
	X(OP_WRITE,     "write",                simp_opwrite            )

typedef struct Simp             Simp;
typedef unsigned long long      SimpSiz;
typedef long long               SimpInt;
typedef Simp Builtin(Simp ctx, Simp oprnds, Simp env);

enum Exceptions {
#define X(n, s) n,
	EXCEPTIONS
	NEXCEPTIONS
#undef  X
};

enum Operations {
#define X(n, s, p) n,
	OPERATIONS
	NOPERATIONS
#undef  X
};

struct Simp {
	union {
		SimpInt         num;
		double          real;
		Simp           *vector;
		void           *string;
		unsigned char  *errmsg;
		unsigned char   byte;
		void           *port;
		Builtin        *builtin;
	} u;
	enum Type {
		TYPE_APPLICATIVE,
		TYPE_BUILTIN,
		TYPE_BYTE,
		TYPE_ENVIRONMENT,
		TYPE_EOF,
		TYPE_EXCEPTION,
		TYPE_FALSE,
		TYPE_OPERATIVE,
		TYPE_BINDING,
		TYPE_PORT,
		TYPE_REAL,
		TYPE_SIGNUM,
		TYPE_STRING,
		TYPE_SYMBOL,
		TYPE_TRUE,
		TYPE_VECTOR,
		TYPE_VOID,
	} type;
};

/* error handling */
unsigned char *simp_errorstr(int exception);

/* data pair utils */
Simp    simp_car(Simp ctx, Simp obj);
Simp    simp_cdr(Simp ctx, Simp obj);
Simp    simp_cons(Simp ctx, Simp a, Simp b);

/* data constant utils */
Simp    simp_nil(void);
Simp    simp_empty(void);
Simp    simp_eof(void);
Simp    simp_false(void);
Simp    simp_true(void);
Simp    simp_void(void);

/* data type accessors */
Builtin *simp_getbuiltin(Simp ctx, Simp obj);
unsigned char simp_getbyte(Simp ctx, Simp obj);
SimpInt simp_getnum(Simp ctx, Simp obj);
void   *simp_getport(Simp ctx, Simp obj);
double  simp_getreal(Simp ctx, Simp obj);
SimpSiz   simp_getsize(Simp ctx, Simp obj);
unsigned char *simp_getstring(Simp ctx, Simp obj);
unsigned char *simp_getsymbol(Simp ctx, Simp obj);
unsigned char *simp_getexception(Simp ctx, Simp obj);
Simp    simp_getstringmemb(Simp ctx, Simp obj, SimpSiz pos);
Simp    simp_getvectormemb(Simp ctx, Simp obj, SimpSiz pos);
enum Type simp_gettype(Simp ctx, Simp obj);
Simp    simp_getapplicativeenv(Simp ctx, Simp obj);
Simp    simp_getapplicativeparam(Simp ctx, Simp obj);
Simp    simp_getapplicativebody(Simp ctx, Simp obj);
Simp    simp_getoperativeenv(Simp ctx, Simp obj);
Simp    simp_getoperativeparam(Simp ctx, Simp obj);
Simp    simp_getoperativebody(Simp ctx, Simp obj);

/* data type predicates */
bool    simp_isapplicative(Simp ctx, Simp obj);
bool    simp_isbool(Simp ctx, Simp obj);
bool    simp_isbuiltin(Simp ctx, Simp obj);
bool    simp_isbyte(Simp ctx, Simp obj);
bool    simp_isempty(Simp ctx, Simp obj);
bool    simp_isenvironment(Simp ctx, Simp obj);
bool    simp_iseof(Simp ctx, Simp obj);
bool    simp_isexception(Simp ctx, Simp obj);
bool    simp_isfalse(Simp ctx, Simp obj);
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
void    simp_setcar(Simp ctx, Simp obj, Simp val);
void    simp_setcdr(Simp ctx, Simp obj, Simp val);
void    simp_setstring(Simp ctx, Simp obj, SimpSiz pos, unsigned char val);
void    simp_setvector(Simp ctx, Simp obj, SimpSiz pos, Simp val);

/* data type constructors */
Simp    simp_makebuiltin(Simp ctx, Builtin *fun);
Simp    simp_makebyte(Simp ctx, unsigned char byte);
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
Simp    simp_contextiport(Simp ctx);
Simp    simp_contextoport(Simp ctx);
Simp    simp_contexteport(Simp ctx);
Simp    simp_contextnew(void);

/* environment operations */
Simp    simp_envget(Simp ctx, Simp env, Simp sym);
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
