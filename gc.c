#include <stdbool.h>
#include <stdlib.h>

#include "simp.h"

enum {
	/*
	 * Heap objects begin marked with 0.
	 *
	 * The garbage factor begins as 1.
	 *
	 * At each garbage collection run, the garbage factor switches
	 * between 1 and -1.
	 *
	 * We begin with all allocated objects listed on gc->p[FREE].
	 * Then, we mark all reachable objects with the current garbage
	 * factor and move them to gc->p[CURR].  While we're visiting
	 * objects, we ignore those which has already been marked.
	 *
	 * All allocated objects remaining on gc->p[FREE] are freed.
	 */
	MARK_ZERO = 0,
	MARK_ONE  = 1,
	MARK_MUL  = -1,

	/*
	 * Heap objects points to the previous and next objects in a
	 * doubly linked list.
	 */
	PREV = 0,
	NEXT = 1,

	/*
	 * Garbage context is also a heap object, but instead point to
	 * the list of garbage objects (to be freed), and the list of
	 * reachable objects (to be kept).
	 */
	GARBAGE = 0,
	REACHED = 1,
};

struct Heap {
	struct Heap    *p[2];
	void           *data;
	int             mark;
	const char     *filename;
	SimpSiz         lineno;
	SimpSiz         column;
	SimpSiz         size;
};

static bool isheap[] = {
#define X(n, h) [n] = h,
	TYPES
#undef  X
};

static void
mark(Heap *gc, Simp obj)
{
	Heap *heap;
	SimpSiz i;
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
	if (heap->p[NEXT] != NULL)
		heap->p[NEXT]->p[PREV] = heap->p[PREV];
	if (heap->p[PREV] != NULL)
		heap->p[PREV]->p[NEXT] = heap->p[NEXT];
	else
		gc->p[GARBAGE] = heap->p[NEXT];
	heap->p[NEXT] = gc->p[REACHED];
	heap->p[PREV] = NULL;
	if (gc->p[REACHED] != NULL)
		gc->p[REACHED]->p[PREV] = heap;
	gc->p[REACHED] = heap;
	if (heap->size == 0)
		return;
	for (i = 0; i < heap->size; i++) {
		mark(gc, ((Simp *)heap->data)[i]);
	}
}

static void
sweep(Heap *gc)
{
	Heap *heap, *tmp;

	heap = gc->p[GARBAGE];
	while (heap != NULL) {
		tmp = heap;
		heap = heap->p[NEXT];
		free(tmp->data);
		free(tmp);
	}
}

void
simp_gc(Simp ctx, Simp *objs, SimpSiz nobjs)
{
	Heap *gc = simp_getgcmemory(ctx);
	SimpSiz i;

	gc->p[GARBAGE] = gc->p[REACHED];
	gc->p[REACHED] = NULL;
	for (i = 0; i < nobjs; i++)
		mark(gc, objs[i]);
	for (i = 0; i < gc->size; i++)
		mark(gc, ((Simp *)gc->data)[i]);
	sweep(gc);
	gc->mark *= MARK_MUL;
	gc->p[GARBAGE] = NULL;
}

void
simp_gcfree(Simp ctx)
{
	Heap *gc = simp_getgcmemory(ctx);

	gc->p[GARBAGE] = gc->p[REACHED];
	sweep(gc);
	free(gc->data);
	free(gc);
}

Heap *
simp_gcnewobj(Heap *gc, SimpSiz size, SimpSiz nobjs, const char *filename, SimpSiz lineno, SimpSiz column)
{
	Heap *heap = NULL;
	void *data = NULL;

	if ((heap = malloc(sizeof(*heap))) == NULL)
		goto error;
	if ((data = malloc(size)) == NULL)
		goto error;
	*heap = (Heap){
		.mark = MARK_ZERO,
		.p = { NULL, NULL },
		.data = data,
		.filename = filename,
		.lineno = lineno,
		.column = column,
		.size = nobjs,
	};
	if (gc == NULL) {
		/* there's no garbage context (we're creating it right now) */
		heap->mark = MARK_ONE;
		return heap;
	}
	heap->p[NEXT] = gc->p[REACHED];
	if (gc->p[REACHED] != NULL)
		gc->p[REACHED]->p[PREV] = heap;
	gc->p[REACHED] = heap;
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

bool
simp_getsource(Heap *heap, const char **filename, SimpSiz *lineno, SimpSiz *column)
{
	if (heap == NULL || heap->filename == NULL)
		return false;
	if (filename != NULL)
		*filename = heap->filename;
	if (lineno != NULL)
		*lineno = heap->lineno;
	if (column != NULL)
		*column = heap->column;
	return true;
}
