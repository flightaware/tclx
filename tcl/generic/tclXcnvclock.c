/* 
 * tclXcnvclock.c --
 *
 *      Contains the TCL convertclock command.  This is in a module seperate
 * from clock so that it can be excluded, along with the yacc generated code,
 * since its rather large.
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
 * $Id: tclXcnvclock.c,v 4.4 1995/07/01 19:03:44 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_GetTimeZone --
 *   Determines the current timezone.  The method varies wildly between
 * different Unix implementations, so its hidden in this function.
 *
 * Parameters:
 *   o currentTime (I) - The clock value that is to be used for the current
 *     time.  This is really a time_t, but is long so it can be in tclExtend.h
 *     without requiring time_t.
 * 
 * Returns:
 *    Minutes east of GMT.
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetTimeZone (currentTime)
    long  currentTime;
{
/*
 * Determine how a timezone is obtained from "struct tm".  If there is no time
 * zone in this struct (very lame) then use the timezone variable.  This is
 * done in a way to make the timezone variable the method of last resort, as
 * some systems have it in addition to a field in "struct tm".  The
 * gettimeofday system call can also be used to determine the time zone.
 */
#if defined(HAVE_TM_TZADJ)
#   define TCL_GOT_TIMEZONE
    time_t      curTime = (time_t) currentTime;
    struct tm  *timeDataPtr = localtime (&curTime);
    int         timeZone;

    timeZone = timeDataPtr->tm_tzadj  / 60;
    if (timeDataPtr->tm_isdst)
        timeZone += 60;

    return timeZone;
#endif

#if defined(HAVE_TM_GMTOFF) && !defined (TCL_GOT_TIMEZONE)
#   define TCL_GOT_TIMEZONE
    time_t     curTime = (time_t) currentTime;
    struct tm *timeDataPtr = localtime (&currentTime);
    int        timeZone;

    timeZone = -(timeDataPtr->tm_gmtoff / 60);
    if (timeDataPtr->tm_isdst)
        timeZone += 60;

    return timeZone;
#endif

#if defined(HAVE_GETTIMEOFDAY) && !defined (TCL_GOT_TIMEZONE)
#   define TCL_GOT_TIMEZONE
    struct timeval  tv;
    struct timezone tz;
    int             timeZone;

    gettimeofday (&tv, &tz);
    timeZone = tz.tz_minuteswest;
    if (tz.tz_dsttime)
        timeZone += 60;

    return timeZone;
#endif

#if defined(HAVE_TIMEZONE_VAR) && !defined (TCL_GOT_TIMEZONE)
#   define TCL_GOT_TIMEZONE
    static int setTZ = FALSE;
    int        timeZone;

    if (!setTZ) {
        tzset ();
        setTZ = TRUE;
    }
    timeZone = timezone / 60;

    return timeZone;
#endif

#ifndef TCL_GOT_TIMEZONE
   /*
    * Cause compile error, we don't know how to get timezone.
    */
  error: autoconf did not figure out how to determine the timezone. 
#endif

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ConvertclockCmd --
 *     Implements the TCL convertclock command:
 *         convertclock dateString ?GMT|{}?
 *
 * Results:
 *     Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ConvertclockCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    time_t      baseClock, clockVal;
    long        zone;

    if ((argc < 2) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " dateString ?GMT|{}? ?baseclock?", (char *) NULL);
	return TCL_ERROR;
    }
    if (argc == 4) {
        if (Tcl_GetTime (interp, argv [3], &baseClock) != TCL_OK)
            return TCL_ERROR;
    } else {
        time (&baseClock);
    }

    if ((argc > 2) && (argv [2][0] != '\0')) {
        if (!STREQU (argv [2], "GMT")) {
            Tcl_AppendResult (interp, "invalid argument: expected `GMT', ",
                              "got : `", argv [2], "'", (char *) NULL);
            return TCL_ERROR;
        }
        zone = -50000; /* Force GMT */
    } else {
        zone = Tcl_GetTimeZone (baseClock);
    }

    if (Tcl_GetDate (argv [1], baseClock, zone, &clockVal) < 0) {
        Tcl_AppendResult (interp, "Unable to convert date-time string \"",
                          argv [1], "\"", (char *) NULL);
	return TCL_ERROR;
    }

    sprintf (interp->result, "%ld", (long) clockVal);
    return TCL_OK;
}

