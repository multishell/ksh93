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
#include	"sfdchdr.h"

/*	A discipline to tee the output to a stream to another stream.
**	This is similar to what the "tee" program does. As implemented
**	this discipline only works with file streams.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 03/18/1998.
*/

/* the discipline structure for tee-ing */
typedef struct _tee_s
{	Sfdisc_t	disc;	/* the sfio discipline structure */
	Sfio_t*		tee;	/* the stream to tee to */
	int		status;	/* if tee stream is still ok */
} Tee_t;

/*	write to the teed stream.  */
#if __STD_C
static ssize_t teewrite(Sfio_t* f, const Void_t* buf, size_t size, Sfdisc_t* disc)
#else
static ssize_t teewrite(f,buf,size,disc)
Sfio_t* 	f;	/* the stream being written to */
Void_t*		buf;	/* the buffer of data being output */
size_t		size;	/* the data size */
Sfdisc_t*	disc;	/* the tee discipline */
#endif
{
	reg Tee_t*	te = (Tee_t*)disc;

	/* tee data if still ok */
	if(te->status == 0 && sfwrite(te->tee,buf,size) != (ssize_t)size)
		te->status = -1;

	/* do the actual write */
	return sfwr(f,buf,size,disc);
}

/* on close, remove the discipline */
#if __STD_C
static teeexcept(Sfio_t* f, int type, Void_t* data, Sfdisc_t* disc)
#else
static teeexcept(f,type,data,disc)
Sfio_t*		f;
int		type;
Void_t*		data;
Sfdisc_t*	disc;
#endif
{
	if(type == SF_FINAL || type == SF_DPOP)
		free(disc);

	return 0;
}

#if __STD_C
int sfdctee(Sfio_t* f, Sfio_t* tee)
#else
int sfdctee(f, tee)
Sfio_t*	f;	/* stream to tee from	*/
Sfio_t*	tee;	/* stream to tee to	*/
#endif
{
	reg Tee_t*	te;

	if(!(te = (Tee_t*)malloc(sizeof(Tee_t))) )
		return -1;
	memset(te, 0, sizeof(*te));

	te->disc.writef = teewrite;
	te->disc.exceptf = teeexcept;
	te->tee = tee;

	if(sfdisc(f,(Sfdisc_t*)te) != (Sfdisc_t*)te)
	{	free(te);
		return -1;
	}

	return 0;
}
