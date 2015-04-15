/* This source code is in the public domain. */

/* util.c: Miscellaneous utility functions */
 
#include "vt.h"
#include <signal.h>
#include <pwd.h>
 
extern int rows;
extern void endpwent();

void vterror(s)
	char *s;
{
	char c;
	static int recursive = 0;

	if (recursive)
		exit(1);
	recursive = 1;
	clrscr();
	scroll(0, rows - 1);
	bflushfunc();
	puts(s);
	write(1, "Dump core? ", 12);
	while (!strchr("YyNn", c = getch()));
	tty_mode(0);
	if (c == 'Y' || c == 'y') {
		write(1, "Yes\n", 4);
		abort();
	} else
		write(1, "No\n", 3);
	exit(1);
}

void vtdie(s)
	char *s;
{
	coutput(s);
	cleanup();
	exit(1);
}

void regerror(s)
	char *s;
{
	vtc_errmsg = s;
}

char *dmalloc(size)
	size_t size;
{
	char *ret;
 
	if (size <= 0)
		return NULL;
	ret = tmalloc(size);
	if (!ret)
		vterror("malloc() failed");
	return ret;
}
 
char *drealloc(ptr, oldsize, newsize)
	char *ptr;
	size_t oldsize, newsize;
{
	char *ret;
 
	if (!ptr)
		return dmalloc(newsize);
	ret = trealloc(ptr, oldsize, newsize);
	if (!ret)
		vterror("realloc() failed");
	return ret;
}
 
void dfree(ptr, size)
	char *ptr;
	size_t size;
{
	if (ptr)
		tfree(ptr, size);
}
 
void cleanup()
{
	kill_children();
	tty_mode(0);
	scroll(0, rows - 1);
	cmove(0, rows - 1);
	bflushfunc();
}
 
#ifdef memset
void vtmemset(loc, val, len)
	char *loc;
	int val, len;
{
	while (len--)
		*loc++ = val;
}
#endif
 
/* Credits to Leo Plotkin and Anton Rang */
char *expand(s)
	char *s;
{
	String *buf = &wbufs[0];
	struct passwd *uinfo;
	char *homedir, *rest;
 
	homedir = getenv("HOME");
	if (*s != '~' || !homedir)
		return s;
	if (*++s == '/' || !*s) {
		s_acpy(buf, homedir);
		s_acat(buf, s);
		return buf->c.s;
	}
	rest = strchr(s, '/');
	if (rest)
		*rest = '\0';
	uinfo = getpwnam(s);
	if (rest)
		*rest = '/';
	if (!uinfo)
		return s;
	s_acat(buf, uinfo->pw_dir);
	s_acat(buf, rest ? rest : "");
	return buf->c.s;
}
 
char *itoa(num)
	int num;
{
	static char buf[32];
	char *p = &buf[31];
	int sign = 0;
 
	if (num < 0) {
		sign = 1;
		num = -num;
	}
	if (!num)
		*--p = '0';
	while (num) {
		*--p = num % 10 + '0';
		num /= 10;
	}
	if (sign)
		*--p = '-';
	return p;
}
 
int smatch(p, s)
	char *p, *s;
{
	while (*p) {
		if (*p == '*') {
			while (*++p == '?' || *p == '*') {
				if (*p == '?' && !*s++)
					return 0;
			}
			if (!*p)
				return 1;
			for (; *s; s++) {
				if (lcase(*s) == lcase(*p) && smatch(p, s))
					return 1;
			}
			return 0;
		} else if (*p == '?') {
			if (!*s++)
				return 0;
			p++;
		} else if (lcase(*p++) != lcase(*s++))
			return 0;
	}
	return !*s;
}
 
