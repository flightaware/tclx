#
# Config.mk --
#
#   Master configuration file for Extended Tcl.  This should be the only
# file you have to modify to get Extended Tcl to work.
# 
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
# $Id: Config.mk,v 2.23 1993/07/27 05:17:30 markd Exp markd $
#------------------------------------------------------------------------------
#

#==============================================================================
# Configuration file specification.  Set the macro TCL_CONFIG_FILE to the 
# name of the file to use in the config directory (don't include the directory
# name).  If you find problems with these files or have new onces please send
# them to us (tcl-project@neosoft.com).  At the end of this file is a
# description of all the flags that can be set in the config file.
#
TCL_CONFIG_FILE=scoodt2.0

#==============================================================================
#
# Configuration section.  Modify this section to set some general options and
# selecting a config file for a specific Unix implementation.
#
#==============================================================================

#------------------------------------------------------------------------------
# Location of the UCB Tcl distribution relative to this directory.  TclX works
# with Tcl 7.0
#
TCL_UCB_DIR=../tcl7.0b2

#------------------------------------------------------------------------------
# If you are a Tk user and would like to build a version "wish", the Tk shell,
# that includes the TclX command set, define TK_BUILD=WISHX and the
# location of your Tk directory in TK_UCB_DIR relative to this directory. If
# you do not want a "wishx" compiled, don't define TK_BUILD. The libraries
# required to link Tk are defined in the system specific sections below.

TK_BUILD=WISHX
TK_UCB_DIR=../tk3.3b2

#------------------------------------------------------------------------------
# Compiler debug/optimization/profiling flag to use.  Not that if debugging or
# profiling is enabled, the DO_STRIPPING option below must be disabled.
#
OPTIMIZE_FLAG=-g

#------------------------------------------------------------------------------
# Stripping of the final tclshell binary.  Specify `true' if the binary is to
# be stripped (optimized case) or specify `false' if the binary is not to be
# stripped (debugging case).
#
DO_STRIPPING=false
#DO_STRIPPING=true

#------------------------------------------------------------------------------
# Definition of the compiler you want to use, as well as extra flags for the
# compiler and linker.  Also the yacc program you wish to use.
#
CC=cc
AR=ar
XCFLAGS=
XLDFLAGS=
XLDLIBS=
YACC=yacc
#YACC=bison -b y

#------------------------------------------------------------------------------
# If C++ is to be used these should be used.  Specifying CPLUSOBJS includes the
# C++ support code in the Tcl library. CCPLUS is the command to run your C++
# compiler.
#
#CPLUSOBJS=tcl++.o
CCPLUS=CC

#------------------------------------------------------------------------------
# Memory debugging defines.  These are only of interest if you are adding C
# code to Tcl or debugging Tcl C code. You probably don't need this unless it
# seems like you have memory problems. They help find memory overwrites and
# leaks.  One or more of the following flags may be specified (in the form
# -DFLAGNAME).
#
#    o TCL_MEM_DEBUG - Turn on memory debugging. 
#    o TCL_SHELL_MEM_LEAK - Dump a list of active memory blocks when the
#      shell exits an eof (requires TCL_MEM_DEBUG).
#
# NOTE: If TCL_MEM_DEBUG is enabled, the Berkeley Tcl distribution must be 
# recompiled with this option as well, or it will not link or may fail
# with some mysterious memory problems.  Same goes for Tk if you are using
# Tk.  If this option is to be used, all code being tested MUST be compiled
# with TCL_MEM_DEBUG and use ckalloc and ckfree for all memory passed between
# the application and Tcl.
#
# An addition a flag MEM_VALIDATE may be specified in the Berkeley Tcl
# compilation to do validation of all memory blocks on each allocation or
# deallocation (very slow).
#
MEM_DEBUG_FLAGS=

#==============================================================================
# Install options sections.  This sections describes the installation options.
# Your might want to change some of these values before installing.
#..............................................................................
#
# o TCL_OWNER - The user that will own all Tcl files after installation.
# o TCL_GROUP - The group that all Tcl files will belong to after installation.
#
TCL_OWNER=bin
TCL_GROUP=bin


#
# The master Tcl directory that the Extended Tcl runtime files are installed
# into.  All files are installed in this directory, then symbolic links are
# built from the outside. A directory whose name is the TclX version number
# will be build in this directory.
#
TCL_MASTERDIR=/usr/local/tclX

#
# The master Tk directory that the Tk runtime files are installed into.
# All files are installed in this directory, then symbolic links are
# built from the outside. A directory whose name is the TkX version number
# will be build in this directory.
#
TK_MASTERDIR=/usr/local/tkX

#
# The directory to install Tcl binary into.
#
TCL_BINDIR=/usr/local/bin

#
# The directory tcl.a library goes into.
#
TCL_LIBDIR=/usr/local/lib

#
# The directory the Tcl .h files go into.
#
TCL_INCLUDEDIR=/usr/local/include

#==============================================================================
# These defines specify where and how the manual pages are to be installed.
# They are actually defined in the system specific configuration file in the
# config directory.  Install manual pages is somewhat problematic, so a global
# option not to install manual pages is provided. Since there are so many
# manual pages provided, they are placed together in one Tcl manual page
# directory, rather than splitting into the standard manual pages directories.
# The actual definitions of these variables are set for in the system
# dependent file.  You might want to modify these values.
#..............................................................................

#
# Set to 1 to install manual files, to 0 to not install manual files.
#

TCL_MAN_INSTALL=1

#
# o TCL_MAN_BASEDIR - Base manual directory where all of the man.* and cat.*
#   directories live.
#
TCL_MAN_BASEDIR=/usr/local/man

#
# o TCL_MAN_CMD_SECTION - Section for Tcl command  manual pages. Normal `1' or
#   `C'.  You might perfer TCL since there are some many.
#
# o TCL_MAN_FUNC_SECTION - Section for Tcl C level function manual pages.
#   In some cases it might be desirable install all manual pages in one
#   section, in this case, the value should be the same as TCL_MAN_CMD_SECTION.
#
TCL_MAN_CMD_SECTION=TCL
TCL_MAN_FUNC_SECTION=TCL
#TCL_MAN_CMD_SECTION=1
#TCL_MAN_FUNC_SECTION=3

#
# o TK_MAN_CMD_SECTION - Section for Tk command  manual pages. Normal `1' or
#   `C'.  You might perfer TK since there are some many.
#
# o TK_MAN_FUNC_SECTION - Section for Tk C level function manual pages.
#   In some cases it might be desirable install all manual pages in one
#   section, in this case, the value should be the same as TK_MAN_CMD_SECTION.
#
TK_MAN_CMD_SECTION=TK
TK_MAN_FUNC_SECTION=TK
#TK_MAN_CMD_SECTION=1
#TK_MAN_FUNC_SECTION=3


#..............................................................................
# The rest of the manual page install options are set in the system dependent
# configuration file (config/*)
#

# o MAN_DIR_SEPARATOR - The separator character used in the directory name
#   of the cat* and man* manual directories.  This is usually empty or 
#   a period. i.e "/usr/man/man1" or "/usr/man/man.1".  Autoconf attempts to
#   determine it but it can be overriden here.


#MAN_DIR_SEPARATOR=.
