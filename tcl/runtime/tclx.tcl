#-----------------------------------------------------------------------------
# tclx.tcl -- Extended Tcl initialization.
#-----------------------------------------------------------------------------
# $Id: tclx.tcl,v 5.1 1996/02/16 07:51:27 markd Exp $
#-----------------------------------------------------------------------------

set auto_index(buildpackageindex) {source $tclx_library/buildidx.tcl}
if {[info exists tclx_library] && ([lsearch $auto_path $tclx_library] < 0)} {
    lappend auto_path $tclx_library
}

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
