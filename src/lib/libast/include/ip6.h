/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2009 AT&T Intellectual Property          *
*                      and is licensed under the                       *
*                  Common Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
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

#if !_IP6_H
#define _IP6_H		1

#define IP6ADDR		16
#define IP6BITS		IP6ADDR
#define IP6PREFIX	(IP6ADDR+1)

#if _BLD_ast && defined(__EXPORT__)
#define extern		__EXPORT__
#endif

extern char*	fmtip6(unsigned char*, int);
extern int	strtoip6(const char*, char**, unsigned char*, unsigned char*);

#undef		extern

#endif
