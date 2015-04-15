/* This source code is in the public domain. */

/* interp.c: Execute linear compiled VTC code */

#include "vt.h"
#include "prmt.h"

#ifdef PROTOTYPES
static int pcall(Dframe *, int, int);
static void cpop(void);
static void cpop_normal(void);
static void abort_interp(void);
static void revise_cframes(Cframe *, int, Dframe *, Dframe *);
static void revise_estates(Estate *, Dframe *, Dframe *);
static void interp_context(Unode *, Unode *);
#else
static void revise_cframes(), revise_estates(), cpop(), cpop_normal();
static void abort_interp();
#endif

/* The data and call stacks */
#define INIT_SIZE 64
Dframe *dstack;				/* The data stack	    */
Cframe *cstack;				/* The call stack	    */
int dpos = 0, cpos = 0;			/* Positions in both stacks */
int dsize = INIT_SIZE;			/* Allocated space	    */
int curprmt;				/* Current prmt number	    */
static int csize = INIT_SIZE;
#define INIT_GVARS 100
Array *gvars;				/* Global variable values   */
extern int break_pending;
extern Unode *cur_rmt, *active_win;
extern Unode rmt_ring, win_ring;
extern Estate *events, *gen_read, *gen_high, *gen_low;

/* Short-cut for Cinstr */
static Instr *ins;

extern Prmt prmtab[];
int iwidth[] =
 { 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1 };

#define Ctop	(cstack[cpos - 1])
#define Cinstr	(Ctop.instr)
#define Cprog	(Ctop.prog)
#define Dtop	(dstack[dpos - 1])
#define Dextend if (1) { if (dpos == dsize) double_dstack(); dpos++; } else
#define Dextendn(n) if (1) { while (dpos + (n) > dsize) double_dstack(); } else
#define Dpush(df) if (1) { Dextend; Dtop = (df); } else
#define Dpop() deref_frame(&dstack[--dpos])
#define Ierror(x) if (1) { outputf x; finish_error(); \
			   abort_interp(); return; } else
#define Set_ins(x) if (1) { ins = (x); update = 0; } else

void init_interp()
{
	dstack = Newarray(Dframe, INIT_SIZE);
	cstack = Newarray(Cframe, INIT_SIZE);
	gvars = add_array(INIT_GVARS, dfalloc(INIT_GVARS), 1, 0, 0);
}

/* Interprets program on top of call stack until the highest detached
** frame finishes execution, or the program is aborted or suspended. */
void interp()
{
	Dframe *dp, df;
	Func *func;
	int update, val;

	ins = Cinstr;
    while (1) {
	if (break_pending) {
		abort_interp();
		return;
	}
	update = 1;
	switch(ins->type) {
	    case I_ICONST:
		Dextend;
		Dset_int(Dtop, ins[1].iconst);
	    Case I_SCONST:
		Dextend;
		Dset_sptr_ref(Dtop, istr_rs(ins[1].sconst), 0);
	    Case I_PCALL:
		if (!pcall(&df, ins[1].pc.argc, ins[1].pc.pnum))
			return;
		if (df.type != F_EXCEPT) {
			Dpush(df);
			move_frame_refs(&df, &Dtop);
		}
		update = 0;
	    Case I_FCALL:
		func = ins[2].func;
		if (!func->cmd || ins[1].argc < func->cmd->reqargs)
			Ierror((!func->cmd ? "Undefined function %s"
			       : "Not enough arguments to %s", func->name));
		Cinstr = ins + 3;
		cpush(func->cmd, ins[1].argc, 1, 0);
		Set_ins(Cinstr);
	    Case I_EXEC:
		dp = &dstack[dpos - ins[1].argc - 1];
		if (dp->type == F_PPTR) {
			if (!pcall(dp, ins[1].argc, dp->Dpnum))
				return;
		} else if (dp->type == F_FPTR) {
			if (ins[1].argc < dp->Dfunc->cmd->reqargs)
				Ierror(("Not enough arguments to %s",
				       dp->Dfunc->name));
			Cinstr = ins + 2;
			func = dp->Dfunc;
			move_frames_refs(ins[1].argc, dp + 1, dp);
			Copy(dp + 1, dp, ins[1].argc, Dframe);
			dpos--;
			cpush(func->cmd, ins[1].argc, 1, 0);
			ins = Cinstr;
		} else
			Ierror(("Invalid function call"));
		update = 0;
	    Case I_BOBJ:
		Dextend;
		Dset_elem(Dtop, F_BPTR, Dbobj, ins[1].bobj);
	    Case I_GVAR:
		Dextend;
		Dset_aptr(Dtop, gvars, ins[1].tnum);
	    Case I_AVAR:
		Dextend;
		Dset_aptr(Dtop, Ctop.avars, ins[1].tnum);
		add_fref(&Dtop.Darray->r.fr, &Dtop);
	    Case I_LVAR:
		Dextend;
		Dset_aptr(Dtop, Ctop.lvars, ins[1].tnum);
		add_fref(&Dtop.Darray->r.fr, &Dtop);
	    Case I_PPTR:
		Dextend;
		Dset_elem(Dtop, F_PPTR, Dpnum, ins[1].pnum);
	    Case I_FPTR:
		func = ins[1].func;
		if (!func->cmd)
			Ierror(("Undefined function %s", func->name));
		Dextend;
		Dset_elem(Dtop, F_FPTR, Dfunc, ins[1].func);
	    Case I_JMPF:
		if (Dffalse(Dtop))
			Set_ins(ins[1].loc);
		Dpop();
	    Case I_JMPT:
		if (Dftrue(Dtop))
			Set_ins(ins[1].loc);
		Dpop();
	    Case I_JMPPF:
		if (Dffalse(Dtop))
			Set_ins(ins[1].loc);
		else	
			Dpop();
	    Case I_JMPPT:
		if (Dftrue(Dtop))
			Set_ins(ins[1].loc);
		else
			Dpop();
	    Case I_JMP:
		Set_ins(ins[1].loc);
	    Case I_CVTB: /* Careful */
		dp = &Dtop;
		deref_frame(dp);
		dp->Dval = Dftrue(*dp);
		dp->type = F_INT;
	    Case I_EVAL:
		dp = &Dtop;
		if (dp->type == F_SPTR) {
			val = (unsigned char) Soelem(*dp);
			dec_ref_istr(dp->Distr);
			Dset_int(*dp, val);
		} else if (dp->type == F_APTR) {
			if (Ainbounds(*dp))
				df = dp->Darray->vals[dp->Dapos];
			else
				df.type = F_NULL;
			ref_frame(&df);
			deref_frame(dp);
			*dp = df;
			move_frame_refs(&df, dp);
		} else if (dp->type == F_BPTR) {
			dp->type = F_NULL;
			(*dp->Dbobj)(dp);
			ref_frame(dp);
		} else
			Ierror(("Invalid pointer dereferenced"));
	    Case I_NULL:
		Dextend;
		Dtop.type = F_NULL;
	    Case I_POP:
		Dpop();
	    Case I_DUP:
		Dextend;
		Dtop = dstack[dpos - 2];
		ref_frame(&Dtop);
	    Case I_STOP:
		if (Ctop.attached) {
			cpop_normal();
			ins = Cinstr;
			update = 0;
		} else {
			cpop_normal();
			Dpop();
			return;
		}
	}
	if (update)
		ins += iwidth[ins->type];
    }
}

static int pcall(dp, argc, pnum)
	Dframe *dp;
	int argc, pnum;
{
	Dframe *argv = &dstack[dpos - argc];
	int pargc = prmtab[pnum].argc;

	curprmt = pnum;
	Cinstr = ins + 2;
	if (pargc >= 0 && argc != pargc) {
		outputf("Too %s arguments to %s",
			argc < pargc ? "few" : "many", prmtab[pnum].name);
		finish_error();
		abort_interp();
		return 0;
	}
	dp->type = F_NULL;
	(*prmtab[pnum].func)(dp, argc, argv);
	ins = Cinstr;
	if (dp->type == F_EXCEPT) {
		if (dp->Dval == INTERP_ERROR)
			abort_interp();
		return dp->Dval == INTERP_NOFREE;
	}
	ref_frame(dp);
	deref_frames(argc, &dstack[dpos - argc]);
	dpos -= argc;
	return 1;
}

static void cpop()
{
	dec_ref_prog(Ctop.prog);
	destroy_pointers(Ctop.avars->r.fr);
	arfree(Ctop.avars);
	destroy_pointers(Ctop.lvars->r.fr);
	arfree(Ctop.lvars);
	cpos--;
}

static void cpop_normal()
{
	if (Ctop.dstart != dpos - 1) {
		deref_frames(dpos - Ctop.dstart - 1, &dstack[Ctop.dstart]);
		dstack[Ctop.dstart] = Dtop;
		move_frame_refs(&Dtop, &dstack[Ctop.dstart]);
		dpos = Ctop.dstart + 1;
	}
	cpop();
}

static void abort_interp()
{
	while (Ctop.attached)
		cpop();
	deref_frames(dpos - Ctop.dstart, &dstack[Ctop.dstart]);
	dpos = Ctop.dstart;
	cpop();
}

static void revise_cframes(cf, num, oldstack, newstack)
	Cframe *cf;
	int num;
	Dframe *oldstack, *newstack;
{
	while (num--) {
		cf->avars->vals = cf->avars->vals - oldstack + newstack;
		cf->lvars->vals = cf->lvars->vals - oldstack + newstack;
		cf++;
	}
}

static void revise_estates(es, oldstack, newstack)
	Estate *es;
	Dframe *oldstack, *newstack;
{
	for (; es; es = es->next)
		revise_cframes(es->cimage, es->cframes, oldstack, newstack);
}

void double_dstack()
{
	Dframe *new;
	Unode *un;

	new = Newarray(Dframe, dsize * 2);
	Copy(dstack, new, dpos, Dframe);
	move_frames_refs(dpos, dstack, new);
	revise_cframes(cstack, cpos, dstack, new);
	for (un = rmt_ring.next; !un->dummy; un = un->next)
		revise_estates(un->Rrstack, dstack, new);
	for (un = win_ring.next; !un->dummy; un = un->next) {
		revise_estates(un->Wghstack, dstack, new);
		revise_estates(un->Wglstack, dstack, new);
		revise_estates(un->Wrstack, dstack, new);
	}
	revise_estates(events, dstack, new);
	revise_estates(gen_read, dstack, new);
	revise_estates(gen_high, dstack, new);
	revise_estates(gen_low, dstack, new);
	Discardarray(dstack, Dframe, dsize);
	dstack = new;
	dsize *= 2;
}

void cpush(prog, argc, attached, progress)
	Prog *prog;
	int argc, attached, progress;
{
	Cframe *cf;
	int i, argc_alloc = max(argc, prog->avarc);

	if (cpos == csize)
		Double(cstack, Cframe, csize);
	Dextendn(prog->lvarc + argc_alloc - argc);
	cf = &cstack[cpos++];
	prog->refs++;
	cf->prog = prog;
	cf->instr = prog->code;
	cf->dstart = dpos - argc - progress;
	cf->attached = attached;
	for (i = 0; i < prog->lvarc + argc_alloc - argc; i++, dpos++)
		dstack[dpos].type = F_NULL;
	cf->avars = add_array(argc_alloc, &dstack[cf->dstart], 1, 1, 0);
	cf->lvars = add_array(prog->lvarc, &dstack[cf->dstart + argc_alloc],
			      1, 1, 0);
	cf->avars->r.fr = cf->lvars->r.fr = NULL;
	cf->uargc = argc;
	cf->uargv = 0;
}

/* Interface to interpreter */

/* Set up a cur_rmt and cur_win context and call interp() */
static void interp_context(win, rmt)
	Unode *win, *rmt;
{
	Unode *old_win, *old_rmt;
	int owin_id, ormt_id;

	old_win = cur_win;
	owin_id = cur_win ? cur_win->id : 0;
	old_rmt = cur_rmt;
	ormt_id = cur_rmt ? cur_rmt->id : 0;
	cur_win = win;
	cur_rmt = rmt;
	interp();
	cur_win = (old_win && old_win->id == owin_id) ? old_win : NULL;
	cur_rmt = (old_rmt && old_rmt->id == ormt_id) ? old_rmt : NULL;
}

/* Start up a program with no arguments */
void run_prog(prog)
	Prog *prog;
{
	if (prog->reqargs) {
		outputf("Function %s called with too few arguments\n",
			lookup_prog(prog));
		return;
	}
	cpush(prog, 0, 0, 0);
	interp();
}

void run_prog_istr(prog, is, win, rmt)
	Prog *prog;
	Istr *is;
	Unode *win, *rmt;
{
	Dframe df;

	if (prog->reqargs > 1) {
		outputf("Function %s called with too few arguments\n",
			lookup_prog(prog));
		return;
	}
	Dset_sptr(df, is, 0);
	is->refs++;
	Dpush(df);
	cpush(prog, 1, 0, 0);
	interp_context(win, rmt);
}

/* Suspend execution and return an image of it */
Estate *suspend(argc)
	int argc;
{
	Estate *es;
	Cframe *cp;
	int nf;

	deref_frames(argc, &dstack[dpos - argc]);
	dpos -= argc;
	es = New(Estate);
	for (cp = &Ctop; cp->attached; cp--);
	es->dframes = nf = dpos - cp->dstart;
	es->dimage = Newarray(Dframe, nf);
	Copy(&dstack[cp->dstart], es->dimage, nf, Dframe);
	move_frames_refs(nf, &dstack[cp->dstart], es->dimage);
	dpos = cp->dstart;
	es->cframes = nf = cstack + cpos - cp;
	es->cimage = Newarray(Cframe, nf);
	Copy(cp, es->cimage, nf, Cframe);
	cpos = cp - cstack;
	es->win = cur_win;
	es->wid = cur_win ? cur_win->id : 0;
	es->rmt = cur_rmt;
	es->rid = cur_rmt ? cur_rmt->id : 0;
	return es;
}

/* Free up the information in an image deleted by the console */
void discard_estate(eptr)
	Estate **eptr;
{
	Estate *es = *eptr;
	int i;
	Cframe *cf;

	*eptr = es->next;
	deref_frames(es->dframes, es->dimage);
	Discardarray(es->dimage, Dframe, es->dframes);
	for (cf = es->cimage, i = 0; i < es->cframes; i++) {
		destroy_pointers(cf[i].avars->r.fr);
		arfree(cf[i].avars);
		destroy_pointers(cf[i].lvars->r.fr);
		arfree(cf[i].lvars);
	}
	Discardarray(es->cimage, Cframe, es->cframes);
	Discard(es, Estate);
}

/* Resume a suspended program */
void resume(eptr, dp)
	Estate **eptr;
	Dframe *dp;
{
	Estate *es = *eptr;
	int adjust, i;

	*eptr = es->next;
	if (dpos + es->dframes >= dsize)
		double_dstack();
	if (cpos + es->cframes >= csize)
		Double(cstack, Cframe, csize);
	adjust = es->cimage[0].dstart - dpos;
	if (adjust) {
		for (i = 0; i < es->cframes; i++) {
			es->cimage[i].dstart -= adjust;
			es->cimage[i].avars->vals -= adjust;
			es->cimage[i].lvars->vals -= adjust;
		}
	}
	Copy(es->dimage, &dstack[dpos], es->dframes, Dframe);
	move_frames_refs(es->dframes, es->dimage, &dstack[dpos]);
	Discardarray(es->dimage, Dframe, es->dframes);
	dpos += es->dframes;
	Copy(es->cimage, &cstack[cpos], es->cframes, Cframe);
	Discardarray(es->cimage, Cframe, es->cframes);
	cpos += es->cframes;
	Dpush(*dp);
	interp_context((es->win && es->wid == es->win->id) ? es->win : NULL,
		       (es->rmt && es->rid == es->rmt->id) ? es->rmt : NULL);
	Discard(es, Estate);
}

void resume_istr(eptr, is)
	Estate **eptr;
	Istr *is;
{
	Dframe df;

	Dset_sptr_ref(df, is, 0);
	is->refs++;
	resume(eptr, &df);
}

void resume_int(eptr, num)
	Estate **eptr;
	int num;
{
	Dframe df;

	Dset_int(df, num);
	resume(eptr, &df);
}

void destroy_pipe(elist)
	Estate *elist;
{
	while (elist)
		discard_estate(&elist);
}

void break_pipe(elist)
	Estate *elist;
{
	Dframe null;

	null.type = F_NULL;
	while (elist)
		resume(&elist, &null);
}

/* General routines for dealing with data frames */

void ref_frame(df)
	Dframe *df;
{
	switch (df->type) {
	    case F_WIN:
	    case F_RMT:
	    case F_KEY:
	    case F_FILE:
		add_fref(&df->Dunode->frefs, df);
	    Case F_SPTR:
		df->Distr->refs++;
	    Case F_APTR:
		if (df->Darray->fixed)
			add_fref(&df->Darray->r.fr, df);
		else if (df->Darray->isp)
			df->Darray->r.plist->refs++;
		else if (!df->Darray->intern)
			df->Darray->r.refs++;
	    Case F_REG:
		df->Dreg->refs++;
	    Case F_PLIST:
		df->Dplist->refs++;
	}
}

void ref_frames(num, frames)
	int num;
	Dframe *frames;
{
	while (num--)
		ref_frame(frames++);
}

void move_frame_refs(old, new)
	Dframe *old, *new;
{
	switch(old->type) {
	    case F_WIN:
	    case F_RMT:
	    case F_KEY:
	    case F_FILE:
		move_fref(old->Dunode->frefs, old, new);
	    Case F_APTR:
		if (old->Darray->fixed)
			move_fref(old->Darray->r.fr, old, new);
	}
}

void move_frames_refs(num, old, new)
	int num;
	Dframe *old, *new;
{
	while (num--)
		move_frame_refs(old++, new++);
}

void deref_frame(df)
	Dframe *df;
{
	switch(df->type) {
	    case F_WIN:
	    case F_RMT:
	    case F_KEY:
	    case F_FILE:
		elim_fref(&df->Dunode->frefs, df);
	    Case F_SPTR:
		dec_ref_istr(df->Distr);
	    Case F_APTR:
		if (df->Darray->fixed)
			elim_fref(&df->Darray->r.fr, df);
		else if (df->Darray->isp)
			dec_ref_plist(df->Darray->r.plist);
		else if (!df->Darray->intern)
			dec_ref_array(df->Darray);
	    Case F_REG:
		if (!--df->Dreg->refs) {
			if (df->Dreg->rs)
				dec_ref_rstr(df->Dreg->rs);
			free(df->Dreg);
		}
	    Case F_PLIST:
		dec_ref_plist(df->Dplist);
	}
}

void deref_frames(num, frames)
	int num;
	Dframe *frames;
{
	while (num--)
		deref_frame(frames++);
}

