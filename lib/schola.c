#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <schola.h>

#define STRBUFSIZE      1028
#define NUMBUFSIZE      8
#define MAXOCTALESCAPE  3
#define MAXHEX4ESCAPE   4
#define MAXHEX8ESCAPE   8

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
	TOK_SYMBOL,
	TOK_PLUS,       /* special symbol */
	TOK_MINUS,      /* special symbol */
};

/* objects */
typedef struct schola_cell {
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
		TYPE_NUMBER,
	} type;
	union {
		long l;
		double d;
		char *sym;

		/* vector length */
		struct {
			struct schola_cell **arr;
			size_t len;
		} vector;

		/* string */
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
} *schola_cell;

/* context of the interpreter */
typedef struct schola_context {
	schola_cell stack;
	schola_cell env;

	/* current ports */
	schola_cell iport;              /* current input port */
	schola_cell oport;              /* current output port */
	schola_cell eport;              /* current error port */

	/* are we in an interactive REPL? */
	int isinteractive;
} *schola_context;

/* cell stack for the read procedure */
typedef struct schola_cellstack {
	struct schola_cellstack *next;
	schola_cell cell;
} *schola_cellstack;

/* information of vector being building by read procedure */
typedef struct schola_vectorinfo {
	schola_cell parent;
	schola_cell vector;
	size_t size;
	enum {
		NOTATION_DOT,
		NOTATION_VIRTUAL,
		NOTATION_BRACE,
	} type;
} schola_vectorinfo;

/* vector stack for the read procedure */
typedef struct schola_vectorstack {
	struct schola_vectorstack *next;
	struct schola_vectorinfo v;
} *schola_vectorstack;

/* table of non-standard external representation */
static char *representations[] = {
	[TYPE_NIL]   = "()",
	[TYPE_EOF]   = "#<eof>",
	[TYPE_VOID]  = "#<void>",
	[TYPE_TRUE]  = "#<true>",
	[TYPE_FALSE] = "#<false>",
};

/* immediate types */
static schola_cell schola_nil = &(struct schola_cell){.type = TYPE_NIL};
static schola_cell schola_eof = &(struct schola_cell){.type = TYPE_EOF};
static schola_cell schola_void = &(struct schola_cell){.type = TYPE_VOID};
static schola_cell schola_true = &(struct schola_cell){.type = TYPE_TRUE};
static schola_cell schola_false = &(struct schola_cell){.type = TYPE_FALSE};

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
	/* Is this function really needed? Does locale affect isdigit(3)? */
	return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' ||
	       c == '5' || c == '6' || c == '7' || c == '8' || c == '9';
}

static int
ishex(int c)
{
	/* Is this function really needed? Does locale affect isxdigit(3)? */
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
xgetc(schola_cell port)
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
xungetc(schola_cell port, int c)
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
xprintf(schola_cell port, const char *fmt, ...)
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

static int
peekc(schola_cell port)
{
	int c;

	c = xgetc(port);
	xungetc(port, c);
	return c;
}

static void
warn(schola_context ctx, const char *fmt, ...)
{
	(void)ctx;
	(void)fmt;
	// TODO: handle error
}

static schola_cell
popcell(schola_context ctx, schola_cellstack *cellstack)
{
	schola_cellstack entry;
	schola_cell cell;

	(void)ctx;
	if (*cellstack == NULL) {
		abort();
		// TODO: complain about empty stack;
	}
	entry = *cellstack;
	cell = entry->cell;
	*cellstack = (*cellstack)->next;
	free(entry);
	return cell;
}

static schola_vectorinfo
popvector(schola_context ctx, schola_vectorstack *vectorstack)
{
	schola_vectorstack entry;
	schola_vectorinfo v;

	(void)ctx;
	if (*vectorstack == NULL) {
		abort();
		// TODO: complain about empty stack;
	}
	entry = *vectorstack;
	v = entry->v;
	*vectorstack = (*vectorstack)->next;
	free(entry);
	return v;
}

static void
pushcell(schola_context ctx, schola_cellstack *cellstack, schola_cell cell)
{
	schola_cellstack entry;

	(void)ctx;
	if ((entry = malloc(sizeof(*entry))) == NULL) {
		// TODO: complain about no memory
	}
	entry->next = *cellstack;
	entry->cell = cell;
	*cellstack = entry;
}

static void
pushvector(schola_context ctx, schola_vectorstack *vectorstack, schola_cell parent, schola_cell vector, int type)
{
	schola_vectorstack entry;

	(void)ctx;
	if ((entry = malloc(sizeof(*entry))) == NULL) {
		// TODO: complain about no memory
	}
	entry->next = *vectorstack;
	entry->v.size = 0;
	entry->v.type = type;
	entry->v.parent = parent;
	entry->v.vector = vector;
	*vectorstack = entry;
}

static void
putstr(schola_context ctx, schola_cell port, schola_cell cell)
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
getstr(schola_context ctx, schola_cell port, size_t *len)
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
getnum(schola_context ctx, schola_cell port, size_t *len, int c)
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
			while (isdigit(c = xgetc(port))) {
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
			if (!isdigit(c)) {
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
			} while (isdigit(c = xgetc(port)));
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
getsym(schola_context ctx, schola_cell port, size_t *len, int c)
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
gettok(schola_context ctx, schola_cell port, char **tok, size_t *len)
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
		if (!isdigit((unsigned int)peekc(port)))
			return (c == '+') ? TOK_PLUS : TOK_MINUS;
		/* FALLTHROUGH */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		*tok = getnum(ctx, port, len, c);
		ret = TOK_NUMBER;
		break;
	default:
		*tok = getsym(ctx, port, len, c);
		ret = TOK_SYMBOL;
		break;
	}
	if (*tok == NULL)
		return TOK_ERROR;
	return ret;
}

static schola_cell
newcell(schola_context ctx, int type)
{
	schola_cell cell;

	(void)ctx;              // TODO: garbage collection
	if ((cell = malloc(sizeof(*cell))) == NULL)
		return NULL;
	cell->type = type;
	return cell;
}

static schola_cell
newstr(schola_context ctx, char *tok, size_t len)
{
	schola_cell cell;
	char *sp;

	if ((sp = malloc(len)) == NULL)
		goto error;
	memcpy(sp, tok, len);
	free(tok);
	if ((cell = newcell(ctx, TYPE_STRING)) == NULL)
		goto error;
	cell->v.str.sp = sp;
	cell->v.str.len = len;
	return cell;
error:
	free(tok);
	warn(ctx, "allocation error");
	return NULL;
}

static schola_cell
newsym(schola_context ctx, char *tok, size_t len)
{
	schola_cell cell;
	char *sp;

	if ((sp = malloc(len)) == NULL)
		goto error;
	memcpy(sp, tok, len);
	free(tok);
	if ((cell = newcell(ctx, TYPE_SYMBOL)) == NULL)
		goto error;
	cell->v.sym = sp;
	return cell;
error:
	free(tok);
	warn(ctx, "allocation error");
	return NULL;
}

static schola_cell
newvector(schola_context ctx)
{
	schola_cell cell;

	if ((cell = newcell(ctx, TYPE_VECTOR)) == NULL)
		goto error;
	cell->v.vector.arr = NULL;
	cell->v.vector.len = 0;
	return cell;
error:
	warn(ctx, "allocation error");
	return NULL;
}

static schola_cell
newport(schola_context ctx, void *p, int type)
{
	schola_cell cell;

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
virtualvector(schola_context ctx, schola_cellstack *cellstack, schola_vectorstack *vectorstack)
{
	schola_vectorinfo vectorinfo;
	schola_cell *arr;
	schola_cell vector;
	size_t i;

	vector = schola_nil;
	vectorinfo = popvector(ctx, vectorstack);
	vectorinfo.size++;
	if ((arr = calloc(vectorinfo.size, sizeof(*arr))) == NULL) {
		fprintf(stderr, "no memory\n");
		abort();
	}
	pushcell(ctx, cellstack, vector);
	for (i = 0; i < vectorinfo.size; i++) {
		arr[vectorinfo.size - i - 1] = popcell(ctx, cellstack);
	}
	if ((vectorinfo.vector = newvector(ctx)) == NULL) {
		// TODO: complain about no memory
	}
	vectorinfo.vector->v.vector.arr = arr;
	vectorinfo.vector->v.vector.len = vectorinfo.size;
	if (vectorinfo.parent == NULL) {
		popcell(ctx, cellstack);
		pushcell(ctx, cellstack, vectorinfo.vector);
	} else {
		vectorinfo.parent->v.vector.arr[vectorinfo.parent->v.vector.len - 1] = vectorinfo.vector;
	}
	pushvector(ctx, vectorstack, vectorinfo.vector, vector, NOTATION_VIRTUAL);
}

static void
gotobject(schola_context ctx, schola_cellstack *cellstack, schola_vectorstack *vectorstack, schola_cell cell)
{
	if (cell == NULL) {
		// TODO: complain about no memory
	}
	pushcell(ctx, cellstack, cell);
	if (*vectorstack != NULL) {
		(*vectorstack)->v.size++;
	}
}

static void
gotldelim(schola_context ctx, schola_cellstack *cellstack, schola_vectorstack *vectorstack, int isbrace)
{
	pushcell(ctx, cellstack, schola_nil);
	if (*vectorstack != NULL)
		(*vectorstack)->v.size++;
	pushvector(ctx, vectorstack, NULL, schola_nil, isbrace ? NOTATION_BRACE : NOTATION_DOT);
}

static void
gotrdelim(schola_context ctx, schola_cellstack *cellstack, schola_vectorstack *vectorstack)
{
	schola_vectorinfo vectorinfo;
	schola_cell *arr;
	schola_cell vector;
	size_t i;

	vectorinfo = popvector(ctx, vectorstack);
	if (vectorinfo.type == NOTATION_VIRTUAL)
		return;
	if (vectorinfo.size == 0)
		return;
	if ((arr = calloc(vectorinfo.size, sizeof(*arr))) == NULL) {
		fprintf(stderr, "no memory\n");
		abort();
	}
	for (i = 0; i < vectorinfo.size; i++) {
		arr[vectorinfo.size - i - 1] = popcell(ctx, cellstack);
	}
	vector = popcell(ctx, cellstack);
	if ((vector = newvector(ctx)) == NULL) {
		// TODO: complain about no memory
	}
	vector->v.vector.arr = arr;
	vector->v.vector.len = vectorinfo.size;
	pushcell(ctx, cellstack, vector);
}

int
schola_eof_p(schola_context ctx, schola_cell cell)
{
	(void)ctx;
	return cell == schola_eof;
}

int
schola_void_p(schola_context ctx, schola_cell cell)
{
	(void)ctx;
	return cell == schola_void;
}

int
schola_true_p(schola_context ctx, schola_cell cell)
{
	(void)ctx;
	return cell == schola_true;
}

int
schola_false_p(schola_context ctx, schola_cell cell)
{
	(void)ctx;
	return cell == schola_false;
}

int
schola_nil_p(schola_context ctx, schola_cell cell)
{
	return cell == schola_nil;
}

int
schola_vector_p(schola_context ctx, schola_cell cell)
{
	(void)ctx;
	return cell->type == TYPE_NIL || cell->type == TYPE_VECTOR;
}

schola_cell
schola_read(schola_context ctx)
{
	schola_vectorstack vectorstack = NULL;
	schola_cellstack cellstack = NULL;
	size_t len;
	char *tok;
	int virtual, toktype, prevtok;

	toktype = TOK_DOT;
	while (cellstack == NULL || vectorstack != NULL) {
		prevtok = toktype;
		toktype = gettok(ctx, ctx->iport, &tok, &len);
		virtual = (vectorstack == NULL || vectorstack->v.type != NOTATION_BRACE) &&
		          toktype != TOK_DOT &&
		          toktype != TOK_EOF &&
		          prevtok != TOK_LPAREN &&
		          prevtok != TOK_LBRACE &&
		          prevtok != TOK_DOT;
		if (virtual) {
			virtualvector(ctx, &cellstack, &vectorstack);
		}
		switch (toktype) {
		case TOK_EOF:
			if (cellstack != NULL) {
				fprintf(stderr, "unexpected EOF\n");
				abort();
			}
			return schola_eof;
		case TOK_SYMBOL:
			gotobject(ctx, &cellstack, &vectorstack, newsym(ctx, tok, len));
			break;
		case TOK_STRING:
			gotobject(ctx, &cellstack, &vectorstack, newstr(ctx, tok, len));
			break;
		case TOK_LPAREN:
			gotldelim(ctx, &cellstack, &vectorstack, 0);
			break;
		case TOK_RPAREN:
			gotrdelim(ctx, &cellstack, &vectorstack);
			break;
		case TOK_LBRACE:
			gotldelim(ctx, &cellstack, &vectorstack, 1);
			break;
		case TOK_RBRACE:
			gotrdelim(ctx, &cellstack, &vectorstack);
			break;
		case TOK_PLUS:
		case TOK_MINUS:
		case TOK_NUMBER:
		case TOK_ERROR:
			// TODO
			break;
		case TOK_DOT:
			if (vectorstack == NULL) {
				fprintf(stderr, "unexpected '.'\n");
				abort();
			}
			break;
		default:
			fprintf(stderr, "this should not happen\n");
			abort();
			break;
		}
	}
	return popcell(ctx, &cellstack);
}

schola_cell
schola_eval(schola_context ctx, schola_cell cell)
{
	(void)ctx;
	return cell;
}

schola_cell
schola_write(schola_context ctx, schola_cell cell)
{
	size_t i;
	int space;

	switch (cell->type) {
	case TYPE_NIL:
	case TYPE_EOF:
	case TYPE_VOID:
	case TYPE_TRUE:
	case TYPE_FALSE:
		xprintf(ctx->oport, "%s", representations[cell->type]);
		break;
	case TYPE_SYMBOL:
		xprintf(ctx->oport, "%s", cell->v.sym);
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
		while (cell != NULL) {
			if (schola_nil_p(ctx, cell))
				break;
			if (space)
				xprintf(ctx->oport, " ");
			space = 1;
			for (i = 0; i < cell->v.vector.len; i++) {
				if (i + 1 == cell->v.vector.len) {
					if (i > 0 && schola_vector_p(ctx, cell->v.vector.arr[i])) {
						cell = cell->v.vector.arr[i];
					} else {
						if (i > 0)
							xprintf(ctx->oport, " . ");
						schola_write(ctx, cell->v.vector.arr[i]);
						xprintf(ctx->oport, " .");
						cell = NULL;
					}
					break;
				} else {
					if (i > 0)
						xprintf(ctx->oport, " . ");
					schola_write(ctx, cell->v.vector.arr[i]);
				}
			}
		}
		xprintf(ctx->oport, ")");
		break;
	default:
		break;
	}
	return schola_void;
}

schola_context
schola_init(void)
{
	schola_context ctx;

	if ((ctx = malloc(sizeof(*ctx))) == NULL) {
		warn(ctx, "allocation error");
		return NULL;
	}
	ctx->isinteractive = 0;
	return ctx;
}

void
schola_interactive(schola_context ctx, FILE *ifp, FILE *ofp, FILE *efp)
{
	ctx->isinteractive = 1;
	ctx->iport = newport(ctx, ifp, TYPE_INPUT_STREAM);
	ctx->oport = newport(ctx, ofp, TYPE_OUTPUT_STREAM);
	ctx->eport = newport(ctx, efp, TYPE_OUTPUT_STREAM);
}
