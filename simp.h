#include <stdbool.h>

#define LEN(a)          (sizeof(a) / sizeof((a)[0]))
#define FLAG(f, b)      (((f) & (b)) == (b))
#define MASK(f, m, b)   (((f) & (m)) == (b))
#define RETURN_FAILURE  (-1)
#define RETURN_SUCCESS  0
#define NOTHING         (-1)

#define EXCEPTIONS                                                       \
	X(ERROR_ARGS,           "wrong number of arguments"             )\
	X(ERROR_DIVZERO,        "division by zero"                      )\
	X(ERROR_EMPTY,          "empty operation"                       )\
	X(ERROR_ENVIRON,        "symbol for environment not supplied"   )\
	X(ERROR_UNBOUND,        "unbound variable"                      )\
	X(ERROR_OPERATOR,       "operator is not a procedure"           )\
	X(ERROR_OUTOFRANGE,     "out of range"                          )\
	X(ERROR_MAP,            "map over vectors of different sizes"   )\
	X(ERROR_ILLEXPR,        "ill expression"                        )\
	X(ERROR_ILLFORM,        "ill-formed syntactical form"           )\
	X(ERROR_ILLSYNTAX,      "ill syntax form"                       )\
	X(ERROR_UNKSYNTAX,      "unknown syntax form"                   )\
	X(ERROR_ILLTYPE,        "improper type"                         )\
	X(ERROR_NOTBYTE,        "not a byte"                            )\
	X(ERROR_NOTSYM,         "not a symbol"                          )\
	X(ERROR_STREAM,         "stream error"                          )\
	X(ERROR_RANGE,          "out of range"                          )\
	X(ERROR_MEMORY,         "allocation error"                      )

#define TYPES                                              \
	/* Object type        Is vector   Is allocated   */\
	X(TYPE_VECTOR,        true,       true            )\
	X(TYPE_CLOSURE,       true,       true            )\
	X(TYPE_BUILTIN,       false,      false           )\
	X(TYPE_BYTE,          false,      false           )\
	X(TYPE_ENVIRONMENT,   true,       true            )\
	X(TYPE_EOF,           false,      false           )\
	X(TYPE_EXCEPTION,     false,      false           )\
	X(TYPE_FALSE,         false,      false           )\
	X(TYPE_BINDING,       true,       true            )\
	X(TYPE_PORT,          false,      true            )\
	X(TYPE_REAL,          false,      false           )\
	X(TYPE_SIGNUM,        false,      false           )\
	X(TYPE_STRING,        false,      true            )\
	X(TYPE_SYMBOL,        false,      true            )\
	X(TYPE_TRUE,          false,      false           )

#define PORTS                           \
	X(PORT_STDIN,   stdin,  "r"    )\
	X(PORT_STDOUT,  stdout, "w"    )\
	X(PORT_STDERR,  stderr, "w"    )

typedef struct Simp             Simp;
typedef struct Port             Port;
typedef unsigned long long      SimpSiz;
typedef long long               SimpInt;
typedef struct Builtin          Builtin;

enum Exceptions {
#define X(n, s) n,
	EXCEPTIONS
	NEXCEPTIONS
#undef  X
};

enum Ports {
#define X(n, f, m) n,
	PORTS
#undef  X
};

struct Port {
	enum Porttype {
		PORT_STREAM,
		PORT_STRING,
	} type;
	enum PortMode {
		PORT_OPEN     = 0x01,
		PORT_WRITE    = 0x02,
		PORT_READ     = 0x04,
		PORT_ERR      = 0x08,
		PORT_EOF      = 0x10,
	} mode;
	union {
		void   *fp;
		struct {
			unsigned char *arr;
			SimpSiz curr, size;
		} str;
	} u;
	SimpInt nlines;
};

typedef enum Type {
#define X(n, v, h) n,
	TYPES
#undef  X
} Type;

struct Simp {
	union {
		SimpInt         num;
		double          real;
		Simp           *vector;
		struct Port    *port;
		unsigned char  *string;
		unsigned char  *errmsg;
		unsigned char   byte;
		Builtin        *builtin;
	} u;
	SimpSiz capacity;
	SimpSiz nmembers;
	Type type;
};

/* error handling */
unsigned char *simp_errorstr(int exception);

/* data constant utils */
Simp    simp_nil(void);
Simp    simp_nulenv(void);
Simp    simp_empty(void);
Simp    simp_eof(void);
Simp    simp_false(void);
Simp    simp_true(void);
Simp    simp_void(void);

/* data type accessors */
Builtin *simp_getbuiltin(Simp obj);
unsigned char simp_getbyte(Simp obj);
SimpInt simp_getnum(Simp obj);
void   *simp_getport(Simp obj);
double  simp_getreal(Simp obj);
SimpSiz simp_getsize(Simp obj);
SimpSiz simp_getcapacity(Simp obj);
unsigned char *simp_getstring(Simp obj);
unsigned char *simp_getsymbol(Simp obj);
unsigned char *simp_getexception(Simp obj);
Simp    simp_getvectormemb(Simp obj, SimpSiz pos);
unsigned char simp_getstringmemb(Simp obj, SimpSiz pos);
Type    simp_gettype(Simp obj);
Simp   *simp_getvector(Simp obj);
Simp    simp_getclosureenv(Simp obj);
Simp    simp_getclosureparam(Simp obj);
Simp    simp_getclosurebody(Simp obj);
Simp    simp_getclosurevarargs(Simp obj);
void   *simp_getgcmemory(Simp obj);

/* data type predicates */
bool    simp_isclosure(Simp obj);
bool    simp_isbool(Simp obj);
bool    simp_isbuiltin(Simp obj);
bool    simp_isvarargs(Simp obj);
bool    simp_isbyte(Simp obj);
bool    simp_isempty(Simp obj);
bool    simp_isenvironment(Simp obj);
bool    simp_iseof(Simp obj);
bool    simp_isexception(Simp obj);
bool    simp_isfalse(Simp obj);
bool    simp_isform(Simp obj);
bool    simp_isnum(Simp obj);
bool    simp_isnil(Simp obj);
bool    simp_ispair(Simp obj);
bool    simp_ispair(Simp obj);
bool    simp_isport(Simp obj);
bool    simp_isprocedure(Simp obj);
bool    simp_isreal(Simp obj);
bool    simp_isstring(Simp obj);
bool    simp_issymbol(Simp obj);
bool    simp_istrue(Simp obj);
bool    simp_isvector(Simp obj);
bool    simp_isvoid(Simp obj);

/* data type checkers */
bool    simp_issame(Simp a, Simp b);

/* data type mutators */
void    simp_setstring(Simp obj, SimpSiz pos, unsigned char u);
void    simp_setvector(Simp obj, SimpSiz pos, Simp val);
void    simp_cpyvector(Simp dst, Simp src);
void    simp_cpystring(Simp dst, Simp src);

/* data type constructors */
Simp    simp_exception(int n);
Simp    simp_makebyte(Simp ctx, unsigned char byte);
Simp    simp_makeform(Simp ctx, int);
Simp    simp_makebuiltin(Simp ctx, Builtin *);
Simp    simp_makeclosure(Simp ctx, Simp env, Simp params, Simp extras, Simp body);
Simp    simp_makeenvironment(Simp ctx, Simp parent);
Simp    simp_makenum(Simp ctx, SimpInt n);
Simp    simp_makeport(Simp ctx, void *p);
Simp    simp_makereal(Simp ctx, double x);
Simp    simp_makestring(Simp ctx, unsigned char *src, SimpSiz size);
Simp    simp_makesymbol(Simp ctx, unsigned char *src, SimpSiz size);
Simp    simp_makevector(Simp ctx, SimpSiz size);

/* slicers */
Simp    simp_slicevector(Simp obj, SimpSiz from, SimpSiz size);
Simp    simp_slicestring(Simp obj, SimpSiz from, SimpSiz size);

/* context operations */
Simp    simp_contextenvironment(Simp ctx);
Simp    simp_contextsymtab(Simp ctx);
Simp    simp_contextports(Simp ctx);
Simp    simp_contextiport(Simp ctx);
Simp    simp_contextoport(Simp ctx);
Simp    simp_contexteport(Simp ctx);
Simp    simp_contextforms(Simp ctx);
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
int     simp_readbyte(Simp port);
int     simp_peekbyte(Simp port);
void    simp_unreadbyte(Simp port, int c);
void    simp_printf(Simp port, const char *fmt, ...);

/* eval */
Simp    simp_read(Simp ctx, Simp port);
Simp    simp_write(Simp port, Simp obj);
Simp    simp_display(Simp port, Simp obj);
Simp    simp_eval(Simp ctx, Simp expr, Simp env);
Simp    simp_initforms(Simp ctx);
Simp    simp_initports(Simp ctx);
Simp    simp_initbuiltins(Simp ctx);

/* gc */
void   *simp_gcnewarray(Simp ctx, SimpSiz nmembs, SimpSiz membsiz);
void    simp_gc(Simp ctx, Simp *objs, SimpSiz nobjs);
void    simp_gcfree(Simp ctx);
