#
# loadouster.tcl --
#
# Procedure to load a Ousterhout index when its encountered.
#------------------------------------------------------------------------------
# Copyright 1992-1993 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------
#

proc auto_load_ouster_index f {
    global auto_index
    set dir [file dirname $f]
    source $f
}
