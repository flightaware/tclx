#
# instcopy.tcl -- 
#
# Tcl program to copy files during the installation of Tcl.  This is used
# because "copy -r" is not ubiquitous.  It also adds some minor additional
# functionallity.
#
#------------------------------------------------------------------------------
# Copyright 1992-1994 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: instcopy.tcl,v 3.0 1993/11/19 07:00:03 markd Rel markd $
#------------------------------------------------------------------------------
#
# It is run in the following manner:
#
#  instcopy file1 file2 ... targetdir
#  instcopy -dirname file1 targetdir
#
#  o -dirname - If specified, then a directory is copies as the target
#     directory rather than to it.
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
    puts stderr {usage: instcopy ?-dirname? file1 file2 ... targetdir}
    exit 1
}

#------------------------------------------------------------------------------
# DoACopy --
#
# 
#------------------------------------------------------------------------------

proc DoACopy {file targetDir} {
    global dirNameMode

    if [file isdirectory $file] {
        if $dirNameMode {
            set target $targetDir
        } else {
            set target $targetDir/[file tail $file]
        }
        puts stdout ""
        puts stdout "Copying directory hierarchy $file to $target"
        puts stdout ""
        if ![file exists $target] {
            mkdir -path  $target
        }
        CopyDir $file $target
    } else {
        puts stdout ""
        puts stdout "Copying $file to $targetDir"
        puts stdout ""
        CopyFile $file $targetDir
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

set dirNameMode 0
if {[lindex $argv 0] == "-dirname"} {
    lvarpop argv
    incr argc -1
    set dirNameMode 1
}

set files [lrange $argv 0 [expr $argc-2]]
set targetDir [lindex $argv [expr $argc-1]]

if {[file exists $targetDir] && ![file isdirectory $targetDir]} {
   Usage "Target is not a directory: $targetDir"
}

umask 022

if [catch {
    if ![file exists $targetDir] {
        mkdir -path $targetDir
    }
    foreach file $files {
        DoACopy $file $targetDir
    }
} msg] {
    puts stderr "Error: $msg"
    exit 1
}
