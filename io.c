#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "simp.h"

#define STRBUFSIZE      124
#define MAXOCTALESCAPE  3

typedef struct Token Token;

enum Numtype {
	NUM_BINARY,
	NUM_OCTAL,
	NUM_DECIMAL,
	NUM_HEX,
};

struct Token {
	enum Toktype {
		TOK_CHAR,
		TOK_EOF,
		TOK_ERROR,
		TOK_FIXNUM,
		TOK_IDENTIFIER,
		TOK_LPAREN,
		TOK_QUOTE,
		TOK_REAL,
		TOK_RPAREN,
		TOK_STRING,
	}       type;
	union {
		SimpInt          fixnum;
		double            real;
		struct {
			unsigned char   *str;
			SimpSiz    len;
		} str;
	} u;
	SimpSiz lineno;
	SimpSiz column;
};

struct List {
	Simp obj;
	struct List *next;
};

static bool toktoobj(Simp ctx, Simp *obj, Simp port, Token tok);

static int
cisdecimal(int c)
{
	return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' ||
	       c == '5' || c == '6' || c == '7' || c == '8' || c == '9';
}

static int
cisspace(int c)
{
	return c == '\f' || c == '\n' || c == '\r' ||
               c == '\t' || c == '\v' || c == ' ';
}

static int
cisdelimiter(int c)
{
	return cisspace(c)  || c == '"' || c == '(' || c == ')' ||
	       c == NOTHING || c == '#';
}

static int
ciscntrl(int c)
{
	return c == '\x7f' || (c >= '\x00' && c <= '\x1f');
}

static int
cisoctal(int c)
{
	return c == '\0' || c == '\1' || c == '\2' || c == '\3' ||
	       c == '\4' || c == '\5' || c == '\6' || c == '\7';
}

static int
cishex(int c)
{
	return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' ||
	       c == '5' || c == '6' || c == '7' || c == '8' || c == '9' ||
	       c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' ||
	       c == 'F' || c == 'a' || c == 'b' || c == 'c' || c == 'd' ||
	       c == 'e' || c == 'f';
}

static SimpInt
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

static bool
cisnum(int c, enum Numtype type, SimpInt *num)
{
	switch (type) {
	case NUM_BINARY:
		if (c != '0' && c != '1')
			return false;
		*num <<= 1;
		if (c == '1')
			(*num)++;
		return true;
	case NUM_OCTAL:
		if (!cisoctal(c))
			return false;
		*num <<= 3;
		*num += ctoi(c);
		return true;
	case NUM_HEX:
		if (!cishex(c))
			return false;
		*num <<= 4;
		*num += ctoi(c);
		return true;
	case NUM_DECIMAL:
	default:
		if (!cisdecimal(c))
			return false;
		*num *= 10;
		*num += ctoi(c);
		return true;
	}
}

static int
readbyte(Simp port, int c)
{
	SimpSiz i = 0;
	unsigned char u = 0;

	if (c == NOTHING)
		return c;
	if (c != '\\')
		return c;
	if ((c = simp_readbyte(port)) == NOTHING)
		return c;
	switch (c) {
	case '"':
		return '\"';
	case 'a':
		return '\a';
	case 'b':
		return '\b';
	case 'e':
		return '\033';
	case 'f':
		return '\f';
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	case 't':
		return '\t';
	case 'v':
		return '\v';
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
loop:
		if (i >= 3 || c == NOTHING || !cisoctal(c)) {
			simp_unreadbyte(port, c);
			return u & 0xFF;
		}
		u <<= 3;
		u += ctoi(c);
		i++;
		c = simp_readbyte(port);
		goto loop;
	case 'x':
		if (!cishex(c = simp_readbyte(port))) {
			simp_unreadbyte(port, c);
			return 'x';
		}
		for (; cishex(c); c = simp_readbyte(port)) {
			u <<= 4;
			u += ctoi(c);
		}
		simp_unreadbyte(port, c);
		return u & 0xFF;
	case 'u':
		// TODO: handle 4-digit unicode
		return NOTHING;
	case 'U':
		// TODO: handle 4-digit unicode
		return NOTHING;
	default:
		return c;
	}
	/* UNREACHABLE */
	return NOTHING;
}

static Token
readstr(Simp port)
{
	Token tok = { 0 };
	SimpSiz size, len;
	unsigned char *str, *p;
	int c;

	tok.type = TOK_ERROR;
	tok.lineno = simp_portlineno(port);
	tok.column = simp_portcolumn(port);
	len = 0;
	size = STRBUFSIZE;
	if ((str = malloc(size)) == NULL)
		return tok;
	for (;;) {
		c = simp_readbyte(port);
		if (c == NOTHING)
			goto error;
		if (c == '"')
			break;
		c = readbyte(port, c);
		if (c == NOTHING)
			goto error;
		if (len + 1 >= size) {
			size <<= 2;
			if ((p = realloc(str, size)) == NULL)
				goto error;
			str = p;
		}
		str[len++] = (unsigned char)c;
	}
	str[len] = '\0';
	tok.type = TOK_STRING;
	tok.u.str.str = str;
	tok.u.str.len = len;
	return tok;
error:
	free(str);
	return tok;
}

static Token
readnum(Simp port, int c)
{
	enum Numtype numtype = NUM_DECIMAL;
	double floatn = 0.0;
	double expt = 0.0;
	SimpInt n = 0;
	SimpInt base = 0;
	SimpInt basesign = 1;
	SimpInt exptsign = 1;
	bool isfloat = false;
	Token tok = { 0 };

	tok.type = TOK_ERROR;
	tok.lineno = simp_portlineno(port);
	tok.column = simp_portcolumn(port);
	if (c == '+') {
		c = simp_readbyte(port);
	} else if (c == '-') {
		basesign = -1;
		c = simp_readbyte(port);
	}
	if (c == '0') {
		switch ((c = simp_readbyte(port))) {
		case 'B': case 'b':
			numtype = NUM_BINARY;
			break;
		case 'O': case 'o':
			numtype = NUM_OCTAL;
			break;
		case 'D': case 'd':
			numtype = NUM_DECIMAL;
			break;
		case 'X': case 'x':
			numtype = NUM_HEX;
			break;
		default:
			numtype = NUM_DECIMAL;
			break;
		}
	}
	for (;;) {
		if (!cisnum(c, numtype, &base))
			goto done;
		c = simp_readbyte(port);
	}
done:
	base *= basesign;
	if (c == '.') {
		while (cisnum(c = simp_readbyte(port), numtype, &n)) {
			floatn += (double)n;
			n = 0;
			switch (numtype) {
			case NUM_BINARY:
				floatn /= 2.0;
				break;
			case NUM_OCTAL:
				floatn /= 8.0;
				break;
			case NUM_HEX:
				floatn /= 16.0;
				break;
			case NUM_DECIMAL:
			default:
				floatn /= 10.0;
				break;
			}
		}
		floatn += (double)base;
		isfloat = true;
	}
	if (c == 'e' || c == 'E') {
		if (!isfloat)
			floatn = (double)base;
		isfloat = true;
		c = simp_readbyte(port);
		if (c == '+') {
			c = simp_readbyte(port);
		} else if (c == '-') {
			exptsign = -1;
			c = simp_readbyte(port);
		}
		n = 0;
		while (cisnum(c, numtype, &n)) {
			expt += (double)n;
			n = 0;
			switch (numtype) {
			case NUM_BINARY:
				expt *= 2.0;
				break;
			case NUM_OCTAL:
				expt *= 8.0;
				break;
			case NUM_HEX:
				expt *= 16.0;
				break;
			case NUM_DECIMAL:
			default:
				expt *= 10.0;
				break;
			}
			c = simp_readbyte(port);
		}
		if (exptsign == -1)
			expt = 1.0/expt;
		floatn = pow(floatn, expt);
	}
	if (!cisdelimiter(c))
		return tok;
	simp_unreadbyte(port, c);
	if (isfloat) {
		tok.type = TOK_REAL;
		tok.u.real = floatn;
	} else {
		tok.type = TOK_FIXNUM;
		tok.u.fixnum = base;
	}
	return tok;
}

static Token
readident(Simp port, int c)
{
	SimpSiz size = STRBUFSIZE;
	SimpSiz len = 0;
	unsigned char *str, *p;
	Token tok = { 0 };

	tok.type = TOK_ERROR;
	tok.lineno = simp_portlineno(port);
	tok.column = simp_portcolumn(port);
	if ((str = malloc(size)) == NULL)
		return tok;
	while (!cisdelimiter(c)) {
		if (len + 1 >= size) {
			size <<= 2;
			if ((p = realloc(str, size)) == NULL) {
				free(str);
				return tok;
			}
			str = p;
		}
		str[len++] = (unsigned char)c;
		c = simp_readbyte(port);
	}
	simp_unreadbyte(port, c);
	str[len] = '\0';
	tok.type = TOK_IDENTIFIER;
	tok.u.str.str = str;
	tok.u.str.len = len;
	return tok;
}

static Token
readtok(Simp port)
{
	Token tok = { 0 };
	int c;

loop:
	do {
		c = simp_readbyte(port);
	} while (c != NOTHING && cisspace(c));
	if (c != NOTHING && c == '#') {
		do {
			c = simp_readbyte(port);
		} while (c != NOTHING && c != '\n');
		if (c == '\n')
			goto loop;
	}
	if (c == NOTHING) {
		tok.type = TOK_EOF;
		return tok;
	}
	tok.lineno = simp_portlineno(port);
	tok.column = simp_portcolumn(port);
	switch (c) {
	case '\\':
		tok.type = TOK_QUOTE;
		return tok;
	case '(':
		tok.type = TOK_LPAREN;
		return tok;
	case ')':
		tok.type = TOK_RPAREN;
		return tok;
	case '"':
		return readstr(port);
	case '\'':
		c = simp_readbyte(port);
		c = readbyte(port, c);
		if (c == NOTHING) {
			tok.type = TOK_ERROR;
			return tok;
		}
		tok.u.fixnum = c;
		c = simp_readbyte(port);
		if (c == '\'') {
			tok.type = TOK_CHAR;
			return tok;
		} else {
			tok.type = TOK_ERROR;
			return tok;
		}
		tok.type = TOK_CHAR;
		return tok;
	case '+': case '-':
		if (!cisdecimal(simp_peekbyte(port)))
			goto token;
		/* FALLTHROUGH */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return readnum(port, c);
	default:
token:
		return readident(port, c);
	}
	/* UNREACHABLE */
	return tok;
}

static void
cleanvector(struct List *list)
{
	struct List *tmp;

	while (list != NULL) {
		tmp = list;
		list = list->next;
		free(tmp);
	}
}

static bool
fillvector(Simp ctx, Simp *vect, struct List *list, SimpSiz nitems, SimpSiz lineno, SimpSiz column, const char *filename)
{
	struct List *tmp;
	SimpSiz i = 0;

	if (!simp_makevector(ctx, vect, nitems) ||
	    !simp_setsource(ctx, vect, filename, lineno, column)) {
		cleanvector(list);
		return false;
	}
	while (list != NULL) {
		tmp = list;
		list = list->next;
		simp_setvector(*vect, i++, tmp->obj);
		free(tmp);
	}
	return true;
}

static bool
readvector(Simp ctx, Simp *vect, Simp port, SimpSiz lineno, SimpSiz column)
{
	Token tok;
	struct List *pair, *list, *last;
	SimpSiz nitems = 0;

	list = NULL;
	last = NULL;
	for (;;) {
		tok = readtok(port);
		switch (tok.type) {
		case TOK_ERROR:
		case TOK_EOF:
		case TOK_RPAREN:
			return fillvector(
				ctx,
				vect,
				list,
				nitems,
				lineno,
				column,
				simp_portfilename(port)
			);
		default:
			pair = malloc(sizeof(*pair));
			if (pair == NULL)
				goto error;
			if (!toktoobj(ctx, &pair->obj, port, tok))
				goto error;
			pair->next = NULL;
			if (last == NULL)
				list = pair;
			else
				last->next = pair;
			last = pair;
			nitems++;
			break;
		}
	}
error:
	cleanvector(list);
	return false;
}

static void
simp_printchar(Simp port, int c)
{
	switch (c) {
	case '\"':
		simp_printf(port, "\\\"");
		break;
	case '\a':
		simp_printf(port, "\\a");
		break;
	case '\b':
		simp_printf(port, "\\b");
		break;
	case '\033':
		simp_printf(port, "\\e");
		break;
	case '\f':
		simp_printf(port, "\\f");
		break;
	case '\n':
		simp_printf(port, "\\n");
		break;
	case '\r':
		simp_printf(port, "\\a");
		break;
	case '\t':
		simp_printf(port, "\\t");
		break;
	case '\v':
		simp_printf(port, "\\v");
		break;
	default:
		if (ciscntrl(c)) {
			simp_printf(port, "\\x%x", c);
		} else {
			simp_printf(port, "%c", c);
		}
		break;
	}
}

static void
simp_printbyte(Simp port, Simp obj)
{
	simp_printchar(port, (int)simp_getbyte(obj));
}

static void
simp_printsym(Simp port, unsigned char *str, SimpSiz len)
{
	SimpSiz i;

	for (i = 0; i < len; i++) {
		simp_printchar(port, (int)str[i]);
	}
}

static void
simp_printstr(Simp port, unsigned char *str, SimpSiz len)
{
	SimpSiz i;

	for (i = 0; i < len; i++) {
		simp_printf(port, "%c", str[i]);
	}
}

static bool
toktoobj(Simp ctx, Simp *obj, Simp port, Token tok)
{
	bool success;
	Simp quote, literal;
	const char *filename;
	SimpSiz lineno, column;

	filename = simp_portfilename(port);
	lineno = tok.lineno;
	column = tok.column;
	switch (tok.type) {
	case TOK_LPAREN:
		return readvector(ctx, obj, port, lineno, column);
	case TOK_IDENTIFIER:
		success = simp_makesymbol(
			ctx,
			obj,
			tok.u.str.str,
			tok.u.str.len
		) && simp_setsource(ctx, obj, filename, lineno, column);
		free(tok.u.str.str);
		return success;
	case TOK_QUOTE:
		if (!simp_makevector(ctx, obj, 2) ||
		    !simp_setsource(ctx, obj, filename, lineno, column)) {
			return false;
		}
		tok = readtok(port);
		if (!simp_makesymbol(ctx, &quote, (unsigned char *)"quote", 5))
			return false;
		if (!toktoobj(ctx, &literal, port, tok))
			return false;
		simp_setvector(*obj, 0, quote);
		simp_setvector(*obj, 1, literal);
		return true;
	case TOK_STRING:
		success = simp_makestring(
			ctx,
			obj,
			tok.u.str.str,
			tok.u.str.len
		);
		free(tok.u.str.str);
		return success;
	case TOK_REAL:
		return simp_makereal(ctx, obj, tok.u.real);
	case TOK_FIXNUM:
		return simp_makesignum(ctx, obj, tok.u.fixnum);
	case TOK_CHAR:
		return simp_makebyte(ctx, obj, (unsigned char)tok.u.fixnum);
	case TOK_EOF:
		*obj = simp_eof();
		return true;
	default:
		return false;
	}
}

bool
simp_read(Simp ctx, Simp *obj, Simp port)
{
	Token tok;

	tok = readtok(port);
	return toktoobj(ctx, obj, port, tok);
}

static void
dowrite(Simp port, Simp obj, bool display)
{
	Simp curr;
	SimpSiz len, i;

	switch (simp_gettype(obj)) {
	case TYPE_VOID:
	case TYPE_ENVIRONMENT:
		simp_printf(port, "#<environment>");
		break;
	case TYPE_EOF:
		simp_printf(port, "#<end-of-file>");
		break;
	case TYPE_FALSE:
		simp_printf(port, "#<false>");
		break;
	case TYPE_TRUE:
		simp_printf(port, "#<true>");
		break;
	case TYPE_BYTE:
		if (!display)
			simp_printf(port, "\'");
		simp_printbyte(port, obj);
		if (!display)
			simp_printf(port, "\'");
		break;
	case TYPE_SIGNUM:
		simp_printf(port, "%ld", simp_getsignum(obj));
		break;
	case TYPE_REAL:
		simp_printf(port, "%g", simp_getreal(obj));
		break;
	case TYPE_BUILTIN:
	case TYPE_CLOSURE:
		simp_printf(port, "#<procedure>");
		break;
	case TYPE_PORT:
		simp_printf(port, "#<port %p>", simp_getport(obj));
		break;
	case TYPE_STRING:
		if (!display)
			simp_printf(port, "\"");
		if (display)
			simp_printstr(port, simp_getstring(obj), simp_getsize(obj));
		else
			simp_printsym(port, simp_getstring(obj), simp_getsize(obj));
		if (!display)
			simp_printf(port, "\"");
		break;
	case TYPE_SYMBOL:
		simp_printsym(port, simp_getsymbol(obj), simp_getsize(obj));
		break;
	case TYPE_VECTOR:
		simp_printf(port, "(");
		len = simp_getsize(obj);
		for (i = 0; i < len; i++) {
			if (i > 0)
				simp_printf(port, " ");
			curr = simp_getvectormemb(obj, i);
			dowrite(port, curr, display);
		}
		simp_printf(port, ")");
		break;
	}
}

void
simp_write(Simp port, Simp obj)
{
	dowrite(port, obj, false);
}

void
simp_display(Simp port, Simp obj)
{
	dowrite(port, obj, true);
}
