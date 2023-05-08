#include <stddef.h>
#include <stdint.h>

#define LEN(a)          (sizeof(a) / sizeof((a)[0]))
#define FLAG(f, b)      (((f) & (b)) == (b))
#define MASK(f, m, b)   (((f) & (m)) == (b))
#define RETURN_FAILURE  (-1)
#define RETURN_SUCCESS  0
#define FALSE           0
#define TRUE            1
#define NOTHING         (-1)

#define SIGMIN          INT32_MIN
#define SIGMAX          INT32_MAX
#define FIXMAX          UINT32_MAX
#define TMPMAX          UINT64_MAX

typedef struct Context  Context;
typedef struct Atom     Atom;
typedef struct Port     Port;
typedef double          Real;
typedef uint8_t         Byte;
typedef uint32_t        Fixnum;
typedef uint64_t        Tmpnum;
typedef int32_t         Signum;
typedef Fixnum          Size;
typedef int             Bool;
typedef int             Status;
typedef int             RByte;  /* read byte: [-1, 255] */

/* data type */
struct Atom {
	union {
		Signum  num;
		Real    real;
		Atom   *vector;
		Byte   *string;
		Byte    byte;
		Port   *port;
	} u;
	Size size;
	enum Type {
		TYPE_SYMBOL     = 2,
		TYPE_SIGNUM     = 3,
		TYPE_REAL       = 4,
		TYPE_STRING     = 5,
		TYPE_VECTOR     = 6,
		TYPE_PORT       = 7,
		TYPE_CLOSURE    = 8,
		TYPE_BYTE       = 9,
	} type;
};

/* data pair utils */
Atom    simp_car(Context *ctx, Atom obj);
Atom    simp_cdr(Context *ctx, Atom obj);
Atom    simp_cons(Context *ctx, Atom a, Atom b);

/* data constant utils */
Atom    simp_nil(void);
Atom    simp_empty(void);

/* data type accessors */
Byte    simp_getbyte(Context *ctx, Atom obj);
Signum  simp_getnum(Context *ctx, Atom obj);
Port   *simp_getport(Context *ctx, Atom obj);
Real    simp_getreal(Context *ctx, Atom obj);
Size    simp_getsize(Context *ctx, Atom obj);
Byte   *simp_getstring(Context *ctx, Atom obj);
Byte    simp_getstringmemb(Context *ctx, Atom obj, Size pos);
Atom   *simp_getvector(Context *ctx, Atom obj);
Atom    simp_getvectormemb(Context *ctx, Atom obj, Size pos);

/* data type predicates */
Bool    simp_isbyte(Context *ctx, Atom obj);
Bool    simp_isnum(Context *ctx, Atom obj);
Bool    simp_isnil(Context *ctx, Atom obj);
Bool    simp_isnul(Context *ctx, Atom obj);
Bool    simp_isport(Context *ctx, Atom obj);
Bool    simp_isreal(Context *ctx, Atom obj);
Bool    simp_isstring(Context *ctx, Atom obj);
Bool    simp_issymbol(Context *ctx, Atom obj);
Bool    simp_isvector(Context *ctx, Atom obj);

/* data type mutators */
void    simp_setcar(Context *ctx, Atom obj, Atom val);
void    simp_setcdr(Context *ctx, Atom obj, Atom val);
void    simp_setstring(Context *ctx, Atom obj, Size pos, Byte val);
void    simp_setvector(Context *ctx, Atom obj, Size pos, Atom val);
void    simp_setport(Context *ctx, Atom obj, Fixnum mode);

/* data type constructors */
Atom    simp_makebyte(Context *ctx, Byte byte);
Atom    simp_makenum(Context *ctx, Signum n);
Atom    simp_makeport(Context *ctx, Port *p);
Atom    simp_makereal(Context *ctx, Real x);
Atom    simp_makestring(Context *ctx, Byte *src, Size size);
Atom    simp_makesymbol(Context *ctx, Byte *src, Size size);
Atom    simp_makevector(Context *ctx, Size size, Atom fill);

/* context operations */
Atom    simp_contextintern(Context *ctx, Byte *src, Size size);
Atom    simp_contextlookup(Context *ctx, Byte *src, Size size);
Atom    simp_contextiport(Context *ctx);
Atom    simp_contextoport(Context *ctx);
Atom    simp_contexteport(Context *ctx);
Context *simp_contextnew(void);

/* port operations */
Atom    simp_openiport(Context *ctx);
Atom    simp_openoport(Context *ctx);
Atom    simp_openeport(Context *ctx);
Bool    simp_porteof(Context *ctx, Atom obj);
Bool    simp_porterr(Context *ctx, Atom obj);

/* port byte IO */
RByte   simp_readbyte(Context *ctx, Atom obj);
RByte   simp_peekbyte(Context *ctx, Atom obj);
void    simp_unreadbyte(Context *ctx, Atom obj, RByte c);
void    simp_printf(Context *ctx, Atom obj, const char *fmt, ...);

/* repl operations */
Atom    simp_read(Context *ctx, Atom port);
void    simp_write(Context *ctx, Atom port, Atom obj);
void    simp_repl(Context *ctx);


#define simp_nil()                      \
	(Atom){                         \
		.type = TYPE_VECTOR,    \
		.size = 0,              \
		.u.vector = NULL,       \
	}
