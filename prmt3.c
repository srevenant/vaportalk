/* This source code is in the public domain. */

/* prmt3.c: Primitive functions: strings */

#include "vt.h"
#include "prmt.h"

#define Tcheck23(a, b) Tcheckgen(argc < 2 || T1 != (a) || T2 != (b) || \
				 argc == 3 && T3 != F_INT || argc > 3)

extern Istr wrapbuf;
extern char *strrchr();

/* String primitives */

PDECL(pr_strcpy)
{
	String *s1;
	int len;

	Tcheck23(F_SPTR, F_SPTR);
	if (Dp1.Dspos < 0)
		Berror;
	isolate(Dp1.Distr);
	s1 = &Dp1.Sstr;
	len = (argc == 2 || Int3 > Salen(Dp2)) ? Salen(Dp2) : Int3;
	if (Dp1.Dspos >= s1->c.l) {
		lcheck(s1, Dp1.Dspos);
		memset(s1->c.s + s1->c.l, ' ', Dp1.Dspos - s1->c.l);
		s1->c.s[s1->c.l = Dp1.Dspos] = '\0';
	}
	*rf = Dp1;
	if (!Sinbounds(Dp2) || argc == 3 && Int3 < 0)
		s_term(s1, Dp1.Dspos);
	else if (Dp1.Distr != Dp2.Distr) {
		s_term(s1, Dp1.Dspos);
		s_cat(s1, cstr_sl(Sastr(Dp2), len));
	} else if (Dp1.Dspos < Dp2.Dspos) {
		memmove(Sastr(Dp1), Sastr(Dp2), len);
		s1->c.s[s1->c.l = Dp1.Dspos + len] = '\0';
	} else if (Dp1.Dspos > Dp2.Dspos) {
		lcheck(s1, Dp1.Dspos + len);
		memmove(Sastr(Dp1), Sastr(Dp2), len);
		s1->c.s[s1->c.l = Dp1.Dspos + len] = '\0';
	}
}

PDECL(pr_strcat)
{
	String *s1;
	int len;

	Tcheck23(F_SPTR, F_SPTR);
	Bcheck(Dp1.Dspos < 0);
	isolate(Dp1.Distr);
	s1 = &Dp1.Sstr;
	len = argc == 2 || Int3 > Solen(Dp2) ? Solen(Dp2) :
	      Int3 <= 0 ? 0 : Int3;
	if (Dp1.Dspos >= s1->c.l) {
		lcheck(s1, Dp1.Dspos);
		memset(s1->c.s + s1->c.l, ' ', Dp1.Dspos - s1->c.l);
		s1->c.l = Dp1.Dspos;
	}
	s_cat(s1, cstr_sl(Soastr(Dp2), len));
	*rf = Dp1;
}

PDECL(pr_strdup)
{
	Tcheck1(F_SPTR);
	Dset_sptr(*rf, istr_rs(Dp.Distr->rs), Dp.Dspos);
}

PDECL(pr_strcmp)
{
	Tcheck23(F_SPTR, F_SPTR);
	Dset_int(*rf, (argc == 2) ? strcmp(Soastr(Dp1), Soastr(Dp2))
		      : strncmp(Soastr(Dp1), Soastr(Dp2), max(Int3, 0)));
}

PDECL(pr_stricmp)
{
	Tcheck23(F_SPTR, F_SPTR);
	Dset_int(*rf, (argc == 2) ? strcasecmp(Soastr(Dp1), Soastr(Dp2))
		      : strncasecmp(Soastr(Dp1), Soastr(Dp2), max(Int3, 0)));
}

PDECL(pr_strchr)
{
	char *ptr;

	Tcheck23(F_SPTR, F_INT);
	if (!Sinbounds(Dp1) && Int2 == '\0') {
		*rf = Dp1;
		return;
	}
	if (argc == 3) {
		Int3 = min(max(Int3, 0), Dp1.Slen);
		ptr = vtstrnchr(Soastr(Dp1), Int2, Int3);
	} else
		ptr = strchr(Soastr(Dp1), Int2);
	if (ptr)
		Dset_sptr(*rf, Dp1.Distr, ptr - Dp1.Sbegin);
}

PDECL(pr_strrchr)
{
	char *ptr;

	Tcheck23(F_SPTR, F_INT);
	if (!Sinbounds(Dp1) && Int2 == '\0') {
		*rf = Dp1;
		return;
	}
	if (argc == 3) {
		Int3 = min(max(Int3, 0), Dp1.Slen);
		ptr = vtstrnrchr(Soastr(Dp1), Int2, Int3);
	} else
		ptr = strrchr(Soastr(Dp1), Int2);
	if (ptr)
		Dset_sptr(*rf, Dp1.Distr, ptr - Dp1.Sbegin);
}

PDECL(pr_strcspn)
{
	Tcheck2(F_SPTR, F_SPTR);
	Dset_int(*rf, strcspn(Soastr(Dp1), Soastr(Dp2)));
}

PDECL(pr_strstr)
{
	char *ptr;

	Tcheck2(F_SPTR, F_SPTR);
	if (!Sinbounds(Dp1) && !Sinbounds(Dp2)) {
		*rf = Dp1;
		return;
	}
	ptr = strstr(Soastr(Dp1), Soastr(Dp2));
	if (ptr)
		Dset_sptr(*rf, Dp1.Distr, ptr - Dp1.Sbegin);
}

PDECL(pr_stristr)
{
	char *ptr;

	Tcheck2(F_SPTR, F_SPTR);
	if (!Sinbounds(Dp1) && !Sinbounds(Dp2)) {
		*rf = Dp1;
		return;
	}
	ptr = vtstristr(Soastr(Dp1), Socstr(Dp2));
	if (ptr)
		Dset_sptr(*rf, Dp1.Distr, ptr - Dp1.Sbegin);
}

PDECL(pr_strupr)
{
	char *s;

	Tcheck1(F_SPTR);
	if (Sinbounds(Dp)) {
		isolate(Dp.Distr);
		for (s = Dp.Sbegin; *s; s++)
			*s = ucase(*s);
	}
	*rf = Dp;
}

PDECL(pr_strlwr)
{
	char *s;

	Tcheck1(F_SPTR);
	if (Sinbounds(Dp)) {
		isolate(Dp.Distr);
		for (s = Dp.Sbegin; *s; s++)
			*s = lcase(*s);
	}
	*rf = Dp;
}

PDECL(pr_strlen)
{
	Tcheck1(F_SPTR);
	Dset_int(*rf, Solen(Dp));
}

PDECL(pr_wrap)
{
	char *s, *ptr;
	int l, margin, indent, i;

	Tcheckgen(argc < 2 || T1 != F_SPTR || T2 != F_INT || argc >= 3
		  && T3 != F_INT || argc == 4 && T4 != F_INT || argc > 4);
	s = Soastr(Dp1);
	l = Solen(Dp1);
	margin = argc == 4 ? Int2 - Int4 : Int2;
	indent = argc >= 3 ? Int3 : 0;
	isolate(&wrapbuf);
	s_term(&wrapbuf.rs->str, 0);
	Dset_sptr(*rf, istr_rs(wrapbuf.rs), 0);
	if (margin < 0 || Int2 < indent || !l) {
		    s_add(&wrapbuf.rs->str, '\n');
		    return;
	}
	while (l > margin) {
		ptr = vtstrnrchr(s, ' ', margin);
		i = (ptr && ptr - s > margin / 2) ? ptr - s : margin;
		s_cat(&wrapbuf.rs->str, cstr_sl(s, i));
		s_fadd(&wrapbuf.rs->str, '\n');
		s += i;
		l -= i;
		for (; *s == ' '; s++, l--);
		if (*s) {
			for (i = 0; i < indent; i++)
				s_fadd(&wrapbuf.rs->str, ' ');
		}
		margin = Int2 - indent;
	}
	if (l > 0) {
		s_cat(&wrapbuf.rs->str, cstr_sl(s, l));
		s_fadd(&wrapbuf.rs->str, '\n');
	}
	s_nt(&wrapbuf.rs->str);
}

PDECL(pr_ucase)
{
	Tcheck1(F_INT);
	Dset_int(*rf, ucase(Int));
}

PDECL(pr_lcase)
{
	Tcheck1(F_INT);
	Dset_int(*rf, lcase(Int));
}

PDECL(pr_itoa)
{
	Tcheck1(F_INT);
	Dset_sptr(*rf, istr_s(itoa(Int)), 0);
}

PDECL(pr_atoi)
{
	Tcheck1(F_SPTR);
	Dset_int(*rf, atoi(Soastr(Dp)));
}


