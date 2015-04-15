/* This source code is in the public domain. */

/* vt.h: Header for all VT source files */

#define VERSION "Vaportalk 2.15.1"

/* Include files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "regexp.h"

#if defined(BSD43) || (defined(sun) && !defined(__SVR4__))
extern char *sys_errlist[];
#define strerror(e) sys_errlist[e]
#endif

#ifdef BSD43
#define getcwd(a, b) getwd(a)
#define FD_CLOEXEC 1
typedef int pid_t;
#endif

#ifdef SYSV
#define SYSVTTY
#endif

#if (defined(sun) && !defined(__svr4__)) || defined(BSD43)
#define memmove(a, b, c)	bcopy(b, a, c)
#endif

/* Any system-dependent optimization definitions */
#include "config.h"
#include "string.h"
#include "struct.h"
#include "extern.h"

#define Case			break; case
#define Default			break; default
#define min(x, y)		((x) <= (y) ? (x) : (y))
#define max(x, y)		((x) >= (y) ? (x) : (y))
#define streq(a, b)		(!strcmp(a, b))
#define s_acpy(x, y)		s_cpy(x, cstr_s(y))
#define s_ancpy(x, y, z)	s_ncpy(x, cstr_s(y), z)
#define s_acat(x, y)		s_cat(x, cstr_s(y))
#define istr_c(a)		istr_rs(rstr_c(a))
#define istr_s(a)		istr_c(cstr_s(a))
#define istr_sl(s, l)		istr_c(cstr_sl(s, l))
#define New(t)			(t *)(dmalloc(sizeof(t)))
#define Newarray(t, n)		(t *)(dmalloc(sizeof(t) * (n)))
#define Rearray(p, t, o, n)	(t *)(drealloc((char *) p, sizeof(t) * (o), \
							   sizeof(t) * (n)))
#define Resize(p, t, o, n)	(p) = Rearray(p, t, o, n)
#define Double(p, t, s)		if (1) { Resize(p, t, s, (s) * 2); s *= 2; } \
				else
#define Check(p, t, s, d)	if ((s) < (d)) Double(p, t, s); else
#define Checkinc(p, t, s, v)	if ((v) == (s)) Double(p, t, s); else
#define Push(p, t, s, v, x)	if (1) { Checkinc(p, t, s, v); \
					 (p)[(v)++] = (x); } else
#define Discard(x, t)		dfree((char *) x, sizeof(t))
#define Discardarray(x, t, n)	dfree((char *) x, sizeof(t) * (n))
#define Discardstring(x)	Discardarray(x, char, strlen(x) + 1)
#define Copy(s, d, n, t)	memmove((char*)(d), (char*)(s), \
					(n) * sizeof(t))
#define Cur_win (cur_win ? cur_win : cur_rmt ? cur_rmt->u.r.win : active_win)
#define Cur_rmt (cur_rmt ? cur_rmt : cur_win ? cur_win->u.w.rmt : \
		active_win->u.w.rmt)
#define coutput(x)		output(Cur_win, (x))

extern Unode *active_win, *cur_win, *cur_rmt;
extern int vtc_errflag;
extern char *vtc_errmsg;

/* string.c externals and related macros */
extern Cstr empty_cstr;
extern String wbufs[], empty_string;
#ifdef ANSI_CTYPES
#define lcase(x) (tolower((x) & 0x7f))
#define ucase(x) (toupper((x) & 0x7f))
#else
extern char lowercase_values[], uppercase_values[];
#define lcase(x) (lowercase_values[(x) & 0x7f])
#define ucase(x) (uppercase_values[(x) & 0x7f])
#endif
#define NUM_WBUFS 3

/* window.c externals and related macros */
extern char s_clrscr[], s_clreol[], s_bold_on[], s_bold_off[];
#define clrscr()	    vtwrite(cstr_s(s_clrscr))
#define clreol()	    vtwrite(cstr_s(s_clreol))
#define bold(s)		    vtwrite(cstr_s((s) ? s_bold_on : s_bold_off));

