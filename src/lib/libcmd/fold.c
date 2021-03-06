/*******************************************************************
*                                                                  *
*             This software is part of the ast package             *
*                Copyright (c) 1992-2002 AT&T Corp.                *
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
*                                                                  *
*******************************************************************/
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * fold
 */

static const char usage[] =
"[-?\n@(#)$Id: fold (AT&T Labs Research) 1999-04-10 $\n]"
USAGE_LICENSE
"[+NAME?fold - fold lines]"
"[+DESCRIPTION?\bfold\b is a filter that folds lines from its input, "
	"breaking the lines to have a maximum of \awidth\a column "
	"positions (or bytes if the \b-b\b option is specified).  Lines "
	"are broken by the insertion of a newline character such that "
	"each output line is the maximum width possible that does not "
	"exceed the specified number of column positions, (or bytes).  A line "
	"will not be broken in the middle of a character.] "
"[+?Unless the \b-b\b option is specified, the following will be treated "
	"specially:]{"
	"[+carriage-return?The current count of line width will be set "
		"to zero.  \bfold\b will not insert a newline immediately "
		"before or after a carriage-return.]"
	"[+backspace?If positive, the current count of line width will be "
		"decremented by  one.  \bfold\b will not insert a newline "
		"immediately before or after a backspace.]"
	"[+tab?Each tab character encountered will advance the column "
		"position to the next tab stop.  Tab stops are at each "
		"column position \an\a, where \an\a modulo 8 equals 1.]"
	"}"
"[+?If no \afile\a is given, or if the \afile\a is \b-\b, \bfold\b "
        "reads from standard input.   The start of the file is defined "
        "as the current offset.]"

"[b:bytes?Count bytes rather than columns so that each carriage-return, "
	"backspace, and tab counts as 1.]"
"[s:spaces?Break at word boundaries.  If the line contains any blanks, "
	"(spaces or tabs), within the first \awidth\a column positions or "
	"bytes, the line is broken after the last blank meeting the "
	"\awidth\a constraint.]"
"[w:width]#[width:=80?Use a maximum line length of \awidth\a columns "
	"instead of the default.]"
"\n"
"\n[file ...]\n"
"\n"
"[+EXIT STATUS?]{"
	"[+0?All files processed successfully.]"
	"[+>0?An error occurred.]"
"}"
"[+SEE ALSO?\bpaste\b(1)]"
;


#include <cmdlib.h>

#define WIDTH	80
#define TABSIZE	8

#define T_EOF	1
#define T_NL	2
#define T_BS	3
#define T_TAB	4
#define T_SP	5
#define T_RET	6

static char cols[1<<CHAR_BIT];

static void fold(Sfio_t *in, Sfio_t *out, register int width)
{
	register char *cp, *first;
	register int n, col=0;
	register char *last_space=0;
	cols[0] = 0;
	while(cp  = sfgetr(in,'\n',0))
	{
		/* special case -b since no column adjustment is needed */ 
		if(cols['\b']==0 && (n=sfvalue(in))<=width)
		{
			sfwrite(out,cp,n);
			continue;
		}
		first = cp;
		col = 0;
		last_space = 0;
		while(1)
		{
			while((n=cols[*(unsigned char*)cp++])==0);
			while((cp-first) > (width-col))
			{
				if(last_space)
					col = last_space - first;
				else
					col = width-col;
				sfwrite(out,first,col);
				first += col;
				col = 0;
				last_space = 0;
				if(cp>first+1 || (n!=T_NL && n!=T_BS))
					sfputc(out,'\n');
			}
			if(n==T_NL)
				break;
			switch(n)
			{
			    case T_RET:
				col = 0;
				break;
			    case T_BS:
				if((cp+(--col)-first)>0) 
					col--;
				break;
			    case T_TAB:
				n = (TABSIZE-1) - (cp+col-1-first)&(TABSIZE-1);
				col +=n;
				if((cp-first) > (width-col))
				{
					sfwrite(out,first,(--cp)-first);
					sfputc(out,'\n');
					first = cp;
					col =  TABSIZE-1;
					last_space = 0;
					continue;
				}
				if(cols[' '])
					last_space = cp;
				break;
			    case T_SP:
				last_space = cp;
				break;
			}
		}
		sfwrite(out,first,cp-first);
	}
	if(cp = sfgetr(in,'\n',-1))
		sfwrite(out,cp,sfvalue(in));
}

int
b_fold(int argc, char *argv[], void* context)
{
	register int n, width=WIDTH;
	register Sfio_t *fp;
	register char *cp;

	cmdinit(argv, context, ERROR_CATALOG);
	cols['\t'] = T_TAB;
	cols['\b'] = T_BS;
	cols['\n'] = T_NL;
	cols['\r'] = T_RET;
	while (n = optget(argv, usage)) switch (n)
	{
	    case 'b':
		cols['\r'] = cols['\b'] = 0;
		cols['\t'] = cols[' '];
		break;
	    case 's':
		cols[' '] = T_SP;
		if(cols['\t']==0)
			cols['\t'] = T_SP;
		break;
	    case 'w':
		if((width =  opt_info.num) <= 0)
			error(2, "%d: width must be positive", opt_info.num);
		break;
	    case ':':
		error(2, "%s", opt_info.arg);
		break;
	    case '?':
		error(ERROR_usage(2), "%s", opt_info.arg);
		break;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if(error_info.errors)
		error(ERROR_usage(2),"%s", optusage(NiL));
	if(cp = *argv)
		argv++;
	do
	{
		if(!cp || streq(cp,"-"))
			fp = sfstdin;
		else if(!(fp = sfopen(NiL,cp,"r")))
		{
			error(ERROR_system(0),"%s: cannot open",cp);
			error_info.errors = 1;
			continue;
		}
		fold(fp,sfstdout,width);
		if(fp!=sfstdin)
			sfclose(fp);
	}
	while(cp= *argv++);
	return(error_info.errors);
}

