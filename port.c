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
		port->nlines++;
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

Simp
simp_openstream(Simp ctx, void *p, char *mode)
{
	FILE *stream;
	Port *port;

	stream = (FILE *)p;
	if ((port = simp_gcnewarray(ctx, 1, sizeof(*port))) == NULL)
		return simp_exception(ERROR_MEMORY);
	port->type = PORT_STREAM;
	port->mode = openmode(mode);
	port->u.fp = stream;
	port->nlines = 0;
	return simp_makeport(ctx, port);
}

Simp
simp_openstring(Simp ctx, unsigned char *p, SimpSiz len, char *mode)
{
	Port *port;

	if ((port = simp_gcnewarray(ctx, 1, sizeof(*port))) == NULL)
		return simp_exception(ERROR_MEMORY);
	port->type = PORT_STRING;
	port->mode = openmode(mode);
	port->u.str.arr = p;
	port->u.str.size = len;
	port->u.str.curr = 0;
	port->nlines = 0;
	return simp_makeport(ctx, port);
}

int
simp_porteof(Simp ctx, Simp obj)
{
	Port *port;

	(void)ctx;
	port = simp_getport(obj);
	return port->mode & PORT_EOF;
}

int
simp_porterr(Simp ctx, Simp obj)
{
	Port *port;

	(void)ctx;
	port = simp_getport(obj);
	return port->mode & PORT_ERR;
}

Simp
simp_initports(Simp ctx)
{
	Simp ports, port;
	SimpSiz i;
	struct {
		FILE *fp;
		char *mode;
	} portinit[] = {
#define X(n, f, m) [n] = { .fp = f, .mode = m, },
		PORTS
#undef  X
	};

	ports = simp_makevector(ctx, 3);
	if (simp_isexception(ports))
		return ports;
	for (i = 0; i < LEN(portinit); i++) {
		port = simp_openstream(ctx, portinit[i].fp, portinit[i].mode);
		if (simp_isexception(port))
			return port;
		simp_setvector(ports, i, port);
	}
	return ports;
}

Simp
simp_contextiport(Simp ctx)
{
	Simp ports;

	ports = simp_contextports(ctx);
	return simp_getvectormemb(ports, PORT_STDIN);
}

Simp
simp_contextoport(Simp ctx)
{
	Simp ports;

	ports = simp_contextports(ctx);
	return simp_getvectormemb(ports, PORT_STDOUT);
}

Simp
simp_contexteport(Simp ctx)
{
	Simp ports;

	ports = simp_contextports(ctx);
	return simp_getvectormemb(ports, PORT_STDERR);
}
