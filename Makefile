PROG = simp
OBJS = simp.o data.o port.o eval.o io.o stdlib.o
SRCS = simp.c data.c port.c eval.c io.c
MANS = simp.1 simp.7

DEFS = -D_POSIX_C_SOURCE=200809L
LIBS = -lm

LDEMULATION ?= elf_x86_64

all: ${PROG}

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS} ${LDFLAGS}

.c.o:
	${CC} -std=c99 -pedantic ${DEFS} ${INCS} ${CFLAGS} ${CPPFLAGS} -o $@ -c $<

stdlib.o: stdlib.lisp
	${LD} -r -b binary -o stdlib.o -m ${LDEMULATION} stdlib.lisp

${OBJS}: simp.h

tags: ${SRCS}
	ctags ${SRCS}

lint: ${SRCS}
	-mandoc -T lint -W warning ${MANS}
	-clang-tidy ${SRCS} -- -std=c99 ${DEFS} ${INCS} ${CPPFLAGS}

clean:
	rm -f ${OBJS} ${PROG} ${PROG:=.core} tags

.PHONY: all clean lint
