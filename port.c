#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

struct Port {
	enum {
		PORT_STREAM,
	} type;
	enum {
		PORT_OPEN     = 0x01,
		PORT_WRITE    = 0x02,
		PORT_READ     = 0x04,
		PORT_ERR      = 0x08,
		PORT_EOF      = 0x10,
	} mode;
	union {
		FILE   *fp;
	} u;
	Size nlines;
};

static Bool
canread(Port *port)
{
	return (port->mode & PORT_OPEN) &&
	       (port->mode & PORT_READ) &&
	       !(port->mode & PORT_ERR) &&
	       !(port->mode & PORT_EOF);
}

static Atom
openstream(Context *ctx, FILE *stream, Fixnum mode)
{
	Port *port;

	port = malloc(sizeof(*port));
	// TODO: check error
	*port = (Port){
		.type = PORT_STREAM,
		.mode = mode,
		.u.fp = stream,
		.nlines = 0,
	};
	return simp_makeport(ctx, port);
}

void
simp_printf(Context *ctx, Atom obj, const char *fmt, ...)
{
	Port *port;
	FILE *fp;
	va_list ap;

	port = simp_getport(ctx, obj);
	va_start(ap, fmt);
	switch (port->type) {
	case PORT_STREAM:
		fp = port->u.fp;
		vfprintf(fp, fmt, ap);
		break;
	}
	va_end(ap);
}

RByte
simp_readbyte(Context *ctx, Atom obj)
{
	Port *port = simp_getport(ctx, obj);
	RByte byte = NOTHING;
	FILE *fp;
	int c;

	if (!canread(port))
		return NOTHING;
	switch (port->type) {
	case PORT_STREAM:
		fp = port->u.fp;
		c = fgetc(fp);
		if (c == EOF) {
			if (ferror(fp))
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
simp_unreadbyte(Context *ctx, Atom obj, RByte c)
{
	Port *port;
	FILE *fp;

	if (c == NOTHING)
		return;
	port = simp_getport(ctx, obj);
	switch (port->type) {
	case PORT_STREAM:
		fp = port->u.fp;
		(void)ungetc(c, fp);
		break;
	}
}

RByte
simp_peekbyte(Context *ctx, Atom obj)
{
	RByte c;

	c = simp_readbyte(ctx, obj);
	simp_unreadbyte(ctx, obj, c);
	return c;
}

Atom
simp_openiport(Context *ctx)
{
	return openstream(ctx, stdin, PORT_OPEN | PORT_READ);
}

Atom
simp_openoport(Context *ctx)
{
	return openstream(ctx, stdout, PORT_OPEN | PORT_WRITE);
}

Atom
simp_openeport(Context *ctx)
{
	return openstream(ctx, stderr, PORT_OPEN | PORT_WRITE);
}

Bool
simp_porteof(Context *ctx, Atom obj)
{
	Port *port;

	(void)ctx;
	port = simp_getport(ctx, obj);
	return port->mode & PORT_EOF;
}

Bool
simp_porterr(Context *ctx, Atom obj)
{
	Port *port;

	(void)ctx;
	port = simp_getport(ctx, obj);
	return port->mode & PORT_ERR;
}
