/* This source code is in the public domain. */

/* struct.h: Most of the program's data structures */

typedef struct dframe	Dframe;
typedef union  instr	Instr;
typedef struct prog	Prog;
typedef struct cframe	Cframe;
typedef struct estate	Estate;
typedef struct flist	Flist;
typedef struct ulist	Ulist;
typedef struct rmt	Rmt;
typedef struct win	Win;
typedef struct key	Key;
typedef struct file	File;
typedef struct unode	Unode;
typedef struct func	Func;
typedef struct array	Array;
typedef struct plist	Plist;
typedef struct prmt	Prmt;
typedef struct consttag Const;

/* First part: Interpreter data */

/* A stack frame, holds all possible value types at runtime */
struct dframe {
	int type;
	union {
		long val;
		void (*bobj)();
		Unode *unode;
		struct {
			Istr *is;
			int pos;
		} s;
		struct {
			Array *array;
			int pos;
		} a;
		Func *func;
		int pnum;
		regexp *reg;
		int *assoc;
		Plist *plist;
	} u;
};
#define F_INT		 0  /* Integer value	      val   */
#define F_PPTR		 1  /* Primitive pointer      pnum  */
#define F_BPTR		 2  /* Builtin pointer	      bobj  */
#define F_RMT		 3  /* Remote pointer	      unode */
#define F_WIN		 4  /* Window pointer	      unode */
#define F_KEY		 5  /* Key binding pointer    unode */
#define F_FILE		 6  /* File pointer	      unode */
#define F_SPTR		 7  /* String pointer	      s	    */
#define F_APTR		 8  /* Array pointer	      a	    */
#define F_FPTR		 9  /* Function pointer	      func  */
#define F_REG		10  /* Regexp		      reg   */
#define F_ASSOC		11  /* Association type	      assoc */
#define F_PLIST		12  /* Property list	      plist */
#define F_NULL		13  /* No value			    */
#define F_EXCEPT	14  /* Interpreter exception  val   */
#define Dval	u.val
#define Dbobj	u.bobj
#define Dunode	u.unode
#define Distr	u.s.is
#define Dspos	u.s.pos
#define Darray	u.a.array
#define Dapos	u.a.pos
#define Dfunc	u.func
#define Dpnum	u.pnum
#define Dreg	u.reg
#define Dassoc	u.assoc
#define Dplist	u.plist
#define Dset_elem(df, t, elem, val) if (1) { (df).type = t; \
					     (df).elem = (val); } else
#define Dset_int(df, n) Dset_elem(df, F_INT, Dval, n)
#define Dset_sptr(df, is, pos) if (1) { (df).type = F_SPTR; (df).Distr = (is); \
					(df).Dspos = (pos); } else
#define Dset_sptr_ref(df, istr, pos) if (1) { Dset_sptr(df, istr, pos); \
					     (df).Distr->refs = 1; } else
#define Dset_aptr(df, ar, pos) if (1) { (df).type = F_APTR; \
					(df).Darray = (ar); \
					(df).Dapos = (pos); } else

/* Types of interpreter exceptions */
#define INTERP_ERROR	0	/* Abort interpretation	   */
#define INTERP_SUSPEND	1	/* Exit interpreter	   */
#define INTERP_NOFREE	2	/* Don't fiddle with stack */

#define Utype	    u.unode->type
#define Sstr	    u.s.is->rs->str
#define Sbegin	    Sstr.c.s
#define Slen	    Sstr.c.l
#define Srstr	    u.s.is->rs
#define Srefs	    u.s.is->refs
#define Scstr(df)   (cstr_sl((df).Sbegin + (df).u.s.pos, (df).Slen - \
		     (df).u.s.pos))
#define Sastr(df)   ((df).Sbegin + (df).u.s.pos)
#define Avals	    u.a.array->vals
#define Asize	    u.a.array->alloc
#define Aelem(df)   ((df).Avals[(df).u.a.pos])

struct cframe {
	Prog *prog;
	Instr *instr;
	Array *avars;
	Array *lvars;
	int uargc, uargv;
	int dstart;
	int attached;
};

/* Snapshot of an execution state, to be stored on a stack or timer queue */
struct estate {
	int dframes;
	Dframe *dimage;
	int cframes;
	Cframe *cimage;
	Estate *next;
	time_t timer;
	Unode *win, *rmt;
	int wid, rid;
};

/* A primitive function or operator */
struct prmt {
	char *name;
	void (*func)();
	int argc;
};

/* Second part: user nodes
**
** User nodes can be destroyed by the user, in which case data
** frames which point to those nodes are reset to a value of F_NULL.
** The 'frefs' element of the unode structure is used to keep a list
** of data frames that refer to the unode. */

struct flist {
	Dframe *fr;
	Flist *next;
};

/* Hash tables use lists of user nodes */

struct ulist {
	Unode *un;
	Ulist *next;
};

struct rmt {
	char *addr;
	int port;
	int fd;
	char busy;
	char back;
	char raw;
	char inbuf;
	char disconnected;
	char iac;		/* Telnet state machine: received IAC */
	char intent;		/* Received WILL, WON'T, DO, or DON'T */
	char echo;		/* Echo mode */
	char eor;		/* End-of-record enabled */
	pid_t pid;
	String *buf;
	String *telnetbuf;
	Unode *win;
	Func *netread;
	Func *promptread;
	Estate *rstack;
	Dframe *obj;
};
#define Raddr		u.r.addr
#define Rport		u.r.port
#define Rfd		u.r.fd
#define Rbusy		u.r.busy
#define Rback		u.r.back
#define Rraw		u.r.raw
#define Rinbuf		u.r.inbuf
#define Rdisconnected	u.r.disconnected
#define Riac		u.r.iac
#define Rintent		u.r.intent
#define Recho		u.r.echo
#define Reor		u.r.eor
#define Rpid		u.r.pid
#define Rbuf		u.r.buf
#define Rtelnetbuf	u.r.telnetbuf
#define Rwin		u.r.win
#define Rnetread	u.r.netread
#define Rpromptread	u.r.promptread
#define Rrstack		u.r.rstack
#define Robj		u.r.obj

struct win {
	short top;
	short bot;
	short col;
	char nl;
	Unode *rmt;
	Func *termread;
	Estate *ghstack;
	Estate *glstack;
	Estate *rstack;
	Dframe *obj;
};
#define Wtop		u.w.top
#define Wbot		u.w.bot
#define Wcol		u.w.col
#define Wnl		u.w.nl
#define Wrmt		u.w.rmt
#define Wtermread	u.w.termread
#define Wghstack	u.w.ghstack
#define Wglstack	u.w.glstack
#define Wrstack		u.w.rstack
#define Wobj		u.w.obj

struct key {
	char *seq;
	int type;
	union {
		Func *cmd;
		int efunc;
	} ku;
};
#define K_CMD	0
#define K_EFUNC 1
#define Kseq	u.k.seq
#define Ktype	u.k.type
#define Kcmd	u.k.ku.cmd
#define Kefunc	u.k.ku.efunc

struct file {
	char *name;
	int type;
	FILE *fp;
};
#define FT_FILE 0
#define FT_PROC 1
#define Fname	u.f.name
#define Ftype	u.f.type
#define Ffp	u.f.fp

/* A double-linked ring node, may contain any of the above */
struct unode {
	union {
		Rmt r;
		Win w;
		Key k;
		File f;
	} u;
	short id;
	short dummy;
	Flist *frefs;
	struct unode *prev, *next;
};

struct array {
	Dframe *vals;
	union {
		int refs;
		Flist *fr;
		Array *next;
		Plist *plist;
	} r;
	int alloc;
	unsigned intern		: 1;
	unsigned fixed		: 1;
	unsigned gbit		: 1; /* Garbage bit */
	unsigned used		: 1;
	unsigned garbage	: 1;
	unsigned isp		: 1; /* Is plist */
};

struct plist {
	Array *array;
	int *assoc;
	int refs;
};

struct func {
	char *name;
	Prog *cmd;
	Func *left, *right;
};

/* Third part: Programs */

struct prog {
	Instr *code;
	int refs, avarc, lvarc, reqargs;
};

union instr {
	int type;
	long iconst;
	int tnum;
	int pnum;
	int argc;
	Func *func;
	struct {
		short pnum;
		short argc;
	} pc;
	Rstr *sconst;
	void (*bobj)();
	Instr *loc;
	/* Used in compiler */
	int offset;
	int sindex;
	char *ident;
};

/* These instruction types normally precede data.  The first number
** in the comment is the size of the complete instruction including
** the type.  When changing instruction types, change the width table
** in interp.c as well. */
#define I_ICONST     0	      /* 2 Integer constant		    */
#define I_SCONST     1	      /* 2 String constant		    */
#define I_PCALL	     2	      /* 2 Primitive call		    */
#define I_FCALL	     3	      /* 3 Function call		    */
#define I_EXEC	     4	      /* 2 Call by function pointer	    */
#define I_GVAR	     5	      /* 2 Var or function pointer	    */
#define I_BOBJ	     6	      /* 2 Built-in variable		    */
#define I_AVAR	     7	      /* 2 Argument variable		    */
#define I_LVAR	     8	      /* 2 Local variable		    */
#define I_PPTR	     9	      /* 2 Pointer to primitive		    */
#define I_FPTR	    10	      /* 2 Pointer to function		    */
#define I_JMPF	    11	      /* 2 Jump on false (data is offset)   */
#define I_JMPT	    12	      /* 2 Jump on true			    */
#define I_JMPPF	    13	      /* 2 Jump and preserve on true	    */
#define I_JMPPT	    14	      /* 2 Jump and preserve on false	    */
#define I_JMP	    15	      /* 2 Unconditional jump		    */
#define I_CVTB	    16	      /* 1 Convert top frame to boolean int */
#define I_EVAL	    17	      /* 1 Evaluate an lvalue		    */
#define I_NULL	    18	      /* 1 Push null value		    */
#define I_POP	    19	      /* 1 Explicit pop instruction	    */
#define I_DUP	    20	      /* 1 Duplicate top of stack	    */
#define I_STOP	    21	      /* 1 End of data or command reference */

/* Miscellaneous structures */

/* Constant table entry */
struct consttag {
	char *name;
	int val;
	Const *next;
};

