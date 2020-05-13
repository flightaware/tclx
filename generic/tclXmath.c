/*
 * tclXmath.c --
 *
 * Mathematical Tcl commands.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1999 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * Copyright 2005 ActiveState Corporation.
 *
 *-----------------------------------------------------------------------------
 * $Id: tclXmath.c,v 1.2 2005/03/16 23:48:21 hobbs Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

#ifndef tclx_random
#  define tclx_srandom srandom
#  define tclx_random random
#endif

#ifdef TCL_WIDE_INT_TYPE
#define GET_DOUBLE_VALUE(doubleVar, argPtr, type)		\
    if ((type) == TCL_INT) {					\
	(doubleVar) = (double) (argPtr).intValue;		\
    } else if ((type) == TCL_WIDE_INT) {			\
	(doubleVar) = Tcl_WideAsDouble((argPtr).wideValue);	\
    } else { /* TCL_DOUBLE */					\
	(doubleVar) = (argPtr).doubleValue;			\
    }
#else
#define GET_DOUBLE_VALUE(doubleVar, argPtr, type)		\
    if ((type) == TCL_INT) {					\
	(doubleVar) = (double) (argPtr).intValue;		\
    } else { /* TCL_DOUBLE */					\
	(doubleVar) = (argPtr).doubleValue;			\
    }
#endif

/*
 * Prototypes of internal functions.
 */
static int	ConvertIntOrDoubleObj (Tcl_Interp *interp,
				                   Tcl_Obj    *numStrObj,
                                   double     *valuePtr);

static long	ReallyRandom (long my_range);

static int	TclX_MaxObjCmd (ClientData clientData,
                            Tcl_Interp *interp,
                            int         objc,
                            Tcl_Obj    *const objv[]);

static int	TclX_MinObjCmd (ClientData  clientData,
                            Tcl_Interp *interp,
                            int         objc,
                            Tcl_Obj    *const objv[]);

static int	TclX_RandomObjCmd (ClientData  clientData,
                               Tcl_Interp *interp,
				               int         objc,
                               Tcl_Obj     *const objv[]);


/*-----------------------------------------------------------------------------
 * ConvertIntOrDoubleObj --
 *
 *   Convert a number object that can be in any legal integer or floating point
 * format (including integer hex and octal specifications) to a double.
 *
 * Parameters:
 *   o interp (I) - Interpreters, errors are returns in result.
 *   o numStr (I) - Number to convert.
 *   o valuePtr (O) - Double value is returned here.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int	ConvertIntOrDoubleObj (Tcl_Interp *interp,
				                   Tcl_Obj    *numStrObj,
                                   double     *valuePtr)
{
#ifdef TCL_WIDE_INT_TYPE
    Tcl_WideInt wVal;

    if (Tcl_GetWideIntFromObj (interp, numStrObj, &wVal) == TCL_OK) {
	*valuePtr = Tcl_WideAsDouble(wVal);
	return TCL_OK;
    }
#else
    long lvalue;

    if (Tcl_GetLongFromObj (interp, numStrObj, &lvalue) == TCL_OK) {
	*valuePtr = (double) lvalue;
	return TCL_OK;
    }
#endif

    if (Tcl_GetDoubleFromObj (interp, numStrObj, valuePtr) == TCL_OK)
	return TCL_OK;

    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_MaxObjCmd --
 *      Implements the Tcl max command:
 *        max num1 ?..numN?
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
static int	TclX_MaxObjCmd (ClientData clientData,
                            Tcl_Interp *interp,
                            int         objc,
                            Tcl_Obj    *const objv[])
{
    double value, maxValue = -MAXDOUBLE;
    int idx, maxIdx = 1;

    if (objc < 2)
	return TclX_WrongArgs (interp, objv[0], "num1 ?..numN?");

    for (idx = 1; idx < objc; idx++) {
	if (ConvertIntOrDoubleObj (interp, objv [idx], &value) != TCL_OK)
	    return TCL_ERROR;
        if (value > maxValue) {
            maxValue = value;
            maxIdx = idx;
        }
    }
    Tcl_SetObjResult (interp, objv [maxIdx]);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_MinObjCmd --
 *     Implements the TCL min command:
 *         min num1 ?..numN?
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
static int	TclX_MinObjCmd (ClientData  clientData,
                            Tcl_Interp *interp,
                            int         objc,
                            Tcl_Obj    *const objv[])
{
    double value, minValue = MAXDOUBLE;
    int idx, minIdx   = 1;

    if (objc < 2)
	return TclX_WrongArgs (interp, objv[0], "num1 ?..numN?");

    for (idx = 1; idx < objc; idx++) {
        if (ConvertIntOrDoubleObj (interp, objv [idx], &value) != TCL_OK)
            return TCL_ERROR;
        if (value < minValue) {
            minValue = value;
            minIdx = idx;
	}
    }
    Tcl_SetObjResult (interp, objv [minIdx]);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * ReallyRandom --
 *     Insure a good random return for a range, unlike an arbitrary
 *     random() % n, thanks to Ken Arnold, Unix Review, October 1987.
 *-----------------------------------------------------------------------------
 */
#define RANDOM_RANGE 0x7fffffffL

static long 
ReallyRandom (long myRange)
{
    long maxMultiple, rnum;

    maxMultiple = RANDOM_RANGE / myRange;
    maxMultiple *= myRange;
    while ((rnum = tclx_random ()) >= maxMultiple)
        continue;
    return (rnum % myRange);
}

/*-----------------------------------------------------------------------------
 * TclX_RandomObjCmd  --
 *     Implements the TCL random command:
 *     random limit | seed ?seedval?
 *
 * Results:
 *  Standard TCL results.
 *-----------------------------------------------------------------------------
 */
static int	TclX_RandomObjCmd (ClientData  clientData,
                               Tcl_Interp *interp,
				               int         objc,
                               Tcl_Obj     *const objv[])
{
    long range;
    char *seedString;

    if ((objc < 2) || (objc > 3))
        goto invalidArgs;

    if (Tcl_GetLongFromObj ((Tcl_Interp *) NULL, objv [1], &range) == TCL_OK) {
        if (objc != 2)
            goto invalidArgs;

        if ((range <= 0) || (range > RANDOM_RANGE))
            goto outOfRange;
    } else {
        int seed;

	seedString = Tcl_GetStringFromObj (objv [1], NULL);
	if (!STREQU (seedString, "seed"))
	    goto invalidArgs;

	if (objc == 3) {
	    if (Tcl_GetIntFromObj (interp, objv[2], &seed) != TCL_OK)
		return TCL_ERROR;
	} else {
	    seed = (getpid () + time ((time_t *)NULL));
	}
	(void) tclx_srandom (seed);
	return TCL_OK;
    }
    Tcl_SetIntObj (Tcl_GetObjResult (interp), ReallyRandom (range));
    return TCL_OK;

  invalidArgs:
    return TclX_WrongArgs (interp, objv[0], "limit | seed ?seedval?");

  outOfRange:
    {
        char buf [18];

        sprintf (buf, "%ld", RANDOM_RANGE);
        TclX_AppendObjResult (interp, " range must be > 0 and <= ", buf,
                              (char *) NULL);
        return TCL_ERROR;
    }
}

/*-----------------------------------------------------------------------------
 *  TclX_MathInit --
 *
 *    Initialize the TclX math commands and functions.
 *-----------------------------------------------------------------------------
 */
void
TclX_MathInit (Tcl_Interp *interp)
{
    int major, minor;

    Tcl_CreateObjCommand (interp, "max", TclX_MaxObjCmd,
	    (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "min", TclX_MinObjCmd,
	    (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "random", TclX_RandomObjCmd,
	    (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

}


/* vim: set ts=4 sw=4 sts=4 et : */
