/* This source code is in the public domain. */

/* prmt.h: Some #defines used in the operator and primitive handlers */

#define PDECL(name) void name(rf, argc, argv) Dframe *rf, *argv; int argc;
#define Dp1 (argv[0])
#define Dp2 (argv[1])
#define Dp3 (argv[2])
#define Dp4 (argv[3])
#define Dp Dp1
#define T1 (Dp1.type)
#define T2 (Dp2.type)
#define T3 (Dp3.type)
#define T4 (Dp4.type)
#define T T1
#define Int1 (Dp1.Dval)
#define Int2 (Dp2.Dval)
#define Int3 (Dp3.Dval)
#define Int4 (Dp4.Dval)
#define Int Int1
#define Un1 (Dp1.Dunode)
#define Un2 (Dp2.Dunode)
#define Un3 (Dp3.Dunode)
#define Un4 (Dp4.Dunode)
#define Un Un1
#define Prmterror(x) if (1) { outputf x; finish_error(); \
			      *rf = frame_error; return; } else
#define Terror if (1) { type_error(rf); return; } else
#define Tcheckgen(e) if (e) Terror
#define Tcheck1(t) Tcheckgen(T1 != (t))
#define Tcheck2(t1, t2) Tcheckgen(T1 != (t1) || T2 != (t2))
#define Tcheck3(t1, t2, t3) Tcheckgen(T1 != (t1) || T2 != (t2) || T3 != (t3))
#define Berror if (1) { bounds_error(rf); return; } else
#define Bcheck(e) if (e) Berror
#define Set_err(v, s) if (1) { vtc_errflag = v; vtc_errmsg = s; } else
#define Set_err_neg(s) Set_err(rf->Dval == -1, s)
#define Set_err_null(s) Set_err(rf->type == F_NULL, s)
#define Dffalse(df) ((df).type == F_INT && !(df).Dval || (df).type == F_NULL)
#define Dftrue(df) (!Dffalse(df))
#define Ainbounds(df) ((df).Dapos >= 0 && (df).Dapos < (df).Asize)
#define Awriteok(df) (Ainbounds(df) || !(df).Darray->fixed && (df).Dapos >= 0)
#define Sinbounds(df) ((df).Dspos >= 0 && (df).Dspos < (df).Slen)
#define Soelem(df) (Sinbounds(df) ? (df).Sbegin[(df).Dspos] : 0)
#define Socstr(df) (Sinbounds(df) ? Scstr(df) : empty_cstr)
#define Soastr(df) (Sinbounds(df) ? Sastr(df) : "")
#define Salen(df) ((df).Slen - (df).u.s.pos)
#define Solen(df) (Sinbounds(df) ? Salen(df) : 0)


