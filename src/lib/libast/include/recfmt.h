/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*                  Copyright (c) 1985-2005 AT&T Corp.                  *
*                      and is licensed under the                       *
*                  Common Public License, Version 1.0                  *
*                            by AT&T Corp.                             *
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
*                   Phong Vo <kpv@research.att.com>                    *
*                                                                      *
***********************************************************************/
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Research
 *
 * record format interface
 */

#ifndef _RECFMT_H
#define _RECFMT_H	1

#include <ast.h>

#define REC_fixed	0
#define REC_variable	1
#define REC_delimited	2

#define RECMAKE(t,s)	(((t)<<24)|(s))
#define RECTYPE(n)	(((n)>>24)&((1<<7)-1))
#define RECSIZE(n)	((n)&((1<<24)-1))

#if _BLD_ast && defined(__EXPORT__)
#define extern		__EXPORT__
#endif

extern size_t		recfmt(const void*, size_t, off_t);

#undef	extern

#endif
