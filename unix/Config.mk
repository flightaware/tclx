#
# Config.mk --
#
#   Master configuration file for Extended Tcl.  This should be the only
# file you have to modify to get Extended Tcl to work.  It is used to
# set attributes that configure can't figure out and t override certain 
# attributes set by configure.
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
# $Id: Config.mk,v 2.24 1993/07/30 15:05:15 markd Exp markd $
#------------------------------------------------------------------------------
#

#------------------------------------------------------------------------------
# Location of the UCB Tcl distribution relative to this directory (not an
# absolute path).

TCL_UCB_DIR=../tcl7.0b2

#------------------------------------------------------------------------------
# If you are a Tk user and would like to build a version "wish", the Tk shell,
# that includes the TclX command set, define TK_BUILD=WISHX and the
# location of your Tk directory in TK_UCB_DIR relative to this directory. If
# you do not want a "wishx" compiled, don't define TK_BUILD.

TK_BUILD=WISHX
TK_UCB_DIR=../tk3.3b2

#------------------------------------------------------------------------------
# Compiler debug/optimization/profiling flag to use.  Also a macro that
# controls if the binaries will be stripped.  Specify `true' if the binary is
# to be stripped (optimized case) or specify `false' if the binary is not to be
# stripped (debugging case).  Note that if debugging or profiling is enabled,
# the DO_STRIPPING option must be disabled.

OPTIMIZE_FLAG=-g
DO_STRIPPING=false
#DO_STRIPPING=true

#------------------------------------------------------------------------------
# Definition of the compiler ar and yacc program you wish to use.
#
CC=cc
AR=ar
YACC=yacc
#YACC=bison -b y

#------------------------------------------------------------------------------
# EXtra flags to specified:
#   o XCFLAGS - Extra cc flags on all compiles and links
#   o XLDFLAGS - Extra cc flags to specify at link time.
#   o XLDLIBS - Extra libraries to use at link time.

XCFLAGS=
XLDFLAGS=
XLDLIBS=

#------------------------------------------------------------------------------
# If C++ is to be used these should be set.  Specifying CPLUSOBJS includes the
# C++ support code in the Tcl library. CCPLUS is the command to run your C++
# compiler.

#CPLUSOBJS=tcl++.o
CCPLUS=CC

#------------------------------------------------------------------------------
# The master Tcl directory that the Extended Tcl runtime files are installed
# into.  All files are installed in this directory, then symbolic links are
# built from the outside. A directory whose name is the TclX version number
# will be build in this directory.
#
TCL_MASTERDIR=/usr/local/tclX

#------------------------------------------------------------------------------
# The master Tk directory that the Tk runtime files are installed into.
# All files are installed in this directory, then symbolic links are
# built from the outside. A directory whose name is the TkX version number
# will be build in this directory.

TK_MASTERDIR=/usr/local/tkX

#------------------------------------------------------------------------------
# The directory to install Tcl binary into.

TCL_BINDIR=/usr/local/bin

#------------------------------------------------------------------------------
# The directory tcl.a library goes into.

TCL_LIBDIR=/usr/local/lib

#------------------------------------------------------------------------------
# The directory the Tcl .h files go into.

TCL_INCLUDEDIR=/usr/local/include

#==============================================================================
# These defines specify where and how the manual pages are to be installed.
# They are actually defined in the system specific configuration file in the
# config directory.  Install manual pages is somewhat problematic, so a global
# option not to install manual pages is provided. Since there are so many
# manual pages provided, they are placed together in one Tcl manual page
# directory, rather than splitting into the standard manual pages directories.
# You might want to modify these values.
#..............................................................................

#------------------------------------------------------------------------------
# Set to 1 to install manual files, to 0 to not install manual files.

TCL_MAN_INSTALL=1

#------------------------------------------------------------------------------
# o TCL_MAN_BASEDIR - Base manual directory where all of the man.* and cat.*
#   directories live.

TCL_MAN_BASEDIR=/usr/local/man

#------------------------------------------------------------------------------
# o TCL_MAN_CMD_SECTION - Section for Tcl command  manual pages. Normal `1' or
#   `C'.  You might perfer TCL since there are some many.
#
# o TCL_MAN_FUNC_SECTION - Section for Tcl C level function manual pages.
#   In some cases it might be desirable install all manual pages in one
#   section, in this case, the value should be the same as TCL_MAN_CMD_SECTION.

TCL_MAN_CMD_SECTION=TCL
TCL_MAN_FUNC_SECTION=TCL
#TCL_MAN_CMD_SECTION=1
#TCL_MAN_FUNC_SECTION=3

#------------------------------------------------------------------------------
# o TK_MAN_CMD_SECTION - Section for Tk command  manual pages. Normal `1' or
#   `C'.  You might perfer TK since there are some many.
#
# o TK_MAN_FUNC_SECTION - Section for Tk C level function manual pages.
#   In some cases it might be desirable install all manual pages in one
#   section, in this case, the value should be the same as TK_MAN_CMD_SECTION.

TK_MAN_CMD_SECTION=TK
TK_MAN_FUNC_SECTION=TK
#TK_MAN_CMD_SECTION=1
#TK_MAN_FUNC_SECTION=3


#------------------------------------------------------------------------------
# o MAN_DIR_SEPARATOR - The separator character used in the directory name
#   of the cat* and man* manual directories.  This is usually empty or 
#   a period. i.e "/usr/man/man1" or "/usr/man/man.1".  Autoconf attempts to
#   determine it but it can be overriden here.

#MAN_DIR_SEPARATOR=.
