#-----------------------------------------------------------------------------
# TclInit.tcl -- Extended Tcl initialization.
#-----------------------------------------------------------------------------
# $Id: TclInit.tcl,v 4.1 1994/12/06 04:37:06 markd Exp markd $
#-----------------------------------------------------------------------------

#
# Unknown command trap handler.
#
proc unknown args {
    if [auto_load [lindex $args 0]] {
        return [uplevel 1 $args]
    }
    if {([info proc tclx_unknown2] == "") && ![auto_load tclx_unknown2]} {
        error "can't find tclx_unknown2 on auto_path"
    }
    return [uplevel 1 [tclx_unknown2 $args]]
}

set auto_index(buildpackageindex) {source $tclx_library/buildidx.tcl}

# == Put any code you want all Tcl programs to include here. ==

if !$tcl_interactive return

# == Interactive Tcl session initialization ==

if ![info exists tcl_prompt1] {
    set tcl_prompt1 {global argv0; puts -nonewline stdout [file tail $argv0]>}
}
if ![info exists tcl_prompt2] {
    set tcl_prompt2 {puts -nonewline stdout =>}
}
