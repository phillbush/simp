#define LEN(a)          (sizeof(a) / sizeof((a)[0]))
#define FLAG(f, b)      (((f) & (b)) == (b))
#define MASK(f, m, b)   (((f) & (m)) == (b))
#define RETURN_FAILURE  (-1)
#define RETURN_SUCCESS  0
#define FALSE           0
#define TRUE            1
#define NOTHING         (-1)

#define EXCEPTIONS                              \
	X(ERROR_UNBOUND, "unbound variable")    \
	X(ERROR_ILLEXPR, "ill expression")      \
	X(ERROR_ILLTYPE, "improper type")       \
	X(ERROR_STREAM,  "stream error")        \
	X(ERROR_RANGE,  "out of range")         \
	X(ERROR_MEMORY, "allocation error")

#define OPERATIONS                                       \
	X(OP_ADD,       "+",            simp_opadd      )\
	X(OP_SUBTRACT,  "-",            simp_opsubtract )\
	X(OP_MULTIPLY,  "*",            simp_opmultiply )\
	X(OP_DIVIDE,    "/",            simp_opdivide   )\
	X(OP_LET,       "let",          simp_oplet      )\
	X(OP_QUOTE,     "quote",        simp_opquote    )

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

/* data type */
struct Simp {
	union {
		SimpInt         num;
		double          real;
		void           *vector;
		void           *string;
		unsigned char  *errmsg;
		unsigned char   byte;
		void           *port;
		Builtin        *builtin;
	} u;
	enum Type {
		TYPE_SYMBOL,
		TYPE_SIGNUM,
		TYPE_REAL,
		TYPE_BUILTIN,
		TYPE_STRING,
		TYPE_VECTOR,
		TYPE_PORT,
		TYPE_BYTE,
		TYPE_EXCEPTION,
		TYPE_EOF,
		TYPE_TRUE,
		TYPE_FALSE,
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
Simp   *simp_getvector(Simp ctx, Simp obj);
Simp    simp_getvectormemb(Simp ctx, Simp obj, SimpSiz pos);
enum Type simp_gettype(Simp ctx, Simp obj);

/* data type predicates */
int     simp_isbool(Simp ctx, Simp obj);
int     simp_isbuiltin(Simp ctx, Simp obj);
int     simp_isbyte(Simp ctx, Simp obj);
int     simp_isempty(Simp ctx, Simp obj);
int     simp_iseof(Simp ctx, Simp obj);
int     simp_isexception(Simp ctx, Simp obj);
int     simp_isfalse(Simp ctx, Simp obj);
int     simp_isnum(Simp ctx, Simp obj);
int     simp_isnil(Simp ctx, Simp obj);
int     simp_ispair(Simp ctx, Simp obj);
int     simp_ispair(Simp ctx, Simp obj);
int     simp_isport(Simp ctx, Simp obj);
int     simp_isreal(Simp ctx, Simp obj);
int     simp_isstring(Simp ctx, Simp obj);
int     simp_issymbol(Simp ctx, Simp obj);
int     simp_istrue(Simp ctx, Simp obj);
int     simp_isvector(Simp ctx, Simp obj);

/* data type checkers */
int     simp_issame(Simp ctx, Simp a, Simp b);

/* data type mutators */
void    simp_setcar(Simp ctx, Simp obj, Simp val);
void    simp_setcdr(Simp ctx, Simp obj, Simp val);
void    simp_setstring(Simp ctx, Simp obj, SimpSiz pos, unsigned char val);
void    simp_setvector(Simp ctx, Simp obj, SimpSiz pos, Simp val);

/* data type constructors */
Simp    simp_makebuiltin(Simp ctx, Builtin *fun);
Simp    simp_makebyte(Simp ctx, unsigned char byte);
Simp    simp_makeexception(Simp ctx, int n);
Simp    simp_makenum(Simp ctx, SimpInt n);
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
void    simp_printf(Simp ctx, Simp obj, const char *fmt, ...);

/* eval */
Simp    simp_read(Simp ctx, Simp port);
void    simp_write(Simp ctx, Simp port, Simp obj);
Simp    simp_eval(Simp ctx, Simp expr, Simp env);
