#
# Modified version of the standard Tcl auto_load proc that calls a TclX
# command load TclX .tndx library indices.
#
# Default system startup file for Tcl-based applications.  Defines
# "unknown" procedure and auto-load facilities.
#
# SCCS: @(#) init.tcl 1.88 97/11/06 12:30:46
#
# Copyright (c) 1991-1993 The Regents of the University of California.
# Copyright (c) 1994-1996 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
proc auto_load {cmd {namespace {}}} {
    global auto_index auto_oldpath auto_path env errorInfo errorCode

    if {[string length $namespace] == 0} {
	set namespace [uplevel {namespace current}]
    }
    set name [auto_qualify $cmd $namespace]
    if [info exists auto_index($name)] {
	uplevel #0 $auto_index($name)
	return [expr {[info commands $name] != ""}]
    }
    if ![info exists auto_path] {
	return 0
    }
    if [info exists auto_oldpath] {
	if {$auto_oldpath == $auto_path} {
	    return 0
	}
    }
    set auto_oldpath $auto_path

    # Check if we are a safe interpreter. In that case, we support only
    # newer format tclIndex files.

    set issafe [interp issafe]
    for {set i [expr [llength $auto_path] - 1]} {$i >= 0} {incr i -1} {
	set dir [lindex $auto_path $i]
	set f ""
        tclx_load_tndxs $dir
	if {$issafe} {
	    catch {source [file join $dir tclIndex]}
	} elseif [catch {set f [open [file join $dir tclIndex]]}] {
	    continue
	} else {
	    set error [catch {
		set id [gets $f]
		if {$id == "# Tcl autoload index file, version 2.0"} {
		    eval [read $f]
		} elseif {$id == \
		    "# Tcl autoload index file: each line identifies a Tcl"} {
		    while {[gets $f line] >= 0} {
			if {([string index $line 0] == "#")
				|| ([llength $line] != 2)} {
			    continue
			}
			set name [lindex $line 0]
			set auto_index($name) \
			    "source [file join $dir [lindex $line 1]]"
		    }
		} else {
		    error \
		      "[file join $dir tclIndex] isn't a proper Tcl index file"
		}
	    } msg]
	    if {$f != ""} {
		close $f
	    }
	    if $error {
		error $msg $errorInfo $errorCode
	    }
	}
    }
    if [info exists auto_index($name)] {
	uplevel #0 $auto_index($name)
	if {[info commands $name] != ""} {
	    return 1
	}
    }
    return 0
}

