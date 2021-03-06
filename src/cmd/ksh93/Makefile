/*
 * ksh makefile
 * This makefile should work with nmake 2.3 and higher 
 */

if CC.HOSTTYPE == "*win*" && CC.SUFFIX.DYNAMIC == ".dll" || "$(PACKAGE_OPTIMIZE:N=space)"
	CCFLAGS = $(CC.OPTIMIZE) $(CC.DLL)
	LIBS_req = -lshell
else
	LIBS_req = +lshell
	if ! VARIANT
		libtype = :static
	end
end

:PACKAGE:	ast$(libtype)

LICENSE = since=1982,author=dgk

CATALOG = libshell

.SOURCE : edit sh bltins data tests fun
.SOURCE.h : include 

SH = ksh		/* name of the shell program */
VERSION = 1.1

clobber = !($(SH))	/* move ksh to ksh.old on clobber */

/* documentation files */
DOCFILES = sh.memo builtins.mm README RELEASE PROMO.mm COMPATIBILITY \
		DESIGN OBSOLETE RELEASE93 RELEASE88

KSHELL == 1		/* this is ksh */

SH_DICT ==

/* set these variables to 1 to enable, null to disable */
DEBUG ==			/* debugging purposes only */
SHOPT_APPEND == 1		/* var+=val string append */
SHOPT_NAMESPACE == 1		/* allow namespaces */
SHOPT_COMPOUND_ARRAY == 1	/* allow compound arrays */
SHOPT_ESH == 1			/* emacs/gmacs edit mode */
SHOPT_VSH == 1			/* vi edit mode */
SHOPT_SPAWN == 			/* use spawnve for fork/exec */
SHOPT_SUID_EXEC == 1		/* allow (safe) suid/sgid shell scripts */
SHOPT_FILESCAN==1		/* fast file scan */
SHOPT_OPTIMIZE==1		/* optimize loop invariants */
SHOPT_MULTIBYTE==1		/* multibyte character handling */

if  CC.HOSTTYPE == "*win*"
	SHOPT_CRNL == 1		/* <cr><nl> is eqivalent to <nl> */
	SHOPT_NOCASE == 1	/* do case insensitive file compare */
end

/* use the following libraries only if they exist */
LIBS_opt := +lm +ljobs +li
if ! "$(-mam:N=static*)" && CC.HOSTTYPE != "sgi.mips*"
	LIBS_opt += +lsocket +lnsl
end
if CC.HOSTTYPE == "linux.sparc"
	/* the linux.sparc linker still has some problems */
	LIBS_opt += -ldll
end
LIBS_opt += +lintl -ldl
$(LIBS_opt)	: .DONTCARE

LDLIBRARIES = $(LIBS_opt)

FILES_opt =		/* optional file list enabled by vars above */
FILES_opt += $(SHOPT_ESH:T=V?emacs.c)
FILES_opt += $(SHOPT_VSH:T=V?vi.c)
if SHOPT_X
LDFLAGS += -u  _XtAppInitialize -L/usr/add-on/X11/lib
end


:ALL: $(SH) shcomp $(SHOPT_SUID_EXEC:T=V?suid_exec.c)

$(SH) :: sh.1 pmain.c $(LIBS_req)

DATAFILES = limits.c msg.c strdata.c testops.c keywords.c options.c \
                 signals.c aliases.c builtins.c variables.c lexstates.c

shell $(VERSION) :LIBRARY: shell.3 nval.3 alarm.c cd_pwd.c cflow.c deparse.c \
	getopts.c hist.c misc.c print.c read.c sleep.c trap.c test.c \
	typeset.c ulimit.c umask.c whence.c main.c \
	arith.c args.c array.c completion.c defs.c edit.c expand.c \
	fault.c fcin.c history.c init.c io.c jobs.c lex.c macro.c name.c \
	parse.c path.c string.c streval.c subshell.c tdump.c timers.c \
	trestore.c waitevent.c xec.c env.c $(DATAFILES) $(FILES_opt) \
	-lcmd -ldll -last -lm

if CC.HOSTTYPE == "win32.*" || "$(-mam)" == "static*"
trestore.o : .CC.NOOPTIMIZE
end

:DLL: $(CC.DEBUG)

:READONLY: $(DATAFILES)

$(INCLUDEDIR) :INSTALLPROTO: nval.h shell.h history.h

suid_exec ::	CC.DLL= suid_exec.c +last

shcomp	::	CC.DLL= SH_DICT='"libshell"' shcomp.c -lshell

$(FUNDIR) :INSTALLDIR: mode=+x dirs popd pushd

shcomp.o : _BLD_shell=

:: syscall.s mamexec mamstate.c deparse.c shopen.c OPTIONS $(DOCFILES)

:: alias.sh append.sh arith.sh arrays.sh attributes.sh basic.sh bracket.sh \
	builtins.sh case.sh comvar.sh coprocess.sh exit.sh functions.sh \
	grep.sh heredoc.sh io.sh nameref.sh options.sh path.sh quoting.sh \
	quoting2.sh return.sh select.sh substring.sh tilde.sh variables.sh \
	shtests

if VARIANT

test : .NULL

else

test : $(SH) shtests
	KSH=$(*:O=1:P=A)
	cd $(*:O=2:P=L!:D)
	SHELL=$KSH $KSH $(*:O=2:B)

end

OPTIONS : .TERMINAL /* avoid case-insensitive options.c => OPTIONS map */
