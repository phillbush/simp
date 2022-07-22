CMD     = bin/schola
CMDOBJS = bin/schola.o
CMDSRCS = bin/schola.c

AR      = lib/libschola.a
SO      = lib/libschola.so
LIBOBJS = lib/schola.o
LIBSRCS = lib/schola.c

INCS    = include/schola.h

OBJS = ${CMDOBJS} ${LIBOBJS}
SRCS = ${CMDSRCS} ${LIBSRCS} ${INCS}

all: cmd
cmd: ${CMD}
lib: ${AR} ${SO}

${CMDOBJS}: ${INCS}

${CMD}: ${CMDOBJS} ${LIBOBJS}
	${CC} -o $@ ${CMDOBJS} ${LIBOBJS} ${LDFLAGS}

.c.o:
	${CC} -Iinclude ${CFLAGS} ${CPPFLAGS} -o $@ -c $<

tags: ${SRCS}
	ctags ${SRCS}

clean:
	rm -f ${OBJS} ${CMD} ${LIB} tags

.PHONY: all tags clean
