#
# symlinkext.sh --
#
# Script for symbolicly linking external files.
#
# Arguments:
#   $1-n - directory to build the symbolic links in.
#   $m - full paths of files to symbolic link.
#------------------------------------------------------------------------------
# Copyright 1993-1994 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: cphelpdir.sh,v 3.1 1994/05/28 03:38:22 markd Exp $
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
