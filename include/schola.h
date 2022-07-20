#ifndef _SCHOLA_H
#define _SCHOLA_H

typedef struct schola_cell *schola_cell;
typedef struct schola_context *schola_context;

int schola_eof_p(schola_context, schola_cell);
int schola_void_p(schola_context, schola_cell);
int schola_true_p(schola_context, schola_cell);
int schola_false_p(schola_context, schola_cell);
schola_context schola_init(void);
schola_cell schola_read(schola_context);
schola_cell schola_eval(schola_context, schola_cell);
void schola_write(schola_context, schola_cell);
void schola_interactive(schola_context, FILE *, FILE *, FILE *);

#endif /* _SCHOLA_H */
