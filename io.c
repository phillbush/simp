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
		TOK_EXCLAM,
		TOK_LPAREN,
		TOK_RPAREN,
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

struct List {
	Simp obj;
	struct List *next;
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
	bool isfloat = false;

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
		isfloat = true;
	}
	if (c == 'e' || c == 'E') {
		if (!isfloat)
			floatn = (double)base;
		isfloat = true;
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
		isfloat = true;
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

loop:
	do {
		c = simp_readbyte(ctx, port);
	} while (c != NOTHING && cisspace(c));
	if (c != NOTHING && c == '#') {
		do {
			c = simp_readbyte(ctx, port);
		} while (c != NOTHING && c != '\n');
		if (c == '\n')
			goto loop;
	}
	if (c == NOTHING) {
		tok.type = TOK_EOF;
		return tok;
	}
	switch (c) {
	case '!':
		tok.type = TOK_EXCLAM;
		return tok;
	case '(':
		tok.type = TOK_LPAREN;
		return tok;
	case ')':
		tok.type = TOK_RPAREN;
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

static Simp
fillvector(Simp ctx, struct List *list, SimpSiz nitems)
{
	struct List *tmp;
	Simp vect;
	SimpSiz i = 0;

	vect = simp_makevector(ctx, nitems);
	if (simp_isexception(ctx, vect)) {
		cleanvector(list);
		return simp_makeexception(ctx, ERROR_MEMORY);
	}
	while (list != NULL) {
		tmp = list;
		list = list->next;
		simp_setvector(ctx, vect, i++, tmp->obj);
		free(tmp);
	}
	return vect;
}

static Simp
readvector(Simp ctx, Simp port)
{
	Token tok;
	struct List *pair, *list, *last;
	SimpSiz nitems = 0;

	list = NULL;
	last = NULL;
	for (;;) {
		tok = readtok(ctx, port);
		switch (tok.type) {
		case TOK_ERROR:
		case TOK_EOF:
		case TOK_RPAREN:
			return fillvector(ctx, list, nitems);
		default:
			pair = malloc(sizeof(*pair));
			if (pair == NULL) {
				cleanvector(list);
				return simp_makeexception(ctx, ERROR_MEMORY);
			}
			pair->obj = toktoobj(ctx, port, tok);
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
	/* UNREACHABLE */
	return simp_makeexception(ctx, -1);
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
simp_printsym(Simp ctx, Simp port, unsigned char *str, SimpSiz len)
{
	SimpSiz i;

	for (i = 0; i < len; i++) {
		simp_printchar(ctx, port, (int)str[i]);
	}
}

static void
simp_printstr(Simp ctx, Simp port, unsigned char *str, SimpSiz len)
{
	SimpSiz i;

	for (i = 0; i < len; i++) {
		simp_printf(ctx, port, "%c", str[i]);
	}
}

static Simp
toktoobj(Simp ctx, Simp port, Token tok)
{
	Simp obj;

	switch (tok.type) {
	case TOK_EXCLAM:
		return simp_makesymbol(ctx, (unsigned char *)"!", 1);
	case TOK_LPAREN:
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
	default:
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

static void
dowrite(Simp ctx, Simp port, Simp obj, bool display)
{
	Simp curr;
	SimpSiz len, i;

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
		if (!display)
			simp_printf(ctx, port, "\'");
		simp_printbyte(ctx, port, obj);
		if (!display)
			simp_printf(ctx, port, "\'");
		break;
	case TYPE_SIGNUM:
		simp_printf(ctx, port, "%ld", simp_getnum(ctx, obj));
		break;
	case TYPE_REAL:
		simp_printf(ctx, port, "%g", simp_getreal(ctx, obj));
		break;
	case TYPE_BUILTIN:
	case TYPE_CLOSURE:
		simp_printf(ctx, port, "#<procedure>");
		break;
	case TYPE_PORT:
		simp_printf(ctx, port, "#<port %p>", simp_getport(ctx, obj));
		break;
	case TYPE_STRING:
		if (!display)
			simp_printf(ctx, port, "\"");
		if (display)
			simp_printstr(ctx, port, simp_getstring(ctx, obj), simp_getsize(ctx, obj));
		else
			simp_printsym(ctx, port, simp_getstring(ctx, obj), simp_getsize(ctx, obj));
		if (!display)
			simp_printf(ctx, port, "\"");
		break;
	case TYPE_SYMBOL:
		simp_printsym(ctx, port, simp_getsymbol(ctx, obj), simp_getsize(ctx, obj));
		break;
	case TYPE_EXCEPTION:
		simp_printf(ctx, port, "ERROR: %s", simp_getexception(ctx, obj));
		break;
	case TYPE_VECTOR:
		simp_printf(ctx, port, "(");
		len = simp_getsize(ctx, obj);
		for (i = 0; i < len; i++) {
			if (i > 0)
				simp_printf(ctx, port, " ");
			curr = simp_getvectormemb(ctx, obj, i);
			dowrite(ctx, port, curr, display);
		}
		simp_printf(ctx, port, ")");
		break;
	}
}

Simp
simp_write(Simp ctx, Simp port, Simp obj)
{
	dowrite(ctx, port, obj, false);
	return simp_void();     // TODO: check write error
}

Simp
simp_display(Simp ctx, Simp port, Simp obj)
{
	dowrite(ctx, port, obj, true);
	return simp_void();     // TODO: check write error
}
