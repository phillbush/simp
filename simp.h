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

typedef struct Simp     Simp;
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
struct Simp {
	union {
		Signum  num;
		Real    real;
		Simp   *vector;
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
Simp    simp_car(Simp ctx, Simp obj);
Simp    simp_cdr(Simp ctx, Simp obj);
Simp    simp_cons(Simp ctx, Simp a, Simp b);

/* data constant utils */
Simp    simp_nil(void);
Simp    simp_empty(void);

/* data type accessors */
Byte    simp_getbyte(Simp ctx, Simp obj);
Signum  simp_getnum(Simp ctx, Simp obj);
Port   *simp_getport(Simp ctx, Simp obj);
Real    simp_getreal(Simp ctx, Simp obj);
Size    simp_getsize(Simp ctx, Simp obj);
Byte   *simp_getstring(Simp ctx, Simp obj);
Byte    simp_getstringmemb(Simp ctx, Simp obj, Size pos);
Simp   *simp_getvector(Simp ctx, Simp obj);
Simp    simp_getvectormemb(Simp ctx, Simp obj, Size pos);

/* data type predicates */
Bool    simp_isbyte(Simp ctx, Simp obj);
Bool    simp_isnum(Simp ctx, Simp obj);
Bool    simp_isnil(Simp ctx, Simp obj);
Bool    simp_isnul(Simp ctx, Simp obj);
Bool    simp_isport(Simp ctx, Simp obj);
Bool    simp_isreal(Simp ctx, Simp obj);
Bool    simp_isstring(Simp ctx, Simp obj);
Bool    simp_issymbol(Simp ctx, Simp obj);
Bool    simp_isvector(Simp ctx, Simp obj);

/* data type mutators */
void    simp_setcar(Simp ctx, Simp obj, Simp val);
void    simp_setcdr(Simp ctx, Simp obj, Simp val);
void    simp_setstring(Simp ctx, Simp obj, Size pos, Byte val);
void    simp_setvector(Simp ctx, Simp obj, Size pos, Simp val);
void    simp_setport(Simp ctx, Simp obj, Fixnum mode);

/* data type constructors */
Simp    simp_makebyte(Simp ctx, Byte byte);
Simp    simp_makenum(Simp ctx, Signum n);
Simp    simp_makeport(Simp ctx, Port *p);
Simp    simp_makereal(Simp ctx, Real x);
Simp    simp_makestring(Simp ctx, Byte *src, Size size);
Simp    simp_makesymbol(Simp ctx, Byte *src, Size size);
Simp    simp_makevector(Simp ctx, Size size, Simp fill);

/* context operations */
Simp    simp_contextintern(Simp ctx, Byte *src, Size size);
Simp    simp_contextlookup(Simp ctx, Byte *src, Size size);
Simp    simp_contextiport(Simp ctx);
Simp    simp_contextoport(Simp ctx);
Simp    simp_contexteport(Simp ctx);
Simp simp_contextnew(void);

/* port operations */
Simp    simp_openstream(Simp ctx, void *p, char *mode);
Bool    simp_porteof(Simp ctx, Simp obj);
Bool    simp_porterr(Simp ctx, Simp obj);

/* port byte IO */
RByte   simp_readbyte(Simp ctx, Simp obj);
RByte   simp_peekbyte(Simp ctx, Simp obj);
void    simp_unreadbyte(Simp ctx, Simp obj, RByte c);
void    simp_printf(Simp ctx, Simp obj, const char *fmt, ...);

/* repl operations */
Simp    simp_read(Simp ctx, Simp port);
void    simp_write(Simp ctx, Simp port, Simp obj);
void    simp_repl(Simp ctx);
