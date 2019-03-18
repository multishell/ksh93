/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*                  Copyright (c) 1985-2006 AT&T Corp.                  *
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
 * OBSOLETE 20030321 -- use spawnveg()
 */

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide spawnve spawnvpe spawnvp spawnlp
#else
#define spawnve		______spawnve
#define spawnvpe	______spawnvpe
#define spawnvp		______spawnvp
#define spawnlp		______spawnlp
#endif

#include <ast.h>
#include <error.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide spawnve spawnvpe spawnvp spawnlp
#else
#undef	spawnve
#undef	spawnvpe
#undef	spawnvp
#undef	spawnlp
#endif

#if defined(__EXPORT__)
#define extern	__EXPORT__
#endif

#if _lib_spawnve

NoN(spawnve)

#else

extern pid_t
spawnve(const char* cmd, char* const argv[], char* const envv[])
{
	return spawnveg(cmd, argv, envv, 0);
}

#endif

#if _lib_spawnvpe

NoN(spawnvpe)

#else

extern pid_t
spawnvpe(const char* name, char* const argv[], char* const envv[])
{
	register const char*	path = name;
	pid_t			pid;
	char			buffer[PATH_MAX];

	if (*path != '/')
		path = pathpath(buffer, name, NULL, X_OK|PATH_REGULAR);
	if ((pid = spawnve(path, argv, envv)) >= 0)
		return pid;
	if (errno == ENOEXEC)
	{
		register char**	newargv;
		register char**	ov;
		register char**	nv;

		for (ov = (char**)argv; *ov++;);
		if (newargv = newof(0, char*, ov + 1 - (char**)argv, 0))
		{
			nv = newargv;
			*nv++ = "sh";
			*nv++ = (char*)path;
			ov = (char**)argv;
			while (*nv++ = *++ov);
			path = pathshell();
			pid = spawnve(path, newargv, environ);
			free(newargv);
		}
		else
			errno = ENOMEM;
	}
	return pid;
}

#endif

#if _lib_spawnvp

NoN(spawnvp)

#else

extern pid_t
spawnvp(const char* name, char* const argv[])
{
	return spawnvpe(name, argv, environ);
}

#endif

#if _lib_spawnlp

NoN(spawnlp)

#else

extern pid_t
spawnlp(const char* name, const char* arg, ...)
{
	va_list	ap;
	pid_t	pid;

	va_start(ap, arg);
	pid = spawnvp(name, (char* const*)&arg);
	va_end(ap);
	return pid;
}

#endif
