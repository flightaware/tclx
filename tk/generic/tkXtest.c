/* 
 * tkXtest.c --
 *
 *    Tcl_AppInit function for Extended Tcl Tk test program.
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
 * $Id: tkXtest.c,v 5.2 1996/02/09 18:43:57 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtend.h"

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
main (argc, argv)
    int    argc;
    char **argv;
{
    TkX_Main(argc, argv, Tcl_AppInit);
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
    if (TclX_Init (interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (TkX_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (Tktest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    return TCL_OK;
}
