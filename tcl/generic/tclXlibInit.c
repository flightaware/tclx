/*
 * tclXlibInit.c
 *
 * Function to add the Extented Tcl library commands into an interpreter. This
 * also sets the Tcl auto_path, tcl_library and tclx_library variable.
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
 * $Id: tclXlibInit.c,v 1.3 1995/06/26 06:21:08 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
UCBRuntimeSetup _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *tclXLibDir));


/*
 * Used to override the library and library environment variable used to
 * find the TclX startup file and runtime library.  The values of these
 * fields must be changed before TclX_Init is called.
 */
char *tclX_library    = TCLX_LIBRARY;
char *tclX_libraryEnv = "TCLX_LIBRARY";

/*
 * Tcl library procedures to delete.
 */
static char *tclDeleteProcs [] = {
    "auto_execok",
    "auto_reset",
    "auto_mkindex",
    NULL
};

/*
 *  Tcl library variables to delete.
 */
static char *tclDeleteVars [] = {
    "auto_execs",
    "auto_index",
    "auto_oldpath",
    NULL
};


/*
 *-----------------------------------------------------------------------------
 * UCBRuntimeSetup --
 *
 *   Determine if the UCB Tcl runtime has been initialize by a call to
 * Tcl_Init.  If it has, then we want to remove the Tcl library management
 * functions (auto_*) so that TclX's will be dynamically loaded.  The auto_load
 * procedure is not deleted, since we have/will replace it. If the Tcl run
 * time has not been loaded, set the tcl_library variable to point to the
 * TclX directory.  Note that $tclx_library must be inserted before
 * $tcl_library or the standard Tcl library management functions will be used
 * instead of the TclX ones by the same name.
 *
 * Parameters:
 *   o interp (I) - The interpreter.
 *   o tclXLibDir (I) - Path to the TclX library directory.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
UCBRuntimeSetup (interp, tclXLibDir)
    Tcl_Interp *interp;
    char       *tclXLibDir;
{
    Tcl_CmdInfo cmdInfo;
    int         idx;
    Tcl_DString deleteVarCmd;

    if (!Tcl_GetCommandInfo (interp, tclDeleteProcs [0], &cmdInfo)) {
        if (Tcl_SetVar (interp, "tcl_library", tclXLibDir,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;
        return TCL_OK;
    }
    
    /*
     * Tcl_Init was called, clean up.
     */
    for (idx = 0; tclDeleteProcs [idx] != NULL; idx++) {
        Tcl_DeleteCommand (interp, tclDeleteProcs [idx]);
    }
    Tcl_DStringInit (&deleteVarCmd);
    for (idx = 0; tclDeleteVars [idx] != NULL; idx++) {
        if (Tcl_GetVar (interp, tclDeleteVars [idx],
                        TCL_GLOBAL_ONLY) != NULL) {
            Tcl_DStringAppend (&deleteVarCmd, "unset ", -1);
            Tcl_DStringAppend (&deleteVarCmd, tclDeleteVars [idx], -1);
            Tcl_GlobalEval (interp, Tcl_DStringValue (&deleteVarCmd));
            Tcl_DStringFree (&deleteVarCmd);
        }
    }
    return TCL_OK;
}


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
    char  *libDir, *autoPath, **pathArgv;
    int    idx, pathArgc;
    

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
        libDir = Tcl_GetVar (interp, "tclx_library", TCL_GLOBAL_ONLY);
    }
    if (libDir == NULL) {
        if (tclX_library != NULL)
            libDir  = tclX_library;
        else
            libDir = "";
    }

    /*
     * Deal with UCB library enviroment based on if Tcl_Init has been called
     * or not.
     */
    if (UCBRuntimeSetup (interp, libDir) == TCL_ERROR)
        return TCL_ERROR;

    /*
     * Set the tclx_library variable.  If it was already set, then it
     * will be reset to the same value.
     */
    if (Tcl_SetVar (interp, "tclx_library", libDir,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    if (libDir [0] == '\0')
        return;  /* No library, don't modify path */

    /*
     * Insert  to auto_path to the tclx_library directory.
     */
    autoPath = Tcl_GetVar (interp, "auto_path", TCL_GLOBAL_ONLY);
    if (autoPath == NULL) {
        /*
         * Optimal and normal case when Tcl_Init has not been called.
         */
        if (Tcl_SetVar (interp, "auto_path", libDir,
                        TCL_GLOBAL_ONLY  | TCL_APPEND_VALUE |
                        TCL_LIST_ELEMENT | TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;
    } else {
        if (Tcl_SplitList (interp, autoPath, &pathArgc, &pathArgv) != TCL_OK)
            return TCL_ERROR;

        /*
         * Insert tclx_library at the top.
         */
        for (idx = pathArgc; idx > 0; idx--)
            pathArgv [idx] = pathArgv [idx - 1];
        pathArgv [0] = libDir;
        
        autoPath = Tcl_Merge (pathArgc + 1, pathArgv);
        ckfree ((char *) pathArgv);

        if (Tcl_SetVar (interp, "auto_path", autoPath,
                        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            ckfree (autoPath);
            return TCL_ERROR;
        }
        ckfree (autoPath);
    }
    return TCL_OK;
}

