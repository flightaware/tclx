#
# testlib.tcl --
#
# Test support routines.  Some of these are based on routines provided with
# standard Tcl.
#------------------------------------------------------------------------------
# Set the global variable or environment variable TEST_ERROR_INFO to display
# errorInfo when a test fails.
#------------------------------------------------------------------------------
# Copyright 1992-1995 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: testlib.tcl,v 4.3 1995/07/01 17:50:09 markd Exp markd $
#------------------------------------------------------------------------------
#

# Save the unknown command in a variable SAVED_UNKNOWN.  To get it back, eval
# that variable.  Don't do this more than once.

global SAVED_UNKNOWN TCL_PROGRAM env TEST_ERROR_INFO

if [info exists env(TEST_ERROR_INFO)] {
    set TEST_ERROR_INFO 1
}

#
# Save path to Tcl program to exec, use it when running children in the
# tests.
#
if ![info exists TCL_PROGRAM] {
    if [info exists env(TCL_PROGRAM)] {
       set TCL_PROGRAM $env(TCL_PROGRAM)
    } else {
       puts stderr ""
       puts stderr {WARNING: No environment variable TCL_PROGRAM,}
       puts stderr {         assuming "tcl" as program to use for}
       puts stderr {         running subprocesses in the tests.}
       puts stderr ""
       set TCL_PROGRAM tcl
    }
}

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
    global TEST_ERROR_INFO errorInfo errorCode
    set int(0) TCL_OK
    set int(1) TCL_ERROR
    set int(2) TCL_RETURN
    set int(3) TCL_BREAK
    set int(4) TCL_CONTINUE

    puts stderr "==== $test_name $test_description"
    puts stderr "==== Contents of test case:"
    puts stderr "$contents_of_test"
    puts stderr "==== Result was: $int($int_result)"
    puts stderr "$result"
    puts stderr "---- Result should have been: $int($passing_int_result)"
    puts stderr "$passing_result"
    puts stderr "---- $test_name FAILED" 
    if {[info exists TEST_ERROR_INFO] && [info exists errorInfo]} {
        puts stderr $errorCode
        puts stderr $errorInfo
        puts stderr "---------------------------------------------------"
    }
}

#
# Routine to execute tests and compare to expected results.
#
proc Test {test_name test_description contents_of_test passing_int_result
           passing_result} {
    set int_result [catch {uplevel $contents_of_test} result]

    if {($int_result != $passing_int_result) ||
        ![cequal $result $passing_result]} {
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
# Can optionally set its pgroup.  Wait till child has actually execed or
# kill breaks on some systems (i.e. AIX).

proc ForkLoopingChild {{setPGroup 0}} {
    global TCL_PROGRAM
    close [open CHILD.RUN w]
    flush stdout
    flush stderr
    set newPid [fork]
    if {$newPid != 0} {
        # Wait till the child is actually running.
        while {[file exists CHILD.RUN]} {
            sleep 1
        }
        return $newPid
    }
    if $setPGroup {
        id process group set
    }
    catch {
        execl $TCL_PROGRAM \
            {-qc {unlink CHILD.RUN; catch {while {1} {sleep 1}}; exit 10}}
    } msg
    puts stderr "execl failed (ForkLoopingChild): $msg"
    exit 1
}

#
# Create a file.  If the directory doesn't exist, create it.
#
proc tcltouch file {
    if ![file exists [file dirname $file]] {
        mkdir -path [file dirname $file]
    }
    close [open $file w]
}

