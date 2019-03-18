/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*           Copyright (c) 1992-2007 AT&T Knowledge Ventures            *
*                      and is licensed under the                       *
*                  Common Public License, Version 1.0                  *
*                      by AT&T Knowledge Ventures                      *
*                                                                      *
*                A copy of the License is available at                 *
*            http://www.opensource.org/licenses/cpl1.0.txt             *
*         (with md5 checksum 059e8cd6165cb4c31e351f2b69388fd9)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                                                                      *
***********************************************************************/
#pragma prototyped

/*
 * ksh builtin command api
 */

#ifndef _SHCMD_H
#define _SHCMD_H	1

#ifndef SH_VERSION
#   define Shell_t	void
#endif
#ifndef NV_DEFAULT
#   define Namval_t	void
#endif
#ifndef ERROR_NOTIFY
#   define ERROR_NOTIFY	1
#endif

typedef int (*Shbltin_f)(int, char**, void*);

#undef Shbltin_t
typedef struct Shbltin_s
{
	Shell_t		*shp;
	void		*ptr;
	int		version;
	int		(*shrun)(int, char**);
	int		(*shtrap)(const char*, int);
	void		(*shexit)(int);
	Namval_t	*(*shbltin)(const char*, Shbltin_f, void*);
	unsigned char	notify;
	unsigned char	sigset;
	unsigned char	nosfio;
	Namval_t	*bnode;
	Namval_t	*vnode;
	char		*data;
	int		flags;
} Shbltin_t;

#if defined(SH_VERSION) ||  defined(_SH_PRIVATE)
#   undef Shell_t
#   undef Namval_t
#else 
#   define sh_run(c, ac, av)	((c)?(*((Shbltin_t*)(c))->shrun)(ac,av):-1)
#   define sh_system(c,str)	((c)?(*((Shbltin_t*)(c))->shtrap)(str,0):system(str))
#   define sh_exit(c,n)		((c)?(*((Shbltin_t*)(c))->shexit)(n):exit(n))
#   define sh_checksig(c)	((c) && ((Shbltin_t*)(c))->sigset)
#   if defined(SFIO_VERSION) || defined(_AST_H)
#	define LIB_INIT(c)
#   else
#	define LIB_INIT(c)	((c) && (((Shbltin_t*)(c))->nosfio = 1))
#   endif
#   ifndef _CMD_H
#	define cmdinit(ac,av,c,cat,flg)		do { if((ac)<=0) return(0); \
	    (((Shbltin_t*)(c))->notify = ((flg)&ERROR_NOTIFY)?1:0);} while(0)
#   endif
#endif

#endif
