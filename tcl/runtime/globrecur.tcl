#
# globrecur.tcl --
#
#  Build up a directory list recursively.
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
# $Id: globrecur.tcl,v 2.1 1993/04/07 02:42:32 markd Exp markd $
#------------------------------------------------------------------------------
#

#@package: TclX-globrecur recursive_glob

proc recursive_glob {globlist} {
    set result ""
    foreach pattern $globlist {
        foreach file [glob -nocomplain $pattern] {
            lappend result $file
            if {[file type $file] == "directory"} {
                set result [concat $result [recursive_glob $file/*]]
            }
        }
    }
    return $result
}
