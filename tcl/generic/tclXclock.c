/* 
 * tclXclock.c --
 *
 *      Contains the TclX time and date related commands.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1995 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXclock.c,v 4.3 1995/04/25 03:46:17 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_GetclockCmd --
 *     Implements the TCL getclock command:
 *         getclock
 *
 * Results:
 *     Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetclockCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    if (argc != 1) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0], (char *) NULL);
        return TCL_ERROR;
    }
    sprintf (interp->result, "%ld", time ((time_t *) NULL));
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_FmtclockCmd --
 *     Implements the TCL fmtclock command:
 *         fmtclock clockval ?format? ?GMT|{}?
 *
 * Results:
 *     Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_FmtclockCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int              useGMT = FALSE;
    time_t           clockVal;
    char            *format;
    struct tm       *timeDataPtr;
    Tcl_DString      buffer;
    int              bufSize;
#ifdef TCL_USE_TIMEZONE_VAR
    int              savedTimeZone;
    char            *savedTZEnv;
#endif

#ifdef HAVE_TZSET
    /*
     * Some systems forgot to call tzset in localtime, make sure its done.
     */
    static int  calledTzset = FALSE;

    if (!calledTzset) {
        tzset ();
        calledTzset = TRUE;
    }
#endif

    if ((argc < 2) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " clockval ?format? ?GMT|{}?", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetTime (interp, argv[1], &clockVal) != TCL_OK)
        return TCL_ERROR;
    if ((argc == 4) && (argv [3][0] != '\0')) {
        if (!STREQU (argv [3], "GMT")) {
            Tcl_AppendResult (interp, "expected \"GMT\" or {} got \"",
                              argv [3], "\"", (char *) NULL);
            return TCL_ERROR;
        }
        useGMT = TRUE;
    }

    if ((argc >= 3) && (argv [2][0] != '\0'))
        format = argv[2];
    else
        format = "%a %b %d %X %Z %Y";

#ifdef TCL_USE_TIMEZONE_VAR
    /*
     * This is a horrible kludge for systems not having the timezone in
     * struct tm.  No matter what was specified, they use the global time
     * zone.  (Thanks Solaris).
     */
    if (useGMT) {
        char *varValue;

        varValue = Tcl_GetVar2 (interp, "env", "TZ", TCL_GLOBAL_ONLY);
        if (varValue != NULL)
            savedTZEnv = ckstrdup (varValue);
        else
            savedTZEnv = NULL;
        Tcl_SetVar2 (interp, "env", "TZ", "GMT", TCL_GLOBAL_ONLY);
        savedTimeZone = timezone;
        timezone = 0;
        tzset ();
    }
#endif

    if (useGMT)
        timeDataPtr = gmtime (&clockVal);
    else    
        timeDataPtr = localtime (&clockVal);

    /*
     * Format the time, increasing the buffer size until strftime succeeds.
     */
    bufSize = TCL_DSTRING_STATIC_SIZE - 1;
    Tcl_DStringInit (&buffer);
    Tcl_DStringSetLength (&buffer, bufSize);

    while (strftime (buffer.string, bufSize, format, timeDataPtr) == 0) {
        bufSize *= 2;
        Tcl_DStringSetLength (&buffer, bufSize);
    }

#ifdef TCL_USE_TIMEZONE_VAR
    if (useGMT) {
        if (savedTZEnv != NULL) {
            Tcl_SetVar2 (interp, "env", "TZ", savedTZEnv, TCL_GLOBAL_ONLY);
            ckfree (savedTZEnv);
        } else {
            Tcl_UnsetVar2 (interp, "env", "TZ", TCL_GLOBAL_ONLY);
        }
        timezone = savedTimeZone;
        tzset ();
    }
#endif

    Tcl_DStringResult (interp, &buffer);
    return TCL_OK;
}
