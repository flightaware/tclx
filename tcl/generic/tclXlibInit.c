/*
 * tclXlibInit.c
 *
 * Function to add the Extented Tcl library commands into an interpreter. This
 * also sets the Tcl auto_path and and tclx_library variable.
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
 * $Id: tclXlibInit.c,v 1.1 1995/01/16 07:36:59 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Used to override the library and library environment variable used to
 * find the TclX startup file and runtime library.  The values of these
 * fields must be changed before TclX_Init is called.
 */
char *tclX_library    = TCLX_LIBRARY;
char *tclX_libraryEnv = "TCL_LIBRARY";


/*
 *-----------------------------------------------------------------------------
 * TclXLib_Init --
 *
 *   Initialize the Extended Tcl library facility commands.  Add the TclX
 * library directory to auto_path and set the tclx_library variable.
 *-----------------------------------------------------------------------------
 */
int
TclXLib_Init (interp)
    Tcl_Interp *interp;
{
    char  *libDir;

    TclX_LibraryInit (interp);

    /*
     * Determine the path to the master (library) directory.  Store it in a
     * Tcl variable. This may set it to the empty string.
     */
    libDir = NULL;
    if (tclX_libraryEnv != NULL) {
        libDir = Tcl_GetVar2 (interp, "env", tclX_libraryEnv, TCL_GLOBAL_ONLY);
    }
    if (libDir == NULL) {
        if (tclX_library != NULL)
            libDir  = tclX_library;
        else
            libDir = "";
    }

    if (Tcl_SetVar (interp, "tclx_library", libDir,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    /*
     * Append to auto_path if set actually have a directory.
     */
    if (libDir [0] != '\0') {
        if (Tcl_SetVar (interp, "auto_path", libDir,
                        TCL_GLOBAL_ONLY  | TCL_APPEND_VALUE |
                        TCL_LIST_ELEMENT | TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;
    }

    return TCL_OK;
}

