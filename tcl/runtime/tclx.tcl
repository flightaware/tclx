#-----------------------------------------------------------------------------
# TclInit.tcl -- Extended Tcl initialization.
#-----------------------------------------------------------------------------
# $Id: TclInit.tcl,v 2.4 1993/06/21 05:58:43 markd Exp markd $
#-----------------------------------------------------------------------------

#
# Unknown command trap handler.
#
proc unknown {name args} {
    if [auto_load $name] {
        return [uplevel 1 $name $args]
    }
    if {([info proc tclx_unknown2] == "") && ![auto_load tclx_unknown2]} {
        error "can't find tclx_unknown2 on auto_path"
    }
    return [tclx_unknown2 [concat $name $args]]
}

set auto_index(buildpackageindex) {source [info library]/buildidx.tcl}

# == Put any code you want all Tcl programs to include here. ==

if !$interactiveSession return

# == Interactive Tcl session initialization ==

set TCLENV(topLevelPromptHook) {global programName; concat "$programName>" }
set TCLENV(downLevelPromptHook) {concat "=>"}

if [file readable ~/.tclrc] {source ~/.tclrc}

