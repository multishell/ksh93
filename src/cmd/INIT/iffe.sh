####################################################################
#                                                                  #
#             This software is part of the ast package             #
#                Copyright (c) 1999-2002 AT&T Corp.                #
#        and it may only be used by you under license from         #
#                       AT&T Corp. ("AT&T")                        #
#         A copy of the Source Code Agreement is available         #
#                at the AT&T Internet web site URL                 #
#                                                                  #
#       http://www.research.att.com/sw/license/ast-open.html       #
#                                                                  #
#    If you have copied or used this software without agreeing     #
#        to the terms of the license you are infringing on         #
#           the license and copyright and are violating            #
#               AT&T's intellectual property rights.               #
#                                                                  #
#            Information and Software Systems Research             #
#                        AT&T Labs Research                        #
#                         Florham Park NJ                          #
#                                                                  #
#               Glenn Fowler <gsf@research.att.com>                #
#                                                                  #
####################################################################
# Glenn Fowler & Phong Vo
# AT&T Labs Research
#
# test if feature exists
# this script written to make it through all sh variants
#
# NOTE: .exe a.out suffix and [\\/] in path patterns for dos/nt

case $-:$BASH_VERSION in
*x*:[0123456789]*)	: bash set -x is broken :; set +ex ;;
esac

command=iffe
version=2002-02-14 # update in USAGE too #

pkg() # package
{
	case $1 in
	'<')	shift ;;
	*)	return ;;
	esac
	case $1 in
	X|X11*)	i="openwin"
		case $1 in
		X)	set X11 ;;
		esac
		case $1 in
		X11)	case $# in
			1)	set $1 6 5 4 ;;
			esac
			;;
		esac
		;;
	*)	i=
		;;
	esac
	pth="{ usr . - . contrib local $i - . share - . lib - $1"
	i=$1
	while	:
	do	shift
		case $# in
		0)	break ;;
		esac
		case $1 in
		'>')	shift; break ;;
		esac
		pth="$pth ${i}R$1 ${i}.$1"
	done
	pth="$pth . } $*"
}

is() # op name
{
	case $verbose in
	1)	case $complete in
		1)	failure ;;
		esac
		oo=$1
		shift
		case $1 in
		?*)	yy=is
			ii=$1
			complete=1
			case $oo in
			cmd)	mm="a command" ;;
			dat)	mm="a library data symbol" ;;
			dfn)	mm="a macro with extractable value" ;;
			exp)	mm="true" ;;
			hdr)	mm="a header" ;;
			lcl)	mm="a native header" ;;
			key)	mm="a reserved keyword" ;;
			lib)	mm="a library function" ;;
			LIB)	case $2 in
				"")	mm="a library" ;;
				*)	ii=$*; mm="a library group" ;;
				esac
				;;
			mac)	mm="a macro" ;;
			mem)	mm="a member of $2" ;;
			mth)	mm="a math library symbol" ;;
			nos)	mm="a non-opaque struct" ;;
			npt)	mm="a symbol that needs a prototype" ;;
			num)	mm="a numeric constant or enum" ;;
			nxt)	mm="an include path for the native header" ;;
			pth)	mm="a file" ;;
			run)	yy="capture output of" mm= ;;
			siz)	mm="a type with know size" ;;
			sym)	mm="a typed variable" ;;
			sys)	mm="a system header" ;;
			typ)	mm="a type or typedef" ;;
			val)	yy="determine" mm="value" ;;
			*)	yy= mm= ;;
			esac
			case $ii in
			[abcdefghijklmnopqrstuvwxyz]*[abcdefghijklmnopqrstuvwxyz]"{") ii="$ii ... }end" ;;
			esac
			$show "$command: test:" $yy $ii $mm "...$SHOW" >&$stderr
			complete=1
			;;
		esac
		;;
	esac
}

success()
{
	case $complete:$verbose in
	1:1)	case $suspended in
		1)	suspended=0
			$show "$command: test:" $yy $ii $mm "...$SHOW" >&$stderr
			;;
		esac
		complete=0
		case $# in
		0)	mm="yes" ;;
		*)	mm="'$*'" ;;
		esac
		case $debug in
		0)	echo " $mm" >&$stderr ;;
		*)	echo "$command: ... $mm" >&$stderr ;;
		esac
		;;
	esac
}

failure()
{
	case $complete:$verbose in
	1:1)	case $suspended in
		1)	suspended=0
			$show "$command: test:" $yy $ii $mm "...$SHOW" >&$stderr
			;;
		esac
		complete=0
		case $# in
		0)	mm="no" ;;
		*)	mm=$* ;;
		esac
		case $debug in
		0)	echo " $mm" >&$stderr ;;
		*)	echo "$command: ... $mm" >&$stderr ;;
		esac
		;;
	esac
}

noisy()
{
	case $complete:$verbose in
	1:1)	suspended=1
		echo >&$stderr
		;;
	esac
}

here_broken=0

copy() # output-file data
{
	case $shell in
	ksh)	case $1 in
		-)	print -r -- "$2" ;;
		*)	print -r -- "$2" > "$1" ;;
		esac
		;;
	*)	case $1 in
		-)	if	cat <<!
$2
!
			then	: old here doc botch not present
			else	case $here_broken in
				0)	here_broken=1
					echo "$command: your shell botches here documents; this was fixed back in the 80's" >&$stderr
					;;
				esac
				sh -c "cat <<!
$2
!
"
			fi
			;;
		*)	if	cat > "$1" <<!
$2
!
			then	: old here doc botch not present
			else	case $here_broken in
				0)	here_broken=1
					echo "$command: your shell botches here documents; this was fixed back in the 80's" >&$stderr
					;;
				esac
				sh -c "cat > '$1' <<!
$2
!
"
			fi
			;;
		esac
		;;
	esac
}

checkread()
{
	posix_read=`(read -r line; echo $line) 2>/dev/null <<!
a z
!
`
	case $posix_read in
	"a z")	posix_read=1
		;;
	*)	copy ${tmp}r.c "
		extern int read();
		extern int write();
		main()
		{
			char	c;
			char	r;
			int	k;
			k = 1;
			while (read(0, &c, 1) == 1)
			{
				if (k)
				{
					if (c == ' ' || c == '\\t')
						continue;
					k = 0;
				}
				if (c == '\\r')
				{
					r = c;
					if (read(0, &c, 1) == 1 && c != '\n')
						write(1, &r, 1);
				}
				write(1, &c, 1);
				if (c == '\\n')
					return 0;
			}
			return 1;
		}"
		if	$cc -o ${tmp}r.exe ${tmp}r.c
		then	posix_read=${tmp}r.exe
		else	echo "$command: cannot compile read -r workaround" >&$stderr
			exit 1
		fi
		;;
	esac
	case `(set -f && set x * && echo $# && set +f) 2>/dev/null` in
	2)	posix_noglob="set -f" posix_glob="set +f" ;;
	*)	case `(set -F && set x * && echo $# && set +F) 2>/dev/null` in
		2)	posix_noglob="set -F" posix_glob="set +F" ;;
		*)	posix_noglob=":" posix_glob=":" ;;
		esac
		;;
	esac
}

execute()
{
	if	test -d /NextDeveloper
	then	$tmp.exe <&$nullin >&$nullout
		_execute_=$?
		$tmp.exe <&$nullin | cat
	elif	test "" != "$cross"
	then	crossexec $cross "$@"
		_execute_=$?
	else	"$@"
		_execute_=$?
	fi
	return $_execute_
}

exclude()
{
	case $excludes in
	'')	return 0 ;;
	esac
	for _exclude_var
	do	eval _exclude_old=\$$_exclude_var
		case $_exclude_old in
		*" -I"*);;
		*)	continue ;;
		esac
		_exclude_new=
		_exclude_sep=
		for _exclude_arg in $_exclude_old
		do
			for _exclude_dir in $excludes
			do	case $_exclude_arg in
				-I$_exclude_dir|-I*/$_exclude_dir)
					;;
				*)	_exclude_new="$_exclude_new$_exclude_sep$_exclude_arg"
					_exclude_sep=" "
					;;
				esac
			done
		done
		eval $_exclude_var=\$_exclude_new
		case $debug in
		0)	;;
		*)	echo $command: exclude $_exclude_var: "$_exclude_old => $_exclude_new" >&$stderr
			;;
		esac
	done
}

all=0
binding="-dy -dn -Bdynamic -Bstatic -Wl,-ashared -Wl,-aarchive -call_shared -non_shared '' -static"
complete=0
config=0
defhdr=
iff=
usr=
cross=
debug=0
deflib=
dir=FEATURE
excludes=
executable="test -x"
exists="test -e"
gothdr=
gotlib=
ifs=$IFS
in=
includes=
intrinsic=
libpaths="LD_LIBRARY_PATH LD_LIBRARYN32_PATH LD_LIBRARY64_PATH LIBPATH SHLIB_PATH"
nl="
"
occ=cc
one=
out=
posix_read=
protoflags=
puthdr=
putlib=
pragma=
case $RANDOM in
$RANDOM)shell=bsh
	($executable .) 2>/dev/null || executable='test -r'
	($exists .) 2>/dev/null || exists='test -r'
	;;
*)	case $BASH_VERSION in
	?*)	shell=bash ;;
	*)	shell=ksh ;;
	esac
	;;
esac
reallystatic=
reallystatictest=
static=.
statictest=
style=C
case $COTEMP in
"")	case $HOSTNAME in
	""|?|??|???|????|????)
		tmp=${HOSTNAME}
		;;
	*)	case $shell in
		bsh)	eval `echo $HOSTNAME | sed 's/\\(....\\).*/tmp=\\1/'` ;;
		*)	eval 'tmp=${HOSTNAME%${HOSTNAME#????}}' ;;
		esac
		;;
	esac
	tmp=${tmp}$$
	;;
*)	tmp=$COTEMP
	;;
esac
case $tmp in
??????????*)
	case $shell in
	bsh)	eval `echo $tmp | sed 's/\\(.........\\).*/tmp=\\1/'` ;;
	*)	eval 'tmp=${tmp%${tmp#?????????}}' ;;
	esac
	;;
?????????)
	;;
????????)
	tmp=F$tmp
	;;
esac
tmp=./$tmp
undef=0
verbose=0

# options -- `-' for output to stdout otherwise usage

case $1 in
-)	out=-; shift ;;
esac
set=

case `(getopts '[-][123:xyz]' opt --xyz; echo 0$opt) 2>/dev/null` in
0123)	USAGE=$'
[-?
@(#)$Id: iffe (AT&T Labs Research) 2002-02-14 $
]
'$USAGE_LICENSE$'
[+NAME?iffe - host C compilation environment feature probe]
[+DESCRIPTION?\biffe\b is a command interpreter that probes the host C
	compilation environment for features. A feature is any file, option
	or symbol that controls or is controlled by the C compiler. \biffe\b
	tests features by generating and compiling C programs and observing
	the behavior of the C compiler and generated programs.]
[+?\biffe\b statements are line oriented. Statements may appear in the
	operand list with the \b:\b operand or \bnewline\b as the line
	delimiter. The standard input is read if there are no command
	line statements or if \afile\a\b.iffe\b is omitted.]
[+?Though similar in concept to \bautoconfig\b(1) and \bconfig\b(1), there
	are fundamental differences. The latter tend to generate global
	headers accessed by all components in a package, whereas \biffe\b is
	aimed at localized, self contained feature testing.]
[+?Output is generated in \bFEATURE/\b\atest\a by default, where \atest\a is
	the base name of \afile\a\b.iffe\b or the \biffe\b \brun\b command
	file operand. Output is first generated in a temporary file; the
	output file is updated if it does not exist or if the temporary file
	is different. If the first operand is \b-\b then the output is written
	to the standard output and no update checks are done.]
[+?Files with suffixes \b.iffe\b and \b.iff\b are assumed to contain
	\biffe\b statements.]
[a:all?Define failed test macros \b0\b. By default only successful test macros
	are defined \b1\b.]
[c:cc?Sets the C compiler name and flags to be used in the feature
	tests.]:[C-compiler-name [C-compiler-flags ...]]]
[C:config?Generate \bconfig\b(1) style \aHAVE_\a* macro names. This implies
	\b--undef\b. Since \bconfig\b(1) has inconsistent naming conventions,
	the \bexp\b command may be needed to translate from the (consistent)
	\biffe\b names. Unless otherwise noted a \bconfig\b macro name
	is the \biffe\b macro name prefixed with \bHAVE\b and converted to
	upper case. \b--config\b is set by default if the command arguments
	contain a \brun\b command on an input file with the base name
	\bconfig\b.]
[d:debug?Sets the debug level. Level 0 inhibits most
	error messages, level 1 shows compiler messages, and
	level 2 traces internal \biffe\b \bsh\b(1) actions.]#[level]
[i:input?Sets the input file name to \afile\a, which
	must contain \biffe\b statements.]:[file]
[I:include?Adds \b-I\b\adir\a to the C compiler flags.]:[dir]
[L:library?Adds \b-L\b\adir\a to the C compiler flags.]:[dir]
[o:output?Sets the output file name to \afile\a.]:[file]
[e:package?Sets the \bproto\b(1) package name to \aname\a.]:[name]
[p:prototyped?Emits \b#pragma prototyped\b at the top of the
	output file. See \bproto\b(1).]
[P:pragma?Emits \b#pragma\b \atext\a at the top of the output file.]:[text]
[s:shell?Sets the internal shell name to \ashell-path\a. Used for debugging
	Bourne shell compatibility (otherwise \biffe\b uses \aksh\a constructs
	if available).]:[shell-path]
[S:static?Sets the C compiler flags that force static linking. If not set
	then \biffe\b probes the compiler to determine the flags. \biffe\b
	must use static linking (no dlls) because on some systems missing
	library symbols are only detected when referenced at runtime from
	dynamically linked executables.]:[flags]
[u:undef?\b#undef\b failed test macros. By default only successful test macros
	are defined \b1\b.]
[v:verbose?Produce a message line on the standard error for each test as
	it is performed.]
[x:cross?Some tests compile an executable (\ba.out\b) and then run it.
	If the C compiler is a cross compiler and the executable format is
	incompatible with the execution environment then the generated
	executables must be run in a different environment, possibly on
	another host. \acrosstype\a is the HOSTTYPE for generated executables
	(the \bpackage\b(1) command generates a consistent HOSTTYPE namespace).
	Generated executables are run via \bcrossexec\b(1) with \acrosstype\a
	as the first argument. \bcrossexec\b supports remote execution for
	cross-compiled executables. See \bcrossexec\b(1) for
	details.]:[crosstype]
[X:exclude?Removes \b-I\b\adir\a and \b-I\b*/\adir\a C compiler flags.]:[dir]

[ - ] [ file.iffe | statement [ : statement ... ] ]

[+SYNTAX?\biffe\b input consists of a sequence of statement lines. Statements
	that span more than one line contain \abegin\a\b{\b as the last
	operand (where \abegin\a is command specific) and zero
	or more data lines terminated by a line containing
	\b}end\b as the first operand. The statements syntax is:
	\aop\a[,\aop\a...]] [\aarg\a[,\aarg\a...]]]] [\aprereq\a ...]]
	[\abegin\a{ ... |\bend\b ...]] [= [\adefault\a]]]]. \aop\as and \aarg\as
	may be combined, separated by commas, to perform a set of operations
	on a set of arguments.]
[+?\aprereq\as are used when applying the features tests and may be
	combinations of:]{
		[+compiler options?\b-D\b*, \b-L\b*, etc.]
		[+library references?\b-l\b*, *\b.a\b, etc. \b_LIB_\b\aname\a
			is defined to be 1 if \b-l\b\aname\a is a library.]
		[+header references?*\b.h\b. \a_dir_name\a is defined to be 1
			if \adir/name\a\b.h\b is a header, or if \adir\a is
			omitted, \b_hdr_\b\aname\a is defined to be 1 if
			\aname\a\b.h\b is a header.]
		[+-?Prereq grouping mark; prereqs before the first \b-\b are
			passed to all feature tests. Subsequent groups
			are attempted in left-to-right order until the first
			successful group is found.]
	}
[+?\abegin\a\b{\b ... \b}end\b delimit multiline code blocks that override
	or augment the default code provided by \biffe\b. User supplied code
	blocks should be compatible with the K&R, ANSI, and C++ C language
	dialects for maximal portability. In addition to all macro definitions
	generated by previous tests, all generated code contains the
	following at the top to hide dialect differences:]{
		[+#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)?]
		[+#define _STD_ 1?]
		[+#define _ARG_(x) x?]
		[+#define _VOID_ void?]
		[+#else?]
		[+#define _STD_ 0?]
		[+#define _ARG_(x) ()?]
		[+#define _VOID_ char?]
		[+#endif?]
		[+#if defined(__cplusplus)?]
		[+#define _BEGIN_EXTERNS_ extern "C" {?]
		[+#define _END_EXTERNS_ }?]
		[+#else?]
		[+#define _BEGIN_EXTERNS_?]
		[+#define _END_EXTERNS_?]
		[+#endif?]
		[+#define _NIL_(x) ((x)0)?]
	}
[+?= \adefault\a may be specified for the \bkey\b, \blib\b, \bmth\b and \btyp\b
	commands. If the test fails for \aarg\a then
	\b#define\b \aarg\a \adefault\a is emitted. \bkey\b accepts multiple
	\b= \b\adefault\a values; the first valid one is used.]
[+?Each test statement generates a portion of a C language header that contains
	macro defintions, comments, and other text corresponding to the feature
	test commands. \b#ifndef _def_\b\aname\a\b_\b\adirectory\a ...
	\b#endif\b guards the generated header from multiple \b#include\bs,
	where \aname\a is determined by either the \brun\b command input file
	name if any, or the first \aop\a of the first command, and \adirectory\a
	is the basname component of either the \brun\b command file, if any,
	or the current working directory. The output file name is determined
	in this order:]{
		[+-?If the first command line operand is \b-\b then the output
			is written to the standard output.]
		[+--output=\afile\a?Output is \afile\a.]
		[+set out \afile\a?Output is \afile\a.]
		[+[run]] [\adirectory\a/]]\abase\a[\a.suffix\a]]?Output is
			\bFEATURE/\b\abase\a.]
	}
[+?Generated \biffe\b headers are often referenced in C source as:
	\b#include "FEATURE/\b\afile\a". The \bnmake\b(1) base rules contain
	metarules for generating \bFEATURE/\b\afile\a from
	\bfeatures/\b\afile\a[\asuffix\a]], where \asuffix\a may be omitted,
	\b.c\b, or \b.sh\b (see the \brun\b command below). Because
	\b#include\b prerequisites are automatically detected, \bnmake\b(1)
	ensures that all prerequisite \biffe\b headers are generated before
	compilation. Note that the directories are deliberately named
	\bFEATURE\b and \bfeatures\b to keep case-insensitive file systems
	happy.]
[+?The feature test commands are:]{
	[+# \acomment\a?Comment line - ignored.]
	[+cmd \aname\a?Defines \b_cmd_\b\aname\a if \aname\a is an executable
		in one of the standard system directories (\b/bin, /etc,
		/usr/bin, /usr/etc, /usr/ucb\b).
		\b_\b\adirectory\a\b_\b\aname\a is defined for \adirectory\a
		in which \aname\a is found (with \b/\b translated to \b_\b).]
	[+dat \aname\a?Defines \b_dat_\b\aname\a if \aname\a is a data symbol
		in the default libraries.]
	[+def \aname\a?Equivalent to \bcmd,dat,hdr,key,lib,mth,sys,typ\b
		\aname\a.]
	[+dfn \aname\a?If \aname\a is a macro in the candidate headers then
		a \b#define\b \aname\a \avalue\a statment is output for the
		\avalue\a defined in the headers. The definition is \b#ifndef\b
		guarded.]
	[+exp \aname\a \aexpression\a?If \aexpression\a is a \"...\" string
		then \aname\a is defined to be the string, else if the
		\bexpr\b(1) evaluation of \aexpression\a is not 0 then \aname\a
		is defined to be 1, otherwise \aname\a is defined to be 0.
		Identifiers in \aexpression\a may be previously defined names
		from other \biffe\b commands; undefined names evaluate to 0.
		If \aname\a was defined in a previous successful test then
		the current and subsequent \bexp\b test on \aname\a are
		skipped.]
	[+hdr \aname\a?Defines \b_hdr_\b\aname\a if the header
		\b<\b\aname\a\b.h>\b exists. The \b--config\b macro name is
		\bHAVE_\b\aNAME\a\b_H\b.]
	[+iff \aname\a?The generated header \b#ifndef-#endif\b macro guard is
		\b_\b\aname\a\b_H\b.]
	[+key \aname\a?Defines \b_key_\b\aname\a if \aname\a is a reserved
		word (keyword).]
	[+lcl \aname\a?Generates a \b#include\b statement for the native version
		of either the header \b<\b\aname\a\b.h>\b if it exists or the
		header \b<sys/\b\aname\a\b.h>\b if it exists. Defines
		\b_lcl_\b\aname\a on success. The \b--config\b macro name is
		\bHAVE_\b\aNAME\a\b_H\b.]
	[+lib \aname\a?Defines \b_lib_\b\aname\a if \aname\a is an external
		symbol in the default libraries.]
	[+mac \aname\a?Defines \b_mac_\b\aname\a if \aname\a is a macro.]
	[+mem \astruct.member\a?Defines \b_mem_\b\amember\a\b_\b\astruct\a
		if \amember\a is a member of the structure \astruct\a.]
	[+mth \aname\a?Defines \b_mth_\b\aname\a if \aname\a is an external
		symbol in the math library.]
	[+nop \aname\a?If this is the first command then \aname\a may be used
		to name the output file and/or the output header guard macro.
		Otherwise this command is ignored.]
	[+npt \aname\a?Defines \b_npt_\b\aname\a if the \aname\a symbol
		requires a prototype. The \b--config\b macro name is
		\b\aNAME\a\b_DECLARED\b with the opposite sense.]
	[+num \aname\a?Defines \b_num_\b\aname\a if \aname\a is a numeric
		constant \aenum\a or \amacro\a.]
	[+nxt \aname\a?Defines a string macro \b_nxt_\b\aname\a suitable for
		a \b#include\b statement to include the next (on the include
		path) or native version of either the header
		\b<\b\aname\a\b.h>\b if it exists or the header
		\b<sys/\b\aname\a\b.h>\b if it exists. Also defines the \"...\"
		form \b_nxt_\b\aname\a\b_str\b. The \b--config\b
		macro name is \bHAVE_\b\aNAME\a\b_NEXT\b.]
	[+one \aheader\a ...?Generates a \b#include\b statement for the first
		header found in the \aheader\a list.]
	[+pth \afile\a [ \adir\a ... | { \ag1\a - ... - \agn\a } | < \apkg\a [\aver\a ...]] > ]]?Defines
		\b_pth_\b\afile\a, with embedded \b/\b chars translated to
		\b_\b, to the path of the first instance of \afile\a in the
		\adir\a directories. \b{\b ... \b}\b forms a directory list
		from the cross-product of \b-\b separated directory groups
		\ag1\a ... \agn\a. < ... > forms a directory list for the
		package \apkg\a with optional versions. The \b--config\b macro
		name is \aNAME\a\b_PATH\b.]
	[+run \afile\a?Runs the tests in \afile\a based on the \afile\a
		suffix:]{
		[+.c?\afile\a is compiled and executed and the output is copied
			to the \biffe\b output file.]
		[+.sh?\afile\a is executed as a shell script and the output is
			copied to the \biffe\b output file.]
		[+.iffe \bor no suffix?\afile\a contains \biffe\b
			statements.]
	}
	[+set \aoption value\a?Sets option values. The options are described
		above.]
	[+siz \aname\a?Defines \b_siz_\b\aname\a to be \bsizeof\b(\aname\a) if
		\aname\a is a type in any of \b<sys/types.h>, <times.h>,
		<stddef.h>, <stdlib.h>\b. Any \b.\b characters in \aname\a are
		translated to space before testing and are translated to \b_\b
		in the output macro name.]
	[+sym \aname\a?Defines \b_ary_\b\aname\a if \aname\a is an array,
		\b_fun_\b\aname\a if \aname\a is a function pointer,
		\b_ptr_\b\aname\a if \aname\a is a pointer, or
		\b_reg_\b\aname\a if \aname\a is a scalar. In most cases
		\aname\a is part of a macro expansion.]
	[+sys \aname\a?Defines \b_sys_\b\aname\a if the header
		\b<sys/\b\aname\a\b.h>\b exists. The \b--config\b macro name is
		\bHAVE_SYS_\b\aNAME\a\b_H\b.]
	[+tst \aname\a?A user defined test on name. A source block must be
		supplied. Defines \b_\b\aname\a on success.]
	[+typ \aname\a?Defines \b_typ_\b\aname\a if \aname\a is a type in any
		of \b<sys/types.h>, <times.h>, <stddef.h>, <stdlib.h>\b. Any
		\b.\b characters in \aname\a are translated to space before
		testing and are translated to \b_\b in the output macro name.]
	[+val \aname\a?The output of \becho\b \aname\a is written to the
		output file.]
}
[+?Code block names may be prefixed by \bno\b to invert the test sense. The
	block names are:]{
	[+cat?The block is copied to the output file.]
	[+compile?The block is compiled (\bcc -c\b).]
	[+execute?The block is compiled, linked, and executed. \b0\b exit
		status means success.]
	[+fail?If the test fails then the block text evaluated by \bsh\b(1).]
	[+link?The block is compiled and linked (\bcc -o\b).]
	[+macro?The block is preprocessed (\bcc -E\b) and lines containing
		text bracketed by \b<<"\b ... \b">>\b (\aless-than less-than
		double-quote ... double-quote greater-than greater-than\a)
		are copied to the output file with the brackets omitted.]
	[+no?If the test fails then the block text is copied to the
		output file.]
	[+note?If the test succeeds then the block is copied to the output
		as a \b/*\b ... \b*/\b comment.]
	[+output?The block is compiled, linked, and executed, and the output
		is copied to the output file.]
	[+pass?If the test succeeds then the block text evaluated by \bsh\b(1).]
	[+preprocess?The block is preprocessed (\bcc -E\b).]
	[+run?The block is executed as a shell script and the output is
		copied to the output file. Succesful test macros are also
		defined as shell variables with value \b1\b and are available
		within the block. Likewise, failed test macros are defined
		as shell variables with value \b0\b.]
	[+yes?If the test succeeds then the block text is copied to the
		output file.]
}
[+SEE ALSO?\bautoconfig\b(1), \bconfig\b(1), \bcrossexec\b(1), \bnmake\b(1),
	\bpackage\b(1), \bproto\b(1), \bsh\b(1)]
'
	while	getopts -a "$command" "$USAGE" OPT
	do	case $OPT in
		a)	set="$set set all :" ;;
		c)	set="$set set cc $OPTARG :" ;;
		C)	set="$set set config :" ;;
		d)	set="$set set debug $OPTARG :" ;;
		i)	set="$set set input $OPTARG :" ;;
		I)	set="$set set include $OPTARG :" ;;
		L)	set="$set set library $OPTARG :" ;;
		o)	set="$set set output $OPTARG :" ;;
		e)	set="$set set package $OPTARG :" ;;
		p)	set="$set set prototyped :" ;;
		P)	set="$set set pragma $OPTARG :" ;;
		s)	set="$set set shell $OPTARG :" ;;
		S)	set="$set set static $OPTARG :" ;;
		u)	set="$set set undef :" ;;
		v)	set="$set set verbose :" ;;
		x)	set="$set set cross $OPTARG :" ;;
		X)	set="$set set exclude $OPTARG :" ;;
		esac
	done
	shift `expr $OPTIND - 1`
	;;
*)	while	:
	do	case $# in
		0)	break ;;
		esac
		case $1 in
		-)	break
			;;
		--)	shift
			break
			;;
		--a|--al|--all)
			REM=a
			;;
		--cc=*)	REM=c`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--co|--con|--conf|--confi|--config)
			REM=C
			;;
		--cr=*|--cro=*|--cros=*|--cross=*)
			REM=x`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--d=*|--de=*|--deb=*|--debu=*|--debug=*)
			REM=d`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--e=*|--ex=*|--exc=*|--excl=*|--exclu=*|--exclud=*|--exclude=*)
			REM=X`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--inp=*|--inpu=*|--input=*)
			REM=i`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--inc=*|--incl=*|--inclu=*|--includ=*|--include=*)
			REM=I`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--l=*|--li=*|--lib=*|--libr=*|--libra=*|--librar=*|--library=*)
			REM=L`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--o=*|--ou=*|--out=*|--outp=*|--outpu=*|--output=*)
			REM=o`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--pa=*|--pac=*|--pack=*|--packa=*|--packag=*|--package=*)
			REM=e`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--pro|--prot|--proto|--protot|--prototy|--prototyp|--prototype|--prototyped)
			REM=p
			;;
		--pra=*|--prag=*|--pragma=*)
			REM=P`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--sh=*|--she=*|--shel=*|--shell=*)
			REM=s`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--st=*|--sta=*|--stat=*|--stati=*|--static=*)
			REM=S`echo $1 | sed -e 's,[^=]*=,,'`
			;;
		--u|--un|--und|--unde|--undef)
			REM=u
			;;
		--v|--ve|--ver|--verb|--verbo|--verbos|--verbose)
			REM=v
			;;
		--*)	echo $command: $1: unknown option >&2
			exit 2
			;;
		-*)	REM=`echo $1 | sed -e 's,-,,'`
			;;
		*)	break
			;;
		esac
		shift
		while	:
		do	case $REM in
			'')	break ;;
			esac
			eval `echo $REM | sed -e "s,\(.\)\(.*\),OPT='\1' REM='\2',"`
			case $OPT in
			[cdiILoePsSxX])
				case $REM in
				'')	case $# in
					0)	echo $command: -$OPT: option argument expected >&2
						exit 1
						;;
					esac
					OPTARG=$1
					shift
					;;
				*)	OPTARG=$REM
					REM=''
					;;
				esac
			esac
			case $OPT in
			a)	set="$set set all :" ;;
			c)	set="$set set cc $OPTARG :" ;;
			C)	set="$set set config :" ;;
			d)	set="$set set debug $OPTARG :" ;;
			i)	set="$set set input $OPTARG :" ;;
			I)	set="$set set include $OPTARG :" ;;
			L)	set="$set set library $OPTARG :" ;;
			o)	set="$set set output $OPTARG :" ;;
			e)	set="$set set package $OPTARG :" ;;
			p)	set="$set set prototyped :" ;;
			P)	set="$set set pragma $OPTARG :" ;;
			s)	set="$set set shell $OPTARG :" ;;
			S)	set="$set set static $OPTARG :" ;;
			u)	set="$set set undef :" ;;
			v)	set="$set set verbose :" ;;
			x)	set="$set set cross $OPTARG :" ;;
			X)	set="$set set exclude $OPTARG :" ;;
			*)	echo "Usage: $command [-aCpuv] [-c C-compiler-name [C-compiler-flags ...]] [-d level]
	    [-i file] [-o file] [-e name] [-P text] [-s shell-path] [-S[flags]]
	    [-x cross-exec-prefix] [-I dir] [-L dir] [-X dir] [ - ]
	    [ file.iffe | statement [ : statement ... ] ]" >&2
				exit 2
				;;
			esac
		done
	done
	;;
esac
case $1 in
-)	out=-; shift ;;
esac
case $# in
0)	in=- ;;
esac
set -- $set "$@"
case " $* " in
*' set config '*|*' run config.'*|*' run '*' config.'*|*' run '*'/config.'*)
	config=1
	;;
esac

# standard error to /dev/null unless debugging
# standard output to the current output file
#
#	stdout	original standard output
#	stderr	original standard error
#	nullin	/dev/null input
#	nullout	/dev/null output

stdin=4 stdout=5 stderr=6 nullin=7 nullout=8
eval "exec $stdin<&0 $nullin</dev/null $nullout>/dev/null $stdout>&1 $stderr>&2"
case " $* " in
*" set debug "[3456789]*)
	;;
*)	eval "exec 2>&$nullout"
	;;
esac

# prompt complications

case `print -n aha 2>/dev/null` in
aha)	show='print -n' SHOW='' ;;
*)	case `echo -n aha 2>/dev/null` in
	-n*)	show=echo SHOW='\c' ;;
	*)	show='echo -n' SHOW='' ;;
	esac
	;;
esac

# tmp files cleaned up on exit
# status: 0:success 1:failure 2:interrupt

status=1
if	(ulimit -c 0) >/dev/null 2>&1
then	ulimit -c 0
	core=
else	core=core
fi
trap "rm -f $core $tmp*.*; exit \$status" 0 1 2
if	(:>$tmp.c) 2>/dev/null
then	rm -f $tmp.c
else	echo "$command: cannot create tmp files in current dir" >&2
	exit 1
fi
status=2

# standard header for c source

std='#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define _STD_		1
#define _ARG_(x)	x
#define _VOID_		void
#else
#define _STD_		0
#define _ARG_(x)	()
#define _VOID_		char
#endif
#if defined(__cplusplus)
#define _BEGIN_EXTERNS_	extern "C" {
#define _END_EXTERNS_	}
#else
#define _BEGIN_EXTERNS_
#define _END_EXTERNS_
#endif
#define _NIL_(x)	((x)0)'
ext='#ifndef feof
_BEGIN_EXTERNS_
#if _STD_
extern int	printf(const char*, ...);
#else
extern int	printf();
#endif
_END_EXTERNS_
#endif'

# loop on op [ arg [ ... ] ] [ : op [ arg [ ... ] ] ]

argx=0
cur=.
can=
cansep=
cctest=
hdrtest=
while	:
do	case $in in
	"")	case $argx:$* in
		1:$argv);;
		1:*)	argx=0
			set x $argv
			shift
			;;
		esac
		case $# in
		0)	set set out + ;;
		esac
		;;
	*)	read lin || lin="set out +"
		set x $lin
		shift
		;;
	esac

	# check if "run xxx" is equivalent to "set in xxx"

	case $1 in
	*.iffe|*.iff)	set run "$@" ;;
	esac
	case $1 in
	:)	shift
		continue
		;;
	run)	case $shell in
		bsh)	case $2 in
			*/*)	x=`echo $2 | sed -e 's,.*[\\/],,'` ;;
			*)	x=$2 ;;
			esac
			;;
		*)	eval 'x=${2##*[\\/]}'
			;;
		esac
		case $x in
		*.iffe|*.iff)
			set set in $2 ;;
		*.*)	;;
		*)	set set in $2 ;;
		esac
		;;
	esac

	# set drops out early

	case $1 in
	""|\#*)	continue
		;;
	set)	shift
		case $1 in
		""|\#*)	op=
			;;
		*)	arg=
			op=$1
			case $op in
			--*)	case $shell in
				bsh)	op=`echo '' $op | sed 's/.*--//'` ;;
				*)	op=${op#--} ;;
				esac
				;;
			-*)	case $op in
				-??*)	case $shell in
					bsh)	arg=`echo '' $op | sed 's/-.//'`
						op=`echo '' $op | sed 's/\\(-.\\).*/\\1/'`
						;;
					*)	arg=${op#-?}
						op=${op%$arg}
						;;
					esac
					;;
				esac
				case $op in
				a)	op=all ;;
				c)	op=cc ;;
				C)	op=config ;;
				d)	op=debug ;;
				i)	op=input ;;
				I)	op=include ;;
				L)	op=library ;;
				o)	op=output ;;
				e)	op=package ;;
				p)	op=prototyped ;;
				P)	op=pragma ;;
				s)	op=shell ;;
				S)	op=static ;;
				u)	op=undef ;;
				v)	op=verbose ;;
				x)	op=cross ;;
				X)	op=exclude ;;
				esac
				;;
			esac
			shift
			while	:
			do	case $# in
				0)	break ;;
				esac
				case $1 in
				*" "*)	shift
					continue
					;;
				""|\#*)	break
					;;
				:)	shift
					break
					;;
				esac
				case $arg in
				"")	arg=$1 ;;
				*)	arg="$arg $1" ;;
				esac
				shift
			done
			;;
		esac
		case $op in
		all)	all=1
			continue
			;;
		cc)	occ=
			for x in $arg
			do	case $occ in
				"")	case $x in
					*=*)	case $shell in
						bsh)	eval $x
							export `echo $x | sed -e 's/=.*//'`
							;;
						*)	export $x
							;;
						esac
						;;
					*)	occ=$x
						;;
					esac
					;;
				*)	occ="$occ $x"
					;;
				esac
			done
			exclude occ
			continue
			;;
		config)	config=1
			continue
			;;
		cross)	case $arg in
			""|-)	cross= ;;
			*)	cross="$arg" libpaths= ;;
			esac
			continue
			;;
		debug)	debug=$arg
			case $arg in
			0)	exec 2>&$nullout
				set -
				show=echo
				SHOW=
				;;
			""|1)	exec 2>&$stderr
				set -
				show=echo
				SHOW=
				;;
			2|3)	exec 2>&$stderr
				case $shell in
				ksh)	eval 'PS4="${PS4%+*([ 	])}+\$LINENO+ "'
				esac
				show=echo
				SHOW=
				set -x
				;;
			*)	echo "$command: $arg: debug levels are 0, 1, 2, 3" >&$stderr
				;;
			esac
			continue
			;;
		exclude)case $arg in
			""|-)	excludes= ;;
			*)	excludes="$excludes $arg" ;;
			esac
			exclude includes occ
			continue
			;;
		"in"|input)
			case $arg in
			"")	in=-
				;;
			*)	in=$arg
				if	test ! -r $in
				then	echo "$command: $in: not found" >&$stderr
					exit 1
				fi
				exec < $in
				case $out in
				"")	case $in in
					*[.\\/]*)
						case $shell in
						bsh)	eval `echo $in | sed -e 's,.*[\\/],,' -e 's/\\.[^.]*//' -e 's/^/out=/'`
							;;
						*)	eval 'out=${in##*[\\/]}'
							eval 'out=${out%.*}'
							;;
						esac
						;;
					*)	out=$in
						;;
					esac
					;;
				esac
				;;
			esac
			continue
			;;
		include)case $arg in
			""|-)	includes= ;;
			*)	includes="$includes -I$arg" ;;
			esac
			exclude includes
			continue
			;;
		library)for y in $libpaths
			do	eval $y=\"\$$y:\$arg\"
				eval export $y
			done
			continue
			;;
		nodebug)exec 2>&$nullout
			set -
			continue
			;;
		out|output)
			out=$arg
			defhdr=
			usr=
			deflib=
			one=
			puthdr=
			putlib=
			case $op in
			output)	continue ;;
			esac
			;;
		package)protoflags="$protoflags -e $arg"
			continue
			;;
		prototyped|noprototyped)
			pragma="$pragma $op"
			continue
			;;
		pragma) pragma="$pragma $arg"
			continue
			;;
		shell)	shell=$arg
			continue
			;;
		static)	static=$arg
			continue
			;;
		undef)	undef=1
			continue
			;;
		verbose)verbose=1
			continue
			;;
		*)	echo "$command: $op: unknown option" >&$stderr
			exit 1
			;;
		esac
		;;
	*)	case $1 in
		*\{)	op=-
			;;
		*)	op=$1
			shift
			;;
		esac
		arg=
		cc="$occ $includes"
		fail=
		hdr=
		lib=
		mac=
		no=
		note=
		opt=
		pass=
		pth=
		run=
		set=
		src=
		test=
		yes=
		case $op in
		pth)	lst=$op ;;
		*)	lst=hdr ;;
		esac
		case $# in
		0)	;;
		*)	case $1 in
			\#*)	set x
				shift
				;;
			*)	case $op in
				ref)	;;
				*)	case $1 in
					*\{)	arg=-
						;;
					*)	arg=$1
						shift
					esac
					;;
				esac
				;;
			esac
			while	:
			do	case $# in
				0)	break ;;
				esac
				case $1 in
				"")	;;
				\#*)	set x
					;;
				\=)	shift
					set=$*
					case $set in
					"")	set=" " ;;
					esac
					while	:
					do	case $# in
						0)	break ;;
						esac
						shift
					done
					break
					;;
				[abcdefghijklmnopqrstuvwxyz]*\{)
					v=$1
					shift
					x=
					case $v in
					note\{)	sep=" " ;;
					*)	sep=$nl ;;
					esac
					SEP=
					while	:
					do	case $# in
						0)	case $posix_read in
							'')	checkread ;;
							esac
							case $in in
							"")	echo "$command: missing }end" >&$stderr
								exit 1
								;;
							esac
							while	:
							do	case $posix_read in
								1)	read -r lin ;;
								*)	lin=`$posix_read` ;;
								esac
								case $? in
								0)	$posix_noglob
									set x $lin
									$posix_glob
									case $2 in
									\}end)	shift
										break 2
										;;
									esac
									x="$x$SEP$lin"
									SEP=$sep
									;;
								*)	echo "$command: missing }end" >&$stderr
									exit 1
									;;
								esac
							done
							;;
						esac
						case $1 in
						\}end)	break
							;;
						*)	x="$x$SEP$1"
							SEP=$sep
							;;
						esac
						shift
					done
					x="$x$nl" # \r\n bash needs this barf #
					case $v in
					fail\{)		fail=$x ;;
					nofail\{)	pass=$x v=pass\{ ;;
					nopass\{)	fail=$x v=fail\{ ;;
					no\{)		no=$x ;;
					note\{)		note=$x ;;
					pass\{)		pass=$x ;;
					test\{)		test=$x ;;
					yes\{)		yes=$x ;;
					*)		src=$x run=$v ;;
					esac
					;;
				:)	shift
					break
					;;
				*[\"\'\(\)\{\}\ \	]*)
					case $op in
					pth)	pth="$pth $1"
						lst=pth
						;;
					*)	test="$test $1"
						;;
					esac
					;;
				-)	case $2 in
					[-+]l*)	lst=lib ;;
					-D*)	lst=mac ;;
					esac
					case $lst in
					hdr)	hdr="$hdr $1" ;;
					lib)	lib="$lib $1" ;;
					mac)	mac="$mac $1" ;;
					pth)	pth="$pth $1" ;;
					esac
					;;
				-l*)	lib="$lib $1"
					lst=lib
					;;
				+l*)	case $shell in
					bsh)	x=`echo '' $1 | sed 's/.*+//'` ;;
					*)	eval 'x=${1#+}' ;;
					esac
					lib="$lib -$x"
					lst=lib
					;;
				-*|+*)	case $op in
					ref)	cc="$cc $1"
						occ="$occ $1"
						case $1 in
						-L*)	case $shell in
							ksh)	x=${1#-L} ;;
							*)	x=`echo x$1 | sed 's,^x-L,,'` ;;
							esac
							for y in $libpaths
							do	eval $y=\"\$$y:\$x\"
								eval export $y
							done
							;;
						esac
						;;
					run)	opt="$opt $1"
						;;
					*)	case $1 in
						-D*)	mac="$mac $1"
							lst=mac
							;;
						*)	cc="$cc $1"
							;;
						esac
						;;
					esac
					;;
				*.[aAxX]|*.[dD][lL][lL]|*.[lL][iI][bB])
					lib="$lib $1"
					lst=lib
					;;
				*[.\\/]*)
					case $op in
					pth)	pth="$pth $1"
						lst=pth
						;;
					*)	hdr="$hdr $1"
						lst=hdr
						;;
					esac
					;;
				*)	case $op in
					pth)	pth="$pth $1"
						lst=pth
						;;
					*)	test="$test $1"
						;;
					esac
					;;
				esac
				shift
			done
			;;
		esac
		;;
	esac

	# check for local package root directories

	case $PACKAGE_PATH in
	?*)	for i in `echo $PACKAGE_PATH | sed 's,:, ,g'`
		do	if	test -d $i/include
			then	cc="$cc -I$i/include"
				occ="$occ -I$i/include"
			fi
			if	test -d $i/lib
			then	cc="$cc -L$i/lib"
				occ="$occ -L$i/lib"
				for y in $libpaths
				do	eval $y=\"\$$y:\$i/lib\"
					eval export $y
				done
			fi
		done
		;;
	esac
	case $cc in
	"")	cc="$occ $includes" ;;
	esac

	# save $* for ancient shells

	argx=1
	argv=$*

	# check the candidate macros

	case $mac in
	?*)	case " $mac " in
		*" - "*);;
		*)	cc="$cc $mac"
			mac=
			;;
		esac
		;;
	esac

	# make sure $cc compiles C

	case $cctest in
	"")	echo "int i = 1;" > $tmp.c
		if	$cc -c $tmp.c <&$nullin >&$nullout
		then	echo "(;" > $tmp.c
			if	$cc -c $tmp.c <&$nullin >&$nullout
			then	cctest="should not compile '(;'"
			fi
		else	cctest="should compile 'int i = 1;'"
		fi
		case $cctest in
		"")	cctest=0
			;;
		*)	echo "$command: $cc: not a C compiler: $cctest" >&$stderr
			exit 1
			;;
		esac
		;;
	esac

	# check for global default headers (some cc -E insist on compiling)

	case $hdrtest in
	'')	hdrtest=1
		allinc=
		for x in types
		do	case $config in
			0)	c=_sys_${x}
				;;
			1)	case $shell in
				ksh)	typeset -u u=$x ;;
				*)	u=`echo $x | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ` ;;
				esac
				c=HAVE_SYS_${u}_H
				;;
			esac
			x=sys/$x.h
			echo "${allinc}#include <$x>" > $tmp.c
			is hdr $x
			if	$cc -E $tmp.c <&$nullin >&$nullout
			then	success
				gothdr="$gothdr + $x"
				can="$can$cansep#define	$c	1	/* #include <$x> ok */"
				cansep=$nl
				eval $c=1
				allinc="${allinc}#include <$x>$nl"
			else	failure
				gothdr="$gothdr - $x"
				case $all$config$undef in
				?1?|??1)can="$can$cansep#undef	$c		/* #include <$x> not ok */"
					cansep=$nl
					;;
				1??)	can="$can$cansep#define	$c	0	/* #include <$x> not ok */"
					cansep=$nl
					;;
				esac
			fi
		done
		;;
	esac

	# add implicit headers/libraries before the checks

	case $op in
	npt)	hdr="sys/types.h stdlib.h unistd.h $hdr"
		;;
	siz|typ)hdr="sys/types.h time.h sys/time.h sys/times.h stddef.h stdlib.h $hdr"
		;;
	esac

	# check the candidate headers

	case $hdr in
	?*)	z=$hdr
		hdr=
		hit=0
		for x in $z
		do	case $x in
			-)	case $hit in
				0)	hit=1 ;;
				1)	;;
				*)	break ;;
				esac
				continue
				;;
			*.h)	case " $gothdr " in
				*" - $x "*)
					continue
					;;
				*" + $x "*)
					;;
				*)	case $shell in
					bsh)	eval `echo $x | sed -e 's,^\\([^\\/]*\\).*[\\/]\\([^\\/]*\\)\$,\\1_\\2,' -e 's/\\..*//' -e 's/^/c=/'`
						;;
					*)	eval 'c=${x##*[\\/]}'
						eval 'c=${c%%.*}'
						case $x in
						*/*)	eval 'c=${x%%[\\/]*}_${c}' ;;
						esac
						;;
					esac
					case $config in
					0)	case $x in
						*/*)	c=_${c} ;;
						*)	c=_hdr_${c} ;;
						esac
						;;
					1)	case $shell in
						ksh)	typeset -u u=$c ;;
						*)	u=`echo $c | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ` ;;
						esac
						c=HAVE_${u}_H
						;;
					esac
					echo "${allinc}#include <$x>" > $tmp.c
					is hdr $x
					if	$cc -E $tmp.c <&$nullin >&$nullout
					then	success
						gothdr="$gothdr + $x"
						can="$can$cansep#define	$c	1	/* #include <$x> ok */"
						cansep=$nl
						eval $c=1
					else	failure
						gothdr="$gothdr - $x"
						case $all$config$undef in
						?1?|??1)can="$can$cansep#undef	$c		/* #include <$x> not ok */"
							cansep=$nl
							;;
						1??)	can="$can$cansep#define	$c	0	/* #include <$x> not ok */"
							cansep=$nl
							;;
						esac
						continue
					fi
					;;
				esac
				;;
			*)	test -r $x || continue
				;;
			esac
			hdr="$hdr $x"
			case $hit in
			0)	;;
			1)	hit=2 ;;
			esac
		done
		;;
	esac

	# check the candidate libraries

	case $lib in
	?*)	z=$lib
		lib=
		p=
		hit=0
		echo "main(){return(0);}" > $tmp.c
		for x in $z -
		do	case $x in
			-)	case $hit in
				0)	hit=1
					continue
					;;
				esac
				;;
			*)	case $hit in
				0)	p=$x
					;;
				*)	case $p in
					'')	p=$x ;;
					*)	p="$p $x" ;;
					esac
					continue
					;;
				esac
				;;
			esac
			case " $gotlib " in
			*"- $p "*)
				p=
				;;
			*"+ $p "*)
				lib="$lib $p"
				;;
			*)	rm -f $tmp.exe
				is LIB $p
				if	$cc -o $tmp.exe $tmp.c $p <&$nullin >&$nullout
				then	success
					gotlib="$gotlib + $p"
					lib="$lib $p"
					e=0
				else	failure
					gotlib="$gotlib - $p"
					e=1
				fi
				y=
				for x in $p
				do	case $shell in
					bsh)	c=`echo '' $x | sed 's, *-l,,'` ;;
					*)	eval 'c=${x#-l}' ;;
					esac
					case $c in
					*[!abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_]*)
						c=`echo '' $c | sed -e 's,.*[\\\\/],,' -e 's,\.[^.]*$,,' -e 's,[^abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_],_,g' -e '/^lib./s,^lib,,'`
						;;
					esac
					case $config in
					0)	case $e$p in
						0*' '*)	case " $gotlib " in
							*[-+]" $x "*)
								;;
							*)	can="$can$cansep#define	_LIB_$c	1	/* $x is a library */"
								cansep=$nl
								eval _LIB_$c=1
								;;
							esac
							;;
						esac
						;;
					1)	case $shell in
						ksh)	typeset -u u=$c ;;
						*)	u=`echo $c | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ` ;;
						esac
						c=$u
						case $e in
						0*' '*)	case " $gotlib " in
							*[-+]" $x "*)
								;;
							*)	can="$can$cansep#define	HAVE_${c}_LIB	1	/* $x is a library */"
								cansep=$nl
								eval HAVE_${c}_LIB=1
								;;
							esac
							;;
						esac
						;;
					esac
					y=${y}_$c
				done
				case $config in
				0)	c=_LIB${y} ;;
				1)	c=HAVE${y}_LIB ;;
				esac
				case $p in
				*' '*)	q="a library group" ;;
				*)	q="a library" ;;
				esac
				case $e in
				0)	can="$can$cansep#define	$c	1	/* $p is $q */"
					cansep=$nl
					eval $c=1
					case $hit in
					1)	break ;;
					esac
					;;
				1)	case $all$config$undef in
					?1?|??1)can="$can$cansep#undef	$c		/* $p is not $q */"
						cansep=$nl
						;;
					1??)	can="$can$cansep#define	$c	0	/* $p is not $q */"
						cansep=$nl
						;;
					esac
					eval $c=0
					;;
				esac
				p=
				;;
			esac
		done
		;;
	esac

	# last op precheck

	case $op in
	ref)	deflib="$deflib $lib"
		defhdr="$defhdr $hdr"
		continue
		;;
	esac
	IFS=" ,"
	case $shell in
	bash)	op=`echo $op`
		arg=`echo $arg`
		;;
	*)	eval op=\"$op\"
		eval arg=\"$arg\"
		;;
	esac
	IFS=$ifs

	# check for op aliases

	x=
	for o in $op
	do	case $o in
		def|default)	x="$x cmd dat hdr key lib mth sys typ" ;;
		*)		x="$x $o" ;;
		esac
	done

	# loop on the ops o and args a

	for o in $x
	do	for a in $arg
		do	user_pf= user_yn=
			case $a in
			*[.\\/]*)
				case $o in
				hdr|pth|sys)
					case $a in
					*[\\/]*) x=/ ;;
					*)	 x=. ;;
					esac
					case $shell in
					bsh)	case $x in
						.)	x="\\$x" ;;
						esac
						eval `echo $a | sed -e 's,\\(.*\\)'"${x}"'\\(.*\\),p=\\1 v=\\2,'`
						;;
					*)	eval 'p=${a%%${x}*}'
						eval 'v=${a##*${x}}'
						;;
					esac
					case $v in
					lcl|nxt)t=$p
						p=$v
						v=$t
						;;
					esac
					;;
				*)	case $shell in
					bsh)	eval `echo $a | sed -e 's,.*[\\/],,' -e 's/\\(.*\\)\\.\\(.*\\)/p=\\1 v=\\2/'`
						;;
					*)	eval 'p=${a%.*}'
						eval 'p=${p##*[\\/]}'
						eval 'v=${a##*.}'
						eval 'v=${v##*[\\/]}'
						;;
					esac
					;;
				esac
				f=${p}/${v}
				case $o in
				run)	v=$p
					p=
					m=_${v}
					;;
				mem)	case $p in
					*.*)	case $shell in
						bsh)	eval `echo $p | sed -e 's/\\([^.]*\\)\\.\\(.*\\)/p=\\1 m=\\2/'`
							;;
						*)	eval 'm=${p#*.}'
							eval 'p=${p%%.*}'
							;;
						esac
						v=${m}.${v}
					esac
					case $config in
					0)	m=_${v}_${p} ;;
					1)	m=_${v}_in_${p} ;;
					esac
					;;
				*)	m=_${p}_${v}
					;;
				esac
				;;
			*)	p=
				v=$a
				f=$a
				m=_${v}
				;;
			esac
			M=$m
			case $m in
			*[!abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_]*)
				m=`echo "X$m" | sed -e 's,^.,,' -e 's,[^abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_],_,g'`
				;;
			esac

			# check output redirection

			case $out in
			$cur)	;;
			*)	case $cur in
				$a|$v)	;;
				*)	case $cur in
					.)	;;
					-)	case $iff in
						?*)	echo "#endif" ;;
						esac
						;;
					*)	case $iff in
						?*)	echo "#endif" ;;
						esac
						exec >/dev/null
						case $cur in
						*[\\/]*|*.h)	x=$cur ;;
						*)		x=$dir/$cur ;;
						esac
						case $style in
						C)	(proto -r $protoflags $tmp.h) >/dev/null 2>&1
							sed 's,/\*[^/]*\*/, ,g' $tmp.h > $tmp.c
							sed 's,/\*[^/]*\*/, ,g' $x > $tmp.t
							;;
						*)	sed '/^#/d' $tmp.h > $tmp.c
							sed '/^#/d' $x > $tmp.t
							;;
						esac
						if	cmp -s $tmp.c $tmp.t
						then	rm -f $tmp.h
							case $verbose in
							1) echo "$command: $x: unchanged" >&$stderr ;;
							esac
						else	case $x in
							${dir}[\\/]$cur)	test -d $dir || mkdir $dir || exit 1 ;;
							esac
							mv $tmp.h $x
						fi
						;;
					esac
					case $out in
					+)	case $status in
						1)	;;
						*)	status=0 ;;
						esac
						exit $status
						;;
					-)	eval "exec >&$stdout"
						;;
					*)	exec >$tmp.h
						;;
					esac
					case $out in
					"")	case $a in
						*[\\/]*|???????????????*) cur=$v ;;
						*)			cur=$a ;;
						esac
						;;
					*)	cur=$out
						;;
					esac
					case $in in
					""|-|+)	case $o in
						run)	x=" from $a" ;;
						*)	x= ;;
						esac
						;;
					*)	x=" from $in"
						;;
					esac

					# output header comments

					case $style in
					C)	echo "/* : : generated$x by $command version $version : : */"
						for x in $pragma
						do	echo "#pragma $x"
						done
						case $out in
						""|-|+)	x=$m
							;;
						*.*)	case $shell in
							bsh)	eval `echo $in | sed -e 's,\\.,_,g' -e 's/^/x=/'`
								;;
							*)	i=$out
								x=_
								while	:
								do	case $i in
									*.*)	eval 'x=$x${i%%.*}_'
										eval 'i=${i#*.}'
										;;
									*)	x=$x$i
										break
										;;
									esac
								done
								;;
							esac
							;;
						*)	x=_$out
							;;
						esac
						case $o in
						iff)	case $M in
							""|*-*)	;;
							*)	iff=${m}_H ;;
							esac
							;;
						*)	case $x in
							*-*)	;;
							*)	x=`pwd | sed -e 's,.*[\\/],,' -e 's,\\..*,,' -e 's,^lib,,' -e 's,^,'${x}_',' -e 's,[^abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_],_,g'`
								iff=_def${x}
								;;
							esac
							;;
						esac
						case $iff in
						?*)	echo "#ifndef $iff"
							echo "#define $iff	1"
							;;
						esac
						;;
					*)	echo "# : : generated$x by $command version $version : : #"
						;;
					esac
					;;
				esac
				;;
			esac
			case $can in
			?*)	echo "$can"
				can=
				cansep=
				;;
			esac

			# set up the candidate include list

			inc=
			for x in $defhdr $hdr
			do	case $x in
				*.h)	case $shell in
					bsh)	eval `echo $x | sed -e 's,^\\([^\\/]*\\).*[\\/]\\([^\\/]*\\)\$,\\1_\\2,' -e 's/\\..*//' -e 's/^/c=/'`
						;;
					*)	eval 'c=${x##*[\\/]}'
						eval 'c=${c%%.*}'
						case $x in
						*/*)	eval 'c=${x%%[\\/]*}_${c}' ;;
						esac
						;;
					esac
					case $config in
					0)	case $x in
						*/*)	c=_${c} ;;
						*)	c=_hdr_${c} ;;
						esac
						;;
					1)	case $shell in
						ksh)	typeset -u u=$c ;;
						*)	u=`echo $c | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ` ;;
						esac
						c=HAVE_${u}_H
						;;
					esac
					case " $puthdr " in
					*" $c "*)
						;;
					*)	puthdr="$puthdr $c"
						usr="$usr$nl#define $c 1"
						;;
					esac
					inc="$inc
#include <$x>"
					;;
				esac
			done

			# set up the candidate lib list

			for x in $lib $deflib
			do	case $shell in
				ksh)	eval 'c=${x#-l}' ;;
				*)	c=`echo '' $x | sed 's, *-l,,'` ;;
				esac
				case $c in
				*[!abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_]*)
					c=`echo '' $c | sed -e 's,.*[\\\\/],,' -e 's,\.[^.]*$,,' -e 's,[^abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_],_,g' -e '/^lib./s,^lib,,'`
					;;
				esac
				case $config in
				0)	c=_LIB_${c}
					;;
				1)	case $shell in
					ksh)	typeset -u u=$c ;;
					*)	u=`echo $c | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ` ;;
					esac
					c=HAVE_${u}_LIB
					;;
				esac
				case " $putlib " in
				*" $c "*)
					;;
				*)	putlib="$putlib $c"
					usr="$usr$nl#define $c 1"
					;;
				esac
			done

			# src overrides builtin test

			case $config in
			0)	case $o in
				tst)	;;
				*)	m=_${o}${m} ;;
				esac
				;;
			1)	case $o in
				tst)	m=${v} ;;
				esac
				case $shell in
				ksh)	typeset -u u=$m ;;
				*)	u=`echo $m | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ` ;;
				esac
				case $o in
				tst)	case $m in
					$u)	;;
					*)	case $m in
						hdr_*|lib_*|sys_*)
							case $shell in
							ksh)	u=${u#????} ;;
							*)	u=`echo $u | sed 's/....//'` ;;
							esac
							;;
						esac
						m=HAVE_${u}
						;;
					esac
					;;
				hdr|lcl)m=HAVE${u}_H ;;
				npt|pth)case $op in
					npt)	m=${u}_DECLARED ;;
					pth)	m=${u}_PATH ;;
					esac
					case $shell in
					ksh)	m=${m#_} ;;
					*)	m=`echo $m | sed 's,^_,,'` ;;
					esac
					;;
				nxt)	m=HAVE${u}_NEXT ;;
				sys)	m=HAVE_SYS${u}_H ;;
				*)	m=HAVE${u} ;;
				esac
				;;
			esac
			case $v in
			*.*)	pre=
				for x in `echo $v | sed 's,\\., ,g'`
				do	pre="$pre
#undef $x"
				done
				;;
			*)	pre="#undef $v"
				;;
			esac
			case $src in
			?*)	case $src in
				*['<"'][Ss][Tt][Dd][Ii][Oo].[Hh]['">']*)EXT= ;;
				*)					EXT=$ext ;;
				esac
				copy $tmp.c "$std
$usr
$inc
$EXT
$src"
				e=
				is tst "${note:-$run}"
				case $run in
				cat*|nocat*)
					copy - "$src"
					;;
				run*|norun*)
					(eval "$src") <&$nullin || e=1
					;;
				mac*|nomac*)
					if	$cc -E $tmp.c <&$nullin >$tmp.i
					then	sed -e '/<<[ 	]*".*"[ 	]*>>/!d' -e 's/<<[ 	]*"//g' -e 's/"[ 	]*>>//g' $tmp.i
					else	e=1
					fi
					;;
				p*|nop*)$cc -DTEST=$p -DID=$v -E $tmp.c <&$nullin >&$nullout || e=1
					;;
				c*|noc*)$cc -DTEST=$p -DID=$v -c $tmp.c <&$nullin >&$nullout || e=1
					;;
				*)	case $run in
					s*|nos*)case $reallystatictest in
						'')	#UNDENT...

		reallystatictest=.
		echo '#include <stdio.h>
main(){printf("hello");return(0);}' > ${tmp}s.c
		rm -f ${tmp}s.exe
		if	$cc -c ${tmp}s.c <&$nullin >&$nullout && $cc -o ${tmp}s.exe ${tmp}s.o <&$nullin >&$nullout 2>${tmp}s.e && $executable ${tmp}s.exe
		then	e=`wc -l ${tmp}s.e`
			eval set x x $binding
			while	:
			do	shift
				shift
				case $# in
				0)	break ;;
				esac
				rm -f ${tmp}s.exe
				$cc -o ${tmp}s.exe $1 ${tmp}s.o <&$nullin >&$nullout 2>${tmp}s.e && $executable ${tmp}s.exe || continue
				case `wc -l ${tmp}s.e` in
				$e)	;;
				*)	continue ;;
				esac
				d=`ls -s ${tmp}s.exe`
				rm -f ${tmp}s.exe
				$cc -o ${tmp}s.exe $2 ${tmp}s.o <&$nullin >&$nullout 2>${tmp}s.e && $executable ${tmp}s.exe || continue
				case `wc -l ${tmp}s.e` in
				$e)	;;
				*)	continue ;;
				esac
				case `ls -s ${tmp}s.exe` in
				$d)	;;
				*)	reallystatic=$2
					set x
					shift
					break
					;;
				esac
			done
		fi
		rm -f ${tmp}s.*
							#...INDENT
							;;
						esac
						ccflags=$reallystatic
						;;
					*)	ccflags=
						;;
					esac
					set x $mac
					e=1
					while	:
					do	o=
						shift
						while	:
						do	case $# in
							0)	break ;;
							esac
							case $1 in
							-)	break ;;
							esac
							o="$o $1"
							shift
						done
						rm -f $tmp.exe
						if	$cc $ccflags $o -DTEST=$p -DID=$v -o $tmp.exe $tmp.c $lib $deflib <&$nullin >&$nullout && $executable $tmp.exe
						then	case $run in

			no[ls]*);;
			[ls]*)	e= && break ;;
			noo*)	execute $tmp.exe <&$nullin || break ;;
			o*)	execute $tmp.exe <&$nullin && e= && break ;;
			no*)	execute $tmp.exe <&$nullin >&$nullout || break ;;
			*)	execute $tmp.exe <&$nullin >&$nullout && e= && break ;;

							esac
						else	case $run in
							no[els]*)e=1 && break ;;
							esac
						fi
						case $# in
						0)	case $run in
							no*)	e= ;;
							esac
							break
							;;
						esac
					done
					;;
				esac
				case $run in
				no*)	case $e in
					"")	e=1 ;;
					*)	e= ;;
					esac
					;;
				esac
				case $e in
				"")	success
					case $M in
					*-*)	;;
					*)	usr="$usr$nl#define $m 1"
						echo "#define $m	1	/* "${note:-"$run passed"}" */"
						eval $m=1
						;;
					esac
					user_pf=$pass user_yn=$yes
					;;
				*)	failure
					case $M in
					*-*)	;;
					*)	case $all$config$undef in
						?1?|??1)echo "#undef	$m		/* "${note:-"$run"}" failed */" ;;
						1??)	echo "#define	$m	0	/* "${note:-"$run"}" failed */" ;;
						esac
						eval $m=0
						;;
					esac
					user_pf=$fail user_yn=$no
					;;
				esac
				case $user_pf in
				?*)	(eval "$user_pf") <&$nullin ;;
				esac
				case $user_yn in
				?*)	case $note in
					?*) user_yn="$user_yn	/* $note */" ;;
					esac
					copy - "$user_yn"
					;;
				esac
				continue
				;;
			esac

			# initialize common builtin state

			case $o in
			dat|lib|mth|run)
				case $statictest in
				"")	statictest=FoobaR
					copy $tmp.c "
$std
$usr
$ext
_BEGIN_EXTERNS_
extern int $statictest;
_END_EXTERNS_
main(){char* i = (char*)&$statictest; return i!=0;}
"
					rm -f $tmp.exe
					if	$cc -o $tmp.exe $tmp.c <&$nullin >&$nullout && $executable $tmp.exe
					then	case $static in
						.)	static=
							echo '#include <stdio.h>
main(){printf("hello");return(0);}' > $tmp.c
							rm -f $tmp.exe
							if	$cc -c $tmp.c <&$nullin >&$nullout && $cc -o $tmp.exe $tmp.o <&$nullin >&$nullout 2>$tmp.e && $executable $tmp.exe
							then	e=`wc -l $tmp.e`
								eval set x x $binding
								while	:
								do	shift
									shift
									case $# in
									0)	break ;;
									esac
									rm -f $tmp.exe
									$cc -o $tmp.exe $1 $tmp.o <&$nullin >&$nullout 2>$tmp.e && $executable $tmp.exe || continue
									case `wc -l $tmp.e` in
									$e)	;;
									*)	continue ;;
									esac
									d=`ls -s $tmp.exe`
									rm -f $tmp.exe
									$cc -o $tmp.exe $2 $tmp.o <&$nullin >&$nullout 2>$tmp.e && $executable $tmp.exe || continue
									case `wc -l $tmp.e` in
									$e)	;;
									*)	continue ;;
									esac
									case `ls -s $tmp.exe` in
									$d)	;;
									*)	static=$2
										set x
										shift
										break
										;;
									esac
								done
							fi
							;;
						esac
					else	static=
					fi
					;;
				esac
				;;
			esac

			# builtin tests

			case $o in
			cmd)	case $p in
				?*)	continue ;;
				esac
				k=
				for j in "" usr
				do	case $j in
					"")	d= s= ;;
					*)	d=/$j s=_$j ;;
					esac
					for i in bin etc ucb
					do	if	test -f $d/$i/$a
						then	case $k in
							"")	k=1
								usr="$usr$nl#define $m 1"
								echo "#define $m	1	/* $a in ?(/usr)/(bin|etc|ucb) */"
								;;
							esac
							c=${s}_${i}_${v}
							usr="$usr$nl#define $c 1"
							echo "#define $c	1	/* $d/$i/$a found */"
						fi
					done
				done
				;;
			dat)	case $p in
				?*)	continue ;;
				esac
				{
				copy - "
$std
$usr
$pre
"
				case $inc in
				?*)	echo "$inc"
					;;
				*)	echo "_BEGIN_EXTERNS_
extern int $v;
_END_EXTERNS_"
					;;
				esac
				echo "$ext
#ifdef _DLL
#define _REF_
#else
#define _REF_	&
#endif
main(){char* i = (char*) _REF_ $v; return i!=0;}"
				} > $tmp.c
				is $o $v
				rm -f $tmp.exe
				if	$cc -c $tmp.c <&$nullin >&$nullout &&
					$cc $static -o $tmp.exe $tmp.o $lib $deflib <&$nullin >&$nullout &&
					$executable $tmp.exe
				then	success
					usr="$usr$nl#define $m 1"
					echo "#define $m	1	/* $v in default lib(s) */"
					eval $m=1
				else	failure
					case $all$config$undef in
					?1?|??1)echo "#undef	$m		/* $v not in default lib(s) */" ;;
					1??)	echo "#define	$m	0	/* $v not in default lib(s) */" ;;
					esac
					eval $m=0
				fi
				;;
			dfn)	case $p in
				?*)	continue ;;
				esac
				is dfn $v
				echo "$pre
$inc
#ifdef $v
<<\"#ifndef $v\">>
<<\"#define $v\">>	$v	<<\"/* native $v */\">>
<<\"#endif\">>
#endif" > $tmp.c
				if	$cc -E $tmp.c <&$nullin >$tmp.i
				then	sed -e '/<<[ 	]*".*"[ 	]*>>/!d' -e 's/<<[ 	]*"//g' -e 's/"[ 	]*>>//g' $tmp.i > $tmp.t
					if	test -s $tmp.t
					then	success
						cat $tmp.t
					else	failure
					fi
				else	failure
				fi
				;;
			exp)	case $a in
				-)	;;
				*)	eval x='$'$a
					case $x in
					1)	continue ;;
					esac
					;;
				esac
				case $test in
				\ \"*\"|\ [01])
					case $a in
					-)	;;
					*)	case $note in
						'')	echo "#define $a	$test" ;;
						*)	echo "#define $a	$test	/* $note */" ;;
						esac
						;;
					esac
					;;
				*)	
					case $note in
					'')	note=$test ;;
					*)	note=' '$note ;;
					esac
					is exp "$note"
					x=
					for i in `echo '' $test | sed 's,[^abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_], & ,g'`
					do	case $i in
						[\ \	])
							;;
						[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_]*)
							x="$x \${$i:-0}"
							;;
						'!')	x="$x 0 ="
							;;
						'&'|'|')case $x in
							*"$i")	;;
							*)	x="$x \\$i" ;;
							esac
							;;
						*)	x="$x \\$i"
							;;
						esac
					done
					case `eval expr $x 2>&$stderr` in
					0)	failure
						case $a in
						-)	;;
						*)	case $all$config$undef in
							?1?|??1)echo "#undef	$a		/*$note is false */" ;;
							1??)	echo "#define	$a	0	/*$note is false */" ;;
							esac
							eval $a=0
							;;
						esac
						user_pf=$fail user_yn=$no
						;;
					*)	success
						case $a in
						-)	;;
						*)	usr="$usr$nl#define $a 1"
							echo "#define $a	1	/*$note is true */"
							eval $a=1
							;;
						esac
						user_pf=$pass user_yn=$yes
						;;
					esac
					;;
				esac
				;;
			hdr|lcl|nxt|sys)
				case $o in
				lcl|nxt)p=$o ;;
				esac
				case $p in
				lcl|nxt)eval f='$'_${p}_$v
					case $f in
					?*)	continue ;;
					esac
					eval _${p}_$v=1
					f=$v
					;;
				*)	case $o in
					hdr)	x=$f.h ;;
					sys)	x=sys/$f.h ;;
					esac
					case " $gothdr " in
					*" - $x "*)
						;;
					*" + $x "*)
						;;
					*)	echo "${allinc}#include <$x>" > $tmp.c
						is hdr $x
						if	$cc -E $tmp.c <&$nullin >&$nullout
						then	success
							case " $puthdr " in
							*" $m "*)
								;;
							*)	puthdr="$puthdr $m"
								usr="$usr$nl#define $m 1"
								;;
							esac
							gothdr="$gothdr + $x"
							echo "#define $m	1	/* #include <$x> ok */"
							eval $m=1
						else	failure
							gothdr="$gothdr - $x"
							case $all$config$undef in
							?1?|??1)echo "#undef	$m		/* #include <$x> not ok */" ;;
							1??)	echo "#define	$m	0	/* #include <$x> not ok */" ;;
							esac
							eval $m=0
						fi
						;;
					esac
					continue
					;;
				esac
				x=$f
				case $f in
				*[\\/]*)g=$f ;;
				*)	g="$f sys/$f" ;;
				esac
				is $o $x
				for f in $g
				do	echo "$pre
$inc
#include <$f.h>" > $tmp.c
					case $f in
					sys/*)	e= ;;
					*)	e='-e /[\\\/]sys[\\\/]'$x'\\.h"/d' ;;
					esac
					if	$cc -E $tmp.c <&$nullin >$tmp.i
					then	i=`sed -e '/^#[line 	]*[0123456789][0123456789]*[ 	][ 	]*"[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:]*[\\\\\\/].*[\\\\\\/]'$x'\\.h"/!d' $e -e s'/.*"\\(.*\\)".*/\\1/' -e 's,\\\\,/,g' -e 's,///*,/,g' $tmp.i | sed 1q`
						case $i in
						[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ]:[\\/]*)
							;;
						*/*/*)	k=`echo "$i" | sed 's,.*/\([^/]*/[^/]*\)$,../\1,'`
							echo "$pre
$inc
#include <$k>" > $tmp.c
							if	$cc -E $tmp.c <&$nullin >$tmp.i
							then	j=`sed -e '/^#[line 	]*[0123456789][0123456789]*[ 	][ 	]*"[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:]*[\\\\\\/].*[\\\\\\/]'$x'\\.h"/!d' $e -e s'/.*"\\(.*\\)".*/\\1/' -e 's,\\\\,/,g' -e 's,///*,/,g' $tmp.i | sed 1q`
								wi=`wc < "$i"`
								wj=`wc < "$j"`
								case $wi in
								$wj)	i=$k	;;
								esac
							fi
							;;
						*)	echo "$pre
$inc
#include <../include/$f.h>" > $tmp.c
							if	$cc -E $tmp.c <&$nullin >&$nullout
							then	i=../include/$f.h
							fi
							;;
						esac
					else	i=
					fi
					case $i in
					[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ]:[\\/]*|[\\/]*)
						success
						case $p in
						lcl)	echo "#if defined(__STDPP__directive)"
							echo "__STDPP__directive pragma pp:hosted"
							echo "#endif"
							echo "#include <$i>	/* the native <$f.h> */"
							echo "#undef	$m"
							usr="$usr$nl#define $m 1"
							echo "#define $m	1"
							;;
						nxt)	echo "#define $m <$i>	/* include path for the native <$f.h> */"
							echo "#define ${m}_str \"$i\"	/* include string for the native <$f.h> */"
							usr="$usr$nl#define $m <$i>$nl#define ${m}_str \"$i\""
							eval $m=\\\<$i\\\>
							;;
						esac
						break
						;;
					../*/*)	success
						case $p in
						lcl)	echo "#include <$i>	/* the native <$f.h> */"
							echo "#undef	$m"
							usr="$usr$nl#define $m 1"
							echo "#define $m	1"
							eval $m=1
							;;
						nxt)	echo "#define $m <$i>	/* include path for the native <$f.h> */"
							echo "#define ${m}_str \"$i\"	/* include string for the native <$f.h> */"
							usr="$usr$nl#define $m <$i>$nl#define ${m}_str \"$i\""
							eval $m=\\\<$i\\\>
							;;
						esac
						break
						;;
					*)	failure
						case $p in
						lcl)	case $all$config$undef in
							?1?|??1)echo "#undef	$m		/* no native <$f.h> */" ;;
							1??)	echo "#define	$m	0	/* no native <$f.h> */" ;;
							esac
							eval $m=0
							;;
						nxt)	case $all$config$undef in
							?1?|??1)echo "#undef	$m		/* no include path for the native <$f.h> */" ;;
							esac
							;;
						esac
						;;
					esac
				done
				;;
			iff)	;;
			key)	case $p in
				?*)	continue ;;
				esac
				w=$v
				while	:
				do	is $o $w
					echo "$pre
int f(){int $w = 1;return($w);}" > $tmp.c
					if	$cc -c $tmp.c <&$nullin >&$nullout
					then	failure
						case $set in
						*" ="|*" = "*)
							set x $set
							shift
							w=
							while	:
							do	case $# in
								0)	break ;;
								esac
								case $1 in
								=)	break ;;
								esac
								w="$w $1"
								shift
							done
							case $1 in
							=)	shift
								set=" $*"
								;;
							*)	set=
								;;
							esac
							continue
							;;
						esac
						case $all$config$undef in
						?1?|??1)echo "#undef	$m		/* $w is not a reserved keyword */" ;;
						1??)	echo "#define	$m	0	/* $w is not a reserved keyword */" ;;
						esac
						eval $m=0
						case $set in
						?*)	echo "#define	$v	$set	/* default for reserved keyword $v */" ;;
						esac
					else	success
						usr="$usr$nl#define $m 1"
						echo "#define $m	1	/* $v is a reserved keyword */"
						eval $m=1
						case $w in
						$v)	;;
						*)	echo "#define	$v	$w	/* alternate for reserved keyword $v */" ;;
						esac
					fi
					break
				done
				;;
			lib|mth)case $p in
				?*)	continue ;;
				esac
				case $v in
				-)	continue ;;
				esac
				is $o $v
				copy $tmp.c "
$std
$usr
$pre
$inc
$ext
typedef int (*_IFFE_fun)();
#ifdef _IFFE_extern
_BEGIN_EXTERNS_
extern int $v();
_END_EXTERNS_
#endif
static _IFFE_fun i=(_IFFE_fun)$v;int main(){return ((unsigned int)i)^0xaaaa;}
"
				if	$cc -D_IFFE_extern -c $tmp.c <&$nullin >&$nullout || $cc -c $tmp.c <&$nullin >&$nullout
				then	rm -f $tmp.exe
					if	$cc $static -o $tmp.exe $tmp.o $lib $deflib <&$nullin >&$nullout && $executable $tmp.exe
					then	case $o in
						lib)	success
							usr="$usr$nl#define $m 1"
							echo "#define $m	1	/* $v() in default lib(s) */"
							eval $m=1
							;;
						*)	failure
							case $all$config$undef in
							?1?|??1)echo "#undef	$m		/* $v() not in math lib */" ;;
							1??)	echo "#define	$m	0	/* $v() not in math lib */" ;;
							esac
							eval $m=0
							case $set in
							?*)	echo "#define	$v	$set	/* default for function $v() */" ;;
							esac
							;;
						esac
					else	case $o in
						mth)	rm -f $tmp.exe
							if	$cc $static -o $tmp.exe $tmp.o -lm <&$nullin >&$nullout && $executable $tmp.exe
							then	success
								usr="$usr$nl#define $m 1"
								echo "#define $m	1	/* $v() in math lib */"
								eval $m=1
							else	failure
								case $all$config$undef in
								?1?|??1)echo "#undef	$m		/* $v() not in math lib */" ;;
								1??)	echo "#define	$m	0	/* $v() not in math lib */" ;;
								esac
								eval $m=0
								case $set in
								?*)	echo "#define	$v	$set	/* default for function $v() */" ;;
								esac
							fi
							;;
						*)	failure
							case $all$config$undef in
							?1?|??1)echo "#undef	$m		/* $v() not in default lib(s) */" ;;
							1??)	echo "#define	$m	0	/* $v() not in default lib(s) */" ;;
							esac
							eval $m=0
							case $set in
							?*)	echo "#define	$v	$set	/* default for function $v() */" ;;
							esac
							;;
						esac
					fi
				else	case $intrinsic in
					'')	copy $tmp.c "
$std
$usr
$pre
$inc
$ext
_BEGIN_EXTERNS_
extern int foo();
_END_EXTERNS_
static int ((*i)())=foo;main(){return(i==0);}
"
						if	$cc -c $tmp.c <&$nullin >&$nullout
						then	intrinsic=1
						else	intrinsic=0
						fi
						;;
					esac
					case $intrinsic in
					0)	failure
						case $o in
						mth)	case $all$config$undef in
							?1?|??1)echo "#undef	$m		/* $v() not in math lib */" ;;
							1??)	echo "#define	$m	0	/* $v() not in math lib */" ;;
							esac
							eval $m=0
							;;
						*)	case $all$config$undef in
							?1?|??1)echo "#undef	$m		/* $v() not in default lib(s) */" ;;
							1??)	echo "#define	$m	0	/* $v() not in default lib(s) */" ;;
							esac
							eval $m=0
							;;
						esac
						case $set in
						?*)	echo "#define	$v	$set	/* default for function $v() */" ;;
						esac
						;;
					1)	success
						case $o in
						lib)	usr="$usr$nl#define $m 1"
							echo "#define $m	1	/* $v() in default lib(s) */"
							;;
						mth)	usr="$usr$nl#define $m 1"
							echo "#define $m	1	/* $v() in math lib */"
							;;
						esac
						;;
					esac
				fi
				;;
			mac)	case $p in
				?*)	continue ;;
				esac
				is mac $v
				echo "$pre
$inc
#ifdef $v
'#define $m	1	/* $v is a macro */'
#endif" > $tmp.c
				if	$cc -E $tmp.c <&$nullin | sed -e "/#define/!d" -e "s/'//g" -e "s/^[ 	][ 	]*//"
				then	success
					usr="$usr$nl#define $m 1"
				else	failure
					case $all$config$undef in
					?1?|??1)echo "#undef	$m	0 /* $v is not a macro */" ;;
					1??)	echo "#define	$m	0 /* $v is not a macro */" ;;
					esac
					eval $m=0
				fi
				;;
			mem)	case $p in
				?*)	eval i='$'_iffe_typedef_$p
					case $i in
					0|1)	;;
					*)	echo "$pre
$inc
static $p i;
int n = sizeof(i);" > $tmp.c
						is typ $p
						if	$cc -c $tmp.c <&$nullin >&$nullout
						then	success
							eval _iffe_typedef_$p=1
							i=1
						else	failure
							eval _iffe_typedef_$p=0
							i=0
						fi
						;;
					esac
					case $i in
					0)	p="struct $p" ;;
					esac
					is mem $v "$p"
					echo "$pre
$inc
static $p i;
int n = sizeof(i.$v);" > $tmp.c
					if	$cc -c $tmp.c <&$nullin >&$nullout
					then	success
						usr="$usr$nl#define $m 1"
						echo "#define $m	1	/* $v is a member of $p */"
						eval $m=1
					else	failure
						case $all$config$undef$i in
						?1?0|??10)echo "#undef	$m		/* $v is not a member of $p */" ;;
						1??0)	echo "#define	$m	0	/* $v is not a member of $p */" ;;
						esac
						eval $m=0
					fi
					;;
				*)	p=$v
					eval i='$'_iffe_typedef_$p
					case $i in
					0|1)	;;
					*)	echo "$pre
$inc
static $p i;
int n = sizeof(i);" > $tmp.c
						is typ $p
						if	$cc -c $tmp.c <&$nullin >&$nullout
						then	success
							eval _iffe_typedef_$p=1
							i=1
						else	failure
							eval _iffe_typedef_$p=0
							i=0
						fi
						;;
					esac
					case $i in
					0)	p="struct $p" ;;
					esac
					is nos "$p"
					echo "$pre
$inc
static $p i;
int n = sizeof(i);" > $tmp.c
					if	$cc -c $tmp.c <&$nullin >&$nullout
					then	echo "$pre
$inc
static $p i;
unsigned long f() { return (unsigned long)i; }" > $tmp.c
						if	$cc -c $tmp.c <&$nullin >&$nullout
						then	:
						else	success
							usr="$usr$nl#define $m 1"
							echo "#define $m	1	/* $p is a non-opaque struct */"
							continue
						fi
					fi
					failure
					case $all$config$undef$i in
					?1?0|??10)echo "#undef	$m		/* $p is not a non-opaque struct */" ;;
					1??0)	echo "#define	$m	0	/* $p is not a non-opaque struct */" ;;
					esac
					eval $m=0
					;;
				esac
				;;
			nop)	;;
			npt)	is npt $v
				copy $tmp.c "
$std
$usr
$pre
$inc
$ext
_BEGIN_EXTERNS_
struct _iffe_struct { int _iffe_member; };
extern struct _iffe_struct* $v _ARG_((struct _iffe_struct*));
_END_EXTERNS_
"
				if	$cc -c $tmp.c <&$nullin >&$nullout
				then	success
					case $config in
					1)	echo "#undef	$m		/* $v() needs a prototype */"
						eval $m=0
						;;
					*)	usr="$usr$nl#define $m 1"
						echo "#define $m	1	/* $v() needs a prototype */"
						eval $m=1
						;;
					esac
				else	failure
					case $config in
					1)	usr="$usr$nl#define $m 1"
						echo "#define	$m	1	/* $v() does not need a prototype */"
						eval $m=1
						;;
					*)	case $all$undef in
						?1)	echo "#undef	$m		/* $v() does not need a prototype */" ;;
						1?)	echo "#define	$m	0	/* $v() does not need a prototype */" ;;
						esac
						eval $m=0
						;;
					esac
				fi
				;;
			num)	is num $v
				copy $tmp.c "
$std
$usr
$pre
$inc
$ext
_BEGIN_EXTERNS_
int _iffe_int = $v / 2;
_END_EXTERNS_
"
				if	$cc -c $tmp.c <&$nullin >&$nullout
				then	success
					usr="$usr$nl#define $m 1"
					echo "#define $m	1	/* $v is a numeric constant */"
					eval $m=1
				else	failure
					case $all$config$undef in
					?1?|??1)echo "#undef	$m		/* $v is not a numeric constant */" ;;
					1??)	echo "#define	$m	0	/* $v is not a numeric constant */" ;;
					esac
					eval $m=0
				fi
				;;
			one)	for i in $a $hdr
				do	x="#include <$i>"
					case " $gothdr " in
					*" - $i "*)
						continue
						;;
					*" + $i "*)
						;;
					*)	is hdr $x
						echo "$x" > $tmp.c
						if	$cc -E $tmp.c <&$nullin >&$nullout
						then	success
							gothdr="$gothdr + $x"
						else	failure
							gothdr="$gothdr - $x"
							continue
						fi
						;;
					esac
					case $one in
					"")	one=$x
						;;
					*"$x"*)	break
						;;
					*)	echo "$one" > $tmp.c
						if	$cc -E $tmp.c <&$nullin >$tmp.i
						then	c=$i
							case $c in
							*[\\/]*)	c=`echo $c | sed 's,[\\\\\\/],[\\\\\\/],g'` ;;
							esac
							case `sed -e '/^#[line 	]*1[ 	][ 	]*"[\\\\\\/].*[\\\\\\/]'$c'"/!d' $tmp.i` in
							?*)	break ;;
							esac
						fi
						one="$one$nl$x"
						;;
					esac
					echo "$x"
					break
				done
				;;
			out|output)
				;;
			pth)	is pth $a
				pkg $pth
				tab="  "
				e=
				f=
				for i in $pth
				do	case $i in
					'{')	e="${nl}}"
						l=
						x=i
						v="\$${x}"
						t=${nl}${tab}
						b="fnd()${nl}{${t}for ${x} in"
						;;
					'}')	b="${b}${t}do${tab}if test -e ${v}/\${1}${t}${tab}${tab}then${tab}f=${v}/\${1}${t}${tab}${tab}${tab}return${t}${tab}${tab}fi"
						e="${t}done${e}"
						eval "${b}${e}"
						fnd $a
						case $f in
						?*)	break ;;
						esac
						;;
					-)	b="${b}${t}do${tab}test \"${v}\" = '' -o -d \"${v}\" &&${t}${tab}${tab}"
						x=${x}i
						v="${v}\$${x}"
						b="${b}for ${x} in"
						e="${t}done${e}"
						t="${t}${tab}${tab}"
						;;
					*)	case $e in
						'')	if	test -e ${i}/${a}
							then	f=${i}/${a}
								break
							fi
							;;
						*)	case $i in
							/|.)	b="${b} ''" ;;
							*)	b="${b} /${i}" ;;
							esac
							;;
						esac
						;;
					esac
				done
				case $f in
				'')	case $set in
					' ')	f=$a ;;
					?*)	f=$set ;;
					esac
					;;
				esac
				case $f in
				'')	failure
					case $all$config$undef in
					?1?|??1)echo "#undef	$m		/* $a path not found */" ;;
					esac
					;;
				?*)	success
					echo "#define $m	\"$f\"	/* ${note:-$a path} */"
					eval $m=\\\"$f\\\"
					;;
				esac
				;;
			run)	is run $a
				if	test ! -r $a
				then	failure not found
					case $verbose in
					0)	echo "$command: $a: not found" >&$stderr ;;
					esac
					exit 1
				fi
				noisy
				case $a in
				*.c)	rm -f $tmp.exe
					cp $a $tmp.c
					$cc -o $tmp.exe $tmp.c $lib $deflib <&$nullin >&$stderr 2>&$stderr &&
					$executable $tmp.exe &&
					execute $tmp.exe $opt <&$nullin
					;;
				*.sh)	{
					cat <<!
:
set "cc='$cc' executable='$executable' id='$m' static='$static' tmp='$tmp'" $opt $hdr $test
!
					cat $a
					} > $tmp.sh
					chmod +x $tmp.sh
					( . $tmp.sh ) <&$nullin
					;;
				*)	false
					;;
				esac
				case $? in
				0)	success
					;;
				*)	failure cannot run
					case $verbose in
					0)	echo "$command: $a: cannot run" >&$stderr ;;
					esac
					exit 1
					;;
				esac
				;;
			siz)	case $p in
				"")	x= ;;
				*)	x="$p " ;;
				esac
				is siz "$x$v"
				{
				case $p:$v in
				long:*|*:*[_0123456789]int[_0123456789]*)
					echo "$pre
$inc
#include <stdio.h>
static $x$v i;
$x$v f() {
$x$v v; i = 1; v = i;"
					echo "i = v * i; i = i / v; v = v + i; i = i - v;"
					case $v in
					float|double) ;;
					*)	echo "v <<= 4; i = v >> 2; i = 10; i = v % i; i |= v; v ^= i; i = 123; v &= i;" ;;
					esac
					echo "return v; }"
					;;
				*)	echo "$pre
$inc
struct xxx { $x$v mem; };
static struct xxx v;
struct xxx* f() { return &v; }"
					;;
				esac
				case $x in
				""|"struct "|"union ")
					echo "int g() { return 0; }"
					;;
				*)	echo "int g() { return sizeof($x$v)<=sizeof($v); }" ;;
				esac
				copy - "
main() {
	f();
	g();
	printf(\"%u\\n\", sizeof($x$v));
	return 0;
}"
				} > $tmp.c
				rm -f $tmp.exe $tmp.dat
				if	$cc -o $tmp.exe $tmp.c $lib $deflib <&$nullin >&$nullout &&
					$executable $tmp.exe &&
					execute $tmp.exe > $tmp.dat
				then	success
					z=`cat $tmp.dat`
					usr="$usr$nl#define $m $z"
					echo "#define $m	$z	/* sizeof($x$v) */"
					eval $m=1
					user_pf=$pass user_yn=$yes
				else	failure
					case $all$config$undef in
					?1?|??1)echo "#undef	$m		/* $x$v not a type with known size */" ;;
					1??)	echo "#define	$m	0	/* $x$v not a type with known size */" ;;
					esac
					eval $m=0
					user_pf=$fail user_yn=$no
				fi
				;;
			sym)	case $test in
				"")	x=$v ;;
				*)	x=$test ;;
				esac
				echo "$pre
$inc
'=' $x '='" > $tmp.c
				$cc -E $tmp.c <&$nullin \
				| sed \
					-e "/'='/!d" \
					-e "s/'='//g" \
					-e 's/[ 	]//g' \
					-e 's/((([^()]*)))->/->/g' \
					-e 's/(([^()]*))->/->/g' \
					-e 's/([^()]*)->/->/g' \
					-e 's/\([abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_][abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789]*\)\[/\
ary \1[/g' \
					-e 's/\([abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_][abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789]*\)(/\
fun \1[/g' \
					-e 's/\*->\([abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_]\)/->\
ptr \1/g' \
					-e 's/->\([abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_]\)/->\
reg \1/g' \
					-e "/^$v\$/d" \
					-e 's/^[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_][abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789]*$/\
nam &/g' \
				| sed \
					-e '/^... /!d' \
				| sort \
					-u \
				| sed \
					-e 's/\(...\) \([abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_][abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789]*\).*/#ifndef _\1_'$v'\
#define _\1_'$v' \2\
#define _\1_'$v'_str "\2"\
#endif/'
				;;
			typ)	case $p in
				"")	x= ;;
				*)	x="$p " ;;
				esac
				is typ "$x$v"
				{
				case $p:$v in
				long:*|*:*[_0123456789]int[_0123456789]*)
					echo "$pre
$inc
static $x$v i;
$x$v f() {
$x$v v; i = 1; v = i;"
					echo "i = v * i; i = i / v; v = v + i; i = i - v;"
					case $v in
					float|double) ;;
					*)	echo "v <<= 4; i = v >> 2; i = 10; i = v % i; i |= v; v ^= i; i = 123; v &= i;" ;;
					esac
					echo "return v; }"
					;;
				*)	echo "$pre
$inc
struct xxx { $x$v mem; };
static struct xxx v;
struct xxx* f() { return &v; }"
					;;
				esac
				case $x in
				""|"struct "|"union ")
					echo "main() { f(); return 0; }" ;;
				*)	echo "main() { f(); return sizeof($x$v)<=sizeof($v); }" ;;
				esac
				} > $tmp.c
				rm -f $tmp.exe
				if	$cc -o $tmp.exe $tmp.c $lib $deflib <&$nullin >&$nullout &&
					$executable $tmp.exe &&
					execute $tmp.exe
				then	success
					usr="$usr$nl#define $m 1"
					echo "#define $m	1	/* $x$v is a type */"
					eval $m=1
					user_pf=$pass user_yn=$yes
				else	failure
					case $all$config$undef in
					?1?|??1)echo "#undef	$m		/* $x$v is not a type */" ;;
					1??)	echo "#define	$m	0	/* $x$v is not a type */" ;;
					esac
					eval $m=0
					case $set in
					?*)	echo "#define	$v	$set	/* default for type $x$v */" ;;
					esac
					user_pf=$fail user_yn=$no
				fi
				;;
			val)	case $arg in
				\"*\")	echo $arg=\'$val\' ;;
				*)	echo $arg=\"$val\" ;;
				esac
				;;
			:)	shift
				;;
			*)	echo "$command: $o: unknown feature test" >&$stderr
				status=1
				;;
			esac
			case $user_pf in
			?*)	(eval "$user_pf") <&$nullin ;;
			esac
			case $user_yn in
			?*)	case $note in
				?*) user_yn="$user_yn	/* $note */" ;;
				esac
				copy - "$user_yn"
				;;
			esac
		done
	done
done
