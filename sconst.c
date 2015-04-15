/* This source code is in the public domain. */

/* sconst.c: Handle addition and removal of string constants */

#include "vt.h"
#define STSIZE 101

struct sconst {
	Rstr *rs;			/* Given to program		*/
	int refs;			/* Ref count from programs only */
	struct sconst *next;
};

static struct sconst *sctab[STSIZE];

Rstr *add_sconst(cstr)
	Cstr *cstr;
{
	int ind;
	struct sconst *new, *sc;

	ind = hash(cstr->s, STSIZE);
	for (sc = sctab[ind]; sc; sc = sc->next) {
		if (streq(sc->rs->str.c.s, cstr->s))
			break;
	}
	if (sc) {
		sc->refs++;
		return sc->rs;
	}
	new = New(struct sconst);
	new->rs = New(Rstr);
	new->rs->str.c = *cstr;
	new->rs->str.sz = cstr->l + 1;
	new->rs->refs = 1;
	new->refs = 1;
	new->next = sctab[ind];
	sctab[ind] = new;
	cstr->s = NULL;
	return new->rs;
}

void del_sconst(rs)
	Rstr *rs;
{
	int ind;
	struct sconst **sc, *next;

	ind = hash(rs->str.c.s, STSIZE);
	for (sc = &sctab[ind]; (*sc)->rs != rs; sc = &(*sc)->next);
	if (--(*sc)->refs)
		return;
	dec_ref_rstr(rs);
	next = (*sc)->next;
	Discard(*sc, struct sconst);
	*sc = next;
}


