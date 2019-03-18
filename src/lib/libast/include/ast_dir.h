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
 * AT&T Research
 *
 * common dirent maintenance interface
 */

#ifndef _AST_DIR_H
#define _AST_DIR_H

#include <ast_lib.h>

#if _BLD_ast
#include "dirlib.h"
#else
#include <dirent.h>
#endif

#if _mem_d_fileno_dirent || _mem_d_ino_dirent
#if !_mem_d_fileno_dirent
#define _mem_d_fileno_dirent	1
#define d_fileno		d_ino
#endif
#endif

#if _mem_d_fileno_dirent
#define D_FILENO(d)		((d)->d_fileno)
#else
#define D_FILENO(d)		(1)
#endif

#if _mem_d_namlen_dirent
#define D_NAMLEN(d)		((d)->d_namlen)
#else
#define D_NAMLEN(d)		(strlen((d)->d_name))
#endif

#if _mem_d_reclen_dirent
#define D_RECLEN(d)		((d)->d_reclen)
#else
#define D_RECLEN(d)		D_RECSIZ(d,D_NAMLEN(d))
#endif

#define D_RECSIZ(d,n)		(sizeof(*(d))-sizeof((d)->d_name)+((n)+sizeof(char*))&~(sizeof(char*)-1))

#endif
