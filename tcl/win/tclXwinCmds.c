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
 * $Id: tclXwinCmds.c,v 7.2 1996/07/26 05:56:30 markd Exp $
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
Tcl_ChrootCmd (ClientData  clientData,
               Tcl_Interp *interp,
               int         argc,
               char      **argv)
{
    return TclXNotAvailableError (interp, argv [0]);
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
Tcl_TimesCmd (ClientData  clientData,
              Tcl_Interp *interp,
              int         argc,
              char      **argv)
{
    return TclXNotAvailableError (interp, argv [0]);
}

/*-----------------------------------------------------------------------------
 * TclX_ServerInit --
 *     
 *   Stub, does nothing.  The Unix version of the function initilizes some
 * compatiblity functions that are not implemented on Win32.
 *-----------------------------------------------------------------------------
 */
void
TclX_ServerInit (Tcl_Interp *interp)
{
}
