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
	enum {
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
		SSimp          fixnum;
		double            real;
		struct {
			unsigned char   *str;
			SSimp    len;
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

static unsigned char
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
cisnum(int c, enum Numtype type, SSimp *num)
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
readbyte(Simp ctx, Simp port)
{
	SSimp i = 0;
	int c = NOTHING;
	unsigned char u = 0;

	if ((c = simp_readbyte(ctx, port)) == NOTHING)
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
	SSimp size, len;
	unsigned char *str, *p;
	int c;

	len = 0;
	size = STRBUFSIZE;
	if ((str = malloc(size)) == NULL)
		return (Token){.type = TOK_ERROR};
	for (;;) {
		c = simp_readbyte(ctx, port);
		if (c == NOTHING) // TODO: ERROR
			break;
		if (c == '"')
			break;
		simp_unreadbyte(ctx, port, c);
		if (len + 1 >= size) {
			size <<= 2;
			if ((p = realloc(str, size)) == NULL) {
				free(str);
				return (Token){.type = TOK_ERROR};
			}
			str = p;
		}
		str[len++] = readbyte(ctx, port);
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
	SSimp n = 0;
	SSimp base = 0;
	SSimp basesign = 1;
	SSimp exptsign = 1;
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
			simp_unreadbyte(ctx, port, c);
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
			floatn += n;
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
		floatn += base;
		isfloat = TRUE;
	}
	if (c == 'e' || c == 'E') {
		if (!isfloat)
			floatn = base;
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
			expt += n;
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
		return (Token){.type = TOK_EOF};
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
	SSimp size = STRBUFSIZE;
	SSimp len = 0;
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
		str[len++] = c;
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
	return tok;
}

static Simp
readlist(Simp ctx, Simp port)
{
	Token tok;
	Simp list = simp_nil();
	Simp last = simp_nil();
	Simp beg = simp_nil();
	Simp vect, fst, obj;
	SSimp i;
	int gotdot = FALSE;

	for (;;) {
		tok = readtok(ctx, port);
		switch (tok.type) {
		case TOK_ERROR:
		case TOK_EOF:
		case TOK_RBRACE:
			abort();
			break;
		case TOK_RPAREN:
			break;
		case TOK_DOT:
			if (simp_isnil(ctx, list))
				break;
			gotdot = TRUE;
			break;
		default:
			if (!gotdot)
				beg = last;
			obj = toktoobj(ctx, port, tok);
			vect = simp_cons(ctx, obj, simp_nil());
			if (simp_isnil(ctx, last)) {
				last = vect;
				list = last;
			} else {
				i = simp_getsize(ctx, last);
				simp_setvector(ctx, last, i - 1, vect);
				last = vect;
			}
			if (!gotdot)
				fst = vect;
			break;
		}
		if (tok.type == TOK_DOT)
			continue;
		if (gotdot) {
			i = 0;
			for (obj = fst;
			     !simp_isnil(ctx, obj);
			     obj = simp_cdr(ctx, obj))
				i++;
			if (tok.type != TOK_RPAREN)
				i++;
			vect = simp_makevector(ctx, i, simp_nil());
			i = 0;
			for (obj = fst;
			     !simp_isnil(ctx, obj);
			     obj = simp_cdr(ctx, obj))
				simp_setvector(ctx, vect, i++, simp_car(ctx, obj));
			if (simp_isnil(ctx, beg)) {
				list = vect;
			} else {
				i = simp_getsize(ctx, beg);
				simp_setvector(ctx, beg, i - 1, vect);
			}
			last = vect;
		}
		if (tok.type == TOK_RPAREN)
			return list;
		gotdot = FALSE;
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
	Simp vect, pair, obj;
	SSimp nitems, i;

	nitems = 0;
	for (;;) {
		tok = readtok(ctx, port);
		switch (tok.type) {
		case TOK_ERROR:
		case TOK_EOF:
		case TOK_RPAREN:
			abort();
			break;
		case TOK_RBRACE:
			vect = simp_makevector(ctx, nitems, simp_nil());
			i = 0;
			for (obj = list;
			     !simp_isnil(ctx, obj);
			     obj = simp_cdr(ctx, obj))
				simp_setvector(ctx, vect, i++, simp_car(ctx, obj));
			return vect;
		case TOK_DOT:
			break;
		default:
			obj = toktoobj(ctx, port, tok);
			pair = simp_cons(ctx, obj, simp_nil());
			if (simp_isnil(ctx, last)) {
				last = pair;
				list = last;
			} else {
				i = simp_getsize(ctx, last);
				simp_setvector(ctx, last, i - 1, pair);
				last = pair;
			}
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
simp_printstr(Simp ctx, Simp port, Simp obj)
{
	SSimp i, len;

	len = simp_getsize(ctx, obj);
	for (i = 0; i < len; i++) {
		simp_printchar(ctx, port, (int)simp_getstring(ctx, obj)[i]);
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
		obj = simp_makereal(ctx, tok.u.real);
		return obj;
	case TOK_FIXNUM:
		obj = simp_makenum(ctx, tok.u.fixnum);
		return obj;
	default:
		return simp_nil();
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
	SSimp len, i;
	int printspace;

	if (simp_isnil(ctx, obj)) {
		simp_printf(ctx, port, "()");
	} else if (simp_isbyte(ctx, obj)) {
		simp_printf(ctx, port, "\'");
		simp_printbyte(ctx, port, obj);
		simp_printf(ctx, port, "\'");
	} else if (simp_isnum(ctx, obj)) {
		simp_printf(ctx, port, "%ld", simp_getnum(ctx, obj));
	} else if (simp_isreal(ctx, obj)) {
		simp_printf(ctx, port, "%g", simp_getreal(ctx, obj));
	} else if (simp_isport(ctx, obj)) {
		simp_printf(ctx, port, "#<port %p>", simp_getport(ctx, obj));
	} else if (simp_isstring(ctx, obj)) {
		simp_printf(ctx, port, "\"");
		simp_printstr(ctx, port, obj);
		simp_printf(ctx, port, "\"");
	} else if (simp_issymbol(ctx, obj)) {
		simp_printstr(ctx, port, obj);
	} else if (simp_isvector(ctx, obj)) {
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
	}
}

void
simp_repl(Simp ctx)
{
	Simp obj, iport, oport;

	iport = simp_contextiport(ctx);
	oport = simp_contextoport(ctx);
	for (;;) {
		obj = simp_read(ctx, iport);
		if (simp_porteof(ctx, iport))
			break;
		if (simp_porterr(ctx, iport))
			break;
		simp_write(ctx, oport, obj);
		simp_printf(ctx, oport, "\n");
	}
}
