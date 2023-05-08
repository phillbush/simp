#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simp.h"

typedef struct Port {
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
	SSimp nlines;
} Port;

static int
canread(Port *port)
{
	return (port->mode & PORT_OPEN) &&
	       (port->mode & PORT_READ) &&
	       !(port->mode & PORT_ERR) &&
	       !(port->mode & PORT_EOF);
}

void
simp_printf(Simp ctx, Simp obj, const char *fmt, ...)
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

int
simp_readbyte(Simp ctx, Simp obj)
{
	Port *port = simp_getport(ctx, obj);
	int byte = NOTHING;
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
simp_unreadbyte(Simp ctx, Simp obj, int c)
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

int
simp_peekbyte(Simp ctx, Simp obj)
{
	int c;

	c = simp_readbyte(ctx, obj);
	simp_unreadbyte(ctx, obj, c);
	return c;
}

Simp
simp_openstream(Simp ctx, void *p, char *s)
{
	FILE *stream = (FILE *)p;
	Port *port;
	int mode = PORT_OPEN;

	if (strchr(s, 'w') != NULL)
		mode |= PORT_WRITE;
	if (strchr(s, 'r') != NULL)
		mode |= PORT_READ;
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
