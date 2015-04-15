/* This source code is in the public domain. */

/* key.c: Maintains the key ring and lookup table */

#include "vt.h"

#ifdef PROTOTYPES
static Unode *gen_add_key(char *);
#else
static Unode *gen_add_key();
#endif

Unode key_ring;				/* Dummy entry in keys ring	*/
Unode *klookup[128];			/* Lookup table for first chars */

/* Invariants:
** 1. All keys with the same first character are adjacent.
** 2. klookup[c] points to the first key starting with c, or NULL. */

void init_key()
{
	int i;

	for (i = 0; i < 128; i++)
		klookup[i] = NULL;
	key_ring.next = key_ring.prev = &key_ring;
	key_ring.dummy = 1;
	key_ring.Kseq = "";
}

static Unode *gen_add_key(seq)
	char *seq;
{
	Unode *kp, *new;

	if (kp = find_key(seq))
		return kp;
	new = unalloc();
	kp = klookup[lcase(*seq)];
	if (kp) {
		while (lcase(*kp->Kseq) == lcase(*seq))
			kp = kp->next;
		new->prev = kp->prev;
		new->next = kp;
		kp->prev = kp->prev->next = new;
	} else {
		new->prev = key_ring.prev;
		new->next = &key_ring;
		key_ring.prev = key_ring.prev->next = new;
		klookup[lcase(*seq)] = new;
	}
	new->Kseq = vtstrdup(seq);
	return new;
}

Unode *add_key_cmd(seq, cmd)
	char *seq;
	Func *cmd;
{
	Unode *new;

	new = gen_add_key(seq);
	new->Ktype = K_CMD;
	new->Kcmd = cmd;
	return new;
}

Unode *add_key_efunc(seq, efunc)
	char *seq;
	int efunc;
{
	Unode *new;

	new = gen_add_key(seq);
	new->Ktype = K_EFUNC;
	new->Kefunc = efunc;
	return new;
}

void del_key(key)
	Unode *key;
{
	char c = *key->Kseq;

	if (klookup[lcase(c)] == key) 
		klookup[lcase(c)] = (lcase(*key->next->Kseq) == lcase(c))
				    ? key->next : NULL;
	Discardstring(key->Kseq);
	key->prev->next = key->next;
	key->next->prev = key->prev;
	destroy_pointers(key->frefs);
	discard_unode(key);
}

Unode *find_key(seq)
	char *seq;
{
	Unode *kp;

	if (kp = klookup[lcase(*seq)]) {
		for (; lcase(*kp->Kseq) == lcase(*seq); kp = kp->next) {
			if (!strcasecmp(kp->Kseq + 1, seq + 1))
				return kp;
		}
	}
	return NULL;
}


