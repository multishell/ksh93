########################################################################
#                                                                      #
#               This software is part of the ast package               #
#           Copyright (c) 1982-2006 AT&T Knowledge Ventures            #
#                      and is licensed under the                       #
#                  Common Public License, Version 1.0                  #
#                      by AT&T Knowledge Ventures                      #
#                                                                      #
#                A copy of the License is available at                 #
#            http://www.opensource.org/licenses/cpl1.0.txt             #
#         (with md5 checksum 059e8cd6165cb4c31e351f2b69388fd9)         #
#                                                                      #
#              Information and Software Systems Research               #
#                            AT&T Research                             #
#                           Florham Park NJ                            #
#                                                                      #
#                  David Korn <dgk@research.att.com>                   #
#                                                                      #
########################################################################
function err_exit
{
	print -u2 -n "\t"
	print -u2 -r ${Command}[$1]: "${@:2}"
	let Errors+=1
}
alias err_exit='err_exit $LINENO'

function abspath
{
        base=$(basename $SHELL)
        cd ${SHELL%/$base}
        newdir=$(pwd)
        cd ~-
        print $newdir/$base
}
#test for proper exit of shell
Command=$0
integer Errors=0
ABSHELL=$(abspath)
mkdir /tmp/ksh$$ || err_exit "mkdir /tmp/ksh$$ failed"
cd /tmp/ksh$$ || err_exit "cd /tmp/ksh$$ failed"
print exit 0 >.profile
${ABSHELL}  <<!
HOME=$PWD \
LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
LD_LIBRARYN32_PATH=$LD_LIBRARYN32_PATH \
LD_LIBRARY64_PATH=$LD_LIBRARY64_PATH \
LIBPATH=$LIBPATH \
PATH=$PATH \
SHELL=$ABSSHELL \
SHLIBPATH=$SHLIBPATH \
exec -c -a -ksh ${ABSHELL} -c "exit 1" 1>/dev/null 2>&1
!
if [[ $(echo $?) != 0 ]]
then err_exit 'exit in .profile is ignored'
fi
if	[[ $(trap 'code=$?; echo $code; trap 0; exit $code' 0; exit 123) != 123 ]]
then	err_exit 'exit not setting $?'
fi
cat > run.sh <<- "EOF"
	trap 'code=$?; echo $code; trap 0; exit $code' 0
	( trap 0; exit 123 )
EOF
if	[[ $($SHELL ./run.sh) != 123 ]]
then	err_exit 'subshell trap on exit overwrites parent trap'
fi
cd ~- || err_exit "cd back failed"
rm -r /tmp/ksh$$ || err_exit "rm -r /tmp/ksh$$ failed"
exit $((Errors))
