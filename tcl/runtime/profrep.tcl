#
# profrep  --
#
# Generate Tcl profiling reports.
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
# $Id: profrep.tcl,v 7.1 1996/10/08 07:18:56 markd Exp $
#------------------------------------------------------------------------------
#

#@package: TclX-profrep profrep

#
# Convert the profile array from entries that have only the time spent in
# the proc to the time spend in the proc and all it calls.
#
proc profrep:sum {inDataVar outDataVar} {
    upvar 1 $inDataVar inData $outDataVar outData
    
    foreach inStack [array names inData] {
        for {set idx 0} {![lempty [set part [lrange $inStack $idx end]]]} \
                {incr idx} {
            if ![info exists outData($part)] {
                set outData($part) {0 0 0}
            }
            lassign $outData($part) count real cpu
            if {$idx == 0} {
                incr count [lindex $inData($inStack) 0]
            }
            incr real [lindex $inData($inStack) 1]
            incr cpu [lindex $inData($inStack) 2]
            set outData($part) [list $count $real $cpu]
        }
    }
}

#
# Do sort comparison.  May only be called by profrep:sort, as it address its
# local variables.
#
proc profrep:sortcmp {key1 key2} {
    upvar profData profData keyIndex keyIndex
    
    set val1 [lindex $profData($key1) $keyIndex]
    set val2 [lindex $profData($key2) $keyIndex]

    if {$val1 < $val2} {
        return -1
    }
    if {$val1 > $val2} {
        return 1
    }
    return 0
}

#
# Generate a list, sorted in descending order by the specified key, contain
# the indices into the summarized data.
#
proc profrep:sort {profDataVar sortKey} {
    upvar $profDataVar profData

    case $sortKey {
        {calls} {set keyIndex 0}
        {real}  {set keyIndex 1}
        {cpu}   {set keyIndex 2}
        default {
            error "Expected a sort type of: `calls', `cpu' or ` real'"
        }
    }

    return [lsort -integer -decreasing -command profrep:sortcmp \
            [array names profData]]
}

#
# Print the sorted report
#
proc profrep:print {profDataVar sortedProcList outFile userTitle} {
    upvar $profDataVar profData
    
    set maxNameLen 0
    foreach procStack [array names profData] {
        foreach procName $procStack {
            set maxNameLen [max $maxNameLen [clength $procName]]
        }
    }

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
        set data $profData($procStack)
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
#   o outFile (I) - Name of file to write the report to.  If omitted, stdout
#     is assumed.
#   o userTitle (I) - Title line to add to output.

proc profrep {profDataVar sortKey {outFile {}} {userTitle {}}} {
    upvar $profDataVar profData

    profrep:sum profData sumProfData
    set sortedProcList [profrep:sort sumProfData $sortKey]
    profrep:print sumProfData $sortedProcList $outFile $userTitle

}
