#
# setfuncs --
#
# Perform set functions on lists.  Also has a procedure for removing duplicate
# list entries.
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
# $Id: setfuncs.tcl,v 5.1 1996/02/12 18:17:00 markd Exp $
#------------------------------------------------------------------------------
#

#@package: TclX-set_functions union intersect intersect3 lrmdups

#
# return the logical union of two lists, removing any duplicates
#
proc union {lista listb} {
    return [lrmdups [concat $lista $listb]]
}

#
# sort a list, returning the sorted version minus any duplicates
#
proc lrmdups list {
    if [lempty $list] {
        return {}
    }
    set list [lsort $list]
    set last [lvarpop list]
    lappend result $last
    foreach element $list {
	if ![cequal $last $element] {
	    lappend result $element
	    set last $element
	}
    }
    return $result
}

#
# intersect3 - perform the intersecting of two lists, returning a list
# containing three lists.  The first list is everything in the first
# list that wasn't in the second, the second list contains the intersection
# of the two lists, the third list contains everything in the second list
# that wasn't in the first.
#

proc intersect3 {list1 list2} {
    set a1(0) {} ; unset a1(0)
    set a2(0) {} ; unset a2(0)
    set a3(0) {} ; unset a3(0)
    foreach v $list1 {
        set a1($v) {}
    }
    foreach v $list2 {
        if [info exists a1($v)] {
            set a2($v) {} ; unset a1($v)
        } {
            set a3($v) {}
        }
    }
    list [array names a1] [array names a2] [array names a3]
}

#
# intersect - perform an intersection of two lists, returning a list
# containing every element that was present in both lists
#
proc intersect {list1 list2} {
    set intersectList ""

    set list1 [lsort $list1]
    set list2 [lsort $list2]

    while {1} {
        if {[lempty $list1] || [lempty $list2]} break

        set compareResult [string compare [lindex $list1 0] [lindex $list2 0]]

        if {$compareResult < 0} {
            lvarpop list1
            continue
        }

        if {$compareResult > 0} {
            lvarpop list2
            continue
        }

        lappend intersectList [lvarpop list1]
        lvarpop list2
    }
    return $intersectList
}


