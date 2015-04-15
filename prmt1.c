/* This source code is in the public domain. */

/* prmt1.c: Primitive functions: screen handling, windows */

#include "vt.h"
#include "prmt.h"

extern int curs_loc;
#define CURS_ELSEWHERE 2

extern Estate *gen_read;
extern Dframe suspend_frame;
extern int rows;

/* Screen handling primitives */

PDECL(pr_write)
{
	for (; argc--; argv++) {
		Tcheckgen(argv->type != F_SPTR);
		if (Sinbounds(*argv))
			vtwrite(Scstr(*argv));
	}
}

PDECL(pr_cmove)
{
	Tcheck2(F_INT, F_INT);
	cmove(Int1, Int2);
	bflushfunc();
}

PDECL(pr_scroll)
{
	Tcheck2(F_INT, F_INT);
	scroll(Int1, Int2);
	bflushfunc();
}

PDECL(pr_scr_fwd)
{
	Tcheck1(F_INT);
	scr_fwd(Int);
	bflushfunc();
}

PDECL(pr_scr_rev)
{
	Tcheck2(F_INT, F_INT);
	scr_rev(Int);
	bflushfunc();
}

PDECL(pr_clrscr)
{
	clrscr();
}

PDECL(pr_clreol)
{
	clreol();
}

PDECL(pr_bold)
{
	bold(Dftrue(Dp));
}

PDECL(pr_curs_reset)
{
	curs_loc = CURS_ELSEWHERE;
}

/* Window primitives */

PDECL(pr_split)
{
	Tcheck2(F_WIN, F_INT);
	if (Int2 >= Un1->Wtop + 2 && Int2 <= Un1->Wbot - 2)
		Dset_elem(*rf, F_WIN, Dunode, split_window(Un1, Int2));
	Set_err_null("Argument out of range");
}

PDECL(pr_close)
{
	Tcheck2(F_WIN, F_INT);
	Dset_int(*rf, 0);
	Int2 = Int2 ? Int2 : Un1->prev->dummy ? 1 : -1;
	if ((!Un1->prev->dummy || Int2 > 0) && (!Un1->next->dummy || Int2 < 0))
		close_window(Un1, Int2);
	else
		rf->Dval = -1;
	Set_err_neg("Cannot close window in given direction");
}

PDECL(pr_resize)
{
	int bot;

	Tcheck2(F_WIN, F_INT);
	bot = Un1->next->dummy ? rows - 2 : Un1->next->Wbot - 2;
	if (Int2 <= bot && Int2 >= Un1->Wtop + 2) {
		resize_window(Un1, Int2);
		Dset_int(*rf, 0);
	} else
		Dset_int(*rf, -1);
	Set_err_neg("Argument out of range");
}

PDECL(pr_set_termread)
{
	Tcheckgen(T1 != F_WIN || T2 != F_FPTR && T2 != F_NULL);
	Un1->Wtermread = T2 == F_FPTR ? Dp2.Dfunc : NULL;
}

PDECL(pr_set_obj)
{
	Dframe **objptr;

	Tcheckgen(T1 != F_WIN && T1 != F_RMT);
	objptr = (T1 == F_WIN) ? &Un1->Wobj : &Un1->Robj;
	if (*objptr)
		deref_frame(*objptr);
	else
		*objptr = New(Dframe);
	**objptr = Dp2;
	ref_frame(*objptr);
}

PDECL(pr_obj)
{
	Dframe *obj;

	Tcheckgen(T != F_WIN && T != F_RMT);
	obj = (T == F_WIN) ? Un->Wobj : Un->Robj;
	if (obj)
		*rf = *obj;
}

PDECL(pr_echo)
{
	Unode *win = Cur_win;

	if (argc && argv[0].type == F_WIN) {
		win = argv[0].Dunode;
		argv++;
		argc--;
	}
	while (argc--) {
		Tcheckgen(argv->type != F_SPTR);
		output(win, Soastr(*argv));
		argv++;
	}
}

PDECL(pr_display)
{
	Tcheckgen(T1 != F_WIN || T2 != F_RMT && T2 != F_NULL);
	if (T2 == F_RMT && Un2->Rwin) {
		Dset_int(*rf, (Un2->Rwin == Un1) ? 0 : -1);
		Set_err_neg("Remote already displayed");
		return;
	}
	vtc_errflag = 0;
	if (Un1->Wrmt)
		Un1->Wrmt->Rwin = NULL;
	Un1->Wrmt = (T2 == F_RMT) ? Un2 : NULL;
	if (T2 == F_RMT)
		Un2->Rwin = Un1;
	update_echo_mode();
	Dset_int(*rf, 0);
}

PDECL(pr_read)
{
	Estate *image;

	Tcheckgen(argc > 1 || argc == 1 && T != F_WIN && T != F_RMT);
	image = suspend(argc);
	add_ioqueue(!argc ? &gen_read : T == F_WIN ? &Un->Wrstack
		    : &Un->Rrstack, image);
	*rf = suspend_frame;
}

PDECL(pr_reread)
{
	Estate *image, **queue;

	Tcheckgen(argc > 1 || argc == 1 && T != F_WIN && T != F_RMT);
	image = suspend(argc);
	queue = !argc ? &gen_read : T == F_WIN ? &Un->Wrstack : &Un->Rrstack;
	image->next = *queue;
	*queue = image;
	*rf = suspend_frame;
}

PDECL(pr_pass)
{
	Istr *is;

	Tcheckgen(T1 != F_WIN && T1 != F_RMT || T2 != F_SPTR);
	is = Dp2.Dspos ? istr_c(Socstr(Dp2)) : istr_rs(Dp2.Distr->rs);
	if (T1 == F_WIN)
		give_window(Un1, is);
	else
		give_remote(Un1, is, 0);
}

PDECL(pr_win_top)
{
	Tcheck1(F_WIN);
	Dset_int(*rf, Un->Wtop);
}

PDECL(pr_win_bottom)
{
	Tcheck1(F_WIN);
	Dset_int(*rf, Un->Wbot);
}

PDECL(pr_win_col)
{
	Tcheck1(F_WIN);
	Dset_int(*rf, Un->Wcol);
}

PDECL(pr_win_rmt)
{
	Tcheck1(F_WIN);
	if (Un->Wrmt)
		Dset_elem(*rf, F_RMT, Dunode, Un->Wrmt);
}

PDECL(pr_win_termread)
{
	Tcheck1(F_WIN);
	if (Un->Wtermread)
		Dset_elem(*rf, F_FPTR, Dfunc, Un->Wtermread);
}


