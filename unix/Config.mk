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
# $Id: Config.mk,v 5.13 1996/03/10 04:43:01 markd Exp $
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
#   o Absolute paths (starting with /), e.g.  /usr/local/lib/libtcl.a
#   o Paths relative to the source directory make macro ${srcbasedir}, e.g.
#     -I$(srcbasedir)/../tk4.0
#   o Paths relative to the build directory make macro ${bldbasedir}, e.g.
#     ${bldbasedir}/../tk4.0/libtk.a
#
# Other macros used in file paths:
#   o TCLX_SYM_VERSION is the full TclX version.
#   o TKX_SYM_VERSION is the full TkX version.
#   o ARCH is either ".arch", as specified to configure, or empty.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# The Tcl source distribution directory, the path to tclConfig.sh, the Tcl
# library (libtcl7.4.a) and the flags neccessary to link with the Tcl shared
# library (libtcl7.4.so).  Note, access is required to tclInt.h which is not
# installed by Tcl.

TCL_SRC=${srcbasedir}/../tcl7.5b3
TCL_CONFIG=${srcbasedir}/../tcl7.5b3/unix/tclConfig.sh
TCL_LIB=${bldbasedir}/../tcl7.5b3/unix/libtcl7.5.a
TCL_SHLIB_DIR=${bldbasedir}/../tcl7.5b3/unix
TCL_SHLIB=-L${TCL_SHLIB_DIR} -ltcl7.5

#------------------------------------------------------------------------------
# Unless configure is going to be run with --with-tk=NO, these defines must be
# set.  They define the directory containing the Tk source distribution, the
# path to tkConfig.sh, the path to the Tk library (libtk4.1.a) and the flags
# neccessary to link with the Tk shared library (libtk4.1.so).

TK_SRC=${srcbasedir}/../tk4.1b3
TK_CONFIG=${srcbasedir}/../tk4.1b3/unix/tkConfig.sh
TK_LIB=${bldbasedir}/../tk4.1b3/unix/libtk4.1.a
TK_SHLIB_DIR=${bldbasedir}/../tk4.1b3/unix
TK_SHLIB=-L${TK_SHLIB_DIR} -ltk4.1

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
# The following definition can be set to non-null for special systems
# like AFS with replication.  It allows the pathnames used for installation
# to be different than those used for actually reference files at
# run-time.  INSTALL_ROOT is prepended to $prefix and $exec_prefix
# when installing files.

INSTALL_ROOT =

#------------------------------------------------------------------------------
# The TclX runtime directory that the Extended Tcl runtime files are installed
# into.

TCLX_INST_RUNTIME=${prefix}/tclX/${TCLX_SYM_VERSION}

#------------------------------------------------------------------------------
# The TkX runtimes that the Tk runtime files are installed into.  A directory
# whose name is the TkX version number will be built in this directory.

TKX_INST_RUNTIME=${prefix}/tkX/${TKX_SYM_VERSION}

#------------------------------------------------------------------------------
# The directory to install the tcl, wishx and tclhelp binaries in when the
# standard install model is used.

TCLX_BIN=${exec_prefix}/bin${ARCH}

#------------------------------------------------------------------------------
# The directory to install the libtclx.a library in when the standard install
# model is used.

TCLX_LIB=${exec_prefix}/lib${ARCH}

#------------------------------------------------------------------------------
# The directory the TclX .h files go in when the standard install model is
# used.

TCLX_INCL=${prefix}/include

#==============================================================================
# These defines specify where and how the manual pages are to be installed.
# Since there are so many manual pages provided, they are placed together in
# one Tcl manual page directory by default, rather than splitting into the
# standard manual pages directories. You might want to modify these values.
#..............................................................................

#------------------------------------------------------------------------------
# o TCLX_MAN - Base manual directory where all of the man* and cat*
#   directories live.

TCLX_MAN=${prefix}/man

#------------------------------------------------------------------------------
# o TCLX_MAN_CMD_SECTION - Section for Tcl command manual pages.
# o TCLX_MAN_FUNC_SECTION - Section for Tcl C level function manual pages.
#

TCLX_MAN_CMD_SECTION=n
TCLX_MAN_FUNC_SECTION=3

#------------------------------------------------------------------------------
# o TKX_MAN_CMD_SECTION - Section for Tk command  manual pages.
# o TKX_MAN_UNIXCMD_SECTION - Section for Tk Unix commands (the wish program)
#   manual pages.
#
# o TK_MAN_FUNC_SECTION - Section for Tk C level function manual pages.
#
TKX_MAN_CMD_SECTION=n
TKX_MAN_UNIXCMD_SECTION=1
TKX_MAN_FUNC_SECTION=3


#------------------------------------------------------------------------------
# o MAN_DIR_SEPARATOR - The separator character used in the directory name
#   of the cat* and man* manual directories.  This is usually empty or 
#   a period. i.e "/usr/man/man1" or "/usr/man/man.1".  Autoconf attempts to
#   determine it but it can be overridden here.

#MAN_DIR_SEPARATOR=.
