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
# $Id: compat.tcl,v 6.0 1996/05/10 16:16:27 markd Exp $
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

#@package: TclX-ServerCompat server_open server_connect server_send \
                             server_info server_cntl

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
    set handle [uplevel [concat $cmd $args]]
    if $buffered {
        lappend handle [dup $handle]
    }
    return $handle
}

# Added TclX 7.5a

proc server_connect args {
    set cmd socket

    set buffered 1
    set twoids 0
    while {[string match -* [lindex $args 0]]} {
        switch -- [set opt [lvarpop args]] {
            -buf {
                set buffered 1
            }
            -nobuf {
                set buffered 0
            }
            -myip {
                lappend cmd -myaddr [lvarpop args]
            }
            -myport {
                lappend cmd -myport [lvarpop args]
            }
            -twoids {
                set twoids 1
            }
            default {
                error "unknown option \"$opt\""
            }
        }
    }
    set handle [uplevel [concat $cmd $args]]
    if !$buffered {
        fconfigure $handle -buffering none 
    }
    if $twoids {
        lappend handle [dup $handle]
    }
    return $handle
}

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

proc server_info args {
    eval [concat host_info $args]
}

proc server_cntl args {
    eval [concat fcntl $args]
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
