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
 * $Id: tclXwinCmds.c,v 7.5 1996/09/03 23:39:10 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*-----------------------------------------------------------------------------
 * Tcl_ChrootCmd --
 *   Stub to return an error if the chroot command is used on Windows.
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
 *   Stub to return an error if the times command is used on Windows.
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
 * Tcl_SelectCmd --
 *   Stub to return an error if the select command is used on Windows.
 *-----------------------------------------------------------------------------
 */
int
Tcl_SelectCmd (ClientData  clientData,
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
