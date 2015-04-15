/* This source code is in the public domain. */

/* const.c: Table of constants */

#include "vt.h"

#define CTSIZE 23
#define NUM_CONST (sizeof(consttab) / sizeof(Const))

static Const *chtab[CTSIZE];

static Const consttab[] = {
	{ "T_INT"	, F_INT	    , NULL },
	{ "T_PPTR"	, F_PPTR    , NULL },
	{ "T_BPTR"	, F_BPTR    , NULL },
	{ "T_RMT"	, F_RMT	    , NULL },
	{ "T_WIN"	, F_WIN	    , NULL },
	{ "T_KEY"	, F_KEY	    , NULL },
	{ "T_FILE"	, F_FILE    , NULL },
	{ "T_SPTR"	, F_SPTR    , NULL },
	{ "T_APTR"	, F_APTR    , NULL },
	{ "T_FPTR"	, F_FPTR    , NULL },
	{ "T_REG"	, F_REG	    , NULL },
	{ "T_ASSOC"	, F_ASSOC   , NULL },
	{ "T_PLIST"	, F_PLIST   , NULL },
	{ "T_NULL"	, F_NULL    , NULL },
	{ "K_CUP"	,  0	    , NULL },
	{ "K_CDOWN"	,  1	    , NULL },
	{ "K_CLEFT"	,  2	    , NULL },
	{ "K_CRIGHT"	,  3	    , NULL },
	{ "K_CHOME"	,  4	    , NULL },
	{ "K_CEND"	,  5	    , NULL },
	{ "K_CWLEFT"	,  6	    , NULL },
	{ "K_CWRIGHT"	,  7	    , NULL },
	{ "K_BSPC"	,  8	    , NULL },
	{ "K_BWORD"	,  9	    , NULL },
	{ "K_BHOME"	, 10	    , NULL },
	{ "K_DBUF"	, 11	    , NULL },
	{ "K_DCH"	, 12	    , NULL },
	{ "K_DWORD"	, 13	    , NULL },
	{ "K_DEND"	, 14	    , NULL },
	{ "K_REFRESH"	, 15	    , NULL },
	{ "K_REDRAW"	, 16	    , NULL },
	{ "K_MODE"	, 17	    , NULL },
	{ "K_PROCESS"	, 18	    , NULL },
	{ "SEEK_SET"	,  0	    , NULL },
	{ "SEEK_CUR"	,  1	    , NULL },
	{ "SEEK_END"	,  2	    , NULL },
	{ "HIGH"	,  0	    , NULL },
	{ "LOW"		,  1	    , NULL },
	{ "INTR"	,  2	    , NULL },
	{ "EOF"		, EOF	    , NULL },
	{ "NSUBEXP"	, NSUBEXP   , NULL }
};

#define NUM_CONST (sizeof(consttab) / sizeof(Const))

void init_const()
{
	int i, ind;

	for (i = 0; i < NUM_CONST; i++) {
		ind = hash(consttab[i].name, CTSIZE);
		consttab[i].next = chtab[ind];
		chtab[ind] = &consttab[i];
	}
}

Const *find_const(name)
	char *name;
{
	Const *cp;

	for (cp = chtab[hash(name, CTSIZE)]; cp; cp = cp->next) {
		if (streq(cp->name, name))
			return cp;
	}
	return NULL;
}

