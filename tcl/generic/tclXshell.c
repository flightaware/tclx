/*
 * tclXstartup.c --
 *
 * Startup code for the Tcl shell and other interactive applications.  Also
 * create special commands used just by Tcl shell features.
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
 * $Id: tclXstartup.c,v 2.14 1993/07/20 03:43:10 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"
#include "patchlevel.h"

extern char *optarg;
extern int   optind, opterr;

typedef struct tclParms_t {
    int       execFile;      /* Run the specified file. (no searching)       */
    int       execCommand;   /* Execute the specified command.               */
    unsigned  options;       /* Startup options.                             */
    char     *execStr;       /* Command file or command to execute.          */
    } tclParms_t;

/*
 * Prototypes of internal functions.
 */
static void
ParseCmdArgs _ANSI_ARGS_((Tcl_Interp  *interp,
                          int          argc,
                          char       **argv,
                          tclParms_t  *tclParmsPtr));

static void
MergeMasterDirPath _ANSI_ARGS_((char        *dir,
                                char        *version1,
                                char        *version2,
                                Tcl_DString *dirPathPtr));

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ErrorAbort --
 *
 * Display error information and abort when an error is returned in the
 * interp->result.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter, should contain the
 *     error message in `result'.
 *   o noStackDump - If TRUE, then the procedure call stack will not be
 *     displayed.
 *   o exitCode - The code to pass to exit.
 *-----------------------------------------------------------------------------
 */
void
Tcl_ErrorAbort (interp, noStackDump, exitCode)
    Tcl_Interp  *interp;
    int          noStackDump;
    int          exitCode;
{
    char *errorStack;

    fflush (stdout);
    fprintf (stderr, "Error: %s\n", interp->result);

    if (noStackDump == 0) {
        errorStack = Tcl_GetVar (interp, "errorInfo", 1);
        if (errorStack != NULL)
            fprintf (stderr, "%s\n", errorStack);
    }
    exit (exitCode);
}

/*
 *-----------------------------------------------------------------------------
 *
 * ParseCmdArgs --
 *
 * Parse the arguments passed to the Tcl shell.  Set the Tcl variables
 * argv0, argv & argc from the remaining parameters.
 *
 * Parameters:
 *   o interp  (I) - A pointer to the interpreter.
 *   o argc, argv - Arguments passed to main.
 *   o tclParmsPtr - Results of the parsed Tcl shell command line.
 *-----------------------------------------------------------------------------
 */
static void
ParseCmdArgs (interp, argc, argv, tclParmsPtr)
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
    tclParms_t  *tclParmsPtr;
{
    char   *scanPtr, *tclArgv, numBuf [32];
    int     option;

    tclParmsPtr->execFile = FALSE;
    tclParmsPtr->execCommand = FALSE;
    tclParmsPtr->options = 0;
    tclParmsPtr->execStr = NULL;

    /*
     * Scan arguments looking for flags to process here rather than to pass
     * on to the scripts.  The '-c' or '-f' must also be the last option to
     * allow for script arguments starting with `-'.
     */
    while ((option = getopt (argc, argv, "qc:f:un")) != -1) {
        switch (option) {
            case 'q':
                if (tclParmsPtr->options & TCLSH_QUICK_STARTUP)
                    goto usageError;
                tclParmsPtr->options |= TCLSH_QUICK_STARTUP;
                break;
            case 'n':
                if (tclParmsPtr->options & TCLSH_NO_STACK_DUMP)
                    goto usageError;
                tclParmsPtr->options |= TCLSH_NO_STACK_DUMP;
                break;
            case 'c':
                tclParmsPtr->execCommand = TRUE;
                tclParmsPtr->execStr = optarg;
                goto exitParse;
            case 'f':
                tclParmsPtr->execFile = TRUE;
                tclParmsPtr->execStr = optarg;
                goto exitParse;
            case 'u':
            default:
                goto usageError;
        }
    }
    exitParse:
  
    /*
     * If neither `-c' nor `-f' were specified and at least one parameter
     * is supplied, then if is the file to execute.  The rest of the arguments
     * are passed to the script.  Check for '--' as the last option, this also
     * is a terminator for the file to execute.
     */
    if ((!tclParmsPtr->execCommand) && (!tclParmsPtr->execFile) &&
        (optind != argc) && !STREQU (argv [optind-1], "--")) {
        tclParmsPtr->execFile = TRUE;
        tclParmsPtr->execStr = argv [optind];
        optind++;
    }

    /*
     * Set the Tcl argv0, argv & argc variables.
     */
    Tcl_SetVar (interp, "argv0",
                (tclParmsPtr->execFile) ? tclParmsPtr->execStr : argv[0],
                TCL_GLOBAL_ONLY);

    tclArgv = Tcl_Merge (argc - optind,  &argv [optind]);
    Tcl_SetVar (interp, "argv", tclArgv, TCL_GLOBAL_ONLY);
    ckfree (tclArgv);
    sprintf (numBuf, "%d", argc - optind);
    Tcl_SetVar (interp, "argc", numBuf, TCL_GLOBAL_ONLY);

    return;

usageError:
    fprintf (stderr, "usage: %s %s\n", argv [0],
             "?-qun? ?-f? ?script?|?-c command? ?args?");
    exit (1);
}


/*
 *-----------------------------------------------------------------------------
 * MergeMasterDirPath --
 *
 *   Merge mastter directory components and into a path name.  
 *
 * Parameters:
 *   o dir (I) - The directory name.
 *   o version1, version2 - Two part version number forming a directory under
 *     dir.  Either maybe NULL.
 *   o dirPathPtr (O) - Dynamic string containing the directory path.  Will
 *     be initialized.
 *-----------------------------------------------------------------------------
 */
static void
MergeMasterDirPath (dir, version1, version2, dirPathPtr)
    char        *dir;
    char        *version1;
    char        *version2;
    Tcl_DString *dirPathPtr;
{
    Tcl_DStringInit (dirPathPtr);

    Tcl_DStringAppend (dirPathPtr, dir, -1);
    Tcl_DStringAppend (dirPathPtr, "/", -1);
    if (version1 != NULL)
        Tcl_DStringAppend (dirPathPtr, version1, -1);
    if (version2 != NULL)
        Tcl_DStringAppend (dirPathPtr, version2, -1);
}

/*
 *-----------------------------------------------------------------------------
 * Tcl_SetLibrayDirEnvVar --
 *
 *   Set an library (master) directory environment variable to override
 * default that Tcl or TK were compiled with.  Only overrides if the variable
 * does not exist.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter.
 *   o envVar (I) - Environement variable to set if it does not exist.
 *   o dir (I) - The directory name.
 *   o version1, version2 - Two part version number forming a directory under
 *     dir.  Either maybe NULL.
 *-----------------------------------------------------------------------------
 */
void
Tcl_SetLibraryDirEnvVar (interp, envVar, dir, version1, version2)
    Tcl_Interp  *interp;
    char        *envVar;
    char        *dir;
    char        *version1;
    char        *version2;
{
    Tcl_DString   masterDir;

    if (Tcl_GetVar (interp, envVar, TCL_GLOBAL_ONLY) != NULL)
        return;

    MergeMasterDirPath (dir, version1, version2, &masterDir);
    Tcl_SetVar (interp, envVar, masterDir.string, TCL_GLOBAL_ONLY);

    Tcl_DStringFree (&masterDir);

}

/*
 *-----------------------------------------------------------------------------
 * Tcl_ProcessInitFile --
 *
 *   Find and evaluate a Tcl init file.  This assumes that the init file
 * lives in a master directory and appends the directory name to the 
 * "auto_path" variable.
 *
 * Parameters:
 *   o interp  (I) - A pointer to the interpreter.
 *   o dirEnvVar (I) - Environment variable used to override the directory
 *     path.
 *   o dir (I) - The directory name.
 *   o version1, version2 - Two part version number forming a directory under
 *     dir.  Either maybe NULL.
 *   o initFile (I) - The name of the init file, which is found either in the
 *     directory pointed to by envVar or by the directory formed from dir1,
 *     dir2 & dir3.
 * Returns:
 *   TCL_OK if all is ok, TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
int
Tcl_ProcessInitFile (interp, dirEnvVar, dir, version1, version2, initFile)
    Tcl_Interp *interp;
    char       *dirEnvVar;
    char       *dir;
    char       *version1;
    char       *version2;
    char       *initFile;
{
    char         *dirEnvValue;
    Tcl_DString   filePath;
    struct stat   statBuf;

    /*
     * Determine the master (library) directory name.
     */
    dirEnvValue = getenv (dirEnvVar);
    if (dirEnvValue != NULL) {
        Tcl_DStringInit (&filePath);
        Tcl_DStringAppend (&filePath, dirEnvValue, -1);
    } else {
        MergeMasterDirPath (dir, version1, version2, &filePath);
    }

    /*
     * Include this in the auto_path variable.
     */

    if (Tcl_SetVar (interp, "auto_path", filePath.string,
                    TCL_GLOBAL_ONLY  | TCL_APPEND_VALUE |
                    TCL_LIST_ELEMENT | TCL_LEAVE_ERR_MSG) == NULL)
        goto errorExit;


    /*
     * Eval the init file in that directory.
     */
    Tcl_DStringAppend (&filePath, "/", -1);
    Tcl_DStringAppend (&filePath, initFile, -1);
    
    if (stat (filePath.string, &statBuf) < 0) {
        Tcl_AppendResult (interp,
                          "Can't access initialization file \"",
                          filePath.string, "\".\n", 
                          "  Override directory containing this file with ",
                          "the environment variable: \"",
                          dirEnvVar, "\"",
                          (char *) NULL);
        return TCL_ERROR;

    }
    if (Tcl_EvalFile (interp, filePath.string) != TCL_OK)
        goto errorExit;
    Tcl_ResetResult (interp);

    Tcl_DStringFree (&filePath);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&filePath);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ShellEnvInit --
 *
 *   Initialize the Tcl shell environment.  Including evaluating the standard
 * TclX init file and setting various variable and values for infox to access.
 * If this is an interactive Tcl session, SIGINT is set to generate a Tcl
 * error.  This routine is provided for the wishx shell or similar
 * environments where the Tcl_Startup command line parsing is not desired.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter.
 *   o options - Flags to control the behavior of this routine, the following
 *     options are supported:
 *       o TCLSH_INTERACTIVE - Set interactiveSession to 1.
 *       o TCLSH_QUICK_STARTUP - Don't source the Tcl initialization file.
 *       o TCLSH_ABORT_STARTUP_ERR - If set, abort the process if an error
 *         occurs.
 *       o TCLSH_NO_STACK_DUMP - If an error occurs, don't dump out the
 *         procedure call stack, just print an error message.
 * Notes:
 *   The variables tclAppName, tclAppLongName, tclAppVersion 
 * must be set before calling thus routine if special values are desired.
 *
 * Returns:
 *   TCL_OK if all is ok, TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
int
Tcl_ShellEnvInit (interp, options)
    Tcl_Interp  *interp;
    unsigned     options;
{
    if (Tcl_SetVar (interp, "interactiveSession", 
                    (options & TCLSH_INTERACTIVE) ? "1" : "0",
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
        goto errorExit;

    tclxVersion = ckalloc (strlen (TCL_VERSION) + 
                           strlen (TCL_EXTD_VERSION_SUFFIX) + 1);
    strcpy (tclxVersion, TCL_VERSION);
    strcat (tclxVersion, TCL_EXTD_VERSION_SUFFIX);

#ifdef PATCHLEVEL
    tclxPatchlevel = PATCHLEVEL;
#else
    tclxPatchlevel = 0;
#endif

    /*
     * Set application specific values to return from the infox if they
     * have not been set.
     */
    if (tclAppName == NULL)
        tclAppName = "TclX";
    if (tclAppLongname == NULL)
        tclAppLongname = "Extended Tcl";
    if (tclAppVersion == NULL)
        tclAppVersion = tclxVersion;

    Tcl_SetLibraryDirEnvVar (interp,
                             "TCL_LIBRARY",
                             TCL_MASTERDIR,
                             TCL_VERSION,
                             TCL_EXTD_VERSION_SUFFIX);

    if ((options & TCLSH_QUICK_STARTUP) == 0) {
        if (Tcl_ProcessInitFile (interp,
                                 "TCL_LIBRARY",
                                 TCL_MASTERDIR,
                                 TCL_VERSION,
                                 TCL_EXTD_VERSION_SUFFIX,
                                 "TclInit.tcl") == TCL_ERROR)
            goto errorExit;
    }
    if (options & TCLSH_INTERACTIVE)
        Tcl_SetupSigInt ();

    return TCL_OK;

errorExit:
    if (options & TCLSH_ABORT_STARTUP_ERR)
        Tcl_ErrorAbort (interp, options & TCLSH_NO_STACK_DUMP, 255);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_Startup --
 *
 *   Initializes the Tcl extended environment.  This function processes the
 * standard command line arguments and locates the Tcl initialization file.
 * It then sources the initialization file.  Either an interactive command
 * loop is created or a Tcl script file is executed depending on the command
 * line.  This functions calls Tcl_ShellEnvInit, so it should not be called
 * separately.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter.
 *   o options (I) - Options that control startup behavior.  None are
 *     currently defined.
 *   o argc, argv - Arguments passed to main for the command line.
 * Notes:
 *   The variables tclAppName, tclAppLongName, tclAppVersion must be set
 * before calling thus routine if special values are desired.
 *-----------------------------------------------------------------------------
 */
void
Tcl_Startup (interp, options, argc, argv)
    Tcl_Interp      *interp;
    unsigned         options;
    int              argc;
    CONST char     **argv;
{
    char       *cmdBuf;
    tclParms_t  tclParms;
    int         result;

    /*
     * Process the arguments.
     */
    ParseCmdArgs (interp, argc, (char **) argv, &tclParms);

    if (tclParms.execStr == NULL)
        tclParms.options |= TCLSH_INTERACTIVE;

    if (Tcl_ShellEnvInit (interp, tclParms.options) != TCL_OK)
        goto errorAbort;

    /*
     * If the invoked tcl interactively, give the user an interactive session,
     * otherwise, source the command file or execute the specified command.
     */
    if (tclParms.execFile) {
        result = Tcl_EvalFile (interp, tclParms.execStr);
        if (result != TCL_OK)
            goto errorAbort;
    } else if (tclParms.execCommand) {
        result = Tcl_Eval (interp, tclParms.execStr);
        if (result != TCL_OK)
            goto errorAbort;
    } else {
        Tcl_CommandLoop (interp);
    }

    Tcl_ResetResult (interp);
    return;

errorAbort:
    Tcl_ErrorAbort (interp, tclParms.options & TCLSH_NO_STACK_DUMP, 255);
}

