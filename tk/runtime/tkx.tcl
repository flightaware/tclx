#-----------------------------------------------------------------------------
# tkx.tcl -- Extended Tcl Tk initialization.
#-----------------------------------------------------------------------------
# $Id: tkx.tcl,v 1.1.1.1 1996/12/20 05:39:22 karl Exp $
#-----------------------------------------------------------------------------

if {[info exists tkx_library] && ![cequal $tkx_library {}]} {
    if ![lcontain $auto_path $tkx_library] {
	lappend auto_path $tkx_library
    }
}
