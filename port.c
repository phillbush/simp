#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simp.h"

#define PORTS                           \
	X(PORT_STDIN,   stdin,  "r"    )\
	X(PORT_STDOUT,  stdout, "w"    )\
	X(PORT_STDERR,  stderr, "w"    )

enum Ports {
#define X(n, f, m) n,
	PORTS
#undef  X
};

typedef struct Port {
	enum PortType {
		PORT_STREAM,
		PORT_STRING,
	} type;
	enum PortMode {
		PORT_OPEN     = 0x01,
		PORT_WRITE    = 0x02,
		PORT_READ     = 0x04,
		PORT_ERR      = 0x08,
		PORT_EOF      = 0x10,
	} mode;
	union {
		FILE   *fp;
		struct {
			unsigned char *arr;
			SimpSiz curr, size;
		} str;
	} u;
	SimpInt nlines;
} Port;

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

Simp
simp_printf(Simp ctx, Simp obj, const char *fmt, ...)
{
	Port *port;
	va_list ap;

	port = simp_getport(ctx, obj);
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
		vfprintf(port->u.fp, fmt, ap);
		break;
	}
	va_end(ap);
	return simp_void();
}

int
simp_readbyte(Simp ctx, Simp obj)
{
	Port *port = simp_getport(ctx, obj);
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
		c = fgetc(port->u.fp);
		if (c == EOF) {
			if (ferror(port->u.fp))
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
simp_unreadbyte(Simp ctx, Simp obj, int c)
{
	Port *port;

	if (c == NOTHING)
		return;
	port = simp_getport(ctx, obj);
	if (!canread(port))
		return;
	switch (port->type) {
	case PORT_STRING:
		if (port->u.str.curr > 0)
			port->u.str.arr[--port->u.str.curr] = (unsigned char)c;
		break;
	case PORT_STREAM:
		(void)ungetc(c, port->u.fp);
		break;
	}
}

int
simp_peekbyte(Simp ctx, Simp obj)
{
	int c;

	c = simp_readbyte(ctx, obj);
	simp_unreadbyte(ctx, obj, c);
	return c;
}

Simp
simp_openstream(Simp ctx, void *p, char *mode)
{
	FILE *stream = (FILE *)p;
	Port *port;

	if ((port = malloc(sizeof(*port))) == NULL)
		return simp_makeexception(ctx, ERROR_MEMORY);
	*port = (Port){
		.type = PORT_STREAM,
		.mode = openmode(mode),
		.u.fp = stream,
		.nlines = 0,
	};
	return simp_makeport(ctx, port);
}

Simp
simp_openstring(Simp ctx, unsigned char *p, SimpSiz len, char *mode)
{
	Port *port;

	if ((port = malloc(sizeof(*port))) == NULL)
		return simp_makeexception(ctx, ERROR_MEMORY);
	*port = (Port){
		.type = PORT_STRING,
		.mode = openmode(mode),
		.u.str.arr = p,
		.u.str.size = len,
		.u.str.curr = 0,
		.nlines = 0,
	};
	return simp_makeport(ctx, port);
}

int
simp_porteof(Simp ctx, Simp obj)
{
	Port *port;

	(void)ctx;
	port = simp_getport(ctx, obj);
	return port->mode & PORT_EOF;
}

int
simp_porterr(Simp ctx, Simp obj)
{
	Port *port;

	(void)ctx;
	port = simp_getport(ctx, obj);
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
	if (simp_isexception(ctx, ports))
		return ports;
	for (i = 0; i < LEN(portinit); i++) {
		port = simp_openstream(ctx, portinit[i].fp, portinit[i].mode);
		if (simp_isexception(ctx, port))
			return port;
		simp_getvector(ctx, ports)[i] = port;
	}
	return ports;
}

Simp
simp_contextiport(Simp ctx)
{
	Simp ports;

	ports = simp_contextports(ctx);
	return simp_getvectormemb(ctx, ports, PORT_STDIN);
}

Simp
simp_contextoport(Simp ctx)
{
	Simp ports;

	ports = simp_contextports(ctx);
	return simp_getvectormemb(ctx, ports, PORT_STDOUT);
}

Simp
simp_contexteport(Simp ctx)
{
	Simp ports;

	ports = simp_contextports(ctx);
	return simp_getvectormemb(ctx, ports, PORT_STDERR);
}
