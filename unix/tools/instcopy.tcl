#
# instcopy.tcl -- 
#
# Tcl program to copy files during the installation of Tcl.  This is used
# because "copy -r" is not ubiquitous.  It also adds some minor additional
# functionallity.
#
#------------------------------------------------------------------------------
# Copyright 1992-1996 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: instcopy.tcl,v 5.1 1995/08/30 15:03:22 markd Exp $
#------------------------------------------------------------------------------
#
# It is run in the following manner:
#
#  instcopy file1 file2 ... targetdir
#  instcopy -filename file1 targetfile
#
#  o -filename - If specified, then the last file is the name of a file rather
#    than a directory. 
#  o files - List of files to copy. If one of directories are specified, they
#    are copied.
#  o targetdir - Target directory to copy the files to.  If the directory does
#    not exist, it is created (including parent directories).
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#------------------------------------------------------------------------------
# Usage --
#
#   Issue a usage message and exit.
#------------------------------------------------------------------------------
proc Usage {{msg {}}} {
    if {"$msg" != ""} {
        puts stderr "Error: $msg"
    }
    puts stderr {usage: instcopy ?-filename? file1 file2 ... targetdir}
    exit 1
}

#------------------------------------------------------------------------------
# DoACopy --
#------------------------------------------------------------------------------

proc DoACopy {file target mode} {

    if [cequal [file tail $file] "CVS"] {
        return
    }
    if {$mode == "FILENAME"} {
        set targetDir [file dirname $target]
        if [file exists $target] {
            unlink $target
        }
    } else {
        set targetDir $target
    }
    if ![file exists $targetDir] {
        mkdir -path  $targetDir
    }

    if [file isdirectory $file] {
        CopyDir $file $target
    } else {
        CopyFile $file $target
    }
}


#------------------------------------------------------------------------------
# Main program code.
#------------------------------------------------------------------------------

#
# Parse the arguments
#
if {$argc < 2} {
    Usage "Not enough arguments"
}

switch -exact -- [lindex $argv 0] {
    -filename {
        set mode FILENAME
        lvarpop argv
        incr argc -1
    }
    default {
        set mode {}
    }
}

set files [lrange $argv 0 [expr $argc-2]]
set targetDir [lindex $argv [expr $argc-1]]

if {[file exists $targetDir] && ![file isdirectory $targetDir] &&
    ($mode != "FILENAME")} {
   Usage "Target is not a directory: $targetDir"
}

umask 022

if [catch {
    foreach file $files {
        DoACopy $file $targetDir $mode
    }
} msg] {
    puts stderr "Error: $msg"
    exit 1
}
