#!/bin/bash
# grmon must be in path!


#build application
make || exit 1


mydir=$PWD
srcdir=../../../../

#cp config ${srcdir}/.config || exit 1
cp config-gr740 ${srcdir}/.config || exit 1

echo -e "CONFIG_EMBED_APPLICATION=\"$PWD/executable_demo\"" >> ${srcdir}/.config

cd ${srcdir} || exit 1

#make clean || exit 1

make || exit 1

#grmon -ftdi -u -ftdifreq 0 -jtagcable 3 -nb -c $mydir/grmon_cmds
