#
# globrecur.tcl --
#
#  Build or process a directory list recursively.
#------------------------------------------------------------------------------
# Copyright 1992-1994 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: globrecur.tcl,v 3.0 1993/11/19 07:00:29 markd Rel markd $
#------------------------------------------------------------------------------
#

#@package: TclX-globrecur recursive_glob

proc recursive_glob {dirlist globlist} {
    set result {}
    set recurse {}
    foreach dir $dirlist {
        if ![file isdirectory $dir] {
            error "\"$dir\" is not a directory"
        }
        foreach pattern $globlist {
            set result [concat $result [glob -nocomplain -- $dir/$pattern]]
        }
        foreach file [glob -nocomplain -- $dir/* $dir/.*] {
            if [file isdirectory $file] {
                set fileTail [file tail $file]
                if {!(($fileTail == ".") || ($fileTail == ".."))} {
                    lappend recurse $file
                }
            }
        }
    }
    if ![lempty $recurse] {
        set result [concat $result [recursive_glob $recurse $globlist]]
    }
    return $result
}

#@package: TclX-forrecur for_recursive_glob

proc for_recursive_glob {var dirlist globlist code {depth 1}} {
    upvar $depth $var myVar
    set recurse {}
    foreach dir $dirlist {
        if ![file isdirectory $dir] {
            error "\"$dir\" is not a directory"
        }
        foreach pattern $globlist {
            foreach file [glob -nocomplain -- $dir/$pattern] {
                set myVar $file
                uplevel $depth $code
            }
        }
        foreach file [glob -nocomplain -- $dir/* $dir/.*] {
            if [file isdirectory $file] {
                set fileTail [file tail $file]
                if {!(($fileTail == ".") || ($fileTail == ".."))} {
                    lappend recurse $file
                }
            }
        }
    }
    if ![lempty $recurse] {
        for_recursive_glob $var $recurse $globlist $code [expr {$depth + 1}]
    }
    return {}
}
