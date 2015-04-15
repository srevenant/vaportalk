/* This source code is in the public domain. */

/* var.c: Maintains variable indices hash table */

#include "vt.h"
#define VTSIZE 203

/* Variable values are kept in a global array.	Variable names are
** kept in a hash table used to access indices.	 The function
** get_index() is called by walk.c. */

/* Variables are hashed to unordered chains. */

struct var {
	char *name;
	int ind;
	struct var *next;
};

static int vindex = 0;
static struct var *vtab[VTSIZE];

/* Return an index to a variable, creating one if necessary */
int get_vindex(name)
	char *name;
{
	int hval;
	struct var *vp;

	hval = hash(name, VTSIZE);
	for (vp = vtab[hval]; vp; vp = vp->next) {
		if (streq(vp->name, name))
			return vp->ind;
	}
	vp = New(struct var);
	vp->name = vtstrdup(name);
	vp->ind = vindex++;
	vp->next = vtab[hval];
	vtab[hval] = vp;
	return vp->ind;
}

void write_vartab(fp)
	FILE *fp;
{
	int i;
	struct var *vp;

	fputs("VARS\n", fp);
	for (i = 0; i < VTSIZE; i++) {
		for (vp = vtab[i]; vp; vp = vp->next)
			fprintf(fp, "%d %s\n", vp->ind, vp->name);
	}
}


