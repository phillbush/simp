#ifndef _SIMP_H
#define _SIMP_H

typedef struct simpctx_t *simpctx_t;

simpctx_t simp_init(FILE *, FILE *, FILE *);
void simp_repl(simpctx_t);
void simp_clean(simpctx_t);

#endif /* _SIMP_H */
