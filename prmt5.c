/* This source code is in the public domain. */

/* prmt5.c: Primitive functions: regexps, environment */

#include "vt.h"
#include "prmt.h"
#include <fcntl.h>

extern Unode file_ring;
extern Istr freader;

/* File primitives */

PDECL(pr_fopen)
{
	FILE *fp;
	Unode *un;

	Tcheck2(F_SPTR, F_SPTR);
	fp = fopen(expand(Soastr(Dp1)), Soastr(Dp2));
	if (fp) {
		fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
		un = unalloc();
		un->Fname = vtstrdup(Soastr(Dp1));
		un->Ftype = FT_FILE;
		un->Ffp = fp;
		un->prev = file_ring.prev;
		un->next = &file_ring;
		file_ring.prev = file_ring.prev->next = un;
		Dset_elem(*rf, F_FILE, Dunode, un);
	}
	Set_err_null(NULL);
}

PDECL(pr_popen)
{
	FILE *fp;
	Unode *un;

	Tcheck2(F_SPTR, F_SPTR);
	fp = popen(Soastr(Dp1), Soastr(Dp2));
	if (fp) {
		fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
		un = unalloc();
		un->Fname = vtstrdup(Soastr(Dp1));
		un->Ftype = FT_PROC;
		un->Ffp = fp;
		un->prev = file_ring.prev;
		un->next = &file_ring;
		file_ring.prev = file_ring.prev->next = un;
		Dset_elem(*rf, F_FILE, Dunode, un);
	}
	Set_err_null(NULL);
}

PDECL(pr_fclose)
{
	Tcheck1(F_FILE);
	Discardstring(Un->Fname);
	Dset_int(*rf, Un->Ftype == FT_PROC ? pclose(Un->Ffp)
					   : fclose(Un->Ffp));
	Set_err_neg(NULL);
	Un->prev->next = Un->next;
	Un->next->prev = Un->prev;
	destroy_pointers(Un->frefs);
	discard_unode(Un);
}

PDECL(pr_fwrite)
{
	Tcheck2(F_FILE, F_SPTR);
	Dset_int(*rf, Sinbounds(Dp2) ? fwrite(Sastr(Dp2), 1, Dp2.Slen
					      - Dp2.Dspos, Un1->Ffp) : 0);
	Set_err_neg(NULL);
}

PDECL(pr_fread)
{
	Tcheck1(F_FILE);
	isolate(&freader);
	if (s_fget(&freader.rs->str, Un->Ffp))
		Dset_sptr(*rf, istr_rs(freader.rs), 0);
	Set_err_null(NULL);
}

PDECL(pr_fseek)
{
	Tcheck3(F_FILE, F_INT, F_INT);
	Dset_int(*rf, fseek(Un1->Ffp, Int2, Int3));
	Set_err_neg(NULL);
}

PDECL(pr_ftell)
{
	Tcheck1(F_FILE);
	Dset_int(*rf, ftell(Un->Ffp));
	Set_err_neg(NULL);
}

PDECL(pr_fputc)
{
	Tcheck2(F_INT, F_FILE);
	Dset_int(*rf, putc(Int1, Un2->Ffp));
	Set_err_neg(NULL);
}

PDECL(pr_fgetc)
{
	Tcheck1(F_FILE);
	Dset_int(*rf, getc(Un->Ffp));
	Set_err_neg(NULL);
}

PDECL(pr_fflush)
{
	Tcheck1(F_FILE);
	fflush(Un->Ffp);
}

PDECL(pr_feof)
{
	Tcheck1(F_FILE);
	Dset_int(*rf, feof(Un->Ffp));
}

PDECL(pr_fsize)
{
	struct stat buf;

	Tcheck1(F_SPTR);
	Dset_int(*rf,
		 (stat(expand(Soastr(Dp)), &buf) == -1) ? -1 : buf.st_size);
	Set_err_neg(NULL);
}

PDECL(pr_fmtime)
{
	struct stat buf;

	Tcheck1(F_SPTR);
	Dset_int(*rf,
		 (stat(expand(Soastr(Dp)), &buf) == -1) ? -1 : buf.st_mtime);
	Set_err_neg(NULL);
}

PDECL(pr_unlink)
{
	Tcheck1(F_SPTR);
	Dset_int(*rf, unlink(expand(Soastr(Dp))));
	Set_err_neg(NULL);
}

PDECL(pr_load_file)
{
	Tcheck1(F_SPTR);
	Dset_int(*rf, load_file(expand(Soastr(Dp))));
	Set_err_neg(NULL);
}

PDECL(pr_find_file)
{
	Unode *f;

	Tcheck1(F_SPTR);
	for (f = file_ring.next; !f->dummy; f = f->next) {
		if (streq(f->Fname, Soastr(Dp)))
			break;
	}
	if (f->dummy)
		return;
	Dset_elem(*rf, F_FILE, Dunode, f);
}

PDECL(pr_file_name)
{
	Tcheck1(F_FILE);
	Dset_sptr(*rf, istr_s(Un->Fname), 0);
}

/* Regexp primitives */

PDECL(pr_regcomp)
{
	regexp *reg;

	Tcheck1(F_SPTR);
	reg = regcomp(Soastr(Dp));
	if (reg) {
		reg->refs = 0;
		reg->rs = NULL;
		Dset_elem(*rf, F_REG, Dreg, reg);
	}
	vtc_errflag = (rf->type == F_NULL);
}

PDECL(pr_regexec)
{
	regexp *reg = Dp1.Dreg;

	Tcheck2(F_REG, F_SPTR);
	Dset_int(*rf, regexec(reg, Soastr(Dp2)));
	if (reg->rs)
		dec_ref_rstr(reg->rs);
	if (rf->Dval) {
		reg->rs = Dp2.Distr->rs;
		reg->rs->refs++;
	} else
		reg->rs = NULL;
}

PDECL(pr_regmatch)
{
	regexp *r = Dp1.Dreg;
	int n = Int2;

	Tcheck2(F_REG, F_INT);
	if (n < 0 || n >= NSUBEXP || !r->rs || !r->startp[n])
		return;
	Dset_sptr(*rf, istr_sl(r->startp[n], r->endp[n] - r->startp[n]), 0);
}

/* Environment primitives */

PDECL(pr_getenv)
{
	char *env;

	Tcheck1(F_SPTR);
	env = getenv(Soastr(Dp));
	if (env)
		Dset_sptr(*rf, istr_s(env), 0);
	Set_err_null(NULL);
}

PDECL(pr_system)
{
	Tcheck1(F_SPTR);
	tty_mode(0);
	Dset_int(*rf, system(Soastr(Dp)));
	tty_mode(1);
}

PDECL(pr_ctime)
{
	Tcheck1(F_INT);
	Dset_sptr(*rf, istr_s(ctime(&Int)), 0);
}

PDECL(pr_smatch)
{
	Tcheck2(F_SPTR, F_SPTR);
	Dset_int(*rf, smatch(Soastr(Dp1), Soastr(Dp2)));
}


