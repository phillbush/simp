CMD     = bin/simp
CMDOBJS = bin/simp.o
CMDSRCS = bin/simp.c

AR      = lib/libsimp.a
SO      = lib/libsimp.so
LIBOBJS = lib/simp.o
LIBINCS = lib/posix.h
LIBSRCS = lib/simp.c ${LIBINCS}

INCS    = include/simp.h

OBJS = ${CMDOBJS} ${LIBOBJS}
SRCS = ${CMDSRCS} ${LIBSRCS} ${INCS}

all: cmd
cmd: ${CMD}
lib: ${AR} ${SO}

${CMD}: ${CMDOBJS} ${LIBOBJS}
	${CC} -o $@ ${CMDOBJS} ${LIBOBJS} ${LDFLAGS}

.c.o:
	${CC} -Iinclude ${CFLAGS} ${CPPFLAGS} -o $@ -c $<

${CMDOBJS}: ${INCS} ${CMDINCS}
${LIBOBJS}: ${INCS} ${LIBINCS}

tags: ${SRCS}
	ctags ${SRCS}

clean:
	rm -f ${OBJS} ${CMD} ${LIB} tags

.PHONY: all tags clean
