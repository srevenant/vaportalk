/* This source code is in the public domain. */

/* remote.c: Handle I/O to remote */

#include "vt.h"
#include <errno.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#ifdef PROTOTYPES
static int get_in_addr(char *, struct in_addr *);
static void receive_input(Unode *rmt);
static Unode *create_rmt(void);
#else
static void receive_input();
static Unode *create_rmt();
#endif

#ifndef FD_ZERO
typedef struct fd_set_tag {
	long masks[9];
} DESCR_MASK;
#define FD_SET(n, p)   ((p)->masks[(n) / 32] |=	 (1 << ((n) % 32)))
#define FD_CLR(n, p)   ((p)->masks[(n) / 32] &= ~(1 << ((n) % 32)))
#define FD_ISSET(n, p) ((p)->masks[(n) / 32] &	 (1 << ((n) % 32)))
#define FD_ZERO(p)     memset((p), 0, sizeof(*(p)))
#else
typedef fd_set DESCR_MASK;
#endif

#define INBUF_SIZE 1024
extern int errno, sys_nerr;
extern unsigned long inet_addr();
Unode rmt_ring;				/* Dummy node for remotes ring */
Unode *cur_rmt = NULL;			/* Remote being processed      */
String kin;

void init_rmt()
{
	rmt_ring.next = rmt_ring.prev = &rmt_ring;
	rmt_ring.dummy = 1;
}

static int get_in_addr(name, addr)
	char *name;
	struct in_addr *addr;
{
	struct hostent *hostent;

	if (isdigit(*name)) {
		addr->s_addr = inet_addr(name);
		return addr->s_addr != -1;
	}
	hostent = gethostbyname(name);
	if (!hostent)
		return 0;
	memcpy(addr, hostent->h_addr, sizeof(struct in_addr));
	return 1;
}

static Unode *create_rmt()
{
	Unode *rmt;

	rmt = unalloc();
	rmt->Rbuf = New(String);
	*rmt->Rbuf = empty_string;
	rmt->Rtelnetbuf = New(String);
	*rmt->Rtelnetbuf = empty_string;
	rmt->Rwin = NULL;
	rmt->Rnetread = NULL;
	rmt->Rpromptread = NULL;
	rmt->Rrstack = NULL;
	rmt->Robj = NULL;
	rmt->Rback = rmt->Rbusy = rmt->Rraw = 0;
	rmt->Riac = rmt->Rintent = 0;
	rmt->Recho = 1;
	rmt->Rdisconnected = 0;
	rmt->prev = rmt_ring.prev;
	rmt->next = &rmt_ring;
	rmt_ring.prev = rmt_ring.prev->next = rmt;
	return rmt;
}

Unode *new_rmt(addr, port)
	char *addr;
	int port;
{
	struct sockaddr_in saddr;
	struct in_addr hostaddr;
	int fd;
	Unode *rmt;
 
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	if (!get_in_addr(addr, &hostaddr)) {
		vtc_errmsg = "Couldn't find host";
		return NULL;
	}
	vtc_errmsg = NULL;
	memcpy(&saddr.sin_addr, &hostaddr, sizeof(struct in_addr));
	fd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	if (fd < 0)
		return NULL;
	if (connect(fd, &saddr, sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return NULL;
	}
	rmt = create_rmt();
	rmt->Raddr = vtstrdup(addr);
	rmt->Rport = port;
	rmt->Rfd = fd;
	rmt->Rpid = -1;
	return rmt;
}

Unode *new_rmt_shell(cmd)
	char *cmd;
{
	pid_t pid;
	int fds[2], save_errno;
	Unode *rmt;

	vtc_errmsg = NULL;
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
		return NULL;

	pid = fork();
	if (pid < 0) {
		save_errno = errno;
		close(fds[0]);
		close(fds[1]);
		errno = save_errno;
		return NULL;
	}

	if (pid == 0) {
		close(fds[1]);
		dup2(fds[0], 0);
		dup2(fds[0], 1);
		dup2(fds[0], 2);
#ifdef BSD43
		setpgrp(getpid());
#else
		setsid();
#endif
		execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
		perror("exec");
		exit(1);
	}
	close(fds[0]);

	rmt = create_rmt();
	rmt->Raddr = vtstrdup(cmd);
	rmt->Rport = -1;
	rmt->Rfd = fds[1];
	rmt->Rpid = pid;
	return rmt;
}

void disconnect(rmt)
	Unode *rmt;
{
	Func *func;
	Unode *old_rmt;
	int ormt_id;

	if (rmt->Rdisconnected)
		return;
	rmt->Rdisconnected = 1;
	func = find_func("disconnect_hook");
	if (func && func->cmd) {
		old_rmt = cur_rmt;
		if (cur_rmt)
			ormt_id = cur_rmt->id;
		cur_rmt = rmt;
		run_prog(func->cmd);
		cur_rmt = old_rmt && ormt_id == old_rmt->id ? old_rmt : NULL;
	}
	if (rmt->Rwin) {
		rmt->Rwin->Wrmt = NULL;
		update_echo_mode();
	}
	close(rmt->Rfd);
	if (rmt->Rpid != -1) {
		/* Send SIGHUP to process. */
#ifdef BSD43
		kill(-rmt->Rpid, SIGHUP);
		wait(NULL);
#else
		int status;
		kill(-rmt->Rpid, SIGHUP);
		waitpid(rmt->Rpid, &status, 0);
#endif
	}
	s_free(rmt->Rbuf);
	Discard(rmt->Rbuf, String);
	rmt->prev->next = rmt->next;
	rmt->next->prev = rmt->prev;
	if (cur_rmt == rmt)
		cur_rmt = NULL;
	destroy_pointers(rmt->frefs);
	break_pipe(rmt->Rrstack);
	discard_unode(rmt);
}

static void receive_input(rmt)
	Unode *rmt;
{
	int count;
	char block[INBUF_SIZE + 1];

	do
		count = read(rmt->Rfd, block, INBUF_SIZE);
	while (count == -1 && (errno == EINTR || errno == EWOULDBLOCK));
	if (count <= 0) {
		vtc_errflag = 1;
		if (count == 0)
		    vtc_errmsg = "Connection closed by foreign host.";
		else
		    vtc_errmsg = NULL;
		disconnect(rmt);
		return;
	}
	block[count] = '\0';
	s_acat(rmt->Rbuf, block);
	rmt->Rinbuf = 1;
}

int io_check(sec, usec)
	long sec, usec;
{
	struct timeval tv, *tvp;
	DESCR_MASK readers, except;
	int count, nfd, fd;
	Unode *rp, *next;
	char block[INBUF_SIZE + 1];

	if (sec == -1) {
		tvp = NULL;
	} else {
		tv.tv_sec = sec;
		tv.tv_usec = usec;
		tvp = &tv;
	}
	FD_ZERO(&readers);
	FD_ZERO(&except);
	FD_SET(0, &readers);
	nfd = 1;
	for (rp = rmt_ring.next; !rp->dummy; rp = rp->next) {
		if ((rp->Rwin || rp->Rback) && !rp->Rbusy) {
			fd = rp->Rfd;
			FD_SET(fd, &readers);
			if (nfd <= fd)
				nfd = fd + 1;
		}
	}
	count = select(nfd, &readers, (DESCR_MASK *) NULL, &except, tvp);
	if (count == 0 || count == -1 && errno == EINTR)
		return 0;
	if (count == -1)
		operror("select", NULL);
	for (rp = rmt_ring.next; !rp->dummy; rp = next) {
		next = rp->next;
		if ((rp->Rwin || rp->Rback) && !rp->Rbusy) {
			if (FD_ISSET(rp->Rfd, &readers))
				receive_input(rp);
		}
	}
	if (FD_ISSET(0, &readers)) {
		do
			count = read(0, block, INBUF_SIZE);
		while (count == -1 && errno == EINTR);
		s_cat(&kin, cstr_sl(block, count));
	}
	return 1;
}

int transmit(rmt, cstr)
	Unode *rmt;
	Cstr cstr;
{
	int written;

	if (rmt->Rdisconnected)
		return -1;
	while (cstr.l > 0) {
		written = write(rmt->Rfd, cstr.s, cstr.l);
		if (written == -1) {
			if (errno == EWOULDBLOCK) {
				sleep(1);
				continue;
			}
			vtc_errflag = 1;
			vtc_errmsg = NULL;
			disconnect(rmt);
			return -1;
		}
		cstr.s += written;
		cstr.l -= written;
		if (cstr.l)
			sleep(1);
	}
	return 0;
}

int input_waiting(fd)
	int fd;
{
	struct timeval tv;
	DESCR_MASK readers;

	tv.tv_sec = tv.tv_usec = 0;
	FD_ZERO(&readers);
	FD_SET(fd, &readers);
	return select(fd + 1, &readers, (DESCR_MASK *) NULL,
		      (DESCR_MASK *) NULL, &tv);
}

int process_remote_text(rmt)
	Unode *rmt;
{
	int rid = rmt->id;
	char *s;

	for (s = rmt->Rbuf->c.s; *s; s++) {
		if (!(rmt->Rwin || rmt->Rback) || rmt->Rbusy) {
			s_cpy(rmt->Rbuf, cstr_sl(s, rmt->Rbuf->c.l -
				(s - rmt->Rbuf->c.s)));
			telnet_state_machine(rmt, 0);
			return 1;
		}
		rmt->Rinbuf = (s[1] != 0);
		telnet_state_machine(rmt, (unsigned char) *s);
		if (rmt->id != rid)
			return 0;
	}
	telnet_state_machine(rmt, 0);
	s_term(rmt->Rbuf, 0);
	rmt->Rinbuf = (rmt->Rbuf->c.l != 0);
	return 1;
}

void give_remote(rmt, is, isgoahead)
	Unode *rmt;
	Istr *is;
	int isgoahead;
{
	Func *func;

	is->refs++;
	if (rmt->Rrstack && !isgoahead) {
		resume_istr(&rmt->Rrstack, is);
	} else {
		func = (isgoahead) ? rmt->Rpromptread : rmt->Rnetread;
		if (func) {
			run_prog_istr(func->cmd, is, NULL, rmt);
		} else {
			output(rmt->Rwin, is->rs->str.c.s);
			if (!isgoahead)
				output(rmt->Rwin, "\n");
		}
	}
	dec_ref_istr(is);
}

void kill_children()
{
	/* Kill off all shell processes. */
	Unode *rmt;

	for (rmt = rmt_ring.next; !rmt->dummy; rmt = rmt->next) {
		if (rmt->Rpid != -1)
			kill(-rmt->Rpid, SIGHUP);
	}
}
