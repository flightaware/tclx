/*
 * tclXinit.c --
 *
 * Extended Tcl initialzation and initialization utilitied.
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
 * $Id: tclXlib.c,v 2.9 1993/07/30 15:05:15 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

static char *tclLibraryEnv = "TCL_LIBRARY";

/*
 * Prototypes of internal functions.
 */
static int
ProcessInitFile _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *initFile,
                             char       *overrideEnv));


/*
 *-----------------------------------------------------------------------------
 *
 * TclX_ErrorExit --
 *
 * Display error information and abort when an error is returned in the
 * interp->result. It uses TCLXENV(noDump) to determine if the stack should be
 * dumped.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter, should contain the
 *     error message in `result'.
 *   o exitCode - The code to pass to exit.
 *-----------------------------------------------------------------------------
 */
void
TclX_ErrorExit (interp, exitCode)
    Tcl_Interp  *interp;
    int          exitCode;
{
    char *errorStack;

    fflush (stdout);
    fprintf (stderr, "Error: %s\n", interp->result);

    if (Tcl_GetVar2 (interp, "TCLXENV", "noDump", TCL_GLOBAL_ONLY) == NULL) {
        errorStack = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
        if (errorStack != NULL)
            fprintf (stderr, "%s\n", errorStack);
    }
    exit (exitCode);
}

/*
 *-----------------------------------------------------------------------------
 * TclX_InitLibDirPath --
 *
 *   Initialize the path to the library directory.  If the specified
 * environment variable exists, use it, otherwise set the environment variable
 * to fool various pieces of Tcl/Tk code.  The path is also returned so it may
 * be used when its time to run the init file, etc.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter.
 *   o libDirPtr (O) - Pointer to a dynamic string to return the library path
 *     that is to be used.  Will be initialized.
 *   o envVar (I) - Environement variable to set if it does not exist or return
 *     in libDirPath if it does.
 *   o dir (I) - The directory name.
 *   o version1, version2 - Two part version number forming a directory under
 *     dir.  Either maybe NULL.
 *-----------------------------------------------------------------------------
 */
void
TclX_InitLibDirPath (interp, libDirPtr, envVar, dir, version1, version2)
    Tcl_Interp  *interp;
    Tcl_DString *libDirPtr;
    char        *envVar;
    char        *dir;
    char        *version1;
    char        *version2;
{
    char  *envValue;

    Tcl_DStringInit (libDirPtr);

    envValue = Tcl_GetVar2 (interp, "env", envVar, TCL_GLOBAL_ONLY);
    if (envValue != NULL) {
        Tcl_DStringAppend (libDirPtr, envValue, -1);
        return;
    }

    Tcl_DStringAppend (libDirPtr, dir, -1);
    Tcl_DStringAppend (libDirPtr, "/", -1);
    if (version1 != NULL)
        Tcl_DStringAppend (libDirPtr, version1, -1);
    if (version2 != NULL)
        Tcl_DStringAppend (libDirPtr, version2, -1);

    Tcl_SetVar2 (interp, "env", envVar, libDirPtr->string, TCL_GLOBAL_ONLY);
}

/*
 *-----------------------------------------------------------------------------
 * ProcessInitFile --
 *
 *   Evaluate the specified init file.
 *
 * Parameters:
 *   o interp  (I) - A pointer to the interpreter.
 *   o initFile (I) - The path to the init file.
 *   o overrideEnv (I) - Directory override environment variable for error msg.
 * Returns:
 *   TCL_OK if all is ok, TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
static int
ProcessInitFile (interp, initFile, overrideEnv)
    Tcl_Interp *interp;
    char       *initFile;
    char       *overrideEnv;
{
    struct stat  statBuf;

    /*
     * Check for file before evaling it so we can return a helpful error.
     */
    if (stat (initFile, &statBuf) < 0) {
        Tcl_AppendResult (interp,
                          "Can't access initialization file \"",
                          initFile, "\".\n", 
                          "  Override directory containing this file with ",
                          "the environment variable: \"",
                          overrideEnv, "\"", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_EvalFile (interp, initFile) != TCL_OK)
        return TCL_ERROR;
        
    Tcl_ResetResult (interp);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * TclX_Init --
 *
 *   Initialize all Extended Tcl commands, set auto_path and source the
 * Tcl init file.
 *-----------------------------------------------------------------------------
 */
int
TclX_Init (interp)
    Tcl_Interp *interp;
{
    Tcl_DString  libDir;

    if (TclXCmd_Init (interp) == TCL_ERROR)
        return TCL_ERROR;

    if (TclXLib_Init (interp) == TCL_ERROR)
        return TCL_ERROR;

    /*
     * Get the path to the library directory and deal with the environment
     * variable.
     */
    TclX_InitLibDirPath (interp,
                         &libDir,
                         tclLibraryEnv,
                         TCL_MASTERDIR,
                         TCL_VERSION,
                         TCL_EXTD_VERSION_SUFFIX);

    /*
     * Set auto_path.
     */
    if (Tcl_SetVar (interp, "auto_path", libDir.string,
                    TCL_GLOBAL_ONLY  | TCL_APPEND_VALUE |
                    TCL_LIST_ELEMENT | TCL_LEAVE_ERR_MSG) == NULL)
        goto errorExit;

    /*
     * Evaluate the init file unless the quick flag is set.
     */
    if (Tcl_GetVar2 (interp, "TCLXENV", "quick", TCL_GLOBAL_ONLY) == NULL) {
        Tcl_DStringAppend (&libDir, "/", -1);
        Tcl_DStringAppend (&libDir, "TclInit.tcl", -1);

        if (ProcessInitFile (interp, libDir.string,
                             tclLibraryEnv) == TCL_ERROR)
            goto errorExit;
    }

    Tcl_DStringFree (&libDir);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&libDir);
    return TCL_ERROR;
}
