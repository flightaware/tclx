#-----------------------------------------------------------------------------
# tkx.tcl -- Extended Tcl Tk initialization.
#-----------------------------------------------------------------------------
# $Id: tkx.tcl,v 7.2 1996/10/04 04:43:35 markd Exp $
#-----------------------------------------------------------------------------

if {[info exists tkx_library] && ![cequal $tkx_library {}]} {
    if ![lcontain $auto_path $tkx_library] {
	lappend auto_path $tkx_library
    }
}
