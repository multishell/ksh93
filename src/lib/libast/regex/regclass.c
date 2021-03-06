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
 * RE character class support
 */

#include "reglib.h"

typedef struct Ctype_s
{
	const char*	name;
	size_t		size;
	regclass_t	ctype;
#if _lib_wctype
	wctype_t	wtype;
#endif
} Ctype_t;

#define CTYPES		12
#if _lib_wctype
#define WTYPES		8
#else
#define WTYPES		0
#endif

/*
 * this stuff gets around posix failure to define isblank,
 * and the fact that ctype functions are macros
 * and any local extensions that may not even have functions or macros
 */

#if _lib_wctype && !defined(iswblank) && !_lib_iswblank

int
_reg_iswblank(wint_t wc)
{
	static int	initialized;
	static wctype_t	wt;

	if (!initialized)
	{
		initialized = 1;
		wt = wctype("blank");
	}
	return iswctype(wc, wt);
}

#endif

static int  Isalnum(int c) { return  isalnum(c); }
static int  Isalpha(int c) { return  isalpha(c); }
static int  Isblank(int c) { return  isblank(c); }
static int  Iscntrl(int c) { return  iscntrl(c); }
static int  Isdigit(int c) { return  isdigit(c); }
static int Notdigit(int c) { return !isdigit(c); }
static int  Isgraph(int c) { return  isgraph(c); }
static int  Islower(int c) { return  islower(c); }
static int  Isprint(int c) { return  isprint(c); }
static int  Ispunct(int c) { return  ispunct(c); }
static int  Isspace(int c) { return  isspace(c); }
static int Notspace(int c) { return !isspace(c); }
static int  Isupper(int c) { return  isupper(c); }
static int   Isword(int c) { return  isalnum(c) || c == '_'; }
static int  Notword(int c) { return !isalnum(c) && c != '_'; }
static int Isxdigit(int c) { return  isxdigit(c);}

#if _lib_wctype

static int Is_wc_1(int);
static int Is_wc_2(int);
static int Is_wc_3(int);
static int Is_wc_4(int);
static int Is_wc_5(int);
static int Is_wc_6(int);
static int Is_wc_7(int);
static int Is_wc_8(int);

#endif

#define SZ(s)		s,(sizeof(s)-1)

static Ctype_t ctype[] =
{
	{ SZ("alnum"), Isalnum },
	{ SZ("alpha"), Isalpha },
	{ SZ("blank"), Isblank },
	{ SZ("cntrl"), Iscntrl },
	{ SZ("digit"), Isdigit },
	{ SZ("graph"), Isgraph },
	{ SZ("lower"), Islower },
	{ SZ("print"), Isprint },
	{ SZ("punct"), Ispunct },
	{ SZ("space"), Isspace },
	{ SZ("upper"), Isupper },
	{ SZ("word"),  Isword  },
	{ SZ("xdigit"),Isxdigit},
#if _lib_wctype
	{ 0, 0,        Is_wc_1 },
	{ 0, 0,        Is_wc_2 },
	{ 0, 0,        Is_wc_3 },
	{ 0, 0,        Is_wc_4 },
	{ 0, 0,        Is_wc_5 },
	{ 0, 0,        Is_wc_6 },
	{ 0, 0,        Is_wc_7 },
	{ 0, 0,        Is_wc_8 },
#endif
};

#if _lib_wctype

static int Is_wc_1(int c) { return iswctype(c, ctype[CTYPES+0].wtype); }
static int Is_wc_2(int c) { return iswctype(c, ctype[CTYPES+1].wtype); }
static int Is_wc_3(int c) { return iswctype(c, ctype[CTYPES+2].wtype); }
static int Is_wc_4(int c) { return iswctype(c, ctype[CTYPES+3].wtype); }
static int Is_wc_5(int c) { return iswctype(c, ctype[CTYPES+4].wtype); }
static int Is_wc_6(int c) { return iswctype(c, ctype[CTYPES+5].wtype); }
static int Is_wc_7(int c) { return iswctype(c, ctype[CTYPES+6].wtype); }
static int Is_wc_8(int c) { return iswctype(c, ctype[CTYPES+7].wtype); }

#endif

/*
 * return pointer to ctype function for :class:] in s
 * s points to the first char after the initial [
 * if e!=0 it points to next char in s
 * 0 returned on error
 */

regclass_t
regclass(const char* s, char** e)
{
	register int		c;
	register int		i;
	register size_t		n;
	register const char*	t;

	if (c = *s++)
	{
		for (t = s; *t && (*t != c || *(t + 1) != ']'); t++);
		if (*t != c)
			return 0;
		n = t - s;
		for (i = 0; i < elementsof(ctype); i++)
		{
#if _lib_wctype
			if (!ctype[i].size && (ctype[i].name = (const char*)memdup(s, n + 1)))
			{
				*((char*)ctype[i].name + n) = 0;
				if (!(ctype[i].wtype = wctype(ctype[i].name)))
				{
					free((char*)ctype[i].name);
					return 0;
				}
				ctype[i].size = n;
			}
#endif
			if (n == ctype[i].size && strneq(s, ctype[i].name, n))
			{
				if (e)
					*e = (char*)t + 2;
				return ctype[i].ctype;
			}
		}
	}
	return 0;
}

/*
 * return pointer to ctype function for token
 */

regclass_t
classfun(int type)
{
	switch (type)
	{
	case T_ALNUM:		return  Isword;
	case T_ALNUM_NOT:	return Notword;
	case T_DIGIT:		return  Isdigit;
	case T_DIGIT_NOT:	return Notdigit;
	case T_SPACE:		return  Isspace;
	case T_SPACE_NOT:	return Notspace;
	}
	return 0;
}
