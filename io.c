#include <stdarg.h>
#include <stdlib.h>
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
		TOK_ERROR,
		TOK_EOF,
		TOK_LPAREN,
		TOK_RPAREN,
		TOK_LBRACE,
		TOK_RBRACE,
		TOK_DOT,
		TOK_CHAR,
		TOK_STRING,
		TOK_FIXNUM,
		TOK_REAL,
		TOK_IDENTIFIER,
	}       type;
	union {
		SimpInt          fixnum;
		double            real;
		struct {
			unsigned char   *str;
			SimpSiz    len;
		} str;
	} u;
};

static Simp toktoobj(Simp ctx, Simp port, Token tok);

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
	       c == NOTHING || c == '#' || c == '[' || c == ']';
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

static int
cisnum(int c, enum Numtype type, SimpInt *num)
{
	switch (type) {
	case NUM_BINARY:
		if (c != '0' && c != '1')
			return FALSE;
		*num <<= 1;
		if (c == '1')
			(*num)++;
		return TRUE;
	case NUM_OCTAL:
		if (!cisoctal(c))
			return FALSE;
		*num <<= 3;
		*num += ctoi(c);
		return TRUE;
	case NUM_HEX:
		if (!cishex(c))
			return FALSE;
		*num <<= 4;
		*num += ctoi(c);
		return TRUE;
	case NUM_DECIMAL:
	default:
		if (!cisdecimal(c))
			return FALSE;
		*num *= 10;
		*num += ctoi(c);
		return TRUE;
	}
}

static int
readbyte(Simp ctx, Simp port, int c)
{
	SimpSiz i = 0;
	unsigned char u = 0;

	if (c == NOTHING)
		return c;
	if (c != '\\')
		return c;
	if ((c = simp_readbyte(ctx, port)) == NOTHING)
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
			simp_unreadbyte(ctx, port, c);
			return u & 0xFF;
		}
		u <<= 3;
		u += ctoi(c);
		i++;
		c = simp_readbyte(ctx, port);
		goto loop;
	case 'x':
		if (!cishex(c = simp_readbyte(ctx, port))) {
			simp_unreadbyte(ctx, port, c);
			return 'x';
		}
		for (; cishex(c); c = simp_readbyte(ctx, port)) {
			u <<= 4;
			u += ctoi(c);
		}
		simp_unreadbyte(ctx, port, c);
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
readstr(Simp ctx, Simp port)
{
	SimpSiz size, len;
	unsigned char *str, *p;
	int c;

	len = 0;
	size = STRBUFSIZE;
	if ((str = malloc(size)) == NULL)
		return (Token){.type = TOK_ERROR};
	for (;;) {
		c = simp_readbyte(ctx, port);
		if (c == NOTHING)
			return (Token){ .type = TOK_ERROR };
		if (c == '"')
			break;
		c = readbyte(ctx, port, c);
		if (c == NOTHING)
			return (Token){ .type = TOK_ERROR };
		if (len + 1 >= size) {
			size <<= 2;
			if ((p = realloc(str, size)) == NULL) {
				free(str);
				return (Token){.type = TOK_ERROR};
			}
			str = p;
		}
		str[len++] = (unsigned char)c;
	}
	str[len] = '\0';
	return (Token){
		.type = TOK_STRING,
		.u.str.str = str,
		.u.str.len = len,
	};
}

static Token
readnum(Simp ctx, Simp port, int c)
{
	enum Numtype numtype = NUM_DECIMAL;
	double floatn = 0.0;
	double expt = 0.0;
	SimpInt n = 0;
	SimpInt base = 0;
	SimpInt basesign = 1;
	SimpInt exptsign = 1;
	int isfloat = FALSE;

	if (c == '+') {
		c = simp_readbyte(ctx, port);
	} else if (c == '-') {
		basesign = -1;
		c = simp_readbyte(ctx, port);
	}
	if (c == '0') {
		switch ((c = simp_readbyte(ctx, port))) {
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
		c = simp_readbyte(ctx, port);
	}
done:
	base *= basesign;
	if (c == '.') {
		while (cisnum(c = simp_readbyte(ctx, port), numtype, &n)) {
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
		isfloat = TRUE;
	}
	if (c == 'e' || c == 'E') {
		if (!isfloat)
			floatn = (double)base;
		isfloat = TRUE;
		c = simp_readbyte(ctx, port);
		if (c == '+') {
			c = simp_readbyte(ctx, port);
		} else if (c == '-') {
			exptsign = -1;
			c = simp_readbyte(ctx, port);
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
			c = simp_readbyte(ctx, port);
		}
		if (exptsign == -1)
			expt = 1.0/expt;
		floatn = pow(floatn, expt);
		isfloat = TRUE;
	}
	if (!cisdelimiter(c))
		return (Token){.type = TOK_ERROR};
	simp_unreadbyte(ctx, port, c);
	if (isfloat) {
		return (Token){
			.type = TOK_REAL,
			.u.real = floatn,
		};
	} else {
		return (Token){
			.type = TOK_FIXNUM,
			.u.fixnum = base,
		};
	}
}

static Token
readident(Simp ctx, Simp port, int c)
{
	SimpSiz size = STRBUFSIZE;
	SimpSiz len = 0;
	unsigned char *str, *p;

	if ((str = malloc(size)) == NULL)
		return (Token){.type = TOK_ERROR};
	while (!cisdelimiter(c)) {
		if (len + 1 >= size) {
			size <<= 2;
			if ((p = realloc(str, size)) == NULL) {
				free(str);
				return (Token){.type = TOK_ERROR};
			}
			str = p;
		}
		str[len++] = (unsigned char)c;
		c = simp_readbyte(ctx, port);
	}
	simp_unreadbyte(ctx, port, c);
	str[len] = '\0';
	return (Token){
		.type = TOK_IDENTIFIER,
		.u.str.str = str,
		.u.str.len = len,
	};
}

static Token
readtok(Simp ctx, Simp port)
{
	Token tok = { 0 };
	int c;

	do {
		c = simp_readbyte(ctx, port);
	} while (c != NOTHING && cisspace(c));
	if (cisspace(c))
	if (c != NOTHING && c == '#') {
		do {
			c = simp_readbyte(ctx, port);
		} while (c != NOTHING && c != '\n');
	}
	if (c == NOTHING) {
		tok.type = TOK_EOF;
		return tok;
	}
	switch (c) {
	case '(':
		tok.type = TOK_LPAREN;
		return tok;
	case ')':
		tok.type = TOK_RPAREN;
		return tok;
	case '[':
		tok.type = TOK_LBRACE;
		return tok;
	case ']':
		tok.type = TOK_RBRACE;
		return tok;
	case '.':
		tok.type = TOK_DOT;
		return tok;
	case '"':
		return readstr(ctx, port);
	case '\'':
		c = simp_readbyte(ctx, port);
		c = readbyte(ctx, port, c);
		if (c == NOTHING) {
			tok.type = TOK_ERROR;
			return tok;
		}
		tok.u.fixnum = c;
		c = simp_readbyte(ctx, port);
		if (c == '\'') {
			tok.type = TOK_CHAR;
			return tok;
		}
		while (c != NOTHING && c != '\'') {
			c = simp_readbyte(ctx, port);
		}
		tok.type = TOK_CHAR;
		return tok;
	case '+': case '-':
		if (!cisdecimal(simp_peekbyte(ctx, port)))
			goto token;
		/* FALLTHROUGH */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return readnum(ctx, port, c);
	default:
token:
		return readident(ctx, port, c);
	}
	/* UNREACHABLE */
	return tok;
}

static Simp
fillvector(Simp ctx, Simp list, SimpSiz nitems)
{
	Simp vect, obj;
	SimpSiz i = 0;

	vect = simp_makevector(ctx, nitems, simp_nil());
	// TODO: check if vector could not be built
	for (obj = list; !simp_isnil(ctx, obj); obj = simp_cdr(ctx, obj))
		simp_setvector(ctx, vect, i++, simp_car(ctx, obj));
	return vect;
}

static Simp
readlist(Simp ctx, Simp port)
{
	Token tok;
	enum Toktype prevtype = TOK_DOT;
	Simp list = simp_nil();
	Simp last = simp_nil();
	Simp vect = simp_nil();
	Simp prev = simp_nil();
	Simp pair, obj;
	SimpSiz nitems = 0;
	SimpSiz i;

	for (;;) {
		tok = readtok(ctx, port);
		switch (tok.type) {
		case TOK_ERROR:
		case TOK_EOF:
		case TOK_RBRACE:
			return simp_makeexception(ctx, ERROR_ILLEXPR);
			break;
		case TOK_RPAREN:
			i = (prevtype != TOK_DOT ? 1 : 0);
			if (simp_isnil(ctx, vect))
				return fillvector(ctx, list, nitems + i);
			obj = fillvector(ctx, list, nitems + i);
			i = simp_getsize(ctx, prev);
			simp_setvector(ctx, prev, i - 1, obj);
			return vect;
		case TOK_DOT:
			break;
		default:
			if (prevtype != TOK_DOT) {
				obj = fillvector(ctx, list, nitems + 1);
				if (simp_isnil(ctx, vect)) {
					vect = obj;
				} else {
					i = simp_getsize(ctx, prev);
					simp_setvector(ctx, prev, i - 1, obj);
				}
				prev = obj;
				list = simp_nil();
				nitems = 0;
			}
			obj = toktoobj(ctx, port, tok);
			pair = simp_cons(ctx, obj, simp_nil());
			if (simp_isnil(ctx, list))
				list = pair;
			else
				simp_setcdr(ctx, last, pair);
			last = pair;
			nitems++;
			break;
		}
		prevtype = tok.type;
	}
	/* UNREACHABLE */
	return list;
}

static Simp
readvector(Simp ctx, Simp port)
{
	Token tok;
	Simp list = simp_nil();
	Simp last = simp_nil();
	Simp pair, obj;
	SimpSiz nitems = 0;

	for (;;) {
		tok = readtok(ctx, port);
		switch (tok.type) {
		case TOK_ERROR:
		case TOK_EOF:
		case TOK_RPAREN:
			return simp_makeexception(ctx, ERROR_ILLEXPR);
			break;
		case TOK_RBRACE:
			return fillvector(ctx, list, nitems);
		case TOK_DOT:
			break;
		default:
			obj = toktoobj(ctx, port, tok);
			pair = simp_cons(ctx, obj, simp_nil());
			if (simp_isnil(ctx, last))
				list = pair;
			else
				simp_setcdr(ctx, last, pair);
			last = pair;
			nitems++;
			break;
		}
	}
	/* UNREACHABLE */
	return list;
}

static void
simp_printchar(Simp ctx, Simp port, int c)
{
	switch (c) {
	case '\"':
		simp_printf(ctx, port, "\\\"");
		break;
	case '\a':
		simp_printf(ctx, port, "\\a");
		break;
	case '\b':
		simp_printf(ctx, port, "\\b");
		break;
	case '\033':
		simp_printf(ctx, port, "\\e");
		break;
	case '\f':
		simp_printf(ctx, port, "\\f");
		break;
	case '\n':
		simp_printf(ctx, port, "\\n");
		break;
	case '\r':
		simp_printf(ctx, port, "\\a");
		break;
	case '\t':
		simp_printf(ctx, port, "\\t");
		break;
	case '\v':
		simp_printf(ctx, port, "\\v");
		break;
	default:
		if (ciscntrl(c)) {
			simp_printf(ctx, port, "\\x%x", c);
		} else {
			simp_printf(ctx, port, "%c", c);
		}
		break;
	}
}

static void
simp_printbyte(Simp ctx, Simp port, Simp obj)
{
	simp_printchar(ctx, port, (int)simp_getbyte(ctx, obj));
}

static void
simp_printstr(Simp ctx, Simp port, unsigned char *str, SimpSiz len)
{
	SimpSiz i;

	for (i = 0; i < len; i++) {
		simp_printchar(ctx, port, (int)str[i]);
	}
}

static Simp
toktoobj(Simp ctx, Simp port, Token tok)
{
	Simp obj;

	switch (tok.type) {
	case TOK_LPAREN:
		return readlist(ctx, port);
	case TOK_LBRACE:
		return readvector(ctx, port);
	case TOK_IDENTIFIER:
		obj = simp_makesymbol(ctx, tok.u.str.str, tok.u.str.len);
		free(tok.u.str.str);
		return obj;
	case TOK_STRING:
		obj = simp_makestring(ctx, tok.u.str.str, tok.u.str.len);
		free(tok.u.str.str);
		return obj;
	case TOK_REAL:
		return simp_makereal(ctx, tok.u.real);
	case TOK_FIXNUM:
		return simp_makenum(ctx, tok.u.fixnum);
	case TOK_CHAR:
		return simp_makebyte(ctx, (unsigned char)tok.u.fixnum);
	case TOK_EOF:
		return simp_eof();
	case TOK_RPAREN:
	case TOK_RBRACE:
	case TOK_DOT:
	case TOK_ERROR:
		return simp_makeexception(ctx, ERROR_ILLEXPR);
	}
}

Simp
simp_read(Simp ctx, Simp port)
{
	Token tok;

	tok = readtok(ctx, port);
	return toktoobj(ctx, port, tok);
}

void
simp_write(Simp ctx, Simp port, Simp obj)
{
	Simp curr;
	SimpSiz len, i;
	int printspace;

	switch (simp_gettype(ctx, obj)) {
	case TYPE_BINDING:
		simp_printf(ctx, port, "#<variable binding>");
		break;
	case TYPE_ENVIRONMENT:
		simp_printf(ctx, port, "#<environment>");
		break;
	case TYPE_EOF:
		simp_printf(ctx, port, "#<end-of-file>");
		break;
	case TYPE_FALSE:
		simp_printf(ctx, port, "#<false>");
		break;
	case TYPE_TRUE:
		simp_printf(ctx, port, "#<true>");
		break;
	case TYPE_BYTE:
		simp_printf(ctx, port, "\'");
		simp_printbyte(ctx, port, obj);
		simp_printf(ctx, port, "\'");
		break;
	case TYPE_SIGNUM:
		simp_printf(ctx, port, "%ld", simp_getnum(ctx, obj));
		break;
	case TYPE_REAL:
		simp_printf(ctx, port, "%g", simp_getreal(ctx, obj));
		break;
	case TYPE_BUILTIN:
	case TYPE_APPLICATIVE:
	case TYPE_OPERATIVE:
		simp_printf(ctx, port, "#<operation>");
		break;
	case TYPE_PORT:
		simp_printf(ctx, port, "#<port %p>", simp_getport(ctx, obj));
		break;
	case TYPE_STRING:
		simp_printf(ctx, port, "\"");
		simp_printstr(ctx, port, simp_getstring(ctx, obj), simp_getsize(ctx, obj));
		simp_printf(ctx, port, "\"");
		break;
	case TYPE_SYMBOL:
		simp_printstr(ctx, port, simp_getsymbol(ctx, obj), simp_getsize(ctx, obj));
		break;
	case TYPE_EXCEPTION:
		simp_printf(ctx, port, "ERROR: %s", simp_getexception(ctx, obj));
		break;
	case TYPE_VECTOR:
		simp_printf(ctx, port, "(");
		printspace = FALSE;
		while (!simp_isnil(ctx, obj)) {
			if (printspace)
				simp_printf(ctx, port, " ");
			printspace = TRUE;
			len = simp_getsize(ctx, obj);
			for (i = 0; i < len; i++) {
				curr = simp_getvectormemb(ctx, obj, i);
				if (i + 1 == len) {
					if (i > 0 && simp_isvector(ctx, curr)) {
						obj = curr;
					} else {
						if (i > 0)
							simp_printf(ctx, port, " . ");
						simp_write(ctx, port, curr);
						simp_printf(ctx, port, " .");
						obj = simp_nil();
					}
					break;
				} else {
					if (i > 0)
						simp_printf(ctx, port, " . ");
					simp_write(ctx, port, curr);
				}
			}
		}
		simp_printf(ctx, port, ")");
		break;
	}
}
