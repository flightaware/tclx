/*
 * tkXstartup.c --
 *
 * Startup code for the wishx and other Tk & Extended Tcl based applications.
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
 * $Id: tkXstartup.c,v 1.3 1993/07/23 06:42:57 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"
#include "tk.h"
#include "tkX.h"

/*
 * Code taken from wish.tcl that we still need for wishx.  Its so small that
 * we "rom" it here, rather than have another init file.
 */

static char *wishRom = "if [info exists geometry] {wm geometry . $geometry}";


/*
 *-----------------------------------------------------------------------------
 *
 * TkX_Startup --
 *
 *   Do basic startup for wishx.  This is called before Tk_CreateMainWindow.
 * It does the basic TclX shell environment intialization and then doctors
 * the TK_LIBRARY environment variable to point to our library.  This does
 * not source the Tk init file, that must be done after the main window is
 * created.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter.
 *   o interactive (I) - TRUE if this is interactive, FALSE otherwise.
 *   o errorSignalProc (I) - Function to call when an error signal occurs.
 *     This can handle flushing of interactive input buffers if necessary.
 *     Ignored if interactive is FALSE.
 *-----------------------------------------------------------------------------
 */
void
TkX_Startup (interp, interactive, errorSignalProc)
    Tcl_Interp          *interp;
    int                  interactive;
    TkX_ShellSignalProc *errorSignalProc;
{

    tclAppName     = "Wishx";
    tclAppLongname = "Extended Tk Shell - Wishx";
    tclAppVersion  = TK_VERSION;
    Tcl_ShellEnvInit (interp, 
                      TCLSH_ABORT_STARTUP_ERR |
                          (interactive ? TCLSH_INTERACTIVE : 0));

    if (interactive)
        tkXshellSignalProc = errorSignalProc;

    Tcl_SetLibraryDirEnvVar (interp,
                             "TK_LIBRARY",
                             TK_MASTERDIR,
                             TK_VERSION,
                             TCL_EXTD_VERSION_SUFFIX);
}


/*
 *-----------------------------------------------------------------------------
 *
 * TkX_WishInit --
 *
 *   Do the rest of the wish initalization.  This sources the tk.tcl file,
 * sets up auto_path and does what ever else would be done in wish.tcl.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter.
 *-----------------------------------------------------------------------------
 */
void
TkX_WishInit (interp)
    Tcl_Interp *interp;
{
    if (Tcl_ProcessInitFile (interp,
                             "TK_LIBRARY",
                             TK_MASTERDIR,
                             TK_VERSION,
                             TCL_EXTD_VERSION_SUFFIX,
                             "tk.tcl")  == TCL_ERROR)
        goto errorExit;

    if (Tcl_GlobalEval (interp, wishRom) == TCL_ERROR)
        goto errorExit;
    return;

  errorExit:
        Tcl_ErrorAbort (interp, 0, 255);
}
