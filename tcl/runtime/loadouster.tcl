#
# loadouster.tcl --
#
# Procedure to load a Ousterhout index when its encountered.
#------------------------------------------------------------------------------
# Copyright 1992-1996 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: loadouster.tcl,v 7.1 1996/07/07 02:31:56 markd Exp $
#------------------------------------------------------------------------------
#

proc auto_load_ouster_index fn {
    global auto_index
    set dir [file dirname $fn]

    if [catch {set f [open $dir/tclIndex]}] {
        return
    }
    set error [catch {
        set id [gets $f]
        if {($id == {# Tcl autoload index file, version 2.0}) ||
            ($id == {# Tcl autoload index file, version 2.0 for [incr Tcl]})} {
            eval [read $f]
        } elseif {$id == "# Tcl autoload index file: each line identifies a Tcl"} {
            while {[gets $f line] >= 0} {
                if {([string index $line 0] == "#")
                        || ([llength $line] != 2)} {
                    continue
                }
                set name [lindex $line 0]
                if {![info exists auto_index($name)]} {
                    set auto_index($name) "source $dir/[lindex $line 1]"
                }
            }
        } else {
            error "$dir/tclIndex isn't a proper Tcl index file"
        }
    } msg]
    if {$f != ""} {
        close $f
    }
    if $error {
        global errorInfo errorCode
        error $msg $errorInfo $errorCode
    }
}
