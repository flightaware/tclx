#
# tcllib.tcl --
#
# Various command dealing with auto-load libraries.  Some of this code is
# taken directly from the UCB Tcl library/init.tcl file.
#------------------------------------------------------------------------------
# Copyright 1992-1995 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# Copyright (c) 1991-1994 The Regents of the University of California.
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
# $Id: tcllib.tcl,v 4.1 1995/01/01 19:50:01 markd Exp markd $
#------------------------------------------------------------------------------
#

#@package: TclX-libraries searchpath auto_load_file

#------------------------------------------------------------------------------
# searchpath:
# Search a path list for a file. (catch is for bad ~user)
#
proc searchpath {pathlist file} {
    foreach dir $pathlist {
        if {$dir == ""} {set dir .}
        if {[catch {file exists $dir/$file} result] == 0 && $result}  {
            return $dir/$file
        }
    }
    return {}
}

#------------------------------------------------------------------------------
# auto_load_file:
# Search auto_path for a file and source it.
#
proc auto_load_file {name} {
    global auto_path errorCode
    if {[string first / $name] >= 0} {
        return  [uplevel 1 source $name]
    }
    set where [searchpath $auto_path $name]
    if [lempty $where] {
        error "couldn't find $name in any directory in auto_path"
    }
    uplevel 1 source $where
}

#@package: TclX-lib-list auto_packages auto_commands

#------------------------------------------------------------------------------
# auto_packages:
# List all of the loadable packages.  If -files is specified, the file paths
# of the packages is also returned.

proc auto_packages {{option {}}} {
    global auto_pkg_index

    auto_load  ;# Make sure all indexes are loaded.
    if ![info exists auto_pkg_index] {
        return {}
    }
    
    set packList [array names auto_pkg_index] 
    if [lempty $option] {
        return $packList
    }

    if {$option != "-files"} {
        error "Unknow option \"$option\", expected \"-files\""
    }
    set locList {}
    foreach pack $packList {
        lappend locList [list $pack [lindex $auto_pkg_index($pack) 0]]
    }
    return $locList
}

#------------------------------------------------------------------------------
# auto_commands:
# List all of the loadable commands.  If -loaders is specified, the commands
# that will be involked to load the commands is also returned.

proc auto_commands {{option {}}} {
    global auto_index

    auto_load  ;# Make sure all indexes are loaded.
    if ![info exists auto_index] {
        return {}
    }
    
    set cmdList [array names auto_index] 
    if [lempty $option] {
        return $cmdList
    }

    if {$option != "-loaders"} {
        error "Unknow option \"$option\", expected \"-loaders\""
    }
    set loadList {}
    foreach cmd $cmdList {
        lappend loadList [list $cmd $auto_index($cmd)]
    }
    return $loadList
}

#@package: TclX-ucblib auto_reset auto_mkindex

#------------------------------------------------------------------------------
# auto_reset:
# Destroy all cached information for auto-loading and auto-execution,
# so that the information gets recomputed the next time it's needed.
# Also delete any procedures that are listed in the auto-load index
# except those related to auto-loading.
# *** MODIFIED FOR TclX ***

proc auto_reset {} {
    global auto_execs auto_index auto_oldpath
    foreach p [info procs] {
	if {[info exists auto_index($p)] && ($p != "unknown")
		&& ![string match auto_* $p]} {
	    rename $p {}
	}
    }
    catch {unset auto_execs}
    catch {unset auto_index}
    catch {unset auto_oldpath}
    # *** TclX ***
    global auto_pkg_index
    catch {unset auto_pkg_index}
    set auto_index(buildpackageindex) {source $tclx_library/buildidx.tcl}
    return
}

#------------------------------------------------------------------------------
# auto_mkindex:
# Regenerate a tclIndex file from Tcl source files.  Takes two arguments:
# the name of the directory in which the tclIndex file is to be placed,
# and a glob pattern to use in that directory to locate all of the relevant
# files.

proc auto_mkindex {dir files} {
    global errorCode errorInfo
    set oldDir [pwd]
    cd $dir
    set dir [pwd]
    append index "# Tcl autoload index file, version 2.0\n"
    append index "# This file is generated by the \"auto_mkindex\" command\n"
    append index "# and sourced to set up indexing information for one or\n"
    append index "# more commands.  Typically each line is a command that\n"
    append index "# sets an element in the auto_index array, where the\n"
    append index "# element name is the name of a command and the value is\n"
    append index "# a script that loads the command.\n\n"
    foreach file [glob $files] {
	set f ""
	set error [catch {
	    set f [open $file]
	    while {[gets $f line] >= 0} {
		if [regexp {^proc[ 	]+([^ 	]*)} $line match procName] {
		    append index "set [list auto_index($procName)]"
		    append index " \"source \$dir/$file\"\n"
		}
	    }
	    close $f
	} msg]
	if $error {
	    set code $errorCode
	    set info $errorInfo
	    catch [close $f]
	    cd $oldDir
	    error $msg $info $code
	}
    }
    set f [open tclIndex w]
    puts $f $index nonewline
    close $f
    cd $oldDir
}

