#
# testlib.tcl --
#
# Test support routines.  Some of these are based on routines provided with
# standard Tcl.
#------------------------------------------------------------------------------
# Set the global variable or environment variable TEST_ERROR_INFO to display
# errorInfo when a test fails.
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
# $Id: testlib.tcl,v 7.0 1996/06/16 05:33:03 markd Exp $
#------------------------------------------------------------------------------
#

# Save the unknown command in a variable SAVED_UNKNOWN.  To get it back, eval
# that variable.  Don't do this more than once.

global SAVED_UNKNOWN TCL_PROGRAM env TEST_ERROR_INFO tcl_platform

if [info exists env(TEST_ERROR_INFO)] {
    set TEST_ERROR_INFO 1
}
# Check configuration information that will determine which tests
# to run.  To do this, create an array testConfig.  Each element
# has a 0 or 1 value, and the following elements are defined:
#	unixOnly -	1 means this is a UNIX platform, so it's OK
#			to run tests that only work under UNIX.
#	pcOnly -	1 means this is a PC platform, so it's OK to
#			run tests that only work on PCs.
#	unixOrPc -	1 means this is a UNIX or PC platform.
#	tempNotPc -	The inverse of pcOnly.  This flag is used to
#			temporarily disable a test.

catch {unset testConfig}
if {$tcl_platform(platform) == "unix"} {
    set testConfig(unixOnly) 1
    set testConfig(tempNotPc) 1
} else {
    set testConfig(unixOnly) 0
} 
if {$tcl_platform(platform) == "windows"} {
    set testConfig(pcOnly) 1
} else {
    set testConfig(pcOnly) 0
}
set testConfig(unixOrPc) [expr $testConfig(unixOnly) || $testConfig(pcOnly)]

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
# Convert a Tcl result code to a string.
#
proc TestResultCode code {
    switch -- $code {
        0 {return TCL_OK}
        1 {return TCL_ERROR}
        2 {return TCL_RETURN}
        3 {return TCL_BREAK}
        4 {return TCL_CONTINUE}
        default {return "***Unknown error code $code***"}
    }
}

#
# Output a test error.
#
proc OutTestError {test_name test_description contents_of_test
                   passing_int_result passing_result int_result result} {
    global TEST_ERROR_INFO errorInfo errorCode

    puts stderr "==== $test_name $test_description"
    puts stderr "==== Contents of test case:"
    puts stderr "$contents_of_test"
    puts stderr "==== Result was: [TestResultCode $int_result]"
    puts stderr "$result"
    puts stderr "---- Result should have been: [TestResultCode $passing_int_result]"
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
           passing_result {constraints {}}} {

    # Check constraints to see if we should run this test.
    foreach constraint $constraints {
        if {![info exists testXConfig($constraint)] ||
            !$testConfig($constraint)} {
                return
        }
    }

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

