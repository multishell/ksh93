/*******************************************************************
*                                                                  *
*             This software is part of the ast package             *
*                Copyright (c) 1997-2002 AT&T Corp.                *
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
*                                                                  *
*******************************************************************/
#pragma prototyped
/*
 * provide dlopen/dlsym/dlerror interface
 *
 * David Korn
 * Glenn Fowler
 * AT&T Labs Research
 */

static const char id[] = "\n@(#)$Id: dll library (AT&T Labs Research) 2002-01-11 $\0\n";

#include <ast.h>
#include <dlldefs.h>
#include <error.h>

#define T(x)	ERROR_dictionary(x)

#if _BLD_dll && defined(__EXPORT__)
#define extern	__EXPORT__
#endif

#if _hdr_dlfcn && _lib_dlopen

	/*
	 * standard
	 */

#	include <dlfcn.h> 

#else
#if _hdr_dl

	/*
	 * HP-UX
 	 */

#	include <dl.h>
#	ifndef BIND_FIRST
#	define BIND_FIRST	0x4
#	endif
#	ifndef BIND_NOSTART
#	define BIND_NOSTART	0x10
#	endif

	static shl_t	all;
	static int	err;

	extern void* dlopen(const char* path, int mode)
	{
		void*	dll;

		if (!path)
			return (void*)&all;
		if (mode)
			mode = (BIND_IMMEDIATE|BIND_FIRST|BIND_NOSTART);
		if (!(dll = (void*)shl_load(path, mode, 0L)))
			err = errno;
		return dll;
	}

	extern int dlclose(void* dll)
	{
		return 0;
	}

	extern void* dlsym(void* dll, const char* name)
	{
		shl_t	handle;
		long	addr;

		handle = dll == (void*)&all ? (shl_t)0 : (shl_t)dll;
		if (shl_findsym(&handle, name, TYPE_UNDEFINED, &addr))
		{
			err = errno;
			return 0;
		}
		return (void*)addr;
	}

	extern char* dlerror(void)
	{
		char*	msg;

		if (!err)
			return 0;
		msg = fmterror(err);
		err = 0;
		return msg;
	}

#else
#if _sys_ldr && _lib_loadbind

	/*
	 * rs6000
	 */

#	include <sys/ldr.h>
#	include <xcoff.h>

	/* xcoff module header */
	struct hdr
	{
		struct filehdr	f;
		struct aouthdr	a;
		struct scnhdr	s[1];
	};

	static struct ld_info*	ld_info;
	static unsigned int	ld_info_size = 1024;
	static void*		last_module;
	static int		err;

	extern void* dlopen(const char* path, int mode)
	{
		void*	dll;

		if (!(dll = (void*)load((char*)path, mode, getenv("LIBPATH"))))
			err = errno;
		return dll;
	}

	extern int dlclose(void* dll)
	{
		return 0;
	}

	static int getquery(void)
	{
		if (!ld_info)
			ld_info = malloc(ld_info_size);
		for (;;)
		{
			if (!ld_info)
				return 1;
			if (!loadquery(L_GETINFO, ld_info, ld_info_size))
				return 0;
			if (errno != ENOMEM)
				return 1;
			ld_info = realloc(ld_info, ld_info_size *= 2);
		}
 	}

	/* find the loaded module whose data area contains the
	 * address passed in. Remember that procedure pointers
	 * are implemented as pointers to descriptors in the
	 * data area of the module defining the procedure
	 */
	static struct ld_info* getinfo(void* module)
	{
		struct ld_info*	info = ld_info;
		register int	n = 1;

		if (!ld_info || module != last_module)
		{
			last_module = module;
			if (getquery())
				return 0;
			info = ld_info;
		}
		while (n)
		{
			if ((char*)(info->ldinfo_dataorg) <= (char*)module &&
				(char*)module <= ((char*)(info->ldinfo_dataorg)
				+ (unsigned)(info->ldinfo_datasize)))
				return info;
			if (n=info->ldinfo_next)
				info = (void*)((char*)info + n);
		}
		return 0;
	}

	static char* getloc(struct hdr* hdr, char* data, char* name)
	{
		struct ldhdr*	ldhdr;
		struct ldsym*	ldsym;
		ulong		datareloc;
		ulong		textreloc;
		int		i;

		/* data is relocated by the difference between
		 * its virtual origin and where it was
		 * actually placed
		 */
		/*N.B. o_sndata etc. are one based */
		datareloc = (ulong)data - hdr->s[hdr->a.o_sndata-1].s_vaddr;
		/*hdr is address of header, not text, so add text s_scnptr */
		textreloc = (ulong)hdr + hdr->s[hdr->a.o_sntext-1].s_scnptr
			- hdr->s[hdr->a.o_sntext-1].s_vaddr;
		ldhdr = (void*)((char*)hdr+ hdr->s[hdr->a.o_snloader-1].s_scnptr);
		ldsym = (void*) (ldhdr+1);
		/* search the exports symbols */
		for(i=0; i < ldhdr->l_nsyms;ldsym++,i++)
		{
			char *symname,symbuf[9];
			char *loc;
			/* the symbol name representation is a nuisance since
			 * 8 character names appear in l_name but may
			 * not be null terminated. This code works around
			 * that by brute force
			 */
			if (ldsym->l_zeroes)
			{
				symname = symbuf;
				memcpy(symbuf,ldsym->l_name,8);
				symbuf[8] = 0;
			}
			else
				symname = (void*)(ldsym->l_offset + (ulong)ldhdr + ldhdr->l_stoff);
			if (strcmp(symname,name))
				continue;
			loc = (char*)ldsym->l_value;
			if ((ldsym->l_scnum==hdr->a.o_sndata) ||
				(ldsym->l_scnum==hdr->a.o_snbss))
				loc += datareloc;
			else if (ldsym->l_scnum==hdr->a.o_sntext)
				loc += textreloc;
			return loc;
		}
		return 0;
	}

	extern void* dlsym(void* handle, const char* name)
	{
		void*		addr;
		struct ld_info*	info;

		if (!(info = getinfo(handle)) || !(addr = getloc(info->ldinfo_textorg,info->ldinfo_dataorg,(char*)name)))
		{
			err = errno;
			return 0;
		}
		return addr;
	}

	extern char* dlerror(void)
	{
		char*	msg;

		if (!err)
			return 0;
		msg = fmterror(err);
		err = 0;
		return msg;
	}

#else
#if _hdr_dll && _lib_dllload

	/*
	 * MVS
	 */

#	include <dll.h>

	static int	err;

	extern void* dlopen(const char* path, int mode)
	{
		void*	dll;

		NoP(mode);
		if (!(dll = (void*)dllload(path)))
			err = errno;
		return dll;
	}

	extern int dlclose(void* dll)
	{
		return 0;
	}

	extern void* dlsym(void* handle, const char* name)
	{
		void*	addr;

		if (!(addr = (void*)dllqueryfn(handle, (char*)name)))
			err = errno;
		return addr;
	}

	extern char* dlerror(void)
	{
		char*	msg;

		if (!err)
			return 0;
		msg = fmterror(err);
		err = 0;
		return msg;
	}

#else
#if _hdr_mach_o_dyld

	/*
	 * mac[h]
	 */

#	include <mach-o/dyld.h>

	typedef struct Dll_s
	{
		unsigned long	magic;
		NSModule	module;
		char		path[1];
	} Dll_t;

	#define DL_MAGIC	0x04190c04
	#define DL_NEXT		((Dll_t*)RTLD_NEXT)

	static const char*	dlmessage = "no error";

	static const char	e_cover[] = T("cannot access covered library");
	static const char	e_handle[] = T("invalid handle");
	static const char	e_space[] = T("out of space");
	static const char	e_static[] = T("image statically linked");
	static const char	e_undefined[] = T("undefined symbol");

	static void undefined(const char* name)
	{
	}

	static NSModule multiple(NSSymbol sym, NSModule om, NSModule nm)
	{
		return om;
	}

	static void linkedit(NSLinkEditErrors c, int n, const char* f, const char* m)
	{
		dlmessage = m;
	}

	static NSLinkEditErrorHandlers handlers =
	{
		undefined, multiple, linkedit
	};

	extern void* dlopen(const char* path, int mode)
	{
		Dll_t*		dll;

		static int	init = 0;

		if (!_dyld_present())
		{
			dlmessage = e_static;
			return 0;
		}
		if (!init)
		{
			init = 1;
			NSInstallLinkEditErrorHandlers(&handlers);
		}
		if (!(dll = newof(0, Dll_t, 1, strlen(path))))
		{
			dlmessage = e_space;
			return 0;
		}
#if 0
{
	int			i;
	NSObjectFileImage	image;
	i = NSCreateObjectFileImageFromFile(path, &image);
	sfprintf(sfstderr, "NSCreateObjectFileImageFromFile %s => %d\n", path, i);
}
#endif
		if (!NSAddLibrary(path))
		{
			free(dll);
			dlmessage = e_space;
			return 0;
		}
		strcpy(dll->path, path);
		dll->magic = DL_MAGIC;
		return (void*)dll;
	}

	extern int dlclose(void* handle)
	{
		Dll_t*	dll = (Dll_t*)handle;

		if (!dll || dll == DL_NEXT || dll->magic != DL_MAGIC)
		{
			dlmessage = e_handle;
			return -1;
		}
		free(dll);
		return 0;
	}

	extern void* dlsym(void* handle, const char* name)
	{
		Dll_t*		dll = (Dll_t*)handle;
		unsigned long	address;
		void*		module;
		char		buf[1024];

		if (dll == DL_NEXT)
		{
			if (!_dyld_func_lookup(name, &address))
			{
				dlmessage = e_cover;
				return 0;
			}
		}
		else if (dll && dll->magic != DL_MAGIC)
		{
			dlmessage = e_handle;
			return 0;
		}
		else
		{
			if (!NSIsSymbolNameDefined(name))
			{
				if (name[0] == '_' || strlen(name) >= (sizeof(buf) - 1))
				{
					dlmessage = e_undefined;
					return 0;
				}
				buf[0] = '_';
				strcpy(buf + 1, name);
				name = (char*)buf;
				if (!NSIsSymbolNameDefined(name))
				{
					dlmessage = e_undefined;
					return 0;
				}
			}
			_dyld_lookup_and_bind(name, &address, &module);
		}
		return (void*)address;
	}

	extern char* dlerror(void)
	{
		char*	msg;

		msg = (char*)dlmessage;
		dlmessage = 0;
		return msg;
	}

#else
	/*
	 * punt
	 */

	static int	err;

	extern void* dlopen(const char* path, int mode)
	{
		err = 1;
		return 0;
	}

	extern int dlclose(void* dll)
	{
		err = 1;
		return 0;
	}

	extern void* dlsym(void* handle, const char* name)
	{
		err = 1;
		return 0;
	}

	extern char* dlerror(void)
	{
		if (!err)
			return 0;
		err = 0;
		return "dynamic linking not supported";
	}

#endif
#endif
#endif
#endif
#endif
