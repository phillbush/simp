#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simp.h"

static enum PortMode
openmode(char *s)
{
	enum PortMode mode = PORT_OPEN;

	if (strchr(s, 'w') != NULL)
		mode |= PORT_WRITE;
	if (strchr(s, 'r') != NULL)
		mode |= PORT_READ;
	return mode;
}

static int
canread(Port *port)
{
	return (port->mode & PORT_OPEN) &&
	       (port->mode & PORT_READ) &&
	       !(port->mode & PORT_ERR) &&
	       !(port->mode & PORT_EOF);
}

void
simp_printf(Simp obj, const char *fmt, ...)
{
	Port *port;
	va_list ap;

	port = simp_getport(obj);
	va_start(ap, fmt);
	switch (port->type) {
	case PORT_STRING:
		if (port->u.str.curr >= port->u.str.size)
			break;
		vsnprintf(
			(char *)port->u.str.arr,
			port->u.str.size - port->u.str.curr,
			fmt,
			ap
		);
		break;
	case PORT_STREAM:
		vfprintf((FILE *)port->u.fp, fmt, ap);
		break;
	}
	va_end(ap);
}

int
simp_readbyte(Simp obj)
{
	Port *port = simp_getport(obj);
	int byte = NOTHING;
	int c;

	if (!canread(port))
		return NOTHING;
	if (port->count == PORT_NEWLINE) {
		port->lineno++;
		port->column = 1;
	} else if (port->count == PORT_NEWCHAR) {
		port->column++;
	}
	switch (port->type) {
	case PORT_STRING:
		if (port->u.str.curr >= port->u.str.size) {
			port->mode |= PORT_EOF;
			return NOTHING;
		}
		byte = port->u.str.arr[port->u.str.curr++];
		break;
	case PORT_STREAM:
		c = fgetc((FILE *)port->u.fp);
		if (c == EOF) {
			if (ferror((FILE *)port->u.fp))
				port->mode |= PORT_ERR;
			else
				port->mode |= PORT_EOF;
			return NOTHING;
		}
		byte = c;
		break;
	}
	if (byte == '\n')
		port->count = PORT_NEWLINE;
	else
		port->count = PORT_NEWCHAR;
	return byte;
}

void
simp_unreadbyte(Simp obj, int c)
{
	Port *port;

	if (c == NOTHING)
		return;
	port = simp_getport(obj);
	if (!canread(port))
		return;
	port->count = PORT_NOTHING;
	switch (port->type) {
	case PORT_STRING:
		if (port->u.str.curr > 0)
			port->u.str.arr[--port->u.str.curr] = (unsigned char)c;
		break;
	case PORT_STREAM:
		(void)ungetc(c, (FILE *)port->u.fp);
		break;
	}
}

int
simp_peekbyte(Simp obj)
{
	int c;

	c = simp_readbyte(obj);
	simp_unreadbyte(obj, c);
	return c;
}

bool
simp_openstream(Simp ctx, Simp *ret, const char *file, void *p, char *mode)
{
	FILE *stream;
	Port *port;
	Heap *heap;
	Simp sym;
	const unsigned char *filename = (const unsigned char *)file;

	stream = (FILE *)p;
	heap = simp_gcnewobj(simp_getgcmemory(ctx), sizeof(*port), 0);
	if (heap == NULL)
		return false;
	port = (Port *)simp_getheapdata(heap);
	port->type = PORT_STREAM;
	port->mode = openmode(mode);
	port->u.fp = stream;
	if (!simp_makesymbol(ctx, &sym, filename, strlen(file) + 1))
		return false;
	port->filename = (const char *)simp_getsymbol(sym);
	port->lineno = 1;
	port->column = 1;
	return simp_makeport(ctx, ret, heap);
}

bool
simp_openstring(Simp ctx, Simp *ret, const char *name, unsigned char *p, SimpSiz len, char *mode)
{
	Port *port;
	Heap *heap;
	Simp sym;

	heap = simp_gcnewobj(simp_getgcmemory(ctx), sizeof(*port), 0);
	if (heap == NULL)
		return false;
	port = (Port *)simp_getheapdata(heap);
	port->type = PORT_STRING;
	port->mode = openmode(mode);
	port->u.str.arr = p;
	port->u.str.size = len;
	port->u.str.curr = 0;
	if (!simp_makesymbol(ctx, &sym, (unsigned char *)name, strlen(name) + 1))
		return false;
	port->filename = (const char *)simp_getsymbol(sym);
	port->lineno = 1;
	port->column = 1;
	return simp_makeport(ctx, ret, heap);
}

int
simp_porteof(Simp obj)
{
	Port *port;

	port = simp_getport(obj);
	return port->mode & PORT_EOF;
}

int
simp_porterr(Simp obj)
{
	Port *port;

	port = simp_getport(obj);
	return port->mode & PORT_ERR;
}

SimpSiz
simp_portlineno(Simp obj)
{
	Port *port;

	port = simp_getport(obj);
	return port->lineno;
}

SimpSiz
simp_portcolumn(Simp obj)
{
	Port *port;

	port = simp_getport(obj);
	return port->column;
}

const char *
simp_portfilename(Simp obj)
{
	Port *port;

	port = simp_getport(obj);
	return port->filename;
}
