/*
 * tclXunixCmds.c --
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
 * $Id: tclXunixCmds.c,v 7.1 1996/07/18 19:36:32 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


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
             TclXOSTicksToMS (tm.tms_utime),
             TclXOSTicksToMS (tm.tms_stime),
             TclXOSTicksToMS (tm.tms_cutime),
             TclXOSTicksToMS (tm.tms_cstime));
    return TCL_OK;
}
