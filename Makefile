PROG = simp
OBJS = ${PROG:=.o} data.o port.o eval.o io.o
SRCS = ${OBJS:.o=.c}
MANS = simp.1 simp.7

DEFS = -D_POSIX_C_SOURCE=200809L
LIBS = -lm

all: ${PROG}

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS} ${LDFLAGS}

.c.o:
	${CC} -std=c99 -pedantic ${DEFS} ${INCS} ${CFLAGS} ${CPPFLAGS} -o $@ -c $<

${OBJS}: simp.h

tags: ${SRCS}
	ctags ${SRCS}

lint: ${SRCS}
	-mandoc -T lint -W warning ${MANS}
	-clang-tidy ${SRCS} -- -std=c99 ${DEFS} ${INCS} ${CPPFLAGS}

clean:
	rm -f ${OBJS} ${PROG} ${PROG:=.core} tags

.PHONY: all clean lint
