/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
/* Refs count, struct rstr * and prototypes added by GBH */
#define NSUBEXP	 10
typedef struct regexp {
    int refs;
    struct rstr *rs;
    char *startp[NSUBEXP];
    char *endp[NSUBEXP];
    char regstart;	/* Internal use only. */
    char reganch;	/* Internal use only. */
    char *regmust;	/* Internal use only. */
    int regmlen;	/* Internal use only. */
    char program[1];	/* Unwarranted chumminess with compiler. */
} regexp;

extern regexp *regcomp();
extern int regexec();
extern void regsub();
extern void regerror();

#ifdef PROTOTYPES
regexp *regcomp(char *);
int regexec(regexp *, char *);
#endif
