/* This source code is in the public domain. */

/* prmt4.c: Primitive functions: functions, misc, files */

#include "vt.h"
#include "prmt.h"

#ifdef PROTOTYPES
static Prog *prog_pcall(int, int);
#endif

extern String ptext;
extern Dframe *dstack, frame_error, suspend_frame;
extern dpos, dsize;
extern Unode rmt_ring, win_ring, key_ring, file_ring;
extern Array *gvars;
extern Estate *events;
extern int loading;
static Unode *dummies[] = { &rmt_ring, &win_ring, &key_ring, &file_ring };
#define ringdummy(t) (dummies[(t) - F_RMT])
static Dframe *ftemp = NULL;
static int tempsize = 0;

/* Generates a temporary program to run a primitive */

static Prog *prog_pcall(pnum, argc)
	int pnum, argc;
{
	Prog *new;

	new = New(Prog);
	new->code = Newarray(Instr, 3);
	new->code[0].type = I_PCALL;
	new->code[1].pc.pnum = pnum;
	new->code[1].pc.argc = argc;
	new->code[2].type = I_STOP;
	new->refs = 1;
	new->avarc = new->reqargs = 0;
	new->lvarc = 0;
	return new;
}

/* Function primitives */

PDECL(pr_find_func)
{
	Func *func;				    
	Tcheck1(F_SPTR);
	func = find_func(Soastr(Dp));
	if (!func || !func->cmd)
		return;
	Dset_elem(*rf, F_FPTR, Dfunc, func);
}

PDECL(pr_find_prmt)
{
	int pnum;

	Tcheck1(F_SPTR);
	pnum = find_prmt(Soastr(Dp));
	if (pnum == -1)
		return;
	Dset_elem(*rf, F_FPTR, Dpnum, pnum);
}

PDECL(pr_func_name)
{
	Tcheck1(F_FPTR);
	Dset_sptr(*rf, istr_s(Dp.Dfunc->name), 0);
}

PDECL(pr_callv)
{
	int fargc = 0, copy, i;
	Dframe *df, arg0;

	Tcheckgen(argv->type != F_FPTR && argv->type != F_PPTR);
	arg0 = argv[0];
	for (i = 1; i < argc;) {
		df = &argv[i];
		Tcheckgen(df->type != F_INT || i == argc - 1
			  || df->Dval >= 0 && df[1].type != F_APTR
			  || df->Dval < 0 && argc - i - 1 < -df->Dval);
		fargc += (df->Dval < 0) ? -df->Dval : df->Dval;
		i += (df->Dval < 0) ? -df->Dval + 1 : 2;
	}
	if (fargc) {
		if (tempsize < fargc) {
			ftemp = tempsize ? Rearray(ftemp, Dframe, tempsize,
				fargc) : Newarray(Dframe, fargc);
			tempsize = fargc;
		}
		for (df = ftemp, i = 1; i < argc;) {
			if (argv[i].Dval < 0) {
				Copy(&argv[i + 1], df, -argv[i].Dval, Dframe);
				df += -argv[i].Dval;
				i += -argv[i].Dval + 1;
				continue;
			}
			copy = min(argv[i].Dval,
				   argv[i + 1].Asize - argv[i + 1].Dapos);
			copy = max(copy, 0);
			if (copy)
				Copy(&Aelem(argv[i + 1]), df, copy, Dframe);
			df += copy;
			while (copy++ < argv[i].Dval)
				df++->type = F_NULL;
			i += 2;
		}
		ref_frames(fargc, ftemp);
		deref_frames(argc, argv);
		dpos -= argc;
		while (dpos + fargc > dsize)
			double_dstack();
		Copy(ftemp, &dstack[dpos], fargc, Dframe);
		move_frames_refs(fargc, ftemp, &dstack[dpos]);
		dpos += fargc;
	} else {
		deref_frames(argc, argv);
		dpos -= argc;
	}
	if (arg0.type == F_FPTR) {
		if (fargc < arg0.Dfunc->cmd->reqargs)
			Prmterror(("Not enough arguments to function %s",
				  arg0.Dfunc->name));
		cpush(arg0.Dfunc->cmd, fargc, 1, 0);
	} else
		cpush(prog_pcall(arg0.Dpnum, fargc), 0, 1, fargc);
	Dset_elem(*rf, F_EXCEPT, Dval, INTERP_NOFREE);
}

PDECL(pr_detach)
{
	Tcheckgen(T1 != F_FPTR && T1 != F_PPTR);
	if (T1 == F_FPTR) {
		if (argc <= Dp1.Dfunc->cmd->reqargs)
			Prmterror(("Not enough arguments to function %s",
				  Dp1.Dfunc->name));
		cpush(Dp1.Dfunc->cmd, argc - 1, 0, 0);
	} else
		cpush(prog_pcall(Dp1.Dpnum, argc - 1), 0, 0, argc - 1);
	interp();
	deref_frame(&dstack[dpos - 1]);
	dstack[dpos - 1].type = F_NULL;
	Dset_elem(*rf, F_EXCEPT, Dval, INTERP_NOFREE);
}

PDECL(pr_abort)
{
	*rf = frame_error;
}

/* Array primitives */

PDECL(pr_alloc)
{
	Tcheckgen(argc < 1 || argc > 2 || T1 != F_INT
		  || argc == 2 && T2 != F_ASSOC);
	Int1 = max(Int, 1);
	if (argc == 1) {
		Dset_aptr(*rf, add_array(Int1, dfalloc(Int1), 0, 0, 0), 0);
		rf->Darray->r.refs = 0;
		return;
	}
	Dset_elem(*rf, F_PLIST, Dplist, New(Plist));
	rf->Dplist->assoc = Dp2.Dassoc;
	rf->Dplist->array = add_array(Int1, dfalloc(Int1), 0, 0, 1);
	rf->Dplist->array->r.plist = rf->Dplist;
	rf->Dplist->refs = 0;
}

PDECL(pr_new_assoc)
{
	Dset_elem(*rf, F_ASSOC, Dassoc, New(int));
	*rf->Dassoc = 0;
}

PDECL(pr_lookup)
{
	Plist *p = NULL;

	if (T1 == F_ASSOC) {
		Dset_int(*rf, lookup(Dp1.Dassoc, Soastr(Dp2)));
		return;
	}
	if (T1 == F_PLIST)
		p = Dp1.Dplist;
	else if (T1 == F_WIN && Un1->Wobj && Un1->Wobj->type == F_PLIST)
		p = Un1->Wobj->Dplist;
	else if (T1 == F_RMT && Un1->Robj && Un1->Robj->type == F_PLIST)
		p = Un1->Robj->Dplist;
	Tcheckgen(!p || T2 != F_SPTR);
	Dset_aptr(*rf, p->array, lookup(p->assoc, Soastr(Dp2)));
}

PDECL(pr_acopy)
{
	int copy, i;

	Tcheck3(F_APTR, F_APTR, F_INT);
	if (Int3 <= 0)
		return;
	if (Int3 > Dp1.Asize - Dp1.Dapos) {
		if (Dp1.Darray->fixed)
			Int3 = Dp1.Asize - Dp1.Dapos;
		else
			extend_array(Dp1.Darray, Dp1.Dapos + Int3);
	}
	if (tempsize < Int3) {
		ftemp = tempsize ? Rearray(ftemp, Dframe, tempsize, Int3)
				 : Newarray(Dframe, Int3);
		tempsize = Int3;
	}
	copy = min(Int3, Dp2.Asize - Dp2.Dapos);
	copy = max(copy, 0);
	if (copy)
		Copy(&Aelem(Dp2), ftemp, copy, Dframe);
	for (i = copy; i < Int3; i++)
		ftemp[i].type = F_NULL;
	ref_frames(Int3, ftemp);
	deref_frames(Int3, &Aelem(Dp1));
	Copy(ftemp, &Aelem(Dp1), Int3, Dframe);
	move_frames_refs(Int3, ftemp, &Aelem(Dp1));
}

PDECL(pr_base)
{
	if (T == F_PLIST) {
		Dset_aptr(*rf, Dp.Dplist->array, 0);
		return;
	}
	Tcheckgen(T != F_APTR && T != F_SPTR);
	*rf = Dp;
	*((T == F_APTR) ? &rf->Dapos : &rf->Dspos) = 0;
}

PDECL(pr_garbage)
{
	rf->type = F_INT;
	rf->Dval = garbage();
}

/* Miscellaneous primitives */

PDECL(pr_parse)
{
	Tcheck1(F_SPTR);
	parse(Soastr(Dp));
}

PDECL(pr_head)
{
	Tcheckgen(T != F_INT || Int < F_RMT || Int > F_FILE);
	if (!ringdummy(Int)->next->dummy)
		Dset_elem(*rf, Int, Dunode, ringdummy(Int)->next);
}

PDECL(pr_tail)
{
	Tcheckgen(T != F_INT || Int < F_RMT || Int > F_FILE);
	if (!ringdummy(Int)->prev->dummy)
		Dset_elem(*rf, Int, Dunode, ringdummy(Int)->prev);
}

PDECL(pr_next)
{
	Tcheckgen(T < F_RMT || T > F_FILE);
	if (!Un->next->dummy)
		Dset_elem(*rf, Dp.type, Dunode, Un->next);
}

PDECL(pr_prev)
{
	Tcheckgen(T < F_RMT || T > F_FILE);
	if (!Un->prev->dummy)
		Dset_elem(*rf, Dp.type, Dunode, Un->prev);
}

PDECL(pr_type)
{
	Dset_int(*rf, Dp.type);
}

PDECL(pr_find_var)
{
	Tcheck1(F_SPTR);
	Dset_aptr(*rf, gvars, get_vindex(Soastr(Dp)));
}

PDECL(pr_sleep)
{
	Estate *image, **e;

	Tcheck1(F_INT);
	image = suspend(1);
	image->timer = time((long *) NULL) + Int;
	for (e = &events; *e && (*e)->timer <= image->timer; e = &(*e)->next);
	image->next = *e;
	*e = image;
	*rf = suspend_frame;
}

PDECL(pr_quit)
{
	cleanup();
	exit(0);
}

PDECL(pr_rndseed)
{
	Tcheck1(F_INT);
	srand(Int);
}


