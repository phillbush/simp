#include <stdbool.h>
#include <stdlib.h>

#include "simp.h"

enum {
	/*
	 * Vectors begin marked with 0; the current garbage mark begins
	 * with 1.
	 *
	 * At each garbage collection run, the garbage mark switches
	 * between 1 and -1.
	 *
	 * We begin with all allocated vectors listed on gc->free and
	 * mark all reachable vectors with the current garbage mark and
	 * move them to gc->curr.
	 *
	 * All allocated vectors remaining on gc->free are freed.
	 */
	MARK_ZERO = 0,
	MARK_ONE  = 1,
	MARK_MUL  = -1,
	MARK_ALL  = 100,
};

struct Vector {
	struct Vector *prev;
	struct Vector *next;
	void          *data;
	SimpSiz        size;
	int            mark;
};

struct GC {
	/* same structure, just to rename the members */
	struct Vector *free;
	struct Vector *curr;
	void          *data;
	SimpSiz        size;
	int            mark;
};

static bool isvector[] = {
#define X(n, v, h) [n] = v,
	TYPES
#undef  X
};

static bool isheap[] = {
#define X(n, v, h) [n] = h,
	TYPES
#undef  X
};

static void
mark(Simp ctx, Simp obj)
{
	Vector *vector;
	SimpSiz i;
	GC *gc = (GC *)simp_getgcmemory(ctx, ctx);
	enum Type type;

	type = simp_gettype(ctx, obj);
	if (!isheap[type])
		return;
	vector = simp_getgcmemory(ctx, obj);
	if (vector == NULL)
		return;
	if (vector->mark == gc->mark)
		return;
	vector->mark = gc->mark;
	if (vector->next != NULL)
		vector->next->prev = vector->prev;
	if (vector->prev != NULL)
		vector->prev->next = vector->next;
	else
		gc->free = vector->next;
	vector->next = gc->curr;
	vector->prev = NULL;
	if (gc->curr != NULL)
		gc->curr->prev = vector;
	gc->curr = vector;
	if (!isvector[type])
		return;
	for (i = 0; i < vector->size; i++) {
		mark(ctx, ((Simp *)vector->data)[i]);
	}
}

static void
sweep(Simp ctx)
{
	GC *gc = (GC *)simp_getgcmemory(ctx, ctx);
	Vector *vector, *tmp;

	vector = gc->free;
	while (vector != NULL) {
		tmp = vector;
		vector = vector->next;
		free(tmp->data);
		free(tmp);
	}
}

void
simp_gc(Simp ctx)
{
	GC *gc = (GC *)simp_getgcmemory(ctx, ctx);
	SimpSiz i;

	gc->free = gc->curr;
	gc->curr = NULL;
	for (i = 0; i < gc->size; i++)
		mark(ctx, ((Simp *)gc->data)[i]);
	sweep(ctx);
	gc->mark *= MARK_MUL;
	gc->free = NULL;
}

void
simp_gcfree(Simp ctx)
{
	GC *gc = (GC *)simp_getgcmemory(ctx, ctx);

	gc->free = gc->curr;
	sweep(ctx);
	free(gc->data);
}

Vector *
simp_gcnewarray(Simp ctx, SimpSiz nmembs, SimpSiz membsiz)
{
	Vector *vector = NULL;
	void *data = NULL;
	GC *gc;

	if (simp_isnil(ctx, ctx))
		gc = NULL;
	else
		gc = (GC *)simp_getgcmemory(ctx, ctx);
	if ((vector = malloc(sizeof(*vector))) == NULL)
		goto error;
	if ((data = calloc(nmembs, membsiz)) == NULL)
		goto error;
	*vector = (struct Vector){
		.mark = MARK_ZERO,
		.prev = NULL,
		.next = NULL,
		.data = data,
		.size = nmembs,
	};
	if (gc == NULL) {
		/* there's no garbage context (we're creating it right now) */
		vector->mark = MARK_ONE;
		return vector;
	}
	vector->next = gc->curr;
	if (gc->curr != NULL)
		gc->curr->prev = vector;
	gc->curr = vector;
	return vector;
error:
	free(data);
	free(vector);
	return NULL;
}

void *
simp_gcgetdata(Vector *vector)
{
	return vector->data;
}

SimpSiz
simp_gcgetlength(Vector *vector)
{
	return vector->size;
}
