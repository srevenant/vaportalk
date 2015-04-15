/* This source code is in the public domain. */

/* window.c - Handle I/O to terminal */

#include "vt.h"
#include <sys/ioctl.h>
#include <errno.h>

#ifdef USE_STDARG
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef hpux
#include <termio.h>
#else /* not HP-UX */
#ifdef SYSVTTY
#include <termio.h>
#else /* BSD */
#include <sgtty.h>
#endif
#endif

#ifdef PROTOTYPES
#ifdef TERMCAP
static void chrpt(int, int);
static void get_ospeed(void);
static void output_one(int);
static void getccap(char *, char *);
static void tp(char *, int);
#endif /* TERMCAP */
static void cap_formatted(char *, int, int);
static void clear_end(int);
static void clear_space(int, int);
static void move_left(int, int);
static void init_window(Unode *, int, int);
static void mark(Unode *, int);
static void draw_prompt(void);
static void clear_input_window(void);
#else /* not PROTOTYPES */
#ifdef TERMCAP
static void get_ospeed(), output_one(), getccap(), tp();
#endif /* TERMCAP */
static void chrpt(), cap_formatted(), clear_end(), clear_space(), move_left();
static void init_window(), mark(), draw_prompt(), clear_input_window();
#endif /* not PROTOTYPES */

/* Global variables and externs for the terminal routines */

/* The termcap library has no length-checking facilities.  We use 32
** bytes for a string that's not pre-compiled with tputs(), and 64
** bytes for one that is.  This is somewhat kludgy, but then, so are
** the termcap routines. */
static char s_cmove[32];		/* Move cursor		 */
static char s_scroll[32];		/* Set scroll area	 */
static char s_scr_rev[32];		/* Scroll region reverse */
char s_clrscr[64];			/* Clear screen		 */
char s_clreol[64];			/* Clear to end of line	 */
char s_bold_on[64];			/* Turn boldface on	 */
char s_bold_off[64];			/* Turn boldface off	 */

#ifdef HARDCODE
#define HC_CMOVE    "\033[%d;%dH"
#define HC_SCROLL   "\033[%d;%dr"
#define HC_SCR_REV  "\033M"
#define HC_CLRSCR   "\033[2J"
#define HC_CLREOL   "\033[K"
#define HC_BOLD_ON  "\033[1m"
#define HC_BOLD_OFF "\033[m"
#define cap_normal(x, y) Bwritea(x)
#endif
#define HC_ROWS 24
#define HC_COLS 80

int rows = 0, cols = 0;			/* Screen size	       */
static int scr_top = -1, scr_bot = -1;	/* Current scroll area */
#ifdef TERMCAP
static char *cptr;
extern char *tgetstr(), *tgoto();
#ifndef AIX
extern short ospeed;
#endif /* not AIX */
#define cap_normal(x, y) tp(x, y)
#endif /* TERMCAP */

/* Global variables for windowing routines */

#define CURS_INPUT	0
#define CURS_WINDOW	1
#define CURS_ELSEWHERE	2

Unode win_ring;				/* Dummy node in windows ring */
Unode *active_win;			/* Keyboard text is sent here */
Unode *cur_win = NULL;			/* Where text is being processed */
int curs_loc = CURS_ELSEWHERE;		/* General idea of where cursor is */
static Unode *curs_win;			/* More info for CURS_WINDOW pos */
extern Unode *cur_rmt;
static String outbuf = { { "", 0 }, 0 };/* Output buffering */
#define Die(s) if (1) { puts(s); exit(1); } else
#define Bwrite(cstr)	s_cat(&outbuf, cstr)
#define Bwritea(s)	s_acat(&outbuf, s)
#define Bwriteal(s, l)	s_cat(&outbuf, cstr_sl(s, l))
#define Bwritem(s, y, z) Bwriteal(s, min(y, z))
#define Bputch(c)	s_add(&outbuf, c)
#define Bflush		if (1) { vtwrite(outbuf.c); s_term(&outbuf, 0); } else
void bflushfunc() { Bflush; }

/* Global variables for input routines */

int icol;				/* Column of cursor on bottom line */
char *prompt;				/* Text of prompt */
int plen = 0;				/* Length of prompt */
int vtc_mode = 0;			/* Flag: input window in VTC mode */
extern String kbuf;
extern int kpos;
static int echo_mode = 1;
#define Divider (win_ring.prev->Wbot + 1)
#define Itop (Divider + 1)
#define Isize (rows - Itop)
#define Margin (cols - 1)
#define Round(x, y) ((x) - (x) % (y))
#define Klen (kbuf.c.l)
#define Kptr (kbuf.c.s)
#define Oplen (vtc_mode ? 0 : plen)
#define Iscr_rev if (1) { \
	if (Isize > 1) { scroll(Itop, rows - 1); scr_rev(1); } \
	else { cmove(0, rows - 1); Bwritea(s_clreol); } \
} else
#define Iscr_fwd if (1) { \
	if (Isize > 1) { scroll(Itop, rows - 1); scr_fwd(1); } \
	else { cmove(0, rows - 1); Bwritea(s_clreol); } \
} else

/* Low-level I/O */

void vtwrite(cstr)
	Cstr cstr;
{
	int written;

	while (cstr.l > 0) {
		written = write(1, cstr.s, cstr.l);
		if (written == -1) {
			if (errno != EWOULDBLOCK && errno != EINTR) {
				perror("write");
				vterror("Write failed in vtwrite()");
			}
			written = 0;
		}
		cstr = cstr_sl(cstr.s + written, cstr.l - written);
		if (cstr.l)
			sleep(1);
	}
}

static void chrpt(c, num)
	int c, num;
{
	while (num-- > 0)
		Bputch(c);
}

int getch()
{
	char c;
	int rs;

	while ((rs = read(0, &c, 1)) <= 0) {
		if (rs < 0 && errno != EINTR) {
			perror("read");
			vterror("Read failed in getch()");
		}
	}
	return c;
}

void tty_mode(state)
	int state;
{
#ifdef hpux
	struct termio blob;

	if (ioctl(0, TCGETA, &blob) == -1) {
		perror("TCGETA ioctl()");
		exit(1);
	}
	if (state) {
		blob.c_lflag &= ~ECHO;
		blob.c_lflag &= ~ECHOE;
		blob.c_lflag &= ~ICANON;
		blob.c_cc[VMIN] = 0;
		blob.c_cc[VTIME] = 0;
	} else {
		blob.c_lflag |= ECHO;
		blob.c_lflag |= ECHOE;
		blob.c_lflag |= ICANON;
	}
	ioctl(0, TCSETAF, &blob);
#else /* Not HP-UX */
#ifdef SYSVTTY
	struct termio blob;
	static struct termio old_tty_state;
	static int first = 1;

	if (first) {
		ioctl(0, TCGETA, &old_tty_state);
		first = 0;
	}
	if (state) {
		ioctl(0, TCGETA, &blob);
		blob.c_cc[VMIN] = 0;
		blob.c_cc[VTIME] = 0;
		blob.c_iflag = IGNBRK | IGNPAR | ICRNL;
		blob.c_oflag = OPOST | ONLCR;
		blob.c_lflag = ISIG;
		ioctl(0, TCSETA, &blob);
	} else if (!first)
		ioctl(0, TCSETA, &old_tty_state);
#else /* BSD */
	struct sgttyb blob;

	if (ioctl(0, TIOCGETP, &blob) == -1) {
		perror("TIOCGETP ioctl()");
		exit(1);
	}
	blob.sg_flags |= state ? CBREAK : ECHO;
	blob.sg_flags &= state ? ~ECHO : ~CBREAK;
	ioctl(0, TIOCSETP, &blob);
#endif
#endif
}

/* Terminal routines */

#ifdef TERMCAP
static void get_ospeed()
{
#ifndef hpux
#ifndef AIX
	struct sgttyb sgttyb;

	ioctl(0, TIOCGETP, &sgttyb);
	ospeed = sgttyb.sg_ospeed;
#endif
#endif
}

static void output_one(c)
	int c;
{
	*cptr++ = c;
}

static void gettcap(dest, cap)
	char *dest, *cap;
{
	if (!tgetstr(cap, &dest))
		Die("Insufficient terminal capabilities.");
}

static void getccap(dest, cap)
	char *dest, *cap;
{
	char temp[64];

	gettcap(temp, cap);
	cptr = dest;
	tputs(temp, 1, output_one);
	*cptr = '\0';
}
#endif

void init_term()
{
#ifdef HARDCODE
	strcpy(s_cmove	  , HC_CMOVE   );
	strcpy(s_scroll	  , HC_SCROLL  );
	strcpy(s_scr_rev  , HC_SCR_REV );
	strcpy(s_clrscr	  , HC_CLRSCR  );
	strcpy(s_clreol	  , HC_CLREOL  );
	strcpy(s_bold_on  , HC_BOLD_ON );
	strcpy(s_bold_off , HC_BOLD_OFF);
	rows = HC_ROWS;
	cols = HC_COLS;
#else /* HARDCODE not defined */
	char tinfo[1024], *termname, *bon, *boff;

	bon = getenv("VTBOLDON");
	if (!bon)
		bon = "md";
	boff = getenv("VTBOLDOFF");
	if (!boff)
		boff = "me";
	termname = getenv("TERM");
	if (!termname)
		Die("TERM not set");
	if (tgetent(tinfo, termname) != 1)
		Die("Terminal type not defined.");
	rows = tgetnum("li");
	if (rows == -1)
		rows = HC_ROWS;
	cols = tgetnum("co");
	if (cols == -1)
		cols = HC_COLS;
	get_ospeed();
	gettcap(s_cmove	   , "cm");
	gettcap(s_scroll   , "cs");
	gettcap(s_scr_rev  , "sr");
	getccap(s_clrscr   , "cl");
	getccap(s_clreol   , "ce");
	getccap(s_bold_on  , bon);
	getccap(s_bold_off , boff);
#endif /* HARDCODE not defined */
#ifdef TIOCGWINSZ
	{
		struct winsize size;

		ioctl(0, TIOCGWINSZ, &size);
		if (size.ws_row && size.ws_col) {
			rows = size.ws_row;
			cols = size.ws_col;
		}
	}
#endif /* TIOCGWINSZ */
	prompt = vtstrdup("");
}

#ifdef TERMCAP
static void bfuncputch(c) int c; { s_fadd(&outbuf, c); }
static void tp(s, affcnt)
	char *s;
	int affcnt;
{
	tputs(s, affcnt, bfuncputch);
	s_nt(&outbuf);
}
#endif

static void cap_formatted(cbuf, arg1, arg2)
	char *cbuf;
	int arg1, arg2;
{
#ifdef HARDCODE
	static char buffer[24];

	sprintf(buffer, cbuf, arg2 + 1, arg1 + 1);
	Bwritea(buffer);
#else
	tp(tgoto(cbuf, arg1, arg2), 1);
#endif
}

void cmove(col, row)
	int col, row;
{
	cap_formatted(s_cmove, col, row);
}

void scroll(top, bottom)
	int top, bottom;
{
	if (scr_top == top && scr_bot == bottom)
		return;
	cap_formatted(s_scroll, bottom, top);
	scr_top = top;
	scr_bot = bottom;
}

void scr_fwd(num)
	int num;
{
	cmove(0, scr_bot);
	while (num--)
		Bputch('\n');
}

void scr_rev(num)
	int num;
{
	cmove(0, scr_top);
	while (num--)
		cap_normal(s_scr_rev, scr_bot - scr_top + 1);
}

/* Slightly higher-level termcap routines */

static void clear_end(len)
	int len;
{
	if (len > 1)
		Bwritea(s_clreol);
	else if (len == 1)
		Bwritea(" \010");
}

static void clear_space(top, bottom)
	int top, bottom;
{
	int i;

	scroll(0, rows - 1);
	cmove(0, top);
	Bwritea(s_clreol);
	for (i = top; i < bottom; i++) {
		Bputch('\n');
		Bwritea(s_clreol);
	}
	Bflush;
}

static void move_left(n, new)
	int n, new;
{
	if (n < 7)
		chrpt('\010', n);
	else
		cmove(new, rows - 1);
}

/* Windowing routines */

/* Ensure that cursor and scroll area are in window */
void curs_window(win)
	Unode *win;
{
	if (curs_loc == CURS_WINDOW && curs_win == win)
		return;
	scroll(win->Wtop, win->Wbot);
	cmove(win->Wcol, win->Wbot);
	Bflush;
	curs_loc = CURS_WINDOW;
	curs_win = win;
}

/* Ensure that cursor is in input window */
void curs_input()
{
	if (curs_loc == CURS_INPUT)
		return;
	cmove(icol, rows - 1);
	Bflush;
	curs_loc = CURS_INPUT;
}

static void init_window(win, top, bottom)
	Unode *win;
	int top, bottom;
{
	win->Wtop = top;
	win->Wbot = bottom;
	win->Wcol = win->Wnl = 0;
	win->Wrmt = NULL;
	win->Wtermread = NULL;
	win->Wobj = NULL;
	win->Wghstack = NULL;
	win->Wglstack = NULL;
	win->Wrstack = NULL;
}

void draw_Divider(win)
	Unode *win;
{
	scroll(0, rows - 1);
	cmove(0, win->Wbot + 1);
	Bputch((win == active_win) ? '*' : '_');
	chrpt('_', win->next->dummy ? cols - 6 : cols - 1);
	if (win->next->dummy)
		Bwritea(vtc_mode ? "VTC__" : "Text_");
	Bflush;
	curs_loc = CURS_ELSEWHERE;
}

void toggle_imode()
{
	scroll(0, rows - 1);
	vtc_mode = !vtc_mode;
	cmove(cols - 5, Divider);
	Bwritea(vtc_mode ? "VTC__" : "Text_");
	curs_loc = CURS_ELSEWHERE;
	if (plen) {
		clear_input_window();
		draw_prompt();
		input_draw();
	}
}

static void mark(win, value)
	Unode *win;
	int value;
{
	cmove(0, win->Wbot + 1);
	Bputch(value ? '*' : '_');
	Bflush;
	curs_loc = CURS_ELSEWHERE;
}

void init_screen()
{
	Unode *first_win;

	first_win = unalloc();
	init_window(first_win, 0, rows - 5);
	win_ring.next = win_ring.prev = first_win;
	win_ring.dummy = 1;
	first_win->next = first_win->prev = &win_ring;
	active_win = first_win;
	Bwritea(s_clrscr);
	draw_Divider(first_win);
}

void redraw_screen()
{
	Unode *wp;

	Bwritea(s_clrscr);
	for (wp = win_ring.next; !wp->dummy; wp = wp->next)
		draw_Divider(wp);
	cmove(0, rows - 1);
	draw_prompt();
	input_draw();
}

void auto_redraw()
{
	Func *func;

	func = find_func("redraw_hook");
	if (func && func->cmd)
		run_prog(func->cmd);
	else
		redraw_screen();
}

Unode *split_window(win, loc)
	Unode *win;
	int loc;
{
	Unode *new;

	new = unalloc();
	init_window(new, win->Wtop, loc - 1);
	win->Wtop = loc + 1;
	new->prev = win->prev;
	new->next = win;
	win->prev = win->prev->next = new;
	clear_space(new->Wtop, new->Wbot);
	draw_Divider(new);
	return new;
}

void close_window(win, dir)
	Unode *win;
	int dir;
{
	Unode *into;

	if (dir < 0) {
		into = win->prev;
		scroll(into->Wtop, win->Wbot + 1);
		scr_rev(win->Wbot - into->Wbot);
		into->Wbot = win->Wbot;
		if (win->next->dummy) {
			cmove(cols - 5, Divider);
			Bwritea(vtc_mode ? "VTC__" : "Text_");
		}
		Bflush;
	} else {
		into = win->next;
		clear_space(win->Wtop, win->Wbot + 1);
		into->Wtop = win->Wtop;
	}
	if (win->Wrmt)
		win->Wrmt->Rwin = NULL;
	if (active_win == win) {
		active_win = into;
		mark(into, 1);
	}
	if (cur_win == win)
		cur_win = NULL;
	win->prev->next = win->next;
	win->next->prev = win->prev;
	destroy_pointers(win->frefs);
	break_pipe(win->Wghstack);
	break_pipe(win->Wglstack);
	break_pipe(win->Wrstack);
	discard_unode(win);
	curs_loc = CURS_ELSEWHERE;
}

void resize_window(win, row)
	Unode *win;
	int row;
{
	if (row - 1 > win->Wbot) {
		scroll(win->Wtop, row);
		scr_rev(row - win->Wbot - 1);
	} else if (row - 1 < win->Wbot) {
		scroll(win->Wtop, win->Wbot + 1);
		scr_fwd(win->Wbot - row + 1);
	}
	Bflush;
	win->next->Wtop = row + 1;
	win->Wbot = row - 1;
	curs_loc = CURS_ELSEWHERE;
}

void new_active_win(win)
	Unode *win;
{
	if (win == active_win)
		return;
	mark(active_win, 0);
	mark(win, 1);
	active_win = win;
	curs_loc = CURS_ELSEWHERE;
	update_echo_mode();
}

void resize_screen(new_rows, new_cols)
	int new_rows, new_cols;
{
	int new_Isize, new_ospace, old_ospace, pos = 0, used = 0, overrun;
	int num_wins = 0, size, decr = 0;
	Unode *w;

	/* Ignore ridiculous size. */
	if (new_rows == 0 || new_cols == 0)
		return;

	/* Die if we can't do anything useful in a screen this small. */
	if (new_rows < 4 || new_cols < 15)
		vtdie("Screen size too small");

	/* Optimize if only the columns changed. */
	if (new_rows == rows) {
		cols = new_cols;
		auto_redraw();
		return;
	}

	for (w = win_ring.next; !w->dummy; w = w->next, num_wins++);
	old_ospace = rows - Isize - num_wins;
	while (num_wins > (new_rows - 1) / 3) {
		w = win_ring.prev;
		if (w->Wrmt)
			w->Wrmt->Rwin = NULL;
		if (active_win == w)
			active_win = w->prev;
		if (cur_win == w)
		    cur_win = NULL;
		w->prev->next = &win_ring;
		win_ring.prev = w->prev;
		destroy_pointers(w->frefs);
		destroy_pipe(w->Wghstack);
		destroy_pipe(w->Wglstack);
		destroy_pipe(w->Wrstack);
		discard_unode(w);
		num_wins--;
	}
	new_Isize = min(Isize, new_rows - num_wins * 3);
	new_ospace = new_rows - new_Isize - num_wins;
	for (w = win_ring.next; !w->dummy; w = w->next) {
		size = ((w->Wbot - w->Wtop + 1) * new_ospace
		       + old_ospace / 2) / old_ospace;
		size = max(size, 2);
		w->Wtop = pos;
		w->Wbot = pos + size - 1;
		pos += size + 1;
		used += size;
	}
	overrun = used - new_ospace;
	for (w = win_ring.next; !w->dummy; w = w->next) {
		w->Wtop -= decr;
		w->Wbot -= decr;
		if (overrun > 0 && w->Wbot - w->Wtop > 1) {
			w->Wbot--;
			decr++;
			overrun--;
		}
	}
	rows = new_rows;
	cols = new_cols;
	auto_redraw();
}

/* Input routines */

static void draw_prompt()
{
	int pos = 0;

	while (pos < Oplen) {
		Bwritem(&prompt[pos], plen - pos, Margin);
		if ((pos += Margin) <= plen)
			Iscr_fwd;
	}
	icol = Oplen % Margin;
	Bflush;
}

static void clear_input_window()
{
	if (kpos + Oplen <= Isize * Margin) {
		input_cmove(0);
		while (plen > Margin) {
			Iscr_rev;
			plen -= Margin;
		}
		cmove(0, rows - 1);
		Bwritea(s_clreol);
		Bflush;
	} else
		clear_space(Itop, rows - 1);
}

void input_puts(cstr)
	Cstr cstr;
{
	int n;

	if (!echo_mode)
		return;
	while (cstr.l) {
		n = min(cstr.l, Margin - icol);
		Bwriteal(cstr.s, n);
		if (!(icol = (icol + n) % Margin))
			Iscr_fwd;
		cstr = cstr_sl(cstr.s + n, cstr.l - n);
	}
	n = min(Klen - kpos, Margin - icol);
	Bwriteal(&Kptr[kpos], n);
	move_left(n, icol);
	Bflush;
}

void input_cmove(new)
	int new;
{
	int offset = new - kpos, pos, n;

	if (new == kpos || !echo_mode)
		return;
	if (new == kpos - 1 && icol) {
		Bputch('\010');
		icol--;
	} else if (new == kpos + 1 && icol < Margin - 1) {
		Bputch(Kptr[kpos]);
		icol++;
	} else if (new > kpos) {
		pos = Round(kpos + Oplen, Margin) + Margin - Oplen;
		while (icol + offset >= Margin) {
			Iscr_fwd;
			Bwritem(&Kptr[pos], Klen - pos, Margin);
			pos += Margin;
			offset -= Margin;
		}
		cmove(icol += offset, rows - 1);
	} else {
		pos = Round(kpos + Oplen, Margin) - Oplen - Isize * Margin;
		while (icol + offset < 0) {
			Iscr_rev;
			if (pos >= 0)
				Bwritem(&Kptr[pos], Klen - pos, Margin);
			else if (pos + Oplen >= 0) {
				n = min(-pos, Margin);
				Bwriteal(&prompt[plen + pos], n);
				Bwritem(Kptr, Klen, Margin - n);
			}
			pos -= Margin;
			offset += Margin;
		}
		cmove(icol += offset, rows - 1);
	}
	Bflush;
}

void input_bdel(num)
	int num;
{
	int n;

	if (!num || !echo_mode)
		return;
	input_cmove(kpos - num);
	n = min(Klen - kpos, Margin - icol);
	Bwriteal(&Kptr[kpos], n);
	clear_end(min(num, Margin - icol - n));
	move_left(n, icol);
	Bflush;
}

void input_fdel(num)
	int num;
{
	int n;

	if (!echo_mode)
		return;
	n = min(Klen - kpos - num, Margin - icol);
	Bwriteal(&Kptr[kpos + num], n);
	clear_end(min(num, Margin - icol - n));
	move_left(n, icol);
	Bflush;
}

void input_clear()
{
	if (!echo_mode)
		return;
	if (kpos <= Isize * Margin) {
		input_cmove(0);
		Bwritea(s_clreol);
	} else {
		clear_space(Itop, rows - 1);
		draw_prompt();
	}
	icol = Oplen % Margin;
	Bflush;
}

void input_draw()
{
	int pos = 0, n = 0, col;

	if (!echo_mode)
		return;
	col = Oplen % Margin;
	while (pos <= kpos) {
		n = min(Klen - pos, Margin - col);
		Bwriteal(&Kptr[pos], n);
		if ((pos += Margin) <= kpos)
			Iscr_fwd;
		col = 0;
	}
	icol = Oplen + kpos % Margin;
	move_left(n - icol, icol);
	Bflush;
}

void input_newline()
{
	input_cmove(Klen);
	Iscr_fwd;
	icol = 0;
	if (plen && !vtc_mode) {
		Discardstring(prompt);
		prompt = vtstrdup("");
		plen = 0;
	}
	Bflush;
}

void change_prompt(new, l)
	char *new;
	int l;
{
	char *p, *q;

	if (!vtc_mode) {
		curs_input();
		clear_input_window();
	}
	Discardstring(prompt);
	for (p = new; *p; p++) {
		if (!isprint(*p))
			l--;
	}
	q = prompt = Newarray(char, l + 1);
	for (p = new; *p; p++) {
		if (isprint(*p))
			*q++ = *p;
	}
	*q = 0;
	plen = l;
	if (!vtc_mode) {
		draw_prompt();
		input_draw();
	}
}

void update_echo_mode()
{
	int new_mode = (active_win->Wrmt) ? active_win->Wrmt->Recho : 1;

	if (echo_mode != new_mode) {
		echo_mode = 1;
		curs_input();
		if (new_mode)
			input_draw();
		else
			input_clear();
		echo_mode = new_mode;
	}
}

/* Output routines */

void output(win, text)
	Unode *win;
	char *text;
{
	int norm, n;

	if (!win) {
		win = active_win;
		if (win->Wnl)
			output(win, "[background] ");
	}
	curs_window(win);
	if (win->Wnl) {
		Bputch('\n');
		win->Wnl = 0;
	}
	while (1) {
		norm = strcspn(text, "\n\b\f\v\t\r");
		n = cols - win->Wcol;
		for (; norm >= n; text += n, norm -= n, n = cols) {
			Bputch('\n');
			cmove(win->Wcol, win->Wbot - 1);
			Bwriteal(text, n);
			cmove(0, win->Wbot);
			win->Wcol = 0;
		}
		Bwriteal(text, norm);
		win->Wcol = (win->Wcol + norm) % cols;
		text += norm;
		switch(*text++) {
		    case '\0':
			Bflush;
			return;
		    case '\n':
		    case '\v':
			if (*text)
				Bputch('\n');
			else
				win->Wnl = 1;
			win->Wcol = 0;
		    Case '\b':
			if (win->Wcol) {
				Bputch('\b');
				win->Wcol--;
			}
		    Case '\f':
			clear_space(win->Wtop, win->Wbot);
			win->Wcol = 0;
			curs_window(win);
		    Case '\t':
			if (win->Wcol == cols - 1) {
				Bputch('\n');
				win->Wcol = 0;
			} else {
				do {
					Bputch(' ');
					win->Wcol++;
				} while (win->Wcol % 8 && 
					 win->Wcol < cols - 1);
			}
		    Case '\r':
			Bputch('\r');
			win->Wcol = 0;
		}
	}
}

#ifdef USE_STDARG
void outputf(char *fmt, ...)
#else
void outputf(va_alist)
	va_dcl
#endif
{
	va_list ap;
	char *ptr, *sval, *intstr;
	String *buf = &wbufs[1];
	int width = 0;
#ifdef USE_STDARG
	va_start(ap, fmt);
#else
	char *fmt;
	va_start(ap);
	fmt = va_arg(ap, char *);
#endif

	s_term(buf, 0);
	ptr = strchr(fmt, '%');
	while (ptr) {
		s_cat(buf, cstr_sl(fmt, ptr - fmt));
		if (isdigit(*++ptr)) {
			width = atoi(ptr);
			while (isdigit(*++ptr));
		}
		if (*ptr == 'd') {
			intstr = itoa(va_arg(ap, int));
			for (width -= strlen(intstr); width > 0; width--)
				s_fadd(buf, ' ');
			s_acat(buf, intstr);
		} else if (*ptr == 's') {
			sval = va_arg(ap, char *);
			for (width -= strlen(sval); width > 0; width--)
				s_fadd(buf, ' ');
			s_acat(buf, sval);
		} else if (*ptr == 'm') {
			sval = strerror(errno);
			for (width -= strlen(sval); width > 0; width--)
				s_fadd(buf, ' ');
			s_acat(buf, sval);
		} else
			s_fadd(buf, *ptr);
		fmt = ptr + 1;
		ptr = strchr(fmt, '%');
	}
	s_acat(buf, fmt);
	output(Cur_win, buf->c.s);
	va_end(ap);
}

void operror(s, rmt)
	char *s;
	Unode *rmt;
{
	char errbuf[128];
	Unode *win = rmt ? rmt->Rwin : Cur_win;

	strcpy(errbuf, s);
	strcat(errbuf, ": ");
	strcat(errbuf, strerror(errno));
	strcat(errbuf, "\n");
	output(win, errbuf);
}
