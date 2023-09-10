#include <stdbool.h>
#include <stdlib.h>

#include "simp.h"

#define ALIGN (sizeof(SimpSiz) > sizeof(void *) ? sizeof(SimpSiz) : sizeof(void *))

enum {
	/*
	 * Footers begin marked with 0; the current garbage mark begins
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

typedef struct Footer {
	struct Footer  *prev;
	struct Footer  *next;
	void           *data;
	int             mark;
	const char     *filename;
	SimpSiz         lineno;
	SimpSiz         column;
} Footer;

typedef struct GC {
	/* same structure, just to rename the members */
	struct Footer  *free;
	struct Footer  *curr;
	void           *data;
	int             mark;
	const char     *filename;
	SimpSiz         lineno;
	SimpSiz         column;
} GC;

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
	Footer *footer;
	SimpSiz size, i;
	GC *gc = (GC *)simp_getgcmemory(ctx);
	enum Type type;

	type = simp_gettype(obj);
	if (!isheap[type])
		return;
	footer = simp_getgcmemory(obj);
	if (footer == NULL)
		return;
	if (footer->mark == gc->mark)
		return;
	footer->mark = gc->mark;
	if (footer->next != NULL)
		footer->next->prev = footer->prev;
	if (footer->prev != NULL)
		footer->prev->next = footer->next;
	else
		gc->free = footer->next;
	footer->next = gc->curr;
	footer->prev = NULL;
	if (gc->curr != NULL)
		gc->curr->prev = footer;
	gc->curr = footer;
	if (!isvector[type])
		return;
	size = simp_getsize(obj);
	for (i = 0; i < size; i++) {
		mark(ctx, ((Simp *)footer->data)[i]);
	}
}

static void
sweep(Simp ctx)
{
	GC *gc = (GC *)simp_getgcmemory(ctx);
	Footer *footer, *tmp;

	footer = gc->free;
	while (footer != NULL) {
		tmp = footer;
		footer = footer->next;
		free(tmp->data);
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
}

void *
simp_gcnewarray(Simp ctx, SimpSiz nmembs, SimpSiz membsiz, const char *filename, SimpSiz lineno, SimpSiz column)
{
	Footer *footer = NULL;
	void *data = NULL;
	GC *gc;

	if (simp_isnil(ctx))
		gc = NULL;
	else
		gc = (GC *)simp_getgcmemory(ctx);
	if (posix_memalign(&data, ALIGN, nmembs * membsiz + sizeof(*footer)) != 0)
		goto error;
	footer = (Footer *)((char *)data + nmembs * membsiz);
	*footer = (struct Footer){
		.mark = MARK_ZERO,
		.prev = NULL,
		.next = NULL,
		.data = data,
		.filename = filename,
		.lineno = lineno,
		.column = column,
	};
	if (gc == NULL) {
		/* there's no garbage context (we're creating it right now) */
		footer->mark = MARK_ONE;
		return data;
	}
	footer->next = gc->curr;
	if (gc->curr != NULL)
		gc->curr->prev = footer;
	gc->curr = footer;
	return data;
error:
	free(data);
	return NULL;
}
