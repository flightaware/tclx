
# tclhelp.tcl --
#
# Tk program to access Extended Tcl & Tk help pages.  Uses internal functions
# of TclX help command.
# 
#------------------------------------------------------------------------------
# Copyright 1993 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: buildhelp.tcl,v 2.6 1993/06/24 07:31:01 markd Exp $
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
        pack append $parent $nframe {top expand frame w fillx}
    }

    # Set up the button.

    set buttonName $nframe.col$col
    if $isDir {
        button $buttonName -text $fileName -width 18 \
            -command "DisplaySubject $filePath"
    } else {
        button $buttonName -text $fileName -width 18 \
            -command "DisplayPage $filePath"
    }
    pack append $nframe $buttonName {left frame w}
}

#------------------------------------------------------------------------------
# Create a frame to hold buttons for the specified list of either help files
# or directories.

proc ButtonFrame {w title subject fileList} {
    frame $w
    label $w.label -relief flat -text $title -background SlateGray1
    pack append $w $w.label {top fill}
    frame $w.buttons
    pack append $w $w.buttons {top expand frame w}
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
    pack append .tkhelp $frame {top fillx}
    
    ButtonFrame $frame.subjects "Subjects available in $subject" \
        $subject $subjects
    pack append  $frame $frame.subjects {top fillx}

    ButtonFrame $frame.pages "Help files available in $subject" \
        $subject $pages
    pack append  $frame $frame.pages {top fillx}
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
    frame $w.frame -borderwidth 10

    scrollbar $w.frame.yscroll -relief sunken \
        -command "$w.frame.page yview"
    text $w.frame.page -yscroll "$w.frame.yscroll set" \
        -width 80 -height 20 -relief sunken -wrap word
    pack append $w.frame $w.frame.yscroll {right filly} \
                         $w.frame.page    {top}

    if [catch {
            set contents [read_file [help:ConvertPath $page]]
        } msg] {
        set contents $msg
    }
    $w.frame.page insert 0.0 $contents
    $w.frame.page configure -state disabled

    button $w.ok -text OK -command "destroy $w"
    pack append $w $w.frame {top} $w.ok {bottom fill}
}

#------------------------------------------------------------------------------
# Set up the apropos panel.

proc AproposPanel {} {
    catch {destory .tkhelp.apropos}
    frame .tkhelp.apropos {top fillx}
    #???
}

#------------------------------------------------------------------------------
# Tk base help command for Tcl/Tk/TclX.

proc tkhelp {} {
    global TCLENV

    if ![auto_load help] {
        puts stderr "couldn't auto_load TclX 'help' command"
        exit 255
    }
    catch {destroy .tkhelp}
    frame .tkhelp
    pack append . .tkhelp {top fill}

    DisplaySubject "/"

}
catch {close $cmdlog}
set cmdlog [open cmd.log w]
echo @@@@ cmdtracing...
cmdtrace on $cmdlog 
if {[catch {tkhelp}] != 0} {
    echo $errorInfo
}
