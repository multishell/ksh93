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

#include <ast.h>

/*
 * convert string to 4 byte local byte order ip address
 * with optional prefix bits
 * pointer to first unused char placed in *e
 * return 0:ok <0:error
 *
 * valid addresses match the egrep RE:
 *
 *	[0-9]{1,3}(\.[0-9]{1,3})*|0[xX][0-9a-fA-Z]+
 *
 * valid bits/masks match the egrep RE:
 *
 *	(/([0-9]+|[0-9]{1,3}(\.[0-9]{1,3})*))?
 *
 * if pbits!=0 and no bits/mask specified then trailing 0's in addr
 * are used to compute the mask
 */

int
strtoip4(register const char* s, char** e, unsigned _ast_int4_t* paddr, unsigned char* pbits)
{
	register int			c;
	register unsigned int		n;
	register unsigned _ast_int4_t	addr;
	register int			part;
	register unsigned char		bits;
	unsigned _ast_int4_t		z;
	int				old;
	const char*			b;

	while ((c = *s) == ' ' || c == '\t')
		s++;
	b = s;
	addr = 0;
	bits = 0;
	part = 0;
	do
	{
		n = 0;
		while ((c = *s++) >= '0' && c <= '9')
			n = n * 10 + (c - '0');
		if ((c == 'x' || c == 'X') && !part)
		{
			addr = n;
			for (;;)
			{
				if ((c = *s++) >= '0' && c <= '9')
					c -= '0';
				else if (c >= 'a' && c <= 'f')
					c -= 'a' - 10;
				else if (c >= 'A' && c <= 'F')
					c -= 'F' - 10;
				else
					break;
				addr = addr * 16 + c;
			}
			part = 4;
			break;
		}
		if (n > 0xff)
			return -1;
		addr = (addr << 8) | n;
		part++;
	} while (c == '.');
	if ((s - b) == 1 && c != '/')
		return -1;
	if (part > 4)
		return -1;
	if (old = part < 4)
		while (part++ < 4)
			addr <<= 8;
	if (pbits)
	{
		if (c == '/')
		{
			part = 0;
			z = 0;
			for (;;)
			{
				n = 0;
				while ((c = *s++) >= '0' && c <= '9')
					n = n * 10 + (c - '0');
				z = (z << 8) | n;
				part++;
				if (c != '.')
					break;
				old = 1;
			}
			if (part > 4)
				return -1;
			if (z <= 32 && (!old || part < 2))
				bits = z;
			else if (z)
			{
				if (part == 4 && (z & 0x8000001) == 1)
					z = ~z;
				while (!(z & 1))
					z >>= 1;
				while (z & 1)
				{
					z >>= 1;
					bits++;
				}
			}
		}
		else if ((z = (addr >> 24)) < 128)
			bits = 8;
		else if (z < 192)
			bits = 16;
		else
			bits = 24;
		if (*pbits = bits)
			addr &= ~((((unsigned _ast_int4_t)1)<<(32-bits))-1);
		else
			addr = 0;
	}
	if (paddr)
		*paddr = addr;
	if (e)
		*e = (char*)(s - 1);
	return 0;
}
