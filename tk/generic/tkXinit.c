/*
 * tkXinit.c --
 *
 * Initialization code for the wishx and other Tk & Extended Tcl based
 * applications.
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
 * $Id: tkXinit.c,v 5.4 1996/02/12 18:17:17 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"
#include "tk.h"

/*
 * Used to override the library and library environment variable used to
 * find the TkX startup file and runtime library.  The values of these
 * fields must be changed before TkX_Init is called.
 */
char *tkX_library    = TKX_LIBRARY;
char *tkX_libraryEnv = "TKX_LIBRARY";


/*-----------------------------------------------------------------------------
 * Tkx_Init --
 *
 *   Do TkX initialization.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
Tkx_Init (interp)
    Tcl_Interp  *interp;
{
    return TclX_SetRuntimeLocation (interp, "tkx_library", tkX_libraryEnv,
                                    tkX_library);
}
