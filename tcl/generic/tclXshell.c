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
 * $Id: tclXstartup.c,v 2.8 1993/06/06 15:05:35 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"
#include "patchlevel.h"

extern char * etenv ();

extern char *optarg;
extern int   optind, opterr;

typedef struct tclParms_t {
    int       execFile;      /* Run the specified file. (no searching)       */
    int       execCommand;   /* Execute the specified command.               */
    unsigned  options;       /* Quick startup option.                        */
    char     *execStr;       /* Command file or command to execute.          */
    char    **tclArgv;       /* Arguments to pass to tcl script.             */
    int       tclArgc;       /* Count of arguments to pass to tcl script.    */
    char     *programName;   /* Name of program (less path).                 */
    } tclParms_t;

/*
 * Prototypes of internal functions.
 */
static void
ParseCmdArgs _ANSI_ARGS_((int          argc,
                          char       **argv,
                          tclParms_t  *tclParmsPtr));

static void
GetInitFile _ANSI_ARGS_((tclInitFile_t  *initFileDesc,
                         char          **dirPathPtr,
                         char          **filePathPtr));

static char **
InitFilesSetup _ANSI_ARGS_((Tcl_Interp     *interp,
                            tclInitFile_t **initFiles));

static int
EvalInitFile _ANSI_ARGS_((Tcl_Interp    *interp,
                          tclInitFile_t *initFileDesc,
                          char          *initFilePath));

static int
ProcessInitFiles _ANSI_ARGS_((Tcl_Interp     *interp,
                              tclInitFile_t **initFiles,
                              int             noEval));

/*
 * Default initialization file definition.
 */
tclInitFile_t tclDefaultInitFile = {
    "TCLINIT",
    TCL_MASTERDIR, TCL_VERSION, TCL_EXTD_VERSION_SUFFIX,
    "TclInit.tcl"
};

static tclInitFile_t *defaultInitFiles [] =
    {&tclDefaultInitFile, NULL};



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
 * Parse the arguments passed to the Tcl shell
 *
 * Parameters:
 *   o argc, argv - Arguments passed to main.
 *   o tclParmsPtr - Results of the parsed Tcl shell command line.
 *-----------------------------------------------------------------------------
 */
static void
ParseCmdArgs (argc, argv, tclParmsPtr)
    int          argc;
    char       **argv;
    tclParms_t  *tclParmsPtr;
{
    char   *scanPtr, *programName;
    int     programNameLen;
    int     option;

    tclParmsPtr->execFile = FALSE;
    tclParmsPtr->execCommand = FALSE;
    tclParmsPtr->options = 0;
    tclParmsPtr->execStr = NULL;

    /*
     * Determine file name (less directories) that the Tcl interpreter is
     * being run under.
     */
    scanPtr = programName = argv[0];
    while (*scanPtr != '\0') {
        if (*scanPtr == '/')
            programName = scanPtr + 1;
        scanPtr++;
    }
    tclParmsPtr->programName = programName;
    programNameLen = strlen (programName);
    
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

    tclParmsPtr->tclArgv = &argv [optind];
    tclParmsPtr->tclArgc = argc - optind;
    return;

usageError:
    fprintf (stderr, "usage: %s %s\n", argv [0],
             "?-qun? ?-f? ?script?|?-c command? ?args?");
    exit (1);
}


/*
 *-----------------------------------------------------------------------------
 * GetInitFile --
 *
 *   Given an init file description, return both the path to the init
 * directory and the initFile, if a file is specified.  Does not actually
 * check for the existance of the init file.  Checks the environment for the
 * init file under the specified variable, if not found, assembles the name
 * from the description.
 *
 * Parameters
 *   o initFileDesc (I) - Description of the initfile.
 *   o dirPathPtr (O) - The path of the init directory is returned here.
 *   o filePathPtr (O) - If this points to a file, the file path is returned
 *     here, otherwise, NULL is returned.
 *-----------------------------------------------------------------------------
 */
static void
GetInitFile (initFileDesc, dirPathPtr, filePathPtr)
    tclInitFile_t  *initFileDesc;
    char          **dirPathPtr;
    char          **filePathPtr;
{
    char *dirPath, *envPath, *strPtr;
    int   pathLen;

    /*
     * Check if its in the environment.  If it is, still treat the value as a
     * file or as a directory depending on if a initFile was in the
     * description or just a directory.
     */
    if ((initFileDesc->envVar != NULL) &&
        ((envPath = getenv (initFileDesc->envVar)) != NULL)) {

        *dirPathPtr = ckstrdup (envPath);

        if (initFileDesc->initFile != NULL) {
            strPtr = strrchr (*dirPathPtr, '/');
            if (strPtr != NULL)
                *strPtr = '\0';
            *filePathPtr = ckstrdup (envPath);
        } else {
            *filePathPtr = NULL;
        }
        return;
    }

    /*
     * Assemble name from pieces, usually containing the version.
     */
    pathLen = 1;
    if (initFileDesc->dir1 != NULL)
        pathLen += strlen (initFileDesc->dir1);
    if (initFileDesc->dir2 != NULL)
        pathLen += strlen (initFileDesc->dir2);
    if (initFileDesc->dir3 != NULL)
        pathLen += strlen (initFileDesc->dir3);

    dirPath = ckalloc (pathLen);
    dirPath [0] = '\0';
    if (initFileDesc->dir1 != NULL)
        strcat (dirPath, initFileDesc->dir1);
    if (initFileDesc->dir2 != NULL)
        strcat (dirPath, initFileDesc->dir2);
    if (initFileDesc->dir3 != NULL)
        strcat (dirPath, initFileDesc->dir3);
    *dirPathPtr = dirPath;
    if (initFileDesc->initFile != NULL) {
        *filePathPtr =
            ckalloc (pathLen + strlen (initFileDesc->initFile) + 1);
        strcpy (*filePathPtr, dirPath);
        strcat (*filePathPtr, "/");
        strcat (*filePathPtr, initFileDesc->initFile);
    } else {
        *filePathPtr = NULL;
    }
}

/*
 *-----------------------------------------------------------------------------
 * InitFilesSetup --
 *
 *   Convert the initFiles description to there paths and set the values of
 * the TCLPATH and TCLINITS variables.
 *
 * Parameters
 *   o interp  (I) - A pointer to the interpreter.
 *   o initFiles (I) - The file name of the init file to use, it
 *     normally contains a version number.
 * Returns:
 *   A pointer to a NULL terminated, dyamic array of init file paths or 
 * NULL if an error occured.
 *
 * Notes:
 *   A small amount of memory is lost on an error.  We figure we are going to
 * exit anyway.
 *-----------------------------------------------------------------------------
 */
static char **
InitFilesSetup (interp, initFiles)
    Tcl_Interp     *interp;
    tclInitFile_t **initFiles;
{
    int    numInitFiles, numInitDirs, idx;
    char  *filePath, *strPtr;
    char **initFilePaths, **initDirPaths;
    char  *initDirsBuffer [8];

    /*
     * Determine the number of initFiles and convert them to an array of file
     * paths and an array of directory names.  There maybe more directories
     * that files since the initFile is optional.
     */
    for (numInitDirs = 0; initFiles [numInitDirs] != NULL; numInitDirs++)
        continue;
    if (numInitDirs <= sizeof (initDirsBuffer) / sizeof (char *))
        initDirPaths = initDirsBuffer;
    else
        initDirPaths =
            (char **) ckalloc ((numInitDirs + 1) * sizeof (char **));
    initFilePaths = (char **) ckalloc ((numInitDirs + 1) * sizeof (char **));

    /*
     * Get the init file names and fill in the arrays.  Nuke file name
     * in directory paths containing file
     */
    numInitFiles = 0;
    for (idx = 0; idx < numInitDirs; idx++) {
        GetInitFile (initFiles [idx], &(initDirPaths [idx]), &filePath);
        if (filePath != NULL)
            initFilePaths [numInitFiles++] = filePath;
    }
    initFilePaths [numInitFiles] = NULL;

    /*
     * Set the auto_path variable.  It can be overriden by an environment
     * variable.???
     */
    if ((strPtr = getenv ("TCLPATH")) != NULL) {
        if (Tcl_SetVar (interp, "auto_path", strPtr,
                        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            return NULL;
        }
    } else {
        strPtr = Tcl_Merge (numInitDirs, initDirPaths);
        if (Tcl_SetVar (interp, "auto_path", strPtr,
                        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            return NULL;
        }
        ckfree (strPtr);
    }

    /*
     * Set the TCLINITS variable array.
     */
    strPtr = Tcl_Merge (numInitFiles, initFilePaths);
    if (Tcl_SetVar (interp, "TCLINITS", strPtr,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
        return NULL;
    }
    ckfree (strPtr);

    for (idx = 0; idx < numInitDirs; idx++)
        ckfree (initDirPaths [idx]);
    if (initDirPaths != initDirsBuffer)
        ckfree (initDirPaths);

    return initFilePaths;
}

/*
 *-----------------------------------------------------------------------------
 * EvalInitFile --
 *
 *   Process the an initialization file.
 *
 * Parameters
 *   o interp  (I) - A pointer to the interpreter.
 *   o initFileDesc (I) - Description of the init file.
 *   o initFilePath (I) Path to init file.
 * Returns:
 *   TCL_OK if all is ok, TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
static int
EvalInitFile (interp, initFileDesc, initFilePath)
    Tcl_Interp    *interp;
    tclInitFile_t *initFileDesc;
    char          *initFilePath;
{
    struct stat statBuf;

    if (stat (initFilePath, &statBuf) < 0) {
        Tcl_AppendResult (interp,
                          "Can't access initialization file \"",
                          initFilePath, "\".\n", 
                          "  Override location with environment variable: \"",
                          initFileDesc->envVar, "\"",
                          (char *) NULL);
        return TCL_ERROR;

    }
    if (Tcl_EvalFile (interp, initFilePath) != TCL_OK)
        return TCL_ERROR;
    Tcl_ResetResult (interp);

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * ProcessInitFiles --
 *
 *   Process all init files, including setting up the TCLPATH.
 *
 * Parameters
 *   o interp  (I) - A pointer to the interpreter.
 *   o initFiles (I) - Descriptions of the init files.
 *   o noEval (I) - If specified, set up the variables, but don't actually
 *     eval the files.
 * Returns:
 *   TCL_OK if all is ok, TCL_ERROR if an error occured.
 *
 * Notes:
 *   A small amount of memory is lost on an error.  We figure we are going to
 * exit anyway.
 *-----------------------------------------------------------------------------
 */
static int
ProcessInitFiles (interp, initFiles, noEval)
    Tcl_Interp     *interp;
    tclInitFile_t **initFiles;
    int             noEval;
{
    int    idx, result;
    char **initFilePaths;

    initFilePaths = InitFilesSetup (interp, initFiles);
    if (initFilePaths == NULL)
        return TCL_ERROR;

    if (!noEval) {
        for (idx = 0; initFilePaths [idx] != NULL; idx++) {
            if (EvalInitFile (interp, initFiles [idx],
                              initFilePaths [idx]) != TCL_OK)
                return TCL_ERROR;
        }
    }

    for (idx = 0; initFilePaths [idx] != NULL; idx++)
        ckfree (initFilePaths [idx]);
    ckfree (initFilePaths);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ShellEnvInit --
 *
 *   Initialize the Tcl shell environment.  Including processing initialization
 * files.  This sets the TCLPATH variable to contain all directories that
 * contain intialization files.
 *
 * If this is an interactive Tcl session, SIGINT is set to generate a Tcl
 * error.
 *
 * Parameters
 *   o interp - A pointer to the interpreter.
 *   o options - Flags to control the behavior of this routine, the following
 *     options are supported:
 *       o TCLSH_QUICK_STARTUP - Don't source the init file or Tcl init
 *         file.
 *       o TCLSH_ABORT_STARTUP_ERR - If set, abort the process if an error
 *         occurs.
 *       o TCLSH_NO_STACK_DUMP - If an error occurs, don't dump out the
 *         procedure call stack, just print an error message.
 *       o TCLSH_INTERACTIVE - Set interactiveSession to 1.
 *   o programName (I) - The name of the program being executed, usually
 *     taken from the main argv [0].  Used to set the Tcl variable.  If NULL
 *     then the variable will not be set.
 *   o argc, argv (I) - Arguments to pass to the program in a Tcl list variable
 *     `argv'.  Argv [0] should contain the first argument not the program
 *     name.  If argv is NULL, then the variable will not be set.
 *   o initFiles (I) - Table used to construct the startup files to
 *     evaulate.  Terminated by a NULL. The table consists of the entries with
 *     the following fields:
 *     o envVar - Environment variable used to override the path of the file.
 *       Maybe NULL if env should not be checked.
 *     o dir1, dir2, dir3 - 3 part directory name, strings are concatenated
 *       together.  Usually contains version numbers.  Maybe NULL.
 *     o initFile - Actual initialization file in dir.
 * Notes:
 *   The variables tclAppName, tclAppLongName, tclAppVersion 
 * must be set before calling thus routine if special values are desired.
 *
 * Returns:
 *   TCL_OK if all is ok, TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
int
Tcl_ShellEnvInit (interp, options, programName, argc, argv, initFiles)
    Tcl_Interp      *interp;
    unsigned         options;
    CONST char      *programName; 
    int              argc;
    CONST char     **argv;
    tclInitFile_t  **initFiles;
{
    char *args;


    if (initFiles == NULL)
        initFiles = defaultInitFiles;

    if (programName != NULL) {
        if (Tcl_SetVar (interp, "programName", (char *) programName,
                        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;
    }

    if (argv != NULL) {
        args = Tcl_Merge (argc, (char **) argv);
        if (Tcl_SetVar (interp, "argv", args,
                        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            ckfree (args);
            goto errorExit;
        }
        ckfree (args);
    }

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

    /*
     * Process all init files.
     */
    if (ProcessInitFiles (interp, initFiles, 
                          options & TCLSH_QUICK_STARTUP) != TCL_OK)
        goto errorExit;

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
 *    Initializes the Tcl extended environment.  This function processes the
 * standard command line arguments, sets the TCLPATH variable and process the
 * initialization files.
 *
 * Parameters
 *   o interp - A pointer to the interpreter.
 *   o options (I) - Options that control startup behavior.  None are
 *     currently defined.
 *   o argc, argv - Arguments passed to main for the command line.
 *   o initFiles (I) - Table used to construct the startup files to
 *     evaulate.  Terminated by a NULL. The table consists of the entries with
 *     the following fields:
 *     o envVar - Environment variable used to override the path of the file.
 *       Maybe NULL if env should not be checked.
 *     o dir1, dir2, dir3 - 3 part directory name, strings are concatenated
 *       together.  Usually contains version numbers.  Maybe NULL.
 *     o initFile - Actual initialization file in dir.
 * Notes:
 *   The variables tclAppName, tclAppLongName, tclAppVersion must be set
 * before calling thus routine if special values are desired.
 *-----------------------------------------------------------------------------
 */
void
Tcl_Startup (interp, options, argc, argv, initFiles)
    Tcl_Interp      *interp;
    unsigned         options;
    int              argc;
    CONST char     **argv;
    tclInitFile_t  **initFiles;
{
    char       *cmdBuf;
    tclParms_t  tclParms;
    int         result;

    /*
     * Process the arguments.
     */
    ParseCmdArgs (argc, (char **) argv, &tclParms);

    if (tclParms.execStr == NULL)
        tclParms.options |= TCLSH_INTERACTIVE;

    if (Tcl_ShellEnvInit (interp,
                          tclParms.options,
                          tclParms.programName,
                          tclParms.tclArgc,
                          tclParms.tclArgv,
                          initFiles) != TCL_OK)
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
        result = Tcl_Eval (interp, tclParms.execStr, 0, NULL);
        if (result != TCL_OK)
            goto errorAbort;
    } else {
        Tcl_CommandLoop (interp, stdin, stdout, 0);
    }

    Tcl_ResetResult (interp);
    return;

errorAbort:
    Tcl_ErrorAbort (interp, tclParms.options & TCLSH_NO_STACK_DUMP, 255);
}

