/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*                  Copyright (c) 1992-2006 AT&T Corp.                  *
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
*                                                                      *
***********************************************************************/
#pragma prototyped

/*
 * common ast cmd library definitions
 */

#ifndef _CMDLIB_H
#define _CMDLIB_H

#include <ast.h>
#include <cmd.h>
#include <error.h>
#include <stak.h>

#define rev_line	_cmd_revline

extern int		rev_line(Sfio_t*, Sfio_t*, off_t);

#endif
