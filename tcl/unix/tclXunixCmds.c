/*
 * tclXunixCmds.c --
 *
 * Tcl commands to access unix system calls that are not portable to other
 * platforms.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1997 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXunixCmds.c,v 8.0.4.1 1997/04/14 02:02:49 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*-----------------------------------------------------------------------------
 * Tcl_ChrootObjCmd --
 *     Implements the TCL chroot command:
 *         chroot path
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ChrootObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    char   *chrootString;
    int     chrootStrLen;

    if (objc != 2)
	return TclX_WrongArgs (interp, objv [0], "path");

    chrootString = Tcl_GetStringFromObj (objv [1], &chrootStrLen);

    if (chroot (chrootString) < 0) {
        TclX_StringAppendObjResult (interp, "changing root to \"", 
				    chrootString,
                                    "\" failed: ", Tcl_PosixError (interp),
                                    (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_TimesObjCmd --
 *     Implements the TCL times command:
 *     times
 *
 * Results:
 *  Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_TimesObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    struct tms tm;
    char       timesBuf [48];

    if (objc != 1)
	return TclX_WrongArgs (interp, objv [0], "");

    times (&tm);

    sprintf (timesBuf, "%ld %ld %ld %ld", 
             TclXOSTicksToMS (tm.tms_utime),
             TclXOSTicksToMS (tm.tms_stime),
             TclXOSTicksToMS (tm.tms_cutime),
             TclXOSTicksToMS (tm.tms_cstime));

    Tcl_SetStringObj (Tcl_GetObjResult (interp), timesBuf, -1);
    return TCL_OK;
}


