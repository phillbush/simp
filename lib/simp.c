/*
 *               Simp: A Simplistic Programming Language
 *
 * An object in The Simp Programing Language can be classifyed as an
 * immediate object or a heap object based on whether it is necessary
 * to allocate memory for representing this object:
 *
 * - An immediate object fits in our object representation, and thus
 *   needs not be allocated in the heap.  A value for an immediate
 *   object holds the encoded (packed) form of the immediate object.
 *   Macros such as SIMP_UNPACK_FIXNUM must be used to extract (unpack)
 *   an immediate object from a value.  The macro SIMP_IS_IMMEDIATE
 *   tests whether a given value represents an immediate object.
 *
 * - A heap object may not fit in our object representation, and thus
 *   needs to be allocated in the heap.  A value for a heap object
 *   actually holds the address in the memory to the data encoding the
 *   heap object.  The macro SIMP_UNPACK_HEAPOBJ must be used to extract
 *   (unpack) the address to the heap object from a value.  The macro
 *   SIMP_IS_HEAPOBJ tests whether a given value represents a heap
 *   object.
 *
 * We use three types (simpptr_t, usimpint_t and simpint_t) for
 * representing Simp objects.
 *
 * - A `simpptr_t` is a pointer value that can ONLY represent a
 *   Simp object.  It can be converted into a `usimpint_t` with the
 *   SIMP_TOINT macro.
 *
 * - A `usimpint_t` is an unsigned integer value that represents either
 *   a plain integer, or the comparable and manipulable integer form of
 *   a simp object.  It can be converted into a `simpptr_t` using the
 *   SIMP_TOPTR macro.
 *
 * - A `simpint_t` is a signed integer value of the same size of
 *   a `usimpint_t`.
 *
 * We use two tagging layers to associate metadata with objects: pointer
 * tagging and "memory tagging" (this is in quotes for I coined this
 * term in the lack of an actual name for it).
 *
 * - Pointer tagging (our first tagging layer) is the technique of using
 *   a few bits in the representation of an object for holding metadata.
 *   We use two bits for pointer tagging.  One bit distinguishes whether
 *   a value represents a heap object or an immediate object.  The other
 *   bit, on heap objects, represents whether the object is marked by
 *   the garbage collector; and, on immediate objects, distinguishes
 *   whether it is a fixnum or not.  Pointer tagging depends on the
 *   assumption that heap objects are allocated at addresses aligned by
 *   a given number of bytes, leaving a few least significant bits
 *   meaningless and available for tagging.  We enforce such alignment
 *   by using posix_memalign(3) in POSIX systems, and mallocalign(2) on
 *   Plan 9 systems.
 *
 * - Memory tagging (our second tagging layer) is the technique of
 *   allocating, before the heap object payload, an identifier header
 *   for holding metadata.  Memory tagging is only used on heap objects.
 *   We use a whole usimpint_t for memory tagging.  The least
 *   significant byte of this usimpint_t informs the structure of the
 *   memory for the object (see below); the remaining part stores
 *   additional information.
 *
 * We arrange the allocated memory for a heap object as an array of
 * `usimpint_t` values.  The macro SIMP_ACCESS_ELEM access an element of
 * this array.  Each heap object has an idiosyncratic structure for its
 * memory, as follows (in general, it's an identifier header, a size
 * header, and the object payload):
 *
 * - The allocated memory area of a vector has the following format.
 *   The macro SIMP_CALCSIZE_VECTOR must be used to compute the size
 *   of the memory to be allocated given the number of elements in the
 *   vector.
 *
 *       ,------------   ----.
 *       |   |   |    ...    |
 *       `------------   ----'
 *        ~~~ ~~~ ~~~~~~~~~~~
 *         |   |       |
 *         |   |       `--> The payload
 *         |   |            (Array of usimpint_t objects).
 *         |   |
 *         |   `--> The size header
 *         |        (unpacked simpint_t with the number of usimpint_t
 *         |        elements in the payload).
 *         |
 *         `--> The identifier header.
 *              (SIMP_MEMTAG_VECTOR on least significant byte).
 *
 * - The allocated memory area of a string (aka bytevectors) has the
 *   following format.  The macro SIMP_CALCSIZE_STRING must be used to
 *   compute the size of the memory to be allocated given the number of
 *   bytes in the string.
 *
 *       ,------------   ----.
 *       |   |   |    ...    |
 *       `------------   ----'
 *        ~~~ ~~~ ~~~~~~~~~~~
 *         |   |       |
 *         |   |       `--> The payload
 *         |   |            (Array of chars).
 *         |   |
 *         |   `--> The size header
 *         |        (unpacked simpint_t with the number of characters
 *         |        in the payload).
 *         |
 *         `--> The identifier header.
 *              (SIMP_MEMTAG_STRING on least significant byte).
 *
 * - The allocated memory area of a non-immediate number has the
 *   following format:
 *
 *       [TODO: number heap objects are not implemented yet]
 *
 * - The allocated memory area of a port has the following format.  The
 *   macro SIMP_CALCSIZE_PORT must be used to compute the size of the
 *   memory to be allocated for a port.
 *
 *       ,-----------.
 *       |   |   |   |
 *       `-----------'
 *        ~~~ ~~~ ~~~
 *         |   |   |
 *         |   |   `--> The payload
 *         |   |        Pointer to the I/O stream (FILE * in POSIX).
 *         |   |
 *         |   `--> The line number
 *         |        (The number of lines read from the port).
 *         |
 *         `--> The identifier header
 *              (SIMP_MEMTAG_PORT on least significant byte).
 *
 */

#ifdef plan9
#include "plan9.h"
#else
#include "posix.h"
#endif

#include <simp.h>

#define STRBUFSIZE              1028
#define MAXOCTALESCAPE          3
#define SYMTABSIZE              389
#define SYMTABMULT              37

enum {
	TOK_ERROR,
	TOK_EOF,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_DOT,
	TOK_STRING,
	TOK_NUMBER,
	TOK_IDENTIFIER,
};

enum {
	/*
	 * indices for the entries of the vector stack (check
	 * simp_read(), newvirtualvector(), and got*() routines).
	 */
	VECTORSTACK_PARENT  = 0,
	VECTORSTACK_NMEMB   = 1,
	VECTORSTACK_ISLIST  = 2,
	VECTORSTACK_NEXT    = 3,
	VECTORSTACK_SIZE    = 4,
};

/* pointer tags (2 bits long) */
enum {
	SIMP_PTRTAG_HEAPOBJ     = 0x00,    /* non-marked heap object */
	SIMP_PTRTAG_HEAPOBJMARK = 0x01,    /* marked heap object */
	SIMP_PTRTAG_CONSTANT    = 0x02,    /* non-integer immediate */
	SIMP_PTRTAG_FIXNUM      = 0x03,    /* integer immediate */
};

/* memory tags (1 byte long) */
enum {
	SIMP_MEMTAG_VECTOR      = 0x01,
	SIMP_MEMTAG_STRING      = 0x02,
	SIMP_MEMTAG_NUMBER      = 0x03,
	SIMP_MEMTAG_PORT        = 0x04,
};

/* memory tag bit masks (applied to the actual memory tags) */
#define SIMP_VECTOR_ISMUTABLE   (0x01 << SIMP_MEMTAG_SIZE)
#define SIMP_STRING_ISMUTABLE   (0x01 << SIMP_MEMTAG_SIZE)
#define SIMP_STRING_ISSYMBOL    (0x02 << SIMP_MEMTAG_SIZE)
#define SIMP_PORT_ISOPEN        (0x01 << SIMP_MEMTAG_SIZE)
#define SIMP_PORT_ISREADING     (0x02 << SIMP_MEMTAG_SIZE)
#define SIMP_PORT_ISWRITING     (0x04 << SIMP_MEMTAG_SIZE)

/* tag sizes, masks and getters */
#define SIMP_PTRTAG_SIZE        2
#define SIMP_PTRTAG_MASK        0x03
#define SIMP_MEMTAG_SIZE        8
#define SIMP_MEMTAG_MASK        0xFF
#define SIMP_GET_PTRTAG(x)      ((usimpint_t)(SIMP_TOINT(x) & SIMP_PTRTAG_MASK))
#define SIMP_GET_MEMTAG(x)      ((usimpint_t)(SIMP_ACCESS_HEADER(x) & SIMP_MEMTAG_MASK))

/* object checkers */
#define SIMP_IS_SAME(x, y)      (SIMP_TOINT(x) == SIMP_TOINT(y))
#define SIMP_IS_FIXNUM(x)       (SIMP_GET_PTRTAG(x) == SIMP_PTRTAG_FIXNUM)
#define SIMP_IS_CONSTANT(x)     (SIMP_GET_PTRTAG(x) == SIMP_PTRTAG_CONSTANT)
#define SIMP_IS_IMMEDIATE(x)    (SIMP_IS_FIXNUM(x) || SIMP_IS_CONSTANT(x))
#define SIMP_IS_HEAPOBJ(x)      (!SIMP_IS_IMMEDIATE(x))

/* object representation conversors */
#define SIMP_TOINT(x)           ((usimpint_t)(x))
#define SIMP_TOPTR(x)           ((simpptr_t)(x))

/* object packers */
#define SIMP_PACK_HEAPOBJ(x)    (SIMP_TOPTR(x))
#define SIMP_PACK_FIXNUM(x)     (SIMP_TOPTR((((usimpint_t)(x))<<SIMP_PTRTAG_SIZE)|SIMP_PTRTAG_FIXNUM))

/* object unpackers */
#define SIMP_UNPACK_HEAPOBJ(x)  ((usimpint_t *)SIMP_TOPTR(x))
#define SIMP_UNPACK_FIXNUM(x)   ((SIMP_TOINT(x) > SIMP_MAX)                             \
                                ? (-1 - (simpint_t)(~SIMP_TOINT(x)>>SIMP_PTRTAG_SIZE))  \
                                : (simpint_t)(SIMP_TOINT(x)>>SIMP_PTRTAG_SIZE))

/* heap object accessors */
#define SIMP_ACCESS_ELEM(x, n)  (SIMP_UNPACK_HEAPOBJ(x)[(n)])
#define SIMP_ACCESS_REST(x)     (SIMP_UNPACK_HEAPOBJ(x)+2)
#define SIMP_ACCESS_HEADER(x)   (SIMP_ACCESS_ELEM(x, 0))
#define SIMP_ACCESS_NMEMB(x)    (SIMP_ACCESS_ELEM(x, 1))
#define SIMP_ACCESS_VECTOR(x)   ((simpptr_t *)SIMP_ACCESS_REST(x))
#define SIMP_ACCESS_STRING(x)   ((char *)SIMP_ACCESS_REST(x))
#define SIMP_ACCESS_PORT(x)     (((void **)(SIMP_ACCESS_REST(x)))[0])  /* access port stream */
#define SIMP_ACCESS_LINENO(x)   (SIMP_ACCESS_ELEM(x, 1))               /* access port line number */

/* heap object memory size calculator */
#define SIMP_CALCSIZE_STRING(x)  (2 * sizeof(usimpint_t) + (x) + 1)
#define SIMP_CALCSIZE_VECTOR(x)  ((2 + (x)) * sizeof(usimpint_t))
#define SIMP_CALCSIZE_PORT(x)    (2 * sizeof(usimpint_t) + sizeof(void *))

/* non-integer immediates */
#define SIMP_MAKE_IMMEDIATE(x)   (SIMP_TOPTR(((x)<<SIMP_PTRTAG_SIZE)|SIMP_PTRTAG_CONSTANT))
#define SIMP_IMM_VOID            (SIMP_MAKE_IMMEDIATE(0x01))
#define SIMP_IMM_NIL             (SIMP_MAKE_IMMEDIATE(0x02))
#define SIMP_IMM_EMPTYSTR        (SIMP_MAKE_IMMEDIATE(0x03))
#define SIMP_IMM_UNDEF           (SIMP_MAKE_IMMEDIATE(0x04))
#define SIMP_IMM_TRUE            (SIMP_MAKE_IMMEDIATE(0x05))
#define SIMP_IMM_FALSE           (SIMP_MAKE_IMMEDIATE(0x06))
#define SIMP_IMM_EOF             (SIMP_MAKE_IMMEDIATE(0x07))

typedef struct simpctx_t {

	/*
	 * A simpctx_t is a pointer to a struct simpctx_t, which
	 * maintains all the data required for the interpreter to run,
	 * such as the stack, the environment, the symbol table, the
	 * current I/O ports, etc.  This object allows the co-existence
	 * of multiple interpreter instances in a single program.
	 *
	 * A simpctx_t is created by the simp_init() function and
	 * destroyed by the simp_clean() procedure.  Each interpreter
	 * should have a single simpctx_t that should be passed
	 * around to virtually all the functions in the library.
	 */

	simpptr_t stack;
	simpptr_t env;
	simpptr_t symtab;             /* symbol hash table */

	/*
	 * Stacks used while reading expressions and building them
	 * (check simp_read(), newvirtualvector(), and got*() routines).
	 */
	simpptr_t readstack;
	simpptr_t vectorstack;

	/*
	 * Current ports
	 */
	simpptr_t iport;              /* current input port */
	simpptr_t oport;              /* current output port */
	simpptr_t eport;              /* current error port */
} *simpctx_t;

static int
isspace(int c)
{
	return c == '\f' || c == '\n' || c == '\r' ||
	       c == '\t' || c == '\v' || c == ' ';
}

static int
iscntrl(int c)
{
	return c == '\x7f' || (c >= '\x00' && c <= '\x1f');
}

static int
isdelimiter(int c)
{
	return isspace(c) || c == EOF || c == '(' || c == ')' ||
	       c == '"'   || c == '#' || c == '[' || c == ']';
}

static int
isoctal(int c)
{
	return c == '0' || c == '1' || c == '2' || c == '3' ||
	       c == '4' || c == '5' || c == '6' || c == '7';
}

static int
isdecimal(int c)
{
	return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' ||
	       c == '5' || c == '6' || c == '7' || c == '8' || c == '9';
}

static int
ishex(int c)
{
	return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' ||
	       c == '5' || c == '6' || c == '7' || c == '8' || c == '9' ||
	       c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' ||
	       c == 'F' || c == 'a' || c == 'b' || c == 'c' || c == 'd' ||
	       c == 'e' || c == 'f';
}

static int
ctoi(int c)
{
	switch (c) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return c - '0';
	case 'A': case 'a':
		return 10;
	case 'B': case 'b':
		return 11;
	case 'C': case 'c':
		return 12;
	case 'D': case 'd':
		return 13;
	case 'E': case 'e':
		return 14;
	case 'F': case 'f':
		return 15;
	}
	return 0;
}

static int
readbyte(simpctx_t ctx, simpptr_t port)
{
	int c;

	(void)ctx;
	c = GETC(SIMP_ACCESS_PORT(port));
	if (c == '\n')
		SIMP_ACCESS_NMEMB(port)++;
	return c;
}

static void
unreadbyte(simpctx_t ctx, simpptr_t port, int c)
{
	(void)ctx;
	if (c == EOF)
		return;
	UNGETC(SIMP_ACCESS_PORT(port), c);
}

static int
peekc(simpctx_t ctx, simpptr_t port)
{
	int c;

	c = readbyte(ctx, port);
	unreadbyte(ctx, port, c);
	return c;
}

static simpint_t
symtabhash(const char *str, simpint_t len)
{
	simpint_t i, h;

	h = 0;
	for (i = 0; i < len; i++)
		h = SYMTABMULT * h + str[i];
	return h % SYMTABSIZE;
}

static void
warn(simpctx_t ctx, const char *fmt, ...)
{
	(void)ctx;
	(void)fmt;
	// TODO: handle error
}

static char *
getstr(simpctx_t ctx, simpptr_t port, size_t *len)
{
	size_t j, size;
	int c;
	char *buf;

	size = STRBUFSIZE;
	if ((buf = MALLOC(size)) == NULL) {
		warn(ctx, "allocation error");
		return NULL;
	}
	*len = 0;
	for (;;) {
		c = readbyte(ctx, port);
		if (*len + 1 >= size) {
			size <<= 2;
			if ((buf = REALLOC(buf, size)) == NULL) {
				free(buf);
				warn(ctx, "allocation error");
				return NULL;
			}
		}
		if (c == '\"')
			break;;
		if (c != '\\') {
			buf[(*len)++] = c;
			continue;
		}
		switch ((c = readbyte(ctx, port))) {
		case '"':
			buf[(*len)++] = '\"';
			break;
		case 'a':
			buf[(*len)++] = '\a';
			break;
		case 'b':
			buf[(*len)++] = '\b';
			break;
		case 'e':
			buf[(*len)++] = '\033';
			break;
		case 'f':
			buf[(*len)++] = '\f';
			break;
		case 'n':
			buf[(*len)++] = '\n';
			break;
		case 'r':
			buf[(*len)++] = '\r';
			break;
		case 't':
			buf[(*len)++] = '\t';
			break;
		case 'v':
			buf[(*len)++] = '\v';
			break;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			buf[*len] = 0;
			for (j = 0; j < MAXOCTALESCAPE && isoctal(c); j++, c = readbyte(ctx, port))
				buf[*len] = (buf[*len] << 3) + ctoi(c);
			unreadbyte(ctx, port, c);
			(*len)++;
			break;
		case 'x':
			if (!ishex(c = readbyte(ctx, port))) {
				buf[(*len)++] = 'x';
				unreadbyte(ctx, port, c);
				break;
			}
			for (; ishex(c); c = readbyte(ctx, port))
				buf[*len] = (buf[*len] << 4) + ctoi(c);
			unreadbyte(ctx, port, c);
			(*len)++;
			break;
		case 'u':
			// TODO: handle 4-digit unicode
			break;
		case 'U':
			// TODO: handle 8-digit unicode
			break;
		default:
			buf[(*len)++] = c;
			break;
		}
	}
	buf[*len] = '\0';
	return buf;
}

static char *
getnum(simpctx_t ctx, simpptr_t port, size_t *len, int c)
{
	enum {
		NUM_BINARY,
		NUM_OCTAL,
		NUM_DECIMAL,
		NUM_HEX,
	} numtype;
	size_t size;
	int isexact;
	char sign, *buf;

	/*
	 * We represent read numbers internally as a string in the
	 * following format:
	 * - "[EI][+-]B[01]*" for binary.
	 * - "[EI][+-]O[01234567]*" for octal.
	 * - "[EI][+-]D[0123456789]*" for decimal.
	 * - "[EI][+-]X[0123456789ABCDEF]*" for hexadecimal.
	 * - "[EI][+-]F[0123456789]*(\.[0123456789]*)?([+-][0123456789]*)?"
	 *   for floating point.
	 *
	 * Yes, I convert a read number to string to them convert it
	 * into a number.
	 */
	size = STRBUFSIZE;
	if ((buf = MALLOC(size)) == NULL) {
		warn(ctx, "allocation error");
		return NULL;
	}
	isexact = 1;
	*len = 1;
	if (c == '+' || c == '-') {
		buf[(*len)++] = c;
		c = readbyte(ctx, port);
	} else {
		buf[(*len)++] = '+';
	}
	if (c == '0') {
		switch (c = readbyte(ctx, port)) {
		case 'B': case 'b':
			buf[(*len)++] = 'B';
			numtype = NUM_BINARY;
			break;
		case 'O': case 'o':
			buf[(*len)++] = 'O';
			numtype = NUM_OCTAL;
			break;
		case 'D': case 'd':
			buf[(*len)++] = 'D';
			numtype = NUM_DECIMAL;
			break;
		case 'X': case 'x':
			buf[(*len)++] = 'X';
			numtype = NUM_HEX;
			break;
		default:
			buf[(*len)++] = 'D';
			unreadbyte(ctx, port, c);
			numtype = NUM_DECIMAL;
			break;
		}
	}
	for (;;) {
		if (*len + 2 >= size) {
			size <<= 2;
			if ((buf = REALLOC(buf, size)) == NULL) {
				free(buf);
				warn(ctx, "allocation error");
				return NULL;
			}
		}
		switch (numtype) {
		case NUM_BINARY:
			if (c != '0' && c != '1')
				goto done;
			break;
		case NUM_OCTAL:
			if (!isoctal(c))
				goto done;
			break;
		case NUM_DECIMAL:
			if (!isdecimal(c))
				goto done;
			break;
		case NUM_HEX:
			if (!isoctal(c))
				goto done;
			break;
		default:
			goto done;
		}
		buf[(*len)++] = c;
		c = readbyte(ctx, port);
	}
done:
	if (numtype == NUM_DECIMAL) {
		if (c == '.') {
			isexact = 0;
			buf[2] = 'F';
			buf[(*len)++] = '.';
			while (isdecimal(c = readbyte(ctx, port))) {
				if (*len + 2 >= size) {
					size <<= 2;
					if ((buf = REALLOC(buf, size)) == NULL) {
						free(buf);
						warn(ctx, "allocation error");
						return NULL;
					}
				}
				buf[(*len)++] = c;
			}
		}
		if (c == 'e' || c == 'E') {
			isexact = 0;
			buf[2] = 'F';
			c = readbyte(ctx, port);
			sign = '+';
			if (c == '+' || c == '-') {
				sign = c;
				c = readbyte(ctx, port);
			}
			if (!isdecimal(c)) {
				isexact = 1;
				goto checkdelimiter;
			}
			buf[(*len)++] = sign;
			do {
				if (*len + 2 >= size) {
					size <<= 2;
					if ((buf = REALLOC(buf, size)) == NULL) {
						free(buf);
						warn(ctx, "allocation error");
						return NULL;
					}
				}
				buf[(*len)++] = c;
			} while (isdecimal(c = readbyte(ctx, port)));
		}
	}
	if (c == 'E' || c == 'e') {
		isexact = 1;
		c = readbyte(ctx, port);
	} else if (c == 'I' || c == 'i') {
		isexact = 0;
		c = readbyte(ctx, port);
	}
checkdelimiter:
	if (!(isdelimiter(c))) {
		free(buf);
		warn(ctx, "invalid numeric syntax: \"%c\"", c);
		return NULL;
	}
	unreadbyte(ctx, port, c);
	buf[0] = isexact ? 'E' : 'I';
	buf[*len] = '\0';
	return buf;
}

static char *
getident(simpctx_t ctx, simpptr_t port, size_t *len, int c)
{
	size_t size;
	char *buf;

	size = STRBUFSIZE;
	if ((buf = MALLOC(size)) == NULL) {
		warn(ctx, "allocation error");
		return NULL;
	}
	*len = 0;
	while (!isdelimiter(c)) {
		if (*len + 1 >= size) {
			size <<= 2;
			if ((buf = REALLOC(buf, size)) == NULL) {
				free(buf);
				warn(ctx, "allocation error");
				return NULL;
			}
		}
		buf[(*len)++] = c;
		c = readbyte(ctx, port);
	}
	unreadbyte(ctx, port, c);
	buf[*len] = '\0';
	return buf;
}

static int
gettok(simpctx_t ctx, simpptr_t port, char **tok, size_t *len)
{
	int ret, c;

	*tok = NULL;
	*len = 0;
	while ((c = readbyte(ctx, port)) != EOF && isspace((unsigned char)c))
		;
	if (c == '#')
		while ((c = readbyte(ctx, port)) != '\n' && c != EOF)
			;
	switch (c) {
	case EOF:
		return TOK_EOF;
	case '(':
		return TOK_LPAREN;
	case ')':
		return TOK_RPAREN;
	case '[':
		return TOK_LBRACE;
	case ']':
		return TOK_RBRACE;
	case '.':
		return TOK_DOT;
	case '"':
		*tok = getstr(ctx, port, len);
		ret = TOK_STRING;
		break;
	case '+': case '-':
		if (!isdecimal((unsigned int)peekc(ctx, port)))
			goto token;
		/* FALLTHROUGH */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		*tok = getnum(ctx, port, len, c);
		ret = TOK_NUMBER;
		break;
	default:
token:
		*tok = getident(ctx, port, len, c);
		ret = TOK_IDENTIFIER;
		break;
	}
	if (*tok == NULL)
		return TOK_ERROR;
	return ret;
}

static int
iseof(simpptr_t obj)
{
	return SIMP_IS_SAME(obj, SIMP_IMM_EOF);
}

static int
isfalse(simpptr_t obj)
{
	return SIMP_IS_SAME(obj, SIMP_IMM_FALSE);
}

static int
istrue(simpptr_t obj)
{
	return SIMP_IS_SAME(obj, SIMP_IMM_TRUE);
}

static int
isnil(simpptr_t obj)
{
	return SIMP_IS_SAME(obj, SIMP_IMM_NIL);
}

static int
isvector(simpptr_t obj)
{
	return isnil(obj) || (SIMP_IS_HEAPOBJ(obj) && SIMP_GET_MEMTAG(obj) == SIMP_MEMTAG_VECTOR);
}

static simpptr_t
newstr(simpctx_t ctx, char *tok, simpint_t len, usimpint_t attrs)
{
	simpptr_t obj;

	(void)ctx;
	obj = SIMP_PACK_HEAPOBJ(MALLOCALIGN(SIMP_CALCSIZE_STRING(len)));
	MEMCPY(SIMP_ACCESS_REST(obj), tok, len + 1);           /* +1 for '\0' */
	SIMP_ACCESS_HEADER(obj) = SIMP_MEMTAG_STRING | attrs;
	SIMP_ACCESS_NMEMB(obj) = len;
	return obj;
}

static simpptr_t
newvector(simpctx_t ctx, simpint_t len, simpptr_t fill, usimpint_t attrs)
{
	simpptr_t obj;
	simpint_t i;

	(void)ctx;
	obj = SIMP_PACK_HEAPOBJ(MALLOCALIGN(SIMP_CALCSIZE_VECTOR(len)));
	SIMP_ACCESS_HEADER(obj) = SIMP_MEMTAG_VECTOR | attrs;
	SIMP_ACCESS_NMEMB(obj) = len;
	for (i = 0; i < len; i++)
		SIMP_ACCESS_VECTOR(obj)[i] = fill;
	return obj;
}

static simpptr_t
newport(simpctx_t ctx, void *p, usimpint_t attrs)
{
	simpptr_t obj;

	(void)ctx;
	obj = SIMP_PACK_HEAPOBJ(MALLOCALIGN(SIMP_CALCSIZE_PORT(1)));
	SIMP_ACCESS_HEADER(obj) = SIMP_MEMTAG_PORT | attrs;
	SIMP_ACCESS_LINENO(obj) = 0;
	SIMP_ACCESS_PORT(obj) = p;
	return obj;
}

static void
vector_set(simpctx_t ctx, simpptr_t vector, simpint_t index, simpptr_t value)
{
	(void)ctx;
	SIMP_ACCESS_VECTOR(vector)[index] = value;
}

static simpptr_t
vector_ref(simpctx_t ctx, simpptr_t vector, simpint_t index)
{
	(void)ctx;
	return SIMP_ACCESS_VECTOR(vector)[index];
}

static simpint_t
vector_len(simpctx_t ctx, simpptr_t vector)
{
	(void)ctx;
	return SIMP_ACCESS_NMEMB(vector);
}

static simpint_t
string_len(simpctx_t ctx, simpptr_t str)
{
	(void)ctx;
	return SIMP_ACCESS_NMEMB(str);
}

static void
set_car(simpctx_t ctx, simpptr_t pair, simpptr_t value)
{
	vector_set(ctx, pair, 0, value);
}

static void
set_cdr(simpctx_t ctx, simpptr_t pair, simpptr_t value)
{
	vector_set(ctx, pair, 1, value);
}

static simpptr_t
cons(simpctx_t ctx, simpptr_t a, simpptr_t b)
{
	simpptr_t pair;

	pair = newvector(ctx, 2, SIMP_IMM_UNDEF, 0);
	set_car(ctx, pair, a);
	set_cdr(ctx, pair, b);
	return pair;
}

static simpptr_t
car(simpctx_t ctx, simpptr_t pair)
{
	return vector_ref(ctx, pair, 0);
}

static simpptr_t
cdr(simpctx_t ctx, simpptr_t pair)
{
	return vector_ref(ctx, pair, 1);
}

static simpptr_t
newsym(simpctx_t ctx, char *tok, simpint_t len)
{
	simpptr_t pair, list, sym;
	simpint_t bucket;

	bucket = symtabhash(tok, len);
	list = vector_ref(ctx, ctx->symtab, bucket);
	for (pair = list; !isnil(pair); pair = cdr(ctx, pair)) {
		sym = car(ctx, pair);
		if (memcmp(tok, SIMP_ACCESS_STRING(sym), len) == 0) {
			return sym;
		}
	}
	sym = newstr(ctx, tok, len, 0);
	SIMP_ACCESS_HEADER(sym) = SIMP_MEMTAG_STRING | SIMP_STRING_ISSYMBOL;
	pair = cons(ctx, sym, list);
	vector_set(ctx, ctx->symtab, bucket, pair);
	list = vector_ref(ctx, ctx->symtab, bucket);
	return sym;
}

static simpptr_t
fillvector(simpctx_t ctx, simpint_t len)
{
	simpptr_t vector;
	simpint_t i;

	if (len == 0)
		return SIMP_IMM_NIL;
	vector = newvector(ctx, len, NULL, SIMP_VECTOR_ISMUTABLE);
	for (i = 0; i < len; i++) {
		vector_set(ctx, vector, len - i - 1, car(ctx, ctx->readstack));
		ctx->readstack = cdr(ctx, ctx->readstack);
	}
	return vector;
}

static simpint_t
incrvectorstack(simpctx_t ctx)
{
	simpint_t n;

	/* increment count of elements in vector stack; return new count */
	n = SIMP_UNPACK_FIXNUM(vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NMEMB));
	n++;
	vector_set(ctx, ctx->vectorstack, VECTORSTACK_NMEMB, SIMP_PACK_FIXNUM(n));
	return n;
}

static void
newvirtualvector(simpctx_t ctx)
{
	simpptr_t parent, vector, vhead;
	simpint_t newcnt;

	ctx->readstack = cons(ctx, SIMP_IMM_NIL, ctx->readstack);
	parent = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_PARENT);
	newcnt = incrvectorstack(ctx);
	vector = fillvector(ctx, newcnt);
	ctx->vectorstack = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NEXT);
	if (isnil(parent))
		ctx->readstack = cons(ctx, vector, cdr(ctx, ctx->readstack));
	else
		vector_set(ctx, parent, vector_len(ctx, parent) - 1, vector);
	vhead = newvector(ctx, VECTORSTACK_SIZE, NULL, SIMP_VECTOR_ISMUTABLE);
	vector_set(ctx, vhead, VECTORSTACK_PARENT, vector);
	vector_set(ctx, vhead, VECTORSTACK_NMEMB, SIMP_PACK_FIXNUM(0));
	vector_set(ctx, vhead, VECTORSTACK_ISLIST, SIMP_IMM_TRUE);
	vector_set(ctx, vhead, VECTORSTACK_NEXT, ctx->vectorstack);
	ctx->vectorstack = vhead;
}

static void
gotobject(simpctx_t ctx, simpptr_t obj)
{
	ctx->readstack = cons(ctx, obj, ctx->readstack);
	if (!isnil(ctx->vectorstack)) {
		incrvectorstack(ctx);
	}
}

static void
gotldelim(simpctx_t ctx, int isparens)
{
	simpptr_t vhead;

	ctx->readstack = cons(ctx, SIMP_IMM_NIL, ctx->readstack);
	if (!isnil(ctx->vectorstack))
		incrvectorstack(ctx);
	vhead = newvector(ctx, VECTORSTACK_SIZE, NULL, SIMP_VECTOR_ISMUTABLE);
	vector_set(ctx, vhead, VECTORSTACK_PARENT, SIMP_IMM_NIL);
	vector_set(ctx, vhead, VECTORSTACK_NMEMB, SIMP_PACK_FIXNUM(0));
	vector_set(ctx, vhead, VECTORSTACK_ISLIST, isparens ? SIMP_IMM_TRUE : SIMP_IMM_FALSE);
	vector_set(ctx, vhead, VECTORSTACK_NEXT, ctx->vectorstack);
	ctx->vectorstack = vhead;
}

static void
gotrdelim(simpctx_t ctx)
{
	simpptr_t parent, vector;
	simpint_t cnt;

	cnt = SIMP_UNPACK_FIXNUM(vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NMEMB));
	parent = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_PARENT);
	ctx->vectorstack = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NEXT);
	if (cnt == 0)
		return;
	vector = fillvector(ctx, cnt);
	if (!isnil(parent)) {
		vector_set(ctx, parent, vector_len(ctx, parent) - 1, vector);
	} else {
		ctx->readstack = cons(ctx, vector, cdr(ctx, ctx->readstack));
	}
}

static void
putstr(simpctx_t ctx, simpptr_t port, simpptr_t obj)
{
	simpint_t i;
	int c;

	for (i = 0; i < string_len(ctx, obj); i++) {
		c = SIMP_ACCESS_STRING(obj)[i];
		switch (c) {
		case '\"':
			PRINT(SIMP_ACCESS_PORT(port), "\\\"");
			break;
		case '\a':
			PRINT(SIMP_ACCESS_PORT(port), "\\a");
			break;
		case '\b':
			PRINT(SIMP_ACCESS_PORT(port), "\\b");
			break;
		case '\033':
			PRINT(SIMP_ACCESS_PORT(port), "\\e");
			break;
		case '\f':
			PRINT(SIMP_ACCESS_PORT(port), "\\f");
			break;
		case '\n':
			PRINT(SIMP_ACCESS_PORT(port), "\\n");
			break;
		case '\r':
			PRINT(SIMP_ACCESS_PORT(port), "\\r");
			break;
		case '\t':
			PRINT(SIMP_ACCESS_PORT(port), "\\t");
			break;
		case '\v':
			PRINT(SIMP_ACCESS_PORT(port), "\\v");
			break;
		default:
			if (iscntrl(c)) {
				PRINT(SIMP_ACCESS_PORT(ctx->oport), "\\x%x", c);
			} else {
				PRINT(SIMP_ACCESS_PORT(ctx->oport), "%c", c);
			}
			break;
		}
	}
}

static simpptr_t
simp_read(simpctx_t ctx)
{
	simpint_t len;
	int toktype, prevtok;
	char *tok;

	/*
	 * In order to make the reading process iterative (rather than
	 * recursive), we need to keep two stacks: one for the objs
	 * being read and one for the vectors/lists/pairs we created
	 * while reading.
	 *
	 * In the following examples, I'll use #t rather than #<true>
	 * and #f rather than #<false> to represent true and false.
	 * I'll also use a slash (/) to represent nil inside a vector.
	 *
	 *
	 *                  Example 1: reading (a b)
	 *                  ========================
	 *
	 * (0) Before reading, both the vector stack and the read
	 * stack are empty.
	 *
	 *      readstack----> nil
	 *      vectorstack--> nil
	 *
	 * (1) We read a "(".  We call gotldelim(), which creates a new
	 * empty vector (nil), and pushes it into the read stack.  Since
	 * we're building a vector, we push four objects into the vector
	 * stack: the parent of the vector if it is virtual (nil, in
	 * this case), the current number of elements (zero), and
	 * whether the vector was created with a left parenthesis rather
	 * than with a square bracket (which is true in this case).
	 *
	 *                    ,-------.
	 *      readstack---->| / | / |
	 *                    `-------'
	 *                    ,---------------.
	 *      vectorstack-->| / | 0 | #t| / |
	 *                    `---------------'
	 *
	 * (2) Since the type of the last read token is TOK_LPAREN,
	 * virtual is false and a new virtual vector is not created.
	 *
	 * (3) We read an "a".  We call gotobject(), which gets a new
	 * symbol for "a" and pushes it into the read stack.  Since we
	 * added a new object to a vector, we increment the count of
	 * objects in the 4-tuple at the top of the vectorstack.
	 *
	 *                    ,-------.   ,-------.
	 *      readstack---->| a | +---->| / | / |
	 *                    `-------'   `-------'
	 *                    ,---------------.
	 *      vectorstack-->| / | 1 | #t| / |
	 *                    `---------------'
	 *
	 * (4) Since we're building a list (we're using parentheses) and
	 * the type of the last read token requires us to build a
	 * virtual vector, we should build a virtual vector.  The
	 * construction of a virtual vector occurs in four steps (see
	 * the code on newvirtualvector() to see how these four steps
	 * are implemented):
	 *
	 * (4.1) We get a new empty vector (nil), and push it into the
	 * read stack.  We also increment the counting of objects in the
	 * 4-tuple at the top of the vectorstack.
	 *
	 *                    ,-------.   ,-------.   ,-------.
	 *      readstack---->| / | +---->| a | +---->| / | / |
	 *                    `-------'   `-------'   `-------'
	 *                    ,---------------.
	 *      vectorstack-->| / | 2 | #t| / |
	 *                    `---------------'
	 *
	 * (4.2) We create a new vector whose size is equal to the
	 * counter of objects in the 4-tuple at the top of the
	 * vectorstack, we pop this much entries from the read stack and
	 * fill them into the new created vector (in reverse order).
	 * We can then pop the vectorstack.  We save the object pointed
	 * to by the VECTORSTACK_PARENT field of the top vector in the
	 * vector stack into a parent variable.
	 *
	 *      parent------->nil
	 *                    ,-------.
	 *      newvector---->| a | / |
	 *                    `-------'
	 *                    ,-------.
	 *      readstack---->| / | / |
	 *                    `-------'
	 *      vectorstack-->nil
	 *
	 * (4.3) We're in the case where parent is nil, so we pop the
	 * readstack and push newvector into it.
	 *
	 *      parent------->nil
	 *                    ,-------.
	 *      readstack---->| + | / |
	 *                    `-|-----'
	 *                      |
	 *                      V
	 *                    ,-------.
	 *                    | a | / |
	 *                    `-------'
	 *      vectorstack-->nil
	 *
	 * (4.4) We push a new 4-tuple vector into the vectorstack, to
	 * indicate we're build a new vector into the read stack.  This
	 * 4-tuple has the following information:
	 * - The vector we just built in the VECTORSTACK_PARENT field
	 * - zero in the VECTORSTACK_NMEMB field.
	 * - true in the VECTORSTACK_ISLIST field (we're in a list).
	 *
	 *                    ,-------.
	 *      readstack---->| + | / |
	 *                    `-|-----'
	 *                      |
	 *                      V
	 *                    ,-------.
	 *                    | a | / |
	 *                    `-------'
	 *                      ^
	 *                      |
	 *                    ,-|-------------.
	 *      vectorstack-->| + | 0 | #t| / |
	 *                    `---------------'
	 *
	 * (5) We read an "b".  We get a new symbol for "b" and push it
	 * into the read stack.  Since we added a new object to a
	 * vector, we increment the count of objects in the 4-tuple at
	 * the top of the vectorstack.
	 *
	 *                    ,-------.   ,-------.
	 *      readstack---->| b | +---->| + | / |
	 *                    `-------'   `-|-----'
	 *                                  |
	 *                                  V
	 *                                ,-------.
	 *                                | a | / |
	 *                                `-------'
	 *                                  ^
	 *                      .-----------'
	 *                    ,-|-------------.
	 *      vectorstack-->| + | 1 | #t| / |
	 *                    `---------------'
	 *
	 * (6) Since we're building a list and the type of the last read
	 * token requires us to build a virtual vector, we should build
	 * one.
	 *
	 * (6.1) We get a new empty vector (nil), and push it into the
	 * read stack.  We also increment the counting of objects in the
	 * 4-tuple at the top of the vectorstack.
	 *
	 *                    ,-------.   ,-------.   ,-------.
	 *      readstack---->| / | +---->| b | +---->| + | / |
	 *                    `-------'   `-------'   `-|-----'
	 *                                              |
	 *                                              V
	 *                                            ,-------.
	 *                                            | a | / |
	 *                                            `-------'
	 *                                              ^
	 *                      .-----------------------'
	 *                    ,-|-------------.
	 *      vectorstack-->| + | 2 | #t| / |
	 *                    `---------------'
	 *
	 * (6.2) We create a new 2-tuple vector, we pop this much
	 * entries from the read stack and fill them into the new
	 * created vector (in reverse order).  We then pop the
	 * vectorstack, saving the object pointed to by the
	 * VECTORSTACK_PARENT field into a parent variable.
	 *
	 *                    ,-------.
	 *      readstack---->| + | / |
	 *                    `-|-----'
	 *                      |
	 *                      V
	 *                    ,-------.
	 *      parent------->| a | / |
	 *                    `-------'
	 *                    ,-------.
	 *      newvector---->| b | / |
	 *                    `-------'
	 *      vectorstack-->nil
	 *
	 * (6.3) We're in the case where parent is not nil, so we set
	 * it's last element to the new created vector.
	 *
	 *                    ,-------.
	 *      readstack---->| + | / |
	 *                    `-|-----'
	 *                      |
	 *                      V
	 *                    ,-------.   ,-------.
	 *      parent------->| a | +---->| b | / |
	 *                    `-------'   `-------'
	 *      vectorstack-->nil
	 *
	 * (6.4) We push a new 4-tuple vector into the vectorstack, to
	 * indicate we're build a new vector into the read stack.  This
	 * 4-tuple has the following information:
	 * - The vector we just built in the VECTORSTACK_PARENT field
	 * - zero in the VECTORSTACK_NMEMB field.
	 * - true in the VECTORSTACK_ISLIST field (we're in a list).
	 *
	 *                    ,-------.
	 *      readstack---->| + | / |
	 *                    `-|-----'
	 *                      |
	 *                      V
	 *                    ,-------.   ,-------.
	 *                    | a | +---->| b | / |
	 *                    `-------'   `-------'
	 *                                  ^
	 *                      .-----------'
	 *                    ,-|-------------.
	 *      vectorstack-->| + | 0 | #t| / |
	 *                    `---------------'
	 *
	 * (7) We read a ")".  We call gotrdelim(), which saves the
	 * value at VECTORSTACK_PARENT of the topmost 4-tuple in
	 * vectorstack into a `parent` variable, and the value at
	 * VECTORSTACK_NMEMB into a `size` variable.  We then pop the
	 * vectorstack.  If the size is zero, we can return (and jump to
	 * step 8), that's the case.
	 *
	 *                    ,-------.
	 *      readstack---->| + | / |
	 *                    `-|-----'
	 *                      |
	 *                      V
	 *                    ,-------.   ,-------.
	 *                    | a | +---->| b | / |
	 *                    `-------'   `-------'
	 *                                  ^
	 *      parent----------------------'
	 *      size--------->0
	 *      vectorstack-->nil
	 *
	 * (8) readstack is not nil, and vectorstack is nil.  We exit
	 * the while loop.  We then return from this function returning
	 * the car() of the top element of readstack.
	 *
	 *                    ,-------.   ,-------.
	 *      return:       | a | +---->| b | / |
	 *                    `-------'   `-------'
	 */

	ctx->readstack = SIMP_IMM_NIL;
	ctx->vectorstack = SIMP_IMM_NIL;
	toktype = TOK_DOT;
	while (isnil(ctx->readstack) || !isnil(ctx->vectorstack)) {
		prevtok = toktype;
		toktype = gettok(ctx, ctx->iport, &tok, &len);
		if ((isnil(ctx->vectorstack) ||
		     istrue(vector_ref(ctx, ctx->vectorstack, VECTORSTACK_ISLIST))) &&
		    toktype != TOK_DOT &&
		    toktype != TOK_EOF &&
		    prevtok != TOK_LPAREN &&
		    prevtok != TOK_LBRACE &&
		    prevtok != TOK_DOT) {
			newvirtualvector(ctx);
		}
		switch (toktype) {
		case TOK_EOF:
			if (!isnil(ctx->readstack)) {
				fprintf(stderr, "unexpected EOF\n");
				abort();
			}
			return SIMP_IMM_EOF;
		case TOK_LPAREN:
			gotldelim(ctx, 1);
			break;
		case TOK_LBRACE:
			gotldelim(ctx, 0);
			break;
		case TOK_RPAREN:
			gotrdelim(ctx);
			break;
		case TOK_RBRACE:
			gotrdelim(ctx);
			break;
		case TOK_IDENTIFIER:
			gotobject(ctx, newsym(ctx, tok, len));
			break;
		case TOK_STRING:
			gotobject(ctx, newstr(ctx, tok, len, 0));
			break;
		case TOK_NUMBER:
			// TODO
			break;
		case TOK_DOT:
			if (isnil(ctx->vectorstack)) {
				fprintf(stderr, "unexpected '.'\n");
				abort();
			}
			break;
		case TOK_ERROR:
			// TODO
			break;
		default:
			fprintf(stderr, "this should not happen\n");
			abort();
			break;
		}
		free(tok);
	}
	return car(ctx, ctx->readstack);
}

static simpptr_t
simp_eval(simpctx_t ctx, simpptr_t obj)
{
	(void)ctx;
	return obj;
}

static void
simp_write(simpctx_t ctx, simpptr_t obj)
{
	simpptr_t curr;
	simpint_t len, i;
	int printspace;

	if (SIMP_IS_CONSTANT(obj)) {
		if (isfalse(obj))
			PRINT(SIMP_ACCESS_PORT(ctx->oport), "#<false>");
		else if (istrue(obj))
			PRINT(SIMP_ACCESS_PORT(ctx->oport), "#<false>");
		else if (iseof(obj))
			PRINT(SIMP_ACCESS_PORT(ctx->oport), "#<eof>");
		else if (isnil(obj))
			PRINT(SIMP_ACCESS_PORT(ctx->oport), "()");
		return;
	}
	if (SIMP_IS_FIXNUM(obj)) {
		// TODO: print fixnum
		return;
	}
	switch (SIMP_GET_MEMTAG(obj)) {
	case SIMP_MEMTAG_PORT:
		// TODO: print port;
		break;
	case SIMP_MEMTAG_STRING:
		if (!(SIMP_ACCESS_HEADER(obj) & SIMP_STRING_ISSYMBOL))
			PRINT(SIMP_ACCESS_PORT(ctx->oport), "\"");
		putstr(ctx, ctx->oport, obj);
		if (!(SIMP_ACCESS_HEADER(obj) & SIMP_STRING_ISSYMBOL))
			PRINT(SIMP_ACCESS_PORT(ctx->oport), "\"");
		break;
	case SIMP_MEMTAG_NUMBER:
		// TODO: print bignum
		break;
	case SIMP_MEMTAG_VECTOR:
		PRINT(SIMP_ACCESS_PORT(ctx->oport), "(");
		printspace = 0;
		while (!isnil(obj)) {
			if (printspace)
				PRINT(SIMP_ACCESS_PORT(ctx->oport), " ");
			printspace = 1;
			len = vector_len(ctx, obj);
			for (i = 0; i < len; i++) {
				curr = vector_ref(ctx, obj, i);
				if (i + 1 == len) {
					if (i > 0 && isvector(curr)) {
						obj = curr;
					} else {
						if (i > 0)
							PRINT(SIMP_ACCESS_PORT(ctx->oport), " . ");
						simp_write(ctx, curr);
						PRINT(SIMP_ACCESS_PORT(ctx->oport), " .");
						obj = SIMP_IMM_NIL;
					}
					break;
				} else {
					if (i > 0)
						PRINT(SIMP_ACCESS_PORT(ctx->oport), " . ");
					simp_write(ctx, curr);
				}
			}
		}
		PRINT(SIMP_ACCESS_PORT(ctx->oport), ")");
		break;
	}
}

simpctx_t
simp_init(FILE *ifp, FILE *ofp, FILE *efp)
{
	simpctx_t ctx;

	if ((ctx = malloc(sizeof(*ctx))) == NULL) {
		warn(ctx, "allocation error");
		return NULL;
	}
	ctx->iport = newport(ctx, ifp, SIMP_PORT_ISOPEN | SIMP_PORT_ISREADING);
	ctx->oport = newport(ctx, ofp, SIMP_PORT_ISOPEN | SIMP_PORT_ISWRITING);
	ctx->eport = newport(ctx, efp, SIMP_PORT_ISOPEN | SIMP_PORT_ISWRITING);
	ctx->symtab = newvector(ctx, SYMTABSIZE, SIMP_IMM_NIL, SIMP_VECTOR_ISMUTABLE);
	ctx->readstack = SIMP_IMM_NIL;
	ctx->vectorstack = SIMP_IMM_NIL;
	return ctx;
}

void
simp_repl(simpctx_t ctx)
{
	simpptr_t obj;

	while (PRINT(SIMP_ACCESS_PORT(ctx->oport), "> "), !iseof(obj = simp_read(ctx))) {
		obj = simp_eval(ctx, obj);
		simp_write(ctx, obj);
		PRINT(SIMP_ACCESS_PORT(ctx->oport), "\n");
	}
}

void
simp_clean(simpctx_t ctx)
{
	(void)ctx;
	// TODO: clean context
}
