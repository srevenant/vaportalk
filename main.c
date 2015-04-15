/* This source code is in the public domain. */

/* main.c: Initialization and main loop */

#include "vt.h"

#ifdef PROTOTYPES
static void load_startup_file(int, char **);
static void main_loop(void);
static int io_cycle(void);
#else
static void load_startup_file(), main_loop();
#endif

Estate *events = NULL, *gen_read = NULL, *gen_high = NULL, *gen_low = NULL;
extern Unode rmt_ring, *cur_rmt;
extern String kin;
extern int curs_loc;
extern int break_pending, console_pending, stop_pending, winresize_pending;
#define CURS_INPUT     0
#define CURS_ELSEWHERE 2
Unode file_ring;
Istr freader, wrapbuf;

static char *search_path[] = {
	"~/main.vtc",
	"~/vt/dist/main.vtc",
	"~/vt/main.vtc",
	"/mit/outland/share/lib/vt/dist/main.vtc",
	NULL
};

main(argc, argv)
	int argc;
	char **argv;
{
	if (argc > 2) {
		puts("Usage: vt [startfile]");
		exit(1);
	}
	file_ring.prev = file_ring.next = &file_ring;
	file_ring.dummy = 1;
	freader.rs = New(Rstr);
	freader.rs->str = empty_string;
	freader.rs->refs = 1;
	freader.refs = 1;
	wrapbuf.rs = New(Rstr);
	wrapbuf.rs->str = empty_string;
	wrapbuf.rs->refs = 1;
	wrapbuf.refs = 1;
	tty_mode(1);			/* window.c */
	init_array();			/* array.c  */
	init_wbufs();			/* string.c */
	init_signals();			/* signal.c */
	init_tables();			/* string.c */
	init_const();			/* const.c  */
	init_term();			/* window.c */
	init_unalloc();			/* unode.c  */
	init_key();			/* key.c    */
	init_rmt();			/* remote.c */
	init_prmt();			/* prmtab.c */
	init_bobj();			/* bobj.c   */
	init_interp();			/* interp.c */
	init_screen();			/* window.c */
	init_compile();			/* vtc.y    */
	load_startup_file(argc, argv);
	main_loop();
}

static void load_startup_file(argc, argv)
	int argc;
	char **argv;
{
	char *s, **p;

	if (argc > 1) {
		if (load_file(argv[1]) != -1)
			return;
		outputf("Warning: couldn't open file %s\n", argv[1]);
	}
	s = getenv("VTSTART");
	if (s) {
		if (load_file(s) != -1)
			return;
		outputf("Warning: couldn't open file %s\n", s);
	}
	for (p = search_path; *p && load_file(*p) == -1; p++);
	if (!*p)
		vtdie("Couldn't find startup file");
}

static void main_loop()
{
	long sec, usec;
	Dframe df;

	df.type = F_NULL;
	for (;;) {
		break_pending = 0;
		if (stop_pending)
			stop();
		if (winresize_pending)
			winresize();
		if (console_pending)
			console();
		sec = -1;
		usec = 250000;
		if (events) {
			sec = events->timer - time((long *) NULL);
			if (--sec < 0)
				usec = sec = 0;
		}
		if (curs_loc != CURS_INPUT)
			sec = 0;
		if (io_check(sec, usec)) {
			if (break_pending) {
				/* Quarter-second max delay next cycle */
				curs_loc = CURS_ELSEWHERE;
				continue;
			}
			while (!io_cycle());
			if (kin.c.l) {
				process_incoming(kin.c.s);
				s_term(&kin, 0);
			}
		} else
			curs_input();
		if (events && events->timer <= time((long *) NULL)
		 && !break_pending)
			resume(&events, &df);
	}
}

static int io_cycle()
{
	Unode *rmt;

	for (rmt = rmt_ring.next; !rmt->dummy; rmt = rmt->next) {
		if (!process_remote_text(rmt))
			return 0;
	}
	return 1;
}

void add_ioqueue(queue, estate)
	Estate **queue, *estate;
{
	estate->next = NULL;
	for (; *queue; queue = &(*queue)->next);
	*queue = estate;
}

