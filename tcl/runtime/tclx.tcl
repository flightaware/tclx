#-----------------------------------------------------------------------------
# TclInit.tcl -- Extended Tcl initialization.
#-----------------------------------------------------------------------------
# $Id: TclInit.tcl,v 2.3 1993/06/09 06:05:29 markd Exp markd $
#-----------------------------------------------------------------------------

#
# Unknown command trap handler.
#
proc unknown {args} {
    if [auto_load [lindex $args 0]] {
        return [uplevel $args]
    }
    if {([info proc tclx_unknown2] == "") && ![auto_load tclx_unknown2]} {
        error "can't find tclx_unknown2 on auto_path"
    }
    return [tclx_unknown2 $args]
}

# == Put any code you want all Tcl programs to include here. ==

if !$interactiveSession return

# == Interactive Tcl session initialization ==

set TCLENV(topLevelPromptHook) {global programName; concat "$programName>" }
set TCLENV(downLevelPromptHook) {concat "=>"}

if [file readable ~/.tclrc] {source ~/.tclrc}

