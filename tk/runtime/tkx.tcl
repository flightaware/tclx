#-----------------------------------------------------------------------------
# tkx.tcl -- Extended Tcl Tk initialization.
#-----------------------------------------------------------------------------
# $Id: tkx.tcl,v 8.0.4.1 1997/04/14 02:03:03 markd Exp $
#-----------------------------------------------------------------------------

if {[info exists tkx_library] && ![cequal $tkx_library {}]} {
    if ![lcontain $auto_path $tkx_library] {
	lappend auto_path $tkx_library
    }
}


