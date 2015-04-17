/* This source code is in the public domain. */

/* extern.h: Prototypes for external functions */

#ifdef PROTOTYPES

/* array.c */
void init_array(void);
void arfree(Array *);
Dframe *dfalloc(int);
Array *add_array(int, Dframe *, int, int, int);
void del_array(Array *);
void extend_array(Array *, int);
void dec_ref_array(Array *);
void dec_ref_plist(Plist *);
int lookup(int *, char *);
int garbage(void);

/* bobj.c */
void init_bobj(void);
void (*find_bobj(char *))();
void assign_bobj(Dframe *, void (*)(), Dframe *);

/* console.c */
void console(void);

/* const.c */
void init_const(void);
Const *find_const(char *);

/* func.c */
Func *find_func(char *);
Func *add_func(char *, Prog *);
void del_prog(Prog *);
void dec_ref_prog(Prog *);
char *lookup_prog(Prog *);

/* interp.c */
void init_interp(void);
void interp(void);
void double_dstack(void);
void cpush(Prog *, int, int, int);
void run_prog(Prog *);
void run_prog_istr(Prog *, Istr *, Unode *, Unode *);
Estate *suspend(int);
void discard_estate(Estate **);
void resume(Estate **, Dframe *);
void resume_istr(Estate **, Istr *);
void resume_int(Estate **, int);
void destroy_pipe(Estate *);
void break_pipe(Estate *);
void ref_frame(Dframe *);
void ref_frames(int, Dframe *);
void move_frame_refs(Dframe *, Dframe *);
void move_frames_refs(int, Dframe *, Dframe *);
void deref_frame(Dframe *);
void deref_frames(int, Dframe *);

/* key.c */
void init_key(void);
Unode *add_key_cmd(char *, Func *);
Unode *add_key_efunc(char *, int);
void del_key(Unode *);
Unode *find_key(char *);

/* keyboard.c */
void do_edit_func(int);
void process_incoming(char *);
void give_window(Unode *, Istr *);

/* main.c */
void add_ioqueue(Estate **, Estate *);

/* prmtab.c */
void init_prmt(void);
int find_prmt(char *);
void finish_error(void);
void type_errmsg(void);
void type_error(Dframe *);
void bounds_error(Dframe *);

/* remote.c */
void init_rmt(void);
Unode *new_rmt(char *, int);
Unode *new_rmt_shell(char *);
void disconnect(Unode *);
int io_check(long, long);
int transmit(Unode *, Cstr);
int input_waiting(int);
int process_remote_text(Unode *);
void give_remote(Unode *, Istr *, int);
void kill_children();

/* sconst.c */
Rstr *add_sconst(Cstr *);
void del_sconst(Rstr *);

/* signal.c */
void init_signals(void);
void winresize(void);
void stop(void);

/* string.c */
char *vtstrnchr(char *, int, int);
char *vtstrnrchr(char *, int, int);
int vtstrcspn(char *, char *);
int vtstricmp(char *, char *);
int vtstrnicmp(char *, char *, int);
char *vtstrstr(char *, char *);
char *vtstristr(char *, Cstr);
char *vtstrdup(char *);
void init_tables(void);
int hash(char *, int);
Cstr cstr_sl(char *, int);
Cstr cstr_s(char *s);
Cstr cstr_c(Cstr);
void init_wbufs(void);
void s_free(String *);
void lcheck(String *, int);
void s_add(String *, int);
void s_fadd(String *, int);
void s_nt(String *);
void s_term(String *, int);
void s_cpy(String *, Cstr);
void s_cat(String *, Cstr);
void s_insert(String *, Cstr, int);
void s_delete(String *, int, int);
char *s_fget(String *, FILE *);
Rstr *rstr_c(Cstr);
Rstr *rstr_rs(Rstr *);
void dec_ref_rstr(Rstr *);
Istr *istr_rs(Rstr *);
void isolate(Istr *);
void dec_ref_istr(Istr *);

/* telnet.c */
void telnet_state_machine(Unode *, int);

/* tmalloc.c */
char *tmalloc(size_t);
void tfree(char *, size_t);
char *trealloc(char *, size_t, size_t);

/* unode.c */
void init_unalloc(void);
Unode *unalloc(void);
void destroy_pointers(Flist *);
void discard_unode(Unode *);
void add_fref(Flist **, Dframe *);
void move_fref(Flist *, Dframe *, Dframe *);
void elim_fref(Flist **, Dframe *);

/* util.c */
void vterror(char *);
void vtdie(char *);
char *dmalloc(size_t);
char *drealloc(char *, size_t, size_t);
void dfree(char *, size_t);
void cleanup(void);
#ifndef vtmemset
void vtmemset(char *, int, int);
#endif
char *expand(char *);
char *itoa(int);
int smatch(char *, char *);

/* var.c */
int get_vindex(char *);
void write_vartab(FILE *);

/* window.c */
void bflushfunc(void);
void vtwrite(Cstr);
int getch(void);
void tty_mode(int);
void init_term(void);
void cmove(int, int);
void scroll(int, int);
void scr_fwd(int);
void scr_rev(int);
void curs_window(Unode *);
void curs_input(void);
void draw_divider(Unode *);
void toggle_imode(void);
void init_screen(void);
void redraw_screen(void);
void auto_redraw(void);
Unode *split_window(Unode *, int);
void close_window(Unode *, int);
void resize_window(Unode *, int);
void new_active_win(Unode *);
void resize_screen(int, int);
void input_puts(Cstr);
void input_cmove(int);
void input_bdel(int);
void input_fdel(int);
void input_clear(void);
void input_draw(void);
void input_newline(void);
void update_echo_mode(void);
void change_prompt(char *, int);
void output(Unode *, char *);
void coutput(Cstr);
#ifdef USE_STDARG
void outputf(char *, ...);
#else
void outputf(); /* Prototype doesn't work with varargs */
#endif
void operror(char *, Unode *);

/* vtc.y */
void init_compile(void);
void parse(char *);
char *expand(char *);
int load_file(char *);

#else /* PROTOTYPES */

Dframe *dfalloc();
Array *add_array();
void (*find_bobj())(), init_bobj(), assign_bobj();
char *lookup_prog(), *expand(), *kfunc_name(), *vtstrnchr(), *vtstrnrchr();
char *vtstrnichr(), *vtstrnrichr(), *vtstrstr(), *vtstrrstr(), *vtstristr();
char *vtstrristr(), *vtstrdup(), *s_fget(), *dmalloc(), *drealloc(), *itoa();
char *tmalloc(), *trealloc();
Const *find_const();
Func *find_func(), *add_func(), *functbl();
Estate *suspend();
Unode *add_key_cmd(), *add_key_efunc(), *find_key(), *find_rmt();
Unode *new_rmt(), *new_rmt_shell(), *split_window(), *unalloc();
Rstr *add_sconst(), *rstr_c(), *rstr_rs();
Istr *istr_rs();
Cstr cstr_sl(), cstr_s(), cstr_c();

void init_array(), arfree(), del_array(), extend_array(), dec_ref_array();
void dec_ref_plist(), console(), init_const(), init_func(), del_prog();
void dec_ref_prog(), init_interp(), interp(), double_dstack(), cpush();
void run_prog(), run_prog_istr(), discard_estate(), resume(), resume_istr();
void resume_int(), break_pipe(), destroy_pipe(), ref_frame(), ref_frames();
void move_frame_refs(), move_frames_refs(), deref_frame(), deref_frames();
void init_key(), del_key(), do_edit_func(), process_incoming(), list_func();
void give_window(), give_remote(), init_prmt(), finish_error(), type_errmsg();
void type_error(), bounds_error(), init_rmt(),  disconnect(), init_sconst();
void del_sconst(), init_signals(), init_tables(), init_wbufs(), s_free();
void lcheck(), s_add(), s_fadd(), s_nt(), s_term(), s_cpy(), s_cat();
void s_insert(), s_delete(), s_replace(), dec_ref_rstr(), isolate();
void dec_ref_istr(), telnet_state_machine(), init_unalloc();
void destroy_pointers(), discard_unode(), add_fref(), move_fref();
void elim_fref(), vterror(), vtdie(), dfree(), cleanup();
void init_var(), vtwrite(), tty_mode(), init_term(), cmove(), scroll();
void scr_fwd(), scr_rev(), curs_window(), curs_input(), draw_divider();
void toggle_imode(), init_screen(), redraw_screen(), bflushfunc(), scroll();
void scr_fwd(), scr_rev(), curs_window(), curs_input(), auto_redraw();
void close_window(), resize_window(), new_active_win(), resize_screen();
void input_puts(), input_cmove(), input_bdel(), input_fdel(), input_clear();
void input_draw(), input_newline(), change_prompt(), output(), coutput();
void outputf(), operror(), init_compile(), parse(), add_ioqueue(), stop();
void winresize(), tfree(), update_echo_mode(), kill_children();

#endif /* PROTOTYPES */

