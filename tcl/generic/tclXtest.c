/* 
 * tclXtest.c --
 *
 *    Tcl_AppInit and main functions for the Extended Tcl test program.
 *
 *-----------------------------------------------------------------------------
 * Copyright 1991-1996 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXtest.c,v 5.7 1996/03/14 05:04:05 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtend.h"

/*
 * Error handler proc that causes errors to come out in the same format as
 * the standard Tcl test shell.  This keeps certain Tcl tests from reporting
 * errors.
 */
static char errorHandler [] =
    "proc TclXTestError msg {global errorInfo; puts stderr $errorInfo; exit 1}\n \
     set tclx_errorHandler TclXTestError";

/*
 * The following variable is a special hack that insures the tcl
 * version of matherr() is used when linking against shared libraries.
 * Even if matherr is not used on this system, there is a dummy version
 * in libtcl.
 */
EXTERN int matherr ();
int (*tclDummyMathPtr)() = matherr;


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *----------------------------------------------------------------------
 */
int
main (argc, argv)
    int    argc;
    char **argv;
{
    TclX_Main(argc, argv, Tcl_AppInit);
    return 0;			/* Needed only to prevent compiler warning. */
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *  Initialize TclX test application.
 *
 * Results:
 *      Returns a standard Tcl completion code, and leaves an error
 *      message in interp->result if an error occurs.
 *----------------------------------------------------------------------
 */
int
Tcl_AppInit (interp)
    Tcl_Interp *interp;
{
    if (Tcl_Init (interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tclx_Init (interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcltest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

    return Tcl_GlobalEval (interp, errorHandler);
}
