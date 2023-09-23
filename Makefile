PROG = simp
SRCS = simp.c data.c port.c eval.c gc.c io.c arith.c
OBJS = ${SRCS:.c=.o}
MANS = simp.1
HEDS = simp.h

DEFS = -D_POSIX_C_SOURCE=200809L
LIBS = -lm

PDFS = simp.pdf

all: ${PROG}

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS} ${LDFLAGS}

.c.o:
	${CC} -std=c99 -pedantic ${DEFS} ${CFLAGS} ${CPPFLAGS} -o $@ -c $<

${PDFS}: ${@:.pdf=.1}
	{ printf '.fp 5 CW DejaVuSansMono\n' ; cat "${@:.pdf=.1}" ; } | \
	eqn | tbl | troff -mdoc - | dpost | ps2pdf -sPAPERSIZE=letter - >"$@"

${OBJS}: ${HEDS}

tags: ${SRCS}
	ctags ${SRCS}

lint: ${SRCS}
	-mandoc -T lint -W warning ${MANS}
	-cppcheck --enable=portability --std=c99 ${DEFS} ${SRCS}
	#-clang-tidy ${SRCS} -- -std=c99 ${DEFS} ${CPPFLAGS}

loc:
	@echo "Lines of code:"
	@cat ${SRCS} ${HEDS} | egrep -v '^([[:blank:]]|/\*.*\*/)*$$' | wc -l

clean:
	rm -f ${OBJS} ${PROG} ${PROG:=.core} tags

stage: Makefile README.md ${SRCS} ${MANS} ${HEDS} ${PDFS}
	git add Makefile README.md ${SRCS} ${MANS} ${HEDS} ${PDFS}

commit: all
	git commit

push:
	git push

.PHONY: all clean lint loc stage commit push
