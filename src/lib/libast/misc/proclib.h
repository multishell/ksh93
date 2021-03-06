/*******************************************************************
*                                                                  *
*             This software is part of the ast package             *
*                Copyright (c) 1985-2002 AT&T Corp.                *
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
*               Glenn Fowler <gsf@research.att.com>                *
*                David Korn <dgk@research.att.com>                 *
*                 Phong Vo <kpv@research.att.com>                  *
*                                                                  *
*******************************************************************/
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Research
 *
 * process library definitions
 */

#ifndef _PROCLIB_H
#define _PROCLIB_H

#include <ast.h>
#include <errno.h>
#include <sig.h>

#if _lib_sigprocmask
typedef sigset_t Sig_mask_t;
#else
typedef unsigned long Sig_mask_t;
#endif

#define _PROC_PRIVATE_ \
	long		flags;		/* original PROC_* flags	*/ \
	Sig_mask_t	mask;		/* original blocked sig mask	*/ \
	Sig_handler_t	sigchld;	/* PROC_FOREGROUND SIG_DFL	*/ \
	Sig_handler_t	sigint;		/* PROC_FOREGROUND SIG_IGN	*/ \
	Sig_handler_t	sigquit;	/* PROC_FOREGROUND SIG_IGN	*/

#include <proc.h>

#define proc_default	_proc_info_	/* hide external symbol		*/

extern Proc_t		proc_default;	/* first proc			*/

#ifndef errno
extern int		errno;
#endif

#endif
