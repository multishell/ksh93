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
 * David Korn
 * AT&T Labs
 *
 */

#include	"defs.h"
#include	<fcin.h>
#include	<ls.h>
#include	<nval.h>
#include	"variables.h"
#include	"path.h"
#include	"io.h"
#include	"jobs.h"
#include	"history.h"
#include	"test.h"
#include	"FEATURE/externs"

#define RW_ALL	(S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH)

struct edata
{
	Shell_t		*sh;
	char		**envp;
	Pathcomp_t	*libpath;
	int		exec_err;
};


static Pathcomp_t	*execs(Pathcomp_t*, const char*, char**, struct edata*);
static int		canexecute(char*,int);
static void		funload(Shell_t*,int,const char*);
static void		exscript(Shell_t*,char*, char*[], char**);
static int		path_chkfpath(Pathcomp_t*,Pathcomp_t*,Pathcomp_t*,int);


/*
 * make sure PWD is set up correctly
 * Return the present working directory
 * Invokes getcwd() if flag==0 and if necessary
 * Sets the PWD variable to this value
 */
char *path_pwd(int flag)
{
	register char *cp;
	register char *dfault = (char*)e_dot;
	register int count = 0;
	Shell_t *shp = &sh;
	if(shp->pwd)
		return((char*)shp->pwd);
	while(1) 
	{
		/* try from lowest to highest */
		switch(count++)
		{
			case 0:
				cp = nv_getval(PWDNOD);
				break;
			case 1:
				cp = nv_getval(HOME);
				break;
			case 2:
				cp = "/";
				break;
			case 3:
				cp = (char*)e_crondir;
				if(flag) /* skip next case when non-zero flag */
					++count;
				break;
			case 4:
			{
				if(cp=getcwd(NIL(char*),0))
				{  
					nv_offattr(PWDNOD,NV_NOFREE);
					nv_unset(PWDNOD);
					PWDNOD->nvalue.cp = cp;
					goto skip;
				}
				break;
			}
			case 5:
				return(dfault);
		}
		if(cp && *cp=='/' && test_inode(cp,e_dot))
			break;
	}
	if(count>1)
	{
		nv_offattr(PWDNOD,NV_NOFREE);
		nv_putval(PWDNOD,cp,NV_RDONLY);
	}
skip:
	nv_onattr(PWDNOD,NV_NOFREE|NV_EXPORT);
	shp->pwd = (char*)(PWDNOD->nvalue.cp);
	return(cp);
}

/*
 * delete current Pathcomp_t structure
 */
void  path_delete(Pathcomp_t *first)
{
	register Pathcomp_t *pp=first, *old=0, *ppnext;
	while(pp)
	{
		ppnext = pp->next;
		if(--pp->refcount<=0)
		{
			free((void*)pp);
			if(old)
				old->next = ppnext;
		}
		else
			old = pp;
		pp = ppnext; 
	}
}

/*
 * returns library variable from .fpath
 * The value might be returned on the stack overwriting path
 */
static char *path_lib(Pathcomp_t *pp, char *path)
{
	register char *last = strrchr(path,'/');
	register int r;
	struct stat statb;
	if(last)
		*last = 0;
	else
		path = ".";
	r = stat(path,&statb);
	if(last)
		*last = '/';
	if(r>=0)
	{
		Pathcomp_t pcomp;
		char save[8];
		for( ;pp; pp=pp->next)
		{
			if(pp->ino==statb.st_ino && pp->dev==statb.st_dev)
				return(pp->lib);
		}
		pcomp.len = 0;
		if(last)
			pcomp.len = last-path;
		memcpy((void*)save, (void*)stakptr(PATH_OFFSET+pcomp.len),sizeof(save));
		if(path_chkfpath((Pathcomp_t*)0,(Pathcomp_t*)0,&pcomp,PATH_OFFSET))
			return(stakfreeze(1));
		memcpy((void*)stakptr(PATH_OFFSET+pcomp.len),(void*)save,sizeof(save));
	}
	return(0);
}

#if 1
void path_dump(register Pathcomp_t *pp)
{
	sfprintf(sfstderr,"dump\n");
	while(pp)
	{
		sfprintf(sfstderr,"pp=%x dev=%d ino=%d len=%d flags=%o name=%.*s\n",
			pp,pp->dev,pp->ino,pp->len,pp->flags,pp->len,pp->name);
		pp = pp->next;
	}
}
#endif

/*
 * write the next path to search on the current stack
 * if last is given, all paths that come before <last> are skipped
 * the next pathcomp is returned.
 */
Pathcomp_t *path_nextcomp(register Pathcomp_t *pp, const char *name, Pathcomp_t *last)
{
	stakseek(PATH_OFFSET);
	if(*name=='/')
		pp = 0;
	else
	{
		for(;pp && pp!=last;pp=pp->next)
		{
			if(pp->flags&PATH_SKIP)
				continue;
			if(!last || *pp->name!='/')
				break;
		}
		if(!pp)		/* this should not happen */
			pp = last;
	}
	if(pp && (pp->name[0]!='.' || pp->name[1]))
	{
		if(*pp->name!='/')
		{
			stakputs(path_pwd(1));
			if(*stakptr(staktell()-1)!='/')
				stakputc('/');
		}
		stakwrite(pp->name,pp->len);
		if(pp->name[pp->len-1]!='/')
			stakputc('/');
	}
	stakputs(name);
	stakputc(0);
	return(pp && pp!=last?pp->next:(Pathcomp_t*)0);
}

static void path_init(Shell_t *shp)
{
	const char *val = astconf("PATH",NIL(char*),NIL(char*));
	Pathcomp_t *pp = (void*)path_addpath((Pathcomp_t*)0,(val?val:e_defpath),PATH_PATH);
	if(shp->defpathlist = (void*)pp)
		pp->shp = shp;
	if(val=(PATHNOD)->nvalue.cp)
	{
		pp = (void*)path_addpath((Pathcomp_t*)shp->pathlist,val,PATH_PATH);
		if(shp->pathlist = (void*)pp)
			pp->shp = shp;
	}
	else
		shp->pathlist = (void*)path_dup(pp);
	if(val=(FPATHNOD)->nvalue.cp)
	{
		pp = (void*)path_addpath((Pathcomp_t*)shp->pathlist,val,PATH_FPATH);
		if(shp->pathlist = (void*)pp)
			pp->shp = shp;
	}
}

/*
 * returns that pathlist to search
 */
Pathcomp_t *path_get(register const char *name)
{
	register Shell_t *shp = &sh;
	register Pathcomp_t *pp;
	if(!shp->defpathlist)
		path_init(shp);
	if(*name && strchr(name,'/'))
		return(0);
	if(!sh_isstate(SH_DEFPATH))
		pp = (Pathcomp_t*)shp->pathlist;
	if(!pp && !(PATHNOD)->nvalue.cp)
		pp = (Pathcomp_t*)shp->defpathlist;
	return(pp);
}

/*
 * open file corresponding to name using path give by <pp>
 */
static int	path_opentype(const char *name, register Pathcomp_t *pp, int fun)
{
	register int fd= -1;
	struct stat statb;
	Pathcomp_t *oldpp;
	Shell_t *shp;
	if(pp)
		shp = pp->shp;
	else
	{
		shp = sh_getinterp();
		if(!shp->defpathlist)
			path_init(shp);
	}
	if(!fun && strchr(name,'/'))
	{
		if(sh_isoption(SH_RESTRICTED))
			errormsg(SH_DICT,ERROR_exit(1),e_restricted,name);
	}
	do
	{
		oldpp=pp;
		pp = path_nextcomp(pp,name,0);
		if(fun && !(oldpp->flags&PATH_FPATH))
			continue;
		if((fd = sh_open(path_relative(stakptr(PATH_OFFSET)),O_RDONLY,0)) >= 0)
		{
			if(fstat(fd,&statb)<0 || S_ISDIR(statb.st_mode))
			{
				errno = EISDIR;
				sh_close(fd);
				fd = -1;
			}
		}
	}
	while( fd<0 && pp);
	if(fd>=0 && (fd = sh_iomovefd(fd)) > 0)
	{
		fcntl(fd,F_SETFD,FD_CLOEXEC);
		shp->fdstatus[fd] |= IOCLEX;
	}
	return(fd);
}

/*
 * open file corresponding to name using path give by <pp>
 */
int	path_open(const char *name, register Pathcomp_t *pp)
{
	return(path_opentype(name,pp,0));
}

/*
 * given a pathname return the base name
 */

char	*path_basename(register const char *name)
{
	register const char *start = name;
	while (*name)
		if ((*name++ == '/') && *name)	/* don't trim trailing / */
			start = name;
	return ((char*)start);
}

/*
 * load functions from file <fno>
 */
static void funload(Shell_t *shp,int fno, const char *name)
{
	char buff[IOBSIZE+1];
	int savestates = sh_isstate(~0);
	sh_onstate(SH_NOLOG|SH_NOALIAS);
	shp->readscript = (char*)name;
	sh_eval(sfnew(NIL(Sfio_t*),buff,IOBSIZE,fno,SF_READ),0);
	shp->readscript = 0;
	sh_setstate(savestates);
}

/*
 * do a path search and track alias if requested
 * if flag is 0, or if name not found, then try autoloading function
 * if flag==2, returns 1 if name found on FPATH
 * returns 1, if function was autoloaded.
 * If endpath!=NULL, Path search ends when path matches endpath.
 */

int	path_search(register const char *name,Pathcomp_t *endpath, int flag)
{
	register Namval_t *np;
	register int fno;
	Pathcomp_t *pp=0;
	Shell_t *shp = &sh;
	if(!shp->defpathlist)
		path_init(shp);
	if(flag)
	{
		if(!(pp=path_absolute(name,endpath)) && endpath)
			pp = path_absolute(name,NIL(Pathcomp_t*));
		if(!pp && (np=nv_search(name,sh.fun_tree,HASH_NOSCOPE))&&np->nvalue.ip)
			return(1);
		if(!pp)
			*stakptr(PATH_OFFSET) = 0;
	}
	if(flag==0 || !pp || (pp->flags&PATH_FPATH))
	{
		if(!pp)
			pp=shp->pathlist;
		if(pp && strmatch(name,e_alphanum)  && (fno=path_opentype(name,pp,1))>=0)
		{
			if(flag==2)
			{
				sh_close(fno);
				return(1);
			}
			funload(shp,fno,name);
			if((np=nv_search(name,sh.fun_tree,HASH_NOSCOPE))&&np->nvalue.ip)
				return(1);
		}
		*stakptr(PATH_OFFSET) = 0;
		return(0);
	}
	else if(pp && !sh_isstate(SH_DEFPATH) && *name!='/')
	{
		if(np=nv_search(name,shp->track_tree,NV_ADD))
			path_alias(np,pp);
	}
	return(0);
}


/*
 * do a path search and find the full pathname of file name
 * end search of path matches endpath without checking execute permission
 */

Pathcomp_t *path_absolute(register const char *name, Pathcomp_t *endpath)
{
	register int	f,isfun;
	int		noexec=0;
	Pathcomp_t	*pp,*oldpp;
	Shell_t *shp = &sh;
	shp->path_err = ENOENT;
	if(!(pp=path_get("")))
		return(0);
	shp->path_err = 0;
	while(1)
	{
		sh_sigcheck();
		isfun = (pp->flags&PATH_FPATH);
		if(oldpp=pp)
			pp = path_nextcomp(pp,name,0);
		if(endpath)
			return(endpath);
		f = canexecute(stakptr(PATH_OFFSET),isfun);
		if(isfun && f>=0)
		{
			funload(shp,f,stakptr(PATH_OFFSET));
			f = -1;
			pp = 0;
		}
		if(!pp || f>=0)
			break;
		if(errno!=ENOENT)
			noexec = errno;
	}
	if(f<0)
	{
		if(!endpath)
			shp->path_err = (noexec?noexec:ENOENT);
		return(0);
	}
	stakputc(0);
	return(oldpp);
}

/*
 * returns 0 if path can execute
 * sets exec_err if file is found but can't be executable
 */
#undef S_IXALL
#ifdef S_IXUSR
#   define S_IXALL	(S_IXUSR|S_IXGRP|S_IXOTH)
#else
#   ifdef S_IEXEC
#	define S_IXALL	(S_IEXEC|(S_IEXEC>>3)|(S_IEXEC>>6))
#   else
#	define S_IXALL	0111
#   endif /*S_EXEC */
#endif /* S_IXUSR */

static int canexecute(register char *path, int isfun)
{
	struct stat statb;
	register int fd=0;
	path = path_relative(path);
	if(isfun)
	{
		if((fd=open(path,O_RDONLY,0))<0 || fstat(fd,&statb)<0)
			goto err;
	}
	else if(stat(path,&statb) < 0)
	{
#ifdef _UWIN
		/* check for .exe or .bat suffix */
		char *cp;
		if(errno==ENOENT && (!(cp=strrchr(path,'.')) || strlen(cp)>4 || strchr(cp,'/')))
		{
			int offset = staktell()-1;
			stakseek(offset);
			stakputs(".bat");
			path = stakptr(PATH_OFFSET);
			if(stat(path,&statb) < 0)
			{
				if(errno!=ENOENT)
					goto err;
				memcpy(stakptr(offset),".sh",4);
				if(stat(path,&statb) < 0)
					goto err;
			}
		}
		else
#endif /* _UWIN */
		goto err;
	}
	errno = EPERM;
	if(S_ISDIR(statb.st_mode))
		errno = EISDIR;
	else if((statb.st_mode&S_IXALL)==S_IXALL || sh_access(path,X_OK)>=0)
		return(fd);
	if(isfun && fd>=0)
		sh_close(fd);
err:
	return(-1);
}

/*
 * Return path relative to present working directory
 */

char *path_relative(register const char* file)
{
	register const char *pwd;
	register const char *fp = file;
	/* can't relpath when sh.pwd not set */
	if(!(pwd=sh.pwd))
		return((char*)fp);
	while(*pwd==*fp)
	{
		if(*pwd++==0)
			return((char*)e_dot);
		fp++;
	}
	if(*pwd==0 && *fp == '/')
	{
		while(*++fp=='/');
		if(*fp)
			return((char*)fp);
		return((char*)e_dot);
	}
	return((char*)file);
}

void	path_exec(register const char *arg0,register char *argv[],struct argnod *local)
{
	struct edata data;
	char **envp;
	Pathcomp_t *libpath, *pp=0;
	Shell_t *shp = &sh;
	int slash=0;
	nv_setlist(local,NV_EXPORT|NV_IDENT|NV_ASSIGN);
	data.sh = shp;
	envp = sh_envgen();
	if(strchr(arg0,'/'))
	{
		slash=1;
		/* name containing / not allowed for restricted shell */
		if(sh_isoption(SH_RESTRICTED))
			errormsg(SH_DICT,ERROR_exit(1),e_restricted,arg0);
	}
	else
		pp=path_get(arg0);
	/* leave room for inserting _= pathname in environment */
	envp--;
	data.exec_err = ENOENT;
	sfsync(NIL(Sfio_t*));
	timerdel(NIL(void*));
	/* find first path that has a library component */
	for(libpath=pp; libpath && !libpath->lib ; libpath=libpath->next);
	if(pp || slash) do
	{
		data.libpath = libpath;
		data.envp = envp;
	}
	while(pp=execs(pp,arg0,argv,&data));
	/* force an exit */
	((struct checkpt*)shp->jmplist)->mode = SH_JMPEXIT;
	if((errno = data.exec_err)==ENOENT)
		errormsg(SH_DICT,ERROR_exit(ERROR_NOENT),e_found,arg0);
	else
		errormsg(SH_DICT,ERROR_system(ERROR_NOEXEC),e_exec,arg0);
}

static Pathcomp_t *execs(Pathcomp_t *pp,const char *arg0,register char **argv, struct edata *dp)
{
	Shell_t *shp = dp->sh;
	register char *path, *opath=0;
	char **xp=0, *xval, *libenv= (pp?pp->lib:0); 
	Namval_t*	vp;
	char		*s, *v;
	int		r, n;
	sh_sigcheck();
	pp = path_nextcomp(pp,arg0,0);
#if _lib_readlink
	/* save original pathname */
	opath = stakfreeze(1)+PATH_OFFSET;
	vp=nv_search(arg0,shp->track_tree,0);
	if(!vp || nv_size(vp)>0)
	{
		/* check for symlink and use symlink name */
		char buff[PATH_MAX+1];
		char save[PATH_MAX+1];
		stakseek(PATH_OFFSET);
		stakputs(opath);
		path = stakptr(PATH_OFFSET);
		while((n=readlink(path,buff,PATH_MAX))>0)
		{
			buff[n] = 0;
			n = PATH_OFFSET;
			if((v=strrchr(path,'/')) && *buff!='/')
			{
				if(buff[0]=='.' && buff[1]=='.' && (r = strlen(path) + 1) <= PATH_MAX)
					memcpy(save, path, r);
				else
					r = 0;
				n += (v+1-path);
			}
			stakseek(n);
			stakputs(buff);
			stakputc(0);
			path = stakptr(PATH_OFFSET);
			if(buff[0]=='.' && buff[1]=='.')
			{
				pathcanon(path, 0);
				if(r && access(path,X_OK))
				{
					memcpy(path, save, r);
					break;
				}
			}
			if(libenv = path_lib(dp->libpath,path))
				break;
		}
		stakseek(0);
	}
#endif
	if(libenv)
	{
		v = strchr(libenv,'=');
		n = v - libenv;
		*v = 0;
		vp = nv_open(libenv,shp->var_tree,0);
		*v = '=';
		s = nv_getval(vp);
		stakputs(libenv);
		if(s)
		{
			stakputc(':');
			stakputs(s);
		}
		v = stakfreeze(1);
		r = 1;
		xp = dp->envp + 2;
		while (s = *xp++)
		{
			if (strneq(s, v, n) && s[n] == '=')
			{
				xval = *--xp;
				*xp = v;
				r = 0;
				break;
			}
		}
		if (r)
		{
			*dp->envp-- = v;
			xp = 0;
		}
	}
	if(!opath)
		opath = stakptr(PATH_OFFSET);
	dp->envp[0] =  opath-PATH_OFFSET;
	dp->envp[0][0] =  '_';
	dp->envp[0][1] =  '=';
	sfsync(sfstderr);
	sh_sigcheck();
	path = path_relative(opath);
#ifdef SHELLMAGIC
	if(*path!='/' && path!=opath)
	{
		/*
		 * The following code because execv(foo,) and execv(./foo,)
		 * may not yield the same results
		 */
		char *sp = (char*)malloc(strlen(path)+3);
		sp[0] = '.';
		sp[1] = '/';
		strcpy(sp+2,path);
		path = sp;
	}
#endif /* SHELLMAGIC */
	execve(opath, &argv[0] ,dp->envp);
	if(xp)
		*xp = xval;
#ifdef SHELLMAGIC
	if(*path=='.' && path!=opath)
	{
		free(path);
		path = path_relative(opath);
	}
#endif /* SHELLMAGIC */
	switch(errno)
	{
#ifdef apollo
	    /* 
  	     * On apollo's execve will fail with eacces when
	     * file has execute but not read permissions. So,
	     * for now we will pretend that EACCES and ENOEXEC
 	     * mean the same thing.
 	     */
	    case EACCES:
#endif /* apollo */
	    case ENOEXEC:
#ifdef SHOPT_SUID_EXEC
	    case EPERM:
		/* some systems return EPERM if setuid bit is on */
#endif
		exscript(shp,path,argv,dp->envp);
#ifndef apollo
	    case EACCES:
	    {
		struct stat statb;
		if(stat(path,&statb)>=0)
		{
			if(S_ISDIR(statb.st_mode))
				errno = EISDIR;
#ifdef S_ISSOCK
			if(S_ISSOCK(statb.st_mode))
				exscript(shp,path,argv,dp->envp);
#endif
		}
	    }
		/* FALL THROUGH */
#endif /* !apollo */
#ifdef ENAMETOOLONG
	    case ENAMETOOLONG:
#endif /* ENAMETOOLONG */
#ifndef SHOPT_SUID_EXEC
	    case EPERM:
#endif
		dp->exec_err = errno;
	    case ENOTDIR:
	    case ENOENT:
	    case EINTR:
#ifdef EMLINK
	    case EMLINK:
#endif /* EMLINK */
		while(pp && (pp->flags&PATH_FPATH))
			pp = path_nextcomp(pp,arg0,0);
		return(pp);
	    default:
		errormsg(SH_DICT,ERROR_system(ERROR_NOEXEC),e_exec,path);
	}
	return 0;
}

/*
 * File is executable but not machine code.
 * Assume file is a Shell script and execute it.
 */

static void exscript(Shell_t *shp,register char *path,register char *argv[],char **envp)
{
	register Sfio_t *sp;
	path = path_relative(path);
	shp->comdiv=0;
	shp->bckpid = 0;
	shp->st.ioset=0;
	/* clean up any cooperating processes */
	if(shp->cpipe[0]>0)
		sh_pclose(shp->cpipe);
	if(shp->cpid)
		sh_close(*shp->outpipe);
	shp->cpid = 0;
	if(sp=fcfile())
		while(sfstack(sp,SF_POPSTACK));
	job_clear();
	if(shp->infd>0)
		sh_close(shp->infd);
	sh_setstate(SH_FORKED);
	sfsync(sfstderr);
#ifdef SHOPT_SUID_EXEC
	/* check if file cannot open for read or script is setuid/setgid  */
	{
		static char name[] = "/tmp/euidXXXXXXXXXX";
		register int n;
		register uid_t euserid;
		char *savet;
		struct stat statb;
		if((n=sh_open(path,O_RDONLY,0)) >= 0)
		{
			/* move <n> if n=0,1,2 */
			n = sh_iomovefd(n);
			if(fstat(n,&statb)>=0 && !(statb.st_mode&(S_ISUID|S_ISGID)))
				goto openok;
			sh_close(n);
		}
		if((euserid=geteuid()) != shp->userid)
		{
			strncpy(name+9,fmtbase((long)getpid(),10,0),sizeof(name)-10);
			/* create a suid open file with owner equal effective uid */
			if((n=open(name,O_CREAT|O_TRUNC|O_WRONLY,S_ISUID|S_IXUSR)) < 0)
				goto fail;
			unlink(name);
			/* make sure that file has right owner */
			if(fstat(n,&statb)<0 || statb.st_uid != euserid)
				goto fail;
			if(n!=10)
			{
				sh_close(10);
				fcntl(n, F_DUPFD, 10);
				sh_close(n);
				n=10;
			}
		}
		savet = *--argv;
		*argv = path;
		execve(e_suidexec,argv,envp);
	fail:
		/*
		 *  The following code is just for compatibility
		 */
		if((n=open(path,O_RDONLY,0)) < 0)
			errormsg(SH_DICT,ERROR_system(1),e_open,path);
		*argv++ = savet;
	openok:
		shp->infd = n;
	}
#else
	shp->infd = sh_chkopen(path);
#endif /* SHOPT_SUID_EXEC */
#ifdef SHOPT_ACCT
	sh_accbegin(path) ;  /* reset accounting */
#endif	/* SHOPT_ACCT */
	shp->arglist = sh_argcreate(argv);
	shp->lastarg = strdup(path);
	/* save name of calling command */
	shp->readscript = error_info.id;
	/* close history file if name has changed */
	if(shp->hist_ptr && (path=nv_getval(HISTFILE)) && strcmp(path,shp->hist_ptr->histname))
	{
		hist_close(shp->hist_ptr);
		(HISTCUR)->nvalue.lp = 0;
	}
	sh_offstate(SH_FORKED);
	siglongjmp(*shp->jmplist,SH_JMPSCRIPT);
}

#ifdef SHOPT_ACCT
#   include <sys/acct.h>
#   include "FEATURE/time"

    static struct acct sabuf;
    static struct tms buffer;
    static clock_t	before;
    static char *SHACCT; /* set to value of SHACCT environment variable */
    static shaccton;	/* non-zero causes accounting record to be written */
    static int compress(time_t);
    /*
     *	initialize accounting, i.e., see if SHACCT variable set
     */
    void sh_accinit(void)
    {
	SHACCT = getenv("SHACCT");
    }
    /*
    * suspend accounting unitl turned on by sh_accbegin()
    */
    void sh_accsusp(void)
    {
	shaccton=0;
#ifdef AEXPAND
	sabuf.ac_flag |= AEXPND;
#endif /* AEXPAND */
    }

    /*
     * begin an accounting record by recording start time
     */
    void sh_accbegin(const char *cmdname)
    {
	if(SHACCT)
	{
		sabuf.ac_btime = time(NIL(time_t *));
		before = times(&buffer);
		sabuf.ac_uid = getuid();
		sabuf.ac_gid = getgid();
		strncpy(sabuf.ac_comm, (char*)path_basename(cmdname),
			sizeof(sabuf.ac_comm));
		shaccton = 1;
	}
    }
    /*
     * terminate an accounting record and append to accounting file
     */
    void	sh_accend(void)
    {
	int	fd;
	clock_t	after;

	if(shaccton)
	{
		after = times(&buffer);
		sabuf.ac_utime = compress(buffer.tms_utime + buffer.tms_cutime);
		sabuf.ac_stime = compress(buffer.tms_stime + buffer.tms_cstime);
		sabuf.ac_etime = compress( (time_t)(after-before));
		fd = open( SHACCT , O_WRONLY | O_APPEND | O_CREAT,RW_ALL);
		write(fd, (const char*)&sabuf, sizeof( sabuf ));
		close( fd);
	}
    }
 
    /*
     * Produce a pseudo-floating point representation
     * with 3 bits base-8 exponent, 13 bits fraction.
     */
    static int compress(register time_t t)
    {
	register int exp = 0, rund = 0;

	while (t >= 8192)
	{
		exp++;
		rund = t&04;
		t >>= 3;
	}
	if (rund)
	{
		t++;
		if (t >= 8192)
		{
			t >>= 3;
			exp++;
		}
	}
	return((exp<<13) + t);
    }
#endif	/* SHOPT_ACCT */



/*
 * add a pathcomponent to the path search list and eliminate duplicates
 * and non-existing absolute paths.
 */
static Pathcomp_t *path_addcomp(Pathcomp_t *first, Pathcomp_t *old,const char *name, int flag)
{
	register Pathcomp_t *pp, *oldpp;
	struct stat statb;
	int len, offset=staktell();
	if(!(flag&PATH_BFPATH))
	{
		register const char *cp = name;
		while(*cp && *cp!=':')
			stakputc(*cp++);
		len = staktell()-offset;
		stakputc(0);
	}
	else
		len = strlen(name);
	stakseek(offset);
	for(pp=first; pp; pp=pp->next)
	{
		if(memcmp(name,pp->name,len)==0 && (pp->name[len]==':' || pp->name[len]==0))
		{
			pp->flags |= flag;
			return(first);
		}
	}
	if(old && (old=path_dirfind(old,stakptr(offset),0)))
	{
		statb.st_ino = old->ino;
		statb.st_dev = old->dev;
		if(old->ino==0 && old->dev==0)
			flag |= PATH_SKIP;
	}
	else if(stat(stakptr(offset),&statb)<0 || !S_ISDIR(statb.st_mode))
	{
		if(*name=='/')
			return(first);
		flag |= PATH_SKIP;
		statb.st_ino = 0;
		statb.st_dev = 0;
	}
	for(pp=first, oldpp=0; pp; oldpp=pp, pp=pp->next)
	{
		if(pp->ino==statb.st_ino && pp->dev==statb.st_dev)
		{
			/* if both absolute paths, eliminate second */
			pp->flags |= flag;
			if(*name=='/' && *pp->name=='/')
				return(first);
			/* keep the path but mark it as skip */
			flag |= PATH_SKIP;
		}
	}
	pp = newof((Pathcomp_t*)0,Pathcomp_t,1,len+1);
	pp->refcount = 1;
	memcpy((char*)(pp+1),stakptr(offset),len+1);
	pp->name = (char*)(pp+1);
	pp->len = len;
	pp->dev = statb.st_dev;
	pp->ino = statb.st_ino;
	if(oldpp)
		oldpp->next = pp;
	else
		first = pp;
	pp->flags = flag;
	if(flag!=PATH_PATH)
		return(first);
	path_chkfpath(first,old,pp,offset);
	return(first);
}

/*
 * This function checks for the .fpath file in directory in <pp>
 * it assumes that the directory is on the stack at <offset> 
 */
static int path_chkfpath(Pathcomp_t *first, Pathcomp_t* old,Pathcomp_t *pp, int offset)
{
	struct stat statb;
	int k,m,n,fd;
	char *sp,*cp,*ep;
	stakseek(offset+pp->len);
	stakputs("/.paths");
	if((fd=open(stakptr(offset),O_RDONLY))>=0)
	{
		fstat(fd,&statb);
		n = statb.st_size;
		stakseek(offset+pp->len+n+2);
		sp = stakptr(offset+pp->len);
		*sp++ = '/';
		n=read(fd,cp=sp,n);
		sp[n] = 0;
		close(fd);
		for(ep=0; n--; cp++)
		{
			if(*cp=='=')
			{
				ep = cp+1;
				continue;
			}
			else if(*cp!='\r' &&  *cp!='\n')
				continue;
			if(*sp=='#' || sp==cp)
			{
				sp = cp+1;
				continue;
			}
			*cp = 0;
			if(!ep || memcmp((void*)sp,(void*)"FPATH=",6)==0)
			{
				if(first)
				{
					char *ptr = stakptr(offset+pp->len+1);
					stakseek(offset);
					if(ep)
						strcpy(ptr,ep);
					path_addcomp(first,old,stakptr(offset),PATH_FPATH|PATH_BFPATH);
				}
			}
			else if(ep)
			{
				m = ep-sp;
				pp->lib = (char*)malloc(cp-sp+pp->len+2);
				memcpy((void*)pp->lib,(void*)sp,m);
				memcpy((void*)&pp->lib[m],stakptr(offset),pp->len);
				pp->lib[k=m+pp->len] = '/';
				strcpy((void*)&pp->lib[k+1],ep);
				pathcanon(&pp->lib[m],0);
				if(!first)
				{
					stakseek(0);
					stakputs(pp->lib);
					free((void*)pp->lib);
					return(1);
				}
			}
			sp = cp+1;
			ep = 0;
		}
	}
	return(0);
}


Pathcomp_t *path_addpath(Pathcomp_t *first, register const char *path,int type)
{
	register const char *cp;
	Pathcomp_t *old=0;
	if(!path && type!=PATH_PATH)
		return(first);
	if(type!=PATH_FPATH)
	{
		old = first;
		first = 0;
	}
	if(path) while(*(cp=path))
	{
		if(*cp==':')
		{
			if(type!=PATH_FPATH)
				first = path_addcomp(first,old,".",type);
			while(*++path == ':');
		}
		else
		{
			int c;
			while(*path && *path!=':')
				path++;
			c = *path++;
			first = path_addcomp(first,old,cp,type);
			if(c==0)
				break;
			if(*path==0)
				path--;
		}
	}
	if(old)
	{
		if(!first && !path)
			first = path_dup((Pathcomp_t*)(old->shp->defpathlist));
		if(cp=(FPATHNOD)->nvalue.cp)
			first = (void*)path_addpath((Pathcomp_t*)first,cp,PATH_FPATH);
		path_delete(old);
	}
	return(first);
}

/*
 * duplicate the path give by <first> by incremented reference counts
 */
Pathcomp_t *path_dup(Pathcomp_t *first)
{
	register Pathcomp_t *pp=first;
	while(pp)
	{
		pp->refcount++;
		pp = pp->next;
	}
	return(first);
}

/*
 * called whenever the directory is changed
 */
void path_newdir(Pathcomp_t *first)
{
	register Pathcomp_t *pp=first, *next, *pq;
	struct stat statb;
	for(pp=first; pp; pp=pp->next)
	{
		pp->flags &= ~PATH_SKIP;
		if(*pp->name=='/')
			continue;
		/* delete .fpath component */
		if((next=pp->next) && (next->flags&PATH_BFPATH))
		{
			pp->next = next->next;
			if(--next->refcount<=0)
				free((void*)next);
		}
		if(stat(pp->name,&statb)<0 || !S_ISDIR(statb.st_mode))
		{
			pp->dev = 0;
			pp->ino = 0;
			continue;
		}
		pp->dev = statb.st_dev;
		pp->ino = statb.st_ino;
		for(pq=first;pq!=pp;pq=pq->next)
		{
			if(pp->ino==pq->ino && pp->dev==pq->dev)
				pp->flags |= PATH_SKIP;
		}
		for(pq=pp;pq=pq->next;)
		{
			if(pp->ino==pq->ino && pp->dev==pq->dev)
				pq->flags |= PATH_SKIP;
		}
		if(pp->flags==PATH_PATH)
		{
			/* try to insert .fpath component */
			int offset = staktell();
			stakputs(pp->name);
			stakseek(offset);
			next = pp->next;
			pp->next = 0;
			path_chkfpath(first,(Pathcomp_t*)0,pp,offset);
			if(pp->next)
				pp = pp->next;
			pp->next = next;
		}
	}
#if 0
	path_dump(first);
#endif
}

Pathcomp_t *path_unsetfpath(Pathcomp_t *first)
{
	register Pathcomp_t *pp=first, *old=0;
	while(pp)
	{
		if((pp->flags&PATH_FPATH) && !(pp->flags&PATH_BFPATH))
		{
			if(pp->flags&PATH_PATH)
				pp->flags &= ~PATH_FPATH;
			else
			{
				Pathcomp_t *ppsave=pp;
				if(old)
					old->next = pp->next;
				else
					first = pp->next;
				pp = pp->next;
				if(--ppsave->refcount<=0)
				{
					if(ppsave->lib)
						free((void*)ppsave->lib);
					free((void*)ppsave);
				}
				continue;
			}
			
		}
		old = pp;
		pp = pp->next;
	}
	return(first);
}

Pathcomp_t *path_dirfind(Pathcomp_t *first,const char *name,int c)
{
	register Pathcomp_t *pp=first;
	while(pp)
	{
		if(memcmp(name,pp->name,pp->len)==0 && name[pp->len]==c) 
			return(pp);
		pp = pp->next;
	}
	return(0);
}

/*
 * get discipline for tracked alias
 */
static char *talias_get(Namval_t *np, Namfun_t *nvp)
{
	Pathcomp_t *pp = (Pathcomp_t*)np->nvalue.cp;
	char *ptr;
	if(!pp)
		return(NULL);
	path_nextcomp(pp,nv_name(np),pp);
	ptr = stakfreeze(0);
	return(ptr+PATH_OFFSET);
}

static void talias_put(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	if(!val && np->nvalue.cp)
	{
		Pathcomp_t *pp = (Pathcomp_t*)np->nvalue.cp;
		if(--pp->refcount<=0)
			free((void*)pp);
	}
	nv_putv(np,val,flags,fp);
}

static const Namdisc_t talias_disc   = { 0, talias_put, talias_get   };
static Namfun_t  talias_init = { &talias_disc };

/*
 *  set tracked alias node <np> to value <pp>
 */
void path_alias(register Namval_t *np,register Pathcomp_t *pp)
{
	if(pp)
	{
		struct stat statb;
		char *sp;
		nv_offattr(np,NV_NOPRINT);
		nv_stack(np,&talias_init);
		np->nvalue.cp = (char*)pp;
		pp->refcount++;
		nv_setattr(np,NV_TAGGED|NV_NOFREE);
		path_nextcomp(pp,nv_name(np),pp);
		sp = stakptr(PATH_OFFSET);
		if(sp && lstat(sp,&statb)>=0 && S_ISLNK(statb.st_mode))
			nv_setsize(np,statb.st_size+1);
		else
			nv_setsize(np,0);
	}
	else
		nv_unset(np);
}

