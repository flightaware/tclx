/*
 * tkXinit.c --
 *
 * Initialization code for the wishx and other Tk & Extended Tcl based
 * applications.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1995 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tkXinit.c,v 5.2 1995/10/11 03:01:29 markd Exp $
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

/*
 * Procedure that is used to determine if Tk_Init has already been called.
 */
static char *tkInitProc = "tkScreenChanged";


/*
 *-----------------------------------------------------------------------------
 *
 * TkX_Init --
 *
 *   Do Tk initialization for wishx.  This includes overriding the value
 * of the variable "tk_library" so it points to our library instead of 
 * the standard one and then calls Tk_Init.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TkX_Init (interp)
    Tcl_Interp  *interp;
{
    char        *interact, *libDir;
    Tcl_CmdInfo  cmdInfo;

    tclAppName     = "Wishx";
    tclAppLongname = "Extended Tk Shell - Wishx";
    tclAppVersion  = TK_VERSION;

    /*
     * If Tk_Init has been called, don't do anything else.
     */
    if (Tcl_GetCommandInfo (interp, tkInitProc, &cmdInfo)) {
        return TCL_OK;
    }

    /*
     * Set tk_library to point to the TkX library.  It maybe an empty
     * string if the path is not defined.
     */
    libDir = NULL;
    if (tkX_libraryEnv != NULL) {
        libDir = Tcl_GetVar2 (interp, "env", tkX_libraryEnv, TCL_GLOBAL_ONLY);
    }
    if (libDir == NULL) {
        if (tkX_library != NULL)
            libDir = tkX_library;
        else
            libDir = "";
    }
    if (Tcl_SetVar (interp, "tk_library", libDir,
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;


    return Tk_Init (interp);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tkx_Init --
 *
 *   Interface to TkX_Init that follows the Tcl dynamic loading naming
 * conventions. 
 *-----------------------------------------------------------------------------
 */
int
Tkx_Init (interp)
    Tcl_Interp *interp;
{
    return Tkx_Init (interp);
}
