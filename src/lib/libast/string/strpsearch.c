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
 */

#include <ast.h>
#include <ccode.h>
#include <ctype.h>

/*
 * return a pointer to the isalpha() identifier matching
 * name in the CC_ASCII sorted tab of num elements of
 * size siz where the first member of each
 * element is a char*
 *
 * [xxx] brackets optional identifier characters
 * * starts optional identifier characters
 *
 * 0 returned if name not found
 * otherwise if next!=0 then it points to the next
 * unmatched char in name
 */

void*
strpsearch(const void* tab, size_t num, size_t siz, const char* name, char** next)
{
	register char*		lo = (char*)tab;
	register char*		hi = lo + (num - 1) * siz;
	register char*		mid;
	register unsigned char*	s;
	register unsigned char*	t;
	register int		c;
	register int		v;
	int			sequential = 0;

	c = ccmapc(*((unsigned char*)name), CC_NATIVE, CC_ASCII);
	while (lo <= hi)
	{
		mid = lo + (sequential ? 0 : (((hi - lo) / siz) / 2) * siz);
		if (!(v = c - ccmapc(*(s = *((unsigned char**)mid)), CC_NATIVE, CC_ASCII)) || *s == '[' && !(v = c - ccmapc(*++s, CC_NATIVE, CC_ASCII)) && (v = 1))
		{
			t = (unsigned char*)name;
			for (;;)
			{
				if (!v && (*s == '[' || *s == '*'))
				{
					v = 1;
					s++;
				}
				else if (v && *s == ']')
				{
					v = 0;
					s++;
				}
				else if (!isalpha(*t))
				{
					if (v || !*s)
					{
						if (next)
							*next = (char*)t;
						return (void*)mid;
					}
					if (!sequential)
					{
						while ((mid -= siz) >= lo && (s = *((unsigned char**)mid)) && ((c == ccmapc(*s, CC_NATIVE, CC_ASCII)) || *s == '[' && c == ccmapc(*(s + 1), CC_NATIVE, CC_ASCII)));
						sequential = 1;
					}
					v = 1;
					break;
				}
				else if (*t != *s)
				{
					v = ccmapc(*t, CC_NATIVE, CC_ASCII) - ccmapc(*s, CC_NATIVE, CC_ASCII);
					break;
				}
				else
				{
					t++;
					s++;
				}
			}
		}
		else if (sequential)
			break;
		if (v > 0)
			lo = mid + siz;
		else
			hi = mid - siz;
	}
	return 0;
}
