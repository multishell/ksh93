/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2009 AT&T Intellectual Property          *
*                      and is licensed under the                       *
*                  Common Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
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
*                                                                      *
***********************************************************************/
#pragma prototyped

static const char usage[] =
"[-?\n@(#)$Id: mktemp (AT&T Research) 2009-02-02 $\n]"
USAGE_LICENSE
"[+NAME?mktemp - make temporary file or directory]"
"[+DESCRIPTION?\bmktemp\b creates a temporary file with optional base "
    "name prefix \aprefix\a. If \aprefix\a is omitted then \btmp_\b is used "
    "and \b--tmp\b is implied. A temporary file will have mode \brw-------\b "
    "and a temporary directory will have mode \brwx------\b, subject to "
    "\bumask\b(1). Generated paths have these attributes:]"
    "{"
        "[+*?Lower case to avoid clashes on case ignorant filesystems.]"
        "[+*?Pseudo-random part to deter denial of service attacks.]"
        "[+*?Pseudo-random part is \a3-chars\a.\a3-chars\a to accomodate "
            "8.3 filesystems.]"
    "}"
"[+?A consecutive sequence of \bX\b's in \aprefix\a is replaced by the "
    "pseudo-random part. If there are no \bX\b's then the pseudo-random part "
    "is appended to the prefix.]"
"[d:directory?Create a directory instead of a regular file.]"
"[m:mode]:[mode?Set the mode of the created temporary to \amode\a. "
    "\amode\a is symbolic or octal mode as in \bchmod\b(1). Relative modes "
    "assume an initial mode of \bu=rwx\b.]"
"[q:quiet?Suppress file and directory error diagnostics.]"
"[t:tmp|temporary-directory?Create a path rooted in a temporary "
    "directory.]"
"\n"
"\n[ prefix ]\n"
"\n"
"[+SEE ALSO?\bmkdir\b(1), \bpathtemp\b(3), \bmktemp\b(3)]"
;

#include <cmd.h>
#include <ls.h>

int
b_mktemp(int argc, char** argv, void* context)
{
	mode_t		mode = 0;
	mode_t		mask;
	int		fd;
	int		quiet = 0;
	int*		fdp = &fd;
	char*		dir = "";
	char*		pfx;
	char*		t;
	char		path[PATH_MAX];

	cmdinit(argc, argv, context, ERROR_CATALOG, 0);
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 0:
			break;
		case 'd':
			fdp = 0;
			continue;
		case 'm':
			mode = strperm(pfx = opt_info.arg, &opt_info.arg, S_IRWXU);
			if (*opt_info.arg)
				error(ERROR_exit(0), "%s: invalid mode", pfx);
			continue;
		case 'q':
			quiet = 1;
			continue;
		case 't':
			dir = 0;
			continue;
		case ':':
			error(2, "%s", opt_info.arg);
			continue;
		case '?':
			error(ERROR_usage(2), "%s", opt_info.arg);
			continue;
		}
		break;
	}
	argv += opt_info.index;
	if (error_info.errors || (pfx = *argv++) && *argv)
		error(ERROR_usage(2), "%s", optusage(NiL));
	mask = umask(0);
	if (!mode)
		mode = (fdp ? (S_IRUSR|S_IWUSR) : S_IRWXU) & ~mask;
	umask(~mode & (S_IRWXU|S_IRWXG|S_IRWXO));
	if (!pfx)
	{
		pfx = "tmp_";
		dir = 0;
	}
	if (*pfx == '/')
	{
		t = pfx;
		pfx = strrchr(pfx, '/') + 1;
		dir = fmtbuf(pfx - t);
		strcpy(dir, t);
	}
	for (;;)
	{
		if (!pathtemp(path, sizeof(path), dir, pfx, fdp))
		{
			if (quiet)
				error_info.errors++;
			else
				error(ERROR_SYSTEM|2, "cannot create temporary path");
			break;
		}
		if (fdp || !mkdir(path, mode))
		{
			if (fdp)
				close(*fdp);
			sfputr(sfstdout, path, '\n');
			break;
		}
	}
	umask(mask);
	return error_info.errors != 0;
}
