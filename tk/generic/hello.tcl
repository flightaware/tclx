#!/bin/sh
# The next line restarts using tcl \
exec tcl $0 ${1+"$@"}

# List here all packages used in this file
package require Tcl 8.0
package require Tclx 8.0.3
package require Tk 8.0
package require Tkx 8.0.3

# Here the Tcl-script starts:
frame .hello -text "Hello, World"
button .exit -text Exit -command {destroy .}
pack .hello .exit
