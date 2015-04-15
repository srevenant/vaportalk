/* This source code is in the public domain. */

/* console.c: Control over suspended programs */

#include "vt.h"

#ifdef PROTOTYPES
static void main_menu(void);
static void show_menu(void);
static void del_menu(void);
static void del_estate(Estate **);
static int get_num(int);
static void list_estates(Estate *);
static int get_one_of(char *);
#else
static void main_menu(), show_menu(), del_menu(), del_estate();
static void list_estates();
#endif

int console_mode = 0;			/* Console currently executing */
static int nwin;
static int nrmt;
extern int console_pending;
extern Unode win_ring, rmt_ring;
extern Estate *events, *gen_read, *gen_high, *gen_low;

void console()
{
	Unode *un;

	console_mode = 1;
	console_pending = 0;
	coutput("Entering VT console mode\n");
	for (nwin = 0, un = win_ring.next; !un->dummy; un = un->next, nwin++);
	for (nrmt = 0, un = rmt_ring.next; !un->dummy; un = un->next, nrmt++);
	main_menu();
	console_mode = 0;
}

static void main_menu()
{
	for (;;) {
		coutput("[S]how [D]elete [Q]uit: ");
		switch (get_one_of("sdq\033")) {
		    case 's':
			coutput("Show\n");
			show_menu();
		    Case 'd':
			coutput("Del\n");
			del_menu();
		    Case 'q':
		    case '\033':
			coutput("Quit\n");
			return;
		}
	}
}

static void show_menu()
{
	int i;
	Unode *un;

	coutput("[W]indow [R]emote [G]eneral: ");
	switch (get_one_of("wrg\033")) {
	    case 'w':
		coutput("Window #");
		i = get_num(nwin - 1);
		if (i == -1)
			break;
		for (un = win_ring.next; i--; un = un->next);
		coutput("[GH] getch(HIGH) [GL] getch(LOW) [R] read(): ");
		switch (get_one_of("gr\033")) {
		    case 'g':
			coutput("getch(");
			switch(get_one_of("hl\033")) {
			    case 'h':
				coutput("HIGH)\n");
				list_estates(un->Wghstack);
			    Case 'l':
				coutput("LOW)\n");
				list_estates(un->Wglstack);
			    Case '\033':
				coutput("\n");
			}
		    Case 'r':
			coutput("read()\n");
			list_estates(un->Wrstack);
		    Case '\033':
			coutput("\n");
		}
	    Case 'r':
		coutput("Remote #");
		i = get_num(nrmt - 1);
		if (i != -1) {
			for (un = rmt_ring.next; i--; un = un->next);
			list_estates(un->Rrstack);
		}
	    Case 'g':
		coutput("General\n");
		coutput("[S]leep [R]ead [GH] getch(HIGH) [GL] getch(LOW): ");
		switch(get_one_of("srg\033")) {
		    case 's':
			coutput("sleep()\n");
			list_estates(events);
		    Case 'r':
			coutput("read()\n");
			list_estates(gen_read);
		    Case 'g':
			coutput("getch(");
			switch(get_one_of("hl")) {
			    case 'h':
				coutput("HIGH)\n");
				list_estates(gen_high);
			    Case 'l':
				coutput("LOW)\n");
				list_estates(gen_low);
			    Case '\033':
				coutput("\n");
			}
		    Case '\033':
			coutput("\n");
		}
	    Case '\033':
		coutput("\n");
	}
}

static void del_menu()
{
	int i;
	Unode *un;

	coutput("[W]indow [R]emote [G]eneral: ");
	switch(get_one_of("wrg\033")) {
	    case 'w':
		coutput("Window #");
		i = get_num(nwin - 1);
		if (i == -1)
			break;
		for (un = win_ring.next; i--; un = un->next);
		coutput("[GH] getch(HIGH) [GL] getch(LOW) [R] read(): ");
		switch(get_one_of("gr\033")) {
		    case 'g':
			coutput("getch(");
			switch(get_one_of("hl")) {
			    case 'h':
				coutput("HIGH) #");
				del_estate(&un->Wghstack);
			    Case 'l':
				coutput("LOW) #");
				del_estate(&un->Wglstack);
			    Case '\033':
				coutput("\n");
			}
		    Case 'r':
			coutput("read() #");
			del_estate(&un->Wrstack);
		    Case '\033':
			coutput("\n");
		}
	    Case 'r':
		coutput("Remote #");
		i = get_num(nrmt - 1);
		if (i != -1) {
		    for (un = rmt_ring.next; i--; un = un->next);
		    coutput("read_rmt() #");
		    del_estate(&un->Rrstack);
		}
	    Case 'g':
		coutput("General\n");
		coutput("[S]leep [R]ead [GH] getch(HIGH) [GL] getch(LOW): ");
		switch(get_one_of("srg\033")) {
		    case 's':
			coutput("sleep() #");
			del_estate(&events);
		    Case 'r':
			coutput("read() #");
			del_estate(&gen_read);
		    Case 'g':
			coutput("getch(");
			switch (get_one_of("hl")) {
			    case 'h':
				coutput("HIGH) #");
				del_estate(&gen_high);
			    Case 'l':
				coutput("LOW) #");
				del_estate(&gen_low);
			    Case '\033':
				coutput("\n");
			}
		    Case '\033':
			coutput("\n");
		}
	    Case '\033':
		coutput("\n");
	}
}

static void del_estate(list)
	Estate **list;
{
	Estate *es;
	int num;

	for (num = 0, es = *list; es; num++, es = es->next);
	num = get_num(num - 1);
	if (num == -1)
		return;
	for (; num--; list = &(*list)->next);
	discard_estate(list);
}

static int get_num(maxval)
	int maxval;
{
	char c, buf[2];
	int val = 0, blank = 1;

	if (maxval < 0) {
		coutput("None exist\n");
		return -1;
	}
	for (;;) {
		do
			c = getch();
		while (!isdigit(c) && c != '\033' && c != '\n');
		if (c == '\033') {
			coutput("\n");
			return -1;
		}
		if (c == '\n') {
			if (blank)
				coutput("0");
			coutput("\n");
			return val;
		}
		val = val * 10 + c - '0';
		if (val > maxval) {
			coutput("Invalid\n");
			return -1;
		}
		buf[0] = c;
		buf[1] = '\0';
		coutput(buf);
		blank = 0;
		if (val * 10 > maxval || !val) {
			coutput("\n");
			return val;
		}
	}
}

static void list_estates(list)
	Estate *list;
{
	int count = 0, i;
	char *name;

	for (; list; list = list->next, count++) {
		outputf("%3d: ", count);
		for (i = 0; i < list->cframes; i++) {
			name = lookup_prog(list->cimage[i].prog);
			coutput(name ? name : "<no name>");
			coutput((i == list->cframes - 1) ? "\n" : "\n	  ");
		}
	}
}

static int get_one_of(possibilities)
	char *possibilities;
{
	char c;

	do
		c = lcase(getch());
	while (!strchr(possibilities, c));
	return c;
}

