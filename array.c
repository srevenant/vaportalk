/* This source code is in the public domain. */

/* array.c: Handle allocation and manipulation of arrays */

#include "vt.h"

#ifdef PROTOTYPES
static Array *aralloc(void);
static void mark_cframes(int, Cframe *);
static void mark_estates(Estate *);
static void mark_dframes(int, Dframe *);
static void mark_dframe(Dframe *);
#else
static Array *aralloc();
static void mark_cframes(), mark_estates(), mark_dframes();
static void mark_dframe();
#endif

#define INIT_AINDEX_SIZE 128
#define BLOCK_SIZE 64
static Array **aindex, firstblock[BLOCK_SIZE], *unused;
static int aindex_size, ipos = 1, bpos = 0, collecting = 0;

#define PTSIZE 101
typedef struct aentry Aentry;
typedef struct asubelem Asubelem;
struct aentry { char *name; Asubelem *sub; Aentry *left, *right; };
struct asubelem { int *assoc; int num; Asubelem *next; };
static Aentry *atbl[PTSIZE];

extern Array *gvars;
extern Dframe *dstack;
extern Cframe *cstack;
extern int dpos, cpos;
extern Unode rmt_ring, win_ring;
extern Estate *events, *gen_read, *gen_high, *gen_low;

void init_array()
{
	aindex = Newarray(Array *, aindex_size = INIT_AINDEX_SIZE);
	aindex[0] = firstblock;
}

static Array *aralloc()
{
	Array *temp;

	if (unused) {
		temp = unused;
		temp->used = 1;
		unused = unused->r.next;
		return temp;
	}
	if (bpos == BLOCK_SIZE) {
		if (ipos == aindex_size)
			Double(aindex, Array *, aindex_size);
		aindex[ipos++] = Newarray(Array, BLOCK_SIZE);
		bpos = 0;
	}
	aindex[ipos - 1][bpos].used = 1;
	return &aindex[ipos - 1][bpos++];
}

void arfree(ar)
	Array *ar;
{
	ar->used = 0;
	ar->r.next = unused;
	unused = ar;
}

Dframe *dfalloc(num)
	int num;
{
	Dframe *vals, *vp;

	vals = Newarray(Dframe, num);
	for (vp = vals; num--; vp++)
		vp->type = F_NULL;
	return vals;
}

Array *add_array(size, vals, intern, fixed, isp)
	int size, intern, fixed, isp;
	Dframe *vals;
{
	Array *new;

	new = aralloc();
	new->alloc = size;
	new->vals = vals;
	new->intern = intern;
	new->fixed = fixed;
	new->isp = isp;
	return new;
}

void del_array(array)
	Array *array;
{
	deref_frames(array->alloc, array->vals);
	Discardarray(array->vals, Dframe, array->alloc);
	arfree(array);
}

void extend_array(ar, size)
	Array *ar;
	int size;
{
	Dframe *new;

	if (size <= ar->alloc)
		return;
	size *= 2;
	new = Newarray(Dframe, size);
	Copy(ar->vals, new, ar->alloc, Dframe);
	move_frames_refs(ar->alloc, ar->vals, new);
	Discardarray(ar->vals, Dframe, ar->alloc);
	ar->vals = new;
	for (new += ar->alloc; ar->alloc < size; ar->alloc++)
		new++->type = F_NULL;
}

void dec_ref_array(ar)
	Array *ar;
{
	if (!--ar->r.refs && !collecting)
		del_array(ar);
}

void dec_ref_plist(plist)
	Plist *plist;
{
	if (!--plist->refs && !collecting) {
		del_array(plist->array);
		Discard(plist, Plist);
	}
}

int lookup(assoc, str)
	int *assoc;
	char *str;
{
	Aentry **a;
	Asubelem **s;
	int val;

	a = &atbl[hash(str, PTSIZE)];
	while (*a) {
		val = strcmp(str, (*a)->name);
		if (!val)
			break;
		a = (val > 0) ? &(*a)->right : &(*a)->left;
	}
	if (!*a) {
		*a = New(Aentry);
		(*a)->name = vtstrdup(str);
		(*a)->left = (*a)->right = NULL;
		(*a)->sub = NULL;
	}
	for (s = &(*a)->sub; *s && (*s)->assoc != assoc; s = &(*s)->next);
	if (!*s) {
		*s = New(Asubelem);
		(*s)->assoc = assoc;
		(*s)->next = NULL;
		(*s)->num = (*assoc)++;
	}
	return (*s)->num;
}

int garbage()
{
	int i, j, counter = 0, n;
	Unode *un;
	Array *ar;

	for (i = 0; i < ipos; i++) {
		n = (i == ipos - 1) ? bpos : BLOCK_SIZE;
		for (j = 0; j < n; j++)
			aindex[i][j].garbage = 0;
	}
	mark_dframes(gvars->alloc, gvars->vals);
	mark_dframes(dpos, dstack);
	mark_cframes(cpos, cstack);
	for (un = rmt_ring.next; !un->dummy; un = un->next) {
		if (un->Robj)
			mark_dframe(un->Robj);
		mark_estates(un->Rrstack);
	}
	for (un = win_ring.next; !un->dummy; un = un->next) {
		if (un->Wobj)
			mark_dframe(un->Wobj);
		mark_estates(un->Wghstack);
		mark_estates(un->Wglstack);
		mark_estates(un->Wrstack);
	}
	mark_estates(events);
	mark_estates(gen_read);
	mark_estates(gen_high);
	mark_estates(gen_low);
	collecting = 1;
	for (i = 0; i < ipos; i++) {
		n = (i == ipos - 1) ? bpos : BLOCK_SIZE;
		for (j = 0; j < n; j++) {
			ar = &aindex[i][j];
			if (ar->used && !ar->intern && !ar->garbage) {
				if (ar->isp)
					Discard(ar->r.plist, Plist);
				del_array(ar);
				counter++;
			}
		}
	}
	collecting = 0;
	return counter;
}

static void mark_cframes(num, frames)
	int num;
	Cframe *frames;
{
	for (; num--; frames++)
	       mark_dframes(frames->lvars->alloc, frames->lvars->vals);
}

static void mark_estates(es)
	Estate *es;
{
	for (; es; es = es->next) {
		mark_dframes(es->dframes, es->dimage);
		mark_cframes(es->cframes, es->cimage);
	}
}

static void mark_dframes(num, frames)
	int num;
	Dframe *frames;
{
	while (num--)
		mark_dframe(frames++);
}

static void mark_dframe(f)
	Dframe *f;
{
	Array *a = NULL;

	if (f->type == F_APTR && !f->Darray->intern)
		a = f->Darray;
	else if (f->type == F_PLIST)
		a = f->Dplist->array;
	if (a && !a->garbage) {
		a->garbage = 1;
		mark_dframes(a->alloc, a->vals);
	}
}


