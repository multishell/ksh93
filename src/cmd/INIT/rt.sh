########################################################################
#                                                                      #
#               This software is part of the ast package               #
#                  Copyright (c) 1994-2006 AT&T Corp.                  #
#                      and is licensed under the                       #
#                  Common Public License, Version 1.0                  #
#                            by AT&T Corp.                             #
#                                                                      #
#                A copy of the License is available at                 #
#            http://www.opensource.org/licenses/cpl1.0.txt             #
#         (with md5 checksum 059e8cd6165cb4c31e351f2b69388fd9)         #
#                                                                      #
#              Information and Software Systems Research               #
#                            AT&T Research                             #
#                           Florham Park NJ                            #
#                                                                      #
#                 Glenn Fowler <gsf@research.att.com>                  #
#                                                                      #
########################################################################
: rt - nmake test output filter

command=rt
flags='--silent --keepgoing'
verbose=0

case `(getopts '[-][123:xyz]' opt --xyz; echo 0$opt) 2>/dev/null` in
0123)	ARGV0="-a $command"
	USAGE=$'
[-?
@(#)$Id: rt (AT&T Research) 2005-04-15 $
]
'$USAGE_LICENSE$'
[+NAME?rt - run "nmake test" and filter output]
[+DESCRIPTION?\brt\b runs \vnmake test\v and filters the regression
	test output to contain only test summary lines. If no \atest\a
	operands are specified then \btest\b is assumed. If \b-\b is
	specified then the \afile\a operands, or the standard input
	if no \afile\a operands are specified, are filtered instead
	of the output from \bnmake\b.]
[v:verbose?Run with \vREGRESSFLAGS=-v\v.]

[ test ... | - [ file ... ] ]

[+SEE ALSO?\bnmake\b(1), \bregress\b(1)]
[+CAVEATS?\brt\b guesses the regression test output style. Garbled
	output indicates a bad guess.]
'
	;;
*)	ARGV0=""
	USAGE="fv"
	;;
esac

function usage
{
	OPTIND=0
	getopts $ARGV0 "$USAGE" OPT '-?'
	exit 2
}

while	getopts $ARGV0 "$USAGE" OPT
do	case $OPT in
	v)	(( verbose=$OPTARG )) && flags="$flags REGRESSFLAGS=-v" ;;
	esac
done
shift `expr $OPTIND - 1`

ifs=${IFS:-$' \t\n'}
set -o noglob
component=
dots='............................................'
bad=' ***'
style=unknown
integer tests errors lineno=0 skip=0
typeset -l lower

function results # tests errors
{
	typeset label note
	if	[[ $style != unknown ]] && (( errors >= 0 ))
	then	if	(( $1 >= 0 ))
		then	if	(( $1 == 1))
			then	label="test "
			else	label=tests
			fi
			printf $'%s%5d %s' "$prefix" "$1" "$label"
			prefix=
		else	prefix="$prefix..........."
		fi
		if	(( $2 == 1))
		then	label=error
		else	label=errors
		fi
		if	(( $2 == 1 ))
		then	note=" $bad"
		elif	(( $2 > 1 ))
		then	note=$bad
		fi
		printf $'%s%5d %s%s\n' "$prefix" "$2" "$label" "$note"
		style=unknown
	fi
}

function unit
{
	typeset x
	if	[[ $component ]]
	then	x=${component##*/}
		if	[[ " $x " != *' '$unit' '* && " $unit " != *' '$x' '* ]]
		then	if	[[ $component == cmd/?*lib/* ]]
			then	unit="$unit $x"
			else	unit="$x $unit"
			fi
		fi
	fi
	unit="$unit ${dots:1:${#dots}-${#unit}}"
	if	[[ $1 ]]
	then	unit="$unit..........."
	fi
	print -r -n -- "$unit"
}

if	[[ $1 == - ]]
then	shift
	cat "$@"
else	(( $# )) || set test
	nmake "$@" $flags 2>&1
fi |
while	read -r line
do	set '' $line
	shift
	case $line in
	TEST[' 	']*', '*' error'*)
		IFS=${IFS}","
		set '' $line
		IFS=$ifs
		set '' $*
		while	:
		do	case $2 in
			'')	break
				;;
			error|errors)
				errors=$1
				break
				;;
			test|tests)
				tests=$1
				;;
			esac
			shift
		done
		results $tests $errors
		continue
		;;
	TEST[' 	']*)
		results $tests $errors
		IFS=${IFS}","
		set '' $line
		IFS=$ifs
		set '' $*
		unit=${3##*/}
		case $4 in
		[a-zA-Z]*)	unit="$unit $4" ;;
		esac
		unit
		prefix=
		errors=0
		style=regress
		continue
		;;
	'pathname and options of item under test')
		read -r line || break
		results $tests $errors
		set '' $line
		unit=${2##*/}
		unit
		tests=0
		errors=0
		style=script
		continue
		;;
	'test '*' begins at '*' '*' '*' '*' '*)
		results $tests $errors
		unit=${2##*/}
		unit=${unit%.sh}
		unit
		prefix=
		tests=-1
		errors=0
		style=shell
		continue
		;;
	'test '*' at '*' '*' '*' '*' '*)
		case $line in
		*' [ '*test*error*' ]')
			while	:
			do	case $1 in
				'[')	tests=$2
					errors=$4
					break
					;;
				esac
				shift
			done
			;;
		*)	if	[[ $3 != passed ]]
			then	(( errors )) || (( errors++ ))
			fi
			;;
		esac
		results $tests $errors
		continue
		;;
	'## ---'*(-)'--- ##')
		(( ++lineno > skip )) || continue
		read -r line || break
		lower=$line
		set '' $lower
		case $lower in
		'##'*'test suite:'*'##')
			results $tests $errors
			set -- ${lower//*suite:}
			set -- ${*//[.#]/}
			unit=$*
			if	[[ $unit == *' tests' ]]
			then	unit=${unit/' tests'/}
			fi
			main=$unit
			prefix=
			tests=0
			errors=0
			category=
			style=autotest
			(( skip = lineno + 1 ))
			unit
			continue
			;;
		esac
		;;
	+(-))	case $style in
		regress)	continue ;;
		esac
		(( ++lineno > skip )) || continue
		read -r line || break
		set '' $line
		case $line in
		'Running tests for '*)
			results $tests $errors
			shift 4
			unit=
			while	(( $# ))
			do	if	[[ $1 == on ]]
				then	break
				fi
				if	[[ $unit ]]
				then	unit="$unit "
				fi
				unit=$unit${1##*/}
				shift
			done
			main=$unit
			prefix=
			tests=-1
			errors=-1
			category=
			style=perl
			(( skip = lineno + 1 ))
			continue
			;;
		*' : '*)results $tests $errors
			unit=${2##*/}
			unit=${unit%.sh}
			unit
			prefix=
			tests=0
			errors=0
			style=timing
			(( skip = lineno + 1 ))
			continue
			;;
		esac
		;;
	+([0-9])*([a-zA-Z0-9])' '*)
		case $style in
		script)	case $line in
			*FAILED*|*failed*)
				(( errors++ ))
				;;
			*)	(( tests++ ))
				;;
			esac
			;;
		esac
		;;
	make:*|'make ['*']:'*)
		case $line in
		*': warning:'*|*'making test'*|*'action'?(s)' failed'*|*': *** '*)
			;;
		*)	results $tests $errors
			print -r -u2 -- "$line"
			;;
		esac
		continue
		;;
	+([/a-zA-Z_0-9]):)
		component=${line%:}
		;;
	'')	continue
		;;
	esac
	case $style in
	autotest)
		case $line in
		+([0-9]):*ok)
			(( tests++ ))
			;;
		+([0-9]):*FAILED*)
			(( tests++ ))
			(( errors++ ))
			if	(( $verbose ))
			then	if	[[ ! $prefix ]]
				then	prefix=$unit
					print
				fi
				print -r -- "	${line//*'FAILED '/}"
			fi
			;;
		esac
		continue
		;;
	perl)	case $line in
		*'........ '*)
			if	[[ $1 == */* ]]
			then	cat=${1%%/*}
				if	[[ $cat != $category ]]
				then	results $tests $errors
					category=$cat
					unit="$main $category"
					unit
					prefix=
					tests=0
					errors=0
				fi
				(( tests++ ))
				case $line in
				*' ok')	;;
				*)	(( errors++ ))
					if	(( $verbose ))
					then	if	[[ ! $prefix ]]
						then	prefix=$unit
							print
						fi
						print -r -- "$line"
					fi
					;;
				esac
			else	results $tests $errors
				case $line in
				*' ok')	errors=0 ;;
				*)	errors=1 ;;
				esac
				unit="$main $1"
				unit
				if	(( $verbose && errors ))
				then	prefix=$unit
					print
					shift 2
					print -r -- "$@"
				else	prefix=
				fi
				results $tests $errors
				tests=-1
				errors=-1
				category=
			fi
			style=perl
			;;
		esac
		continue
		;;
	esac
	case $line in
	*FAILED*|*failed*)
		(( errors++ ))
		;;
	*)	case $style in
		regress)case $line in
			['<>']*);;
			*)	continue ;;
			esac
			;;
		script)	continue
			;;
		shell)	((errors++ ))
			;;
		timing)	(( tests++ ))
			continue
			;;
		unknown)continue
			;;
		esac
		;;
	esac
	if	(( $verbose ))
	then	if	[[ ! $prefix ]]
		then	prefix=$unit
			print
		fi
		print -r -- "$line"
	fi
done
results $tests $errors