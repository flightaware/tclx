/*
 * tclXinit.c --
 *
 * Extended Tcl initialzation and initialization utilitied.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1996 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXinit.c,v 5.6 1996/02/09 18:42:58 markd Exp $
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

static int
InitSetup _ANSI_ARGS_((Tcl_Interp *interp));


/*
 *-----------------------------------------------------------------------------
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
    char  numBuf [32];
    Tcl_Channel stdoutChan, stderrChan;
    Tcl_DString savedResult;

    Tcl_DStringInit (&savedResult);
    Tcl_DStringAppend (&savedResult, interp->result, -1);

    stdoutChan = TclX_Stdout (interp);
    stderrChan = TclX_Stderr (interp);

    if (stdoutChan != NULL)
        Tcl_Flush (stdoutChan);

    if (stderrChan != NULL) {
        TclX_WriteStr (stderrChan, "Error: ");
        if (Tcl_GetVar2 (interp, "TCLXENV", "noDump",
                         TCL_GLOBAL_ONLY) == NULL) {
            errorStack = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
            if ((errorStack != NULL) && (errorStack [0] != '\0'))
                TclX_WriteStr (stderrChan, errorStack);
            else
                TclX_WriteStr (stderrChan, Tcl_DStringValue (&savedResult));
        } else {
                TclX_WriteStr (stderrChan, Tcl_DStringValue (&savedResult));
        }
        TclX_WriteNL (stderrChan);
        Tcl_Flush (stderrChan);
    }

    Tcl_Exit (exitCode);
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
 * InitSetup --
 *
 *   So setup common to both normal and safe initialization.
 *-----------------------------------------------------------------------------
 */
static int
InitSetup (interp)
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
    if (InitSetup (interp) == TCL_ERROR)
        goto errorExit;

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
 * TclX_SafeInit --
 *
 *   Initialize safe Extended Tcl commands.
 *-----------------------------------------------------------------------------
 */
int
TclX_SafeInit (interp)
    Tcl_Interp *interp;
{
    if (InitSetup (interp) == TCL_ERROR)
        goto errorExit;

    if (TclXCmd_SafeInit (interp) == TCL_ERROR)
        goto errorExit;

    return TCL_OK;

  errorExit:
    Tcl_AddErrorInfo (interp,
                     "\n    (while initializing safe TclX)");
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tclx_Init, Tclx_SafeInit --
 *
 *   Interface to TclX_Init and TclX_SafeInit that follows the Tcl dynamic
 * loading naming conventions.
 *-----------------------------------------------------------------------------
 */
int
Tclx_Init (interp)
    Tcl_Interp *interp;
{
    return TclX_Init (interp);
}

int
Tclx_SafeInit (interp)
    Tcl_Interp *interp;
{
    return TclX_SafeInit (interp);
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
    char        *path;

    path = Tcl_GetVar (interp, "tcl_rcFileName", TCL_GLOBAL_ONLY);
    if (path == NULL)
        return;

    Tcl_DStringInit (&buffer);

    path = Tcl_TranslateFileName (interp, path, &buffer);
    if (path == NULL)
        TclX_ErrorExit (interp, 1);
        
    if (access (path, R_OK) == 0) {
        if (TclX_Eval (interp,
                       TCLX_EVAL_GLOBAL | TCLX_EVAL_FILE |
                       TCLX_EVAL_ERR_HANDLER,
                       path) == TCL_ERROR) {
            TclX_ErrorExit (interp, 1);
        }
    }
    Tcl_DStringFree(&buffer);
}
