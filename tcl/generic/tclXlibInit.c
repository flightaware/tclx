/*
 * tclXlibInit.c
 *
 * Function to add the Extented Tcl library commands into an interpreter. This
 * also sets the Tcl auto_path, tcl_library and tclx_library variable.
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
 * $Id: tclXlibInit.c,v 5.1 1996/02/12 18:16:01 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Used to override the library and library environment variable used to
 * find the TclX startup file and runtime library.  The values of these
 * fields must be changed before Tclxlib_Init is called.
 */
char *tclX_library    = TCLX_LIBRARY;
char *tclX_libraryEnv = "TCLX_LIBRARY";


/*
 *-----------------------------------------------------------------------------
 * Tclxlib_Init --
 *
 *   Initialize the Extended Tcl library facility commands.  Add the TclX
 * library directory to auto_path and set the tclx_library variable.
 *-----------------------------------------------------------------------------
 */
int
Tclxlib_Init (interp)
    Tcl_Interp *interp;
{
    TclX_LibraryInit (interp);

    return TclX_SetRuntimeLocation (interp, "tclx_library",
                                    tclX_libraryEnv, tclX_library);
}

