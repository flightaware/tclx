#-----------------------------------------------------------------------------
# tkx.tcl -- Extended Tcl Tk initialization.
#-----------------------------------------------------------------------------
# $Id: tkx.tcl,v 7.1 1996/10/04 04:25:22 markd Exp $
#-----------------------------------------------------------------------------

if {[info exists tkx_library] && ![cequal $tkx_library {}]} {
    if ![lcontain $auto_path $tkx_library] {
	lappend auto_path $tkx_library
    }
}
