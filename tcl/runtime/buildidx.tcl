#
# buildidx.tcl --
#
# Code to build Tcl package library. Defines the proc `buildpackageindex'.
# 
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
# $Id: buildidx.tcl,v 4.0 1994/07/16 05:30:02 markd Rel markd $
#------------------------------------------------------------------------------
#

#------------------------------------------------------------------------------
# The following code passes around a array containing information about a
# package.  The following fields are defined
#
#   o name - The name of the package.
#   o offset - The byte offset of the package in the file.
#   o procs - The list of entry point procedures defined for the package.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Write a line to the index file describing the package.
#
proc TCLSH:PutIdxEntry {outfp pkgInfo endOffset} {
    puts $outfp [concat [keylget pkgInfo name] \
                        [keylget pkgInfo offset] \
                        [expr {$endOffset - [keylget pkgInfo offset] - 1}] \
                        [keylget pkgInfo procs]]
}

#------------------------------------------------------------------------------
# Parse a package header found by a scan match.  Handle backslashed
# continuation lines.
#
proc TCLSH:ParsePkgHeader matchInfoVar {
    upvar $matchInfoVar matchInfo

    set line [string trimright $matchInfo(line)]
    while {[string match {*\\} $line]} {
        set line [csubstr $line 0 [expr [clength $line]-1]]
        append line " " [string trimright [gets $matchInfo(handle)]]
    }

    keylset pkgInfo name   [lindex $line 1]
    keylset pkgInfo offset [tell $matchInfo(handle)]
    keylset pkgInfo procs  [lrange $line  2 end]
    return $pkgInfo
}

#------------------------------------------------------------------------------
# Do the actual work of creating a package library index from a library file.
#
proc TCLSH:CreateLibIndex {libName} {

    if {[file extension $libName] != ".tlib"} {
        error "Package library `$libName' does not have the extension `.tlib'"
    }
    set idxName "[file root $libName].tndx"

    unlink -nocomplain $idxName
    set libFH [open $libName r]
    set idxFH [open $idxName w]
    set packageCnt 0

    set contectHdl [scancontext create]

    scanmatch $contectHdl "^#@package: " {
        if {[llength $matchInfo(line)] < 2} {
            error "invalid package header \"$matchInfo(line)\""
        }
        if ![lempty $pkgInfo] {
            TCLSH:PutIdxEntry $idxFH $pkgInfo $matchInfo(offset)
        }
        set pkgInfo [TCLSH:ParsePkgHeader matchInfo]
        incr packageCnt
    }

    scanmatch $contectHdl "^#@packend" {
        if [lempty $pkgInfo] {
            error "#@packend without #@package in $libName"
        }
        TCLSH:PutIdxEntry $idxFH $pkgInfo $matchInfo(offset)
        set pkgInfo {}
    }

    set pkgInfo {}
    if {[catch {
        scanfile $contectHdl $libFH
        if {$packageCnt == 0} {
            error "No \"#@package:\" definitions found in $libName"
        }   
    } msg] != 0} {
       global errorInfo errorCode
       close $libFH
       close $idxFH
       unlink -nocomplain $idxName
       error $msg $errorInfo $errorCode
    }
    if ![lempty $pkgInfo] {
        TCLSH:PutIdxEntry $idxFH $pkgInfo [tell $libFH] 
    }
    close $libFH
    close $idxFH
    
    scancontext delete $contectHdl

    # Set mode and ownership of the index to be the same as the library.
    # Ignore errors if you can't set the ownership.

    file stat $libName statInfo
    chmod $statInfo(mode) $idxName
    catch {
       chown [list $statInfo(uid) $statInfo(gid)] $idxName
    }
}

#------------------------------------------------------------------------------
# Create a package library index from a library file.
#
proc buildpackageindex {libfilelist} {
    foreach libfile $libfilelist {
        set status [catch {
            TCLSH:CreateLibIndex $libfile
        } errmsg]
        if {$status != 0} {
            global errorInfo errorCode
            error "building package index for `$libfile' failed: $errmsg" \
                $errorInfo $errorCode
        }
    }
}

