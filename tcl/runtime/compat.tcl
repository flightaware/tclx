#
# compat --
#
# This file provides commands compatible with older versions of Extended Tcl.
# 
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
# $Id: compat.tcl,v 5.2 1996/02/12 18:16:44 markd Exp $
#------------------------------------------------------------------------------
#

#@package: TclX-GenCompat assign_fields cexpand

proc assign_fields {list args} {
    puts stderr {**** Your program is using an obsolete TclX proc, "assign_fields".}
    puts stderr {**** Please use the command "lassign". Compatibility support will}
    puts stderr {**** be removed in the next release.}

    proc assign_fields {list args} {
        if [lempty $args] {
            return
        }
        return [uplevel lassign [list $list] $args]
    }
    return [uplevel assign_fields [list $list] $args]
}

# Added TclX 7.4a
proc cexpand str {subst -nocommands -novariables $str}

#@package: TclX-ServerCompat assign_fields cexpand server_open server_send

# Added TclX 7.4a

proc server_open args {
    set cmd server_connect

    set buffered 1
    while {[string match -* [lindex $args 0]]} {
        set opt [lvarpop args]
        if [cequal $opt -buf] {
            set buffered 1
        } elseif  [cequal $opt -nobuf] {
            set buffered 0
        }
        lappend cmd $opt
    }
    set handles [uplevel [concat $cmd $args]]
    if $buffered {
        lappend handles [dup $handles]
    }
    return $handles
}

# Added TclX 7.5a

proc server_send args {
    set cmd puts

    while {[string match -* [lindex $args 0]]} {
        switch -- [set opt [lvarpop args]] {
            {-dontroute} {
                error "server_send if obsolete, -dontroute is not supported by the compatibility proc"
            }
            {-outofband} {
                error "server_send if obsolete, -outofband is not supported by the compatibility proc"
            }
        }
        lappend cmd $opt
    }
    uplevel [concat $cmd $args]
    flush [lindex $args 0]
}

#@package: TclX-ClockCompat fmtclock convertclock getclock

# Added TclX 7.5a

proc fmtclock {clockval {format {}} {zone {}}} {
    lappend cmd clock format $clockval
    if ![lempty $format] {
        lappend cmd -format $format
    }
    if ![lempty $zone] {
        lappend cmd -gmt 1
    }
    return [eval $cmd]
}

# Added TclX 7.5a

proc convertclock {dateString {zone {}} {baseClock {}}} {
    lappend cmd clock scan $dateString
    if ![lempty $zone] {
        lappend cmd -gmt 1
    }
    if ![lempty $baseClock] {
        lappend cmd -base $baseClock
    }
    return [eval $cmd]
}

# Added TclX 7.5a

proc getclock {} {
    return [eval clock seconds]
}
