/*
 * tclXinit.c --
 *
 * Extended Tcl initialzation and initialization utilitied.
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
 * $Id: tclXinit.c,v 5.0 1995/07/25 05:59:07 markd Rel markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * The init file name, which is either library relative or absolute.
 */
char *tclX_initFile = "TclInit.tcl";

/*
 * Prototypes of internal functions.
 */
static int
ProcessInitFile _ANSI_ARGS_((Tcl_Interp *interp));

static int
InsureVarExists _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *varName,
                             char       *defaultValue));


/*
 *-----------------------------------------------------------------------------
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
    FILE *stdoutPtr, *stderrPtr;

    stdoutPtr = TCL_STDOUT;
    stderrPtr = TCL_STDERR;

    fflush (stdoutPtr);

    if (Tcl_GetVar2 (interp, "TCLXENV", "noDump", TCL_GLOBAL_ONLY) == NULL) {
        errorStack = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
        if ((errorStack != NULL) && (errorStack [0] != '\0'))
            fprintf (stderrPtr, "Error: %s\n", errorStack);
        else
            fprintf (stderrPtr, "Error: %s\n", interp->result);
    } else {
        fprintf (stderrPtr, "Error: %s\n", interp->result);
    }
    fflush (stderrPtr);

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
 * InsureVarExists --
 *
 *   Insure that the specified global variable exists.
 *
 * Parameters:
 *   o interp  (I) - A pointer to the interpreter.
 *   o varName (I) - Name of the variable.
 *   o defaultValue (I) - Value to set the variable to if it doesn't already
 *     exist.
 * Returns:
 *   TCL_OK if all is ok, TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
static int
InsureVarExists (interp, varName, defaultValue)
    Tcl_Interp *interp;
    char       *varName;
    char       *defaultValue;
{
    if (Tcl_GetVar (interp, varName, TCL_GLOBAL_ONLY) == NULL) {
        if (Tcl_SetVar (interp, varName, defaultValue, 
                        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * ProcessInitFile --
 *
 *   Evaluate the TclX initialization file (normally TclInit.tcl in the
 * library directory.  But all of this can be overridden by changing global
 * definitions before this procedure is called.
 *
 * Parameters:
 *   o interp  (I) - A pointer to the interpreter.
 * Returns:
 *   TCL_OK if all is ok, TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
static int
ProcessInitFile (interp)
    Tcl_Interp *interp;
{
    Tcl_DString  initFile;
    char        *value;
    int          absFilePath = (tclX_initFile [0] == '/');

    Tcl_DStringInit (&initFile);

    /*
     * Get the variable containing the path to the Tcl library if its needed.
     */
    if (!absFilePath) {
        value = Tcl_GetVar (interp, "tclx_library",  
                            TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
        if (value == NULL) {
            Tcl_AppendResult (interp, "can't find variable \"tclx_library\", ",
                              "should have been set by TclXLib_Init",
                              (char *) NULL);
            goto errorExit;
        }

        /*
         * If there is no path in tclx_library, just return without doing
         * anything.
         */
        if (value [0] == '\0')
            goto exitPoint;
    }

    /*
     * Build path to file and eval.  Check for file being readable before
     * evaling it so we can return a helpful error.
     */
    if (!absFilePath) {
        Tcl_DStringAppend (&initFile, value, -1);
        Tcl_DStringAppend (&initFile, "/", -1);
    }
    Tcl_DStringAppend (&initFile, tclX_initFile, -1);

    if (access (initFile.string, R_OK) < 0) {
        Tcl_AppendResult (interp,
                          "Can't access initialization file \"",
                          initFile.string, "\" (", Tcl_PosixError (interp),
                          ")", (char *) NULL);
 
        if ((tclX_libraryEnv != NULL) && (!absFilePath)) {
            Tcl_AppendResult (interp,
                              "\n  Override directory containing this ",
                              "file with the environment variable: \"",
                              tclX_libraryEnv, "\"", (char *) NULL);
        }
        goto errorExit;
    }

    if (TclX_Eval (interp,
                   TCLX_EVAL_GLOBAL | TCLX_EVAL_FILE | TCLX_EVAL_ERR_HANDLER,
                   initFile.string) == TCL_ERROR)
        goto errorExit;

  exitPoint:        
    Tcl_DStringFree (&initFile);
    Tcl_ResetResult (interp);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&initFile);
    Tcl_AddErrorInfo (interp,
                     "\n    (while processing TclX initialization file)");
    return TCL_ERROR;
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
    /*
     * Make sure a certain set of variable exists.  If not, default them.
     * Tcl code often assumes that these exists.
     */
    if (InsureVarExists (interp, "errorInfo", "") == TCL_ERROR)
        return TCL_ERROR;
    if (InsureVarExists (interp, "errorCode", "") == TCL_ERROR)
        return TCL_ERROR;
    if (InsureVarExists (interp, "tcl_interactive", "0") == TCL_ERROR)
        return TCL_ERROR;


    if (TclXCmd_Init (interp) == TCL_ERROR)
        goto errorExit;

    if (TclXLib_Init (interp) == TCL_ERROR)
        goto errorExit;

    /*
     * Evaluate the init file unless the quick flag is set.
     */
    if (Tcl_GetVar2 (interp, "TCLXENV", "quick", TCL_GLOBAL_ONLY) == NULL) {
        if (ProcessInitFile (interp) == TCL_ERROR)
            goto errorExit;
    }

    return TCL_OK;

  errorExit:
    Tcl_AddErrorInfo (interp,
                     "\n    (while initializing TclX)");
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * TclX_EvalRCFile --
 *
 * Evaluate the file stored in tcl_RcFileName it is readable.  Exit if an
 * error occurs.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter.
 *-----------------------------------------------------------------------------
 */
void
TclX_EvalRCFile (interp)
    Tcl_Interp  *interp;
{
    Tcl_DString  buffer;
    char        *fullName;

    if (tcl_RcFileName != NULL) {
        fullName = Tcl_TildeSubst (interp, tcl_RcFileName, &buffer);
        if (fullName == NULL)
            TclX_ErrorExit (interp, 1);
        
        if (access(fullName, R_OK) == 0) {
            if (TclX_Eval (interp,
                           TCLX_EVAL_GLOBAL | TCLX_EVAL_FILE |
                           TCLX_EVAL_ERR_HANDLER,
                           fullName) == TCL_ERROR) {
                Tcl_DStringFree(&buffer);
                TclX_ErrorExit (interp, 1);
            }
        }
	Tcl_DStringFree(&buffer);
    }
}
