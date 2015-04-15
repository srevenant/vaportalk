/* This source code is in the public domain. */

/* signal.c: Handle signals in an intelligent manner */

#include "vt.h"
#include <signal.h>
#include <sys/ioctl.h>

/* Not a very good test */
#if defined(sun) && defined(__svr4__)
#include <sys/tty.h>
#include <sys/stream.h>
#include <sys/ptyvar.h>
#endif

int break_pending = 0, console_pending = 0, winresize_pending = 0;
int stop_pending = 0;
extern int console_mode, rows, cols, debug;

static void sigint_handler()
{
	char c;

	signal(SIGINT, sigint_handler);
	coutput("[Q]uit [R]esume [B]reak [C]onsole [D]ebug: ");
	do
		c = lcase(getch());
	while (!strchr("qbrcd\033", c));
	switch(c) {
	    case 'q':
		coutput("Quit\n");
		cleanup();
		exit(0);
	    Case 'r':
	    case '\033':
		coutput("Resume\n");
	    Case 'b':
		coutput("Break\n");
		break_pending = 1;
	    Case 'c':
		coutput("Console\n");
		console_pending = !console_mode;
	    Case 'd':
		coutput("Debug\n[+] On [-] Off: ");
		while (!strchr("+-\033", c = getch()));
		switch(c) {
		    case '+':
			coutput("On\n");
			debug = 1;
		    Case '-':
			coutput("Off\n");
			debug = 0;
		}
	}
}

static void sigtstp_handler()
{
	signal(SIGTSTP, sigtstp_handler);
	stop_pending = 1;
}

static void sighup_handler()
{
	cleanup();
	exit(0);
}

void stop()
{
	scroll(0, rows - 1);
	cmove(0, rows - 1);
	bflushfunc();
	tty_mode(0);
	kill(getpid(), SIGSTOP);
	tty_mode(1);
	auto_redraw();
	stop_pending = 0;
}

#ifdef SIGWINCH
static void sigwinch_handler()
{
	signal(SIGWINCH, sigwinch_handler);
	winresize_pending = 1;
}
#endif /* SIGWINCH */

void winresize()
{
#ifdef TIOCGWINSZ
	struct winsize size;

	if (ioctl(0, TIOCGWINSZ, &size) >= 0)
		resize_screen(size.ws_row, size.ws_col);
#else /* TIOCGWINSZ */
#ifdef TIOCGSIZE
	struct ttysize size;  

	if (ioctl(0, TIOCGSIZE, &size) >= 0)
		resize_screen(size.ts_lines, size.ts_cols);
#endif /* TIOCGSIZE */
#endif /* TIOCGWINSZ */
	winresize_pending = 0;
}

void init_signals()
{
	signal(SIGINT  , sigint_handler	  );
	signal(SIGTSTP , sigtstp_handler  );
	signal(SIGPIPE , SIG_IGN	  );
	signal(SIGHUP  , sighup_handler   );
#ifdef SIGWINCH
	signal(SIGWINCH, sigwinch_handler );
#endif
}

