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
 * $Id: tclXinit.c,v 5.9 1996/02/23 06:52:43 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
ProcessInitFile _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *tclInitVarName,
                             char       *defaultInitFile,
                             char       *libDir,
                             char       *libEnvVar));

static int
InsureVarExists _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *varName,
                             char       *defaultValue));

static int
InitSetup _ANSI_ARGS_((Tcl_Interp *interp));


/*-----------------------------------------------------------------------------
 * ProcessInitFile --
 *
 *   Evaluate an initialization file.
 *
 * Parameters:
 *   o interp  (I) - A pointer to the interpreter.
 *   o tclInitVarName (I) - Name of Tcl variable that can specify the init
 *     file.
 *   o defaultInitFile (I) - Default path to the initializion file.  If the
 *     first character is not '/', its is relative to libDir.
 *   o libDir (I) - Library directory or NULL (in which case, the init file
 *     must be absolute.
 *   o libEnvVar (I) - Environment variable used to override the location of
 *     the libDir.  NULL if not overridable. This is needed for error messages.
 * Returns:
 *   TCL_OK if all is ok, TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
static int
ProcessInitFile (interp, tclInitVarName, defaultInitFile, libDir, libEnvVar)
    Tcl_Interp *interp;
    char       *tclInitVarName;
    char       *defaultInitFile;
    char       *libDir;
    char       *libEnvVar;
{
    Tcl_DString  initFilePath;
    char *initFile, *value;
    struct stat statInfo;

    Tcl_DStringInit (&initFilePath);

    /*
     * Get the name of the init file, either use the Tcl variable or the
     * default.
     */
    initFile = Tcl_GetVar (interp, tclInitVarName, TCL_GLOBAL_ONLY);
    if (initFile == NULL)
        initFile = defaultInitFile;

    /*
     * Build path to file.
     */
    if (initFile [0] != '/') {
        if (libDir == NULL) {
            Tcl_AppendResult (interp, "No runtime directory defined, ",
                              "can't find initialization file \"",
                              initFile, "\".", (char *) NULL);
            if (libEnvVar != NULL) {
                Tcl_AppendResult (interp,
                                  "\nOverride directory containing this ",
                                  "file with the environment variable: \"",
                                  libEnvVar, "\"", (char *) NULL);
            }
            return NULL;
        }
        Tcl_DStringAppend (&initFilePath, libDir, -1);
        Tcl_DStringAppend (&initFilePath, "/", -1);
    }
    Tcl_DStringAppend (&initFilePath, initFile, -1);

    /*
     * Check for file existing before evaling it so we can return a helpful
     * error.
     */
    if (stat (initFilePath.string, &statInfo) < 0) {
        Tcl_AppendResult (interp,
                          "Can't access initialization file \"",
                          initFilePath.string, "\" (", Tcl_PosixError (interp),
                          ")", (char *) NULL);
 
        if ((libEnvVar != NULL) && (initFile [0] != '/')) {
            Tcl_AppendResult (interp,
                              "\n  Override directory containing this ",
                              "file with the environment variable: \"",
                              libEnvVar, "\"", (char *) NULL);
        }
        goto errorExit;
    }

    if (TclX_Eval (interp,
                   TCLX_EVAL_GLOBAL | TCLX_EVAL_FILE | TCLX_EVAL_ERR_HANDLER,
                   initFilePath.string) == TCL_ERROR)
        goto errorExit;

  exitPoint:        
    Tcl_DStringFree (&initFilePath);
    Tcl_ResetResult (interp);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&initFilePath);
    Tcl_AddErrorInfo (interp,
                     "\n    (while processing initialization file)");
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_RuntimeInit --
 *
 * Set the value of a *_library Tcl variable and evaluate an initialzation
 * file.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter.
 *   o tclLibVarName (I) - Name of the variable to set.
 *   o tclEnvVarName (I) - Tcl variable that may contain the name of the
 *     override variable or an empty string to disable override. (maybe NULL).
 *   o defaultEnvVar (I) - Default override environment variable (maybe NULL).
 *   o defaultDir (I) - Default value to set the variable to (maybe NULL).
 *   o tclInitVarName (I) - Name of Tcl variable that can specify the init
 *     file.
 *   o defaultInitFile (I) - Default path to the initializion file.  If the
 *     first character is not '/', its is relative to libDir.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 * Alogrithm:
 *   o If the Tcl variable tclLibVarName is already set, do nothing.
 *   o If tclEnvVarName is supplied and that Tcl variable exists, use it's
 *     value as the name of the environment variable to check.  If the value is
 *     the empty string, don't check the environment.
 *   o If the tclEnvVarName Tcl variable is not set, then the default
 *     environment variable is used.
 *   o If the environment is to be checked and the environment is defined,
 *     use its value for the value to set tclLibVarName to.
 *   o If a default is supplied for the variable, set tclLibVarName value to
 *     it.
 *-----------------------------------------------------------------------------
 */
int
TclX_RuntimeInit (interp, tclLibVarName, tclEnvVarName, defaultEnvVar,
                  defaultDir, tclInitVarName, defaultInitFile)
    Tcl_Interp *interp;
    char       *tclLibVarName;
    char       *tclEnvVarName;
    char       *defaultEnvVar;
    char       *defaultDir;
    char       *tclInitVarName;
    char       *defaultInitFile;
{
    char *libEnvVar = NULL, *libDir = NULL;

    /*
     * Set the library variable if not already set.
     */
    if (Tcl_GetVar (interp, tclLibVarName, TCL_GLOBAL_ONLY) == NULL) {
        /*
         * Get the name of the environment variable that overrides.
         */
        if (tclEnvVarName != NULL) {
            libEnvVar = Tcl_GetVar (interp, tclLibVarName, TCL_GLOBAL_ONLY);
            if (libEnvVar == NULL) {
                libEnvVar = defaultEnvVar;
            } else {
                if (libEnvVar [0] == '\0')
                    libEnvVar = NULL;
            }
        } else {
            libEnvVar = defaultEnvVar;
        }

        /*
         * Get the value of the environment variable if its defined.
         */
        if (libEnvVar != NULL) {
            libDir = Tcl_GetVar2 (interp, "env", libEnvVar, TCL_GLOBAL_ONLY);
        }
        if (libDir == NULL) {
            libDir = defaultDir;
        }

        /*
         * Set the value if we actually have a path.
         */
        if (libDir != NULL) {
            if (Tcl_SetVar (interp, tclLibVarName, libDir,
                            TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
                return TCL_ERROR;
        }
    }

    if (Tcl_GetVar2 (interp, "TCLXENV", "quick", TCL_GLOBAL_ONLY) == NULL) {
         if (ProcessInitFile (interp, tclInitVarName, defaultInitFile, libDir,
                              libEnvVar) == TCL_ERROR)
             return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
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
    return TCL_OK;
}


/*-----------------------------------------------------------------------------
 * Tclx_Init --
 *
 *   Initialize all Extended Tcl commands, set auto_path and source the
 * Tcl init file.
 *-----------------------------------------------------------------------------
 */
int
Tclx_Init (interp)
    Tcl_Interp *interp;
{
    if (InitSetup (interp) == TCL_ERROR)
        goto errorExit;

    if (Tclxcmd_Init (interp) == TCL_ERROR)
        goto errorExit;

    if (Tclxlib_Init (interp) == TCL_ERROR)
        goto errorExit;

    if (TclX_RuntimeInit (interp,
                          "tclx_library",
                          "tclx_library_env",
                          "TCLX_LIBRARY",
                          TCLX_LIBRARY,
                          "tclx_init",
                          "tclx.tcl") == TCL_ERROR)
        goto errorExit;

    return TCL_OK;

  errorExit:
    Tcl_AddErrorInfo (interp,
                     "\n    (while initializing TclX)");
    return TCL_ERROR;
}


/*-----------------------------------------------------------------------------
 * Tclx_SafeInit --
 *
 *   Initialize safe Extended Tcl commands.
 *-----------------------------------------------------------------------------
 */
int
Tclx_SafeInit (interp)
    Tcl_Interp *interp;
{
    if (InitSetup (interp) == TCL_ERROR)
        goto errorExit;

    if (Tclxcmd_SafeInit (interp) == TCL_ERROR)
        goto errorExit;

    return TCL_OK;

  errorExit:
    Tcl_AddErrorInfo (interp,
                     "\n    (while initializing safe TclX)");
    return TCL_ERROR;
}
