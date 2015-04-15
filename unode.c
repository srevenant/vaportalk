/* This source code is in the public domain. */

/* unode.c: Allocation and general routines dealing with user nodes */

#include "vt.h"

static int id = 0;			/* Used to assign unique IDs   */
static Unode recycle_ring;		/* Re-use discarded unodes     */
extern int debug;
extern Dframe *dstack;

void init_unalloc()
{
	recycle_ring.next = recycle_ring.prev = &recycle_ring;
}

Unode *unalloc()
{
	Unode *ret;

	if (recycle_ring.next != &recycle_ring) {
		ret = recycle_ring.next;
		recycle_ring.next = ret->next;
		ret->next->prev = &recycle_ring;
	} else
		ret = New(Unode);
	ret->frefs = NULL;
	ret->dummy = 0;
	ret->id = id++;
	return ret;
}

void destroy_pointers(frefs)
	Flist *frefs;
{
	Flist *next;

	for (; frefs; frefs = next) {
		next = frefs->next;
		frefs->fr->type = F_NULL;
		Discard(frefs, Flist);
	}
}

void discard_unode(un)
	Unode *un;
{
	un->id = -1;
	un->next = recycle_ring.next;
	un->prev = &recycle_ring;
	recycle_ring.next = recycle_ring.next->prev = un;
}

void add_fref(frefs, frame)
	Flist **frefs;
	Dframe *frame;
{
	Flist *fr;

	fr = New(Flist);
	fr->fr = frame;
	fr->next = *frefs;
	*frefs = fr;
}

void move_fref(frefs, old, new)
	Flist *frefs;
	Dframe *old, *new;
{
	while (frefs->fr != old)
		 frefs = frefs->next;
	frefs->fr = new;
}

void elim_fref(frefs, frame)
	Flist **frefs;
	Dframe *frame;
{
	Flist *next;

	while ((*frefs)->fr != frame)
		frefs = &(*frefs)->next;
	next = (*frefs)->next;
	Discard(*frefs, Flist);
	*frefs = next;
}


