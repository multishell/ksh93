/*******************************************************************
*                                                                  *
*             This software is part of the ast package             *
*                Copyright (c) 1985-2004 AT&T Corp.                *
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
 * AT&T Bell Laboratories
 *
 * time conversion support
 */

#include <ast.h>
#include <tm.h>
#include <ctype.h>

/*
 * return the tab table index that matches s ignoring case and .'s
 * tm_data.format checked if tminfo.format!=tm_data.format
 *
 * ntab and nsuf are the number of elements in tab and suf,
 * -1 for 0 sentinel
 *
 * all isalpha() chars in str must match
 * suf is a table of nsuf valid str suffixes 
 * if e is non-null then it will point to first unmatched char in str
 * which will always be non-isalpha()
 */

int
tmlex(register const char* s, char** e, char** tab, int ntab, char** suf, int nsuf)
{
	register char**	p;
	register int	n;

	for (p = tab, n = ntab; n-- && *p; p++)
		if (tmword(s, e, *p, suf, nsuf))
			return p - tab;
	if (tm_info.format != tm_data.format && tab >= tm_info.format && tab < tm_info.format + TM_NFORM)
	{
		tab = tm_data.format + (tab - tm_info.format);
		if (suf && tab >= tm_info.format && tab < tm_info.format + TM_NFORM)
			suf = tm_data.format + (suf - tm_info.format);
		for (p = tab, n = ntab; n-- && *p; p++)
			if (tmword(s, e, *p, suf, nsuf))
				return p - tab;
	}
	return -1;
}
