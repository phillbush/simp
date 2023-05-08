PROG = simp

OBJS = simp.o\
       data.o\
       port.o\
       context.o\
       repl.o

all: ${PROG}

simp: ${OBJS}
	${CC} -o $@ ${OBJS} -lm ${LDFLAGS}

.c.o:
	${CC} -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L\
	${CFLAGS} ${CPPFLAGS} -o $@ -c $<

${OBJS}: simp.h

clean:
	rm -f ${OBJS} ${PROG} ${PROG:=.core} tags

.PHONY: all tags clean
