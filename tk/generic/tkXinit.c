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
 * $Id: tkXinit.c,v 6.0 1996/05/10 16:19:08 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"
#include "tk.h"


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
    if (Tcl_PkgRequire (interp, "Tcl", TCL_VERSION, 1) == NULL) {
 	return TCL_ERROR;
    }
    if (Tcl_PkgRequire (interp, "Tclx", TCLX_VERSION, 1) == NULL) {
 	return TCL_ERROR;
    }
    if (Tcl_PkgRequire (interp, "Tk", TK_VERSION, 1) == NULL) {
 	return TCL_ERROR;
    }
    if (Tcl_PkgProvide (interp, "Tkx", TKX_VERSION) != TCL_OK) {
 	return TCL_ERROR;
    }

    if (TclX_RuntimeInit (interp,
                          "tkx_library",
                          "tkx_library_env",
                          "TKX_LIBRARY",
                          TKX_LIBRARY,
                          "tkx_init",
                          "tkx.tcl") == TCL_ERROR)
        goto errorExit;

    return TCL_OK;

  errorExit:
    Tcl_AddErrorInfo (interp,
                     "\n    (while initializing TkX)");
    return TCL_ERROR;
}
