#
# tkdemo --
#
# Run the Tk demo at anytime after Extended Tcl is installed.
# 
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
# $Id: tkdemo.tcl,v 1.3 1993/04/07 02:42:32 markd Exp markd $
#------------------------------------------------------------------------------
#

#@package: Tk-demo tkdemo

proc tkdemo {} {
    global auto_path
    if {[info commands tkwait] == ""} {
        error "tkdemo may only be used from wishx"
    }
    set demos [searchpath $auto_path demos]
    if {$demos == "" || ![file isdirectory $demos]} {
        error "can't find Tk `demos' directory on the auto_path (auto_path):
    }
    uplevel #0 source $demos/widget
}


