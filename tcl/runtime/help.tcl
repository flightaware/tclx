#
# help.tcl --
#
# Tcl help command. (see TclX manual)
# 
#------------------------------------------------------------------------------
# Copyright 1992-1997 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# The help facility is based on a hierarchical tree of subjects (directories)
# and help pages (files).  There is a virtual root to this tree. The root
# being the merger of all "help" directories found along the $auto_path
# variable.
#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------
#
# FIX: Convert this to use namespaces.

#@package: TclX-help help helpcd helppwd apropos

namespace eval TclXHelp {

    # Determine the path separator.
    switch $::tcl_platform(platform) {
        unix {
            variable pathSep /
        }
        windows {
            variable pathSep \\
        }
        macintosh {
            variable pathSep :
        }
        default {
            error "unknown platform \"$::tcl_platform(platform)\""
        }
    }

    variable curSubject $pathSep

    #----------------------------------------------------------------------
    # Return a list of help root directories.

    proc RootDirs {} {
        global auto_path
        set roots {}
        foreach dir $auto_path {
            set fname [file join $dir help]
            if {[file isdirectory $fname]} {
                lappend roots $fname
            }
        }
        return $roots
    }

    #--------------------------------------------------------------------------
    # Take a path name which might have "." and ".." elements and flatten them
    # out.  Also removes trailing and adjacent "/", unless its the only
    # character.

    proc NormalizePath helpFile {
        variable pathSep
        set newPath {}
        foreach element [file split $helpFile] {
            if {[cequal $element  .] || [lempty $element]} continue

            if {[cequal $element ..]} {
                if {[llength [file join $newPath]] == 0} {
                    error "Help: name goes above subject directory root" {} \
                        [list TCLXHELP NAMEABOVEROOT $helpFile]
                }
                lvarpop newPath [expr {[llength $newPath]-1}]
            } else {
                lappend newPath $element
            }
            
        }
        set newPath [eval file join $newPath]

        # Take care of the case where we started with something line "/" or "/."
        if {[lempty $newPath] && [cequal [file pathtype $helpFile] absolute]} {
            set newPath $pathSep
        }

        return $newPath
    }

    #--------------------------------------------------------------------------
    # Given a helpFile relative to the virtual help root, convert it to a list
    # of real file paths.  A list is returned because the path could be "/",
    # returning a list of all roots. The list is returned in the same order of
    # the auto_path variable. If path does not start with a "/", it is take as
    # relative to the current help subject.  Note: The root directory part of
    # the name is not flattened.  This lets other commands pick out the part
    # relative to the one of the root directories.

    proc ConvertHelpFile helpFile {
        variable curSubject
        variable pathSep

        if {![cequal [file pathtype $helpFile] absolute]} {
            if {[cequal $curSubject $pathSep]} {
                set helpFile [file join $pathSep $helpFile]
            } else {
                set helpFile [file join $curSubject $helpFile]
            }
        }
        set helpFile [NormalizePath $helpFile]

        # If the virtual root is specified, return a list of directories.

        if {[cequal $helpFile $pathSep]} {
            return [RootDirs]
        }

        # Make help file name into a relative path for joining with real
        # root.
        set helpFile [eval file join [lrange [file split $helpFile] 1 end]]

        # Search for the first match.
        foreach dir [RootDirs] {
            set fname [file join $dir $helpFile]
            if {[file readable $fname]} {
                return [list $fname]
            }
        }

	# Not found, try to find a file matching only the file tail,
	# for example if --> <helpDir>/tcl/control/if.

	set fileTail [file tail $helpFile]
        foreach dir [RootDirs] {
	    set fileName [recursive_glob $dir $fileTail]
	    if {![lempty $fileName]} {
                return [list $fileName]
	    }
	}

        error "\"$helpFile\" does not exist" {} \
            [list TCLXHELP NOEXIST $helpFile]
    }

    #--------------------------------------------------------------------------
    # Return the virtual root relative name of the file given its absolute
    # path.  The root part of the path should not have been normalized, as we
    # would not be able to match it.

    proc RelativePath helpFile {
        variable pathSep
        foreach dir [RootDirs] {
            if {[cequal [csubstr $helpFile 0 [clength $dir]] $dir]} {
                set name [csubstr $helpFile [clength $dir] end]
                if {[lempty $name]} {
                    set name $pathSep
                }
                return $name
            }
        }
        if {![info exists found]} {
            error "problem translating \"$helpFile\"" {} [list TCLXHELP INTERROR]
        }
    }

    #--------------------------------------------------------------------------
    # Given a list of path names to subjects generated by ConvertHelpFile, return
    # the contents of the subjects.  Two lists are returned, subjects under
    # that subject and a list of pages under the subject.  Both lists are
    # returned sorted.  This merges all the roots into a virtual root.
    # helpFile is the string that was passed to ConvertHelpFile and is used for
    # error reporting.  *.brk files are not returned.

    proc ListSubject {helpFile pathList subjectsVar pagesVar} {
        upvar $subjectsVar subjects $pagesVar pages
        variable pathSep

        set subjects {}
        set pages {}
        set foundDir 0
        foreach dir $pathList {
            if {![file isdirectory $dir]} continue
            if {[cequal [file tail $dir] CVS]} continue
            set foundDir 1
            foreach file [glob -nocomplain [file join $dir *]] {
                if {[lsearch {.brf .orig .diff .rej} [file extension $file]] \
                        >= 0} continue
                if {[file isdirectory $file]} {
                    lappend subjects [file tail $file]$pathSep
                } else {
                    lappend pages [file tail $file]
                }
            }
        }
        if {!$foundDir} {
            if {[cequal $helpFile $pathSep]} {
                global auto_path
                error "no \"help\" directories found on auto_path ($auto_path)" {} \
                    [list TCLXHELP NOHELPDIRS]
            } else {
                error "\"$helpFile\" is not a subject" {} \
                    [list TCLXHELP NOTSUBJECT $helpFile]
            }
        }
        set subjects [lsort $subjects]
        set pages [lsort $pages]
        return {}
    }

    #--------------------------------------------------------------------------
    # Display a line of output, pausing waiting for input before displaying if
    # the screen size has been reached.  Return 1 if output is to continue,
    # return 0 if no more should be outputed, indicated by input other than
    # return.
    #

    proc Display line {
        variable lineCnt
        if {$lineCnt >= 23} {
            set lineCnt 0
            puts -nonewline stdout ":"
            flush stdout
            gets stdin response
            if {![lempty $response]} {
                return 0}
        }
        puts stdout $line
        incr lineCnt
    }

    #--------------------------------------------------------------------------
    # Display a help page (file).

    proc DisplayPage filePath {

        set inFH [open $filePath r]
        try_eval {
            while {[gets $inFH fileBuf] >= 0} {
                if {![Display $fileBuf]} {
                    break
                }
            }
        } {} {
            close $inFH
        }
    }

    #--------------------------------------------------------------------------
    # Display a list of file names in a column format. This use columns of 14 
    # characters 3 blanks.

    proc DisplayColumns {nameList} {
        set count 0
        set outLine ""
        foreach name $nameList {
            if {$count == 0} {
                append outLine "   "}
            append outLine $name
            if {[incr count] < 4} {
                set padLen [expr {17-[clength $name]}]
                if {$padLen < 3} {
                   set padLen 3}
                append outLine [replicate " " $padLen]
            } else {
               if {![Display $outLine]} {
                   return}
               set outLine ""
               set count 0
            }
        }
        if {$count != 0} {
            Display [string trimright $outLine]}
        return
    }


    #--------------------------------------------------------------------------
    # Display help on help, the first occurance of a help page called "help" in
    # the help root.

    proc HelpOnHelp {} {
        variable pathSep
        set helpPage [lindex [ConvertHelpFile ${pathSep}help] 0]
        if {[lempty $helpPage]} {
            error "No help page on help found" {} \
                [list TCLXHELP NOHELPPAGE]
        }
        DisplayPage $helpPage
    }

};# namespace TclXHelp


#------------------------------------------------------------------------------
# Help command.

proc help {{what {}}} {
    variable ::TclXHelp::lineCnt 0

    # Special case "help help", so we can get it at any level.

    if {($what == "help") || ($what == "?")} {
        TclXHelp::HelpOnHelp
        return
    }

    set pathList [TclXHelp::ConvertHelpFile $what]
    if {[file isfile [lindex $pathList 0]]} {
        TclXHelp::DisplayPage [lindex $pathList 0]
        return
    }

    TclXHelp::ListSubject $what $pathList subjects pages
    set relativeDir [TclXHelp::RelativePath [lindex $pathList 0]]

    if {[llength $subjects] != 0} {
        TclXHelp::Display "\nSubjects available in $relativeDir:"
        TclXHelp::DisplayColumns $subjects
    }
    if {[llength $pages] != 0} {
        TclXHelp::Display "\nHelp pages available in $relativeDir:"
        TclXHelp::DisplayColumns $pages
    }
}


#------------------------------------------------------------------------------
# helpcd command.  The name of the new current directory is assembled from the
# current directory and the argument.

proc helpcd {{dir {}}} {
    variable ::TclXHelp::curSubject
    if {[lempty $dir]} {
        set dir $TclXHelp::pathSep
    }

    set helpFile [lindex [TclXHelp::ConvertHelpFile $dir] 0]

    if {![file isdirectory $helpFile]} {
        error "\"$dir\" is not a subject" \
            [list TCLXHELP NOTSUBJECT $dir]
    }

    set ::TclXHelp::curSubject [TclXHelp::RelativePath $helpFile]
    return
}

#------------------------------------------------------------------------------
# Helpcd main.

proc helppwd {} {
    variable ::TclXHelp::curSubject
    echo "Current help subject: $::TclXHelp::curSubject"
}

#------------------------------------------------------------------------------
# apropos command.  This search the 

proc apropos {regexp} {
    variable ::TclXHelp::lineCnt 0
    variable ::TclXHelp::curSubject

    set ch [scancontext create]
    scanmatch -nocase $ch $regexp {
        set path [lindex $matchInfo(line) 0]
        set desc [lrange $matchInfo(line) 1 end]
        if {![TclXHelp::Display [format "%s - %s" $path $desc]]} {
            set stop 1
            return}
    }
    set stop 0
    foreach dir [TclXHelp::RootDirs] {
        foreach brief [glob -nocomplain [file join $dir *.brf]] {
            set briefFH [open $brief]
            try_eval {
                scanfile $ch $briefFH
            } {} {
                close $briefFH
            }
            if {$stop} break
        }
        if {$stop} break
    }
    scancontext delete $ch
}
