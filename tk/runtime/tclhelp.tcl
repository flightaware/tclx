
# tclhelp.tcl --
#
# Tk program to access Extended Tcl & Tk help pages.  Uses internal functions
# of TclX help command.
# 
#------------------------------------------------------------------------------
# Copyright 1993-1995 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: tclhelp.tcl,v 4.0 1994/07/16 05:30:47 markd Rel markd $
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Add a button associated with either a file or directory in the help tree.
# This handles creating frames to hold each row of buttons.  Buttons should
# be delivered in accending row order, and accending column order within each
# row.  Special handling is done for '..'/

proc AddButton {parent subject fileName row col} {

    # Prep name to use in button and other file info.

    set isDir [string match */ $fileName]
    if [string match */ $subject] {
        set subject [csubstr $subject 0 len-1]
    }
    if {$fileName == ".."} {
        set fileName "<Up>"
        set filePath [file dirname $subject]
        set isDir 1
    } else {
        set filePath ${subject}/${fileName}
    }

    # Set up a row frame, if needed.

    set nframe $parent.row$row
    if {$col == 0} {
        frame $nframe
        pack $nframe -side top -expand 1 -anchor w -fill x
    }

    # Set up the button.

    set buttonName $nframe.col$col
    if $isDir {
        button $buttonName -text $fileName -width 20 \
            -command "DisplaySubject $filePath"
    } else {
        button $buttonName -text $fileName -width 20 \
            -command "DisplayPage $filePath"
    }
    pack $buttonName -side left -anchor w
}

#------------------------------------------------------------------------------
# Create a frame to hold buttons for the specified list of either help files
# or directories.

proc ButtonFrame {w title subject fileList} {
    frame $w
    label $w.label -relief flat -text $title -background SlateGray1
    pack $w.label -side top -fill both
    frame $w.buttons
    pack $w.buttons -side top -expand 1 -anchor w
    set col 0
    set row 0
    while {![lempty $fileList]} {
        AddButton $w.buttons $subject [lvarpop fileList] $row $col
        if {[incr col] >= 5} {
            incr row
            set col 0
        }
    }
}

#------------------------------------------------------------------------------
# Display the panels contain the subjects (directories) and the help files for
# a given directory.

proc DisplaySubject {subject} {

    help:ListSubject $subject [help:ConvertPath $subject] subjects pages
    if {$subject != "/"} {
        lvarpush subjects ".."
    }

    set frame .tkhelp.pick
    catch {destroy $frame}
    frame $frame
    pack $frame -side top -fill x
    
    ButtonFrame $frame.subjects "Subjects available in $subject" \
        $subject $subjects
    pack $frame.subjects -side top -fill x

    ButtonFrame $frame.pages "Help files available in $subject" \
        $subject $pages
    pack $frame.pages -side top -fill x
}

#------------------------------------------------------------------------------
# Display a file in a top-level text window.

proc DisplayPage {page} {
    set fileName [file tail $page]

    set w ".tkhelp-[translit "." "_" $page]"

    catch {destroy $w}
    toplevel $w

    wm title $w "Help on '$page'"
    wm iconname $w "Help: $page"
    wm minsize $w 1 1
    frame $w.frame -borderwidth 10

    scrollbar $w.frame.yscroll -relief sunken \
        -command "$w.frame.page yview"
    text $w.frame.page -yscroll "$w.frame.yscroll set" \
        -width 80 -height 20 -relief sunken -wrap word
    pack $w.frame.yscroll -side right -fill y
    pack $w.frame.page -side top -expand 1 -fill both

    if [catch {
            set contents [read_file [help:ConvertPath $page]]
        } msg] {
        set contents $msg
    }
    $w.frame.page insert 0.0 $contents
    $w.frame.page configure -state disabled

    button $w.dismiss -text Dismiss -command "destroy $w"
    pack $w.frame -side top -fill both -expand 1
    pack $w.dismiss -side bottom -fill x
}

#------------------------------------------------------------------------------
# Set up the apropos panel.

proc AproposPanel {} {
    global aproposEntryNumber aproposReferenceFrame referenceFrameItem

    set aproposEntryNumber 0
    catch {destory .apropos}
    toplevel .apropos
    wm minsize .apropos 1 1
    frame .apropos.entryFrame
    pack .apropos.entryFrame -side top -fill x

    label .apropos.entryFrame.label -text "Search for"
    pack .apropos.entryFrame.label -side left

    entry .apropos.entryFrame.entry -relief sunken
    pack .apropos.entryFrame.entry -side left -fill x -expand 1

    bind .apropos.entryFrame.entry <Return> PerformAproposSearch

    frame .apropos.canvasFrame
    set w .apropos.canvasFrame

    canvas $w.canvas -yscroll "$w.yscroll set" -xscroll "$w.xscroll set" \
        -width 15c -height 5c -relief sunken

    scrollbar $w.yscroll -relief sunken \
        -command "$w.canvas yview"

    scrollbar $w.xscroll -relief sunken -orient horiz \
        -command "$w.canvas xview"

    pack $w.xscroll -side bottom -fill x
    pack $w.yscroll -side right -fill y
    pack $w.canvas -in $w -expand yes -fill both
    pack $w -side top -expand yes -fill both

    # put in the dismiss button
    set w .apropos.buttonFrame
    frame $w
    pack $w -side top -fill x
    button $w.dismiss -text Dismiss -command "destroy .apropos"
    pack $w.dismiss -side bottom -fill x
}

#---------------------------------------------------------------------------
#put a line in the reference display for this apropos entry we've discovered
#
proc DisplayAproposReference {path description} {
    global aproposEntryNumber aproposReferenceFrame

    set frame $aproposReferenceFrame.e$aproposEntryNumber
    frame $frame
    pack $frame -side top -anchor w

    button $frame.button -text $path -width 30 \
        -command "DisplayPage /$path"
    pack $frame.button -side left

    label $frame.label -text $description
    pack $frame.label -side left

    incr aproposEntryNumber
}

#---------------------------------------------------------------------------
#the actual search is cadged from "apropos" in the tclx help system
#
proc PerformAproposSearch {} {
    global TCLXENV referenceFrameItem aproposEntryNumber aproposReferenceFrame

    #  start variables and clean up any residue from previous searches
    set w .apropos.canvasFrame
    set aproposEntryNumber 0
    .apropos.canvasFrame.canvas delete all
    set aproposReferenceFrame $w.canvas.frame
    catch "destroy $aproposReferenceFrame"
    catch "destroy .apropos.canvasFrame.failed"

    # create the frame we'll pack matches into and put it into the canvas
    frame $aproposReferenceFrame
    set referenceFrameItem \
        [$w.canvas create window 2 2 -window $aproposReferenceFrame -anchor nw]

    set regexp [.apropos.entryFrame.entry get]
    focus none

    set TCLXENV(help:lineCnt) 0

    # set up scan context
    set ch [scancontext create]
    scanmatch -nocase $ch $regexp {
        set path [lindex $matchInfo(line) 0]
        set desc [lrange $matchInfo(line) 1 end]
        DisplayAproposReference $path $desc
    }

    # perform search
    foreach dir [help:RootDirs] {
        foreach brief [glob -nocomplain $dir/*.brf] {
            set briefFH [open $brief]
            scanfile $ch $briefFH
            close $briefFH
        }
    }

    # delete scan context
    scancontext delete $ch

    # force display to update so we can find out our bounding box
    update

    # if nothing matched, complain
    if {$aproposEntryNumber == 0} {
        label $aproposReferenceFrame.failed -text "NOTHING MATCHED."
        pack $aproposReferenceFrame.failed -side left
    }

    # set the canvas scrollregion to the size of the bounding box
    lassign [.apropos.canvasFrame.canvas bbox $referenceFrameItem] \
        dummy dummy xSize ySize
    .apropos.canvasFrame.canvas configure -scrollregion \
	"0 0 $xSize $ySize"
}

#------------------------------------------------------------------------------
# Set up the command buttons.

proc CreateCommandButtons {frameName} {
    frame $frameName

    button $frameName.quit -text "Quit" -command exit
    pack $frameName.quit -side left

    button $frameName.apropos -text "Apropos" -command AproposPanel
    pack $frameName.apropos -side left
}

#------------------------------------------------------------------------------
# Tk base help command for Tcl/Tk/TclX.  Directories in args are pushed on the
# path so that they are included in help search.

proc tkhelp addPaths {
    global auto_path
    if ![auto_load help] {
        puts stderr "couldn't auto_load TclX 'help' command"
        exit 255
    }
    foreach dir $addPaths {
        lvarpush auto_path $dir
    }
    CreateCommandButtons .command
    pack .command -side top -fill x

    catch {destroy .tkhelp}
    frame .tkhelp
    pack .tkhelp -side top -fill both

    DisplaySubject "/"

}

if !$tcl_interactive {
    if [catch {
        tkhelp $argv
    } msg] {
        tkerror $msg
    }
}
