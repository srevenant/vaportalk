/* This source code is in the public domain. */

/* func.c: Maintains the function table */

#include "vt.h"

#ifdef PROTOTYPES
static char *find_prog(Func *, Prog *);
#else
static char *find_prog();
#endif

#define FTSIZE 203

/* Functions are hashed to unordered chains */

static Func *ftab[FTSIZE];
extern int iwidth[];

Func *find_func(name)
	char *name;
{
	Func *fp;
	int val;

	fp = ftab[hash(name, FTSIZE)];
	while (fp) {
		val = strcmp(name, fp->name);
		if (!val)
			return fp;
		fp = val < 0 ? fp->left : fp->right;
	}
	return NULL;
}

Func *add_func(name, cmd)
	char *name;
	Prog *cmd;
{
	Func *new, **fp;

	if (cmd)
		cmd->refs++;
	new = find_func(name);
	if (new) {
		if (new->cmd)
			dec_ref_prog(new->cmd);
		new->cmd = cmd;
		return new;
	}
	new = New(Func);
	new->name = vtstrdup(name);
	new->cmd = cmd;
	new->left = new->right = NULL;
	fp = &ftab[hash(name, FTSIZE)];
	while (*fp)
		fp = strcmp(name, (*fp)->name) < 0
		     ? &(*fp)->left : &(*fp)->right;
	*fp = new;
	return new;
}

void dec_ref_prog(prog)
	Prog *prog;
{
	if (!--prog->refs)
		del_prog(prog);
}

void del_prog(prog)
	Prog *prog;
{
	Instr *ip;

	for (ip = prog->code; ip->type != I_STOP; ip += iwidth[ip->type]) {
		if (ip->type == I_SCONST)
			del_sconst(ip[1].sconst);
	}
	Discardarray(prog->code, Instr, ip - prog->code + 1);
	Discard(prog, Prog);
}

/* Look up a program name.  Slow. */
char *lookup_prog(prog)
	Prog *prog;
{
	int i;
	char *name = NULL;

	for (i = 0; !name && i < FTSIZE; i++)
		name = find_prog(ftab[i], prog);
	return name;
}

static char *find_prog(tree, prog)
	Func *tree;
	Prog *prog;
{
	char *name;

	if (!tree)
		return NULL;
	if (tree->cmd == prog)
		return tree->name;
	name = find_prog(tree->left, prog);
	if (name)
		return name;
	return find_prog(tree->right, prog);
}

