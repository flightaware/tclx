#
# buildutil.tcl -- 
#
# Utility procedures used by the build and install tools.
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
# $Id: buildutil.tcl,v 7.0 1996/06/16 05:34:31 markd Exp $
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

    catch {file delete $targetFile}
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
    foreach sourceFile [readdir $sourceDir] {
        if [file isdirectory $sourceDir/$sourceFile] {
          if [cequal [file tail $sourceFile ] "CVS"] {
                  continue
          }
            set destFile $destDir/$sourceFile
            if {![file exists $destFile]} {
                mkdir $destFile}
            CopySubDir $sourceDir/$sourceFile $destFile
        } else {
            CopyFile $sourceDir/$sourceFile $destDir
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
    if [cequal [file tail $sourceDir] "CVS"] {
          return
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

