/*
 * getversion.c
 *
 * Utility program used during installation. Returns the version suffix to
 * add to the master directory name.  This can be compiled to get either the
 * TclX or TkX version depending on if TK_GET_VERSION is defined.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1994 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: getversion.c,v 3.2 1994/01/24 05:52:15 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtend.h"
#include "tclXpatchl.h"

#ifdef TK_GET_VERSION
#include "tk.h"
#endif

int
main (argc, argv)
    int    argc;
    char **argv;
{
#ifdef TK_GET_VERSION
    printf ("%s%s", TK_VERSION, TCL_EXTD_VERSION_SUFFIX);
#else
    printf ("%s%s", TCL_VERSION, TCL_EXTD_VERSION_SUFFIX);
#endif
#ifdef TCLX_PATCHLEVEL
    if (TCLX_PATCHLEVEL != 0) 
        printf ("-p%d", TCLX_PATCHLEVEL);
#endif    
    printf ("\n");
    exit (0);
}
