/*
 * tkXinit.c --
 *
 * Initialization code for the wishx and other Tk & Extended Tcl based
 * applications.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1993 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tkXinit.c,v 1.6 1993/09/16 05:37:54 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"
#include "tk.h"


/*
 *-----------------------------------------------------------------------------
 *
 * TkX_Init --
 *
 *   Do Tk initialization for wishx.  This includes overriding the value
 * of the variable "tk_library" so it points to our library instead of 
 * the standard one.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter.
 * Returns:
 *   TCL_OK.
 *-----------------------------------------------------------------------------
 */
int
TkX_Init (interp, interactive, errorSignalProc)
    Tcl_Interp          *interp;
    int                  interactive;
    TkX_ShellSignalProc *errorSignalProc;
{
    char        *value;
    Tcl_DString  libDir;

    tclAppName     = "Wishx";
    tclAppLongname = "Extended Tk Shell - Wishx";
    tclAppVersion  = TK_VERSION;

    TclX_InitLibDirPath (interp,
                         &libDir,
                         "TK_LIBRARY",
                         TK_MASTERDIR,
                         TK_VERSION,
                         TCL_EXTD_VERSION_SUFFIX);
    Tcl_SetVar (interp, "tk_library", libDir.string, TCL_GLOBAL_ONLY);

    /*
     * If we are going to be interactive, Setup SIGINT handling.
     */
    value = Tcl_GetVar (interp, "tcl_interactive", TCL_GLOBAL_ONLY);
    if ((value != NULL) && (value [0] != '0'))
        Tcl_SetupSigInt ();

    /*
     * Run the initialization that comes with standard Tk.
     */
    return Tk_Init (interp);
}
