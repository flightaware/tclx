/*
 * tclXunixcmds.c --
 *
 * Tcl commands to access unix system calls that are not portable to other
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
 * $Id: tclXunixCmds.c,v 5.7 1996/03/18 08:50:01 markd Exp $
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

    if (TclX_OSsetitimer (interp, &seconds, "alarm") != TCL_OK)
        return TCL_ERROR;

    sprintf (interp->result, "%g", seconds);

    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_ChrootCmd --
 *     Implements the TCL chroot command:
 *         chroot path
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ChrootCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " path", 
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (chroot (argv [1]) < 0) {
        Tcl_AppendResult (interp, "changing root to \"", argv [1],
                          "\" failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
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

    if (argc > 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ?priorityincr?",
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Return the current priority if an increment is not supplied.
     */
    if (argc == 1) {
        TclX_OSgetpriority (interp, &priority, argv [0]);
        sprintf (interp->result, "%d", priority);
        return TCL_OK;
    }

    /*
     * Increment the priority.
     */
    if (Tcl_GetInt (interp, argv[1], &priorityIncr) != TCL_OK)
        return TCL_ERROR;

    if (TclX_OSincrpriority (interp, priorityIncr, &priority,
                             argv [0]) != TCL_OK)
        return TCL_ERROR;
    sprintf (interp->result, "%d", priority);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_TimesCmd --
 *     Implements the TCL times command:
 *     times
 *
 * Results:
 *  Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_TimesCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    struct tms tm;

    if (argc != 1) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0], (char *) NULL);
        return TCL_ERROR;
    }

    times (&tm);

    sprintf (interp->result, "%ld %ld %ld %ld", 
             TclX_OSTicksToMS (tm.tms_utime),
             TclX_OSTicksToMS (tm.tms_stime),
             TclX_OSTicksToMS (tm.tms_cutime),
             TclX_OSTicksToMS (tm.tms_cstime));
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
        if (link (srcPath, destPath) != 0) {
            Tcl_AppendResult (interp, "linking \"", srcPath, "\" to \"",
                              destPath, "\" failed: ",
                              Tcl_PosixError (interp), (char *) NULL);
            goto errorExit;
        }
    }

    Tcl_DStringFree (&srcPathBuf);
    Tcl_DStringFree (&destPathBuf);
    return TCL_OK;

  unixError:
    interp->result = Tcl_PosixError (interp);

  errorExit:
    Tcl_DStringFree (&srcPathBuf);
    Tcl_DStringFree (&destPathBuf);
    return TCL_ERROR;
}
