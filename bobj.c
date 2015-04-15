/* This source code is in the public domain. */

/* bobj.c: Built-in objects table and corresponding functions */

#include "vt.h"
#include "prmt.h"
#include <errno.h>
#include <string.h>

#define BTSIZE 23
#define NUM_BOBJ (sizeof(btab) / sizeof(struct bobj))
#define BDECL(name) static void name(rf) Dframe *rf;
#define Errmsg (vtc_errmsg ? vtc_errmsg : strerror(errno))
#define Ctop (cstack[cpos - 1])

struct bobj {
	char *name;
	void (*func)();
	struct bobj *next;
};

static struct bobj *bhtab[BTSIZE];

static void
	bo_rows(),
	bo_cols(),
	bo_active(),
	bo_prompt(),
	bo_kbuf(),
	bo_kpos(),
	bo_cur_rmt(),
	bo_cur_win(),
	bo_argc(),
	bo_argv(),
	bo_time(),
	bo_rnd(),
	bo_wd(),
	bo_version(),
	bo_errflag(),
	bo_errmsg(),
	bo_null();

int vtc_errflag;
char *vtc_errmsg;
extern Unode *active_win, *cur_rmt;
extern String kbuf;
extern char *prompt;
extern int kpos, cpos, rows, cols, plen, errno;
extern Cframe *cstack;
extern Dframe frame_error;

static struct bobj btab[] = {
	{ "rows"	, bo_rows	, NULL },
	{ "cols"	, bo_cols	, NULL },
	{ "active"	, bo_active	, NULL },
	{ "prompt"	, bo_prompt	, NULL },
	{ "kbuf"	, bo_kbuf	, NULL },
	{ "kpos"	, bo_kpos	, NULL },
	{ "cur_win"	, bo_cur_win	, NULL },
	{ "cur_rmt"	, bo_cur_rmt	, NULL },
	{ "argc"	, bo_argc	, NULL },
	{ "argv"	, bo_argv	, NULL },
	{ "time"	, bo_time	, NULL },
	{ "wd"		, bo_wd		, NULL },
	{ "rnd"		, bo_rnd	, NULL },
	{ "version"	, bo_version	, NULL },
	{ "errflag"	, bo_errflag	, NULL },
	{ "errmsg"	, bo_errmsg	, NULL },
	{ "NULL"	, bo_null	, NULL }
};

void init_bobj()
{
	int i, ind;

	for (i = 0; i < NUM_BOBJ; i++) {
		ind = hash(btab[i].name, BTSIZE);
		btab[i].next = bhtab[ind];
		bhtab[ind] = &btab[i];
	}
}

void (*find_bobj(name))()
	char *name;
{
	struct bobj *bp;

	for (bp = bhtab[hash(name, BTSIZE)]; bp; bp = bp->next) {
		if (streq(bp->name, name))
			return bp->func;
	}
	return NULL;
}

void assign_bobj(rf, bobj, dp)
	Dframe *rf, *dp;
	void (*bobj)();
{
	int i;

	if (bobj == bo_active && dp->type == F_WIN) {
		new_active_win(dp->Dunode);
	} else if (bobj == bo_prompt && dp->type == F_SPTR) {
		change_prompt(Soastr(*dp), Solen(*dp));
	} else if (bobj == bo_kbuf && dp->type == F_SPTR) {
		curs_input();
		input_clear();
		s_cpy(&kbuf, Socstr(*dp));
		for (i = 0; i < kbuf.c.l; i++) {
			if (!isprint(kbuf.c.s[i]))
				s_delete(&kbuf, i--, 1);
		}
		if (kpos > kbuf.c.l)
			kpos = kbuf.c.l;
		input_draw();
	} else if (bobj == bo_kpos && dp->type == F_INT) {
		dp->Dval = max(0, min(dp->Dval, kbuf.c.l));
		input_cmove(dp->Dval);
		kpos = dp->Dval;
	} else if (bobj == bo_argc && dp->type == F_INT) {
		cstack[cpos - 1].uargc = dp->Dval;
	} else if (bobj == bo_argv && dp->type == F_APTR
		 && dp->Darray == cstack[cpos - 1].avars) {
		cstack[cpos - 1].uargv = dp->Dapos;
	} else if (bobj == bo_wd && dp->type == F_SPTR) {
		Set_err(chdir(expand(Soastr(*dp))) == -1, NULL);
	} else
		Prmterror(("Invalid builtin assignment"));
}

BDECL(bo_rows)	  { Dset_int(*rf, rows); }
BDECL(bo_cols)	  { Dset_int(*rf, cols); }
BDECL(bo_active)  { Dset_elem(*rf, F_WIN, Dunode, active_win); }
BDECL(bo_prompt)  { Dset_sptr(*rf, istr_sl(prompt, plen), 0); }
BDECL(bo_kbuf)	  { Dset_sptr(*rf, istr_c(kbuf.c), 0); }
BDECL(bo_kpos)	  { Dset_int(*rf, kpos); }
BDECL(bo_cur_win) { if (Cur_win) Dset_elem(*rf, F_WIN, Dunode, Cur_win); }
BDECL(bo_cur_rmt) { if (Cur_rmt) Dset_elem(*rf, F_RMT, Dunode, Cur_rmt); }
BDECL(bo_argc)	  { Dset_int(*rf, Ctop.uargc); }
BDECL(bo_argv)	  { Dset_aptr(*rf, Ctop.avars, Ctop.uargv); }
BDECL(bo_time)	  { Dset_int(*rf, time((long *) NULL)); }

BDECL(bo_wd)
{
	char buf[1024];

	if (!getcwd(buf, 1024))
		return;
	Dset_sptr(*rf, istr_s(buf), 0);
}

BDECL(bo_rnd) { Dset_int(*rf, rand()); }
BDECL(bo_version) { Dset_sptr(*rf, istr_s(VERSION), 0); }
BDECL(bo_errflag) { Dset_int(*rf, vtc_errflag); }
BDECL(bo_errmsg)  { Dset_sptr(*rf, istr_s(Errmsg), 0); }
BDECL(bo_null)	  {}


