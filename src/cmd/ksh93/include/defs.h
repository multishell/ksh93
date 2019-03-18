/*******************************************************************
*                                                                  *
*             This software is part of the ast package             *
*                Copyright (c) 1982-2002 AT&T Corp.                *
*        and it may only be used by you under license from         *
*                       AT&T Corp. ("AT&T")                        *
*         A copy of the Source Code Agreement is available         *
*                at the AT&T Internet web site URL                 *
*                                                                  *
*       http://www.research.att.com/sw/license/ast-open.html       *
*                                                                  *
*    If you have copied or used this software without agreeing     *
*        to the terms of the license you are infringing on         *
*           the license and copyright and are violating            *
*               AT&T's intellectual property rights.               *
*                                                                  *
*            Information and Software Systems Research             *
*                        AT&T Labs Research                        *
*                         Florham Park NJ                          *
*                                                                  *
*                David Korn <dgk@research.att.com>                 *
*                                                                  *
*******************************************************************/
#pragma prototyped
/*
 * David Korn
 * AT&T Labs
 *
 * Shell interface private definitions
 *
 */

#include	<ast.h>
#include	<sfio.h>
#include	<error.h>
#include	"FEATURE/options"
#include	<cdt.h>
#include	<history.h>
#include	<env.h>
#include	"fault.h"
#include	"argnod.h"

/*
 * note that the first few fields have to be the same as for
 * Shscoped_t in <shell.h>
 */
struct sh_scoped
{
	struct sh_scoped *prevst;	/* pointer to previous state */
	int		dolc;
	char		**dolv;
	char		*cmdname;
	Dt_t		*save_tree;	/* var_tree for calling function */
	struct slnod	*staklist;	/* link list of function stacks */
	struct sh_scoped *self;		/* pointer to copy of this scope*/
	int		states;
	int		breakcnt;
	int		execbrk;
	int		loopcnt;
	int		firstline;
	long		optindex;
	long		optnum;
	long		tmout;		/* value for TMOUT */ 
	short		optchar;
	short		opterror;
	int		ioset;
	short		trapmax;
	char		*trap[SH_DEBUGTRAP+1];
	char		**trapcom;
	char		**otrapcom;
	void		*timetrap;
};

struct limits
{
	int		open_max;	/* maximum number of file descriptors */
	int		clk_tck;	/* number of ticks per second */
	int		child_max;	/* maxumum number of children */
	int		ngroups_max;	/* maximum number of process groups */
	unsigned char	posix_version;	/* posix version number */
	unsigned char	posix_jobcontrol;/* non-zero for job control systems */
	unsigned char	fs3d;		/* non-zero for 3-d file system */
};

typedef void (*Sh_init_f)(int);

#define _SH_PRIVATE \
	struct sh_scoped st;		/* scoped information */ \
	struct limits	lim;		/* run time limits */ \
	Sfio_t		*heredocs;	/* current here-doc file */ \
	int		**fdptrs;	/* pointer to file numbers */ \
	int		savexit; \
	char		*lastarg; \
	char		*lastpath;	/* last alsolute path found */ \
	int		path_err;	/* last error on path search */ \
	Dt_t		*track_tree;	/* for tracked aliases*/ \
	Namval_t	*bltin_nodes;	/* pointer to built-in variables */ \
	Dt_t		*var_base;	/* global level variables */ \
	Namval_t	*namespace;	/* current active namespace*/ \
	Namval_t	*last_table;	/* last table used in last nv_open  */ \
	Sfio_t		*outpool;	/* ouput stream pool */ \
	long		timeout;	/* read timeout */ \
	int		curenv;		/* current subshell number */ \
	int		jobenv;		/* subshell number for jobs */ \
	int		nextprompt;	/* next prompt is PS<nextprompt> */ \
	Namval_t	*bltin_cmds;	/* pointer to built-in commands */ \
	Namval_t	*posix_fun;	/* points to last name() function */ \
	int		infd;		/* input file descriptor */ \
	char		*outbuff;	/* pointer to output buffer */ \
	char		*errbuff;	/* pointer to stderr buffer */ \
	char		*prompt;	/* pointer to prompt string */ \
	char		*shname;	/* shell name */ \
	char		*comdiv;	/* points to sh -c argument */ \
	char		*prefix;	/* prefix for compound assignment */ \
	sigjmp_buf	*jmplist;	/* longjmp return stack */ \
	char		**sigmsg;	/* points to signal messages */ \
	int		oldexit; \
	uid_t 		userid,euserid;	/* real and effective user id */ \
	gid_t 		groupid,egroupid;/* real and effective group id */ \
	pid_t		pid;		/* process id of shell */ \
	pid_t		bckpid;		/* background process id */ \
	pid_t		cpid; \
	long		ppid;		/* parent process id of shell */ \
	int		topfd; \
	int		sigmax;		/* maximum number of signals */ \
	unsigned char	*sigflag;	/* pointer to signal states */ \
	char		intrap; \
	char		login_sh; \
	char		lastbase; \
	char		forked;	\
	char		binscript; \
	char		used_pos;	/* used postional parameter */\
	unsigned char	lastsig;	/* last signal received */ \
	char		*readscript;	/* set before reading a script */ \
	int		*inpipe;	/* input pipe pointer */ \
	int		*outpipe;	/* output pipe pointer */ \
	int		cpipe[2]; \
	int		coutpipe; \
	struct argnod	*envlist; \
	struct dolnod	*arglist; \
	int		fn_depth; \
	int		dot_depth; \
	long		nforks; \
	Env_t		*env; \
	void		*init_context; \
	void		*mac_context; \
	void		*lex_context; \
	void		*arg_context; \
	void		*ed_context; \
	void		*job_context; \
	void		*pathlist; \
	void		*defpathlist; \
	void		*cdpathlist; \
	char		**argaddr; \
	void		*optlist; \
	int		optcount ; \
	struct sh_scoped global; \
	struct checkpt	checkbase; \
	Sh_init_f	userinit; \
	char		*cur_line; \
	short		offsets[10]; \
	char		ifstable[256];

#include	<shell.h>


/* error exits from various parts of shell */
#define	NIL(type)	((type)0)

#define new_of(type,x)	((type*)malloc((unsigned)sizeof(type)+(x)))

#define exitset()	(sh.savexit=sh.exitval)

#ifndef SH_DICT
#define SH_DICT		(void*)e_dict
#endif

/* states */
/* low numbered states are same as options */
#define SH_NOFORK	0x1	/* set when fork not necessary, not a state */
#define SH_COMPLETE	0x1	/* set for command completion */
#define SH_TTYWAIT	0x40	/* waiting for keyboard input */ 
#define	SH_FORKED	0x80	/* set when process has been forked */
#define	SH_PROFILE	0x100	/* set when processing profiles */
#define SH_NOALIAS	0x200	/* do not expand non-exported aliases */
#define SH_NOTRACK	0x400	/* set to disable sftrack() function */
#define SH_STOPOK	0x800	/* set for stopable builtins */
#define SH_GRACE	0x1000	/* set for timeout grace period */
#define SH_TIMING	0x2000	/* set while timing pipelines */
#define SH_DEFPATH	0x4000	/* set when using default path */
#define SH_INIT		0x8000	/* set when initializing the shell */

extern void 		*sh_argopen(Shell_t*);
extern Namval_t		*sh_assignok(Namval_t*,int);
extern char		*sh_checkid(char*,char*);
extern int 		sh_echolist(Sfio_t*, int, char**);
extern struct argnod	*sh_endword(int);
extern char 		**sh_envgen(void);
extern void 		sh_envnolocal(Namval_t*,void*);
extern double		sh_arith(const char*);
extern void		*sh_arithcomp(char*);
extern pid_t 		sh_fork(int,int*);
extern char 		*sh_mactrim(char*,int);
extern int 		sh_macexpand(struct argnod*,struct argnod**,int);
extern void 		sh_machere(Sfio_t*, Sfio_t*, char*);
extern void 		*sh_macopen(Shell_t*);
extern char 		*sh_mactry(char*);
extern int 		sh_readline(Shell_t*,char**,int,int,long);
extern Sfio_t		*sh_sfeval(char*[]);
extern void		sh_setmatch(const char*,int,int,int[]);
extern Dt_t		*sh_subaliastree(int);
extern Dt_t		*sh_subfuntree(int);
extern int		sh_subsavefd(int);
extern void		sh_subtmpfile(void);
extern char 		*sh_substitute(const char*,const char*,char*);
extern const char	*_sh_translate(const char*);
extern int		sh_trace(char*[],int);
extern void		sh_trim(char*);
extern void		sh_utol(const char*, char*);
extern int 		sh_whence(char**,int);

#ifndef ERROR_dictionary
#   define ERROR_dictionary(s)	(s)
#endif
#define sh_translate(s)	_sh_translate(ERROR_dictionary(s))

#define sh_isoption(x)	(sh.options & (x))
#define sh_onoption(x)	(sh.options |= (x))
#define sh_offoption(x)	(sh.options &= ~(x))

#define	sh_isstate(x)	(sh.st.states&((int)(x)))
#define	sh_onstate(x)	(sh.st.states |= ((int)(x)))
#define	sh_setstate(x)	(sh.st.states = ((int)(x)))
#define	sh_offstate(x)	(sh.st.states &= ~((int)(x)))

#define sh_sigcheck() do{if(sh.trapnote&SH_SIGSET)sh_exit(SH_EXITSIG);} while(0)

extern time_t		sh_mailchk;
extern const char	e_dict[];
