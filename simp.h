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
	X(ERROR_MEMORY, "allocation error")

#define OPERATIONS                                      \
	X(OP_LET,     "let",          simp_oplet)       \
	X(OP_QUOTE,   "quote",        simp_opquote)

typedef struct Simp             Simp;
typedef unsigned long long      USimp;
typedef long long               SSimp;
typedef Simp Builtin(Simp ctx, Simp oprnds, Simp env);

enum Exceptions {
#define X(n, s) n,
	EXCEPTIONS
	NEXCEPTIONS
#undef  X
};

enum Procedures {
#define X(n, s, p) n,
	OPERATIONS
	NOPERATIONS
#undef  X
};

/* data type */
struct Simp {
	union {
		SSimp           num;
		double          real;
		Simp           *vector;
		unsigned char  *string;
		unsigned char   byte;
		void           *port;
		Builtin        *builtin;
	} u;
	SSimp size;
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

/* data type accessors */
Builtin *simp_getbuiltin(Simp ctx, Simp obj);
unsigned char simp_getbyte(Simp ctx, Simp obj);
SSimp  simp_getnum(Simp ctx, Simp obj);
void   *simp_getport(Simp ctx, Simp obj);
double  simp_getreal(Simp ctx, Simp obj);
SSimp   simp_getsize(Simp ctx, Simp obj);
unsigned char *simp_getstring(Simp ctx, Simp obj);
unsigned char *simp_getsymbol(Simp ctx, Simp obj);
unsigned char *simp_getexception(Simp ctx, Simp obj);
Simp    simp_getstringmemb(Simp ctx, Simp obj, SSimp pos);
Simp   *simp_getvector(Simp ctx, Simp obj);
Simp    simp_getvectormemb(Simp ctx, Simp obj, SSimp pos);

/* data type predicates */
int     simp_isbuiltin(Simp ctx, Simp obj);
int     simp_isbyte(Simp ctx, Simp obj);
int     simp_isexception(Simp ctx, Simp obj);
int     simp_isnum(Simp ctx, Simp obj);
int     simp_isnil(Simp ctx, Simp obj);
int     simp_isnul(Simp ctx, Simp obj);
int     simp_ispair(Simp ctx, Simp obj);
int     simp_ispair(Simp ctx, Simp obj);
int     simp_isport(Simp ctx, Simp obj);
int     simp_isreal(Simp ctx, Simp obj);
int     simp_isstring(Simp ctx, Simp obj);
int     simp_issymbol(Simp ctx, Simp obj);
int     simp_isvector(Simp ctx, Simp obj);

/* data type checkers */
int     simp_issame(Simp ctx, Simp a, Simp b);

/* data type mutators */
void    simp_setcar(Simp ctx, Simp obj, Simp val);
void    simp_setcdr(Simp ctx, Simp obj, Simp val);
void    simp_setstring(Simp ctx, Simp obj, SSimp pos, unsigned char val);
void    simp_setvector(Simp ctx, Simp obj, SSimp pos, Simp val);

/* data type constructors */
Simp    simp_makebuiltin(Simp ctx, Builtin *fun);
Simp    simp_makebyte(Simp ctx, unsigned char byte);
Simp    simp_makeexception(Simp ctx, int n);
Simp    simp_makenum(Simp ctx, SSimp n);
Simp    simp_makeport(Simp ctx, void *p);
Simp    simp_makereal(Simp ctx, double x);
Simp    simp_makestring(Simp ctx, unsigned char *src, SSimp size);
Simp    simp_makesymbol(Simp ctx, unsigned char *src, SSimp size);
Simp    simp_makevector(Simp ctx, SSimp size, Simp fill);

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
int     simp_porteof(Simp ctx, Simp obj);
int     simp_porterr(Simp ctx, Simp obj);

/* IO */
int     simp_readbyte(Simp ctx, Simp port);
int     simp_peekbyte(Simp ctx, Simp port);
void    simp_unreadbyte(Simp ctx, Simp obj, int c);
void    simp_printf(Simp ctx, Simp obj, const char *fmt, ...);
Simp    simp_read(Simp ctx, Simp port);
void    simp_write(Simp ctx, Simp port, Simp obj);

/* eval */
Simp    simp_eval(Simp ctx, Simp expr, Simp env);
