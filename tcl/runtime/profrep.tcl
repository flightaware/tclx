#
# profrep  --
#
# Generate Tcl profiling reports.
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
# $Id: profrep.tcl,v 2.1 1993/04/07 02:42:32 markd Exp markd $
#------------------------------------------------------------------------------
#

#@package: TclX-profrep profrep

#
# Summarize the data from the profile command to the specified significant
# stack depth.  Returns the maximum number of characters in any of the
# procedure names.  (useful in columnizing reports).
#
proc profrep:summarize {profDataVar stackDepth sumProfDataVar} {
    upvar $profDataVar profData $sumProfDataVar sumProfData

    if {(![info exists profData]) || ([catch {array size profData}] != 0)} {
        error "`profDataVar' must be the name of an array returned by the `profile off' command"
    }
    set maxNameLen 0
    foreach procStack [array names profData] {
        foreach procName $procStack {
            set maxNameLen [max $maxNameLen [clength $procName]]
        }
        if {[llength $procStack] < $stackDepth} {
            set sigProcStack $procStack
        } else {
            set sigProcStack [lrange $procStack 0 [expr {$stackDepth - 1}]]
        }
        if [info exists sumProfData($sigProcStack)] {
            set cur $sumProfData($sigProcStack)
            set add $profData($procStack)
            set     new [expr [lindex $cur 0]+[lindex $add 0]]
            lappend new [expr [lindex $cur 1]+[lindex $add 1]]
            lappend new [expr [lindex $cur 2]+[lindex $add 2]]
            set $sumProfData($sigProcStack) $new
        } else {
            set sumProfData($sigProcStack) $profData($procStack)
        }
    }
    return $maxNameLen
}

#
# Generate a list, sorted in descending order by the specified key, contain
# the indices into the summarized data.
#
proc profrep:sort {sumProfDataVar sortKey} {
    upvar $sumProfDataVar sumProfData

    case $sortKey {
        {calls} {set keyIndex 0}
        {real}  {set keyIndex 1}
        {cpu}   {set keyIndex 2}
        default {
            error "Expected a sort type of: `calls', `cpu' or ` real'"}
    }

    # Build a list to sort cosisting of a fix-length string containing the
    # key value and proc stack. Then sort it.

    foreach procStack [array names sumProfData] {
        set key [format "%016d" [lindex $sumProfData($procStack) $keyIndex]]
        lappend keyProcList [list $key $procStack]
    }
    set keyProcList [lsort $keyProcList]

    # Convert the assending sorted list into a descending list of proc stacks.

    for {set idx [expr [llength $keyProcList]-1]} {$idx >= 0} {incr idx -1} {
        lappend sortedProcList [lindex [lindex $keyProcList $idx] 1]
    }
    return $sortedProcList
}

#
# Print the sorted report
#

proc profrep:print {sumProfDataVar sortedProcList maxNameLen outFile
                    userTitle} {
    upvar $sumProfDataVar sumProfData
    
    if {$outFile == ""} {
        set outFH stdout
    } else {
        set outFH [open $outFile w]
    }

    # Output a header.

    set stackTitle "Procedure Call Stack"
    set maxNameLen [max [expr $maxNameLen+6] [expr [clength $stackTitle]+4]]
    set hdr [format "%-${maxNameLen}s %10s %10s %10s" $stackTitle \
                    "Calls" "Real Time" "CPU Time"]
    if {$userTitle != ""} {
        puts $outFH [replicate - [clength $hdr]]
        puts $outFH $userTitle
    }
    puts $outFH [replicate - [clength $hdr]]
    puts $outFH $hdr
    puts $outFH [replicate - [clength $hdr]]

    # Output the data in sorted order.

    foreach procStack $sortedProcList {
        set data $sumProfData($procStack)
        puts $outFH [format "%-${maxNameLen}s %10d %10d %10d" \
                            [lvarpop procStack] \
                            [lindex $data 0] [lindex $data 1] [lindex $data 2]]
        foreach procName $procStack {
            if {$procName == "<global>"} break
            puts $outFH "    $procName"
        }
    }
    if {$outFile != ""} {
        close $outFH
    }
}

#------------------------------------------------------------------------------
# Generate a report from data collect from the profile command.
#   o profDataVar (I) - The name of the array containing the data from profile.
#   o sortKey (I) - Value to sort by. One of "calls", "cpu" or "real".
#   o stackDepth (I) - The stack depth to consider significant.
#   o outFile (I) - Name of file to write the report to.  If omitted, stdout
#     is assumed.
#   o userTitle (I) - Title line to add to output.

proc profrep {profDataVar sortKey stackDepth {outFile {}} {userTitle {}}} {
    upvar $profDataVar profData

    set maxNameLen [profrep:summarize profData $stackDepth sumProfData]
    set sortedProcList [profrep:sort sumProfData $sortKey]
    profrep:print sumProfData $sortedProcList $maxNameLen $outFile $userTitle

}
