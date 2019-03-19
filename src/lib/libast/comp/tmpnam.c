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
 * tmpnam implementation
 */

#define _def_map_ast	1

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide tmpnam
#else
#define tmpnam		______tmpnam
#endif

#include <ast.h>
#include <stdio.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide tmpnam
#else
#undef	tmpnam
#endif

#undef	_def_map_ast

#include <ast_map.h>

#if ~(~L_tmpnam+0) == 0 && ~(~L_tmpnam+1) == 1
// Defined with no value
#undef L_tmpnam
#endif
#ifndef L_tmpnam
#define L_tmpnam	25
#endif

#if defined(__EXPORT__)
#define extern	__EXPORT__
#endif

extern char*
tmpnam(char* s)
{
	static char	buf[L_tmpnam];

	return pathtemp(s ? s : buf, L_tmpnam, NiL, "tn", NiL);
}
