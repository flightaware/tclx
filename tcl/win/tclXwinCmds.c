/*
 * tclXwinCmds.c --
 *
 * Tcl commands to access Win32 functionality and stubs for Unix commands that
 * are not implemented.
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
 * $Id: tclXunixCmds.c,v 7.1 1996/07/18 19:36:32 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*-----------------------------------------------------------------------------
 * Tcl_ChrootCmd --
 *     Implements the TCL chroot command:
 *         chroot path
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ChrootCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * Tcl_TimesCmd --
 *     Implements the TCL times command:
 *     times
 *
 * Results:
 *  Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_TimesCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return TclXNotAvailableError (interp, argv [0]);
}

/*-----------------------------------------------------------------------------
 * Temp stub procedures.
 * FIX: Some of the functionality here needs to be ported
 *-----------------------------------------------------------------------------
 */
int
Tcl_ChmodCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    Tcl_AppendResult (interp, "the ", argv [1], " command has not yet been ",
                      "ported to Windows 95/NT", (char *) NULL);
    return TCL_ERROR;
}

int
Tcl_ChownCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    Tcl_AppendResult (interp, "the ", argv [1], " command has not yet been ",
                      "ported to Windows 95/NT", (char *) NULL);
    return TCL_ERROR;
}

int
Tcl_ChgrpCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    Tcl_AppendResult (interp, "the ", argv [1], " command has not yet been ",
                      "ported to Windows 95/NT", (char *) NULL);
    return TCL_ERROR;
}

int
Tcl_FcntlCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    Tcl_AppendResult (interp, "the ", argv [1], " command has not yet been ",
                      "ported to Windows 95/NT", (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_ServerInit --
 *     
 *   Stub, does nothing.  The Unix version of the function initilizes some
 * compatiblity functions that are not implemented on Win32.
 *-----------------------------------------------------------------------------
 */
void
TclX_ServerInit (interp)
    Tcl_Interp *interp;
{
}
