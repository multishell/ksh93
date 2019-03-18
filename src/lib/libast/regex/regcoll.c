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
 * regex collation symbol support
 */

#include "reglib.h"

#include <cdt.h>
#include <ccode.h>

typedef unsigned _ast_int2_t Ucs_code_t;
typedef unsigned _ast_int4_t Ucs_attr_t;

typedef struct Ucs_map_s
{
	Ucs_attr_t		attr[3];
	Ucs_code_t		code;
	const char*		name;
	Dtlink_t		link;
	struct Ucs_map_s*	next;
} Ucs_map_t;

#define setattr(a,i)	((a)[(i)>>5]|=(1<<((i)&((1<<5)-1))))
#define tstattr(a,i)	((a)[(i)>>5]&(1<<((i)&((1<<5)-1))))
#define clrattr(a,i)	((a)[(i)>>5]&=~(1<<((i)&((1<<5)-1))))

#ifndef UCS_BYTE
#define UCS_BYTE	1
#endif

#include "ucs_names.h"

static struct Local_s
{
	int		fatal;
	Dt_t*		attrs;
	Dt_t*		names;
	Dtdisc_t	dtdisc;
} local;

static int
initialize(void)
{
	register int		i;
	register Ucs_map_t*	a;

	local.dtdisc.link = offsetof(Ucs_map_t, link);
	local.dtdisc.key = offsetof(Ucs_map_t, name);
	local.dtdisc.size = -1;
	if (!(local.attrs = dtopen(&local.dtdisc, Dttree)))
	{
		local.fatal = 1;
		return -1;
	}
	if (!(local.names = dtopen(&local.dtdisc, Dttree)))
	{
		dtclose(local.attrs);
		local.fatal = 1;
		return -1;
	}
	for (i = 0; i < elementsof(ucs_attrs); i++)
		dtinsert(local.attrs, ucs_attrs+i);
	for (i = 0; i < elementsof(ucs_names); i++)
		if (a = (Ucs_map_t*)dtsearch(local.names, ucs_names+i))
		{
			while (a->next)
				a = a->next;
			a->next = ucs_names+i;
		}
		else
			dtinsert(local.names, ucs_names+i);
	return 0;
}

/*
 * return the collating symbol delimited by [c c], where c is either '=' or '.'
 * s points to the first char after the initial [
 * if e!=0 it is set to point to the next char in s on return
 *
 * the collating symbol is converted to multibyte in <buf,size>
 * the return value is:
 *	-1	syntax error or buf not large enough
 *	>=0	size with 0-terminated mb collation element
 *		or ligature value in buf
 */

int
regcollate(register const char* s, char** e, char* buf, int size)
{
	register int		c;
	register char*		u;
	register char*		b;
	register char*		x;
	register Ucs_map_t*	a;
	Ucs_map_t*		z;
	const char*		t;
	const char*		v;
	int			n;
	int			r;
	int			ul;
	int			term;
	wchar_t			w[2];
	Ucs_attr_t		attr[3];

	if (size < 2)
		r = -1;
	else if ((term = *s++) != '.' && term != '=')
	{
		s--;
		r = -1;
	}
	else if (*s == term && *(s + 1) == ']')
		r = -1;
	else
	{
		t = s;
		mbchar(s);
		if ((n = (s - t)) == 1)
		{
			if (*s == term && *(s + 1) == ']')
			{
				s += 2;
				r = -1;
			}
			else
			{
				if (!local.attrs && initialize())
					return -1;
				attr[0] = attr[1] = attr[2] = 0;
				ul = 0;
				b = buf;
				x = buf + size - 2;
				r = 1;
				s = t;
				do
				{
					v = s;
					u = b;
					for (;;)
					{
						if (!(c = *s++))
							return -1;
						if (c == term)
						{
							if (!(c = *s++))
								return -1;
							if (c != term)
							{
								if (c != ']')
									return -1;
								r = -1;
								break;
							}
						}
						if (c == ' ' || c == '-' && u > b && *s != ' ' && *s != '-')
							break;
						if (isupper(c))
							c = tolower(c);
						if (u > x)
							break;
						*u++ = c;
					}
					*u = 0;
					if (a = (Ucs_map_t*)dtmatch(local.attrs, b))
						setattr(attr, a->code);
					else
					{
						if (u < x)
							*u++ = ' ';
						if (b == buf)
						{
							if (isupper(*v))
								ul = UCS_UC;
							else if (islower(*v))
								ul = UCS_LC;
						}
						b = u;
					}
				} while (r > 0);
				if (b > buf && *(b - 1) == ' ')
					b--;
				*b = 0;
				attr[0] &= ~((Ucs_attr_t)1);
				if (ul)
				{
					if (tstattr(attr, UCS_UC) || tstattr(attr, UCS_LC))
						ul = 0;
					else
						setattr(attr, ul);
				}
				if (z = (Ucs_map_t*)dtmatch(local.names, buf))
					for(;;)
					{
						for (a = z; a; a = a->next)
							if ((attr[0] & a->attr[0]) == attr[0] && (attr[1] & a->attr[1]) == attr[1] && (attr[2] & a->attr[2]) == attr[2])
							{
								if (a->code <= 0xff)
								{
									buf[0] = ccmapc(a->code, CC_ASCII, CC_NATIVE);
									buf[r = 1] = 0;
									ul = 0;
									break;
								}
								w[0] = a->code;
								w[1] = 0;
								if ((r = wcstombs(buf, w, size)) > 0)
								{
									r--;
									ul = 0;
								}
								break;
							}
						if (!ul)
							break;
						clrattr(attr, ul);
						ul = 0;
					}
			}
			if (r < 0)
			{
				if ((r = s - t - 2) > (size - 1))
					return -1;
				memcpy(buf, t, r);
				buf[r] = 0;
			}
		}
		else if (*s++ != term || *s++ != ']')
		{
			s--;
			r = -1;
		}
		else if (n > (size - 1))
			r = -1;
		else
		{
			memcpy(buf, t, n);
			buf[r = n] = 0;
		}
	}
	if (e)
		*e = (char*)s;
	return r;
}
