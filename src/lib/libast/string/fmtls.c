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
 * AT&T Bell Laboratories
 *
 * ls formatter
 */

#include <ast.h>
#include <ls.h>
#include <tm.h>

#ifndef LS_W_MAX
#define LS_W_MAX	128
#endif

/*
 * ls formatter
 *
 *	buf	results placed here
 *	name	file name
 *	st	file stat buffer
 *	info	optional info
 *	link	link text if != 0
 *	flags	LS_* flags
 *
 *	return	end of formatted buf
 */

char*
fmtls(char* buf, const char* name, register struct stat* st, const char* info, const char* link, register int flags)
{
	register char*		s;
	time_t			tm;
	Sfoff_t			n;

	s = buf;
	if (flags & LS_INUMBER)
		s += sfsprintf(s, LS_W_MAX, "%*I*u ", LS_W_INUMBER - 1, sizeof(st->st_ino), st->st_ino);
	if (flags & LS_BLOCKS)
	{
		n = iblocks(st);
		s += sfsprintf(s, LS_W_MAX, "%*I*u ", LS_W_BLOCKS - 1, sizeof(n), n);
	}
	if (flags & LS_LONG)
	{
		s += sfsprintf(s, LS_W_MAX, "%s%3u", fmtmode(st->st_mode, flags & LS_EXTERNAL), (unsigned int)st->st_nlink);
		if (!(flags & LS_NOUSER))
		{
			if (flags & LS_NUMBER)
				s += sfsprintf(s, LS_W_MAX, " %-*I*d", LS_W_NAME - 1, sizeof(st->st_uid), st->st_uid);
			else
				s += sfsprintf(s, LS_W_MAX, " %-*s", LS_W_NAME - 1, fmtuid(st->st_uid));
		}
		if (!(flags & LS_NOGROUP))
		{
			if (flags & LS_NUMBER)
				s += sfsprintf(s, LS_W_MAX, " %-*I*d", LS_W_NAME - 1, sizeof(st->st_gid), st->st_gid);
			else
				s += sfsprintf(s, LS_W_MAX, " %-*s", LS_W_NAME - 1, fmtgid(st->st_gid));
		}
		if (S_ISBLK(st->st_mode) || S_ISCHR(st->st_mode))
			s += sfsprintf(s, LS_W_MAX, "%8s ", fmtdev(st));
		else
			s += sfsprintf(s, LS_W_MAX, "%8I*u ", sizeof(st->st_size), st->st_size);
		tm = (flags & LS_ATIME) ? st->st_atime : (flags & LS_CTIME) ? st->st_ctime : st->st_mtime;
		s = tmform(s, "%?%l", &tm);
		*s++ = ' ';
	}
	if (info)
	{
		while (*s = *info++)
			s++;
		*s++ = ' ';
	}
	while (*s = *name++)
		s++;
	if (flags & LS_MARK)
	{
		if (S_ISDIR(st->st_mode))
			*s++ = '/';
#ifdef S_ISLNK
		else if (S_ISLNK(st->st_mode))
			*s++ = '@';
#endif
		else if (st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH))
			*s++ = '*';
	}
	if (link)
	{
		s += sfsprintf(s, LS_W_MAX, " %s %s",
#ifdef S_ISLNK
			S_ISLNK(st->st_mode) ? "->" :
#endif
				"==", link);
	}
	*s = 0;
	return s;
}
