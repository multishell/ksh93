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
 * install error message handler for fatal malloc exceptions
 */

#include <ast.h>
#include <error.h>
#include <vmalloc.h>

#if _std_malloc

void
memfatal(void)
{
}

#else

/*
 * print message and fail on VM_BADADDR,VM_NOMEM
 */

static int
nomalloc(Vmalloc_t* region, int type, void* obj, Vmdisc_t* disc)
{
	Vmstat_t	st;

	NoP(disc);
	switch (type)
	{
	case VM_BADADDR:
		error(ERROR_SYSTEM|3, "invalid pointer %p passed to free or realloc", obj);
		return(-1);
	case VM_NOMEM:
		vmstat(region, &st);
		error(ERROR_SYSTEM|3, "storage allocator out of space on %lu byte request ( region %lu segments %lu busy %lu:%lu:%lu free %lu:%lu:%lu )", (size_t)obj, st.extent, st.n_seg, st.n_busy, st.s_busy, st.m_busy, st.n_free, st.s_free, st.m_free);
		return(-1);
	}
	return(0);
}

/*
 * initialize the malloc exception handler
 */

void
memfatal(void)
{
	Vmdisc_t*	disc;

	if (disc = vmdisc(Vmheap, NiL))
		disc->exceptf = nomalloc;
}

#endif
