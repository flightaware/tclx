#
# buildutil.tcl -- 
#
# Utility procedures used by the build and install tools.
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
# $Id: buildutil.tcl,v 1.3 1993/10/01 03:49:16 markd Exp markd $
#------------------------------------------------------------------------------
#

#------------------------------------------------------------------------------
# CopyFile -- 
#
# Copy the specified file and change the ownership.  If target is a directory,
# then the file is copied to it, otherwise target is a new file name.
# If the source file was owner-executable, the all-executable is set on the
# created file.
#------------------------------------------------------------------------------

proc CopyFile {sourceFile target} {
    if {[file isdirectory $target]} {
        set targetFile "$target/[file tail $sourceFile]"
    } else {
        set targetFile $target
    }

    unlink -nocomplain $targetFile
    set sourceFH [open $sourceFile r]
    set targetFH [open $targetFile w]
    copyfile $sourceFH $targetFH
    close $sourceFH
    close $targetFH

    # Fixup the mode.

    file stat $sourceFile sourceStat
    if {$sourceStat(mode) & 0100} {
        chmod a+rx $targetFile
    } else {
        chmod a+r  $targetFile
    }
}

#------------------------------------------------------------------------------
# CopySubDir --
#
# Recursively copy part of a directory tree, changing ownership and 
# permissions.  This is a utility routine that actually does the copying.
#------------------------------------------------------------------------------

proc CopySubDir {sourceDir destDir} {
    foreach sourceFile [glob -nocomplain $sourceDir/*] {
        if [file isdirectory $sourceFile] {
            set destFile $destDir/[file tail $sourceFile]
            if {![file exists $destFile]} {
                mkdir $destFile}
            CopySubDir $sourceFile $destFile
        } else {
            CopyFile $sourceFile $destDir
        }
    }
}

#------------------------------------------------------------------------------
# CopyDir --
#
# Recurisvely copy a directory tree.
#------------------------------------------------------------------------------

proc CopyDir {sourceDir destDir} {
    set cwd [pwd]
    if ![file exists $sourceDir] {
        error "\"$sourceDir\" does not exist"
    }
    if ![file isdirectory $sourceDir] {
        error "\"$sourceDir\" isn't a directory"
    }
    
    # Dirs must be absolutes paths, as we are going to change directories.

    set sourceDir [glob $sourceDir]
    if {[cindex $sourceDir 0] != "/"} {
        set sourceDir "$cwd/$sourceDir"
    }
    set destDir [glob $destDir]
    if {[cindex $destDir 0] != "/"} {
        set destDir "$cwd/$destDir"
    }

    if {![file exists $destDir]} {
        mkdir $destDir
    }
    if ![file isdirectory $destDir] {
        error "\"$destDir\" isn't a directory"
    }
    cd $sourceDir
    set status [catch {CopySubDir . $destDir} msg]
    cd $cwd
    if {$status != 0} {
        global errorInfo errorCode
        error $msg $errorInfo $errorCode
    }
}

