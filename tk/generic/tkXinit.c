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
 * $Id: tkXinit.c,v 1.9 1993/10/23 23:33:57 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"
#include "tk.h"

/*
 * The following is used to force the version of tkWindow.c that was compiled
 * for TclX to be brought in rather than the standard version.
 */
int *tclxDummyMainWindowPtr = (int *) Tk_MainWindow;


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
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TkX_Init (interp)
    Tcl_Interp          *interp;
{
    char        *value;
    Tcl_DString  libDir;

    tclAppName     = "Wishx";
    tclAppLongname = "Extended Tk Shell - Wishx";
    tclAppVersion  = TK_VERSION;

    Tcl_DStringInit (&libDir);

    /*
     * Get the path to the master (library) directory.
     */
    value = Tcl_GetVar2 (interp, "env", "TK_LIBRARY", TCL_GLOBAL_ONLY);
    if (value != NULL)
        Tcl_DStringAppend (&libDir, value, -1);
    else
        Tcl_DStringAppend (&libDir, TK_MASTERDIR, -1);

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
