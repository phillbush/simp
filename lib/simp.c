#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <simp.h>

#define STRBUFSIZE      1028
#define NUMBUFSIZE      8
#define MAXOCTALESCAPE  3
#define MAXHEX4ESCAPE   4
#define MAXHEX8ESCAPE   8
#define SYMTABSIZE      389
#define SYMTABMULT      37

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
	TOK_PLUS,       /* special identifier */
	TOK_MINUS,      /* special identifier */
};

enum {
	/*
	 * indexes for the entries of the vector stack (check
	 * simp_read(), newvirtualvector(), and got*() routines).
	 */
	VECTORSTACK_PARENT  = 0,
	VECTORSTACK_NMEMB   = 1,
	VECTORSTACK_ISLIST  = 2,
	VECTORSTACK_NEXT    = 3,
	VECTORSTACK_SIZE    = 4,
};

typedef struct simp_cell {

	/*
	 * The simp_cell, along with the simp_context, are the two
	 * main data types used in simp code.
	 *
	 * A simp_cell is a pointer to a struct simp_cell, which
	 * maintains the type and the data of a simp object.
	 */

	enum {
		/* immediate types without standard external representation */
		TYPE_NIL,
		TYPE_EOF,
		TYPE_VOID,
		TYPE_TRUE,
		TYPE_FALSE,

		/* non-immediate types without standard external representation */
		TYPE_INPUT_STREAM,
		TYPE_INPUT_STRING,
		TYPE_OUTPUT_STREAM,
		TYPE_OUTPUT_STRING,

		/* types with standard external representation */
		TYPE_SYMBOL,
		TYPE_STRING,
		TYPE_VECTOR,
		TYPE_FIXNUM,
	} type;
	union {
		size_t fixnum;

		/* vector length */
		struct {
			struct simp_cell **arr;
			size_t len;
		} vector;

		/* string or symbol */
		struct {
			char *sp;
			size_t len;
		} str;

		/* file port */
		struct {
			FILE *fp;
			size_t lineno;
		} fport;

		/* string port */
		struct {
			char *start;
			char *curr;
		} sport;
	} v;
} *simp_cell;

typedef struct simp_context {

	/*
	 * The simp_context, along with the simp_cell, are the two
	 * main data types used in simp code.
	 *
	 * A simp_context is a pointer to a struct simp_context,
	 * which maintains all the data required for the interpreter to
	 * run, such as the stack, the environment, the symbol table,
	 * the current I/O ports, etc.  This object allows the
	 * co-existence of multiple interpreter instances in a single
	 * program.
	 *
	 * A simp_context is created by the simp_init() function and
	 * destroyed by the simp_clean() procedure.  Each interpreter
	 * should have a single simp_context that should be passed
	 * around to virtually all the functions in the library.
	 */

	simp_cell stack;
	simp_cell env;
	simp_cell symtab;             /* symbol hash table */

	/*
	 * Stacks used while reading expressions and building them
	 * (check simp_read(), newvirtualvector(), and got*() routines).
	 */
	simp_cell cellstack;
	simp_cell vectorstack;

	/*
	 * Current ports
	 */
	simp_cell iport;              /* current input port */
	simp_cell oport;              /* current output port */
	simp_cell eport;              /* current error port */
} *simp_context;

/* immediate types */
static simp_cell simp_nil = &(struct simp_cell){.type = TYPE_NIL, .v.vector.arr = NULL, .v.vector.len = 0};
static simp_cell simp_eof = &(struct simp_cell){.type = TYPE_EOF};
static simp_cell simp_void = &(struct simp_cell){.type = TYPE_VOID};
static simp_cell simp_true = &(struct simp_cell){.type = TYPE_TRUE};
static simp_cell simp_false = &(struct simp_cell){.type = TYPE_FALSE};

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
	/*
	 * Is this function really needed?
	 * Does locale affect isdigit(3)?
	 * Should we #include <ctype.h> again?
	 */
	return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' ||
	       c == '5' || c == '6' || c == '7' || c == '8' || c == '9';
}

static int
ishex(int c)
{
	/*
	 * Is this function really needed?
	 * Does locale affect isxdigit(3)?
	 * Should we #include <ctype.h> again?
	 */
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

static void *
xrealloc(void *p, size_t size)
{
	void *newp;

	if ((newp = realloc(p, size)) == NULL)
		free(p);
	return newp;
}

static int
xgetc(simp_cell port)
{
	int c;

	if (port->type == TYPE_INPUT_STRING) {
		if (*port->v.sport.curr == '\0') {
			return EOF;
		} else {
			return *port->v.sport.curr++;
		}
	} else {
		c = fgetc(port->v.fport.fp);
		if (c == '\n')
			port->v.fport.lineno++;
		return c;
	}
	// TODO: check for error
}

static void
xungetc(simp_cell port, int c)
{
	if (c == EOF)
		return;
	if (port->type == TYPE_INPUT_STRING) {
		if (port->v.sport.curr > port->v.sport.start) {
			*--port->v.sport.curr = c;
		}
	} else {
		ungetc(c, port->v.fport.fp);
	}
}

static void
xprintf(simp_cell port, const char *fmt, ...)
{
	va_list ap;

	if (port->type == TYPE_INPUT_STRING) {
		// TODO: write to string
	} else {
		va_start(ap, fmt);
		(void)vfprintf(port->v.fport.fp, fmt, ap);
		va_end(ap);
	}
}

static size_t
symtabhash(const char *str, size_t len)
{
	size_t i, h;

	h = 0;
	for (i = 0; i < len; i++)
		h = SYMTABMULT * h + str[i];
	return h % SYMTABSIZE;
}

static int
peekc(simp_cell port)
{
	int c;

	c = xgetc(port);
	xungetc(port, c);
	return c;
}

static void
warn(simp_context ctx, const char *fmt, ...)
{
	(void)ctx;
	(void)fmt;
	// TODO: handle error
}

static void
putstr(simp_context ctx, simp_cell port, simp_cell cell)
{
	size_t i;

	for (i = 0; i < cell->v.str.len; i++) {
		switch (cell->v.str.sp[i]) {
		case '\"':
			xprintf(port, "\\\"");
			break;
		case '\a':
			xprintf(port, "\\a");
			break;
		case '\b':
			xprintf(port, "\\b");
			break;
		case '\033':
			xprintf(port, "\\e");
			break;
		case '\f':
			xprintf(port, "\\f");
			break;
		case '\n':
			xprintf(port, "\\n");
			break;
		case '\r':
			xprintf(port, "\\r");
			break;
		case '\t':
			xprintf(port, "\\t");
			break;
		case '\v':
			xprintf(port, "\\v");
			break;
		default:
			if (iscntrl(cell->v.str.sp[i])) {
				xprintf(ctx->oport, "\\x%x", cell->v.str.sp[i]);
			} else {
				xprintf(ctx->oport, "%c", cell->v.str.sp[i]);
			}
			break;
		}
	}
}

static char *
getstr(simp_context ctx, simp_cell port, size_t *len)
{
	size_t j, size;
	int c;
	char *buf;

	size = STRBUFSIZE;
	if ((buf = malloc(size)) == NULL) {
		warn(ctx, "allocation error");
		return NULL;
	}
	*len = 0;
	for (;;) {
		c = xgetc(port);
		if (*len + 1 >= size) {
			size <<= 2;
			if ((buf = xrealloc(buf, size)) == NULL) {
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
		switch ((c = xgetc(port))) {
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
			for (j = 0; j < MAXOCTALESCAPE && isoctal(c); j++, c = xgetc(port))
				buf[*len] = (buf[*len] << 3) + ctoi(c);
			xungetc(port, c);
			(*len)++;
			break;
		case 'x':
			if (!ishex(c = xgetc(port))) {
				buf[(*len)++] = 'x';
				xungetc(port, c);
				break;
			}
			for (; ishex(c); c = xgetc(port))
				buf[*len] = (buf[*len] << 4) + ctoi(c);
			xungetc(port, c);
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
getnum(simp_context ctx, simp_cell port, size_t *len, int c)
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
	if ((buf = malloc(size)) == NULL) {
		warn(ctx, "allocation error");
		return NULL;
	}
	isexact = 1;
	*len = 1;
	if (c == '+' || c == '-') {
		buf[(*len)++] = c;
		c = xgetc(port);
	} else {
		buf[(*len)++] = '+';
	}
	if (c == '0') {
		switch (c = xgetc(port)) {
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
			xungetc(port, c);
			numtype = NUM_DECIMAL;
			break;
		}
	}
	for (;;) {
		if (*len + 2 >= size) {
			size <<= 2;
			if ((buf = xrealloc(buf, size)) == NULL) {
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
		c = xgetc(port);
	}
done:
	if (numtype == NUM_DECIMAL) {
		if (c == '.') {
			isexact = 0;
			buf[2] = 'F';
			buf[(*len)++] = '.';
			while (isdecimal(c = xgetc(port))) {
				if (*len + 2 >= size) {
					size <<= 2;
					if ((buf = xrealloc(buf, size)) == NULL) {
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
			c = xgetc(port);
			sign = '+';
			if (c == '+' || c == '-') {
				sign = c;
				c = xgetc(port);
			}
			if (!isdecimal(c)) {
				isexact = 1;
				goto checkdelimiter;
			}
			buf[(*len)++] = sign;
			do {
				if (*len + 2 >= size) {
					size <<= 2;
					if ((buf = xrealloc(buf, size)) == NULL) {
						warn(ctx, "allocation error");
						return NULL;
					}
				}
				buf[(*len)++] = c;
			} while (isdecimal(c = xgetc(port)));
		}
	}
	if (c == 'E' || c == 'e') {
		isexact = 1;
		c = xgetc(port);
	} else if (c == 'I' || c == 'i') {
		isexact = 0;
		c = xgetc(port);
	}
checkdelimiter:
	if (!(isdelimiter(c))) {
		free(buf);
		warn(ctx, "invalid numeric syntax: \"%c\"", c);
		return NULL;
	}
	xungetc(port, c);
	buf[0] = isexact ? 'E' : 'I';
	buf[*len] = '\0';
	return buf;
}

static char *
getident(simp_context ctx, simp_cell port, size_t *len, int c)
{
	size_t size;
	char *buf;

	size = STRBUFSIZE;
	if ((buf = malloc(size)) == NULL) {
		warn(ctx, "allocation error");
		return NULL;
	}
	*len = 0;
	while (!isdelimiter(c)) {
		if (*len + 1 >= size) {
			size <<= 2;
			if ((buf = xrealloc(buf, size)) == NULL) {
				warn(ctx, "allocation error");
				return NULL;
			}
		}
		buf[(*len)++] = c;
		c = xgetc(port);
	}
	xungetc(port, c);
	buf[*len] = '\0';
	return buf;
}

static int
gettok(simp_context ctx, simp_cell port, char **tok, size_t *len)
{
	int ret, c;

	*tok = NULL;
	*len = 0;
	while ((c = xgetc(port)) != EOF && isspace((unsigned char)c))
		;
	if (c == '#')
		while ((c = xgetc(port)) != '\n' && c != EOF)
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
		if (!isdecimal((unsigned int)peekc(port)))
			return (c == '+') ? TOK_PLUS : TOK_MINUS;
		/* FALLTHROUGH */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		*tok = getnum(ctx, port, len, c);
		ret = TOK_NUMBER;
		break;
	default:
		*tok = getident(ctx, port, len, c);
		ret = TOK_IDENTIFIER;
		break;
	}
	if (*tok == NULL)
		return TOK_ERROR;
	return ret;
}

static simp_cell
newcell(simp_context ctx, int type)
{
	simp_cell cell;

	(void)ctx;              // TODO: garbage collection
	if ((cell = malloc(sizeof(*cell))) == NULL)
		return NULL;
	cell->type = type;
	return cell;
}

static simp_cell
newfixnum(simp_context ctx, size_t fixnum)
{
	simp_cell cell;

	if ((cell = newcell(ctx, TYPE_FIXNUM)) == NULL)
		goto error;
	cell->v.fixnum = fixnum;
	return cell;
error:
	warn(ctx, "allocation error");
	return NULL;
}

static simp_cell
newstr(simp_context ctx, char *tok, size_t len)
{
	simp_cell cell;
	char *sp;

	if ((sp = malloc(len + 1)) == NULL)
		goto error;
	memcpy(sp, tok, len);
	sp[len] = '\0';
	if ((cell = newcell(ctx, TYPE_STRING)) == NULL)
		goto error;
	cell->v.str.sp = sp;
	cell->v.str.len = len;
	return cell;
error:
	warn(ctx, "allocation error");
	return NULL;
}

static simp_cell
newvector(simp_context ctx, size_t len, simp_cell value)
{
	simp_cell cell;
	simp_cell *arr;
	size_t i;

	if (len == 0)
		return simp_nil;
	if ((cell = newcell(ctx, TYPE_VECTOR)) == NULL)
		goto error;
	if ((arr = calloc(len, sizeof(*arr))) == NULL)
		goto error;
	if (value != NULL)
		for (i = 0; i < len; i++)
			arr[i] = value;
	cell->v.vector.arr = arr;
	cell->v.vector.len = len;
	return cell;
error:
	warn(ctx, "allocation error");
	return NULL;
}

static simp_cell
newport(simp_context ctx, void *p, int type)
{
	simp_cell cell;

	switch (type) {
	case TYPE_INPUT_STREAM:
	case TYPE_OUTPUT_STREAM:
		if ((cell = newcell(ctx, type)) == NULL)
			goto error;
		cell->v.fport.fp = (FILE *)p;
		cell->v.fport.lineno = 0;
		break;
	case TYPE_INPUT_STRING:
	case TYPE_OUTPUT_STRING:
		if ((cell = newcell(ctx, type)) == NULL)
			goto error;
		cell->v.sport.start = (char *)p;
		cell->v.sport.curr = cell->v.sport.start;
		break;
	}
	return cell;
error:
	warn(ctx, "allocation error");
	return NULL;
}

static void
vector_set(simp_context ctx, simp_cell vector, size_t index, simp_cell value)
{
	(void)ctx;
	vector->v.vector.arr[index] = value;
}

static simp_cell
vector_ref(simp_context ctx, simp_cell vector, size_t index)
{
	(void)ctx;
	return vector->v.vector.arr[index];
}

static size_t
vector_len(simp_context ctx, simp_cell vector)
{
	(void)ctx;
	return vector->v.vector.len;
}

static void
set_car(simp_context ctx, simp_cell pair, simp_cell value)
{
	vector_set(ctx, pair, 0, value);
}

static void
set_cdr(simp_context ctx, simp_cell pair, simp_cell value)
{
	vector_set(ctx, pair, 1, value);
}

static simp_cell
cons(simp_context ctx, simp_cell a, simp_cell b)
{
	simp_cell pair;

	pair = newvector(ctx, 2, NULL);
	set_car(ctx, pair, a);
	set_cdr(ctx, pair, b);
	return pair;
}

static simp_cell
car(simp_context ctx, simp_cell pair)
{
	return vector_ref(ctx, pair, 0);
}

static simp_cell
cdr(simp_context ctx, simp_cell pair)
{
	return vector_ref(ctx, pair, 1);
}

static simp_cell
newsym(simp_context ctx, char *tok, size_t len)
{
	simp_cell pair, list, sym;
	size_t bucket;
	char *sp;

	bucket = symtabhash(tok, len);
	list = vector_ref(ctx, ctx->symtab, bucket);
	for (pair = list; !simp_nil_p(ctx, pair); pair = cdr(ctx, pair)) {
		sym = car(ctx, pair);
		if (memcmp(tok, sym->v.str.sp, len) == 0) {
			return sym;
		}
	}
	if ((sym = newcell(ctx, TYPE_SYMBOL)) == NULL)
		goto error;
	if ((sp = malloc(len + 1)) == NULL)
		goto error;
	memcpy(sp, tok, len);
	sp[len] = '\0';
	sym->v.str.sp = sp;
	sym->v.str.len = len;
	pair = cons(ctx, sym, list);
	vector_set(ctx, ctx->symtab, bucket, pair);
	list = vector_ref(ctx, ctx->symtab, bucket);
	return sym;
error:
	warn(ctx, "allocation error");
	return NULL;
}

static simp_cell
fillvector(simp_context ctx, size_t len)
{
	simp_cell vector;
	size_t i;

	if (len == 0)
		return simp_nil;
	vector = newvector(ctx, len, NULL);
	for (i = 0; i < len; i++) {
		vector_set(ctx, vector, len - i - 1, car(ctx, ctx->cellstack));
		ctx->cellstack = cdr(ctx, ctx->cellstack);
	}
	return vector;
}

static void
newvirtualvector(simp_context ctx)
{
	simp_cell parent, vector, vhead, size;

	ctx->cellstack = cons(ctx, simp_nil, ctx->cellstack);
	parent = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_PARENT);
	size = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NMEMB);
	size->v.fixnum++;
	vector = fillvector(ctx, size->v.fixnum);
	ctx->vectorstack = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NEXT);
	if (simp_nil_p(ctx, parent))
		ctx->cellstack = cons(ctx, vector, cdr(ctx, ctx->cellstack));
	else
		vector_set(ctx, parent, vector_len(ctx, parent) - 1, vector);
	vhead = newvector(ctx, VECTORSTACK_SIZE, NULL);
	size = newfixnum(ctx, 0);
	vector_set(ctx, vhead, VECTORSTACK_PARENT, vector);
	vector_set(ctx, vhead, VECTORSTACK_NMEMB, size);
	vector_set(ctx, vhead, VECTORSTACK_ISLIST, simp_true);
	vector_set(ctx, vhead, VECTORSTACK_NEXT, ctx->vectorstack);
	ctx->vectorstack = vhead;
}

static void
gotobject(simp_context ctx, simp_cell cell)
{
	simp_cell size;

	ctx->cellstack = cons(ctx, cell, ctx->cellstack);
	if (ctx->vectorstack != simp_nil) {
		size = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NMEMB);
		size->v.fixnum++;
	}
}

static void
gotldelim(simp_context ctx, int isparens)
{
	simp_cell vhead, size;

	/* push simp_nil into cellstack */
	ctx->cellstack = cons(ctx, simp_nil, ctx->cellstack);
	if (!simp_nil_p(ctx, ctx->vectorstack)) {
		size = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NMEMB);
		size->v.fixnum++;
	}
	size = newfixnum(ctx, 0);
	vhead = newvector(ctx, VECTORSTACK_SIZE, NULL);
	vector_set(ctx, vhead, VECTORSTACK_PARENT, simp_nil);
	vector_set(ctx, vhead, VECTORSTACK_NMEMB, size);
	vector_set(ctx, vhead, VECTORSTACK_ISLIST, isparens ? simp_true : simp_false);
	vector_set(ctx, vhead, VECTORSTACK_NEXT, ctx->vectorstack);
	ctx->vectorstack = vhead;
}

static void
gotrdelim(simp_context ctx)
{
	simp_cell parent, vector, size;

	size = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NMEMB);
	parent = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_PARENT);
	ctx->vectorstack = vector_ref(ctx, ctx->vectorstack, VECTORSTACK_NEXT);
	if (size->v.fixnum == 0)
		return;
	vector = fillvector(ctx, size->v.fixnum);
	if (!simp_nil_p(ctx, parent)) {
		vector_set(ctx, parent, vector_len(ctx, parent) - 1, vector);
	} else {
		ctx->cellstack = cons(ctx, vector, cdr(ctx, ctx->cellstack));
	}
}

int
simp_eof_p(simp_context ctx, simp_cell cell)
{
	(void)ctx;
	return cell == simp_eof;
}

int
simp_void_p(simp_context ctx, simp_cell cell)
{
	(void)ctx;
	return cell == simp_void;
}

int
simp_true_p(simp_context ctx, simp_cell cell)
{
	(void)ctx;
	return cell == simp_true;
}

int
simp_false_p(simp_context ctx, simp_cell cell)
{
	(void)ctx;
	return cell == simp_false;
}

int
simp_nil_p(simp_context ctx, simp_cell cell)
{
	(void)ctx;
	return cell == simp_nil;
}

int
simp_vector_p(simp_context ctx, simp_cell cell)
{
	(void)ctx;
	return cell->type == TYPE_NIL || cell->type == TYPE_VECTOR;
}

simp_cell
simp_read(simp_context ctx)
{
	size_t len;
	char *tok;
	int toktype, prevtok;

	/*
	 * In order to make the reading process iterative (rather than
	 * recursive), we need to keep two stacks: one for the cells
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
	 * (0) Before reading, both the vector stack and the cell
	 * stack are empty.
	 *
	 *      cellstack----> nil
	 *      vectorstack--> nil
	 *
	 * (1) We read a "(".  We call gotldelim(), which creates a new
	 * empty vector (nil), and pushes it into the cell stack.  Since
	 * we're building a vector, we push four objects into the vector
	 * stack: the parent of the vector if it is virtual (nil, in
	 * this case), the current number of elements (zero), and
	 * whether the vector was created with a left parenthesis rather
	 * than with a square bracket (which is true in this case).
	 *
	 *                    ,-------.
	 *      cellstack---->| / | / |
	 *                    `-------'
	 *                    ,---------------.
	 *      vectorstack-->| / | 0 | #t| / |
	 *                    `---------------'
	 *
	 * (2) Since the type of the last read token is TOK_LPAREN,
	 * virtual is false and a new virtual vector is not created.
	 *
	 * (3) We read an "a".  We call gotobject(), which gets a new
	 * symbol for "a" and pushes it into the cell stack.  Since we
	 * added a new object to a vector, we increment the count of
	 * objects in the 4-tuple at the top of the vectorstack.
	 *
	 *                    ,-------.   ,-------.
	 *      cellstack---->| a | +---->| / | / |
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
	 * cell stack.  We also increment the counting of objects in the
	 * 4-tuple at the top of the vectorstack.
	 *
	 *                    ,-------.   ,-------.   ,-------.
	 *      cellstack---->| / | +---->| a | +---->| / | / |
	 *                    `-------'   `-------'   `-------'
	 *                    ,---------------.
	 *      vectorstack-->| / | 2 | #t| / |
	 *                    `---------------'
	 *
	 * (4.2) We create a new vector whose size is equal to the
	 * counter of objects in the 4-tuple at the top of the
	 * vectorstack, we pop this much entries from the cell stack and
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
	 *      cellstack---->| / | / |
	 *                    `-------'
	 *      vectorstack-->nil
	 *
	 * (4.3) We're in the case where parent is nil, so we pop the
	 * cellstack and push newvector into it.
	 *
	 *      parent------->nil
	 *                    ,-------.
	 *      cellstack---->| + | / |
	 *                    `-|-----'
	 *                      |
	 *                      V
	 *                    ,-------.
	 *                    | a | / |
	 *                    `-------'
	 *      vectorstack-->nil
	 *
	 * (4.4) We push a new 4-tuple vector into the vectorstack, to
	 * indicate we're build a new vector into the cell stack.  This
	 * 4-tuple has the following information:
	 * - The vector we just built in the VECTORSTACK_PARENT field
	 * - zero in the VECTORSTACK_NMEMB field.
	 * - true in the VECTORSTACK_ISLIST field (we're in a list).
	 *
	 *                    ,-------.
	 *      cellstack---->| + | / |
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
	 * into the cell stack.  Since we added a new object to a
	 * vector, we increment the count of objects in the 4-tuple at
	 * the top of the vectorstack.
	 *
	 *                    ,-------.   ,-------.
	 *      cellstack---->| b | +---->| + | / |
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
	 * cell stack.  We also increment the counting of objects in the
	 * 4-tuple at the top of the vectorstack.
	 *
	 *                    ,-------.   ,-------.   ,-------.
	 *      cellstack---->| / | +---->| b | +---->| + | / |
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
	 * entries from the cell stack and fill them into the new
	 * created vector (in reverse order).  We then pop the
	 * vectorstack, saving the object pointed to by the
	 * VECTORSTACK_PARENT field into a parent variable.
	 *
	 *                    ,-------.
	 *      cellstack---->| + | / |
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
	 *      cellstack---->| + | / |
	 *                    `-|-----'
	 *                      |
	 *                      V
	 *                    ,-------.   ,-------.
	 *      parent------->| a | +---->| b | / |
	 *                    `-------'   `-------'
	 *      vectorstack-->nil
	 *
	 * (6.4) We push a new 4-tuple vector into the vectorstack, to
	 * indicate we're build a new vector into the cell stack.  This
	 * 4-tuple has the following information:
	 * - The vector we just built in the VECTORSTACK_PARENT field
	 * - zero in the VECTORSTACK_NMEMB field.
	 * - true in the VECTORSTACK_ISLIST field (we're in a list).
	 *
	 *                    ,-------.
	 *      cellstack---->| + | / |
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
	 *      cellstack---->| + | / |
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
	 * (8) cellstack is not nil, and vectorstack is nil.  We exit
	 * the while loop.  We then return from this function returning
	 * the car() of the top element of cellstack.
	 *
	 *                    ,-------.   ,-------.
	 *      return:       | a | +---->| b | / |
	 *                    `-------'   `-------'
	 */

	ctx->cellstack = simp_nil;
	ctx->vectorstack = simp_nil;
	toktype = TOK_DOT;
	while (ctx->cellstack == simp_nil || ctx->vectorstack != simp_nil) {
		prevtok = toktype;
		toktype = gettok(ctx, ctx->iport, &tok, &len);
		if ((ctx->vectorstack == simp_nil ||
		     simp_true_p(ctx, vector_ref(ctx, ctx->vectorstack, VECTORSTACK_ISLIST))) &&
		    toktype != TOK_DOT &&
		    toktype != TOK_EOF &&
		    prevtok != TOK_LPAREN &&
		    prevtok != TOK_LBRACE &&
		    prevtok != TOK_DOT) {
			newvirtualvector(ctx);
		}
		switch (toktype) {
		case TOK_EOF:
			if (ctx->cellstack != simp_nil) {
				fprintf(stderr, "unexpected EOF\n");
				abort();
			}
			return simp_eof;
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
			gotobject(ctx, newstr(ctx, tok, len));
			break;
		case TOK_PLUS:
			gotobject(ctx, newsym(ctx, "+", 1));
			break;
		case TOK_MINUS:
			gotobject(ctx, newsym(ctx, "-", 1));
			break;
		case TOK_NUMBER:
			// TODO
			break;
		case TOK_DOT:
			if (ctx->vectorstack == simp_nil) {
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
	return car(ctx, ctx->cellstack);
}

simp_cell
simp_eval(simp_context ctx, simp_cell cell)
{
	(void)ctx;
	return cell;
}

simp_cell
simp_write(simp_context ctx, simp_cell cell)
{
	static char *representations[] = {
		[TYPE_NIL]   = "()",
		[TYPE_EOF]   = "#<eof>",
		[TYPE_VOID]  = "#<void>",
		[TYPE_TRUE]  = "#<true>",
		[TYPE_FALSE] = "#<false>",
	};
	simp_cell curr;
	size_t len, i;
	int space;

	// TODO: make simp_write iterative/tail-recursive?

	switch (cell->type) {
	case TYPE_NIL:
	case TYPE_EOF:
	case TYPE_VOID:
	case TYPE_TRUE:
	case TYPE_FALSE:
		xprintf(ctx->oport, "%s", representations[cell->type]);
		break;
	case TYPE_SYMBOL:
		putstr(ctx, ctx->oport, cell);
		break;
	case TYPE_STRING:
		xprintf(ctx->oport, "\"");
		putstr(ctx, ctx->oport, cell);
		xprintf(ctx->oport, "\"");
		break;
	case TYPE_INPUT_STREAM:
		xprintf(ctx->oport, "#<input-port %p>", cell->v.fport.fp);
		break;
	case TYPE_INPUT_STRING:
		xprintf(ctx->oport, "#<intput-port %p>", cell->v.sport.start);
		break;
	case TYPE_OUTPUT_STREAM:
		xprintf(ctx->oport, "#<output-port %p>", cell->v.fport.fp);
		break;
	case TYPE_OUTPUT_STRING:
		xprintf(ctx->oport, "#<output-port %p>", cell->v.sport.start);
		break;
	case TYPE_VECTOR:
		xprintf(ctx->oport, "(");
		space = 0;
		while (!simp_nil_p(ctx, cell)) {
			if (simp_nil_p(ctx, cell))
				break;
			if (space)
				xprintf(ctx->oport, " ");
			space = 1;
			len = vector_len(ctx, cell);
			for (i = 0; i < len; i++) {
				curr = vector_ref(ctx, cell, i);
				if (i + 1 == len) {
					if (i > 0 && simp_vector_p(ctx, curr)) {
						cell = curr;
					} else {
						if (i > 0)
							xprintf(ctx->oport, " . ");
						simp_write(ctx, curr);
						xprintf(ctx->oport, " .");
						cell = simp_nil;
					}
					break;
				} else {
					if (i > 0)
						xprintf(ctx->oport, " . ");
					simp_write(ctx, curr);
				}
			}
		}
		xprintf(ctx->oport, ")");
		break;
	default:
		break;
	}
	return simp_void;
}

simp_context
simp_init(void)
{
	simp_context ctx;

	if ((ctx = malloc(sizeof(*ctx))) == NULL) {
		warn(ctx, "allocation error");
		return NULL;
	}
	ctx->symtab = newvector(ctx, SYMTABSIZE, simp_nil);
	ctx->cellstack = simp_nil;
	ctx->vectorstack = simp_nil;
	return ctx;
}

void
simp_interactive(simp_context ctx, FILE *ifp, FILE *ofp, FILE *efp)
{
	ctx->iport = newport(ctx, ifp, TYPE_INPUT_STREAM);
	ctx->oport = newport(ctx, ofp, TYPE_OUTPUT_STREAM);
	ctx->eport = newport(ctx, efp, TYPE_OUTPUT_STREAM);
}
