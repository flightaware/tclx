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
# $Id: installTcl.tcl,v 2.8 1993/06/06 15:05:35 markd Exp $
#------------------------------------------------------------------------------
#

#------------------------------------------------------------------------------
# ParseMakeMacros --
#
#  Parse make macros out of the specified config file.
#------------------------------------------------------------------------------

proc ParseMakeMacros {configFile configVar} {
    upvar $configVar config

    set cfgFH [open $configFile]
 
    while {[gets $cfgFH line] >= 0} {
        if {[string match {[A-Za-z]*} $line]} {
            set idx [string first "=" $line]
            if {$idx < 0} {
                error "no `=' in: $line"}
            set name  [string trim [csubstr $line 0 $idx]]
            set value [string trim [crange  $line [expr $idx+1] end]]
            set config($name) $value
        }
    }
    close $cfgFH
}

#------------------------------------------------------------------------------
# ParseConfigFile --
#
#  Parse the Config.mk file.  Includes parsing the system config file.
#------------------------------------------------------------------------------

proc ParseConfigFile {configFile configVar} {
    upvar $configVar config

    ParseMakeMacros $configFile config
    if ![info exists config(TCL_CONFIG_FILE)] {
        error "TCL_CONFIG_FILE not defined in $configFile"
    }
    set sysConfig [file dirname $configFile]/config/$config(TCL_CONFIG_FILE)

    if ![file exists $sysConfig] {
        error "can't file $sysConfig, as defined by TCL_CONFIG_FILE in $configFile"
    }
    ParseMakeMacros $sysConfig config
}

#------------------------------------------------------------------------------
# CopyFile -- 
#
# Copy the specified file and change the ownership.  If target is a directory,
# then the file is copied to it, otherwise target is a new file name.
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
        GiveAwayFile $destDir
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

