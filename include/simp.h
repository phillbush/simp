#ifndef _SIMP_H
#define _SIMP_H

typedef struct simp_cell *simp_cell;
typedef struct simp_context *simp_context;

int simp_nil_p(simp_context, simp_cell);
int simp_eof_p(simp_context, simp_cell);
int simp_void_p(simp_context, simp_cell);
int simp_true_p(simp_context, simp_cell);
int simp_false_p(simp_context, simp_cell);
simp_context simp_init(void);
simp_cell simp_read(simp_context);
simp_cell simp_eval(simp_context, simp_cell);
simp_cell simp_write(simp_context, simp_cell);
void simp_interactive(simp_context, FILE *, FILE *, FILE *);

#endif /* _SIMP_H */
