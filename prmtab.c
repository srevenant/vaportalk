/* This source code is in the public domain. */

/* prmtab.c: The primitive table */

#include "vt.h"

#define PTSIZE 201
#define NUM_OPER 24
#define NUM_PRMT (sizeof(prmtab) / sizeof(Prmt))

struct int_list {
	char *name;
	int num;
	struct int_list *next;
};

Dframe suspend_frame, frame_error;
extern Cframe *cstack;
extern int cpos, curprmt;

extern void
	op_bor(),
	op_bxor(),
	op_band(),
	op_eq(),
	op_ne(),
	op_lt(),
	op_le(),
	op_gt(),
	op_ge(),
	op_sl(),
	op_sr(),
	op_add(),
	op_sub(),
	op_mult(),
	op_div(),
	op_mod(),
	op_postinc(),
	op_postdec(),
	op_preinc(),
	op_predec(),
	op_not(),
	op_compl(),
	op_neg(),
	op_asn(),
	pr_write(),
	pr_cmove(),
	pr_scroll(),
	pr_scr_fwd(),
	pr_scr_rev(),
	pr_clrscr(),
	pr_clreol(),
	pr_bold(),
	pr_curs_reset(),
	pr_split(),
	pr_close(),
	pr_resize(),
	pr_set_termread(),
	pr_set_obj(),
	pr_obj(),
	pr_echo(),
	pr_cecho(),
	pr_display(),
	pr_read(),
	pr_reread(),
	pr_pass(),
	pr_win_top(),
	pr_win_bottom(),
	pr_win_col(),
	pr_win_rmt(),
	pr_win_termread(),
	pr_connect(),
	pr_connect_shell(),
	pr_disconnect(),
	pr_set_netread(),
	pr_set_promptread(),
	pr_set_back(),
	pr_set_busy(),
	pr_set_raw(),
	pr_send(),
	pr_input_waiting(),
	pr_rmt_addr(),
	pr_rmt_port(),
	pr_rmt_win(),
	pr_rmt_netread(),
	pr_rmt_promptread(),
	pr_rmt_back(),
	pr_rmt_busy(),
	pr_rmt_raw(),
	pr_rmt_echo(),
	pr_rmt_eor(),
	pr_insert(),
	pr_edfunc(),
	pr_getch(),
	pr_bind(),
	pr_unbind(),
	pr_find_key(),
	pr_key_seq(),
	pr_key_func(),
	pr_strcpy(),
	pr_strcat(),
	pr_strdup(),
	pr_strcmp(),
	pr_stricmp(),
	pr_strchr(),
	pr_strrchr(),
	pr_strcspn(),
	pr_strstr(),
	pr_stristr(),
	pr_strupr(),
	pr_strlwr(),
	pr_strlen(),
	pr_wrap(),
	pr_ucase(),
	pr_lcase(),
	pr_itoa(),
	pr_atoi(),
	pr_find_func(),
	pr_find_prmt(),
	pr_func_name(),
	pr_callv(),
	pr_detach(),
	pr_abort(),
	pr_alloc(),
	pr_new_assoc(),
	pr_lookup(),
	pr_acopy(),
	pr_base(),
	pr_garbage(),
	pr_parse(),
	pr_head(),
	pr_tail(),
	pr_next(),
	pr_prev(),
	pr_type(),
	pr_find_var(),
	pr_sleep(),
	pr_quit(),
	pr_rndseed(),
	pr_fopen(),
	pr_popen(),
	pr_fclose(),
	pr_fwrite(),
	pr_fread(),
	pr_fseek(),
	pr_ftell(),
	pr_fputc(),
	pr_fgetc(),
	pr_fflush(),
	pr_feof(),
	pr_fsize(),
	pr_fmtime(),
	pr_unlink(),
	pr_load_file(),
	pr_find_file(),
	pr_file_name(),
	pr_regcomp(),
	pr_regexec(),
	pr_regmatch(),
	pr_smatch(),
	pr_getenv(),
	pr_system(),
	pr_ctime();

/* The positions of the operators must correspond to the defined values
** in pnode.h */

Prmt prmtab[] = {
	/* Operators (oper.c) */
	{ "|"		, op_bor	,  2 },
	{ "^"		, op_bxor	,  2 },
	{ "&"		, op_band	,  2 },
	{ "=="		, op_eq		,  2 },
	{ "!="		, op_ne		,  2 },
	{ "<"		, op_lt		,  2 },
	{ "<="		, op_le		,  2 },
	{ ">"		, op_gt		,  2 },
	{ ">="		, op_ge		,  2 },
	{ "<<"		, op_sl		,  2 },
	{ ">>"		, op_sr		,  2 },
	{ "+"		, op_add	,  2 },
	{ "-"		, op_sub	,  2 },
	{ "*"		, op_mult	,  2 },
	{ "/"		, op_div	,  2 },
	{ "%"		, op_mod	,  2 },
	{ "++"		, op_postinc	,  1 },
	{ "--"		, op_postdec	,  1 },
	{ "++"		, op_preinc	,  1 },
	{ "--"		, op_predec	,  1 },
	{ "!"		, op_not	,  1 },
	{ "~"		, op_compl	,  1 },
	{ "-"		, op_neg	,  1 },
	{ "="		, op_asn	,  2 },
	/* Screen handling (prmt1.c) */
	{ "write"	, pr_write	, -1 },
	{ "cmove"	, pr_cmove	,  2 },
	{ "scroll"	, pr_scroll	,  2 },
	{ "scr_fwd"	, pr_scr_fwd	,  1 },
	{ "scr_rev"	, pr_scr_rev	,  1 },
	{ "clrscr"	, pr_clrscr	,  0 },
	{ "clreol"	, pr_clreol	,  0 },
	{ "bold"	, pr_bold	,  1 },
	{ "curs_reset"	, pr_curs_reset ,  0 },
	/* Windows (prmt1.c) */
	{ "split"	, pr_split	,  2 },
	{ "close"	, pr_close	,  2 },
	{ "resize"	, pr_resize	,  2 },
	{ "set_termread", pr_set_termread, 2 },
	{ "set_obj"	, pr_set_obj	,  2 },
	{ "obj"		, pr_obj	,  1 },
	{ "echo"	, pr_echo	, -1 },
	{ "display"	, pr_display	,  2 },
	{ "read"	, pr_read	, -1 },
	{ "reread"	, pr_reread	, -1 },
	{ "pass"	, pr_pass	,  2 },
	{ "win_top"	, pr_win_top	,  1 },
	{ "win_bottom"	, pr_win_bottom ,  1 },
	{ "win_col"	, pr_win_col	,  1 },
	{ "win_rmt"	, pr_win_rmt	,  1 },
	{ "win_termread", pr_win_termread, 1 },
	/* Remotes (prmt2.c) */
	{ "connect"	, pr_connect	,  2 },
	{ "connect_shell", pr_connect_shell, 1 },
	{ "disconnect"	, pr_disconnect ,  1 },
	{ "set_netread" , pr_set_netread,  2 },
	{ "set_promptread", pr_set_promptread, 2 },
	{ "set_back"	, pr_set_back	,  2 },
	{ "set_busy"	, pr_set_busy	,  2 },
	{ "set_raw"	, pr_set_raw	,  2 },
	{ "send"	, pr_send	, -1 },
	{ "input_waiting", pr_input_waiting,  1 },
	{ "rmt_addr"	, pr_rmt_addr	,  1 },
	{ "rmt_port"	, pr_rmt_port	,  1 },
	{ "rmt_win"	, pr_rmt_win	,  1 },
	{ "rmt_netread" , pr_rmt_netread,  1 },
	{ "rmt_promptread", pr_rmt_promptread, 1 },
	{ "rmt_back"	, pr_rmt_back	,  1 },
	{ "rmt_busy"	, pr_rmt_busy	,  1 },
	{ "rmt_raw"	, pr_rmt_raw	,  1 },
	{ "rmt_echo"	, pr_rmt_echo	,  1 },
	{ "rmt_eor"	, pr_rmt_eor	,  1 },
	/* Key buffer (prmt2.c) */
	{ "insert"	, pr_insert	,  1 },
	{ "edfunc"	, pr_edfunc	,  1 },
	{ "getch"	, pr_getch	, -1 },
	/* Keys (prmt2.c) */
	{ "bind"	, pr_bind	,  2 },
	{ "unbind"	, pr_unbind	,  1 },
	{ "find_key"	, pr_find_key	,  1 },
	{ "key_seq"	, pr_key_seq	,  1 },
	{ "key_func"	, pr_key_func	,  1 },
	/* String functions (prmt3.c) */
	{ "strcpy"	, pr_strcpy	, -1 },
	{ "strcat"	, pr_strcat	, -1 },
	{ "strdup"	, pr_strdup	,  1 },
	{ "strcmp"	, pr_strcmp	, -1 },
	{ "stricmp"	, pr_stricmp	, -1 },
	{ "strchr"	, pr_strchr	, -1 },
	{ "strrchr"	, pr_strrchr	, -1 },
	{ "strcspn"	, pr_strcspn	,  2 },
	{ "strstr"	, pr_strstr	,  2 },
	{ "stristr"	, pr_stristr	,  2 },
	{ "strupr"	, pr_strupr	,  1 },
	{ "strlwr"	, pr_strlwr	,  1 },
	{ "strlen"	, pr_strlen	,  1 },
	{ "wrap"	, pr_wrap	, -1 },
	{ "ucase"	, pr_ucase	,  1 },
	{ "lcase"	, pr_lcase	,  1 },
	{ "itoa"	, pr_itoa	,  1 },
	{ "atoi"	, pr_atoi	,  1 },
	/* Functions (prmt4.c) */
	{ "find_func"	, pr_find_func	,  1 },
	{ "find_prmt"	, pr_find_prmt	,  1 },
	{ "func_name"	, pr_func_name	,  1 },
	{ "callv"	, pr_callv	, -1 },
	{ "detach"	, pr_detach	, -1 },
	{ "abort"	, pr_abort	,  0 },
	/* Arrays (prmt4.c) */
	{ "alloc"	, pr_alloc	, -1 },
	{ "new_assoc"	, pr_new_assoc	,  0 },
	{ "lookup"	, pr_lookup	,  2 },
	{ "acopy"	, pr_acopy	,  3 },
	{ "base"	, pr_base	,  1 },
	{ "garbage"	, pr_garbage	,  0 },
	/* Misc (prmt4.c) */
	{ "parse"	, pr_parse	,  1 },
	{ "head"	, pr_head	,  1 },
	{ "tail"	, pr_tail	,  1 },
	{ "next"	, pr_next	,  1 },
	{ "prev"	, pr_prev	,  1 },
	{ "type"	, pr_type	,  1 },
	{ "find_var"	, pr_find_var	,  1 },
	{ "sleep"	, pr_sleep	,  1 },
	{ "quit"	, pr_quit	,  0 },
	{ "rndseed"	, pr_rndseed	,  1 },
	/* Files (prmt5.c) */
	{ "fopen"	, pr_fopen	,  2 },
	{ "popen"	, pr_popen	,  2 },
	{ "fclose"	, pr_fclose	,  1 },
	{ "fwrite"	, pr_fwrite	,  2 },
	{ "fread"	, pr_fread	,  1 },
	{ "fputc"	, pr_fputc	,  2 },
	{ "fseek"	, pr_fseek	,  3 },
	{ "ftell"	, pr_ftell	,  1 },
	{ "fgetc"	, pr_fgetc	,  1 },
	{ "fflush"	, pr_fflush	,  1 },
	{ "feof"	, pr_feof	,  1 },
	{ "fsize"	, pr_fsize	,  1 },
	{ "fmtime"	, pr_fmtime	,  1 },
	{ "unlink"	, pr_unlink	,  1 },
	{ "load_file"	, pr_load_file	,  1 },
	{ "find_file"	, pr_find_file	,  1 },
	{ "file_name"	, pr_file_name	,  1 },
	/* Regexps (prmt5.c) */
	{ "regcomp"	, pr_regcomp	,  1 },
	{ "regexec"	, pr_regexec	,  2 },
	{ "regmatch"	, pr_regmatch	,  2 },
	{ "smatch"	, pr_smatch	,  2 },
	/* Environment (prmt5.c) */
	{ "getenv"	, pr_getenv	,  1 },
	{ "system"	, pr_system	,  1 },
	{ "ctime"	, pr_ctime	,  1 },
};

static struct int_list *phtab[PTSIZE];
static struct int_list entries[NUM_PRMT - NUM_OPER];

void init_prmt()
{
	int i, ind;
	struct int_list *entry;

	for (entry = entries, i = NUM_OPER; i < NUM_PRMT; i++, entry++) {
		ind = hash(prmtab[i].name, PTSIZE);
		entry->name = prmtab[i].name;
		entry->num = i;
		entry->next = phtab[ind];
		phtab[ind] = entry;
	}
	suspend_frame.type = frame_error.type = F_EXCEPT;
	suspend_frame.Dval = INTERP_SUSPEND;
	frame_error.Dval = INTERP_ERROR;
}

int find_prmt(name)
	char *name;
{
	struct int_list *entry;

	for (entry = phtab[hash(name, PTSIZE)]; entry; entry = entry->next) {
		if (streq(name, entry->name))
			return entry->num;
	}
	return -1;
}

void finish_error()
{
	char *fname;

	fname = lookup_prog(cstack[cpos - 1].prog);
	if (fname)
		outputf(" in %s", fname);
	coutput("\n");
}

void type_errmsg()
{
	outputf("Error: Invalid arguments to primitive %s",
		prmtab[curprmt].name);
	finish_error();
}

void type_error(rf)
	Dframe *rf;
{
	*rf = frame_error;
	type_errmsg();
}

void bounds_error(rf)
	Dframe *rf;
{
	*rf = frame_error;
	outputf("Error: Out-of-bounds argument to primitive %s",
		prmtab[curprmt].name);
	finish_error();
}


