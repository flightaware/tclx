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
 * $Id: tclXoscmds.c,v 5.2 1996/03/19 07:52:54 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


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

    TclX_OSsleep (time);
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
    if ((argc < 1) || (argc > 2)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ?filehandle?",
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (argc == 1) {
	TclX_OSsync ();
	return TCL_OK;
    }

    return TclX_OSfsync (interp, argv [1]);
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

    if (TclX_OSsystem (interp, argv [1], &exitCode) != TCL_OK)
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
    int           idx, dirArgc, result;
    char        **dirArgv, *dirName, *scanPtr;
    struct stat   statBuf;
    Tcl_DString   pathBuf;

    Tcl_DStringInit (&pathBuf);

    if ((argc < 2) || (argc > 3) || 
        ((argc == 3) && !STREQU (argv [1], "-path"))) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " ?-path? dirlist", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_SplitList (interp, argv [argc - 1], &dirArgc, &dirArgv) != TCL_OK)
        return TCL_ERROR;

    /*
     * Make all the directories, optionally making directories along the path.
     */

    for (idx = 0; idx < dirArgc; idx++) {
        dirName = Tcl_TranslateFileName (interp, dirArgv [idx], &pathBuf);
        if (dirName == NULL)
           goto errorExit;

        /*
         * If -path is specified, make; directories that don't exist.  Also,
         * its not an error it the target directory exists.
         */
        if (argc == 3) {
            scanPtr = dirName;
            result = TCL_OK;  /* Start out ok, for dirs that are skipped */

            while (*scanPtr != '\0') {
                scanPtr = strchr (scanPtr+1, '/');
                if ((scanPtr == NULL) || (*(scanPtr+1) == '\0'))
                    break;
                *scanPtr = '\0';
                if (stat (dirName, &statBuf) < 0)
                    result = TclX_OSmkdir (interp, dirName);
                *scanPtr = '/';
                if (result != TCL_OK)
                   goto errorExit;
            }
            if (stat (dirName, &statBuf) < 0)
                result = TclX_OSmkdir (interp, dirName);
        } else {
            if (TclX_OSmkdir (interp, dirName) != TCL_OK)
                goto errorExit;
        }
        Tcl_DStringFree (&pathBuf);
    }

    ckfree ((char *) dirArgv);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&pathBuf);
    ckfree ((char *) dirArgv);
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
