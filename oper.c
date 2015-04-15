/* This source code is in the public domain. */

/* oper.c: Operator functions */

#include "vt.h"
#include "prmt.h"

#ifdef PROTOTYPES
static int df_eq(Dframe *, Dframe *);
static int df_lt(Dframe *, Dframe *);
static int df_gt(Dframe *, Dframe *);
static void add_data_int(Dframe *, Dframe *, long);
static int incr_data(Dframe *, Dframe *);
static int decr_data(Dframe *, Dframe *);
static void postmod(Dframe *, int, Dframe *, int (*)(), int);
static void premod(Dframe *, int, Dframe *, int (*)(), int);
static void selem_asn(String *, int, int);
#else
static void selem_asn(), postmod(), premod();
#endif

extern Dframe frame_error;

/* Some specialized data routines used by the operators */

static int df_eq(df1, df2)
	Dframe *df1, *df2;
{
	if (df1->type != df2->type)
		return 0;
	switch (df1->type) {
	    case F_INT:
		return df1->Dval == df2->Dval;
	    case F_BPTR:
		return df1->Dbobj == df2->Dbobj;
	    case F_PPTR:
		return df1->Dpnum == df2->Dpnum;
	    case F_RMT:
	    case F_WIN:
	    case F_KEY:
	    case F_FILE:
		return df1->Dunode == df2->Dunode;
	    case F_SPTR:
		return df1->Distr == df2->Distr && df1->Dspos == df2->Dspos;
	    case F_APTR:
		return df1->Darray == df2->Darray && df1->Dapos == df2->Dapos;
	    case F_FPTR:
		return df1->Dfunc == df2->Dfunc;
	    case F_REG:
		return df1->Dreg == df2->Dreg;
	    case F_ASSOC:
		return df1->Dassoc == df2->Dassoc;
	    case F_PLIST:
		return df1->Dplist == df2->Dplist;
	    default:
		return 1;
	}
}

static int df_lt(df1, df2)
	Dframe *df1, *df2;
{
	if (df1->type != df2->type)
		return -1;
	switch (df1->type) {
	    case F_INT:
		return df1->Dval < df2->Dval;
	    case F_APTR:
		return df1->Darray == df2->Darray ?
		       df1->Dapos < df2->Dapos : -1;
	    case F_SPTR:
		return df1->Distr == df2->Distr ?
		       df1->Dspos < df2->Dspos : -1;
	    default:
		return -1;
	}
}

static int df_gt(df1, df2)
	Dframe *df1, *df2;
{
	if (df1->type != df2->type)
		return -1;
	switch (df1->type) {
	    case F_INT:
		return df1->Dval > df2->Dval;
	    case F_APTR:
		return (df1->Darray == df2->Darray) ?
		 df1->Dapos > df2->Dapos : -1;
	    case F_SPTR:
		return (df1->Distr == df2->Distr) ?
		 df1->Dspos > df2->Dspos : -1;
	    default:
		return -1;
	}
}

static void add_data_int(rf, df, num)
	Dframe *rf, *df;
	long num;
{
	rf->type = df->type;
	switch (df->type) {
	    case F_INT:
		rf->Dval = df->Dval + num;
	    Case F_SPTR:
		rf->Distr = df->Distr;
		rf->Dspos = df->Dspos + num;
	    Case F_APTR:
		rf->Darray = df->Darray;
		rf->Dapos = df->Dapos + num;
	    Default:
		*rf = frame_error;
	}
}

static int incr_data(rf, val)
	Dframe *rf, *val;
{
	switch (val->type) {
		case F_INT:	val->Dval++;		return 0;
		case F_APTR:	val->Dapos++;		return 0;
		case F_SPTR:	val->Dspos++;		return 0;
		default:	type_error(rf);		return -1;
	}
}

static int decr_data(rf, val)
	Dframe *rf, *val;
{
	switch (val->type) {
		case F_INT:	val->Dval--;		return 0;
		case F_APTR:	val->Dapos--;		return 0;
		case F_SPTR:	val->Dspos--;		return 0;
		default:	type_error(rf);		return -1;
	}
}

static void postmod(rf, argc, argv, func, addend)
	Dframe *rf, *argv;
	int argc, (*func)(), addend;
{
	Dframe df;

	if (T1 == F_APTR && Ainbounds(Dp1)) {
		*rf = Aelem(Dp1);
		(*func)(rf, &Aelem(Dp1));
	} else if (T1 == F_SPTR) {
		Bcheck(Dp1.Dspos < 0);
		isolate(Dp1.Distr);
		Dset_int(*rf, Soelem(Dp1));
		selem_asn(&Dp1.Sstr, Dp1.Dspos, Soelem(Dp1) + addend);
	} else if (T1 == F_BPTR) {
		(*Dp1.Dbobj)(&df);
		*rf = df;
		if ((*func)(rf, &df) != -1)
			assign_bobj(rf, Dp1.Dbobj, &df);
	} else
		Terror;
}

static void premod(rf, argc, argv, func, addend)
	Dframe *rf, *argv;
	int argc, (*func)(), addend;
{
	if (T1 == F_APTR && Ainbounds(Dp1)) {
		if ((*func)(rf, &Aelem(Dp1)) != -1)
			*rf = Aelem(Dp1);
	} else if (T1 == F_SPTR) {
		Bcheck(Dp1.Dspos < 0);
		isolate(Dp1.Distr);
		selem_asn(&Dp1.Sstr, Dp1.Dspos, Soelem(Dp1) + addend);
		Dset_int(*rf, Soelem(Dp1));
	} else if (T1 == F_BPTR) {
		(*Dp1.Dbobj)(rf);
		if ((*func)(rf, rf) != -1)
			assign_bobj(rf, Dp1.Dbobj, rf);
	} else
		Terror;
}

static void selem_asn(str, pos, val)
	String *str;
	int pos, val;
{
	if (pos >= str->c.l) {
		lcheck(str, pos + 1);
		memset(str->c.s + str->c.l, ' ', pos - str->c.l);
		str->c.s[str->c.l = pos + 1] = '\0';
	}
	if (!val)
		s_term(str, pos);
	else
		str->c.s[pos] = val;
}

/* The operator functions themselves */

PDECL(op_bor)
{
	Tcheck2(F_INT, F_INT);
	Dset_int(*rf, Int1 | Int2);
}

PDECL(op_bxor)
{
	Tcheck2(F_INT, F_INT);
	Dset_int(*rf, Int1 ^ Int2);
}

PDECL(op_band)
{
	Tcheck2(F_INT, F_INT);
	Dset_int(*rf, Int1 & Int2);
}

PDECL(op_eq)
{
	Dset_int(*rf, df_eq(&Dp1, &Dp2));
}

PDECL(op_ne)
{
	Dset_int(*rf, !df_eq(&Dp1, &Dp2));
}

PDECL(op_lt)
{
	int val;

	val = df_lt(&Dp1, &Dp2);
	Tcheckgen(val == -1);
	Dset_int(*rf, val);
}

PDECL(op_le)
{
	int val;

	val = df_gt(&Dp1, &Dp2);
	Tcheckgen(val == -1);
	Dset_int(*rf, !val);
}

PDECL(op_gt)
{
	int val;

	val = df_gt(&Dp1, &Dp2);
	Tcheckgen(val == -1);
	Dset_int(*rf, val);
}

PDECL(op_ge)
{
	int val;

	val = df_lt(&Dp1, &Dp2);
	Tcheckgen(val == -1);
	Dset_int(*rf, !val);
}

PDECL(op_sl)
{
	Tcheck2(F_INT, F_INT);
	Dset_int(*rf, Int1 << Int2);
}

PDECL(op_sr)
{
	Tcheck2(F_INT, F_INT);
	Dset_int(*rf, Int1 >> Int2);
}

PDECL(op_add)
{
	if (T2 == F_INT)
		add_data_int(rf, &Dp1, Int2);
	else if (T1 == F_INT)
		add_data_int(rf, &Dp2, Int1);
	else if (T1 == F_SPTR && T2 == F_SPTR) {
		Dset_sptr(*rf, (Dp1.Srefs == 1) ?
			       Dp1.Distr : istr_rs(Dp1.Srstr), Dp1.Dspos);
		isolate(rf->Distr);
		if (!Sinbounds(*rf))
			rf->Dspos = rf->Slen;
		s_cat(&rf->Sstr, Socstr(Dp2));
	} else
		*rf = frame_error;
	if (rf->type == F_EXCEPT)
		type_errmsg();
}

PDECL(op_sub)
{
	if (T2 == F_INT)
		add_data_int(rf, &Dp1, -Int2);
	else if (T1 == F_APTR && T2 == F_APTR && Dp1.Darray == Dp2.Darray)
		Dset_int(*rf, Dp1.Dapos - Dp2.Dapos);
	else if (T1 == F_SPTR && T2 == F_SPTR && Dp1.Distr == Dp2.Distr)
		Dset_int(*rf, Dp1.Dspos - Dp2.Dspos);
	else
		*rf = frame_error;
	if (rf->type == F_EXCEPT)
		type_errmsg();
}

PDECL(op_mult)
{
	Tcheck2(F_INT, F_INT);
	Dset_int(*rf, Int1 * Int2);
}

PDECL(op_div)
{
	Tcheck2(F_INT, F_INT);
	if (!Int2)
		Prmterror(("Divide-by-zero error"));
	Dset_int(*rf, Int1 / Int2);
}

PDECL(op_mod)
{
	Tcheck2(F_INT, F_INT);
	if (!Int2)
		Prmterror(("Modulo-by-zero error"));
	Dset_int(*rf, Int1 % Int2);
}

PDECL(op_postinc) { postmod(rf, argc, argv, incr_data, 1); }
PDECL(op_postdec) { postmod(rf, argc, argv, decr_data, -1); }
PDECL(op_preinc) { premod(rf, argc, argv, incr_data, 1); }
PDECL(op_predec) { premod(rf, argc, argv, decr_data, -1); }

PDECL(op_not)
{
	Dset_int(*rf, Dffalse(Dp1));
}

PDECL(op_compl)
{
	Tcheck1(F_INT);
	Dset_int(*rf, ~Int1);
}

PDECL(op_neg)
{
	Tcheck1(F_INT);
	Dset_int(*rf, -Int1);
}

PDECL(op_asn)
{
	*rf = Dp2;
	if (T1 == F_APTR) {
		Bcheck(!Awriteok(Dp1));
		extend_array(Dp1.Darray, Dp1.Dapos + 1);
		deref_frame(&Aelem(Dp1));
		Aelem(Dp1) = Dp2;
		ref_frame(&Aelem(Dp1));
	} else if (T1 == F_SPTR && T2 == F_INT) {
		Bcheck(Dp1.Dspos < 0);
		isolate(Dp1.Distr);
		selem_asn(&Dp1.Sstr, Dp1.Dspos, Int2);
	} else if (T1 == F_BPTR)
		assign_bobj(rf, Dp1.Dbobj, &Dp2);
	else
		Terror;
}


