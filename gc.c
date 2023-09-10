#include <stdbool.h>
#include <stdlib.h>

#include "simp.h"

#define ALIGN (sizeof(SimpSiz) > sizeof(void *) ? sizeof(SimpSiz) : sizeof(void *))

enum {
	/*
	 * Heaps begin marked with 0; the current garbage mark begins
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
};

struct Heap {
	struct Heap    *prev;
	struct Heap    *next;
	void           *data;
	int             mark;
	const char     *filename;
	SimpSiz         lineno;
	SimpSiz         column;
	SimpSiz         capacity;
};

typedef struct GC {
	/* same structure, just to rename the members */
	struct Heap    *free;
	struct Heap    *curr;
	void           *data;
	int             mark;
	const char     *filename;
	SimpSiz         lineno;
	SimpSiz         column;
	SimpSiz         capacity;
} GC;

static bool isheap[] = {
#define X(n, h) [n] = h,
	TYPES
#undef  X
};

static void
mark(Simp ctx, Simp obj)
{
	Heap *heap;
	SimpSiz i;
	GC *gc = (GC *)simp_getgcmemory(ctx);
	enum Type type;

	type = simp_gettype(obj);
	if (!isheap[type])
		return;
	heap = simp_getgcmemory(obj);
	if (heap == NULL)
		return;
	if (heap->mark == gc->mark)
		return;
	heap->mark = gc->mark;
	if (heap->next != NULL)
		heap->next->prev = heap->prev;
	if (heap->prev != NULL)
		heap->prev->next = heap->next;
	else
		gc->free = heap->next;
	heap->next = gc->curr;
	heap->prev = NULL;
	if (gc->curr != NULL)
		gc->curr->prev = heap;
	gc->curr = heap;
	if (heap->capacity == 0)
		return;
	for (i = 0; i < heap->capacity; i++) {
		mark(ctx, ((Simp *)heap->data)[i]);
	}
}

static void
sweep(Simp ctx)
{
	GC *gc = (GC *)simp_getgcmemory(ctx);
	Heap *heap, *tmp;

	heap = gc->free;
	while (heap != NULL) {
		tmp = heap;
		heap = heap->next;
		free(tmp->data);
		free(tmp);
	}
}

void
simp_gc(Simp ctx, Simp *objs, SimpSiz nobjs)
{
	GC *gc = (GC *)simp_getgcmemory(ctx);
	SimpSiz size, i;

	gc->free = gc->curr;
	gc->curr = NULL;
	for (i = 0; i < nobjs; i++)
		mark(ctx, objs[i]);
	size = simp_getsize(ctx);
	for (i = 0; i < size; i++)
		mark(ctx, ((Simp *)gc->data)[i]);
	sweep(ctx);
	gc->mark *= MARK_MUL;
	gc->free = NULL;
}

void
simp_gcfree(Simp ctx)
{
	GC *gc = (GC *)simp_getgcmemory(ctx);

	gc->free = gc->curr;
	sweep(ctx);
	free(gc->data);
	free(gc);
}

Heap *
simp_gcnewobj(Simp ctx, SimpSiz size, SimpSiz nobjs, const char *filename, SimpSiz lineno, SimpSiz column)
{
	Heap *heap = NULL;
	void *data = NULL;
	GC *gc;

	if (simp_isnil(ctx))
		gc = NULL;
	else
		gc = (GC *)simp_getgcmemory(ctx);
	if ((heap = malloc(sizeof(*heap))) == NULL)
		goto error;
	if ((data = malloc(size)) == NULL)
		goto error;
	*heap = (Heap){
		.mark = MARK_ZERO,
		.prev = NULL,
		.next = NULL,
		.data = data,
		.filename = filename,
		.lineno = lineno,
		.column = column,
		.capacity = nobjs,
	};
	if (gc == NULL) {
		/* there's no garbage context (we're creating it right now) */
		heap->mark = MARK_ONE;
		return heap;
	}
	heap->next = gc->curr;
	if (gc->curr != NULL)
		gc->curr->prev = heap;
	gc->curr = heap;
	return heap;
error:
	free(data);
	free(heap);
	return NULL;
}

void *
simp_getheapdata(Heap *heap)
{
	return heap->data;
}
