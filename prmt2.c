/* This source code is in the public domain. */

/* prmt2.c: Primitive functions: remotes, key buffer, keys */

#include "vt.h"
#include "prmt.h"

extern Estate *gen_high, *gen_low;
extern Dframe suspend_frame;
extern Unode *cur_rmt, *active_win;
extern int keyboard_input_waiting;

/* Remote primitives */

PDECL(pr_connect)
{
	Unode *rmt;

	Tcheck2(F_SPTR, F_INT);
	if (Sinbounds(Dp1)) {
		rmt = new_rmt(Sastr(Dp1), Int2);
		if (rmt)
			Dset_elem(*rf, F_RMT, Dunode, rmt);
	}
	Set_err_null(vtc_errmsg);
}

PDECL(pr_connect_shell)
{
	Unode *rmt;

	Tcheck1(F_SPTR);
	if (Sinbounds(Dp1)) {
		rmt = new_rmt_shell(Sastr(Dp1));
		if (rmt)
			Dset_elem(*rf, F_RMT, Dunode, rmt);
	}
	Set_err_null(vtc_errmsg);
}

PDECL(pr_disconnect)
{
	Tcheck1(F_RMT);
	vtc_errflag = 0;
	disconnect(Un);
}

PDECL(pr_set_netread)
{
	Tcheckgen(T1 != F_RMT || T2 != F_FPTR && T2 != F_NULL);
	Un1->Rnetread = (T2 == F_FPTR) ? Dp2.Dfunc : NULL;
}

PDECL(pr_set_promptread)
{
	Tcheckgen(T1 != F_RMT || T2 != F_FPTR && T2 != F_NULL);
	Un1->Rpromptread = (T2 == F_FPTR) ? Dp2.Dfunc : NULL;
}

PDECL(pr_set_back)
{
	Tcheck2(F_RMT, F_INT);
	Un1->Rback = Int2 ? 1 : 0;
}

PDECL(pr_set_busy)
{
	Tcheck2(F_RMT, F_INT);
	Un1->Rbusy = Int2 ? 1 : 0;
}

PDECL(pr_set_raw)
{
	Tcheck2(F_RMT, F_INT);
	Un1->Rraw = Int2 ? 1 : 0;
}

PDECL(pr_send)
{
	Unode *rmt = cur_rmt ? cur_rmt : active_win->Wrmt;

	Dset_int(*rf, 0);
	if (argv[0].type == F_RMT) {
		rmt = argv[0].Dunode;
		argv++;
		argc--;
	} else if (!rmt)
		return;
	while (argc--) {
		Tcheckgen(argv->type != F_SPTR);
		if (transmit(rmt, Socstr(*argv)) == -1) {
			rf->Dval = -1;
			break;
		}
		argv++;
	}
	Set_err_neg("Transmission failed");
}

PDECL(pr_input_waiting)
{
	if (T == F_RMT)
		Dset_int(*rf, Un->Rinbuf || input_waiting(Un->Rfd));
	else if (T == F_FILE)
		Dset_int(*rf, input_waiting(fileno(Un->Ffp)));
	else if (T == F_INT && !Int)
		Dset_int(*rf, keyboard_input_waiting || input_waiting(0));
	else
		Terror;
}

PDECL(pr_rmt_addr)
{
	Tcheck1(F_RMT);
	Dset_sptr(*rf, istr_s(Un->Raddr), 0);
}

PDECL(pr_rmt_port)
{
	Tcheck1(F_RMT);
	Dset_int(*rf, Un->Rport);
}

PDECL(pr_rmt_win)
{
	Tcheck1(F_RMT);
	if (Un->Rwin)
		Dset_elem(*rf, F_WIN, Dunode, Un->Rwin);
}

PDECL(pr_rmt_netread)
{
	Tcheck1(F_RMT);
	if (Un->Rnetread)
		Dset_elem(*rf, F_FPTR, Dfunc, Un->Rnetread);
}

PDECL(pr_rmt_promptread)
{
	Tcheck1(F_RMT);
	if (Un->Rpromptread)
		Dset_elem(*rf, F_FPTR, Dfunc, Un->Rpromptread);
}

PDECL(pr_rmt_back)
{
	Tcheck1(F_RMT);
	Dset_int(*rf, Un->Rback);
}

PDECL(pr_rmt_busy)
{
	Tcheck1(F_RMT);
	Dset_int(*rf, Un->Rbusy);
}

PDECL(pr_rmt_raw)
{
	Tcheck1(F_RMT);
	Dset_int(*rf, Un->Rraw);
}

PDECL(pr_rmt_echo)
{
	Tcheck1(F_RMT);
	Dset_int(*rf, Un->Recho);
}

PDECL(pr_rmt_eor)
{
	Tcheck1(F_RMT);
	Dset_int(*rf, Un->Reor);
}

/* Key buffer primitives */

PDECL(pr_insert)
{
	Tcheck1(F_SPTR);
	if (Sinbounds(Dp))
		process_incoming(Sastr(Dp));
}

PDECL(pr_edfunc)
{
	Tcheck1(F_INT);
	do_edit_func(Int);
}

PDECL(pr_getch)
{
	Estate *image, **ioq;

	Tcheckgen(!argc || T1 != F_INT || argc == 2 && T2 != F_WIN
		  || argc > 2);
	if (Int1 == 2 && argc == 1) {
		Dset_int(*rf, getch());
		return;
	}
	Tcheckgen(Int1 && Int1 != 1);
	image = suspend(argc);
	ioq = (argc == 1) ? (Int1 ? &gen_low : &gen_high)
			  : (Int1 ? &Un2->Wglstack : &Un2->Wghstack);
	add_ioqueue(ioq, image);
	*rf = suspend_frame;
}

/* Key primitives */

PDECL(pr_bind)
{
	Tcheckgen(T1 != F_SPTR || T2 != F_INT && T2 != F_FPTR);
	if (!Sinbounds(Dp1))
		return;
	rf->type = F_KEY;
	if (T2 == F_INT)
		rf->Dunode = add_key_efunc(Sastr(Dp1), Int2);
	else
		rf->Dunode = add_key_cmd(Sastr(Dp1), Dp2.Dfunc);
}

PDECL(pr_unbind)
{
	Tcheck1(F_KEY);
	del_key(Un);
}

PDECL(pr_find_key)
{
	Unode *key;

	Tcheck1(F_SPTR);
	key = find_key(Sastr(Dp));
	if (key)
		Dset_elem(*rf, F_KEY, Dunode, key);
}

PDECL(pr_key_seq)
{
	Tcheck1(F_KEY);
	Dset_sptr(*rf, istr_s(Un->Kseq), 0);
}

PDECL(pr_key_func)
{
	Tcheck1(F_KEY);
	rf->type = (Un->Ktype == K_EFUNC) ? F_INT : F_FPTR;
	if (Un->Ktype == K_EFUNC)
		Dset_int(*rf, Un->Kefunc);
	else
		Dset_elem(*rf, F_FPTR, Dfunc, Un->Kcmd);
}


