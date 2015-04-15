/* This source code is in the public domain. */

/* telnet.c: Support telnet protocol */

#include "vt.h"

#define ECHO	01
#define EOR	031
#define GA	0371
#define WILL	0373
#define WONT	0374
#define DO	0375
#define DONT	0376
#define IAC	0377

#ifdef PROTOTYPES
static void handle_echo_option(Unode *);
static void handle_eor_option(Unode *);
static void handle_unsupported_option(Unode *, int);
static void telnet_response(Unode *, int, int);
#else
static void handle_echo_option(), handle_eor_option();
static void handle_unsupported_option(), telnet_response();
#endif

void telnet_state_machine(rmt, c)
	Unode *rmt;
	int c;
{
	String *buf = rmt->Rtelnetbuf;
	char *p;
	int rid;

	if (!c) {
		if (rmt->Rraw && buf->c.l) {
			give_remote(rmt, istr_c(buf->c), 0);
			s_term(buf, 0);
		}
	} else if (!rmt->Riac) {
		switch (c) {
			case IAC:
				rmt->Riac = 1;
				break;
			case '\r':
				if (!rmt->Rraw)
					break;
			case '\n':
				if (!rmt->Rraw) {
					rid = rmt->id;
					give_remote(rmt, istr_c(buf->c), 0);
					if (rid == rmt->id)
						s_term(buf, 0);
					break;
				}
			default:
				s_add(buf, c);
				break;
		}
	} else if (!rmt->Rintent) {
		switch (c) {
			case WILL:
			case WONT:
			case DO:
			case DONT:
				rmt->Rintent = c;
				break;
			case EOR:
				if (!rmt->Reor) {
					rmt->Riac = 0;
					break;
				}
				/* Otherwise fall through. */
			case GA:
				/* Prompt terminator. */
				rid = rmt->id;
				if (rmt->Rraw) {
					p = strrchr(buf->c.s, '\n');
					if (p) {
						p++;
						give_remote(rmt,
						 istr_sl(buf->c.s,
							 p - buf->c.s),
						 0);
						s_delete(buf, 0, p - buf->c.s);
						if (rid != rmt->id)
							break;
					}
				}
				give_remote(rmt, istr_c(buf->c), 1);
				if (rid == rmt->id) {
					s_term(buf, 0);
					rmt->Riac = 0;
				}
				break;
			default:
				rmt->Riac = 0;
				break;
		}
	} else {
		switch(c) {
			case ECHO:
				handle_echo_option(rmt);
				break;
			case EOR:
				handle_eor_option(rmt);
				break;
			default:
				handle_unsupported_option(rmt, c);
				break;
		}
		rmt->Riac = rmt->Rintent = 0;
	}
}

static void handle_echo_option(rmt)
	Unode *rmt;
{
	int mode;

	if ((unsigned char) rmt->Rintent == DO) {
		telnet_response(rmt, WONT, ECHO);
	} else if ((unsigned char) rmt->Rintent != DONT) {
		mode = ((unsigned char) rmt->Rintent == WONT);
		if (rmt->Recho != mode) {
			rmt->Recho = mode;
			telnet_response(rmt, (mode) ? DONT : DO, ECHO);
			update_echo_mode();
		}
	}
}

static void handle_eor_option(rmt)
	Unode *rmt;
{
	int mode;

	if ((unsigned char) rmt->Rintent == DO) {
		telnet_response(rmt, WONT, EOR);
	} else if ((unsigned char) rmt->Rintent != DONT) {
		mode = ((unsigned char) rmt->Rintent == WILL);
		if (rmt->Reor != mode) {
			rmt->Reor = mode;
			telnet_response(rmt, (mode) ? DO : DONT, EOR);
		}
	}
}

static void handle_unsupported_option(rmt, c)
	Unode *rmt;
	int c;
{
	if ((unsigned char) rmt->Rintent == DO)
		telnet_response(rmt, WONT, c);
	else if ((unsigned char) rmt->Rintent == WILL)
		telnet_response(rmt, DONT, c);
}

static void telnet_response(rmt, intent, option)
	Unode *rmt;
	int intent, option;
{
	static char response[3] = { IAC };

	response[1] = intent;
	response[2] = option;
	transmit(rmt, cstr_sl(response, 3));
}

