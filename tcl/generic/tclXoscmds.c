/*
 * tclXoscmds.c --
 *
 * Tcl commands to access unix system calls that are portable to other
 * platforms.
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
 * $Id: tclXoscmds.c,v 7.4 1996/08/04 07:29:59 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
MkdirPath _ANSI_ARGS_((Tcl_Interp *interp,
                       char       *path));


/*-----------------------------------------------------------------------------
 * Tcl_AlarmCmd --
 *     Implements the TCL Alarm command:
 *         alarm seconds
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_AlarmCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    double seconds;

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " seconds", 
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetDouble (interp, argv[1], &seconds) != TCL_OK)
        return TCL_ERROR;

    if (TclXOSsetitimer (interp, &seconds, "alarm") != TCL_OK)
        return TCL_ERROR;

    sprintf (interp->result, "%g", seconds);

    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_LinkCmd --
 *     Implements the TCL link command:
 *         link ?-sym? srcpath destpath
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *-----------------------------------------------------------------------------
 */
int
Tcl_LinkCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    char *srcPath, *destPath;
    Tcl_DString  srcPathBuf, destPathBuf;

    Tcl_DStringInit (&srcPathBuf);
    Tcl_DStringInit (&destPathBuf);

    if ((argc < 3) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " ?-sym? srcpath destpath", (char *) NULL);
        return TCL_ERROR;
    }
    if (argc == 4) {
        if (!STREQU (argv [1], "-sym")) {
            Tcl_AppendResult (interp, "invalid option, expected: \"-sym\", ",
                              "got: ", argv [1], (char *) NULL);
            return TCL_ERROR;
        }
    }

    srcPath = Tcl_TranslateFileName (interp, argv [argc - 2], &srcPathBuf);
    if (srcPath == NULL)
        goto errorExit;

    destPath = Tcl_TranslateFileName (interp, argv [argc - 1], &destPathBuf);
    if (destPath == NULL)
        goto errorExit;

    if (argc == 4) {
        if (TclX_OSsymlink (interp, srcPath, destPath, argv [0]) != TCL_OK)
            goto errorExit;
    } else {
        if (TclX_OSlink (interp, srcPath, destPath, argv [0]) != TCL_OK) {
            goto errorExit;
        }
    }

    Tcl_DStringFree (&srcPathBuf);
    Tcl_DStringFree (&destPathBuf);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&srcPathBuf);
    Tcl_DStringFree (&destPathBuf);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * MkdirPath --
 *    Make a directory path, creating all elements that don't exist. Its not
 * an error it the full target directory exists.
 * Parameters:
 *   o interp (I) - Errors are returned in result.
 *   o path (I) - Path to make.  Should already be translated.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
MkdirPath (interp, path)
    Tcl_Interp *interp;
    char       *path;
{
    int pathArgc, idx;
    char **pathArgv = NULL;
    struct stat  statBuf;
    Tcl_DString pathBuf, prevPathBuf;
    
    Tcl_DStringInit (&pathBuf);
    Tcl_DStringInit (&prevPathBuf);

    Tcl_SplitPath (path, &pathArgc, &pathArgv);

    for (idx = 0; idx < pathArgc; idx++) {
        Tcl_DStringSetLength (&prevPathBuf, 0);
        Tcl_DStringAppend (&prevPathBuf, pathBuf.string, -1);
        Tcl_DStringSetLength (&pathBuf, 0);
        path = TclX_JoinPath (prevPathBuf.string, pathArgv [idx], &pathBuf);

        /*
         * FIX: Handle if it exists but is a file, should just try in this
         * case.
         */
        if (stat (path, &statBuf) < 0) {
            if (TclXOSmkdir (interp, path) == TCL_ERROR)
                goto errorExit;
        }
    }

    Tcl_DStringFree (&pathBuf);
    Tcl_DStringFree (&prevPathBuf);
    if (pathArgv != NULL)
        ckfree ((char *) pathArgv);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&pathBuf);
    Tcl_DStringFree (&prevPathBuf);
    if (pathArgv != NULL)
        ckfree ((char *) pathArgv);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_MkdirCmd --
 *     Implements the TCL Mkdir command:
 *         mkdir ?-path? dirList
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_MkdirCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int idx, dirArgc;
    char **dirArgv, *dirName;
    Tcl_DString pathBuf;

    if ((argc < 2) || (argc > 3) || 
        ((argc == 3) && !STREQU (argv [1], "-path"))) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " ?-path? dirlist", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_SplitList (interp, argv [argc - 1], &dirArgc, &dirArgv) != TCL_OK)
        return TCL_ERROR;

    Tcl_DStringInit (&pathBuf);

    for (idx = 0; idx < dirArgc; idx++) {
        dirName = Tcl_TranslateFileName (interp, dirArgv [idx], &pathBuf);
        if (dirName == NULL)
           goto errorExit;

        /*
         * If -path is specified, make; directories that don't exist.
         */
        if (argc == 3) {
            if (MkdirPath (interp, dirName) != TCL_OK)
                goto errorExit;
        } else {
            if (TclXOSmkdir (interp, dirName) != TCL_OK)
                goto errorExit;
        }
        Tcl_DStringSetLength (&pathBuf, 0);
    }

    Tcl_DStringFree (&pathBuf);
    ckfree ((char *) dirArgv);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&pathBuf);
    ckfree ((char *) dirArgv);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_NiceCmd --
 *     Implements the TCL nice command:
 *         nice ?priorityincr?
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_NiceCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int priorityIncr, priority;
    char numBuf [32];

    if (argc > 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ?priorityincr?",
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Return the current priority if an increment is not supplied.
     */
    if (argc == 1) {
        if (TclXOSgetpriority (interp, &priority, argv [0]) != TCL_OK)
            return TCL_ERROR;
        sprintf (numBuf, "%d", priority);
        Tcl_SetResult (interp, numBuf, TCL_VOLATILE);
        return TCL_OK;
    }

    /*
     * Increment the priority.
     */
    if (Tcl_GetInt (interp, argv[1], &priorityIncr) != TCL_OK)
        return TCL_ERROR;

    if (TclXOSincrpriority (interp, priorityIncr, &priority,
                             argv [0]) != TCL_OK)
        return TCL_ERROR;
    sprintf (numBuf, "%d", priority);
    Tcl_SetResult (interp, numBuf, TCL_VOLATILE);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_SleepCmd --
 *     Implements the TCL sleep command:
 *         sleep seconds
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_SleepCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    unsigned time;

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " seconds", 
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetUnsigned (interp, argv[1], &time) != TCL_OK)
        return TCL_ERROR;

    TclXOSsleep (time);
    return TCL_OK;

}

/*-----------------------------------------------------------------------------
 * Tcl_SyncCmd --
 *     Implements the TCL sync command:
 *         sync
 *
 * Results:
 *      Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_SyncCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    Tcl_Channel channel;

    if ((argc < 1) || (argc > 2)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ?filehandle?",
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (argc == 1) {
	TclXOSsync ();
	return TCL_OK;
    }

    channel = TclX_GetOpenChannel (interp, argv [1], TCL_WRITABLE);
    if (channel == NULL)
        return TCL_ERROR;

    if (Tcl_Flush (channel) < 0) {
        Tcl_AppendResult (interp, Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }

    return TclXOSfsync (interp, channel);
}

/*-----------------------------------------------------------------------------
 * Tcl_SystemCmd --
 *     Implements the TCL system command:
 *     system command
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_SystemCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int exitCode;

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " command",
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (TclXOSsystem (interp, argv [1], &exitCode) != TCL_OK)
        return TCL_ERROR;

    sprintf (interp->result, "%d", exitCode);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_UmaskCmd --
 *     Implements the TCL umask command:
 *     umask ?octalmask?
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_UmaskCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int mask;

    if ((argc < 1) || (argc > 2)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ?octalmask?",
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (argc == 1) {
        mask = umask (0);
        umask ((unsigned short) mask);
        sprintf (interp->result, "%o", mask);
    } else {
        if (!Tcl_StrToInt (argv [1], 8, &mask)) {
            Tcl_AppendResult (interp, "Expected octal number got: ", argv [1],
                              (char *) NULL);
            return TCL_ERROR;
        }

        umask ((unsigned short) mask);
    }

    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_UnlinkCmd --
 *     Implements the TCL unlink command:
 *         unlink ?-nocomplain? fileList
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_UnlinkCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int           idx, fileArgc;
    char        **fileArgv, *fileName;
    int           noComplain;
    Tcl_DString   pathBuf;

    Tcl_DStringInit (&pathBuf);
    
    if ((argc < 2) || (argc > 3))
        goto badArgs;

    if (argc == 3) {
        if (!STREQU (argv [1], "-nocomplain"))
            goto badArgs;
        noComplain = TRUE;
    } else {
        noComplain = FALSE;
    }

    if (Tcl_SplitList (interp, argv [argc - 1], &fileArgc,
                       &fileArgv) != TCL_OK)
        return TCL_ERROR;

    for (idx = 0; idx < fileArgc; idx++) {
        fileName = Tcl_TranslateFileName (interp, fileArgv [idx], &pathBuf);
        if (fileName == NULL) {
            if (!noComplain)
                goto errorExit;
            Tcl_DStringFree (&pathBuf);
            continue;
        }
        if ((unlink (fileName) != 0) && !noComplain) {
            Tcl_AppendResult (interp, fileArgv [idx], ": ",
                              Tcl_PosixError (interp), (char *) NULL);
            goto errorExit;
        }
        Tcl_DStringFree (&pathBuf);
    }

    ckfree ((char *) fileArgv);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&pathBuf);
    ckfree ((char *) fileArgv);
    return TCL_ERROR;

  badArgs:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                      " ?-nocomplain? filelist", (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_RmdirCmd --
 *     Implements the TCL Rmdir command:
 *         rmdir ?-nocomplain?  dirList
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_RmdirCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int          idx, dirArgc;
    char       **dirArgv, *dirName;
    int          noComplain;
    Tcl_DString  pathBuf;

    Tcl_DStringInit (&pathBuf);
    
    if ((argc < 2) || (argc > 3))
        goto badArgs;

    if (argc == 3) {
        if (!STREQU (argv [1], "-nocomplain"))
            goto badArgs;
        noComplain = TRUE;
    } else {
        noComplain = FALSE;
    }

    if (Tcl_SplitList (interp, argv [argc - 1], &dirArgc, &dirArgv) != TCL_OK)
        return TCL_ERROR;

    for (idx = 0; idx < dirArgc; idx++) {
        dirName = Tcl_TranslateFileName (interp, dirArgv [idx], &pathBuf);
        if (dirName == NULL) {
            if (!noComplain)
                goto errorExit;
            Tcl_DStringFree (&pathBuf);
            continue;
        }
        if ((rmdir (dirName) != 0) && !noComplain) {
           Tcl_AppendResult (interp, dirArgv [idx], ": ",
                             Tcl_PosixError (interp), (char *) NULL);
           goto errorExit;
        }
        Tcl_DStringFree (&pathBuf);
    }

    ckfree ((char *) dirArgv);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&pathBuf);
    ckfree ((char *) dirArgv);
    return TCL_ERROR;;

  badArgs:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                      " ?-nocomplain? dirlist", (char *) NULL);
    return TCL_ERROR;
}
