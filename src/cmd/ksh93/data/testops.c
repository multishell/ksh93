/*******************************************************************
*                                                                  *
*             This software is part of the ast package             *
*                Copyright (c) 1982-2002 AT&T Corp.                *
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
*                David Korn <dgk@research.att.com>                 *
*                                                                  *
*******************************************************************/
#pragma prototyped

/*
 * tables for the test builin [[...]] and [...]
 */

#include	"shtable.h"
#include	"test.h"

/*
 * This is the list of binary test and [[...]] operators
 */

const Shtable_t shtab_testops[] =
{
		"!=",		TEST_SNE,
		"-a",		TEST_AND,
		"-ef",		TEST_EF,
		"-eq",		TEST_EQ,
		"-ge",		TEST_GE,
		"-gt",		TEST_GT,
		"-le",		TEST_LE,
		"-lt",		TEST_LT,
		"-ne",		TEST_NE,
		"-nt",		TEST_NT,
		"-o",		TEST_OR,
		"-ot",		TEST_OT,
		"=",		TEST_SEQ,
		"==",		TEST_SEQ,
		"<",		TEST_SLT,
		">",		TEST_SGT,
		"]]",		TEST_END,
		"",		0
};

const char sh_opttest[] =
"[-1c?\n@(#)$Id: test (AT&T Labs Research) 1999-07-17 $\n]"
USAGE_LICENSE
"[+NAME?test - evaluate expression]"
"[+DESCRIPTION?\btest\b evaluates expressions and indicates its "
	"results based on the exit status.  Option parsing is not "
	"performed so that all arguments, including \b--\b are processed "
	" as operands.  The evaluation of the "
	"expression depends on the number of operands as follows:]{"
	"[+0?Evaluates to false.]"
	"[+1?True if argument is not an empty string.]"
	"[+2?If first operand is \b!\b, the result is True if the second "
		"operand an empty string.  Otherwise, it is evaluated "
		"as one of the unary expressions defined below.  If the "
		"unary operator is invalid and the second argument is \b--\b,"
		"then the first argument is processed as an option argument.]"
	"[+3?If first operand is \b!\b, the result is True if the second "
		"and third operand evaluated as a unary expression is False.  "
		"Otherwise, the three operands are evaluaged as one of the  "
		"binary expressions listed below.]"
	"[+4?If first operand is \b!\b, the result is True if the next "
		"three operands are a valid binary expression that is False.]"
"}"

"[+?Unary expressions can be one of the following:]{"
	"[+-a \afile\a?True if \afile\a exists, obsolete.]"
	"[+-b \afile\a?True if \afile\a exists and is a block special file.]"
	"[+-c \afile\a?True if \afile\a exists and is a character special "
		"file.]"
	"[+-d \afile\a?True if \afile\a exists and is a directory.]"
	"[+-e \afile\a?True if \afile\a exists.]"
	"[+-f \afile\a?True if \afile\a exists and is a regular file.]"
	"[+-g \afile\a?True if \afile\a exists and has its set-group-id bit "
		"set.]"
	"[+-h \afile\a?True if \afile\a exists and is a symbolic link.]"
	"[+-k \afile\a?True if \afile\a exists and has its sticky bit on.]"
#ifdef SHOPT_TEST_L
	"[+-l \afile\a?True if \afile\a exists and is a symbolic link.]"
#endif
	"[+-n \astring\a?True if length of \astring\a is non-zero.]"
	"[+-o \aoption\a?True if the shell option \aoption\a is enabled.]"
	"[+-p \afile\a?True if \afile\a exists and is a pipe or fifo.]"
	"[+-r \afile\a?True if \afile\a exists and is readable.]"
	"[+-s \afile\a?True if \afile\a exists and has size > 0.]"
	"[+-t \afildes\a?True if file descriptor number \afildes\a is "
		"open and is associated with a terminal device.]"
	"[+-u \afile\a?True if \afile\a exists and has its set-user-id bit "
		"set.]"
	"[+-w \afile\a?True if \afile\a exists and is writable.]"
	"[+-x \afile\a?True if \afile\a exists and is executable.  For a "
		"directory it means that it can be searched.]"
	"[+-z \astring\a?True if \astring\a is a zero length string.]"
	"[+-L \afile\a?True if \afile\a exists and is a symbolic link.]"
	"[+-G \afile\a?True if \afile\a exists and group is the effective "
		"group id of the current process.]"
	"[+-O \afile\a?True if \afile\a exists and owner is the effective "
		"user id of the current process.]"
	"[+-S \afile\a?True if \afile\a exists and is a socket.]"
"}"
"[+?Binary expressions can be one of the following:]{"
	"[+\astring1\a = \astring2\a?True if \astring1\a is equal to "
		"\astring2\a.]"
	"[+\astring1\a == \astring2\a?True if \astring1\a is equal to "
		"\astring2\a.]"
	"[+\astring1\a != \astring2\a?True if \astring1\a is not equal to "
		"\astring2\a.]"
	"[+\anum1\a -eq \anum2\a?True if numerical value of \anum1\a is "
		"equal to \anum2\a.]"
	"[+\anum1\a -ne \anum2\a?True if numerical value of \anum1\a is not "
		"equal to \anum2\a.]"
	"[+\anum1\a -lt \anum2\a?True if numerical value of \anum1\a is less "
		"than \anum2\a.]"
	"[+\anum1\a -le \anum2\a?True if numerical value of \anum1\a is less "
		"than or equal to \anum2\a.]"
	"[+\anum1\a -gt \anum2\a?True if numerical value of \anum1\a is "
		"greater than \anum2\a.]"
	"[+\anum1\a -ge \anum2\a?True if numerical value of \anum1\a is "
		"greater than or equal to \anum2\a.]"
	"[+\afile1\a -nt \afile2\a?True if \afile1\a is newer than \afile2\a "
		"or \afile2\a does not exist.]"
	"[+\afile1\a -ot \afile2\a?True if \afile1\a is older than \afile2\a "
		"or \afile2\a does not exist.]"
	"[+\afile1\a -ef \afile2\a?True if \afile1\a is another name for "
		"\afile2\a.  This will be true if \afile1\a is a hard link "
		"or a symbolic link to \afile2\a.]"
"}"
"\n"
"\n[expression]\n"
"\n"
"[+EXIT STATUS?]{"
	"[+0?Indicates that the specified expression is True.]"
	"[+1?Indicates that the specified expression is False.]"
	"[+>1?An error occurred.]"
"}"

"[+SEE ALSO?\blet\b(1), \bexpr\b(1)]"
;

const char test_opchars[]	= "HLSVOGCaeohrwxdcbfugk"
#ifdef SHOPT_TEST_L
	"l"
#endif
				"psnzt";
const char e_argument[]		= "argument expected";
const char e_missing[]		= "%s missing";
const char e_badop[]		= "%s: unknown operator";
const char e_tstbegin[]		= "[[ ! ";
const char e_tstend[]		= " ]]\n";
