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
 * $Id: tclXcmdInit.c,v 4.2 1995/01/01 19:49:22 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Used to override the library and library environment variable used to
 * find the TclX startup file and runtime library.  The values of these
 * fields must be changed before TclX_Init is called.
 */
char *tclX_library    = TCL_MASTERDIR;
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
    char        *value;
    Tcl_DString  libDir;

    Tcl_DStringInit (&libDir);

    TclX_LibraryInit (interp);

    /*
     * Determine the path to the master (library) directory.
     */
    value = NULL;
    if (tclX_libraryEnv != NULL) {
        value = Tcl_GetVar2 (interp, "env", tclX_libraryEnv, TCL_GLOBAL_ONLY);
        if (value != NULL)
            Tcl_DStringAppend (&libDir, value, -1);
    }
    if ((value == NULL) && (tclX_library != NULL))
        Tcl_DStringAppend (&libDir, tclX_library, -1);

    /*
     * Store the path in a Tcl variable. This may set it to the empty string.
     */
    if (Tcl_SetVar (interp, "tclx_library", libDir.string,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
        goto errorExit;

    /*
     * Append to auto_path if set actually have a directory.
     */
    if (Tcl_DStringLength (&libDir) > 0) {
        if (Tcl_SetVar (interp, "auto_path", libDir.string,
                        TCL_GLOBAL_ONLY  | TCL_APPEND_VALUE |
                        TCL_LIST_ELEMENT | TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;
    }

    Tcl_DStringFree (&libDir);
    return TCL_OK;
  errorExit:
    Tcl_DStringFree (&libDir);
    return TCL_ERROR;
}

