/*
 * getversion.c
 *
 * Utility program used during installation. Returns the version suffix to
 * add to the master directory name.  This can be compiled to get either the
 * TclX or TkX version depending on if TK_GET_VERSION is defined.  If argv [1]
 * is specified, it is preappended to the version number.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1993 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id$
 *-----------------------------------------------------------------------------
 */

#include "../src/tclExtend.h"

#ifdef TK_GET_VERSION
#include "tk.h"
#endif

int
main (argc, argv)
    int    argc;
    char **argv;
{
    char *masterDir;

    masterDir = (argv [1] != NULL) ? argv [1] : "";

#ifdef TK_GET_VERSION
    printf ("%s%s%s\n", masterDir, TK_VERSION, TCL_EXTD_VERSION_SUFFIX);
#else
    printf ("%s%s%s\n", masterDir, TCL_VERSION, TCL_EXTD_VERSION_SUFFIX);
#endif
    exit (0);
}
