#
# Config.mk --
#
#   Master configuration file for Extended Tcl.  This should be the only
# file you have to modify to get Extended Tcl to work.  It is used to
# set attributes that configure can't figure out and to override certain 
# attributes set by configure.
# 
#   All the values in this directory are set to reasonable defaults.  You might
# want to tune them to your taste.  You may set the value of "CC" and "CFLAGS"
# in the file or on the make command line or set them.  For example:
#
#       make -k CC=gcc CFLAGS=-O
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
# $Id: Config.mk,v 5.5 1996/02/09 18:44:03 markd Exp $
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
#              READ THIS FIRST: FILE PATH SPECIFICATION RULES.
#------------------------------------------------------------------------------
# All paths to files outside of the distribution MUST follow these rules.
# The rules make it easy to specify locations of files either relative to
# the source or build directories or or as absolute directories.  If these
# rules are not followed, it will not build.  All values are defaulted to
# reasonable locations.  If Tcl and Tk are in directories that are siblings
# of the TclX source directory, things will probably work just fine.
#
# File paths MUST be one of:
#
#   o Absolute paths (starting with /), e.g.  /usr/local/lib/libtcl.a
#   o Paths relative to the source directory make macro ${srcbasedir}, e.g.
#     -I$(srcbasedir)/../tk4.0
#   o Paths relative to the build directory make macro ${bldbasedir}, e.g.
#     ${bldbasedir}/../tk4.0/libtk.a
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# The Tcl source distribution directory and the path to the Tcl library
# (libtcl.a),  Note, access is required to tclInt.h which is not installed
# by Tcl.

TCL_SRC=${srcbasedir}/../tcl7.5b1
TCL_LIB=${bldbasedir}/../tcl7.5b1/unix/libtcl7.5.a

#------------------------------------------------------------------------------
# If you are a Tk user and would like to build a version "wish", the Tk shell,
# that includes the TclX command set, define TK_BUILD=WISHX.  Also define the
# the directory containing the Tk source distribution and the path to the Tk
# library (libtk.a).

TK_BUILD = WISHX
TK_SRC=${srcbasedir}/../tk4.1b1
TK_LIB=${bldbasedir}/../tk4.1b1/unix/libtk4.1.a

#------------------------------------------------------------------------------
# C compiler and debug/optimization/profiling flag to use.  Set by configure,
# and are normally overridden on the make command line (make CFLAGS=-g).  The
# can also be overridden here.

#CC=cc
#CFLAGS=-O

#------------------------------------------------------------------------------
# Definition of programs you wish to use. RANLIB is set by configure in the
# Makefiles, but they can be overridden here.
#

#RANLIB=ranlib

AR=ar
STRIP=strip

#------------------------------------------------------------------------------
# X is often in strange places, override what configure figured out if
# its wrong.

#XINCLUDES=-I/usr/somewhere/include
#XLIBSW=-L/usr/somewhere/lib -lX11

#------------------------------------------------------------------------------
# EXtra flags:
#   o XCFLAGS - Extra compiler flags on all compiles and links
#   o XLDFLAGS - Extra compiler flags to specify at link time.
#   o XLDLIBS - Extra libraries to use at link time.

XCFLAGS=
XLDFLAGS=
XLDLIBS=

#------------------------------------------------------------------------------
# Library arguments to use when building share library versions of the TclX
# programs. This can be user to set absolute paths to shared libraries, etc.
# This is used on the "make shlink" target.  Shared libraries must still be
# built by hand, but this makes it easier to link the programs.  See
# README.SHLIB for details.
#
TCLX_SHLIBS=-L${TCL_LIBDIR} -ltclx -ltcl
TKX_SHLIBS=-L${TCL_LIBDIR} -ltkx -ltk

#------------------------------------------------------------------------------
# If C++ is to be used to compile support for the class in tcl++.h, then
# the variable TCLXX should be set to "YES", if support is not compiled in,
# it should be set to "NO".   Normally this is set by configure based on
# if a C++ compiler is found.  It can be overridden here.
#
# CXX is the C++ compiler to use and CXXFLAGS are the debug option flags.
# These are set by configure, and are normally overridden on the make command
# line, but can also be changed here.
#

#TCLXX=YES
#TCLXX=NO
#CXX=CC
#CXXFLAGS=-O


#------------------------------------------------------------------------------
# The following definition can be set to non-null for special systems
# like AFS with replication.  It allows the pathnames used for installation
# to be different than those used for actually reference files at
# run-time.  INSTALL_ROOT is prepended to $prefix and $exec_prefix
# when installing files.
INSTALL_ROOT =

#------------------------------------------------------------------------------
# The master Tcl directory that the Extended Tcl runtime files are installed
# into.  All files are installed in this directory, then symbolic links are
# built from the outside. A directory whose name is the TclX version number
# will be built in this directory.
#
TCL_MASTERDIR=${prefix}/tclX

#------------------------------------------------------------------------------
# The master Tk directory that the Tk runtime files are installed into.
# All files are installed in this directory, then symbolic links are
# built from the outside. A directory whose name is the TkX version number
# will be built in this directory.

TK_MASTERDIR=${prefix}/tkX

#------------------------------------------------------------------------------
# # The directory to install the tcl, wishx and tclhelp binaries into.

TCL_BINDIR=${exec_prefix}/bin

#------------------------------------------------------------------------------
# The directory to install the libtcl.a and libtclx.a libraries into.

TCL_LIBDIR=${exec_prefix}/lib

#------------------------------------------------------------------------------
# The directory the Tcl .h files go into.

TCL_INCLUDEDIR=${prefix}/include

#==============================================================================
# These defines specify where and how the manual pages are to be installed.
# Since there are so many manual pages provided, they are placed together in
# one Tcl manual page directory by default, rather than splitting into the
# standard manual pages directories. You might want to modify these values.
#..............................................................................

#------------------------------------------------------------------------------
# o TCL_MAN_BASEDIR - Base manual directory where all of the man* and cat*
#   directories live.

TCL_MAN_BASEDIR=${prefix}/man

#------------------------------------------------------------------------------
# o TCL_MAN_CMD_SECTION - Section for Tcl command manual pages.
#
# o TCL_MAN_FUNC_SECTION - Section for Tcl C level function manual pages.
#

TCL_MAN_CMD_SECTION=n
TCL_MAN_FUNC_SECTION=3

#------------------------------------------------------------------------------
# o TK_MAN_CMD_SECTION - Section for Tk command  manual pages.
#
# o TK_MAN_UNIXCMD_SECTION - Section for Tk Unix commands (the wish program)
#   manual pages.
#
# o TK_MAN_FUNC_SECTION - Section for Tk C level function manual pages.
#
TK_MAN_CMD_SECTION=n
TK_MAN_UNIXCMD_SECTION=1
TK_MAN_FUNC_SECTION=3


#------------------------------------------------------------------------------
# o MAN_DIR_SEPARATOR - The separator character used in the directory name
#   of the cat* and man* manual directories.  This is usually empty or 
#   a period. i.e "/usr/man/man1" or "/usr/man/man.1".  Autoconf attempts to
#   determine it but it can be overridden here.

#MAN_DIR_SEPARATOR=.
