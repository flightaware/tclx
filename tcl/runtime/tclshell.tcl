#
# tclshell.tcl --
#
# Commands that are used to support an interactive Tcl shell.  These are
# not called directly, but from the "unknown" command.  Much of this code
# is taken directly from the UCB Tcl library/init.tcl file.
#------------------------------------------------------------------------------
# Copyright 1992-1993 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# Copyright (c) 1991-1993 The Regents of the University of California.
# All rights reserved.
#
# Permission is hereby granted, without written agreement and without
# license or royalty fees, to use, copy, modify, and distribute this
# software and its documentation for any purpose, provided that the
# above copyright notice and the following two paragraphs appear in
# all copies of this software.
#
# IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
# OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
# CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
# ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
# PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
#------------------------------------------------------------------------------
# $Id: tclshell.tcl,v 1.8 1993/10/01 03:49:16 markd Exp markd $
#------------------------------------------------------------------------------
#

#@package: TclX-shell tclx_unknown2 auto_execok

#------------------------------------------------------------------------------
# tclx_unknown:
# This implements the slow path of the TclX unknown command.  It must be called
# directly from the unknown command.  This handles exec-ing of Unix programs
# and interactive csh style redo.  Returns the result of the executed command.
#

proc tclx_unknown2 cmd {
    global tcl_interactive auto_noexec

    set name [lindex $cmd 0]

    if ![info exists auto_noexec] {
        if [auto_execok $name] {
            if {!$tcl_interactive || ([info level] > 2) ||
                [info script] != ""} {
                error "Auto execution of Unix commands only supported as interactive commands.\nUse \"exec\" to execute \"$name\""
            }
            uplevel 2 system [list $cmd]
            return
        }
    }

    if {!$tcl_interactive || ([info level] > 2) || [info script] != ""} {
        error "invalid command name \"$name\""
    }

    # csh-style redo.

    if {([info level] == 2) && ([info script] == "")} {
        if {$name == "!!"} {
            return [uplevel 2 {history redo}]
        }
        if [regexp {^!(.+)$} $name dummy event] {
            return [uplevel 2 [list history redo $event]]
        }
        if [regexp {^\^([^^]*)\^([^^]*)\^?$} $name dummy old new] {
            return [uplevel 2 [list history substitute $old $new]]
        }
        set cmds [info commands $name*]
        if {[llength $cmds] == 1} {
            return [uplevel 2 [lreplace $cmd 0 0 $cmds]]
        }
        if {[llength $cmds] != 0} {
            if {$name == ""} {
                return -code error "empty command name \"\""
            } else {
                return -code error \
                        "ambiguous command name \"$name\": [lsort $cmds]"
            }
        }
    }
    error "invalid command name \"$name\""
}


#------------------------------------------------------------------------------
# auto_execok:
# Returns 1 if there's an executable in the current path for the
# given name, 0 otherwise.  Builds an associative array auto_execs
# that caches information about previous checks, for speed.

proc auto_execok name {
    global auto_execs env

    if [info exists auto_execs($name)] {
        return $auto_execs($name)
    }
    set auto_execs($name) 0
    if {[string first / $name] >= 0} {
	if {[file executable $name] && ![file isdirectory $name]} {
	    puts "special, ok!"
	    set auto_execs($name) 1
	}
	return $auto_execs($name)
    }
    foreach dir [split $env(PATH) :] {
        if {[file executable $dir/$name] && ![file isdirectory $dir/$name]} {
            set auto_execs($name) 1
            return 1
        }
    }
    return 0
}
