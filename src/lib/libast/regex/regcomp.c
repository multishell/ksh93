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
 * posix regex compiler
 */

#include "reglib.h"
#include "lclib.h"

#include <cdt.h>

typedef struct Cchr_s
{
	Dtlink_t	lnk;
	unsigned char	nam[2];
	Ckey_t		key;
} Cchr_t;

#define eat(p)		do{if ((p)->token.push)(p)->token.push=0;else (p)->cursor+=(p)->token.len;}while (0)

/*
 * determine whether greedy matching will work, i.e. produce
 * the best match first.  such expressions are "easy", and
 * need no backtracking once a complete match is found.  
 * if an expression has backreferences or alts it's hard
 * else if it has only one closure it's easy
 * else if all closures are simple (i.e. one-character) it's easy
 * else it's hard.
 */

typedef struct Stats_s
{
	unsigned long	l;	/* min length to left of x		*/
	unsigned long	k;	/* min length to left of y		*/
	unsigned long	m;	/* min length				*/
	unsigned int	a;	/* number of alternations		*/
	unsigned short	b;	/* number of backrefs			*/
	unsigned short	c;	/* number of closures			*/
	unsigned short	p;	/* number of named subexpressions	*/
	unsigned short	s;	/* number of simple closures		*/
	unsigned short	t;	/* number of tries			*/
	unsigned short	u;	/* number of unnamed subexpressions	*/
	Rex_t*		x;	/* max length REX_STRING		*/
	Rex_t*		y;	/* max length REX_TRIE			*/
} Stats_t;

typedef struct Token_s
{
	unsigned long	min;
	unsigned long	max;
	short		lex;
	short		len;
	short		esc;
	short		att;
	short		push;
} Token_t;

typedef struct Cenv_s
{
	int		delimiter;	/* pattern delimiter		*/
	int		error;		/* last error			*/
	int		explicit;	/* explicit match on this char	*/
	regflags_t	flags;		/* flags arg to regcomp		*/
	int		type;		/* BRE,ERE,ARE,SRE,KRE		*/
	unsigned char*	cursor;		/* curent point in re		*/
	unsigned char*	pattern;	/* the original pattern		*/
	int		parno;		/* number of last open paren	*/
	int		parnest;	/* paren nest count		*/
	int		posixkludge; 	/* to make * nonspecial		*/
	Token_t		token;		/* token lookahead		*/
	Stats_t		stats;		/* RE statistics		*/
	int		terminator;	/* pattern terminator		*/
	Rex_t*		paren[2*(BACK_REF_MAX+2)];
					/* paren[i]!=0 if \i defined	*/
	regex_t*	regex;		/* user handle			*/
	regdisc_t*	disc;		/* user discipline		*/
} Cenv_t;

/*
 * allocate a new Rex_t node
 */

static Rex_t*
node(Cenv_t* env, int type, int lo, int hi, size_t extra)
{
	register Rex_t*	e;

	if (e = (Rex_t*)alloc(env->disc, 0, sizeof(Rex_t) + extra))
	{
		memset(e, 0, sizeof(Rex_t) + extra);
		e->type = type;
		e->marked = 0;
		e->lo = lo;
		e->hi = hi;
		e->flags = env->flags;
		e->explicit = env->explicit;
		if (extra)
			e->re.data = (char*)e + sizeof(Rex_t);
	}
	return e;
}

/*
 * free a Trie_node_t node
 */

static void
triedrop(regdisc_t* disc, Trie_node_t* e)
{
	if (e)
	{
		triedrop(disc, e->son);
		triedrop(disc, e->sib);
		alloc(disc, e, 0);
	}
}

/*
 * free a Rex_t node
 */

void
drop(regdisc_t* disc, Rex_t* e)
{
	int	i;
	Rex_t*	x;

	if (e && !(disc->re_flags & REG_NOFREE))
		do
		{
			switch (e->type)
			{
			case REX_ALT:
			case REX_CONJ:
				drop(disc, e->re.group.expr.binary.left);
				drop(disc, e->re.group.expr.binary.right);
				break;
			case REX_GROUP:
			case REX_GROUP_AHEAD:
			case REX_GROUP_AHEAD_NOT:
			case REX_GROUP_BEHIND:
			case REX_GROUP_BEHIND_NOT:
			case REX_GROUP_CUT:
			case REX_NEG:
			case REX_REP:
				drop(disc, e->re.group.expr.rex);
				break;
			case REX_TRIE:
				for (i = 0; i <= UCHAR_MAX; i++)
					triedrop(disc, e->re.trie.root[i]);
				break;
			}
			x = e->next;
			alloc(disc, e, 0);
		} while (e = x);
}

/*
 * mark e and descendants minimal
 */

static void
mark(register Rex_t* e, int set)
{
	if (e && !e->marked)
		do
		{
			e->marked = 1;
			if (set)
				e->flags |= REG_MINIMAL;
			else
				e->flags &= ~REG_MINIMAL;
			switch (e->type)
			{
			case REX_ALT:
			case REX_CONJ:
			case REX_GROUP_COND:
				if (e->re.group.expr.binary.left)
					mark(e->re.group.expr.binary.left, set);
				if (e->re.group.expr.binary.right)
					mark(e->re.group.expr.binary.right, set);
				break;
			case REX_GROUP:
			case REX_GROUP_AHEAD:
			case REX_GROUP_AHEAD_NOT:
			case REX_GROUP_BEHIND:
			case REX_GROUP_BEHIND_NOT:
			case REX_GROUP_CUT:
			case REX_NEG:
			case REX_REP:
			case REX_TRIE:
				mark(e->re.group.expr.rex, set);
				break;
			}
		} while (e = e->next);
}

/*
 * assign subexpression numbers by a preorder tree walk
 */

static int
serialize(Cenv_t* env, Rex_t* e, int n)
{
	do
	{
		e->serial = n++;
		switch (e->type)
		{
		case REX_ALT:
		case REX_GROUP_COND:
			if (e->re.group.expr.binary.left)
				n = serialize(env, e->re.group.expr.binary.left, n);
			e->re.group.expr.binary.serial = n++;
			if (e->re.group.expr.binary.right)
				n = serialize(env, e->re.group.expr.binary.right, n);
			break;
		case REX_CONJ:
			n = serialize(env, e->re.group.expr.binary.left, n);
			n = serialize(env, e->re.group.expr.binary.right, n);
			break;
		case REX_GROUP:
		case REX_GROUP_AHEAD:
		case REX_GROUP_AHEAD_NOT:
		case REX_GROUP_BEHIND:
		case REX_GROUP_BEHIND_NOT:
		case REX_GROUP_CUT:
		case REX_NEG:
		case REX_REP:
			n = serialize(env, e->re.group.expr.rex, n);
			break;
		}
	} while (e = e->next);
	return n;
}

/*
 * catenate e and f into a sequence, collapsing them if possible
 */

static Rex_t*
cat(Cenv_t* env, Rex_t* e, Rex_t* f)
{
	Rex_t*	g;

	if (!f)
	{
		drop(env->disc, e);
		return 0;
	}
	if (e->type == REX_NULL)
	{
		drop(env->disc, e);
		return f;
	}
	if (f->type == REX_NULL)
	{
		g = f->next;
		f->next = 0;
		drop(env->disc, f);
		f = g;
	}
	else if (e->type == REX_DOT && f->type == REX_DOT)
	{
		unsigned int	m = e->lo + f->lo;
		unsigned int	n = e->hi + f->hi;

		if (m <= RE_DUP_MAX)
		{
			if (e->hi > RE_DUP_MAX || f->hi > RE_DUP_MAX)
			{
				n = RE_DUP_INF;	
				goto combine;
			}
			else if (n <= RE_DUP_MAX)
			{
			combine:
				e->lo = m;
				e->hi = n;
				g = f->next;
				f->next = 0;
				drop(env->disc, f);
				f = g;
			}
		}
	}
	e->next = f;
	return e;
}

/*
 * collect re statistics
 */

static int
stats(register Cenv_t* env, register Rex_t* e)
{
	register unsigned int	n;
	register unsigned int	m;
	unsigned char		c;
	unsigned char		b;
	unsigned long		l;
	unsigned long		k;
	unsigned long		t;
	Rex_t*			q;
	Rex_t*			x;
	Rex_t*			y;

	do
	{
		switch (e->type)
		{
		case REX_ALT:
			x = env->stats.x;
			l = env->stats.l;
			y = env->stats.y;
			k = env->stats.k;
			t = env->stats.t;
			if (++env->stats.a <= 0)
				return 1;
			n = env->stats.m;
			env->stats.m = 0;
			if (stats(env, e->re.group.expr.binary.left))
				return 1;
			m = env->stats.m;
			env->stats.m = 0;
			if (e->re.group.expr.binary.right && stats(env, e->re.group.expr.binary.right))
				return 1;
			if (env->stats.m > m)
				env->stats.m = m;
			else
				m = env->stats.m;
			if ((env->stats.m += n) < m)
				return 1;
			env->stats.x = x;
			env->stats.l = l;
			env->stats.y = y;
			env->stats.k = k;
			env->stats.t = t;
			break;
		case REX_BACK:
			if (++env->stats.b <= 0)
				return 1;
			break;
		case REX_CLASS:
		case REX_COLL_CLASS:
		case REX_DOT:
		case REX_ONECHAR:
			n = env->stats.m;
			if ((env->stats.m += e->lo) < n)
				return 1;
			if (e->lo != e->hi)
			{
				if (++env->stats.c <= 0)
					return 1;
				if (++env->stats.s <= 0)
					return 1;
			}
			break;
		case REX_CONJ:
			n = env->stats.m;
			env->stats.m = 0;
			if (stats(env, e->re.group.expr.binary.left))
				return 1;
			m = env->stats.m;
			env->stats.m = 0;
			if (stats(env, e->re.group.expr.binary.right))
				return 1;
			if (env->stats.m < m)
				env->stats.m = m;
			else
				m = env->stats.m;
			if ((env->stats.m += n) < m)
				return 1;
			break;
		case REX_GROUP:
			if (e->re.group.number && ++env->stats.p <= 0 || !e->re.group.number && ++env->stats.u <= 0)
				return 1;
			if (stats(env, e->re.group.expr.rex))
				return 1;
			break;
		case REX_GROUP_AHEAD:
		case REX_GROUP_AHEAD_NOT:
		case REX_GROUP_BEHIND:
		case REX_GROUP_BEHIND_NOT:
			m = env->stats.m;
			x = env->stats.x;
			y = env->stats.y;
			if (stats(env, e->re.group.expr.rex))
				return 1;
			env->stats.m = m;
			env->stats.x = x;
			env->stats.y = y;
			switch (e->type)
			{
			case REX_GROUP_AHEAD:
			case REX_GROUP_BEHIND:
				if (++env->stats.u <= 0)
					return 1;
				break;
			}
			break;
		case REX_GROUP_COND:
			if (++env->stats.u <= 0)
				return 1;
			m = env->stats.m;
			x = env->stats.x;
			y = env->stats.y;
			if (e->re.group.size > 0 && ++env->stats.b <= 0)
				return 1;
			if (e->re.group.expr.binary.left && stats(env, e->re.group.expr.binary.left))
				return 1;
			if (q = e->re.group.expr.binary.right)
			{
				if (q->re.group.expr.binary.left && stats(env, q->re.group.expr.binary.left))
					return 1;
				if (q->re.group.expr.binary.right && stats(env, q->re.group.expr.binary.right))
					return 1;
			}
			env->stats.m = m;
			env->stats.x = x;
			env->stats.y = y;
			break;
		case REX_GROUP_CUT:
			if (++env->stats.u <= 0)
				return 1;
			m = env->stats.m;
			x = env->stats.x;
			y = env->stats.y;
			if (stats(env, e->re.group.expr.rex))
				return 1;
			env->stats.m = m;
			env->stats.x = x;
			env->stats.y = y;
			break;
		case REX_NEG:
			x = env->stats.x;
			l = env->stats.l;
			y = env->stats.y;
			k = env->stats.k;
			t = env->stats.t;
			n = env->stats.m;
			env->stats.m = 0;
			if (stats(env, e->re.group.expr.rex))
				return 1;
			env->stats.m = !env->stats.m;
			if ((env->stats.m += n) < n)
				return 1;
			env->stats.x = x;
			env->stats.l = l;
			env->stats.y = y;
			env->stats.k = k;
			env->stats.t = t;
			break;
		case REX_REP:
			x = env->stats.x;
			l = env->stats.l;
			y = env->stats.y;
			k = env->stats.k;
			t = env->stats.t;
			if (++env->stats.c <= 0)
				return 1;
			b = env->stats.b;
			c = env->stats.c;
			n = env->stats.m;
			env->stats.m = 0;
			if (stats(env, e->re.group.expr.rex))
				return 1;
			if (env->stats.m == 1 && b == env->stats.b && c == env->stats.c && ++env->stats.s <= 0)
				return 1;
			m = env->stats.m;
			if ((env->stats.m *= e->lo) > 0 && env->stats.m < m)
				return 1;
			m = env->stats.m;
			if ((env->stats.m += n) < m)
				return 1;
			if (e->lo < 1)
			{
				env->stats.x = x;
				env->stats.l = l;
				env->stats.y = y;
				env->stats.k = k;
				env->stats.t = t;
			}
			break;
		case REX_STRING:
			n = env->stats.m;
			if ((env->stats.m += e->re.string.size) < n)
				return 1;
			if (!env->stats.x || env->stats.x->re.string.size < e->re.string.size)
			{
				env->stats.x = e;
				env->stats.l = n;
			}
			break;
		case REX_TRIE:
			if (++env->stats.s <= 0)
				return 1;
			n = env->stats.m;
			if ((env->stats.m += e->re.trie.min) < n)
				return 1;
			env->stats.t++;
			if (!env->stats.y || env->stats.y->re.trie.min < e->re.trie.min)
			{
				env->stats.y = e;
				env->stats.k = n;
			}
			break;
		}
	} while (e = e->next);
	return 0;
}

static int	token(Cenv_t*);

static int
magic(register Cenv_t* env, register int c, int escaped)
{
	register char*	sp;
	register int	n;
	int		o = c;
	int		e = env->error;
	short*		mp;
	char*		ep;

	if (mp = state.magic[c])
	{
		c = mp[env->type+escaped];
		if (c >= T_META)
		{
			sp = (char*)env->cursor + env->token.len;
			switch (c)
			{
			case T_LEFT:
				n = 0;
				ep = sp;
				while (*sp >= '0' && *sp <= '9')
				{
					if (n > (INT_MAX / 10))
					{
						env->error = REG_BADBR;
						goto bad;
					}
					n = n * 10 + *sp++ - '0';
				}
				if (sp == ep)
				{
					env->error = *sp ? REG_BADBR : REG_EBRACE;
					goto bad;
				}
				else if (n > RE_DUP_MAX)
				{
					env->error = REG_BADBR;
					goto bad;
				}
				env->token.min = n;
				if (*sp == ',')
				{
					n = 0;
					ep = ++sp;
					while (*sp >= '0' && *sp <= '9')
					{
						if (n > (INT_MAX / 10))
						{
							env->error = REG_BADBR;
							goto bad;
						}
						n = n * 10 + *sp++ - '0';
					}
					if (sp == ep)
						n = RE_DUP_INF;
					else if (n < env->token.min)
					{
						env->error = REG_BADBR;
						goto bad;
					}
				}
				env->token.max = n;
				switch (*sp)
				{
				case 0:
					env->error = REG_EBRACE;
					goto bad;
				case '\\':
					if (!escaped)
					{
						env->error = REG_BADBR;
						goto bad;
					}
					sp++;
					break;
				default:
					if (escaped)
					{
						env->error = REG_BADBR;
						goto bad;
					}
					break;
				}
				switch (*sp++)
				{
				case 0:
					env->error = REG_EBRACE;
					goto bad;
				case '}':
					break;
				default:
					env->error = REG_BADBR;
					goto bad;
				}
				env->token.len = sp - (char*)env->cursor;
				break;
			case T_RIGHT:
				env->error = REG_EBRACE;
				goto bad;
			case T_OPEN:
				if (env->type < SRE && *sp == '?')
				{
					env->token.len++;
					goto group;
				}
				break;
			case T_ESCAPE:
				c = chresc(sp - 2, &ep);
				if (ep < sp)
					goto bad;
				env->token.len += ep - sp;
				return c;
			case T_BACK+0:
			case T_BACK+1:
			case T_BACK+2:
			case T_BACK+3:
			case T_BACK+4:
			case T_BACK+5:
			case T_BACK+6:
			case T_BACK+7:
				n = chresc(sp - 2, &ep);
				if (ep > sp + 1)
				{
					env->token.len += ep - sp;
					return n;
				}
				if (env->type == SRE || c == T_BACK && !(env->flags & REG_LENIENT))
				{
					env->error = REG_BADESC;
					goto bad;
				}
				if ((env->flags & REG_MULTIREF) && isdigit(*sp))
				{
					c = (c - T_BACK) * 10 + (*sp - '0');
					if (c > 0 && c <= env->parno && env->paren[c])
						c += T_BACK;
					else
						c = n;
					env->token.len++;
				}
				if (c == T_BACK)
					c = 0;
				break;
			case T_BAD:
				if (escaped == 1 && (env->flags & REG_LENIENT) && (c = mp[env->type+escaped+2]) >= T_META)
					return c;
				goto bad;
			}
			if (env->type >= SRE)
			{
				if (c == T_DOT)
					c = '.';
				else if (c < T_OPEN)
				{
					if (env->type == KRE && *(env->cursor + env->token.len) == '-' && *(env->cursor + env->token.len + 1) == '(')
					{
						env->token.len++;
						env->token.att = 1;
					}
					if (env->type == KRE && *(env->cursor + env->token.len) == '(')
					{
						env->token.len++;
						if (c == T_TILDE)
							goto group;
						if (c != T_AT)
							env->token.lex = c;
						c = T_OPEN;
					}
					else if (c == T_STAR)
						c = T_DOTSTAR;
					else if (c == T_QUES)
						c = T_DOT;
					else if (c == T_PLUS)
						c = '+';
					else if (c == T_BANG)
						c = '!';
					else if (c == T_AT)
						c = '@';
					else if (c == T_TILDE)
						c = '~';
				}
				else if (c > T_BACK)
				{
					c = (c - T_BACK) * 2 - 1;
					c = (c > env->parno || !env->paren[c]) ? o : T_BACK + c;
				}
			}
		}
	}
	else if (escaped == 2)
	{
		if (env->type >= SRE && !(env->flags & REG_SHELL_ESCAPED) || (env->flags & REG_ESCAPE) && (c == '[' || c == '-' || c == ']' || env->delimiter && c == env->delimiter))
			/*ok*/;
		else
		{
			env->error = REG_BADESC;
			goto bad;
		}
	}
	else if (escaped && !(env->flags & REG_LENIENT) && c != ']')
	{
		env->error = REG_BADESC;
		goto bad;
	}
	return c;
 group:
	sp = (char*)env->cursor + env->token.len;
	switch (*sp++)
	{
	case ')':
		break;
	case '#':
		for (;;)
		{
			switch (*sp++)
			{
			case 0:
				env->error = REG_EPAREN;
				return T_BAD;
			case ')':
				break;
			default:
				continue;
			}
			break;
		}
		break;
	default:
		return T_GROUP;
	}
	env->cursor = (unsigned char*)sp;
	return token(env);
 bad:
	if (escaped == 2)
		env->error = e;
	else if (env->flags & REG_LENIENT)
		return o;
	else if (escaped == 1 && !env->error)
	{
		if (mp || o == ']')
			return o;
		env->error = REG_BADESC;
	}
	return T_BAD;
}

static int
token(register Cenv_t* env)
{
	int	c;

	if (env->token.push)
		return env->token.lex;
	env->token.att = env->token.esc = 0;
	if ((env->token.len = mbsize(env->cursor)) > 1)
		return env->token.lex = -1;
	env->token.lex = 0;
	for (;;)
	{
		c = *env->cursor;
		if (c == 0 || c == env->delimiter || c == env->terminator)
			return T_END;
		if (!(env->flags & REG_COMMENT))
			break;
		if (c == '#')
		{
			do
			{
				c = *++env->cursor;
				if (c == 0 || c == env->delimiter)
					return T_END;
			} while (c != '\n');
		}
		else if (!isspace(c))
			break;
		env->cursor++;
	}
	if (c == '\n' && (env->flags & REG_MULTIPLE) && !env->delimiter)
	{
		if (env->parnest)
		{
			env->error = REG_EPAREN;
			return T_BAD;
		}
		env->parno = 0;
		env->pattern = env->cursor + 1;
		return T_BAR;
	}
	if (env->flags & REG_LITERAL)
		return c;
	if (env->posixkludge)
	{
		env->posixkludge = 0;
		if (c == '*')
			return c;
	}
	if (c == '\\')
	{
		if (env->flags & REG_SHELL_ESCAPED)
			return c;
		if (!(c = *(env->cursor + 1)) || c == env->terminator)
		{
			if (env->flags & REG_LENIENT)
			{
				if (c)
				{
					env->token.esc = env->token.len;
					env->token.len += mbsize(env->cursor + 1);
					return c;
				}
				return '\\';
			}
			env->error = REG_EESCAPE;
			return T_BAD;
		}
		env->token.esc = env->token.len;
		env->token.len += mbsize(env->cursor + 1);
		if (env->delimiter && c == 'n')
			return '\n';
		else if (c == env->delimiter)
			return magic(env, c, 0);
		else if (c == '(' && env->type == BRE)
			env->posixkludge = 1;
		else if (c == ')' && env->type == BRE && env->parnest <= 0)
		{
			env->error = REG_EPAREN;
			return T_BAD;
		}
		else if (isspace(c) && (env->flags & REG_COMMENT))
			return c;
		return magic(env, c, 1);
	}
	else if (c == '$')
	{
		if (env->type < SRE && (*(env->cursor + 1) == 0 || *(env->cursor + 1) == env->delimiter || *(env->cursor + 1) == env->terminator) || (env->flags & REG_MULTIPLE) && *(env->cursor + 1) == '\n')
			return T_DOLL;
	}
	else if (c == '^')
	{
		if (env->type == BRE && env->cursor == env->pattern)
		{
			env->posixkludge = 1;
			return T_CFLX;
		}
	}
	else if (c == ')')
	{
		if (env->type != BRE && env->parnest <= 0)
			return c;
	}
	else if (c == '/' && env->explicit == '/')
	{
		while (*(env->cursor + env->token.len) == c)
			env->token.len++;
		return T_SLASHPLUS;
	}
	return magic(env, c, 0);
}

static Celt_t*
col(Celt_t* ce, int ic, unsigned char* bp, int bw, int bc, unsigned char* ep, int ew, int ec)
{
	register unsigned char*	s;
	register unsigned char*	k;
	register unsigned char*	e;
	register int		c;
	register int		cc;
	int			bt;
	int			et;
	Ckey_t			key;

	cc = 0;
	for (;;)
	{
		k = key;
		if (bw == 1)
		{
			c = bc;
			if (ic)
			{
				if (isupper(c))
				{
					c = tolower(c);
					cc = 1;
				}
				else if (islower(c))
				{
					c = toupper(c);
					cc = 1;
				}
			}
			*k++ = c;
		}
		else if (bw < COLL_KEY_MAX)
		{
			s = bp;
			e = s + bw;
			while (s < e)
			{
				c = *s++;
				if (ic)
				{
					if (isupper(c))
					{
						c = tolower(c);
						cc = 1;
					}
					else if (islower(c))
					{
						c = toupper(c);
						cc = 1;
					}
				}
				*k++ = c;
			}
		}
		*k = 0;
		mbxfrm(ce->beg, key, COLL_KEY_MAX);
		if (ep)
		{
			k = key;
			c = mbchar(k);
			if (isupper(c))
				bt = COLL_range_uc;
			else if (islower(c))
				bt = COLL_range_lc;
			else
				bt = COLL_range;
			k = key;
			if (ew == 1)
			{
				c = ec;
				if (ic)
				{
					if (isupper(c))
					{
						c = tolower(c);
						cc = 1;
					}
					else if (islower(c))
					{
						c = toupper(c);
						cc = 1;
					}
				}
				*k++ = c;
			}
			else if (ew < COLL_KEY_MAX)
			{
				s = ep;
				e = s + ew;
				while (s < e)
				{
					c = *s++;
					if (ic)
					{
						if (isupper(c))
						{
							c = tolower(c);
							cc = 1;
						}
						else if (islower(c))
						{
							c = toupper(c);
							cc = 1;
						}
					}
					*k++ = c;
				}
			}
			*k = 0;
			mbxfrm(ce->end, key, COLL_KEY_MAX);
			k = key;
			c = mbchar(k);
			if (isupper(c))
				et = COLL_range_uc;
			else if (islower(c))
				et = COLL_range_lc;
			else
				et = COLL_range;
			ce->typ = bt == et ? bt : COLL_range;
		}
		else
			ce->typ = COLL_char;
		ce++;
		if (!ic || !cc)
			break;
		ic = 0;
	}
	return ce;
}

static Rex_t*
bra(Cenv_t* env)
{
	Rex_t*		e;
	int		c;
	int		i;
	int		w;
	int		neg;
	int		last;
	int		inrange;
	int		ic;
	int		complicated;
	int		collate;
	int		elements;
	unsigned char*	first;
	unsigned char*	start;
	unsigned char*	begin;
	regclass_t	f;
	unsigned char	buf[4 * (COLL_KEY_MAX + 1)];
	char		mbc[COLL_KEY_MAX + 1];

	if (!(e = node(env, REX_CLASS, 1, 1, sizeof(Set_t))))
		return 0;
	collate = complicated = elements = 0;
	first = env->cursor;
	start = first + mbsize(first);
	if (*env->cursor == (env->type >= SRE ? '!' : '^'))
	{
		env->cursor++;
		neg = 1;
	}
	else
		neg = 0;
	if (*env->cursor == 0 || *(env->cursor + 1) == 0 || *env->cursor == env->terminator || *(env->cursor + 1) == env->terminator || (env->flags & REG_ESCAPE) && (*env->cursor == env->delimiter || *env->cursor != '\\' && *(env->cursor + 1) == env->delimiter))
		goto error;
	begin = env->cursor + mbsize(env->cursor);

	/*
	 * inrange: 0=no, 1=possibly, 2=definitely
	 */

	inrange = 0;
	for (;;)
	{
		if (!(c = *env->cursor) || c == env->terminator || (env->flags & REG_ESCAPE) && c == env->delimiter)
			goto error;
		env->cursor += (w = mbsize(env->cursor));
		if (c == '\\')
		{
			if (*env->cursor)
			{
				if (*env->cursor == 'n')
				{
					env->cursor++;
					c = '\n';
				}
				else if (env->type < SRE || !(env->flags & REG_SHELL_ESCAPED))
				{
					env->token.len = 1;
					w = magic(env, *env->cursor, 2);
					if (env->token.len > 1 || w != T_BAD)
					{
						if (env->token.len == 1 && (f = classfun(w)))
						{
							if (inrange > 1)
							{
								if (env->type < SRE && !(env->flags & REG_LENIENT))
									goto erange;
								inrange = 0;
							}
							env->cursor++;
							for (c = 0; c <= UCHAR_MAX; c++)
								if ((*f)(c))
									setadd(e->re.charclass, c);
							complicated++;
							elements++;
							continue;
						}
						if (env->token.len > 1 || w >= 0 && w < T_META)
						{
							c = w;
							if (c > UCHAR_MAX)
							{
								if (env->type < SRE && !(env->flags & REG_LENIENT) && !mbwide())
									goto erange;
								c = UCHAR_MAX;
							}
							env->cursor += env->token.len;
						}
					}
				}
			}
		}
		else if (c == ']')
		{
			if (env->cursor == begin)
			{
				last = c;
				inrange = 1;
				continue;
			}
			if (inrange != 0)
			{
				setadd(e->re.charclass, last);
				elements++;
				if (inrange == 2)
				{
					setadd(e->re.charclass, '-');
					elements++;
				}
			}
			break;
		}
		else if (c == '-')
		{
			if (!inrange && env->cursor != begin && *env->cursor != ']')
			{
				if (env->type < SRE && !(env->flags & REG_LENIENT))
					goto erange;
				continue;
			}
			else if (inrange == 1)
			{
				inrange = 2;
				complicated++;
				continue;
			}
		}
		else if (c == '[')
		{
			switch (*env->cursor)
			{
			case 0:
				goto error;
			case ':':
				if (inrange == 1)
				{
					setadd(e->re.charclass, last);
					elements++;
				}
				if (!(f = regclass((char*)env->cursor, (char**)&env->cursor)))
				{
					if (env->cursor == start && (c = *(env->cursor + 1)) && *(env->cursor + 2) == ':' && *(env->cursor + 3) == ']' && *(env->cursor + 4) == ']')
					{
						switch (c)
						{
						case '<':
							i = REX_WBEG;
							break;
						case '>':
							i = REX_WEND;
							break;
						default:
							i = 0;
							break;
						}
						if (i)
						{
							env->cursor += 5;
							drop(env->disc, e);
							return node(env, i, 0, 0, 0);
						}
					}
					env->error = REG_ECTYPE;
					goto error;
				}
				for (c = 0; c <= UCHAR_MAX; c++)
					if ((*f)(c))
						setadd(e->re.charclass, c);
				inrange = 0;
				complicated++;
				elements++;
				continue;
			case '=':
				if (inrange == 2)
					goto erange;
				if (inrange == 1)
				{
					setadd(e->re.charclass, last);
					elements++;
				}
				if ((c = regcollate((char*)env->cursor, (char**)&env->cursor, (char*)buf, sizeof(buf))) < 0)
					goto ecollate;
				if (c > 1)
					collate++;
				else
					setadd(e->re.charclass, buf[0]);
				c = buf[0];
				inrange = 0;
				complicated++;
				elements++;
				continue;
			case '.':
				if ((c = regcollate((char*)env->cursor, (char**)&env->cursor, (char*)buf, sizeof(buf))) < 0)
					goto ecollate;
				if (c > 1)
					collate++;
				c = buf[0];
				complicated++;
				break;
			default:
				if (*env->cursor == env->terminator || *env->cursor == env->delimiter && (env->flags & REG_ESCAPE))
					goto error;
				break;
			}
		}
		else if (w > 1)
			complicated++;
		if (inrange == 2)
		{
			if (last > c)
			{
				if (env->type < SRE && !(env->flags & REG_LENIENT))
					goto erange;
				setadd(e->re.charclass, last);
				setadd(e->re.charclass, c);
			}
			else
				for (i = last; i <= c; i++)
					setadd(e->re.charclass, i);
			inrange = env->type >= SRE || (env->flags & REG_LENIENT);
			elements += 2;
		}
		else if (inrange == 1)
		{
			setadd(e->re.charclass, last);
			elements++;
		}
		else
			inrange = 1;
		last = c;
	}
	if (complicated && mbcoll())
	{
		Dt_t*			dt;
		Cchr_t*			cc;
		Cchr_t*			tc;
		Cchr_t*			xc;
		Celt_t*			ce;
		Cchr_t			key;
		int			cw;
		int			rw;
		int			rc;
		unsigned char*		rp;
		unsigned char*		pp;

		static Dtdisc_t		disc;

		static const char	primary[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

		if (!(dt = (Dt_t*)LCINFO(AST_LC_COLLATE)->data))
		{
			disc.key = offsetof(Cchr_t, key);
			if ((cc = newof(0, Cchr_t, elementsof(primary) + 2, 0)) && (dt = dtopen(&disc, Dttree)))
			{
				for (i = 0; i < elementsof(primary); i++, cc++)
				{
					cc->nam[0] = primary[i];
					mbxfrm(cc->key, cc->nam, COLL_KEY_MAX);
					dtinsert(dt, cc);
				}
				dtinsert(dt, cc);
				cc++;
				for (i = 0; i < elementsof(cc->key); i++)
					cc->key[i] = ~0;
				dtinsert(dt, cc);
				LCINFO(AST_LC_COLLATE)->data = (void*)dt;
			}
			else
			{
				if (cc)
					free(cc);
				drop(env->disc, e);
				return 0;
			}
		}
		if (dt)
		{
			drop(env->disc, e);
			if (ic = env->flags & REG_ICASE)
				elements *= 2;
			if (!(e = node(env, REX_COLL_CLASS, 1, 1, (elements + 1) * sizeof(Celt_t))))
				return 0;
			ce = (Celt_t*)e->re.data;
			e->re.collate.invert = neg;
			e->re.collate.elements = ce;
			env->cursor = first;
			inrange = 0;
			for (;;)
			{
				if ((c = *env->cursor) == 0 || c == env->terminator || (env->flags & REG_ESCAPE) && c == env->delimiter)
					goto error;
				pp = env->cursor;
				env->cursor += (w = mbsize(env->cursor));
				if (c == '\\')
				{
					if (*env->cursor)
					{
						if (*env->cursor == 'n')
						{
							pp = env->cursor++;
							c = '\n';
						}
						else if (env->type < SRE || !(env->flags & REG_SHELL_ESCAPED))
						{
							env->token.len = 1;
							w = magic(env, *env->cursor, 2);
							if (env->token.len > 1 || w != T_BAD)
							{
								if (env->token.len == 1 && (f = classfun(w)))
								{
									if (inrange > 1)
									{
										if (env->type < SRE && !(env->flags & REG_LENIENT))
											goto erange;
										inrange = 0;
									}
									env->cursor++;
									ce->fun = f;
									ce->typ = COLL_call;
									ce++;
									continue;
								}
								if (env->token.len > 1 || w >= 0 && w < T_META)
								{
									c = w;
									w = wctomb(mbc, c);
									pp = (unsigned char*)mbc;
									env->cursor += env->token.len;
								}
							}
						}
					}
				}
				else if (c == ']')
				{
					if (env->cursor == begin)
					{
						rp = pp;
						rw = w;
						inrange = 1;
						continue;
					}
					if (inrange != 0)
					{
						ce = col(ce, ic, rp, rw, rc, NiL, 0, 0);
						if (inrange == 2)
							ce = col(ce, ic, NiL, 1, '-', NiL, 0, 0);
					}
					break;
				}
				else if (c == '-')
				{
					if (!inrange && env->cursor != begin && *env->cursor != ']')
					{
						if (env->type < SRE && !(env->flags & REG_LENIENT))
							goto erange;
						continue;
					}
					else if (inrange == 1)
					{
						inrange = 2;
						continue;
					}
				}
				else if (c == '[')
				{
					switch (*env->cursor)
					{
					case 0:
						goto error;
					case ':':
						if (inrange == 1)
							ce = col(ce, ic, rp, rw, rc, NiL, 0, 0);
						if (!(f = regclass((char*)env->cursor, (char**)&env->cursor)))
						{
							if (env->cursor == start && (c = *(env->cursor + 1)) && *(env->cursor + 2) == ':' && *(env->cursor + 3) == ']' && *(env->cursor + 4) == ']')
							{
								switch (c)
								{
								case '<':
									i = REX_WBEG;
									break;
								case '>':
									i = REX_WEND;
									break;
								default:
									i = 0;
									break;
								}
								if (i)
								{
									env->cursor += 5;
									drop(env->disc, e);
									return node(env, i, 0, 0, 0);
								}
							}
							env->error = REG_ECTYPE;
							goto error;
						}
						ce->fun = f;
						ce->typ = COLL_call;
						ce++;
						inrange = 0;
						continue;
					case '=':
						if (inrange == 2)
							goto erange;
						if (inrange == 1)
							ce = col(ce, ic, rp, rw, rc, NiL, 0, 0);
						rp = env->cursor + 1;
						if ((rw = regcollate((char*)env->cursor, (char**)&env->cursor, (char*)buf, sizeof(buf) - 1)) < 0)
							goto ecollate;
						c = 0;
						if (ic)
							for (i = 0; i < rw; i += mbsize(buf+i))
								if (isupper(buf[i]))
								{
									buf[i] = tolower(buf[i]);
									c = 1;
								}
								else if (islower(buf[i]))
									c = 1;
						for (;;)
						{
							mbxfrm(key.key, buf, COLL_KEY_MAX);
							if (!(cc = (Cchr_t*)dtsearch(dt, &key)))
							{
								if (!isalnum(buf[0]))
								{
									strcpy((char*)ce->beg, (char*)key.key);
									ce->typ = COLL_char;
									ce++;
									break;
								}
								if (!(cc = (Cchr_t*)dtprev(dt, &key)))
									goto ecollate;
							}
							xc = cc;
							if (islower(buf[0]))
							{
								ce->typ = COLL_range_lc;
								if ((tc = (Cchr_t*)dtprev(dt, cc)) && !strcasecmp((char*)tc->nam, (char*)cc->nam))
									xc = tc;
							}
							else if (isupper(buf[0]))
							{
								ce->typ = COLL_range_uc;
								if ((tc = (Cchr_t*)dtprev(dt, cc)) && !strcasecmp((char*)tc->nam, (char*)cc->nam))
									xc = tc;
							}
							else
							{
								ce->typ = COLL_range;
								xc = cc;
							}
							strcpy((char*)ce->beg, (char*)xc->key);
							if (!(cc = (Cchr_t*)dtnext(dt, cc)))
								goto ecollate;
							if (!strcasecmp((char*)xc->nam, (char*)cc->nam) && (tc = (Cchr_t*)dtnext(dt, cc)))
								cc = tc;
							strcpy((char*)ce->end, (char*)cc->key);
							ce->max = -1;
							ce++;
							if (!c)
								break;
							c = 0;
							for (i = 0; i < rw; i++)
								if (islower(buf[i]))
									buf[i] = toupper(buf[i]);
						}
						inrange = 0;
						c = buf[0];
						continue;
					case '.':
						pp = env->cursor + 1;
						if ((w = regcollate((char*)env->cursor, (char**)&env->cursor, (char*)buf, sizeof(buf) - 1)) < 0)
							goto ecollate;
						if (w > 1)
						{
							if (w > COLL_KEY_MAX)
								goto ecollate;
							if (ic)
								for (i = 0; i < w; i += mbsize(buf+i))
									if (isupper(buf[i]))
										buf[i] = tolower(buf[i]);
							cw = mbxfrm(ce->beg, buf, COLL_KEY_MAX);
							buf[1] = 0;
							if (mbxfrm(ce->beg, buf, COLL_KEY_MAX) < cw)
								goto ecollate;
						}
						c = buf[0];
						break;
					default:
						if (*env->cursor == env->terminator || *env->cursor == env->delimiter && (env->flags & REG_ESCAPE))
							goto error;
						break;
					}
				}
				if (inrange == 2)
				{
					ce = col(ce, ic, rp, rw, rc, pp, w, c);
					if (strcmp((char*)ce->beg, (char*)ce->end) > 0)
					{
						if (env->type < SRE && !(env->flags & REG_LENIENT))
							goto erange;
						(ce-1)->typ = COLL_char;
						strcpy((char*)ce->beg, (char*)(ce-1)->end);
						ce->typ = COLL_char;
						ce++;
					}
					inrange = env->type >= SRE || (env->flags & REG_LENIENT);
				}
				else if (inrange == 1)
					ce = col(ce, ic, rp, rw, rc, NiL, 0, 0);
				else
					inrange = 1;
				rp = pp;
				rw = w;
				rc = c;
			}
			ce->typ = COLL_end;
			return e;
		}
	}
	if (collate)
		goto ecollate;
	if (env->flags & REG_ICASE)
		for (i = 0; i <= UCHAR_MAX; i++)
			if (settst(e->re.charclass, i))
			{
				if (isupper(i))
					c = tolower(i);
				else if (islower(i))
					c = toupper(i);
				else
					continue;
				setadd(e->re.charclass, c);
			}
	if (neg)
	{
		for (i = 0; i < elementsof(e->re.charclass->bits); i++)
			e->re.charclass->bits[i] ^= ~0;
		if (env->explicit)
			setclr(e->re.charclass, env->explicit);
	}
	return e;
 ecollate:
	env->error = REG_ECOLLATE;
	goto error;
 erange:
	env->error = REG_ERANGE;
 error:
	drop(env->disc, e);
	if (!env->error)
		env->error = REG_EBRACK;
	return 0;
}

static Rex_t*
ccl(Cenv_t* env, int type)
{
	int		i;
	Rex_t*		e;
	Celt_t*		ce;
	regclass_t	f;

	if (!(f = classfun(type)))
	{
		env->error = REG_BADESC;
		return 0;
	}
	if (!mbcoll())
	{
		if (!(e = node(env, REX_CLASS, 1, 1, sizeof(Set_t))))
			return 0;
		for (i = 0; i <= UCHAR_MAX; i++)
			if ((*f)(i))
				setadd(e->re.charclass, i);
		if (env->explicit)
			setclr(e->re.charclass, env->explicit);
	}
	else
	{
		if (!(e = node(env, REX_COLL_CLASS, 1, 1, 2 * sizeof(Celt_t))))
			return 0;
		ce = (Celt_t*)e->re.data;
		e->re.collate.invert = 0;
		e->re.collate.elements = ce;
		ce->typ = COLL_call;
		ce->fun = f;
		ce++;
		ce->typ = COLL_end;
	}
	return e;
}

static Rex_t*
rep(Cenv_t* env, Rex_t* e, int number, int last)
{
	Rex_t*		f;
	unsigned long	m = 0;
	unsigned long	n = RE_DUP_INF;
	int		minimal = -1;

	if (!e)
		return 0;
	switch (token(env))
	{
	case T_BANG:
		eat(env);
		if (!(f = node(env, REX_NEG, m, n, 0)))
		{
			drop(env->disc, e);
			return 0;
		}
		f->re.group.expr.rex = e;
		return f;
	case T_QUES:
		eat(env);
		n = 1;
		break;
	case T_STAR:
		eat(env);
		break;
	case T_PLUS:
		eat(env);
		m = 1;
		break;
	case T_LEFT:
		eat(env);
		m = env->token.min;
		n = env->token.max;
		break;
	default:
		return e;
	}
	if (env->token.att)
		minimal = 1;
	else if (env->type < SRE)
		switch (token(env))
		{
		case T_QUES:
			eat(env);
			minimal = !(env->flags & REG_MINIMAL);
			break;
		case T_STAR: /*AST*/
			eat(env);
			minimal = !!(env->flags & REG_MINIMAL);
			break;
		}
	switch (e->type)
	{
	case REX_DOT:
	case REX_CLASS:
	case REX_COLL_CLASS:
	case REX_ONECHAR:
		e->lo = m;
		e->hi = n;
		if (minimal >= 0)
			mark(e, minimal);
		return e;
	case REX_BEG:
	case REX_BEG_STR:
	case REX_END_STR:
	case REX_FIN_STR:
	case REX_WBEG:
	case REX_WEND:
	case REX_WORD:
	case REX_WORD_NOT:
		env->error = REG_BADRPT;
		drop(env->disc, e);
		return 0;
	}
	if (m == 1 && n == 1)
	{
		if (minimal >= 0)
			mark(e, minimal);
		return e;
	}
	if (!(f = node(env, REX_REP, m, n, 0)))
	{
		drop(env->disc, e);
		return 0;
	}
	f->re.group.expr.rex = e;
	f->re.group.number = number;
	f->re.group.last = last;
	if (minimal >= 0)
		mark(f, minimal);
	if (m <= n && n)
	{
		for (; e && e->type >= REX_GROUP && e->type <= REX_GROUP_CUT; e = e->re.group.expr.rex);
		if (e && e->type == REX_NEG)
			f->type = REX_GROUP;
	}
	return f;
}

static int
isstring(Rex_t* e)
{
	switch (e->type)
	{
	case REX_ONECHAR:
		return e->lo == 1 && e->hi == 1;
	case REX_STRING:
		return 1;
	}
	return 0;
}

static Trie_node_t*
trienode(Cenv_t* env, int c)
{
	Trie_node_t*	t;

	if (t = (Trie_node_t*)alloc(env->disc, 0, sizeof(Trie_node_t)))
	{
		memset(t, 0, sizeof(Trie_node_t));
		t->c = c;
	}
	return t;
}

static int
insert(Cenv_t* env, Rex_t* f, Rex_t* g)
{
	unsigned char*	s;
	unsigned char*	e;
	Trie_node_t*	t;
	int		len;
	unsigned char	tmp[2];

	switch (f->type)
	{
	case REX_ONECHAR:
		*(s = tmp) = f->re.onechar;
		e = s + 1;
		break;
	case REX_STRING:
		s = f->re.string.base;
		e = s + f->re.string.size;
		break;
	default:
		return 1;
	}
	if (!(t = g->re.trie.root[*s]) && !(t = g->re.trie.root[*s] = trienode(env, *s)))
		return 1;
	for (len = 1;;)
	{
		if (t->c == *s)
		{
			if (++s >= e)
				break;
			if (!t->son && !(t->son = trienode(env, *s)))
				return 1;
			t = t->son;
			len++;
		}
		else
		{
			if (!t->sib && !(t->sib = trienode(env, *s)))
				return 1;
			t = t->sib;
		}
	}
	if (len < g->re.trie.min)
		g->re.trie.min = len;
	t->end = 1;
	return 0;
}

/*
 * trie() tries to combine nontrivial e and f into a REX_TRIE
 * unless 0 is returned, e and f are deleted as far as possible
 * also called by regcomb
 */

static Rex_t*
trie(Cenv_t* env, Rex_t* e, Rex_t* f)
{
	Rex_t*	g;

	if (e->next || f->next || !isstring(e) || e->flags != f->flags)
		return 0;
	if (isstring(f))
	{
		if (!(g = node(env, REX_TRIE, 0, 0, (UCHAR_MAX + 1) * sizeof(Trie_node_t*))))
			return 0;
		g->re.trie.min = INT_MAX;
		if (insert(env, f, g))
			goto nospace;
		drop(env->disc, f);
	}
	else if (f->type != REX_TRIE)
		return 0;
	else
		g = f;
	if (insert(env, e, g))
		goto nospace;
	drop(env->disc, e);
	return g;
 nospace:
	if (g != f)
		drop(env->disc, g);
	return 0;
}

static Rex_t*		alt(Cenv_t*, int, int);

/*
 * open the pearly gates
 */

static Rex_t*
grp(Cenv_t* env, int parno)
{
	Rex_t*		e;
	Rex_t*		f;
	int		c;
	int		i;
	int		n;
	int		x;
	unsigned char*	p;

	if (c = *env->cursor)
		env->cursor++;
	env->token.len = 0;
	env->parnest++;
	switch (c)
	{
	case '-':
	case '+':
	case 'g':
	case 'i':
	case 'l':
	case 'm':
	case 's':
	case 'x':
	case 'A':
	case 'B':
	case 'E':
	case 'K':
	case 'S':
	case 'U':	/* pcre */
	case 'X':	/* pcre */
		x = REX_GROUP;
		i = 1;
		env->token.push = 1;
		for (;;)
		{
			switch (c)
			{
			case 0:
			case T_CLOSE:
				x = 0;
				goto done;
			case ':':
				eat(env);
				if (token(env) == T_CLOSE)
					x = 0;
				goto done;
			case '-':
				i = 0;
				break;
			case '+':
				i = 1;
				break;
			case 'g':
				if (i)
					env->flags &= ~REG_MINIMAL;
				else
					env->flags |= REG_MINIMAL;
				break;
			case 'i':
				if (i)
					env->flags |= REG_ICASE;
				else
					env->flags &= ~REG_ICASE;
				break;
			case 'l':
				if (i)
					env->flags |= REG_LENIENT;
				else
					env->flags &= ~REG_LENIENT;
				break;
			case 'm':
				if (i)
					env->flags |= REG_NEWLINE;
				else
					env->flags &= ~REG_NEWLINE;
				if (env->type < SRE)
					env->explicit = (env->flags & (REG_NEWLINE|REG_SPAN)) == REG_NEWLINE ? '\n' : -1;
				break;
			case 's':
				if (i)
					env->flags |= REG_SPAN;
				else
					env->flags &= ~REG_SPAN;
				if (env->type < SRE)
					env->explicit = (env->flags & (REG_NEWLINE|REG_SPAN)) == REG_NEWLINE ? '\n' : -1;
				break;
			case 'x':
				if (i)
					env->flags |= REG_COMMENT;
				else
					env->flags &= ~REG_COMMENT;
				break;
			case 'A':
				env->flags &= ~(REG_AUGMENTED|REG_EXTENDED|REG_SHELL);
				env->flags |= REG_AUGMENTED|REG_EXTENDED;
				env->type = ARE;
				break;
			case 'B':
				env->flags &= ~(REG_AUGMENTED|REG_EXTENDED|REG_SHELL);
				env->type = BRE;
				break;
			case 'E':
				env->flags &= ~(REG_AUGMENTED|REG_EXTENDED|REG_SHELL);
				env->flags |= REG_EXTENDED;
				env->type = ERE;
				break;
			case 'K':
				env->flags &= ~(REG_AUGMENTED|REG_EXTENDED|REG_SHELL);
				env->flags |= REG_AUGMENTED|REG_SHELL;
				env->type = KRE;
				break;
			case 'S':
				env->flags &= ~(REG_AUGMENTED|REG_EXTENDED|REG_SHELL);
				env->flags |= REG_SHELL;
				env->type = SRE;
				break;
			case 'U': /* PCRE_UNGREEDY */
				if (i)
					env->flags |= REG_MINIMAL;
				else
					env->flags &= ~REG_MINIMAL;
				break;
			case 'X': /* PCRE_EXTRA */
				break;
			default:
				env->error = REG_BADRPT;
				return 0;
			}
			eat(env);
			c = token(env);
		}
	done:
		break;
	case ':':
		x = REX_GROUP;
		break;
	case '=':
		x = REX_GROUP_AHEAD;
		break;
	case '!':
	case T_BANG:
		x = REX_GROUP_AHEAD_NOT;
		break;
	case '<':
	case T_LT:
		switch (token(env))
		{
		case '=':
			x = REX_GROUP_BEHIND;
			break;
		case '!':
		case T_BANG:
			x = REX_GROUP_BEHIND_NOT;
			break;
		default:
			env->error = REG_BADRPT;
			return 0;
		}
		eat(env);
		break;
	case '>':
	case T_GT:
		x = REX_GROUP_CUT;
		break;
	case '~':
	case T_TILDE:
		if (env->type < SRE || *env->cursor++ != '(')
		{
			env->error = REG_BADRPT;
			return 0;
		}
		/*FALLTHROUGH*/
	case '(':
	case T_OPEN:
		c = 0;
		if (isdigit(*env->cursor))
		{
			f = 0;
			do
			{
				if (c > (INT_MAX / 10))
				{
					env->error = REG_BADRPT;
					return 0;
				}
				c = c * 10 + (*env->cursor++ - '0');
			} while (isdigit(*env->cursor));
			if (*env->cursor++ != ')')
			{
				env->error = REG_BADRPT;
				return 0;
			}
			if (c && env->type >= SRE)
				c = c * 2 - 1;
			if (!c || c > env->parno || !env->paren[c])
			{
				if (!(env->flags & REG_LENIENT))
				{
					env->error = REG_ESUBREG;
					return 0;
				}
				if (c)
					c = -1;
			}
		}
		else
		{
			if (env->type < SRE && *env->cursor++ != '?')
			{
				env->error = REG_BADRPT;
				return 0;
			}
			if (!(f = grp(env, parno + 1)) && env->error)
				return 0;
		}
		if (!(e = node(env, REX_GROUP_COND, 0, 0, 0)))
		{
			drop(env->disc, f);
			return 0;
		}
		e->re.group.size = c;
		e->re.group.expr.binary.left = f;
		if (!(e->re.group.expr.binary.right = alt(env, parno, 1)))
		{
			drop(env->disc, e);
			return 0;
		}
		if (token(env) != T_CLOSE)
		{
			env->error = REG_EPAREN;
			return 0;
		}
		eat(env);
		env->parnest--;
		return rep(env, e, parno, parno);
	case '{':
		p = env->cursor;
		n = 1;
		while (c = *env->cursor)
		{
			if (c == '\\' && *(env->cursor + 1))
				env->cursor++;
			else if (c == '{')
				n++;
			else if (c == '}' && !--n)
				break;
			else if (c == env->delimiter || c == env->terminator)
				break;
			env->cursor++;
		}
		if (c != '}')
		{
			env->error = REG_EBRACE;
			return 0;
		}
		if (*++env->cursor != ')')
		{
			env->error = REG_EPAREN;
			return 0;
		}
		env->cursor++;
		env->parnest--;
		if (env->disc->re_version < REG_VERSION_EXEC)
		{
			env->error = REG_BADRPT;
			return 0;
		}
		if (!env->disc->re_execf)
			return 0;
		if (!(e = node(env, REX_EXEC, 0, 0, 0)))
			return 0;
		e->re.exec.text = (const char*)p;
		e->re.exec.size = env->cursor - p - 2;
		if (!env->disc->re_compf)
			e->re.exec.data = 0;
		else
			e->re.exec.data = (*env->disc->re_compf)(env->regex, e->re.exec.text, e->re.exec.size, env->disc);
		return e;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		c -= '0';
		while (isdigit(*env->cursor))
		{
			if (c > (INT_MAX / 10))
			{
				env->error = REG_ESUBREG;
				return 0;
			}
			c = c * 10 + *env->cursor++ - '0';
		}
		if (*env->cursor == ')')
		{
			env->cursor++;
			env->parnest--;
			env->token.len = 1;
			if (c > env->parno || !env->paren[c])
			{
				env->error = REG_ESUBREG;
				return 0;
			}
			env->paren[c]->re.group.back = 1;
			return rep(env, node(env, REX_BACK, c, 0, 0), 0, 0);
		}
		/*FALLTHROUGH*/
	default:
		env->error = REG_BADRPT;
		return 0;
	}
	if (x && !(e = alt(env, parno, 0)))
		return 0;
	c = token(env);
	env->parnest--;
	if (c != T_CLOSE)
	{
		env->error = REG_EPAREN;
		return 0;
	}
	eat(env);
	if (!x)
		return 0;
	if (!(f = node(env, x, 0, 0, 0)))
	{
		drop(env->disc, e);
		return 0;
	}
	f->re.group.expr.rex = e;
	if (x == REX_GROUP_BEHIND || x == REX_GROUP_BEHIND_NOT)
	{
		if (stats(env, e))
		{
			drop(env->disc, f);
			if (!env->error)
				env->error = REG_ECOUNT;
			return 0;
		}
		f->re.group.size = env->stats.m;
		memset(&env->stats, 0, sizeof(env->stats));
	}
	switch (x)
	{
	case REX_GROUP:
	case REX_GROUP_CUT:
		f = rep(env, f, parno, env->parno);
		break;
	}
	return f;
}

static Rex_t*
seq(Cenv_t* env)
{
	Rex_t*		e;
	Rex_t*		f;
	Token_t		tok;
	int		c;
	int		i;
	int		n;
	int		x;
	int		parno;
	int		type;
	regflags_t	flags;
	unsigned char*	s;
	unsigned char*	p;
	unsigned char*	t;
	unsigned char*	u;
	unsigned char	buf[256];

	for (;;)
	{
		s = buf;
		while ((c = token(env)) < T_META && s < &buf[sizeof(buf) - env->token.len])
		{	
			x = c;
			p = env->cursor;
			if (c >= 0)
			{
				n = 1;
				*s++ = (env->flags & REG_ICASE) ? toupper(c) : c;
			}
			else
			{
				n = env->token.len - env->token.esc;
				for (t = p, u = s + n; s < u; *s++ = *t++);
			}
			eat(env);
		}
		if (c == T_BAD)
			return 0;
		if (s > buf) switch (c)
		{
		case T_STAR:
		case T_PLUS:
		case T_LEFT:
		case T_QUES:
		case T_BANG:
			if ((s -= n) == buf)
				e = 0;
			else
			{
				i = s - buf;
				if (!(e = node(env, REX_STRING, 0, 0, i)))
					return 0;
				memcpy((char*)(e->re.string.base = (unsigned char*)e->re.data), (char*)buf, i);
				e->re.string.size = i;
			}
			if (x >= 0)
			{
				if (!(f = node(env, REX_ONECHAR, 1, 1, 0)))
				{
					drop(env->disc, e);
					return 0;
				}
				f->re.onechar = (env->flags & REG_ICASE) ? toupper(x) : x;
			}
			else
			{
				if (!(e = node(env, REX_STRING, 0, 0, n)))
					return 0;
				memcpy((char*)(e->re.string.base = (unsigned char*)e->re.data), (char*)p, n);
				e->re.string.size = n;
			}
			if (!(f = rep(env, f, 0, 0)) || !(f = cat(env, f, seq(env))))
			{
				drop(env->disc, e);
				return 0;
			}
			if (e)
				f = cat(env, e, f);
			return f;
		default:
			c = s - buf;
			if (!(e = node(env, REX_STRING, 0, 0, c)))
				return 0;
			memcpy((char*)(e->re.string.base = (unsigned char*)e->re.data), (char*)buf, c);
			e->re.string.size = c;
			return cat(env, e, seq(env));
		}
		else if (c > T_BACK)
		{
			eat(env);
			c -= T_BACK;
			if (c > env->parno || !env->paren[c])
			{
				env->error = REG_ESUBREG;
				return 0;
			}
			env->paren[c]->re.group.back = 1;
			e = rep(env, node(env, REX_BACK, c, 0, 0), 0, 0);
		}
		else switch (c)
		{
		case T_AND:
		case T_CLOSE:
		case T_BAR:
		case T_END:
			return node(env, REX_NULL, 0, 0, 0);
		case T_DOLL:
			eat(env);
			e = rep(env, node(env, REX_END, 0, 0, 0), 0, 0);
			break;
		case T_CFLX:
			eat(env);
			if ((e = node(env, REX_BEG, 0, 0, 0)) && (env->flags & REG_EXTENDED))
				e = rep(env, e, 0, 0);
			break;
		case T_OPEN:
			tok = env->token;
			eat(env);
			flags = env->flags;
			type = env->type;
			if (env->token.att)
				env->flags |= REG_MINIMAL;
			env->parnest++;
			if (env->type == KRE)
				++env->parno;
			parno = ++env->parno;
			if (!(e = alt(env, parno + 1, 0)))
				break;
			if (e->type == REX_NULL && env->type == ERE && !(env->flags & REG_NULL))
			{
				drop(env->disc, e);
				env->error = (*env->cursor == 0 || *env->cursor == env->delimiter || *env->cursor == env->terminator) ? REG_EPAREN : REG_ENULL;
				return 0;
			} 
			if (token(env) != T_CLOSE)
			{
				drop(env->disc, e);
				env->error = REG_EPAREN;
				return 0;
			} 
			env->parnest--;
			eat(env);
			if (!(f = node(env, REX_GROUP, 0, 0, 0)))
			{
				drop(env->disc, e);
				return 0;
			}
			if (parno < elementsof(env->paren))
				env->paren[parno] = f;
			f->re.group.back = 0;
			f->re.group.number = parno;
			f->re.group.expr.rex = e;
			if (tok.lex)
			{
				tok.push = 1;
				env->token = tok;
			}
			if (!(e = rep(env, f, parno, env->parno)))
				return 0;
			if (env->type == KRE)
			{
				if (!(f = node(env, REX_GROUP, 0, 0, 0)))
				{
					drop(env->disc, e);
					return 0;
				}
				if (--parno < elementsof(env->paren))
					env->paren[parno] = f;
				f->re.group.back = 0;
				f->re.group.number = parno;
				f->re.group.expr.rex = e;
				e = f;
			}
			env->flags = flags;
			env->type = type;
			break;
		case T_GROUP:
			eat(env);
			flags = env->flags;
			type = env->type;
			if (!(e = grp(env, env->parno + 1)))
			{
				if (env->error)
					return 0;
				continue;
			}
			env->flags = flags;
			env->type = type;
			break;
		case T_BRA:
			eat(env);
			if (e = bra(env))
				e = rep(env, e, 0, 0);
			break;
		case T_ALNUM:
		case T_ALNUM_NOT:
		case T_DIGIT:
		case T_DIGIT_NOT:
		case T_SPACE:
		case T_SPACE_NOT:
			eat(env);
			if (e = ccl(env, c))
				e = rep(env, e, 0, 0);
			break;
		case T_LT:
			eat(env);
			e = rep(env, node(env, REX_WBEG, 0, 0, 0), 0, 0);
			break;
		case T_GT:
			eat(env);
			e = rep(env, node(env, REX_WEND, 0, 0, 0), 0, 0);
			break;
		case T_DOT:
			eat(env);
			e = rep(env, node(env, REX_DOT, 1, 1, 0), 0, 0);
			break;
		case T_DOTSTAR:
			eat(env);
			env->token.lex = T_STAR;
			env->token.push = 1;
			e = rep(env, node(env, REX_DOT, 1, 1, 0), 0, 0);
			break;
		case T_SLASHPLUS:
			eat(env);
			env->token.lex = T_PLUS;
			env->token.push = 1;
			if (e = node(env, REX_ONECHAR, 1, 1, 0))
			{
				e->re.onechar = '/';
				e = rep(env, e, 0, 0);
			}
			break;
		case T_WORD:
			eat(env);
			e = rep(env, node(env, REX_WORD, 0, 0, 0), 0, 0);
			break;
		case T_WORD_NOT:
			eat(env);
			e = rep(env, node(env, REX_WORD_NOT, 0, 0, 0), 0, 0);
			break;
		case T_BEG_STR:
			eat(env);
			e = rep(env, node(env, REX_BEG_STR, 0, 0, 0), 0, 0);
			break;
		case T_END_STR:
			eat(env);
			e = rep(env, node(env, REX_END_STR, 0, 0, 0), 0, 0);
			break;
		case T_FIN_STR:
			eat(env);
			e = rep(env, node(env, REX_FIN_STR, 0, 0, 0), 0, 0);
			break;
		default:
			env->error = REG_BADRPT;
			return 0;
		}
		if (e && *env->cursor != 0 && *env->cursor != env->delimiter && *env->cursor != env->terminator)
			e = cat(env, e, seq(env));
		return e;
	}
}

static Rex_t*
conj(Cenv_t* env)
{
	Rex_t*	e;
	Rex_t*	f;
	Rex_t*	g;

	if (!(e = seq(env)) || !(env->flags & REG_AUGMENTED) || token(env) != T_AND)
		return e;
	eat(env);
	if (!(f = conj(env)))
	{
		drop(env->disc, e);
		return 0;
	}
	if (!(g = node(env, REX_CONJ, 0, 0, 0)))
	{
		drop(env->disc, e);
		drop(env->disc, f);
		return 0;
	}
	g->re.group.expr.binary.left = e;
	g->re.group.expr.binary.right = f;
	return g;
}

static Rex_t*
alt(Cenv_t* env, int number, int cond)
{
	Rex_t*	e;
	Rex_t*	f;
	Rex_t*	g;

	if (!(e = conj(env)))
		return 0;
	else if (token(env) != T_BAR)
	{
		if (!cond)
			return e;
		f = 0;
		if (e->type == REX_NULL)
			goto bad;
	}
	else
	{
		eat(env);
		if (!(f = alt(env, number, 0)))
		{
			drop(env->disc, e);
			return 0;
		}
		if ((e->type == REX_NULL || f->type == REX_NULL) && !(env->flags & REG_NULL))
			goto bad;
		if (!cond && (g = trie(env, e, f)))
			return g;
	}
	if (!(g = node(env, REX_ALT, 0, 0, 0)))
	{
		env->error = REG_ESPACE;
		goto bad;
	}
	g->re.group.number = number;
	g->re.group.last = env->parno;
	g->re.group.expr.binary.left = e;
	g->re.group.expr.binary.right = f;
	return g;
 bad:
	drop(env->disc, e);
	drop(env->disc, f);
	if (!env->error)
		env->error = REG_ENULL;
	return 0;
}

/*
 * add v to REX_BM tables
 */

static void
bmstr(Cenv_t* env, register Rex_t* a, unsigned char* v, int n, Bm_mask_t b)
{
	int	c;
	int	m;
	size_t	z;

	for (m = 0; m < n; m++)
	{
		if (!(z = n - m - 1))
			z = HIT;
		c = v[m];
		a->re.bm.mask[m][c] |= b;
		if (z == HIT || !a->re.bm.skip[c] || a->re.bm.skip[c] > z && a->re.bm.skip[c] < HIT)
			a->re.bm.skip[c] = z;
		if (a->flags & REG_ICASE)
		{
			if (isupper(c))
				c = tolower(c);
			else if (islower(c))
				c = toupper(c);
			else
				continue;
			a->re.bm.mask[m][c] |= b;
			if (z == HIT || !a->re.bm.skip[c] || a->re.bm.skip[c] > z && a->re.bm.skip[c] < HIT)
				a->re.bm.skip[c] = z;
		}
	}
}

/*
 * set up BM table from trie
 */

static int
bmtrie(Cenv_t* env, Rex_t* a, unsigned char* v, Trie_node_t* x, int n, int m, Bm_mask_t b)
{
	do
	{
		v[m] = x->c;
		if (m >= (n - 1))
		{
			bmstr(env, a, v, n, b);
			if (!(b <<= 1))
			{
				b = 1;
				a->re.bm.complete = 0;
			}
			else if (x->son)
				a->re.bm.complete = 0;
		}
		else if (x->son)
			b = bmtrie(env, a, v, x->son, n, m + 1, b);
	} while (x = x->sib);
	return b;
}

/*
 * rewrite the expression tree for some special cases
 * 1. it is a null expression - illegal
 * 2. max length fixed string found -- use BM algorithm
 * 3. it begins with an unanchored string - use KMP algorithm
 * 0 returned on success
 */		

static int
special(Cenv_t* env, regex_t* p)
{
	Rex_t*		a;
	Rex_t*		e;
	Rex_t*		t;
	Rex_t*		x;
	Rex_t*		y;
	unsigned char*	s;
	int*		f;
	int		n;
	int		m;
	int		k;

#define _AST_DEBUG_regex	0

#if _AST_DEBUG_regex
	static int	debug_regex = -1;

	if (debug_regex < 0)
		debug_regex = (s = (unsigned char*)getenv("_AST_DEBUG_regex")) ? strtol((char*)s, NiL, 0) : 0;
#endif
	if (e = p->env->rex)
	{
		if ((x = env->stats.x) && x->re.string.size < 3)
			x = 0;
		if ((t = env->stats.y) && t->re.trie.min < 3)
			t = 0;
		if (x && t)
		{
			if (x->re.string.size >= t->re.trie.min)
				t = 0;
			else
				x = 0;
		}
		if (x || t)
		{
			Bm_mask_t**	mask;
			Bm_mask_t*	h;
			unsigned char*	v;
			size_t*		q;
			unsigned long	l;
			int		i;
			int		j;

			if (x)
			{
				y = x;
				n = x->re.string.size;
				l = env->stats.l;
			}
			else
			{
				y = t;
				n = t->re.trie.min;
				l = env->stats.k;
			}
			if (!(q = (size_t*)alloc(env->disc, 0, (n + 1) * sizeof(size_t))))
				return 1;
			if (!(a = node(env, REX_BM, 0, 0, n * (sizeof(Bm_mask_t*) + (UCHAR_MAX + 1) * sizeof(Bm_mask_t)) + (UCHAR_MAX + n + 2) * sizeof(size_t))))
			{
				alloc(env->disc, q, 0);
				return 1;
			}
			a->flags = y->flags;
			a->re.bm.size = n;
			a->re.bm.left = l - 1;
			a->re.bm.right = env->stats.m - l - n;
			a->re.bm.complete = (y != e && (e->type != REX_GROUP || y != e->re.group.expr.rex) || e->next || ((a->re.bm.left + a->re.bm.right) >= 0)) ? 0 : n;
			h = (Bm_mask_t*)&a->re.bm.mask[n];
			a->re.bm.skip = (size_t*)(h + n * (UCHAR_MAX + 1));
			a->re.bm.fail = &a->re.bm.skip[UCHAR_MAX + 1];
			for (m = 0; m <= UCHAR_MAX; m++)
				a->re.bm.skip[m] = n;
			a->re.bm.skip[0] = a->re.bm.skip['\n'] = (y->next && y->next->type == REX_END) ? HIT : (n + a->re.bm.left);
			for (i = 1; i <= n; i++)
				a->re.bm.fail[i] = 2 * n - i;
			mask = a->re.bm.mask;
			for (m = 0; m < n; m++)
			{
				mask[m] = h;
				h += UCHAR_MAX + 1;
			}
			if (x)
				bmstr(env, a, x->re.string.base, n, 1);
			else
			{
				v = (unsigned char*)q;
				memset(v, 0, n);
				m = 1;
				for (i = 0; i <= UCHAR_MAX; i++)
					if (t->re.trie.root[i])
						m = bmtrie(env, a, v, t->re.trie.root[i], n, 0, m);
			}
			mask--;
			memset(q, 0, n * sizeof(*q));
			for (k = (j = n) + 1; j > 0; j--, k--)
			{
#if _AST_DEBUG_regex
				if (debug_regex)
					sfprintf(sfstderr, "BM#0: k=%d j=%d\n", k, j);
#endif
				for (q[j] = k; k <= n; k = q[k])
				{
					for (m = 0; m <= UCHAR_MAX; m++)
						if (mask[k][m] == mask[j][m])
						{
#if _AST_DEBUG_regex
							if (debug_regex)
								sfprintf(sfstderr, "CUT1: mask[%d][%c]=mask[%d][%c]\n", k, m, j, m);
#endif
							goto cut;
						}
#if _AST_DEBUG_regex
					if (debug_regex)
						sfprintf(sfstderr, "BM#2: fail[%d]=%d => %d\n", k, a->re.bm.fail[k], (a->re.bm.fail[k] > n - j) ? (n - j) : a->re.bm.fail[k]);
#endif
					if (a->re.bm.fail[k] > n - j)
						a->re.bm.fail[k] = n - j;
				}
			cut:	;
			}
#if _AST_DEBUG_regex
			if (debug_regex < 0)
				k = -debug_regex;
#endif
			for (i = 1; i <= n; i++)
				if (a->re.bm.fail[i] > n + k - i)
				{
#if _AST_DEBUG_regex
					if (debug_regex)
						sfprintf(sfstderr, "BM#4: fail[%d]=%d => %d\n", i, a->re.bm.fail[i], n + k - i);
#endif
					a->re.bm.fail[i] = n + k - i;
				}
#if _AST_DEBUG_regex
			if (debug_regex)
			{
				sfprintf(sfstderr, "STAT: complete=%d n=%d k=%d l=%d r=%d y=%d:%d e=%d:%d\n", a->re.bm.complete, n, k, a->re.bm.left, a->re.bm.right, y->type, y->next ? y->next->type : 0, e->type, e->next ? e->next->type : 0);
				for (m = 0; m < n; m++)
					for (i = 1; i <= UCHAR_MAX; i++)
						if (a->re.bm.mask[m][i])
							sfprintf(sfstderr, "MASK: [%d]['%c'] = %032..2u\n", m, i, a->re.bm.mask[m][i]);
				for (i = ' '; i <= UCHAR_MAX; i++)
					if (a->re.bm.skip[i] >= HIT)
						sfprintf(sfstderr, "SKIP: ['%c'] =   *\n", i);
					else if (a->re.bm.skip[i] > 0 && a->re.bm.skip[i] < n)
						sfprintf(sfstderr, "SKIP: ['%c'] = %3d\n", i, a->re.bm.skip[i]);
				for (j = 31; j >= 0; j--)
				{
					sfprintf(sfstderr, "      ");
				next:
					for (m = 0; m < n; m++)
					{
						for (i = 0040; i < 0177; i++)
							if (a->re.bm.mask[m][i] & (1 << j))
							{
								sfprintf(sfstderr, "  %c", i);
								break;
							}
						if (i >= 0177)
						{
							if (j > 0)
							{
								j--;
								goto next;
							}
							sfprintf(sfstderr, "  ?");
						}
					}
					sfprintf(sfstderr, "\n");
				}
				sfprintf(sfstderr, "FAIL: ");
				for (m = 1; m <= n; m++)
					sfprintf(sfstderr, "%3d", a->re.bm.fail[m]);
				sfprintf(sfstderr, "\n");
			}
#endif
			alloc(env->disc, q, 0);
			a->next = e;
			p->env->rex = a;
			return 0;
		}
		switch (e->type)
		{
		case REX_BEG:
			if (env->flags & REG_NEWLINE)
				return 0;
			break;
		case REX_GROUP:
			e = e->re.group.expr.rex;
			if (e->type != REX_DOT)
				return 0;
			/*FALLTHROUGH*/
		case REX_DOT:
			if (e->lo == 0 && e->hi == RE_DUP_INF)
				break;
			return 0;
		case REX_NULL:
			if (env->flags & REG_NULL)
				break;
			env->error = REG_ENULL;
			return 1;
		case REX_STRING:
			if (env->flags & (REG_LEFT|REG_LITERAL|REG_RIGHT))
				return 0;
			s = e->re.string.base;
			n = e->re.string.size;
			if (!(a = node(env, REX_KMP, 0, 0, n * (sizeof(int*) + 1))))
				return 1;
			a->flags = e->flags;
			f = a->re.string.fail;
			memcpy((char*)(a->re.string.base = (unsigned char*)&f[n]), (char*)s, n);
			s = a->re.string.base;
			a->re.string.size = n;
			f[0] = m = -1;
			for (k = 1; k < n; k++)
			{
				while (m >= 0 && s[m+1] != s[k])
					m = f[m];
				if (s[m+1] == s[k])
					m++;
				f[k] = m;
			}
			a->next = e->next;
			p->env->rex = a;
			e->next = 0;
			drop(env->disc, e);
			break;
		default:
			return 0;
		}
	}
	p->env->once = 1;
	return 0;
}		

int
regcomp(regex_t* p, const char* pattern, regflags_t flags)
{
	Rex_t*			e;
	Rex_t*			f;
	regdisc_t*		disc;
	int			i;
	Cenv_t			env;

	if (!p)
		return REG_BADPAT;
	if (flags & REG_DISCIPLINE)
	{
		flags &= ~REG_DISCIPLINE;
		disc = p->re_disc;
	}
	else
		disc = &state.disc;
	if (!disc->re_errorlevel)
		disc->re_errorlevel = 2;
	p->env = 0;
	if (!pattern)
		return fatal(disc, REG_BADPAT, pattern);
 again:
	if (!(p->env = (Env_t*)alloc(disc, 0, sizeof(Env_t))))
		return fatal(disc, REG_ESPACE, pattern);
	memset(p->env, 0, sizeof(*p->env));
	memset(&env, 0, sizeof(env));
	env.regex = p;
	env.flags = flags;
	env.disc = p->env->disc = disc;
	env.explicit = -1;
	if (env.flags & REG_AUGMENTED)
		env.flags |= REG_EXTENDED;
	if (!state.fold[UCHAR_MAX])
	{
		for (i = 0; i <= UCHAR_MAX; i++)
			state.fold[i] = toupper(i);
		for (i = 0; i < elementsof(state.escape); i++)
			state.magic[state.escape[i].key] = state.escape[i].val;
	}
	env.type = (env.flags & REG_AUGMENTED) ? ARE : (env.flags & REG_EXTENDED) ? ERE : BRE;
	if (env.flags & REG_SHELL)
	{
		if (env.flags & REG_SHELL_PATH)
			env.explicit = '/';
		env.flags &= ~REG_NEWLINE;
		env.flags |= REG_LENIENT|REG_NULL;
		env.type = env.type == BRE ? SRE : KRE;
	}
	else if ((env.flags & (REG_NEWLINE|REG_SPAN)) == REG_NEWLINE)
		env.explicit = '\n';
	env.posixkludge = !(env.flags & (REG_EXTENDED|REG_SHELL));
	env.token.lex = 0;
	env.token.push = 0;
	if (env.flags & (REG_DELIMITED|REG_DELIMITED_OLD))
	{
		switch (env.delimiter = *pattern++)
		{
		case 0:
		case '\\':
		case '\n':
		case '\r':
			env.error = REG_EDELIM;
			goto bad;
		}
		env.terminator = '\n';
	}
	env.pattern = env.cursor = (unsigned char*)pattern;
	if (!(p->env->rex = alt(&env, 1, 0)))
		goto bad;
	if (env.parnest)
	{
		env.error = REG_EPAREN;
		goto bad;
	}
	if ((env.flags & REG_LEFT) && p->env->rex->type != REX_BEG)
	{
		if (!(e = node(&env, REX_BEG, 0, 0, 0)))
		{
			regfree(p);
			return fatal(disc, REG_ESPACE, pattern);
		}
		e->next = p->env->rex;
		p->env->rex = e;
		p->env->once = 1;
	}
	for (e = p->env->rex; e->next; e = e->next);
	p->env->done.type = REX_DONE;
	p->env->done.flags = e->flags;
	if (env.flags & REG_RIGHT)
	{
		if (e->type != REX_END)
		{
			if (!(f = node(&env, REX_END, 0, 0, 0)))
			{
				regfree(p);
				return fatal(disc, REG_ESPACE, pattern);
			}
			f->flags = e->flags;
			e->next = f;
		}
	}
	if (stats(&env, p->env->rex))
	{
		if (!env.error)
			env.error = REG_ECOUNT;
		goto bad;
	}
	if (env.stats.a || env.stats.b || env.stats.c > 1 && env.stats.c != env.stats.s || env.stats.t && (env.stats.t > 1 || env.stats.a || env.stats.c))
		p->env->hard = 1;
	if (env.stats.b)
		p->env->separate = 1;
	if (special(&env, p))
		goto bad;
	serialize(&env, p->env->rex, 1);
	p->re_nsub = env.stats.p;
	if (env.type == KRE)
		p->re_nsub /= 2;
	if (env.flags & (REG_DELIMITED|REG_DELIMITED_OLD))
	{
		p->re_npat = env.cursor - env.pattern + 1;
		if (!(env.flags & REG_DELIMITED_OLD) && *env.cursor == env.delimiter)
			p->re_npat++;
		else if (env.flags & REG_MUSTDELIM)
		{
			env.error = REG_EDELIM;
			goto bad;
		}
		else
			env.flags &= ~(REG_DELIMITED|REG_DELIMITED_OLD);
	}
	p->env->explicit = env.explicit;
	p->env->flags = env.flags & REG_COMP;
	p->env->leading = !!(env.flags & REG_SHELL_DOT);
	p->env->min = env.stats.m;
	p->env->nsub = env.stats.p + env.stats.u;
	p->env->refs = 1;
	return 0;
 bad:
	regfree(p);
	if (!env.error)
		env.error = REG_ESPACE;
	if (env.type >= SRE && env.error != REG_ESPACE && !(flags & REG_LITERAL))
	{
		flags |= REG_LITERAL;
		goto again;
	}
	return fatal(disc, env.error, pattern);
}

/*
 * combine two compiled regular expressions if possible,
 * replacing first with the combination and freeing second.
 * return 0 on success.
 * the only combinations handled are building a Trie
 * from String|Kmp|Trie and String|Kmp
 */

int
regcomb(regex_t* p, regex_t* q)
{
	Rex_t*	e = p->env->rex;
	Rex_t*	f = q->env->rex;
	Rex_t*	g;
	Cenv_t	env;

	if (!e || !f)
		return fatal(p->env->disc, REG_BADPAT, NiL);
	if (p->env->separate || q->env->separate)
		return REG_ESUBREG;
	memset(&env, 0, sizeof(env));
	env.disc = p->env->disc;
	if (e->type == REX_BM)
	{
		p->env->rex = e->next;
		e->next = 0;
		drop(env.disc, e);
		e = p->env->rex;
	}
	if (f->type == REX_BM)
	{
		q->env->rex = f->next;
		f->next = 0;
		drop(env.disc, f);
		f = q->env->rex;
	}
	if (e->type == REX_BEG && f->type == REX_BEG)
	{
		p->env->flags |= REG_LEFT;
		p->env->rex = e->next;
		e->next = 0;
		drop(env.disc, e);
		e = p->env->rex;
		q->env->rex = f->next;
		f->next = 0;
		drop(env.disc, f);
		f = q->env->rex;
	}
	if (e->next && e->next->type == REX_END && f->next && f->next->type == REX_END)
	{
		p->env->flags |= REG_RIGHT;
		drop(env.disc, e->next);
		e->next = 0;
		drop(env.disc, f->next);
		f->next = 0;
	}
	if (!(g = trie(&env, f, e)))
		return fatal(p->env->disc, REG_BADPAT, NiL);
	p->env->rex = g;
	if (!q->env->once)
		p->env->once = 0;
	q->env->rex = 0;
	if (p->env->flags & REG_LEFT)
	{
		if (!(e = node(&env, REX_BEG, 0, 0, 0)))
		{
			regfree(p);
			return fatal(p->env->disc, REG_ESPACE, NiL);
		}
		e->next = p->env->rex;
		p->env->rex = e;
		p->env->once = 1;
	}
	if (p->env->flags & REG_RIGHT)
	{
		for (f = p->env->rex; f->next; f = f->next);
		if (f->type != REX_END)
		{
			if (!(e = node(&env, REX_END, 0, 0, 0)))
			{
				regfree(p);
				return fatal(p->env->disc, REG_ESPACE, NiL);
			}
			f->next = e;
		}
	}
	env.explicit = p->env->explicit;
	env.flags = p->env->flags;
	env.disc = p->env->disc;
	if (stats(&env, p->env->rex))
	{
		regfree(p);
		return fatal(p->env->disc, env.error ? env.error : REG_ECOUNT, NiL);
	}
	if (special(&env, p))
	{
		regfree(p);
		return fatal(p->env->disc, env.error ? env.error : REG_ESPACE, NiL);
	}
	p->env->min = g->re.trie.min;
	return 0;
}

/*
 * copy a reference of p into q
 * p and q may then have separate regsubcomp() instantiations
 */

int
regdup(regex_t* p, regex_t* q)
{
	if (!p || !q)
		return REG_BADPAT;
	*q = *p;
	p->env->refs++;
	q->re_sub = 0;
	return 0;
}
