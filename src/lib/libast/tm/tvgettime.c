/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*                  Copyright (c) 1985-2004 AT&T Corp.                  *
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

#include <tv.h>
#include <tm.h>

#include "FEATURE/tvlib"

int
tvgettime(Tv_t* tv)
{

#if _lib_clock_gettime && defined(CLOCK_REALTIME)

	struct timespec			s;

	clock_gettime(CLOCK_REALTIME, &s);
	tv->tv_sec = s.tv_sec;
	tv->tv_nsec = s.tv_nsec;

#else

#if defined(tmgettimeofday)

	struct timeval			v;

	tmgettimeofday(&v);
	tv->tv_sec = v.tv_sec;
	tv->tv_nsec = v.tv_usec * 1000;

#else

	static time_t			s;
	static unsigned _ast_int4_t	n;

	if ((tv->tv_sec = time(NiL)) != s)
	{
		s = tv->tv_sec;
		n = 0;
	}
	else
		n += 1000;
	tv->tv_nsec = n;

#endif

#endif

	return 0;
}
