/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2008 AT&T Intellectual Property          *
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
*                  David Korn <dgk@research.att.com>                   *
*                                                                      *
***********************************************************************/
#pragma prototyped

/*
 * code for tree nodes and name walking
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"defs.h"
#include	"name.h"
#include	"argnod.h"
#include	"lexstates.h"

struct nvdir
{
	Dt_t		*root;
	Namval_t	*hp;
	Namval_t	*table;
	Namval_t	*otable;
	Namval_t	*(*nextnode)(Namval_t*,Dt_t*,Namfun_t*);
	Namfun_t	*fun;
	struct nvdir	*prev;
	int		len;
	char		data[1];
};

char *nv_getvtree(Namval_t*, Namfun_t *);
static void put_tree(Namval_t*, const char*, int,Namfun_t*);

static int read_tree(Namval_t* np, Sfio_t *iop, int n, Namfun_t *dp)
{
	Sfio_t	*sp;
	char	*cp;
	int	c;
	if(n>=0)
		return(-1);
	while((c = sfgetc(iop)) &&  isblank(c));
	sfungetc(iop,c);
	sfprintf(sh.strbuf,"%s=%c",nv_name(np),0);
	cp = sfstruse(sh.strbuf);
	sp = sfopen((Sfio_t*)0,cp,"s");
	sfstack(iop,sp);
	c=sh_eval(iop,SH_READEVAL);
	return(c);
}

static Namval_t *create_tree(Namval_t *np,const char *name,int flag,Namfun_t *dp)
{
	register Namfun_t *fp=dp;
	while(fp=fp->next)
	{
		if(fp->disc && fp->disc->createf)
		{
			if(np=(*fp->disc->createf)(np,name,flag,fp))
				dp->last = fp->last;
			return(np);
		}
	}
	return((flag&NV_NOADD)?0:np);
}

static const Namdisc_t treedisc =
{
	0,
	put_tree,
	nv_getvtree,
	0,
	0,
	create_tree
	,0,0,0,0,
	read_tree
};

static char *nextdot(const char *str)
{
	register char *cp;
	if(*str=='.')
		str++;
	if(*str =='[')
	{
		cp = nv_endsubscript((Namval_t*)0,(char*)str,0);
		return(*cp=='.'?cp:0);
	}
	else
		return(strchr(str,'.'));
}

static  Namfun_t *nextdisc(Namval_t *np)
{
	register Namfun_t *fp;
	if(nv_isref(np))
		return(0);
        for(fp=np->nvfun;fp;fp=fp->next)
	{
		if(fp && fp->disc && fp->disc->nextf)
			return(fp);
	}
	return(0);
}

void *nv_diropen(Namval_t *np,const char *name)
{
	char *next,*last;
	int c,len=strlen(name);
	struct nvdir *save, *dp = new_of(struct nvdir,len);
	Namval_t *nq,fake;
	Namfun_t *nfp=0;
	if(!dp)
		return(0);
	memset((void*)dp, 0, sizeof(*dp));
	if(name[len-1]=='*' || name[len-1]=='@')
		len -= 1;
	name = memcpy(dp->data,name,len);
	dp->data[len] = 0;
	dp->len = len;
	dp->root = sh.last_root?sh.last_root:sh.var_tree;
#if 1
	while(1)
	{
		dp->table = sh.last_table;
		sh.last_table = 0;
		if(*(last=(char*)name)==0)
			break;
		if(!(next=nextdot(last)))
			break;
		
		*next = 0;
		np = nv_open(name, dp->root, NV_NOFAIL);
		*next = '.';
		if(!np || !nv_istable(np))
			break;
		dp->root = nv_dict(np);
		name = next+1;
	}
#else
	dp->table = sh.last_table;
	sh.last_table = 0;
	last = dp->data;
#endif
	if(*name)
	{
		fake.nvname = (char*)name;
		c = strlen(name);
		dp->hp = (Namval_t*)dtprev(dp->root,&fake);
		if(!(dp->hp && memcmp(name,dp->hp->nvname,c)==0 && (dp->hp->nvname[c]==0 || dp->hp->nvname[c]=='.')))
			dp->hp = (Namval_t*)dtnext(dp->root,dp->hp);
	}
	else
		dp->hp = (Namval_t*)dtfirst(dp->root);
	while(1)
	{
		if(!last)
			next = 0;
		else if(next= nextdot(last))
		{
			c = *next;
			*next = 0;
		}
		if(!np)
		{
			if(nfp && nfp->disc && nfp->disc->createf)
				np =  (*nfp->disc->createf)(nq,last,0,nfp);
			else
				np = nv_search(last,dp->root,0);
		}
		if(next)
			*next = c;
		if(np==dp->hp && !next)
			dp->hp = (Namval_t*)dtnext(dp->root,dp->hp);
		if(np && ((nfp=nextdisc(np)) || nv_istable(np)))
		{
			if(!(save = new_of(struct nvdir,0)))
				return(0);
			*save = *dp;
			dp->prev = save;
			if(nv_istable(np))
				dp->root = nv_dict(np);
			else
				dp->root = (Dt_t*)dp;
			if(nfp)
			{
				dp->nextnode = nfp->disc->nextf;
				dp->table = np;
				dp->otable = sh.last_table;
				dp->fun = nfp;
				dp->hp = (*dp->nextnode)(np,(Dt_t*)0,nfp);
			}
			else
				dp->nextnode = 0;
		}
		else
			break;
		if(!next || next[1]==0)
			break;
		last = next+1;
		nq = np;
		np = 0;
	}
	return((void*)dp);
}


static Namval_t *nextnode(struct nvdir *dp)
{
	if(dp->nextnode)
		return((*dp->nextnode)(dp->hp,dp->root,dp->fun));
	if(dp->len && memcmp(dp->data, dp->hp->nvname, dp->len))
		return(0);
	return((Namval_t*)dtnext(dp->root,dp->hp));
}

char *nv_dirnext(void *dir)
{
	register struct nvdir *save, *dp = (struct nvdir*)dir;
	register Namval_t *np, *last_table;
	register char *cp;
	Namfun_t *nfp;
	while(1)
	{
		while(np=dp->hp)
		{
			char *sptr;
			dp->hp = nextnode(dp);
			if(nv_isnull(np) && !nv_isarray(np))
				continue;
			last_table = sh.last_table;
			if(dp->table && dp->otable && !nv_isattr(dp->table,NV_MINIMAL))
			{
				sptr = dp->table->nvenv;
				dp->table->nvenv = (char*)dp->otable;
			}
			sh.last_table = dp->table;
			cp = nv_name(np);
			if(dp->table && dp->otable && !nv_isattr(dp->table,NV_MINIMAL))
				dp->table->nvenv = sptr;
			sh.last_table = last_table;
			if(!dp->len || memcmp(cp,dp->data,dp->len)==0)
			{
				if((nfp=nextdisc(np)) && (nfp->disc->getval||nfp->disc->getnum) && nv_isvtree(np) && strcmp(cp,dp->data))
					nfp = 0;
				if(nfp || nv_istable(np))
				{
					Dt_t *root;
					if(nv_istable(np))
						root = nv_dict(np);
					else
						root = (Dt_t*)dp;
					/* check for recursive walk */
					for(save=dp; save;  save=save->prev) 
					{
						if(save->root==root)
							break;
					}
					if(save)
						continue;
					if(!(save = new_of(struct nvdir,0)))
						return(0);
					*save = *dp;
					dp->prev = save;
					dp->root = root;
					dp->len = 0;
					if(nfp && np->nvfun)
					{
						dp->nextnode = nfp->disc->nextf;
						dp->otable = dp->table;
						dp->table = np;
						dp->fun = nfp;
						dp->hp = (*dp->nextnode)(np,(Dt_t*)0,nfp);
					}
					else
						dp->nextnode = 0;
				}
				return(cp);
			}
		}
		if(!(save=dp->prev))
			break;
		*dp = *save;
		if(np==0)
			dp->hp = 0;
		free((void*)save);
	}
	return(0);
}

void nv_dirclose(void *dir)
{
	struct nvdir *dp = (struct nvdir*)dir;
	if(dp->prev)
		nv_dirclose((void*)dp->prev);
	free(dir);
}

static void outtype(Namval_t *np, Namfun_t *fp, Sfio_t* out, const char *prefix)
{
	char *type=0;
	Namval_t *tp = fp->type;
	if(!tp && fp->disc && fp->disc->typef) 
		tp = (*fp->disc->typef)(np,fp);
	for(fp=fp->next;fp;fp=fp->next)
	{
		if(fp->type || (fp->disc && fp->disc->typef &&(*fp->disc->typef)(np,fp)))
		{
			outtype(np,fp,out,prefix);
			break;
		}
	}
	if(prefix && *prefix=='t')
		type = "-T";
	else if(!prefix)
		type = "type";
	if(type)
	{
		char *cp=tp->nvname;
		if(cp=strrchr(cp,'.'))
			cp++;
		else
			cp = tp->nvname;
		sfprintf(out,"%s %s ",type,cp);
	}
}

/*
 * print the attributes of name value pair give by <np>
 */
void nv_attribute(register Namval_t *np,Sfio_t *out,char *prefix,int noname)
{
	register const Shtable_t *tp;
	register char *cp;
	register unsigned val,mask,attr;
	char *ip=0;
	Namfun_t *fp=0; 
	Namval_t *typep=0;
	for(fp=np->nvfun;fp;fp=fp->next)
	{
		if((typep=fp->type) || (fp->disc && fp->disc->typef && (typep=(*fp->disc->typef)(np,fp))))
			break;
	}
	if(!fp  && !nv_isattr(np,~(NV_MINIMAL|NV_NOFREE)))
	{
		if(nv_isvtree(np) && prefix && *prefix)
			sfprintf(out,"%s -C ",prefix);
		return;
	}

	if ((attr=nv_isattr(np,~NV_NOFREE)) || fp)
	{
		if((attr&NV_NOPRINT|NV_INTEGER)==NV_NOPRINT)
			attr &= ~NV_NOPRINT;
		if(!attr && !fp)
			return;
		if(fp)
		{
			prefix = Empty;
			attr &= NV_RDONLY|NV_ARRAY;
			if(nv_isattr(np,NV_REF|NV_TAGGED)==(NV_REF|NV_TAGGED))
				attr |= (NV_REF|NV_TAGGED);
			if(typep)
			{
				char *cp = typep->nvname;
				if(cp = strrchr(cp,'.'))
					cp++;
				else
					cp = typep->nvname;
				sfputr(out,cp,' ');
				fp = 0;
			}
		}
		else if(prefix && *prefix)
			sfputr(out,prefix,' ');
		for(tp = shtab_attributes; *tp->sh_name;tp++)
		{
			val = tp->sh_number;
			mask = val;
			if(fp && (val&NV_INTEGER))
				break;
			/*
			 * the following test is needed to prevent variables
			 * with E attribute from being given the F
			 * attribute as well
			*/
			if(val==NV_DOUBLE && (attr&(NV_EXPNOTE|NV_HEXFLOAT)))
				continue;
			if(val&NV_INTEGER)
				mask |= NV_DOUBLE;
			else if(val&NV_HOST)
				mask = NV_HOST;
			if((attr&mask)==val)
			{
				if(val==NV_ARRAY)
				{
					Namarr_t *ap = nv_arrayptr(np);
					char **xp=0;
					if(ap && array_assoc(ap))
					{
						if(tp->sh_name[1]!='A')
							continue;
					}
					else if(tp->sh_name[1]=='A')
						continue;
					if(ap && (ap->nelem&ARRAY_TREE))
					{
						if(prefix && *prefix)
							sfprintf(out,"%s -C ",prefix);
					}
					if(ap && !array_assoc(ap) && (xp=(char**)(ap+1)) && *xp)
						ip = nv_namptr(*xp,0)->nvname;
				}
				if(prefix)
				{
					if(*tp->sh_name=='-')
						sfprintf(out,"%.2s ",tp->sh_name);
					if(ip)
					{
						sfprintf(out,"[%s] ",ip);
						ip = 0;
					}
				}
				else
					sfputr(out,tp->sh_name+2,' ');
		                if ((val&(NV_LJUST|NV_RJUST|NV_ZFILL)) && !(val&NV_INTEGER) && val!=NV_HOST)
					sfprintf(out,"%d ",nv_size(np));
				if(val==(NV_REF|NV_TAGGED))
					attr &= ~(NV_REF|NV_TAGGED);
			}
		        if(val==NV_INTEGER && nv_isattr(np,NV_INTEGER))
			{
				if(nv_size(np) != 10)
				{
					if(nv_isattr(np, NV_DOUBLE)== NV_DOUBLE)
						cp = "precision";
					else
						cp = "base";
					if(!prefix)
						sfputr(out,cp,' ');
					sfprintf(out,"%d ",nv_size(np));
				}
				break;
			}
		}
		if(fp)
			outtype(np,fp,out,prefix);
		if(noname)
			return;
		sfputr(out,nv_name(np),'\n');
	}
}

struct Walk
{
	Sfio_t	*out;
	Dt_t	*root;
	int	noscope;
	int	indent;
	int	nofollow;
	int	array;
};

void nv_outnode(Namval_t *np, Sfio_t* out, int indent, int special)
{
	char		*fmtq,*ep,*xp;
	Namval_t	*mp;
	Namarr_t	*ap = nv_arrayptr(np);
	int		c,more,associative = 0;
	if(ap)
	{
		if(!(ap->nelem&ARRAY_SCAN))
			nv_putsub(np,NIL(char*),ARRAY_SCAN);
		sfputc(out,'(');
		if(indent>=0)
		{
			sfputc(out,'\n');
			sfnputc(out,'\t',++indent);
		}
		if(!(associative =(array_assoc(ap)!=0)))
		{
			if(array_elem(ap) < nv_aimax(np)+1)
				associative=1;
		}
	}
	mp = nv_opensub(np);
	while(1)
	{
		if(mp && special && nv_isvtree(mp))
		{
			if(!nv_nextsub(np))
				break;
			mp = nv_opensub(np);
			continue;
		}
		if(associative||special)
		{
			if(!(fmtq = nv_getsub(np)))
				break;
			sfprintf(out,"[%s]",sh_fmtq(fmtq));
			sfputc(out,'=');
		}
		if(mp && nv_isarray(mp))
		{
			nv_outnode(mp, out, indent+(indent>=0),0);
			if(indent>0)
				sfnputc(out,'\t',indent);
			sfputc(out,')');
			sfputc(out,indent>=0?'\n':' ');
			more = nv_nextsub(np);
			goto skip;
		}
		if(mp && nv_isvtree(mp))
			nv_onattr(mp,NV_EXPORT);
		if(!(ep=nv_getval(mp?mp:np)))
			continue;
		xp = 0;
		if(nv_isattr(np,NV_INTEGER|NV_LJUST)==NV_LJUST)
		{
			xp = ep+nv_size(np);
			while(--xp>ep && *xp==' ');
			if(xp>ep || *xp!=' ')
				xp++;
			if(xp < (ep+nv_size(np)))
				*xp = 0;
			else
				xp = 0;
		}
		if(mp && nv_isvtree(mp))
			fmtq = ep;
		else if(!(fmtq = sh_fmtq(ep)))
			fmtq = "";
		else if(!associative && (ep=strchr(fmtq,'=')))
		{
			char *qp = strchr(fmtq,'\'');
			if(!qp || qp>ep)
			{
				sfwrite(out,fmtq,ep-fmtq);
				sfputc(out,'\\');
				fmtq = ep;
			}
		}
		more = nv_nextsub(np);
		c = '\n';
		if(indent<0)
		{
			c = ';';
			if(ap)
				c = more?' ':-1;
		}
		sfputr(out,fmtq,c);
		if(xp)
			*xp = ' ';
	skip:
		if(!more)
			return;
		mp = nv_opensub(np);
		if(indent>0 && !(mp && special && nv_isvtree(mp)))
			sfnputc(out,'\t',indent);
	}
}

static void outval(char *name, const char *vname, struct Walk *wp)
{
	register Namval_t *np, *nq;
        register Namfun_t *fp;
	int isarray=0, associative=0, special=0,mode=0;
	if(*name!='.' || vname[strlen(vname)-1]==']')
		mode = NV_ARRAY;
	if(!(np=nv_open(vname,wp->root,mode|NV_VARNAME|NV_NOADD|NV_NOASSIGN|NV_NOFAIL|wp->noscope)))
		return;
	fp = nv_hasdisc(np,&treedisc);
	if(*name=='.')
	{
		if(nv_isattr(np,NV_BINARY))
			return;
		if(fp && np->nvalue.cp && np->nvalue.cp!=Empty)
		{
			nv_local = 1;
			fp = 0;
		}
		if(fp)
			return;
		if(nv_isarray(np))
			return;
	}
	if(!special && fp)
	{
		Namfun_t *xp;
		if(!wp->out)
		{
			fp = nv_stack(np,fp);
			if(fp = nv_stack(np,NIL(Namfun_t*)))
				free((void*)fp);
			np->nvfun = 0;
			return;
		}
		for(xp=fp->next; xp; xp = xp->next)
		{
			if(xp->disc && (xp->disc->getval || xp->disc->getnum))
				break;
		}
		if(!xp)
			return;
	}
	if((nv_isnull(np) || np->nvalue.cp==Empty) && !nv_isarray(np))
		return;
	if(special || (nv_isarray(np) && nv_arrayptr(np)))
	{
		isarray=1;
		associative= nv_aindex(np)<0;
		if(array_elem(nv_arrayptr(np))==0)
			isarray=2;
		else
			nq = nv_putsub(np,NIL(char*),ARRAY_SCAN|(wp->out?ARRAY_NOCHILD:0));
	}
	if(!wp->out)
	{
		_nv_unset(np,NV_RDONLY);
		nv_close(np);
		return;
	}
	if(isarray==1 && !nq)
		return;
	if(isarray==0 && nv_isarray(np) && nv_isnull(np))  /* empty array */
		isarray = 2;
	special |= wp->nofollow;
	if(!wp->array && wp->indent>0)
		sfnputc(wp->out,'\t',wp->indent);
	if(!special)
	{
		if(*name!='.')
			nv_attribute(np,wp->out,"typeset",'=');
		nv_outname(wp->out,name,-1);
		sfputc(wp->out,(isarray==2?'\n':'='));
		if(isarray==2)
			return;
	}
	fp = np->nvfun;
	if(*name=='.' && !isarray)
		np->nvfun = 0;
	nv_outnode(np, wp->out, wp->indent, special);
	if(*name=='.' && !isarray)
		np->nvfun = fp;
	if(isarray && !special)
	{
		if(wp->indent>0)
		{
			sfnputc(wp->out,'\t',wp->indent);
			sfwrite(wp->out,")\n",2);
		}
		else
			sfwrite(wp->out,");",2);
	}
}

/*
 * format initialization list given a list of assignments <argp>
 */
static char **genvalue(char **argv, const char *prefix, int n, struct Walk *wp)
{
	register char *cp,*nextcp,*arg;
	register Sfio_t *outfile = wp->out;
	register int m,r,l;
	if(n==0)
		m = strlen(prefix);
	else if(cp=nextdot(prefix))
		m = cp-prefix;
	else
		m = strlen(prefix)-1;
	m++;
	if(outfile && !wp->array)
	{
		sfputc(outfile,'(');
		if(wp->indent>=0)
		{
			wp->indent++;
			sfputc(outfile,'\n');
		}
	}
	for(; arg= *argv; argv++)
	{
		cp = arg + n;
		if(n==0 && cp[m-1]!='.')
			continue;
		if(n && cp[m-1]==0)
			break;
		if(n==0 || strncmp(arg,prefix-n,m+n)==0)
		{
			cp +=m;
			r = 0;
			if(*cp=='.')
				cp++,r++;
			if(nextcp=nextdot(cp))
			{
				if(outfile)
				{
					Namval_t *np,*tp;
					*nextcp = 0;
					np=nv_open(arg,wp->root,NV_VARNAME|NV_NOADD|NV_NOASSIGN|NV_NOFAIL|wp->noscope);
					if(!np || (nv_isarray(np) && (!(tp=nv_opensub(np)) || !nv_isvtree(tp))))
					{
						*nextcp = '.';
						continue;
					}
					if(wp->indent>=0)
						sfnputc(outfile,'\t',wp->indent);
					if(tp = nv_type(np))
					{
						char *sp;
						if(sp = strrchr(tp->nvname,'.'))
							sp++;
						else
							sp = tp->nvname;
						sfputr(outfile,sp,' ');
					}
					nv_outname(outfile,cp,nextcp-cp);
					sfputc(outfile,'=');
					*nextcp = '.';
				}
				else
					continue;
				argv = genvalue(argv,cp,n+m+r,wp);
				if(wp->indent>=0)
					sfputc(outfile,'\n');
				if(*argv)
					continue;
				break;
			}
			else if(outfile && !wp->nofollow && argv[1] && memcmp(arg,argv[1],l=strlen(arg))==0 && argv[1][l]=='[')
			{
				Namval_t *np = nv_open(arg,wp->root,NV_VARNAME|NV_NOADD|NV_NOASSIGN|wp->noscope);
				if(!np)
					continue;
				wp->array = nv_isarray(np);
				if(wp->indent>0)
					sfnputc(outfile,'\t',wp->indent);
				nv_attribute(np,outfile,"typeset",1);
				nv_close(np);
				sfputr(outfile,arg+m+r+(n?n:0),'=');
				wp->nofollow=1;
				argv = genvalue(argv,cp,cp-arg ,wp);
				sfputc(outfile,wp->indent<0?';':'\n');
			}
			else if(outfile && *cp=='[')
			{
				if(wp->indent)
					sfnputc(outfile,'\t',wp->indent);
				sfputr(outfile,cp,'=');
				argv = genvalue(++argv,cp,cp-arg ,wp);
				sfputc(outfile,'\n');
			}
			else
			{
				outval(cp,arg,wp);
				if(wp->array)
				{
					if(wp->indent>=0)
						wp->indent++;
					else
						sfputc(outfile,' ');
					wp->array = 0;
				}
			}
		}
		else
			break;
		wp->nofollow = 0;
	}
	wp->array = 0;
	if(outfile)
	{
		int c = prefix[m-1];
		cp = (char*)prefix;
		if(c=='.')
			cp[m-1] = 0;
		outval(".",prefix-n,wp);
		if(c=='.')
			cp[m-1] = c;
		if(wp->indent>0)
			sfnputc(outfile,'\t',--wp->indent);
		sfputc(outfile,')');
	}
	return(--argv);
}

/*
 * walk the virtual tree and print or delete name-value pairs
 */
static char *walk_tree(register Namval_t *np, int flags)
{
	static Sfio_t *out;
	struct Walk walk;
	Sfio_t *outfile;
	int savtop = staktell();
	char *savptr = stakfreeze(0);
	register struct argnod *ap=0; 
	struct argnod *arglist=0;
	char *name,*cp, **argv;
	char *subscript=0;
	void *dir;
	int n=0, noscope=(flags&NV_NOSCOPE);
	Namarr_t *arp = nv_arrayptr(np);
	Dt_t	*save_tree = sh.var_tree;
	Namval_t	*mp=0;
	if(sh.last_root)
		sh.var_tree = sh.last_root;
	stakputs(nv_name(np));
	if(arp && !(arp->nelem&ARRAY_SCAN) && (subscript = nv_getsub(np)))
	{
		mp = nv_opensub(np);
		stakputc('[');
		stakputs(subscript);
		stakputc(']');
		stakputc('.');
	}
	else if(*stakptr(staktell()-1) == ']')
		mp = np;
	name = stakfreeze(1);
	sh.last_root = 0;
	dir = nv_diropen(mp,name);
	walk.root = sh.last_root;
	if(subscript)
		name[strlen(name)-1] = 0;
	while(cp = nv_dirnext(dir))
	{
		stakseek(ARGVAL);
		stakputs(cp);
		ap = (struct argnod*)stakfreeze(1);
		ap->argflag = ARG_RAW;
		ap->argchn.ap = arglist; 
		n++;
		arglist = ap;
	}
	argv = (char**)stakalloc((n+1)*sizeof(char*));
	argv += n;
	*argv = 0;
	for(; ap; ap=ap->argchn.ap)
		*--argv = ap->argval;
	nv_dirclose(dir);
	if(flags&1)
		outfile = 0;
	else if(!(outfile=out))
		outfile = out =  sfnew((Sfio_t*)0,(char*)0,-1,-1,SF_WRITE|SF_STRING);
	else
		sfseek(outfile,0L,SEEK_SET);
	walk.out = outfile;
	walk.indent = (flags&NV_EXPORT)?-1:0;
	walk.nofollow = 0;
	walk.noscope = noscope;
	walk.array = 0;
	genvalue(argv,name,0,&walk);
	stakset(savptr,savtop);
	sh.var_tree = save_tree;
	if(!outfile)
		return((char*)0);
	sfputc(out,0);
	return((char*)out->_data);
}

Namfun_t *nv_isvtree(Namval_t *np)
{
	if(np)
		return(nv_hasdisc(np,&treedisc));
	return(0);
}

/*
 * get discipline for compound initializations
 */
char *nv_getvtree(register Namval_t *np, Namfun_t *fp)
{
	int flags=0;	
	for(; fp && fp->next; fp=fp->next)
	{
		if(fp->next->disc && (fp->next->disc->getnum || fp->next->disc->getval))
			return(nv_getv(np,fp));
	}
	if(nv_isattr(np,NV_BINARY) &&  !nv_isattr(np,NV_RAW))
		return(nv_getv(np,fp));
	if(nv_isattr(np,NV_ARRAY) && !nv_type(np) && nv_arraychild(np,(Namval_t*)0,0)==np)
		return(nv_getv(np,fp));
	if(flags = nv_isattr(np,NV_EXPORT))
		nv_offattr(np,NV_EXPORT);
	return(walk_tree(np,flags));
}

/*
 * put discipline for compound initializations
 */
static void put_tree(register Namval_t *np, const char *val, int flags,Namfun_t *fp)
{
	struct Namarray *ap;
	int nleft = 0;
	if(!val && !fp->next && nv_isattr(np,NV_NOFREE))
		return;
	if(!nv_isattr(np,(NV_INTEGER|NV_BINARY)))
		walk_tree(np,(flags&NV_NOSCOPE)|1);
	nv_putv(np, val, flags,fp);
	if(val && nv_isattr(np,(NV_INTEGER|NV_BINARY)))
		return;
	if(ap= nv_arrayptr(np))
		nleft = array_elem(ap);
	if(nleft==0)
	{
		fp = nv_stack(np,fp);
		if(fp = nv_stack(np,NIL(Namfun_t*)))
			free((void*)fp);
	}
}

/*
 * Insert discipline to cause $x to print current tree
 */
void nv_setvtree(register Namval_t *np)
{
	register Namfun_t *nfp;
	if(sh.subshell)
		sh_assignok(np,1);
	if(nv_hasdisc(np, &treedisc))
		return;
	nfp = newof(NIL(void*),Namfun_t,1,0);
	nfp->disc = &treedisc;
	nv_stack(np, nfp);
}

