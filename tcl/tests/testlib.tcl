#
# testlib.tcl --
#
# Test support routines.  Some of these are based on routines provided with
# standard Tcl.
#------------------------------------------------------------------------------
# Set the global variable TEST_ERROR_INFO to display errorInfo when a test
# fails.
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
# $Id: testlib.tcl,v 2.3 1993/08/13 06:32:33 markd Exp markd $
#------------------------------------------------------------------------------
#

# Save the unknown command in a variable SAVED_UNKNOWN.  To get it back, eval
# that variable.  Don't do this more than once.

global SAVED_UNKNOWN

if {([info command unknown] == "") && ![info exists SAVED_UNKNOWN]} {
    error "can't find either unknown or SAVED_UNKNOWN"
}
if {[info command unknown] != ""} {
    set SAVED_UNKNOWN "proc unknown "
    append SAVED_UNKNOWN "\{[info args unknown]\} "
    append SAVED_UNKNOWN "\{[info body unknown]\}"
    rename unknown {}
}

#
# Output a test error.
#
proc OutTestError {test_name test_description contents_of_test
                   passing_int_result passing_result int_result result} {
    global TEST_ERROR_INFO errorInfo
    set int(0) TCL_OK
    set int(1) TCL_ERROR
    set int(2) TCL_RETURN
    set int(3) TCL_BREAK
    set int(4) TCL_CONTINUE

    puts stdout "==== $test_name $test_description"
    puts stdout "==== Contents of test case:"
    puts stdout "$contents_of_test"
    puts stdout "==== Result was: $int($int_result)"
    puts stdout "$result"
    puts stdout "---- Result should have been: $int($passing_int_result)"
    puts stdout "$passing_result"
    puts stdout "---- $test_name FAILED" 
    if {[info exists TEST_ERROR_INFO] && [info exists errorInfo]} {
        puts stdout $errorInfo
        puts stdout "---------------------------------------------------"
    }
}

#
# Routine to execute tests and compare to expected results.
#
proc Test {test_name test_description contents_of_test passing_int_result
           passing_result} {
    set int_result [catch {uplevel $contents_of_test} result]

    if {($int_result != $passing_int_result) ||
        ($result != $passing_result)} {
        OutTestError $test_name $test_description $contents_of_test \
                     $passing_int_result $passing_result $int_result $result
    }
}

#
# Compare result against case-insensitive regular expression.
#

proc TestReg {test_name test_description contents_of_test passing_int_result
              passing_result} {
    set int_result [catch {uplevel $contents_of_test} result]

    if {($int_result != $passing_int_result) ||
        ![regexp -nocase $passing_result $result]} {
        OutTestError $test_name $test_description $contents_of_test \
                     $passing_int_result $passing_result $int_result $result
    }
}

proc dotests {file args} {
    global TESTS
    set savedTests $TESTS
    set TESTS $args
    source $file
    set TESTS $savedTests
}

# Genenerate a unique file record that can be verified.  The record
# grows quite large to test the dynamic buffering in the file I/O.

proc GenRec {id} {
    return [format "Key:%04d {This is a test of file I/O (%d)} KeyX:%04d %s" \
                    $id $id $id [replicate :@@@@@@@@: $id]]
}

# Proc to fork and exec child that loops until it gets a signal.
# Can optionally set its pgroup.

proc ForkLoopingChild {{setPGroup 0}} {
    unlink -nocomplain {PGROUP.SET}
    flush stdout
    flush stderr
    set newPid [fork]
    if {$newPid != 0} {

        # Wait till the child has set the pgroup.
        if $setPGroup {
            while {![file exists PGROUP.SET]} {
                sleep 1
            }
        unlink PGROUP.SET
        }
        return $newPid
    }
    if $setPGroup {
        id process group set
        close [open PGROUP.SET w]
    }
    execl ../tclmaster/bin/tcl {-qc {catch {while {1} {sleep 1}}; exit 10}}
    error "Should never make it here"
}


