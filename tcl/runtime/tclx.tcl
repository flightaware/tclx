#-----------------------------------------------------------------------------
# TclInit.tcl -- Extended Tcl initialization.
#-----------------------------------------------------------------------------
# $Id: TclInit.tcl,v 5.0 1995/07/25 06:00:03 markd Rel $
#-----------------------------------------------------------------------------

set auto_index(buildpackageindex) {source $tclx_library/buildidx.tcl}

# == Put any code you want all Tcl programs to include here. ==

if !$tcl_interactive return

# == Interactive Tcl session initialization ==

# Replace standard Tcl prompt with out prompt if its the TclX shell

if ![info exists tcl_prompt1] {
    set tcl_prompt1 {global argv0; puts -nonewline stdout [file tail $argv0]>}
}
if ![info exists tcl_prompt2] {
    set tcl_prompt2 {puts -nonewline stdout =>}
}
