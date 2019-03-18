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
 * AT&T Research
 *
 * include style search support
 */

#include <ast.h>
#include <error.h>
#include <ls.h>

#define directory(p,s)	(stat((p),(s))>=0&&S_ISDIR((s)->st_mode))
#define regular(p,s)	(stat((p),(s))>=0&&(S_ISREG((s)->st_mode)||streq(p,"/dev/null")))

typedef struct Dir_s			/* directory list element	*/
{
	struct Dir_s*	next;		/* next in list			*/
	char		dir[1];		/* directory path		*/
} Dir_t;

static struct				/* directory list state		*/
{
	Dir_t*		head;		/* directory list head		*/
	Dir_t*		tail;		/* directory list tail		*/
} state;

/*
 * append dir to pathfind() include list
 */

int
pathinclude(const char* dir)
{
	register Dir_t*	dp;
	struct stat	st;

	if (dir && *dir && !streq(dir, ".") && directory(dir, &st))
	{
		for (dp = state.head; dp; dp = dp->next)
			if (streq(dir, dp->dir))
				return 0;
		if (!(dp = oldof(0, Dir_t, 1, strlen(dir))))
			return -1;
		strcpy(dp->dir, dir);
		dp->next = 0;
		if (state.tail)
			state.tail = state.tail->next = dp;
		else
			state.head = state.tail = dp;
	}
	return 0;
}

/*
 * return path to name using pathinclude() list
 * path placed in <buf,size>
 * if lib!=0 then pathpath() attempted after include search
 * if type!=0 and name has no '.' then file.type also attempted
 * any *: prefix in lib is ignored (discipline library dictionary support)
 */

char*
pathfind(const char* name, const char* lib, const char* type, char* buf, size_t size)
{
	register Dir_t*		dp;
	register char*		s;
	char			tmp[PATH_MAX];
	struct stat		st;

	if (((s = strrchr(name, '/')) || (s = (char*)name)) && strchr(s, '.'))
		type = 0;

	/*
	 * always check the unadorned path first
	 * this handles . and absolute paths
	 */

	if (regular(name, &st))
	{
		strncopy(buf, name, size);
		return buf;
	}
	if (type)
	{
		sfsprintf(buf, size, "%s.%s", name, type);
		if (regular(buf, &st))
			return buf;
	}
	if (*name == '/')
		return 0;

	/*
	 * check the directory of the including file
	 * on the assumption that error_info.file is properly stacked
	 */

	if (error_info.file && (s = strrchr(error_info.file, '/')))
	{
		sfsprintf(buf, size, "%-.*s%s", s - error_info.file + 1, error_info.file, name);
		if (regular(buf, &st))
			return buf;
		if (type)
		{
			sfsprintf(buf, size, "%-.*s%s%.s", s - error_info.file + 1, error_info.file, name, type);
			if (regular(buf, &st))
				return buf;
		}
	}

	/*
	 * check the include dir list
	 */

	for (dp = state.head; dp; dp = dp->next)
	{
		sfsprintf(tmp, sizeof(tmp), "%s/%s", dp->dir, name);
		if (pathpath(buf, tmp, "", PATH_REGULAR))
			return buf;
		if (type)
		{
			sfsprintf(tmp, sizeof(tmp), "%s/%s.%s", dp->dir, name, type);
			if (pathpath(buf, tmp, "", PATH_REGULAR))
				return buf;
		}
	}

	/*
	 * finally a lib related search on PATH
	 */

	if (lib)
	{
		if (s = strrchr((char*)lib, ':'))
			lib = (const char*)s + 1;
		sfsprintf(tmp, sizeof(tmp), "lib/%s/%s", lib, name);
		if (pathpath(buf, tmp, "", PATH_REGULAR))
			return buf;
		if (type)
		{
			sfsprintf(tmp, sizeof(tmp), "lib/%s/%s.%s", lib, name, type);
			if (pathpath(buf, tmp, "", PATH_REGULAR))
				return buf;
		}
	}
	return 0;
}
