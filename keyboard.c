/* This source code is in the public domain. */

/* keyboard.c: Handle keyboard input */

#include "vt.h"

#ifdef PROTOTYPES
static void flush_out(void);
static int match_key(char *, char *);
static int check_keys(char *, Unode **);
static int fword(void);
static int bword(void);
static void process_kbd_line(Cstr);
#else
static void flush_out(), process_kbd_line();
#endif

#define margin (cols - 1)
#define klen (kbuf.c.l)
#define kptr (kbuf.c.s)

/* Key matching results */
#define KM_NONE	    0
#define KM_PARTIAL  1
#define KM_TOTAL    2

/* Indexes to functions table */
#define K_CUP	   0
#define K_CDOWN	   1
#define K_CLEFT	   2
#define K_CRIGHT   3
#define K_CHOME	   4
#define K_CEND	   5
#define K_CWLEFT   6
#define K_CWRIGHT  7
#define K_BSPC	   8
#define K_BWORD	   9
#define K_BHOME	  10
#define K_DBUF	  11
#define K_DCH	  12
#define K_DWORD	  13
#define K_DEND	  14
#define K_REFRESH 15
#define K_REDRAW  16
#define K_MODE	  17
#define K_PROCESS 18

static String kout = { { "", 0 }, 0 };	/* Text to send to input_puts() */
static String kkey = { { "", 0 }, 0 };	/* Text to match against keys	*/
static String kproc = { { "", 0 }, 0 }; /* Temp buffer for processing	*/
String kbuf = { { "", 0 }, 0 };		/* Main key buffer		*/
int kpos = 0;
int keyboard_input_waiting = 0;
extern int cols, vtc_mode;
extern Estate *gen_read, *gen_high, *gen_low;
extern Unode *klookup[], *active_win;

/* Feed the kout buffer to input_inputs() */
static void flush_out()
{
	if (!kout.c.l)
		return;
	s_nt(&kout);
	curs_input();
	input_puts(kout.c);
	s_insert(&kbuf, kout.c, kpos);
	kpos += kout.c.l;
	s_term(&kout, 0);
}

/* Compare a key sequence with a string */
static int match_key(text, seq)
	char *text, *seq;
{
	do
		text++, seq++;
	while (*text && lcase(*text) == lcase(*seq));
	return *text ? KM_NONE : *seq ? KM_PARTIAL : KM_TOTAL;
}

/* Check the string s against all keys in clump */
static int check_keys(text, rkey)
	char *text;
	Unode **rkey;
{
	Unode *kp;
	int match;
	int found_partial = 0;

	kp = klookup[lcase(*text)];
	if (!kp)
		return KM_NONE;
	for (; lcase(*kp->Kseq) == lcase(*text); kp = kp->next) {
		match = match_key(text, kp->Kseq);
		if (match == KM_TOTAL) {
			*rkey = kp;
			return KM_TOTAL;
		} else if (match == KM_PARTIAL)
			found_partial = 1;
	}
	return found_partial ? KM_PARTIAL : KM_NONE;
}

/* Return the position of the beginning of the next word */
static int fword()
{
	int pos = kpos;

	for (; pos < klen && kptr[pos] != ' '; pos++);
	for (; pos < klen && kptr[pos] == ' '; pos++);
	return pos;
}

/* Return the position of the beginning of the previous word */
static int bword()
{
	int pos = kpos - 1;

	for (; pos >= 0 && kptr[pos] == ' '; pos--);
	for (; pos >= 0 && kptr[pos] != ' '; pos--);
	return pos + 1;
}

/* Perform an editing function */
void do_edit_func(func)
	int func;
{
	int pos;

	curs_input();
	switch (func) {
	    case K_BSPC:
		if (kpos) {
			input_bdel(1);
			s_delete(&kbuf, --kpos, 1);
		}
	    Case K_BWORD:
		pos = bword();
		input_bdel(kpos - pos);
		s_delete(&kbuf, pos, kpos - pos);
		kpos = pos;
	    Case K_BHOME:
		input_bdel(kpos);
		s_delete(&kbuf, 0, kpos);
		kpos = 0;
	    Case K_DBUF:
		input_clear();
		s_term(&kbuf, kpos = 0);
	    Case K_DCH:
		if (kpos < klen) {
			input_fdel(1);
			s_delete(&kbuf, kpos, 1);
		}
	    Case K_DWORD:
		pos = fword();
		input_fdel(pos - kpos);
		s_delete(&kbuf, kpos, pos - kpos);
	    Case K_DEND:
		input_fdel(klen - kpos);
		s_term(&kbuf, kpos);
	    Case K_REFRESH:
		input_clear();
		input_draw();
	    Case K_REDRAW:
		redraw_screen();
	    Case K_MODE:
		toggle_imode();
	    Case K_PROCESS:
		input_newline();
		s_cpy(&kproc, kbuf.c);
		s_term(&kbuf, kpos = 0);
		process_kbd_line(kproc.c);
	    Default:
		switch (func) {
		    case K_CUP:		pos = kpos - margin;
		    Case K_CDOWN:	pos = kpos + margin;
		    Case K_CLEFT:	pos = kpos - 1;
		    Case K_CRIGHT:	pos = kpos + 1;
		    Case K_CHOME:	pos = 0;
		    Case K_CEND:	pos = klen;
		    Case K_CWLEFT:	pos = bword();
		    Case K_CWRIGHT:	pos = fword();
		    Default: return;	/* Putz */
		}
		pos = pos < 0 ? 0 : pos > klen ? klen : pos;
		input_cmove(pos);
		kpos = pos;
	}
}

void process_incoming(s)
	char *s;
{
	Unode *key;
	int val;
	char *temp;

	if (*s)
		keyboard_input_waiting++;
	for (; *s; s++) {
		if (!s[1])
			keyboard_input_waiting--;
		if (gen_high || active_win->Wghstack) {
			flush_out();
			resume_int(gen_high ? &gen_high
					    : &active_win->Wghstack, *s);
			continue;
		}
		s_add(&kkey, *s);
		val = check_keys(kkey.c.s, &key);
		if (val == KM_TOTAL) {
			flush_out();
			s_term(&kkey, 0);
			if (key->Ktype == K_EFUNC)
				do_edit_func(key->Kefunc);
			else
				run_prog(key->Kcmd->cmd);
			continue;
		}
		if (val == KM_PARTIAL)
			continue;
		if (gen_low || active_win->Wglstack) {
			flush_out();
			resume_int(gen_low ? &gen_low
					   : &active_win->Wglstack, *kkey.c.s);
		} else if (isprint(*kkey.c.s))
			s_fadd(&kout, *kkey.c.s);
		if (kkey.c.l > 1) {
			temp = vtstrdup(kkey.c.s + 1);
			s_term(&kkey, 0);
			process_incoming(temp);
			Discardstring(temp);
		} else
			s_term(&kkey, 0);
	}
	flush_out();
}

static void process_kbd_line(line)
	Cstr line;
{
	if (vtc_mode)
		parse(line.s);
	else
		give_window(active_win, istr_c(line));
}

void give_window(win, is)
	Unode *win;
	Istr *is;
{
	is->refs++;
	if (gen_read)
		resume_istr(&gen_read, is);
	else if (win->Wrstack)
		resume_istr(&win->Wrstack, is);
	else if (win->Wtermread)
		run_prog_istr(win->Wtermread->cmd, is, win, NULL);
	else if (win->Wrmt) {
		transmit(win->Wrmt, is->rs->str.c);
		if (win->Wrmt)
			transmit(win->Wrmt, cstr_s("\n"));
	}
	dec_ref_istr(is);
}

