#!/bin/sh
#
# symlinkext.sh --
#
# Script for symbolicly linking external files.
#
# Arguments:
#   $1-n - directory to build the symbolic links in.
#   $m - full paths of files to symbolic link.
#------------------------------------------------------------------------------
# Copyright 1993-1995 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: symlinkext.sh,v 4.2 1995/03/13 06:14:17 markd Exp markd $
#------------------------------------------------------------------------------
#

if [ $# -lt 2 ]
then
    echo usage: $0 path1 [...] targetdir
    exit 1
fi

while [ $# -gt 1 ]
do
    paths="$paths $1"
    shift
done
targetdir=$1

test -d $targetdir || mkdir $targetdir

for fn in $paths ; do
    target=$targetdir/`basename $fn`
    rm -f $target
    ln -s $fn $target
done
