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
 * $Id: tclXinit.c,v 1.2 1993/10/11 04:24:49 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * The following is used to force the version of tclCmdIL.c that was compiled
 * for TclX to be brought in rather than the standard version.
 */
int *tclxDummyInfoCmdPtr = (int *) Tcl_InfoCmd;

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
 * dumped.  Attempts to use the "exit" command to exit, so cleanup can be done.
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
    char  numBuf [32];

    fflush (stdout);
    fprintf (stderr, "Error: %s\n", interp->result);

    if (Tcl_GetVar2 (interp, "TCLXENV", "noDump", TCL_GLOBAL_ONLY) == NULL) {
        errorStack = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
        if (errorStack != NULL)
            fprintf (stderr, "%s\n", errorStack);
    }

    /*
     * Use "exit" command to exit.
     */
    sprintf (numBuf, "%d", exitCode);
    Tcl_VarEval (interp, "exit ", numBuf, (char *) NULL);
    
    /*
     * If that failed, really exit.
     */
    exit (exitCode);
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
    char        *value;
    Tcl_DString  libDir;

    if (TclXCmd_Init (interp) == TCL_ERROR)
        return TCL_ERROR;

    if (TclXLib_Init (interp) == TCL_ERROR)
        return TCL_ERROR;

    Tcl_DStringInit (&libDir);

    /*
     * Get the path to the master (library) directory.
     */
    value = Tcl_GetVar2 (interp, "env", tclLibraryEnv, TCL_GLOBAL_ONLY);
    if (value != NULL)
        Tcl_DStringAppend (&libDir, value, -1);
    else
        Tcl_DStringAppend (&libDir, TCL_MASTERDIR, -1);

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
