#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "simp.h"

struct Vector {
	struct Vector *prev;
	struct Vector *next;
	Simp          *data;
	SimpSiz        size;
	bool           mark;
};

struct GC {
	/* same structure, just to rename the members */
	struct Vector *free;
	struct Vector *curr;
	Simp          *root;
	SimpSiz        size;
	bool           ignore;
};

Vector *
simp_gcnewvector(GC *gc, SimpSiz size)
{
	Vector *vector = NULL;
	Simp *data = NULL;

	assert(size > 0);
	if ((vector = malloc(sizeof(*vector))) == NULL)
		goto error;
	if ((data = calloc(size, sizeof(*data))) == NULL)
		goto error;
	*vector = (struct Vector){
		.mark = false,
		.prev = NULL,
		.next = NULL,
		.data = data,
		.size = size,
	};
	if (gc == NULL)         /* there's no garbage context yet */
		return vector;  /* (we are creating it right now) */
	assert((void *)gc != (void *)vector);
	vector->next = gc->curr;
	if (gc->curr != NULL)
		gc->curr->prev = vector;
	gc->curr = vector;
	return vector;
error:
	return NULL;
}

Simp *
simp_gcgetvector(Vector *vector)
{
	return vector->data;
}

SimpSiz
simp_gcgetlength(Vector *vector)
{
	return vector->size;
}
