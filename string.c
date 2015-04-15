/* This source code is in the public domain. */

/* string.c: string manipulation routines */

#include "vt.h"

String wbufs[NUM_WBUFS];
Cstr empty_cstr = { "", 0 };
String empty_string = { { "", 0 }, 0 };
#ifndef ANSI_CTYPES
char lowercase_values[128];
char uppercase_values[128];
#endif
#define INIT_SIZE 32
#define BLOCKSIZE (4 * INIT_SIZE)

/* Operations on character strings, some of which may be redefined */

char *vtstrnchr(s, c, n)
	char *s;
	int c, n;
{
	while (n-- && *s != c)
		s++;
	return n >= 0 ? s : NULL;
}

char *vtstrnrchr(s, c, n)
	char *s;
	int c, n;
{
	for (s += n; n-- && *--s != c;);
	return n >= 0 ? s : NULL;
}

#ifdef strcspn
int vtstrcspn(s1, s2)
	char *s1, *s2;
{
	char *p;

	for (p = s1; *p && !strchr(s2, *p); p++);
	return p - s1;
}
#endif

#ifdef strcasecmp
int vtstricmp(s, t)
	char *s, *t;
{
	for (; *s && *t && lcase(*s) == lcase(*t); s++, t++);
	return lcase(*s) - lcase(*t);
}
#endif

#ifdef strncasecmp
int vtstrnicmp(s, t, n)
	char *s, *t;
	int n;
{
	for (; n-- && *s && *t && lcase(*s) == lcase(*t); s++, t++);
	return (n < 0) ? 0 : lcase(*s) - lcase(*t);
}
#endif

#ifdef strstr
char *vtstrstr(str, find)
	char *str, *find;
{
	int l;

	l = strlen(find);
	while (str = strchr(str, *find)) {
		if (!strncmp(str, find, l))
			return str;
		str++;
	}
	return NULL;
}
#endif

char *vtstristr(s, f)
	char *s;
	Cstr f;
{
	for (; *s; s++) {
		if (lcase(*s) == lcase(*f.s) && !strncasecmp(s, f.s, f.l))
			return s;
	}
	return NULL;
}

char *vtstrdup(s)
	char *s;
{
	char *new;
	int len;

	len = strlen(s);
	new = Newarray(char, len + 1);
	memcpy(new, s, len + 1);
	return new;
}

/* Since a few non-ANSI compilers kludge and define tolower(c) and
** toupper(c) in such a way that they're invalid if c is not uppercase
** or lowercase, respectively, to begin with (e.g. #define tolower(c)
** (c - 'A' + 'a')), we recreate the tables and use those. */
void init_tables()
{
#ifndef ANSI_CTYPES
	int c;

	for (c = 0; c < 128; c++) {
		lowercase_values[c] = isupper(c) ? tolower(c) : c;
		uppercase_values[c] = islower(c) ? toupper(c) : c;
	}
#endif
}

/* Algorithm by Peter J. Weinberger */
int hash(s, size)
	char *s;
	int size;
{
	unsigned hashval, g;

	for (hashval = 0; *s; s++) {
		hashval = (hashval << 4) + *s;
		if (g = hashval & 0xf0000000) {
			hashval ^= g >> 24;
			hashval ^= g;
		}
	}
	return (int)(hashval % size);
}

/* Counted string constructors */

Cstr cstr_sl(s, l)
	char *s;
	int l;
{
	Cstr cstr;

	cstr.s = s;
	cstr.l = l;
	return cstr;
}

Cstr cstr_s(s)
	char *s;
{
	Cstr cstr;

	cstr.s = s;
	cstr.l = strlen(s);
	return cstr;
}

Cstr cstr_c(cstr)
	Cstr cstr;
{
	Cstr new;

	new.s = Newarray(char, cstr.l + 1);
	memcpy(new.s, cstr.s, cstr.l);
	new.s[new.l = cstr.l] = '\0';
	return new;
}

/* Stretchybuffer operations */

void init_wbufs()
{
	int i;

	for (i = 0; i < NUM_WBUFS; i++) {
		wbufs[i].c = empty_cstr;
		wbufs[i].sz = 0;
	}
}

void s_free(str)
	String *str;
{
	if (str->sz)
		Discardarray(str->c.s, char, str->sz);
}

void lcheck(str, len)
	String *str;
	int len;
{
	int sz;

	if (len >= str->sz) {
		for (sz = INIT_SIZE; sz <= len; sz <<= 1);
		if (str->sz) {
			str->c.s = Rearray(str->c.s, char, str->sz, sz);
		} else {
			str->c.s = Newarray(char, sz);
			*str->c.s = '\0';
		}
		str->sz = sz;
	}
}

void s_add(str, c)
	String *str;
	int c;
{
	lcheck(str, str->c.l + 1);
	str->c.s[str->c.l++] = c;
	str->c.s[str->c.l] = '\0';
}

void s_fadd(str, c)
	String *str;
	int c;
{
	lcheck(str, str->c.l + 1);
	str->c.s[str->c.l++] = c;
}

void s_nt(str)
	String *str;
{
	if (str->sz)
		str->c.s[str->c.l] = '\0';
}

void s_term(str, l)
	String *str;
	int l;
{
	if (str->c.l > l)
		str->c.s[str->c.l = l] = '\0';
}

void s_cpy(dest, cstr)
	String *dest;
	Cstr cstr;
{
	lcheck(dest, dest->c.l = cstr.l);
	memcpy(dest->c.s, cstr.s, cstr.l);
	dest->c.s[cstr.l] = '\0';
}

void s_cat(dest, cstr)
	String *dest;
	Cstr cstr;
{
	lcheck(dest, dest->c.l + cstr.l);
	memcpy(dest->c.s + dest->c.l, cstr.s, cstr.l);
	dest->c.l += cstr.l;
	dest->c.s[dest->c.l] = '\0';
}

void s_insert(dest, cstr, loc)
	String *dest;
	Cstr cstr;
	int loc;
{
	if (!cstr.l)
		return;
	lcheck(dest, dest->c.l + cstr.l);
	memmove(dest->c.s + loc + cstr.l, dest->c.s + loc,
		dest->c.l - loc + 1);
	memcpy(dest->c.s + loc, cstr.s, cstr.l);
	dest->c.l += cstr.l;
}

void s_delete(str, loc, l)
	String *str;
	int loc, l;
{
	str->c.l -= l;
	memmove(str->c.s + loc, str->c.s + loc + l, str->c.l - loc + 1);
}

char *s_fget(str, stream)
	String *str;
	FILE *stream;
{
	static char buf[BLOCKSIZE + 1];
	int len;

	if (!fgets(buf, BLOCKSIZE, stream))
		return NULL;
	s_cpy(str, cstr_s(buf));
	len = strlen(buf);
	while (buf[len - 1] != '\n') {
		if (!fgets(buf, BLOCKSIZE + 1, stream))
			break;
		len = strlen(buf);
		s_cat(str, cstr_sl(buf, len));
	}
	return str->c.s;
}

/* Referenced string constructors */

Rstr *rstr_c(c)
	Cstr c;
{
	Rstr *new;

	new = New(Rstr);
	new->str.c = cstr_c(c);
	new->str.sz = c.l + 1;
	new->refs = 0;
	return new;
}

Rstr *rstr_rs(rs)
	Rstr *rs;
{
	Rstr *new;

	new = New(Rstr);
	if (rs->str.sz) {
		new->str.c.s = Newarray(char, rs->str.sz);
		memcpy(new->str.c.s, rs->str.c.s, rs->str.c.l + 1);
		new->str.c.l = rs->str.c.l;
	} else
		new->str = empty_string;
	new->str.sz = rs->str.sz;
	new->refs = 1;
	return new;
}

void dec_ref_rstr(rs)
	Rstr *rs;
{
	if (!--rs->refs) {
		s_free(&rs->str);
		Discard(rs, Rstr);
	}
}

/* Interpreter string operations */

Istr *istr_rs(rs)
	Rstr *rs;
{
	Istr *new;

	new = New(Istr);
	new->rs = rs;
	new->rs->refs++;
	new->refs = 0;
	return new;
}

/* Copy rstr contents of istr in preparation for modification */
void isolate(is)
	Istr *is;
{
	if (is->rs->refs > 1) {
		is->rs->refs--;
		is->rs = rstr_rs(is->rs);
	}
}

void dec_ref_istr(is)
	Istr *is;
{
	if (!--is->refs) {
		dec_ref_rstr(is->rs);
		Discard(is, Istr);
	}
}

