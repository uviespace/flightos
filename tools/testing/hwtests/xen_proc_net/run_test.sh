#!/bin/bash
# grmon must be in path!

srcdir=../../../../

cp config ${srcdir}/.config || exit 1

cd ${srcdir} || exit 1

make clean || exit 1

make || exit 1


timeout 120 bash -c \
	"printf \"load flightos\nrun\nquit\" |\
	 grmon -leon2 -u -uart /dev/ttyS0  |\
       	 tee /dev/tty  			   |\
       	 grep SUCCESS &> /dev/null"
ret=$?

# if we timed out, adjust exit code for git bisect
if [ $ret -eq 124 ]; then
	exit 125
fi
	
exit $ret
