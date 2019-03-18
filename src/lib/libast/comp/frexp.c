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
 * frexp/ldexp implementation
 */

#include <ast.h>

#include "FEATURE/float"

#if _lib_frexp && _lib_ldexp

NoN(frexp)

#else

static double		pow2[DBL_MAX_EXP + 1];

static int
init(void)
{
	register int	x;
	double		g;

	g = 1;
	for (x = 0; x < elementsof(pow2); x++)
	{
		pow2[x] = g;
		g *= 2;
	}
	return 0;
}

#define INIT()		(pow2[0]?0:init())

#if !_lib_frexp

extern double
frexp(double f, int* p)
{
	register int	k;
	register int	x;
	double		g;

	INIT();

	/*
	 * normalize
	 */

	x = k = DBL_MAX_EXP / 2;
	if (f < 1)
	{
		g = 1.0L / f;
		for (;;)
		{
			k = (k + 1) / 2;
			if (g < pow2[x])
				x -= k;
			else if (k == 1 && g < pow2[x+1])
				break;
			else
				x += k;
		}
		if (g == pow2[x])
			x--;
		x = -x;
	}
	else if (f > 1)
	{
		for (;;)
		{
			k = (k + 1) / 2;
			if (f > pow2[x])
				x += k;
			else if (k == 1 && f > pow2[x-1])
				break;
			else
				x -= k;
		}
		if (f == pow2[x])
			x++;
	}
	else
		x = 1;
	*p = x;

	/*
	 * shift
	 */

	x = -x;
	if (x < 0)
		f /= pow2[-x];
	else if (x < DBL_MAX_EXP)
		f *= pow2[x];
	else
		f = (f * pow2[DBL_MAX_EXP - 1]) * pow2[x - (DBL_MAX_EXP - 1)];
	return f;
}

#endif

#if !_lib_ldexp

extern double
ldexp(double f, register int x)
{
	INIT();
	if (x < 0)
		f /= pow2[-x];
	else if (x < DBL_MAX_EXP)
		f *= pow2[x];
	else
		f = (f * pow2[DBL_MAX_EXP - 1]) * pow2[x - (DBL_MAX_EXP - 1)];
	return f;
}

#endif

#endif
