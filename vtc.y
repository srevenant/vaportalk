/* This source code is in the public domain. */

%{
/* vtc.y: VTC compiler and lexical analyzer */

#include "vt.h"

String ptext;
int debug = 0;
extern int iwidth[];
#define ERR_PRMTNAME	"Primitive name used as function name"
#define ERR_RESERVED	"Reserved word used as identifier"
#define ERR_DUPGOTO	"Duplicate goto label"
#define ERR_MISBREAK	"Misplaced break statement"
#define ERR_MISCONT	"Misplaced continue statement"
#define ERR_DUPTAG	"Duplicate parameter or local variable name"
#define ERR_UDGOTO	"One or more goto labels not defined"
#define ERR_TOOMANY	"Too many parameters or local variables"
#define ERR_UNTERMCHAR	"Unterminated character constant"
#define ERR_UNTERMSTR	"Unterminated string constant"
#define ERR_BADBRACE	"Illegal placement of right brace"
#define ERR_ILLFUNC	"Illegal placement of reserved word func"

/* The intermediate program */
#define INIT_CODE 256
static Instr *icode;
static int loc = 0, codesize;
#define Check_code(n) Check(icode, Instr, codesize, loc + (n))

#define Hexdigitvalue(a) (isdigit(a) ? a - '0' : ucase(a) - 'A' + 10)
#define Code(t) if (1) { Check_code(1); icode[loc++].type = t; } else
#define Code2(t, elem, val) if (1) { Check_code(2); icode[loc++].type = t; \
				     icode[loc++].elem = val; } else
#define Code_iconst(val) Code2(I_ICONST, iconst, val)
#define Code_pcall(pn, ac) if (1) { Check_code(2); \
				    icode[loc++].type = I_PCALL; \
				    icode[loc].pc.pnum = pn; \
				    icode[loc++].pc.argc = ac; } else
#define Code_fcall(id, ac) if (1) { Check_code(3); \
				    icode[loc++].type = I_FCALL; \
				    icode[loc++].argc = ac; \
				    icode[loc++].ident = id; } else

typedef struct label {
	char *name;
	int offset;
	struct label *next;
} Label;

#define MAXILEN 32
static Label *lhead = NULL;

#ifdef PROTOTYPES
static void code_var(char *);
static void code_goto(char *, int);
static void free_labels(void);
static void add_tag(char **, int *, char *);
static int find_tag(char **, int, char *);
static char *parse_token(char *);
static int read_char(char **);
static int enter_sconst(Cstr);
static char *read_string(char *);
static char *read_comment(char *);
static void compile(void);
static void scan(void);
static void cleanup_parse();
static void lexerror(char *);
static void yyerror(char *);
static int yylex(void);
#else
static void compile(), scan(), cleanup_parse(), lexerror(), yyerror();
static void code_var(), free_labels(), add_tag(), code_goto();
static char *parse_token(), *read_string(), *read_comment();
#endif

#define INIT_JMP 64
#define INIT_COND 16
int *jmptab, jmpsize, jmppos;
int *condstack, condsize, condpos;
int *loopstack, loopsize, looppos;
#define Incjmp(n)	Check(jmptab, int, jmpsize, jmppos += (n))
#define Pushcond(x)	Push(condstack, int, condsize, condpos, x)
#define Pushloop(x)	Push(loopstack, int, loopsize, looppos, x)
#define Peekloop	(loopstack[looppos - 1])
#define Incloop(n)	if (1) { Pushloop(jmppos); Incjmp(n); } else
#define Decloop		looppos--
#define Ljmp(l, t)	Code2(t, offset, Peekloop + (l))
#define Ldest(l)	(jmptab[Peekloop + (l)] = loc)
#define Break		if (looppos) Code2(I_JMP, offset, Peekloop + 1); \
			else yyerror(ERR_MISBREAK)
#define Continue	if (looppos) Code2(I_JMP, offset, Peekloop); \
			else yyerror(ERR_MISCONT)
#define Jmp(t)		if (1) { Pushcond(jmppos); Code2(t, offset, jmppos); \
				 Incjmp(1); } else
#define Dest		(jmptab[condstack[--condpos]] = loc)
#define Destnext	(jmptab[condstack[--condpos]] = loc + 2)
#define Return		Code2(I_JMP, offset, 0)

/* Local vars and array vars */
#define MAX_TAGS 32
static char *lvars[MAX_TAGS], *avars[MAX_TAGS];
static int lvarc = 0, avarc = 0, reqargs = 0;

static int lexline = -1;		/* Line being analyzed	     */
static int parseline = -1;		/* Line being parsed	     */
static int errflag;			/* Error flag		     */
static char *curfunc = "";		/* Name of current function  */
static char *curfile = "";		/* Name of current file	     */
extern Prmt prmtab[];

#define OP_BOR	     0
#define OP_BXOR	     1
#define OP_BAND	     2
#define OP_EQ	     3
#define OP_NE	     4
#define OP_LT	     5
#define OP_LE	     6
#define OP_GT	     7
#define OP_GE	     8
#define OP_SL	     9
#define OP_SR	    10
#define OP_ADD	    11
#define OP_SUB	    12
#define OP_MULT	    13
#define OP_DIV	    14
#define OP_MOD	    15
#define OP_POSTINC  16
#define OP_POSTDEC  17
#define OP_PREINC   18
#define OP_PREDEC   19
#define OP_NOT	    20
#define OP_COMPL    21
#define OP_NEG	    22
#define OP_ASN	    23
#define PR_LOOKUP   103

%}
%union {
	int num;
	int index;
	void (*bobj)();
	int str;
	char *s;
}
%token <num>	ICONST
%token <str>	SCONST
%token <bobj>	BOBJ
%token <s>	IDENT
%token		FUNC FASSIGN DO WHILE IF ELSE FOR GOTO BREAK CONTINUE RETURN
%type  <num>	exprv args asn_op
%type  <s>	unreserved
%left ';'
%left ','
%right '=' TA DA MA AA SA SLA SRA BAA BXA BOA CONDA
%right '?' ':'
%right DEREF
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left EQ NE
%left '<' LE '>' GE
%left SL SR
%left '+' '-'
%left '*' '/' '%'
%right '!' '~' INC DEC
%left '(' ')' '[' ']'

%%

directive	: fdecl FASSIGN expr
		| fdecl tdecl compound	{ Code(I_NULL); }
		| tdecl command		{ Code(I_NULL); }
		;

command : stmt
	| command stmt
	;

fdecl	: FUNC unreserved adecl		{ curfunc = $2;
					  if (find_prmt($2) != -1)
						  yyerror(ERR_PRMTNAME); }
	;

adecl	: '(' optargs ')'
	| '(' params { reqargs = avarc; } optargs ')'
	;

tdecl	: /* nothing */
	| '[' ']'
	| '[' locals ']'
	;

optargs : /* nothing */
	| '/' params
	;

params	: unreserved			{ add_tag(avars, &avarc, $1); }
	| params ',' unreserved		{ add_tag(avars, &avarc, $3); }
	;

locals	: unreserved			{ add_tag(lvars, &lvarc, $1); }
	| locals ',' unreserved		{ add_tag(lvars, &lvarc, $3); }
	;

post	: postlval			{ Code(I_EVAL); }
	| postfix
	;

postlval: IDENT				{ code_var($1); }
	| BOBJ				{ Code2(I_BOBJ, bobj, $1); }
	| post DEREF IDENT		{ Code2(I_SCONST, sindex,
						enter_sconst(cstr_s($3)));
					  Code_pcall(PR_LOOKUP, 2); }
	| post '[' expr ']'		{ Code_pcall(OP_ADD, 2); }
	| post '[' ICONST ']'		{ if ($3) { Code_iconst($3);
						    Code_pcall(OP_ADD, 2); } }
	| '(' lval ')'
	;

lval	: postlval
	| '*' unary
	;

atom	: ICONST			{ Code_iconst($1); }
	| SCONST			{ Code2(I_SCONST, sindex, $1); }
	| '.' unreserved		{ int ind = find_prmt($2);
					  if (ind != -1)
					       Code2(I_PPTR, pnum, ind);
					  else Code2(I_FPTR, ident, $2); }
	| IDENT '(' args ')'		{ int ind = find_prmt($1);
					  if (ind != -1)
					       Code_pcall(ind, $3);
					  else Code_fcall($1, $3); }
	| '(' lval ')' '(' args ')'	{ Code2(I_EXEC, argc, $5); }
	| '(' expr ')'
	;

/* Can occur before [] */
postfix : atom
	| postlval INC			{ Code_pcall(OP_POSTINC, 1); }
	| postlval DEC			{ Code_pcall(OP_POSTDEC, 1); }
	;

unary	: postfix
	| lval				{ Code(I_EVAL); }
	| INC lval			{ Code_pcall(OP_PREINC, 1); }
	| DEC lval			{ Code_pcall(OP_PREDEC, 1); }
	| '!' unary			{ Code_pcall(OP_NOT, 1); }
	| '~' unary			{ Code_pcall(OP_COMPL, 1); }
	| '-' unary			{ Code_pcall(OP_NEG, 1); }
	| '+' unary
	| '&' lval
	;

binary	: unary
	| binary OR			{ Code(I_CVTB); Jmp(I_JMPPT); }
	  binary			{ Code(I_CVTB); Dest; }
	| binary AND			{ Code(I_CVTB); Jmp(I_JMPPF); }
	  binary			{ Code(I_CVTB); Dest; }
	| binary '|' binary		{ Code_pcall(OP_BOR, 2); }
	| binary '^' binary		{ Code_pcall(OP_BXOR, 2); }
	| binary '&' binary		{ Code_pcall(OP_BAND, 2); }
	| binary EQ binary		{ Code_pcall(OP_EQ, 2); }
	| binary NE binary		{ Code_pcall(OP_NE, 2); }
	| binary '<' binary		{ Code_pcall(OP_LT, 2); }
	| binary LE binary		{ Code_pcall(OP_LE, 2); }
	| binary '>' binary		{ Code_pcall(OP_GT, 2); }
	| binary GE binary		{ Code_pcall(OP_GE, 2); }
	| binary SL binary		{ Code_pcall(OP_SL, 2); }
	| binary SR binary		{ Code_pcall(OP_SR, 2); }
	| binary '+' binary		{ Code_pcall(OP_ADD, 2); }
	| binary '-' binary		{ Code_pcall(OP_SUB, 2); }
	| binary '*' binary		{ Code_pcall(OP_MULT, 2); }
	| binary '/' binary		{ Code_pcall(OP_DIV, 2); }
	| binary '%' binary		{ Code_pcall(OP_MOD, 2); }
	| binary '?'			{ Jmp(I_JMPF); }
	  binary ':'			{ Destnext; Jmp(I_JMP); }
	  binary			{ Dest; }
	| binary '?' ':' { Jmp(I_JMPPT); } binary { Dest; }
	;

assign	: binary
	| lval '=' assign		{ Code_pcall(OP_ASN, 2); }
	| lval asn_op			{ Code(I_DUP); Code(I_EVAL); }
	  assign			{ Code_pcall($2, 2);
					  Code_pcall(OP_ASN, 2); }
	| lval CONDA			{ Code(I_DUP); Code(I_EVAL);
					  Jmp(I_JMPPT); }
	  assign			{ Dest; Code_pcall(OP_ASN, 2); }
	;

asn_op	: TA	{ $$ = OP_MULT; }	| DA	{ $$ = OP_DIV; }
	| MA	{ $$ = OP_MOD; }	| AA	{ $$ = OP_ADD; }
	| SA	{ $$ = OP_SUB; }	| SLA	{ $$ = OP_SL; }
	| SRA	{ $$ = OP_SR; }		| BAA	{ $$ = OP_BAND; }
	| BXA	{ $$ = OP_BXOR; }	| BOA	{ $$ = OP_BOR; }
	;

/* Expression, value equal to last assignment-expression */
expr	: assign
	| expr ',' { Code(I_POP); } assign
	;

/* List of assignment-expressions, retaining values (arg list) */
exprv	: assign			{ $$ = 1; }
	| exprv ',' assign		{ $$ = $1 + 1; }
	;

args	: /* nothing */			{ $$ = 0; }
	| exprv
	;

/* Optional expression, no value */
oexprn	: /* nothing */
	| expr				{ Code(I_POP); }
	;

/* Optional expression with value */
oexprv	: /* nothing */			{ Code_iconst(1); }
	| expr
	;

compound: '{' stmtlist '}' ;

stmtlist: /* nothing */
	| stmtlist stmt
	;

ifcond	: IF '(' expr ')'		{ Jmp(I_JMPF); } ;

stmt	: compound
	| oexprn ';'
	| ifcond stmt			{ Dest; }
	| ifcond stmt ELSE { Destnext; Jmp(I_JMP); } stmt { Dest; }
	| WHILE				{ Incloop(2); Ldest(0); }
	  '(' expr ')'			{ Ljmp(1, I_JMPF); }
	  stmt				{ Ljmp(0, I_JMP); Ldest(1); Decloop; }
	| DO				{ Incloop(3); Ldest(2); }
	  stmt				{ Ldest(0); }
	  WHILE '(' expr ')'		{ Ljmp(2, I_JMPT); Ldest(1); Decloop; }
	| FOR '(' oexprn ';'		{ Incloop(4); Ldest(3); }
	  oexprv ';'			{ Ljmp(1, I_JMPF); Ljmp(2, I_JMP);
					  Ldest(0); }
	  oexprn ')'			{ Ljmp(3, I_JMP); Ldest(2); }
	  stmt				{ Ljmp(0, I_JMP); Ldest(1); Decloop; }
	| IDENT ':' { code_goto($1, 0); } stmt
	| GOTO IDENT ';'		{ code_goto($2, 1); }
	| BREAK ';'			{ Break; }
	| CONTINUE ';'			{ Continue; }
	| RETURN ';'			{ Code(I_NULL); Return; }
	| RETURN expr ';'		{ Return; }
	;

/* Error if identifier is reserved */
unreserved	: IDENT
		| reserved		{ yyerror(ERR_RESERVED); $$ = ""; }
		;

reserved	: BOBJ			{ ; }
		| ICONST		{ ; }
		| DO | WHILE | IF | ELSE | FOR | FUNC | GOTO
		| BREAK | CONTINUE | RETURN ;

%%

/* Auxiliary compiler functions */

static void code_var(ident)
	char *ident;
{
	int tag;

	if ((tag = find_tag(avars, avarc, ident)) != -1)
		Code2(I_AVAR, tnum, tag);
	else if ((tag = find_tag(lvars, lvarc, ident)) != -1)
		Code2(I_LVAR, tnum, tag);
	else
		Code2(I_GVAR, ident, ident);
}

static void code_goto(name, to)
	char *name;
	int to;
{
	Label *label;

	for (label = lhead; label; label = label->next) {
		if (streq(label->name, name)) {
			if (to)
				Code2(I_JMP, offset, label->offset);
			else if (jmptab[label->offset] == -1)
				jmptab[label->offset] = loc;
			else
				yyerror(ERR_DUPGOTO);
			return;
		}
	}
	label = New(Label);
	label->name = name;
	label->offset = jmppos;
	Push(jmptab, int, jmpsize, jmppos, to ? -1 : loc);
	if (to)
		Code2(I_JMP, offset, label->offset);
	label->next = lhead;
	lhead = label;
}

static void free_labels()
{
	Label *label = lhead, *next;

	for (label = lhead; label; label = next) {
		next = label->next;
		if (jmptab[label->offset] == -1)
			yyerror(ERR_UDGOTO);
		Discard(label, Label);
	}
	lhead = NULL;
}

static void add_tag(table, pos, tag)
	char *table[], *tag;
	int *pos;
{
	int i;

	for (i = 0; i < *pos; i++) {
		if (!strcmp(table[i], tag)) {
			yyerror(ERR_DUPTAG);
			return;
		}
	}
	if (*pos == MAX_TAGS)
		yyerror(ERR_TOOMANY);
	else
		table[(*pos)++] = tag;
}

static int find_tag(table, num, ident)
	char *table[], *ident;
	int num;
{
	int i;

	for (i = 0; i < num; i++) {
		if (streq(table[i], ident))
			return i;
	}
	return -1;
}

/* The lexical analyzer */

typedef struct token {
	int type;
	YYSTYPE val;
} Token;

Token *tokbuf;
int tokpos = 0, toksize;
#define INIT_TOKENS 128

#define Textend Checkinc(tokbuf, Token, toksize, tokpos)
#define Add_vtoken(t, elem, v) if (1) { Textend; tokbuf[tokpos].type = (t); \
					tokbuf[tokpos++].val.elem = (v); } else
#define Add_ntoken(t) if (1) { Textend; tokbuf[tokpos++].type = (t); } else

#define INIT_SCONST 32
#define INIT_IDENTS 64
static Cstr *sconsts;			/* Allocated string constants */
static char **idents;			/* Allocated identifiers */
static int sconstpos = 0, identpos = 0, sconstsize, identsize;
static int braces = 0, incomment = 0, instring = 0, infunc = 0, tokindex;
static String sbuf;

static char *ecodes = "ntvbrf\\'\"";
static char *results = "\n\t\v\b\r\f\\'\"";
static int begin[128], end[128];	/* Lookup table for words */
static char transtab[128];		/* Table for escape codes */
/* Table of reserved words and compound operators */
/* These do not need to be in alphabetical order, but words with the
** same first character must be adjacent.  Among compound operators
** with the same first character, longer operators should come first. */
static struct word {
	char *s;
	int val;
} words[] = {
	{ ""		, 0		},
	{ "do"		, DO		},
	{ "while"	, WHILE		},
	{ "if"		, IF		},
	{ "else"	, ELSE		},
	{ "for"		, FOR		},
	{ "func"	, FUNC		},
	{ "goto"	, GOTO		},
	{ "break"	, BREAK		},
	{ "continue"	, CONTINUE	},
	{ "return"	, RETURN	},
	{ "^="		, BXA		},
	{ "*="		, TA		},
	{ "/="		, DA		},
	{ "%="		, MA		},
	{ "+="		, AA		},
	{ "++"		, INC		},
	{ "-="		, SA		},
	{ "-->"		, FASSIGN	},
	{ "--"		, DEC		},
	{ "->"		, DEREF		},
	{ "<<="		, SLA		},
	{ "<<"		, SL		},
	{ "<="		, LE		},
	{ ">>="		, SRA		},
	{ ">>"		, SR		},
	{ ">="		, GE		},
	{ "&="		, BAA		},
	{ "&&"		, AND		},
	{ "|="		, BOA		},
	{ "||"		, OR		},
	{ "=="		, EQ		},
	{ "!="		, NE		},
	{ "?:="		, CONDA		},
	{ ""		, 0		}
};

#define NWORDS (sizeof(words) / sizeof(struct word) - 1)

/* Set up data */
void init_compile()
{
	int i;

	memset(begin, 0, sizeof(begin));
	memset(end, 0, sizeof(end));
	for (i = 0; i < NWORDS; i++) {
		begin[*words[i].s] = i;
		while (*words[i + 1].s == *words[i].s)
			i++;
		end[*words[i].s] = i;
	}
	memset(transtab, 0, sizeof(transtab));
	for (i = 0; ecodes[i]; i++)
		transtab[ecodes[i]] = results[i];
	tokbuf = Newarray(Token, toksize = INIT_TOKENS);
	icode = Newarray(Instr, codesize = INIT_CODE);
	sconsts = Newarray(Cstr, sconstsize = INIT_SCONST);
	idents = Newarray(char *, identsize = INIT_IDENTS);
	jmptab = Newarray(int, jmpsize = INIT_JMP);
	condstack = Newarray(int, condsize = INIT_COND);
	loopstack = Newarray(int, loopsize = INIT_COND);
	sbuf = empty_string;
}

/* Analyze text and place tokens in buffer */

void parse(text)
	char *text;
{
	while (*text) {
		text = parse_token(text);
		if (!text)
			return;
	}
	if (!infunc && !braces && !incomment && !instring && tokpos)
		compile();
}

static char *parse_token(s)
	char *s;
{
	char *ptr, buf[MAXILEN + 1];
	int i;
	void (*bobj)();
	Const *cp;

	if (incomment)
		return read_comment(s);
	if (instring)
		return read_string(s);
	for (; isspace(*s); s++);
	if (!*s || *s == '/' && s[1] == '/')
		return "";
	if (*s == '/' && s[1] == '*')
		return read_comment(s + 2);
	if (*s == '"') {
		s_term(&sbuf, 0);
		return read_string(s + 1);
	}
	if (*s == '\'') {
		s++;
		Add_vtoken(ICONST, num, read_char(&s));
		if (*s++ != '\'')
			lexerror(ERR_UNTERMCHAR);
		return s;
	}
	if (isdigit(*s)) {
		i = 0;
		if (*s == '0' && lcase(s[1]) == 'x' && isxdigit(s[2])) {
			s++;
			while (isxdigit(*++s))
				i = i * 0x10 + Hexdigitvalue(*s);
			Add_vtoken(ICONST, num, i);
			return s;
		}
		if (*s == '0' && s[1] >= '0' && s[1] <= '8') {
			while (*++s >= '0' && *s <= '8')
				i = i * 010 + *s - '0';
			Add_vtoken(ICONST, num, i);
			return s;
		}
		Add_vtoken(ICONST, num, atoi(s));
		while (isdigit(*++s));
		return s;
	}
	if (*s == '\\' && !s[1])
		return NULL;
	if (isalpha(*s) || *s == '_') {
		for (ptr = s; isalnum(*ptr) || *ptr == '_'; ptr++);
		strncpy(buf, s, min(ptr - s, MAXILEN));
		buf[min(ptr - s, MAXILEN)] = '\0';
		if (!(*buf & ~0x7f) && begin[*buf]) {
			for (i = begin[*buf]; i <= end[*buf]; i++) {
				if (streq(buf + 1, words[i].s + 1))
					break;
			}
			if (i <= end[*buf]) {
				if (words[i].val == FUNC) {
					if (tokpos) {
						lexerror(ERR_ILLFUNC);
						braces = 0;
					}
					infunc = 1;
				}
				Add_ntoken(words[i].val);
				return ptr;
			}
		}
		bobj = find_bobj(buf);
		if (bobj) {
			Add_vtoken(BOBJ, bobj, bobj);
			return ptr;
		}
		cp = find_const(buf);
		if (cp) {
			Add_vtoken(ICONST, num, cp->val);
			return ptr;
		}
		Push(idents, char *, identsize, identpos, vtstrdup(buf));
		Add_vtoken(IDENT, s, idents[identpos - 1]);
		return ptr;
	}
	if (!(*s & ~0x7f) && begin[*s]) {
		for (i = begin[*s]; i <= end[*s]; i++) {
			if (!strncmp(s + 1, words[i].s + 1,
				     strlen(words[i].s) - 1))
				break;
		}
		if (i <= end[*s]) {
			Add_ntoken(words[i].val);
			if (words[i].val == FASSIGN)
				infunc = 0;
			return s + strlen(words[i].s);
		}
	}
	Add_ntoken(*s);
	if (*s == '{')
		braces++;
	else if (*s == '}') {
		if (!braces)
			lexerror(ERR_BADBRACE);
		else if (!--braces)
			infunc = 0;
	}
	return s + 1;
}

static int read_char(s)
	char **s;
{
	char c;
	int val = 0, count;

	if (**s != '\\')
		return *(*s)++;
	c = transtab[*++(*s) & 0x7f];
	if (c) {
		(*s)++;
		return c;
	}
	if (**s < '0' || **s > '8')
		return '\\';
	for (count = 0; count < 3 && **s >= '0' && **s <= '8'; (*s)++)
		val = val * 8 + **s - '0';
	return val;
}

static int enter_sconst(c)
	Cstr c;
{
	Push(sconsts, Cstr, sconstsize, sconstpos, cstr_c(c));
	return sconstpos - 1;
}

static char *read_string(s)
	char *s;
{
	while (*s && *s != '"') {
		if (*s == '\\' && !s[1]) {
			instring = 1;
			return "";
		}
		s_fadd(&sbuf, read_char(&s));
	}
	if (!*s) {
		lexerror(ERR_UNTERMSTR);
		return "";
	}
	instring = 0;
	Add_vtoken(SCONST, str, enter_sconst(sbuf.c));
	return s + 1;
}

static char *read_comment(s)
	char *s;
{
	s = strchr(s, '*');
	while (s) {
		if (s[1] == '/') {
			incomment = 0;
			return s + 2;
		}
		s = strchr(s + 1, '*');
	}
	incomment = 1;
	return "";
}

static void compile()
{
	tokindex = 0;
	yyparse();
	if (!errflag) {
		scan();
		return;
	}
	while (yylex());
	condpos = looppos = 0;
	cleanup_parse();
}

static void scan()
{
	Prog *newprog;
	Instr *in, *end;
	char *s;
	Func *func;
	String dtext;

	dtext = empty_string;
	jmptab[0] = loc;
	Code(I_STOP);
	newprog = New(Prog);
	newprog->refs = 0;
	newprog->avarc = avarc;
	newprog->reqargs = reqargs;
	newprog->lvarc = lvarc;
	newprog->code = Newarray(Instr, loc);
	Copy(icode, newprog->code, loc, Instr);
	end = newprog->code + loc;
	for (in = newprog->code; in < end; in += iwidth[in->type]) {
		switch(in->type) {
		    case I_SCONST:
			in[1].sconst = add_sconst(&sconsts[in[1].sindex]);
		    Case I_FCALL:
			s = in[2].ident;
			func = find_func(s);
			if (debug && (!func || !func->cmd)
			    && !strstr(dtext.c.s, s)) {
				s_acat(&dtext, " ");
				s_acat(&dtext, s);
				s_acat(&dtext, "()");
			}
			in[2].func = func ? func : add_func(s, NULL);
		    Case I_FPTR:
			s = in[1].ident;
			func = find_func(s);
			if (debug && (!func || !func->cmd)
			    && !strstr(dtext.c.s, s)) {
				s_acat(&dtext, " .");
				s_acat(&dtext, s);
			}
			in[1].func = func ? func : add_func(s, NULL);
		    Case I_GVAR:
			if (debug && !strstr(dtext.c.s, in[1].ident)) {
				s_acat(&dtext, " ");
				s_acat(&dtext, in[1].ident);
			}
			in[1].tnum = get_vindex(in[1].ident);
		    Case I_JMP:
		    case I_JMPT:
		    case I_JMPF:
		    case I_JMPPT:
		    case I_JMPPF:
			in[1].loc = newprog->code + jmptab[in[1].offset];
		}
	}
	if (debug && *dtext.c.s) {
		if (*curfile)
			outputf("%s ", curfile);
		outputf("%s:%s\n", *curfunc ? curfunc : "Command",
			dtext.c.s);
	}
	s_free(&dtext);
	if (*curfunc) {
		add_func(curfunc, newprog);
		cleanup_parse();
	} else {
		cleanup_parse();
		run_prog(newprog);
	}
}

static void cleanup_parse()
{
	while (identpos--)
		Discardstring(idents[identpos]);
	while (sconstpos--) {
		if (sconsts[sconstpos].s)
			Discardarray(sconsts[sconstpos].s, char,
				     sconsts[sconstpos].l + 1);
	}
	free_labels();
	curfunc = "";
	tokpos = loc = lvarc = avarc = errflag = identpos = sconstpos = 0;
	reqargs = 0;
	jmppos = 1;
}

static void lexerror(s)
	char *s;
{
	if (lexline != -1)
		outputf("%s line %d: ", curfile, lexline);
	outputf("%s\n", s);
	errflag = 1;
}

static void yyerror(msg)
	char *msg;
{
	if (parseline != -1)
		outputf("%s line %d: ", curfile, parseline);
	coutput(msg);
	if (*curfunc)
		outputf(" in function %s", curfunc);
	coutput("\n");
	errflag = 1;
}

static int yylex()
{
	while (tokindex < tokpos && tokbuf[tokindex].type == '\n') {
		tokindex++;
		if (parseline != -1)
			parseline++;
	}
	if (tokindex < tokpos) {
		yylval = tokbuf[tokindex].val;
		return tokbuf[tokindex++].type;
	}
	return 0;
}

int load_file(name)
	char *name;
{
	int olline, opline;
	char *ocfile;
	FILE *fp;
	String freader;

	freader = empty_string;
	fp = fopen(expand(name), "r");
	if (!fp)
		return -1;
	olline = lexline;
	opline = parseline;
	ocfile = curfile;
	lexline = parseline = 1;
	curfile = vtstrdup(name);
	while (s_fget(&freader, fp)) {
		if (freader.c.s[freader.c.l - 1] == '\n')
			freader.c.s[freader.c.l - 1] = '\0';
		parse(freader.c.s);
		if (tokpos)
			Add_ntoken('\n');
		else
			parseline++;
		lexline++;
	}
	lexline = olline;
	parseline = opline;
	Discardstring(curfile);
	curfile = ocfile;
	fclose(fp);
	s_free(&freader);
	return 0;
}

