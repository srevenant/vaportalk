/* This source code is in the public domain. */

/* tmalloc.c: Tray memory allocator */

#include <stdio.h>
#include <sys/types.h>
#include "vt.h"

typedef long Align;
typedef union tlist {
	union tlist *next;
	Align a;
} Tlist;

#define TRAY_INC sizeof(Tlist)
#define NUM_TRAYS 4
#define MAX_USE_TRAY (NUM_TRAYS * TRAY_INC)
#define TRAY_ELEM 512
#define TRAY_SIZE (TRAY_ELEM * TRAY_INC)

static Tlist *trays[NUM_TRAYS];

char *tmalloc(size)
	size_t size;
{
	int t, n, i, max_elem;
	char *p;

	if (size > MAX_USE_TRAY)
		return malloc(size);
	t = (size - 1) / TRAY_INC;
	if (!trays[t]) {
		n = t + 1;
		trays[t] = (Tlist *) malloc(TRAY_SIZE);
		if (!trays[t])
			return NULL;
		max_elem = TRAY_ELEM - 2 * n;
		for (i = 0; i <= max_elem; i += n)
			trays[t][i].next = &trays[t][i + n];
		trays[t][i].next = NULL;
	}
	p = (char *) trays[t];
	trays[t] = trays[t]->next;
	return p;
}

void tfree(ptr, size)
	char *ptr;
	size_t size;
{
	int t;

	if (size > MAX_USE_TRAY) {
		free(ptr);
		return;
	}
	t = (size - 1) / TRAY_INC;
	((Tlist *) ptr)->next = trays[t];
	trays[t] = (Tlist *) ptr;
}

char *trealloc(ptr, oldsize, newsize)
	char *ptr;
	size_t oldsize, newsize;
{
	char *new;

	if (oldsize > MAX_USE_TRAY && newsize > MAX_USE_TRAY)
		return realloc(ptr, newsize);
	new = tmalloc(newsize);
	memmove(new, ptr, oldsize);
	tfree(ptr, oldsize);
	return new;
}


