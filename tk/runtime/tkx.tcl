#-----------------------------------------------------------------------------
# tkx.tcl -- Extended Tcl Tk initialization.
#-----------------------------------------------------------------------------
# $Id: tkx.tcl,v 7.0 1996/06/16 05:33:57 markd Exp $
#-----------------------------------------------------------------------------

if {[info exists tkx_library] && ![cequal $tkx_library {}]} {
    if [lcontain $auto_path $tkx_library] {
	lappend auto_path $tkx_library
    }
}
