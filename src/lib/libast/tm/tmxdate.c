/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*                  Copyright (c) 1985-2005 AT&T Corp.                  *
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
 * Glenn Fowler
 * AT&T Research
 *
 * Time_t conversion support
 *
 * relative times inspired by Steve Bellovin's netnews getdate(3)
 */

#include <tmx.h>
#include <ctype.h>

#define dig1(s,n)	((n)=((*(s)++)-'0'))
#define dig2(s,n)	((n)=((*(s)++)-'0')*10,(n)+=(*(s)++)-'0')
#define dig3(s,n)	((n)=((*(s)++)-'0')*100,(n)+=((*(s)++)-'0')*10,(n)+=(*(s)++)-'0')
#define dig4(s,n)	((n)=((*(s)++)-'0')*1000,(n)+=((*(s)++)-'0')*100,(n)+=((*(s)++)-'0')*10,(n)+=(*(s)++)-'0')

#define BREAK		(1<<0)
#define CCYYMMDDHHMMSS	(1<<1)
#define CRON		(1<<2)
#define DAY		(1<<3)
#define EXACT		(1<<4)
#define FINAL		(1<<5)
#define HOLD		(1<<6)
#define HOUR		(1<<7)
#define LAST		(1<<8)
#define MDAY		(1<<9)
#define MINUTE		(1<<10)
#define MONTH		(1<<11)
#define NEXT		(1<<12)
#define NSEC		(1<<13)
#define SECOND		(1<<14)
#define THIS		(1L<<15)
#define WDAY		(1L<<16)
#define YEAR		(1L<<17)
#define ZONE		(1L<<18)

/*
 * parse cron range into set
 * return: -1:error 0:* 1:some
 */

static int
range(register char* s, char** e, char* set, int lo, int hi)
{
	int	n;
	int	m;
	char*	t;

	while (isspace(*s) || *s == '_')
		s++;
	if (*s == '*')
	{
		*e = s + 1;
		return 0;
	}
	memset(set, 0, hi + 1);
	for (;;)
	{
		n = strtol(s, &t, 10);
		if (s == t || n < lo || n > hi)
			return -1;
		if (*(s = t) == '-')
		{
			m = strtol(++s, &t, 10);
			if (s == t || m < n || m > hi)
				return -1;
			s = t;
		}
		else
			m = n;
		while (n <= m)
			set[n++] = 1;
		if (*s != ',')
			break;
		s++;
	}
	*e = s;
	return 1;
}

/*
 * parse date expression in s and return Time_t value
 *
 * if non-null, e points to the first invalid sequence in s
 * now provides default values
 */

Time_t
tmxdate(register const char* s, char** e, Time_t now)
{
	register Tm_t*	tm;
	register long	n;
	register int	w;
	unsigned long	set;
	unsigned long	state;
	unsigned long	flags;
	Time_t		fix;
	char*		t;
	char*		u;
	const char*	x;
	char*		last;
	char*		type;
	int		day;
	int		dir;
	int		dst;
	int		zone;
	int		f;
	int		i;
	int		j;
	int		k;
	int		l;
	long		m;
	long		p;
	long		q;
	Tm_zone_t*	zp;
	char		skip[UCHAR_MAX + 1];

	/*
	 * check DATEMSK first
	 */

	fix = tmxscan(s, &last, NiL, &t, now, 0);
	if (t && !*last)
	{
		if (e)
			*e = last;
		return fix;
	}

 reset:

	/*
	 * use now for defaults
	 */

	tm = tmxmake(now);
	tm_info.date = tm_info.zone;
	day = -1;
	dst = TM_DST;
	set = state = 0;
	type = 0;
	zone = TM_LOCALZONE;
	skip[0] = 0;
	for (n = 1; n <= UCHAR_MAX; n++)
		skip[n] = isspace(n) || strchr("_,;@=|!^()[]{}", n);

	/*
	 * get <weekday year month day hour minutes seconds ?[ds]t [ap]m>
	 */

	for (;;)
	{
		state &= (state & HOLD) ? ~(HOLD) : ~(EXACT|LAST|NEXT|THIS);
		if ((set|state) & (YEAR|MONTH|DAY))
			skip['/'] = 1;
		for (;;)
		{
			if (*s == '.' || *s == '-' || *s == '+')
			{
				if (((set|state) & (YEAR|MONTH|HOUR|MINUTE|ZONE)) == (YEAR|MONTH|HOUR|MINUTE) && (zone = tmgoff(s, &t, 0)))
				{
					state |= ZONE;
					if (!*(s = t))
						break;
				}
				else if (*s == '+')
					break;
			}
			else if (!skip[*s])
				break;
			s++;
		}
		if (!*(last = (char*)s))
			break;
		if (*s == '#')
		{
			if (isdigit(*++s))
			{
				now = strtoull(s, &t, 0);
				if (*(s = t) == '.')
				{
					fix = 0;
					m = 1000000000;
					while (isdigit(*++s))
						fix += (*s - '0') * (m /= 10);
					now = tmxsns(now, fix);
				}
				else if (now <= 0x7fffffff)
					now = tmxsns(now, 0);
				goto reset;
			}
			else if (*s++ == '#')
			{
				now = tmxtime(tm, zone);
				goto reset;
			}
			break;
		}
		f = -1;
		if (*s == '+')
		{
			while (isspace(*++s) || *s == '_');
			n = strtol(s, &t, 0);
			if (w = t - s)
			{
				for (s = t; skip[*s]; s++);
				state |= (f = n) ? NEXT : THIS;
				set &= ~(EXACT|LAST|NEXT|THIS);
				set |= state & (EXACT|LAST|NEXT|THIS);
			}
			else
				s = last;
		}
		if (!(state & CRON))
		{
			/*
			 * check for cron date
			 *
			 *	min hour day-of-month month day-of-week
			 *
			 * if it's cron then determine the next time
			 * that satisfies the specification
			 *
			 * NOTE: the only spacing is ' '||'_'||';'
			 */

			i = 0;
			n = *(t = (char*)s);
			for (;;)
			{
				if (n == '*')
					n = *++s;
				else if (!isdigit(n))
					break;
				else
					while ((n = *++s) == ',' || n == '-' || isdigit(n));
				if (n != ' ' && n != '_' && n != ';')
				{
					if (!n)
						i++;
					break;
				}
				i++;
				while ((n = *++s) == ' ' || n == '_');
			}
			if (i == 5)
			{
				Time_t	tt;
				char	hit[60];
				char	mon[12];
				char	day[7];

				state |= CRON;
				flags = 0;
				tm->tm_sec = 0;
				tm->tm_min++;
				tmfix(tm);

				/*
				 * minute
				 */

				if ((k = range(t, &t, hit, 0, 59)) < 0)
					break;
				if (k && !hit[i = tm->tm_min])
				{
					hit[i] = 1;
					do if (++i > 59)
					{
						i = 0;
						if (++tm->tm_hour > 59)
						{
							tm->tm_min = i;
							tmfix(tm);
						}
					} while (!hit[i]);
					tm->tm_min = i;
				}

				/*
				 * hour
				 */

				if ((k = range(t, &t, hit, 0, 23)) < 0)
					break;
				if (k && !hit[i = tm->tm_hour])
				{
					hit[i] = 1;
					do if (++i > 23)
					{
						i = 0;
						if (++tm->tm_mday > 28)
						{
							tm->tm_hour = i;
							tmfix(tm);
						}
					} while (!hit[i]);
					tm->tm_hour = i;
				}

				/*
				 * day of month
				 */

				if ((k = range(t, &t, hit, 1, 31)) < 0)
					break;
				if (k)
					flags |= MDAY;

				/*
				 * month
				 */

				if ((k = range(t, &t, mon, 1, 12)) < 0)
					break;
				if (k)
					flags |= MONTH;
				else
					for (i = 1; i <= 12; i++)
						mon[i] = 1;

				/*
				 * day of week
				 */

				if ((k = range(t, &t, day, 0, 6)) < 0)
					break;
				if (k)
					flags |= WDAY;
				s = t;
				if (flags & (MONTH|MDAY|WDAY))
				{
					fix = tmxtime(tm, zone);
					tm = tmxmake(fix);
					i = tm->tm_mon + 1;
					j = tm->tm_mday;
					k = tm->tm_wday;
					for (;;)
					{
						if (!mon[i])
						{
							if (++i > 12)
							{
								i = 1;
								tm->tm_year++;
							}
							tm->tm_mon = i - 1;
							tm->tm_mday = 1;
							tt = tmxtime(tm, zone);
							if (tt < fix)
								goto done;
							tm = tmxmake(tt);
							i = tm->tm_mon + 1;
							j = tm->tm_mday;
							k = tm->tm_wday;
							continue;
						}
						if (flags & (MDAY|WDAY))
						{
							if ((flags & (MDAY|WDAY)) == (MDAY|WDAY))
							{
								if (hit[j] && day[k])
									break;
							}
							else if ((flags & MDAY) && hit[j])
								break;
							else if ((flags & WDAY) && day[k])
								break;
							if (++j > 28)
							{
								tm->tm_mon = i - 1;
								tm->tm_mday = j;
								tm = tmxmake(tmxtime(tm, zone));
								i = tm->tm_mon + 1;
								j = tm->tm_mday;
								k = tm->tm_wday;
							}
							else if ((flags & WDAY) && ++k > 6)
								k = 0;
						}
						else if (flags & MONTH)
							break;
					}
					tm->tm_mon = i - 1;
					tm->tm_mday = j;
					tm->tm_wday = k;
				}
				continue;
			}
			s = t;
		}
		n = -1;
		if (isdigit(*s))
		{
			n = strtol(s, &t, 10);
			w = t - s;
			u = t + (*t == '-');
			if ((w == 2 || w == 4) && (*u == 'W' || *u == 'w') && isdigit(*(u + 1)))
			{
				t = u;
				if (w == 4)
				{
					if ((n -= 1900) < TM_WINDOW)
						break;
				}
				else if (n < TM_WINDOW)
					n += 100;
				m = n;
				n = strtol(s = t + 1, &t, 0);
				if ((i = (t - s)) < 2 || i > 3)
					break;
				if (dig2(s, j) < 0 || j > 53)
					break;
				if (!(t - s) && *t == '-')
					n = strtol(s = t + 1, &t, 0);
				if (!(i = (t - s)))
					k = 1;
				else if (i != 1 || dig1(s, k) < 1 || k > 7)
					break;
				else if (k == 7)
					k = 0;
				tm->tm_year = m;
				tmweek(tm, 2, j, k);
				set |= YEAR|MONTH|DAY;
				continue;
			}
			else if ((w == 6 || w == 8) && (*u == 'T' || *u == 't') && isdigit(*(u + 1)))
			{
				t = u;
				flags = 0;
				if (w == 8)
				{
					dig4(s, m);
					if ((m -= 1900) < TM_WINDOW)
						break;
				}
				else
				{
					dig2(s, m);
					if (m < TM_WINDOW)
						m += 100;
				}
				flags |= YEAR;
				if (dig2(s, l) <= 0 || l > 12)
					break;
				flags |= MONTH;
				if (dig2(s, k) < 1 || k > 31)
					break;
				n = strtol(s = t + 1, &t, 0);
				if ((t - s) < 2)
					break;
				if (dig2(s, j) > 24)
					break;
				if ((t - s) < 2)
				{
					if ((t - s) == 1 || *t++ != '-')
						break;
					n = strtol(s = t, &t, 0);
					if ((t - s) < 2)
						break;
				}
				if (dig2(s, i) > 59)
					break;
				flags |= HOUR|MINUTE;
				if ((t - s) == 2)
				{
					if (dig2(s, n) > (59 + TM_MAXLEAP))
						break;
					flags |= SECOND;
				}
				else if (t - s)
					break;
				else
					n = 0;
				p = 0;
				if (*t == '.')
				{
					q = 1000000000;
					while (isdigit(*++t))
						p += (*t - '0') * (q /= 10);
					set |= NSEC;
				}
				if (n > (59 + TM_MAXLEAP))
					break;
				goto save;
			}
			else if (f == -1 && isalpha(*t) && tmlex(t, &t, tm_info.format + TM_ORDINAL, TM_ORDINALS - TM_ORDINAL, NiL, 0) >= 0)
			{
 ordinal:
				state |= (f = n) ? NEXT : THIS;
				set &= ~(EXACT|LAST|NEXT|THIS);
				set |= state & (EXACT|LAST|NEXT|THIS);
				for (s = t; skip[*s]; s++);
				if (isdigit(*s))
				{
					n = strtol(s, &t, 10);
					s = t;
					if (*s == '_')
						s++;
				}
				else
					n = -1;
			}
			else
			{
				if (!(state & (LAST|NEXT|THIS)) && ((i = t - s) == 4 && (*t == '.' && isdigit(*(t + 1)) && isdigit(*(t + 2)) && *(t + 3) != '.' || (!*t || isspace(*t) || *t == '_' || isalnum(*t)) && n >= 0 && (n % 100) < 60 && ((m = (n / 100)) < 20 || m < 24 && !((set|state) & (YEAR|MONTH|HOUR|MINUTE)))) || i > 4 && i <= 12))
				{
					/*
					 * various { date(1) touch(1) } formats
					 *
					 *	[[cc]yy[mm]]ddhhmm[.ss[.nn...]]
					 *	[cc]yyjjj
					 *	hhmm[.ss[.nn...]]
					 */

					flags = 0;
					if (state & CCYYMMDDHHMMSS)
						break;
					state |= CCYYMMDDHHMMSS;
					p = 0;
					if ((i == 7 || i == 5) && !*t)
					{
						if (i == 7)
						{
							dig4(s, m);
							if ((m -= 1900) < TM_WINDOW)
								break;
						}
						else if (dig2(s, m) < TM_WINDOW)
							m += 100;
						dig3(s, k);
						l = 1;
						j = 0;
						i = 0;
						n = 0;
						flags |= MONTH;
					}
					else if (i & 1)
						break;
					else
					{
						u = t;
						if (i == 12)
						{
							x = s;
							dig2(x, m);
							if (m <= 12)
							{
								u -= 4;
								i -= 4;
								x = s + 8;
								dig4(x, m);
							}
							else
								dig4(s, m);
							m -= 1900;
						}
						else if (i == 10)
						{
							x = s;
							if (!dig2(x, m) || m > 12 || !dig2(x, m) || m > 31 || dig2(x, m) > 24 || dig2(x, m) > 60 || dig2(x, m) <= 60 && !(tm_info.flags & TM_DATESTYLE))
								dig2(s, m);
							else
							{
								u -= 2;
								i -= 2;
								x = s + 8;
								dig2(x, m);
							}
							if (m < TM_WINDOW)
								m += 100;
						}
						else
							m = tm->tm_year;
						if ((u - s) < 8)
							l = tm->tm_mon + 1;
						else if (dig2(s, l) <= 0 || l > 12)
							break;
						else
							flags |= MONTH;
						if ((u - s) < 6)
							k = tm->tm_mday;
						else if (dig2(s, k) < 1 || k > 31)
							break;
						else
							flags |= DAY;
						if ((u - s) < 4)
							break;
						if (dig2(s, j) > 24)
							break;
						if (dig2(s, i) > 59)
							break;
						flags |= HOUR|MINUTE;
						if ((u - s) == 2)
						{
							dig2(s, n);
							flags |= SECOND;
						}
						else if (u - s)
							break;
						else if (*t != '.')
							n = 0;
						else
						{
							n = strtol(t + 1, &t, 10);
							flags |= SECOND;
							if (*t == '.')
							{
								q = 1000000000;
								while (isdigit(*++t))
									p += (*t - '0') * (q /= 10);
								set |= NSEC;
							}
						}
						if (n > (59 + TM_MAXLEAP))
							break;
					}
				save:
					tm->tm_year = m;
					tm->tm_mon = l - 1;
					tm->tm_mday = k;
					tm->tm_hour = j;
					tm->tm_min = i;
					tm->tm_sec = n;
					tm->tm_nsec = p;
					s = t;
					set |= flags;
					continue;
				}
				for (s = t; skip[*s]; s++);
				if (*s == ':')
				{
					if ((state & HOUR) || n > 24)
						break;
					while (isspace(*++s) || *s == '_');
					if (!isdigit(*s))
						break;
					i = n;
					n = strtol(s, &t, 10);
					for (s = t; isspace(*s) || *s == '_'; s++);
					if (n > 59)
						break;
					j = n;
					m = 0;
					if (*s == ':')
					{
						while (isspace(*++s) || *s == '_');
						if (!isdigit(*s))
							break;
						n = strtol(s, &t, 10);
						s = t;
						if (n > (59 + TM_MAXLEAP))
							break;
						set |= SECOND;
						while (isspace(*s))
							s++;
						if (*s == '.')
						{
							q = 1000000000;
							while (isdigit(*++s))
								m += (*s - '0') * (q /= 10);
							set |= NSEC;
						}
					}
					else
						n = 0;
					set |= HOUR|MINUTE;
					skip[':'] = 1;
					k = tm->tm_hour;
					tm->tm_hour = i;
					l = tm->tm_min;
					tm->tm_min = j;
					tm->tm_sec = n;
					tm->tm_nsec = m;
					while (isspace(*s))
						s++;
					switch (tmlex(s, &t, tm_info.format, TM_NFORM, tm_info.format + TM_MERIDIAN, 2))
					{
					case TM_MERIDIAN:
						s = t;
						if (i == 12)
							tm->tm_hour = i = 0;
						break;
					case TM_MERIDIAN+1:
						if (i < 12)
							tm->tm_hour = i += 12;
						break;
					}
					if (f >= 0 || (state & (LAST|NEXT)))
					{
						state &= ~HOLD;
						if (f < 0)
						{
							if (state & LAST)
								f = -1;
							else if (state & NEXT)
								f = 1;
							else
								f = 0;
						}
						if (f > 0)
						{
							if (i > k || i == k && j > l)
								f--;
						}
						else if (i < k || i == k && j < l)
							f++;
						if (f > 0)
						{
							tm->tm_hour += f * 24;
							while (tm->tm_hour >= 24)
							{
								tm->tm_hour -= 24;
								tm->tm_mday++;
							}
						}
						else if (f < 0)
						{
							tm->tm_hour += f * 24;
							while (tm->tm_hour < 24)
							{
								tm->tm_hour += 24;
								tm->tm_mday--;
							}
						}
					}
					continue;
				}
			}
		}
		for (;;)
		{
			if (*s == '-' || *s == '+')
			{
				if (((set|state) & (MONTH|DAY|HOUR|MINUTE)) == (MONTH|DAY|HOUR|MINUTE) || *s == '+' && (!isdigit(s[1]) || !isdigit(s[2]) || s[3] != ':'))
					break;
				s++;
			}
			else if (skip[*s])
				s++;
			else
				break;
		}
		if (isalpha(*s) && n < 1000)
		{
			if ((j = tmlex(s, &t, tm_info.format, TM_NFORM, tm_info.format + TM_SUFFIXES, TM_PARTS - TM_SUFFIXES)) >= 0)
			{
				s = t;
				switch (tm_data.lex[j])
				{
				case TM_EXACT:
					state |= HOLD|EXACT;
					set &= ~(EXACT|LAST|NEXT|THIS);
					set |= state & (EXACT|LAST|NEXT|THIS);
					continue;
				case TM_LAST:
					state |= HOLD|LAST;
					set &= ~(EXACT|LAST|NEXT|THIS);
					set |= state & (EXACT|LAST|NEXT|THIS);
					continue;
				case TM_THIS:
					state |= HOLD|THIS;
					set &= ~(EXACT|LAST|NEXT|THIS);
					set |= state & (EXACT|LAST|NEXT|THIS);
					n = 0;
					continue;
				case TM_NEXT:
					/*
					 * disambiguate english "last ... in" 
					 */

					if (!((state|set) & LAST))
					{
						state |= HOLD|NEXT;
						set &= ~(EXACT|LAST|NEXT|THIS);
						set |= state & (EXACT|LAST|NEXT|THIS);
						continue;
					}
					/*FALLTHROUGH*/
				case TM_FINAL:
					state |= HOLD|THIS|FINAL;
					set &= ~(EXACT|LAST|NEXT|THIS);
					set |= state & (EXACT|LAST|NEXT|THIS|FINAL);
					continue;
				case TM_ORDINAL:
					j += TM_ORDINALS - TM_ORDINAL;
					/*FALLTHROUGH*/
				case TM_ORDINALS:
					n = j - TM_ORDINALS + 1;
					goto ordinal;
				case TM_MERIDIAN:
					if (f >= 0)
						f++;
					else if (state & LAST)
						f = -1;
					else if (state & THIS)
						f = 1;
					else if (state & NEXT)
						f = 2;
					else
						f = 0;
					if (n > 0)
					{
						if (n > 24)
							goto done;
						tm->tm_hour = n;
					}
					for (k = tm->tm_hour; k < 0; k += 24);
					k %= 24;
					if (j == TM_MERIDIAN)
					{
						if (k == 12)
							tm->tm_hour -= 12;
					}
					else if (k < 12)
						tm->tm_hour += 12;
					if (n > 0)
						goto clear_min;
					continue;
				case TM_DAY_ABBREV:
					j += TM_DAY - TM_DAY_ABBREV;
					/*FALLTHROUGH*/
				case TM_DAY:
				case TM_PARTS:
				case TM_HOURS:
					state |= set & (EXACT|LAST|NEXT|THIS);
					if (!(state & (LAST|NEXT|THIS)))
						for (;;)
						{
							while (skip[*s])
								s++;
							if ((k = tmlex(s, &t, tm_info.format + TM_LAST, TM_NOISE - TM_LAST, NiL, 0)) >= 0)
							{
								s = t;
								if (k <= 2)
									state |= LAST;
								else if (k <= 5)
									state |= THIS;
								else if (k <= 8)
									state |= NEXT;
								else
									state |= EXACT;
							}
							else
							{
								state |= (n > 0) ? NEXT : THIS;
								break;
							}
							set &= ~(EXACT|LAST|NEXT|THIS);
							set |= state & (EXACT|LAST|NEXT|THIS);
						}
					/*FALLTHROUGH*/
				case TM_DAYS:
					if (n == -1)
					{
						/*
						 * disambiguate english "second"
						 */

						if (j == TM_PARTS && f == -1)
						{
							n = 2;
							goto ordinal;
						}
						n = 1;
					}
					if (state & LAST)
						n = -n;
					else if (!(state & NEXT))
						n--;
					m = (f > 0) ? f * n : n;
					switch (j)
					{
					case TM_DAYS+0:
						tm->tm_mday--;
						set |= DAY;
						goto clear_hour;
					case TM_DAYS+1:
						set |= DAY;
						goto clear_hour;
					case TM_DAYS+2:
						tm->tm_mday++;
						set |= DAY;
						goto clear_hour;
					case TM_PARTS+0:
						set |= SECOND;
						if ((m < 0 ? -m : m) > (365L*24L*60L*60L))
						{
							now = tmxtime(tm, zone) + tmxsns(m, 0);
							goto reset;
						}
						tm->tm_sec += m;
						goto clear_nsec;
					case TM_PARTS+1:
						tm->tm_min += m;
						set |= MINUTE;
						goto clear_sec;
					case TM_PARTS+2:
						tm->tm_hour += m;
						set |= MINUTE;
						goto clear_min;
					case TM_PARTS+3:
						tm->tm_mday += m;
						if (!(set & FINAL))
							set |= HOUR;
						goto clear_hour;
					case TM_PARTS+4:
						tm = tmxmake(tmxtime(tm, zone));
						tm->tm_mday += 7 * m - tm->tm_wday + 1;
						set |= DAY;
						goto clear_hour;
					case TM_PARTS+5:
						tm->tm_mon += m;
						set |= MONTH;
						goto clear_mday;
					case TM_PARTS+6:
						tm->tm_year += m;
						goto clear_mon;
					case TM_HOURS+0:
						tm->tm_mday += m;
						set |= DAY;
						goto clear_hour;
					case TM_HOURS+1:
						tm->tm_mday += m;
						tm->tm_hour = 6;
						set |= HOUR;
						goto clear_min;
					case TM_HOURS+2:
						tm->tm_mday += m;
						tm->tm_hour = 12;
						set |= HOUR;
						goto clear_min;
					case TM_HOURS+3:
						tm->tm_mday += m;
						tm->tm_hour = 18;
						set |= HOUR;
						goto clear_min;
					}
					tm = tmxmake(tmxtime(tm, zone));
					day = j -= TM_DAY;
					dir = m;
					j -= tm->tm_wday;
					if (state & (LAST|NEXT|THIS))
					{
						if (j < 0)
							j += 7;
					}
					else if (j > 0)
						j -= 7;
					tm->tm_mday += j + m * 7;
					set |= DAY;
					if (state & (LAST|NEXT|THIS))
						goto clear_hour;
					continue;
				case TM_MONTH_ABBREV:
					j += TM_MONTH - TM_MONTH_ABBREV;
					/*FALLTHROUGH*/
				case TM_MONTH:
					if (state & MONTH)
						goto done;
					state |= MONTH;
					i = tm->tm_mon;
					tm->tm_mon = j - TM_MONTH;
					if (n < 0)
					{
						while (skip[*s])
							s++;
						if (isdigit(*s))
						{
							n = strtol(s, &t, 10);
							if (n <= 31 && *t != ':')
								s = t;
							else
								n = -1;
						}
					}
					if (n >= 0)
					{
						if (n > 31)
							goto done;
						state |= MDAY;
						tm->tm_mday = n;
						if (f > 0)
							tm->tm_year += f;
					}
					if (state & (LAST|NEXT|THIS))
					{
						n = i;
						goto rel_month;
					}
					continue;
				case TM_UT:
					if (state & ZONE)
						goto done;
					state |= ZONE;
					zone = tmgoff(s, &t, 0);
					s = t;
					continue;
				case TM_DT:
					if (!dst)
						goto done;
					if (!(state & ZONE))
					{
						dst = tm_info.zone->dst;
						zone = tm_info.zone->west;
					}
					zone += tmgoff(s, &t, dst);
					s = t;
					dst = 0;
					state |= ZONE;
					continue;
				case TM_NOISE:
					continue;
				}
			}
			if (!(state & ZONE) && (zp = tmzone(s, &t, type, &dst)))
			{
				s = t;
				zone = zp->west + dst;
				tm_info.date = zp;
				state |= ZONE;
				continue;
			}
			if (!type && (zp = tmtype(s, &t)))
			{
				s = t;
				type = zp->type;
				continue;
			}
			state |= BREAK;
		}
		else if (*s == '/')
		{
			if (!(state & (YEAR|MONTH)) && n >= 1900 && n < 3000 && (i = strtol(s + 1, &t, 10)) > 0 && i <= 12)
			{
				state |= YEAR;
				tm->tm_year = n - 1900;
				s = t;
				i--;
			}
			else
			{
				if ((state & MONTH) || n <= 0 || n > 31)
					break;
				if (isalpha(*++s))
				{
					if ((i = tmlex(s, &t, tm_info.format, TM_DAY_ABBREV, NiL, 0)) < 0)
						break;
					if (i >= TM_MONTH)
						i -= TM_MONTH;
					s = t;
				}
				else
				{
					i = n - 1;
					n = strtol(s, &t, 10);
					s = t;
					if (n <= 0 || n > 31)
						break;
					if (*s == '/' && !isdigit(*(s + 1)))
						break;
				}
				state |= DAY;
				tm->tm_mday = n;
			}
			state |= MONTH;
			n = tm->tm_mon;
			tm->tm_mon = i;
			if (*s == '/')
			{
				n = strtol(++s, &t, 10);
				w = t - s;
				s = t;
				if (*s == '/' || *s == ':' || *s == '-' || *s == '_')
					s++;
			}
			else
			{
				if (state & (LAST|NEXT|THIS))
				{
				rel_month:
					if (state & LAST)
						tm->tm_year -= (tm->tm_mon < n) ? 0 : 1;
					else
						tm->tm_year += ((state & NEXT) ? 1 : 0) + ((tm->tm_mon < n) ? 1 : 0);
					if (state & MDAY)
						goto clear_hour;
					goto clear_mday;
				}
				continue;
			}
		}
		if (n < 0 || w > 4)
			break;
		if (w == 4)
		{
			if ((state & YEAR) || n < 1900 || n >= 3000)
				break;
			state |= YEAR;
			tm->tm_year = n - 1900;
		}
		else if (w == 3)
		{
			if (state & (MONTH|MDAY|WDAY))
				break;
			state |= MONTH|MDAY;
			tm->tm_mon = 0;
			tm->tm_mday = n;
		}
		else if (w == 2 && !(state & YEAR))
		{
			state |= YEAR;
			if (n < TM_WINDOW)
				n += 100;
			tm->tm_year = n;
		}
		else if (!(state & MONTH) && n >= 1 && n <= 12)
		{
			state |= MONTH;
			tm->tm_mon = n - 1;
		}
		else if (!(state & (MDAY|WDAY)) && n >= 1 && n <= 31)
		{
			state |= MDAY|WDAY;
			tm->tm_mday = n;
		}
		else
			break;
		if (state & BREAK)
		{
			last = t;
			break;
		}
		continue;
	clear_mon:
		if ((set|state) & (EXACT|MONTH))
			continue;
		tm->tm_mon = 0;
	clear_mday:
		set |= MONTH;
		if ((set|state) & (EXACT|DAY|HOUR))
			continue;
		tm->tm_mday = 1;
	clear_hour:
		set |= DAY;
		if ((set|state) & (EXACT|HOUR))
			continue;
		tm->tm_hour = 0;
	clear_min:
		set |= HOUR;
		if ((set|state) & (EXACT|MINUTE))
			continue;
		tm->tm_min = 0;
	clear_sec:
		set |= MINUTE;
		if ((set|state) & (EXACT|SECOND))
			continue;
		tm->tm_sec = 0;
	clear_nsec:
		set |= SECOND;
		if ((set|state) & (EXACT|NSEC))
			continue;
		tm->tm_nsec = 0;
	}
 done:
	if (day >= 0 && !(state & (MDAY|WDAY)))
	{
		if ((m = dir) > 0)
			m--;
		if (state & MONTH)
			tm->tm_mday = 1;
		else if (m < 0)
			m++;
		tm = tmxmake(tmxtime(tm, zone));
		j = day - tm->tm_wday;
		if (j < 0)
			j += 7;
		tm->tm_mday += j + m * 7;
		if (state & FINAL)
			for (n = tm_data.days[tm->tm_mon] + (tm->tm_mon == 1 && tmisleapyear(tm->tm_year)); (tm->tm_mday + 7) <= n; tm->tm_mday += 7);
	}
	else if (day < 0 && (state & FINAL) && (set & DAY))
		tm->tm_mday = tm_data.days[tm->tm_mon] + (tm->tm_mon == 1 && tmisleapyear(tm->tm_year));
	if (e)
		*e = last;
	return tmxtime(tm, zone);
}
