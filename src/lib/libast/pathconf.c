/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2010 AT&T Intellectual Property          *
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
*                   Phong Vo <kpv@research.att.com>                    *
*                                                                      *
***********************************************************************/

/* : : generated by proto : : */

#if !defined(__PROTO__)
#  if defined(__STDC__) || defined(__cplusplus) || defined(_proto) || defined(c_plusplus)
#    if defined(__cplusplus)
#      define __LINKAGE__	"C"
#    else
#      define __LINKAGE__
#    endif
#    define __STDARG__
#    define __PROTO__(x)	x
#    define __OTORP__(x)
#    define __PARAM__(n,o)	n
#    if !defined(__STDC__) && !defined(__cplusplus)
#      if !defined(c_plusplus)
#      	define const
#      endif
#      define signed
#      define void		int
#      define volatile
#      define __V_		char
#    else
#      define __V_		void
#    endif
#  else
#    define __PROTO__(x)	()
#    define __OTORP__(x)	x
#    define __PARAM__(n,o)	o
#    define __LINKAGE__
#    define __V_		char
#    define const
#    define signed
#    define void		int
#    define volatile
#  endif
#  define __MANGLE__	__LINKAGE__
#  if defined(__cplusplus) || defined(c_plusplus)
#    define __VARARG__	...
#  else
#    define __VARARG__
#  endif
#  if defined(__STDARG__)
#    define __VA_START__(p,a)	va_start(p,a)
#  else
#    define __VA_START__(p,a)	va_start(p)
#  endif
#  if !defined(__INLINE__)
#    if defined(__cplusplus)
#      define __INLINE__	extern __MANGLE__ inline
#    else
#      if defined(_WIN32) && !defined(__GNUC__)
#      	define __INLINE__	__inline
#      endif
#    endif
#  endif
#endif
#if !defined(__LINKAGE__)
#define __LINKAGE__		/* 2004-08-11 transition */
#endif
#include <ast.h>
#include <error.h>
#include <ls.h>

#if _UWIN

NoN(pathconf)

#else

/* : : generated by conf from /home/gsf/src/lib/libast/comp/conf.tab : : */

#include "confmap.h"
#include "conflib.h"
#include "FEATURE/unistd"
#include "FEATURE/limits"

static _ast_intmax_t
statconf __PARAM__((struct stat* st, int op), (st, op)) __OTORP__(struct stat* st; int op;){
	switch (op)
	{
#if	_PC_ALLOC_SIZE_MIN+0
case _PC_ALLOC_SIZE_MIN:
#ifdef	ALLOC_SIZE_MIN
		return(ALLOC_SIZE_MIN-0);
#else
		break;
#endif
#endif
#if	_PC_ASYNC_IO+0
case _PC_ASYNC_IO:
#ifdef	ASYNC_IO
		return(ASYNC_IO-0);
#else
#ifdef	_POSIX_ASYNC_IO
		return(_POSIX_ASYNC_IO-0);
#else
		break;
#endif
#endif
#endif
#if	_PC_CHOWN_RESTRICTED+0
case _PC_CHOWN_RESTRICTED:
#ifdef	CHOWN_RESTRICTED
		return(CHOWN_RESTRICTED-0);
#else
#ifdef	_POSIX_CHOWN_RESTRICTED
		return(_POSIX_CHOWN_RESTRICTED-0);
#else
		break;
#endif
#endif
#endif
#if	_PC_FILESIZEBITS+0
case _PC_FILESIZEBITS:
#ifdef	FILESIZEBITS
		return(FILESIZEBITS-0);
#else
#ifdef	_LOCAL_FILESIZEBITS
		return(_LOCAL_FILESIZEBITS-0);
#else
#ifdef	_POSIX_FILESIZEBITS
		return(_POSIX_FILESIZEBITS-0);
#else
		return(32-0);
#endif
#endif
#endif
#endif
#if	_PC_LINK_MAX+0
case _PC_LINK_MAX:
#ifdef	LINK_MAX
		return(LINK_MAX-0);
#else
#ifdef	MAXLINK
		return(MAXLINK-0);
#else
#ifdef	SHRT_MAX
		return(SHRT_MAX-0);
#else
#ifdef	_POSIX_LINK_MAX
		return(_POSIX_LINK_MAX-0);
#else
		return(8-0);
#endif
#endif
#endif
#endif
#endif
#if	_PC_MAX_CANON+0
case _PC_MAX_CANON:
#ifdef	MAX_CANON
		return(MAX_CANON-0);
#else
#ifdef	CANBSIZ
		return(CANBSIZ-0);
#else
#ifdef	_POSIX_MAX_CANON
		return(_POSIX_MAX_CANON-0);
#else
		return(255-0);
#endif
#endif
#endif
#endif
#if	_PC_MAX_INPUT+0
case _PC_MAX_INPUT:
#ifdef	MAX_INPUT
		return(MAX_INPUT-0);
#else
#ifdef	MAX_CANON
		return(MAX_CANON-0);
#else
#ifdef	_POSIX_MAX_INPUT
		return(_POSIX_MAX_INPUT-0);
#else
		return(255-0);
#endif
#endif
#endif
#endif
#if	_PC_NAME_MAX+0
case _PC_NAME_MAX:
#ifdef	NAME_MAX
		return(NAME_MAX-0);
#else
#ifdef	_LOCAL_NAME_MAX
		return(_LOCAL_NAME_MAX-0);
#else
#ifdef	_POSIX_NAME_MAX
		return(_POSIX_NAME_MAX-0);
#else
		return(14-0);
#endif
#endif
#endif
#endif
#if	_PC_NO_TRUNC+0
case _PC_NO_TRUNC:
#ifdef	NO_TRUNC
		return(NO_TRUNC-0);
#else
#ifdef	_POSIX_NO_TRUNC
		return(_POSIX_NO_TRUNC-0);
#else
		break;
#endif
#endif
#endif
#if	_PC_PATH_MAX+0
case _PC_PATH_MAX:
#ifdef	PATH_MAX
		return(PATH_MAX-0);
#else
#ifdef	MAXPATHLEN
		return(MAXPATHLEN-0);
#else
#ifdef	_POSIX_PATH_MAX
		return(_POSIX_PATH_MAX-0);
#else
		return(256-0);
#endif
#endif
#endif
#endif
#if	_PC_PIPE_BUF+0
case _PC_PIPE_BUF:
#ifdef	PIPE_BUF
		return(PIPE_BUF-0);
#else
#ifdef	_POSIX_PIPE_BUF
		return(_POSIX_PIPE_BUF-0);
#else
		return(512-0);
#endif
#endif
#endif
#if	_PC_PRIO_IO+0
case _PC_PRIO_IO:
#ifdef	PRIO_IO
		return(PRIO_IO-0);
#else
#ifdef	_POSIX_PRIO_IO
		return(_POSIX_PRIO_IO-0);
#else
		break;
#endif
#endif
#endif
#if	_PC_REC_INCR_XFER_SIZE+0
case _PC_REC_INCR_XFER_SIZE:
#ifdef	REC_INCR_XFER_SIZE
		return(REC_INCR_XFER_SIZE-0);
#else
		break;
#endif
#endif
#if	_PC_REC_MAX_XFER_SIZE+0
case _PC_REC_MAX_XFER_SIZE:
#ifdef	REC_MAX_XFER_SIZE
		return(REC_MAX_XFER_SIZE-0);
#else
		break;
#endif
#endif
#if	_PC_REC_MIN_XFER_SIZE+0
case _PC_REC_MIN_XFER_SIZE:
#ifdef	REC_MIN_XFER_SIZE
		return(REC_MIN_XFER_SIZE-0);
#else
		break;
#endif
#endif
#if	_PC_REC_XFER_ALIGN+0
case _PC_REC_XFER_ALIGN:
#ifdef	REC_XFER_ALIGN
		return(REC_XFER_ALIGN-0);
#else
		break;
#endif
#endif
#if	_PC_SOCK_MAXBUF+0
case _PC_SOCK_MAXBUF:
#ifdef	SOCK_MAXBUF
		return(SOCK_MAXBUF-0);
#else
#ifdef	_POSIX_SOCK_MAXBUF
		return(_POSIX_SOCK_MAXBUF-0);
#else
		break;
#endif
#endif
#endif
#if	_PC_2_SYMLINKS+0
case _PC_2_SYMLINKS:
#ifdef	_POSIX2_SYMLINKS
		return(_POSIX2_SYMLINKS-0);
#else
		break;
#endif
#endif
#if	_PC_SYMLINK_MAX+0
case _PC_SYMLINK_MAX:
#ifdef	SYMLINK_MAX
		return(SYMLINK_MAX-0);
#else
#ifdef	_LOCAL_SYMLINK_MAX
		return(_LOCAL_SYMLINK_MAX-0);
#else
#ifdef	_POSIX_SYMLINK_MAX
		return(_POSIX_SYMLINK_MAX-0);
#else
		return(255-0);
#endif
#endif
#endif
#endif
#if	_PC_SYNC_IO+0
case _PC_SYNC_IO:
#ifdef	SYNC_IO
		return(SYNC_IO-0);
#else
#ifdef	_POSIX_SYNC_IO
		return(_POSIX_SYNC_IO-0);
#else
		break;
#endif
#endif
#endif
#if	_PC_VDISABLE+0
case _PC_VDISABLE:
#ifdef	VDISABLE
		return(VDISABLE-0);
#else
#ifdef	_POSIX_VDISABLE
		return(_POSIX_VDISABLE-0);
#else
		break;
#endif
#endif
#endif
	default:
		break;
	}
	errno = EINVAL;
	return -1;
}

_ast_intmax_t
pathconf __PARAM__((const char* path, int op), (path, op)) __OTORP__(const char* path; int op;){
	_ast_intmax_t	n;
	struct stat	st;
#if _lib_pathconf
#undef	pathconf
	if ((n = op) >= 0 || -op <= confmap[0] && (n = confmap[-op]) >= 0)
	{
		_ast_intmax_t	val;
		int		olderrno;
		int		syserrno;

		olderrno = errno;
		errno = 0;
		val = pathconf(path, n);
		syserrno = errno;
		errno = olderrno;
		if (val != -1 || syserrno == 0)
			return val;
	}
#endif
	return (n = stat(path, &st)) ? n : statconf(&st, op);
}

#endif