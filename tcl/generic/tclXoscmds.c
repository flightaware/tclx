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
 * $Id: tclXoscmds.c,v 7.6 1996/08/20 03:49:55 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


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
