/*
 * tclXmath.c --
 *
 * Mathematical Tcl commands.
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
 * $Id: tclXmath.c,v 5.4 1996/03/22 04:31:40 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Define return of random function unless stdlib does it.  If we are using
 * out own version, make sure to define it.
 */
#if defined(NO_RANDOM) || defined(NO_RANDOM_PROTO)
long random ();
#endif

/*
 * Prototypes of internal functions.
 */
static int
ConvertIntOrDouble _ANSI_ARGS_((Tcl_Interp *interp,
                                char       *numStr,
                                double     *valuePtr));

static long 
ReallyRandom _ANSI_ARGS_((long my_range));



/*-----------------------------------------------------------------------------
 * ConvertIntOrDouble --
 *
 *   Convert a number that can be in any legal integer or floating point
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
static int
ConvertIntOrDouble (interp, numStr, valuePtr)
    Tcl_Interp *interp;
    char       *numStr;
    double     *valuePtr;
{
    long lvalue;

    if (strpbrk (numStr, ".eE") != NULL) {
        return Tcl_GetDouble (interp, numStr, valuePtr);
    } else {
        if (Tcl_GetLong (interp, numStr, &lvalue) != TCL_OK)
            return TCL_ERROR;
        *valuePtr = (double) lvalue;
        return TCL_OK;
    }
}

/*-----------------------------------------------------------------------------
 * Tcl_MaxCmd --
 *      Implements the Tcl max command:
 *        max num1 ?..numN?
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_MaxCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    double value, maxValue = -MAXDOUBLE;
    int idx, maxIdx   =  1;

    if (argc < 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " num1 ?..numN?", (char *) NULL);
        return TCL_ERROR;
    }

    for (idx = 1; idx < argc; idx++) {
        if (ConvertIntOrDouble (interp, argv [idx], &value) != TCL_OK)
            return TCL_ERROR;
        if (value > maxValue) {
            maxValue = value;
            maxIdx = idx;
        }
    }
    strcpy (interp->result, argv [maxIdx]);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_MinCmd --
 *     Implements the TCL min command:
 *         min num1 ?..numN?
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_MinCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int     argc;
    char      **argv;
{
    double value, minValue = MAXDOUBLE;
    int idx, minIdx   = 1;

    if (argc < 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " num1 ?..numN?", (char *) NULL);
        return TCL_ERROR;
    }

    for (idx = 1; idx < argc; idx++) {
        if (ConvertIntOrDouble (interp, argv [idx], &value) != TCL_OK)
            return TCL_ERROR;
        if (value < minValue) {
            minValue = value;
            minIdx = idx;
            }
        }
    strcpy (interp->result, argv [minIdx]);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 *
 * Tcl_MaxFunc --
 *      Implements the Tcl max math function
 *        expr max(num1, num2)
 *
 * Results:
 *      Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
static int
Tcl_MaxFunc (clientData, interp, args, resultPtr)
    ClientData  clientData;
    Tcl_Interp *interp;
    Tcl_Value  *args;
    Tcl_Value  *resultPtr;
{
    if ((args [0].type == TCL_INT) && (args [1].type == TCL_INT)) {
        resultPtr->type = TCL_INT;

        if (args [0].intValue > args [1].intValue) {
            resultPtr->intValue = args [0].intValue;
        } else {
            resultPtr->intValue = args [1].intValue;
        }
    } else {
        double values [2];

        resultPtr->type = TCL_DOUBLE;

        values [0] = (args [0].type == TCL_INT) ? (double) args [0].intValue :
                                                  args [0].doubleValue;
        values [1] = (args [1].type == TCL_INT) ? (double) args [1].intValue :
                                                  args [1].doubleValue;

        if (values [0] > values [1]) {
            resultPtr->doubleValue = values [0];
        } else {
            resultPtr->doubleValue = values [1];
        }
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 *
 * Tcl_MinFunc --
 *      Implements the Tcl min math function
 *        expr min(num1, num2)
 *
 * Results:
 *      Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
static int
Tcl_MinFunc (clientData, interp, args, resultPtr)
    ClientData  clientData;
    Tcl_Interp *interp;
    Tcl_Value  *args;
    Tcl_Value  *resultPtr;
{
    if ((args [0].type == TCL_INT) && (args [1].type == TCL_INT)) {
        resultPtr->type = TCL_INT;

        if (args [0].intValue < args [1].intValue) {
            resultPtr->intValue = args [0].intValue;
        } else {
            resultPtr->intValue = args [1].intValue;
        }
    } else {
        double values [2];

        resultPtr->type = TCL_DOUBLE;

        values [0] = (args [0].type == TCL_INT) ? (double) args [0].intValue :
                                                  args [0].doubleValue;
        values [1] = (args [1].type == TCL_INT) ? (double) args [1].intValue :
                                                  args [1].doubleValue;

        if (values [0] < values [1]) {
            resultPtr->doubleValue = values [0];
        } else {
            resultPtr->doubleValue = values [1];
        }
    }
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
ReallyRandom (myRange)
    long myRange;
{
    long maxMultiple, rnum;

    maxMultiple = RANDOM_RANGE / myRange;
    maxMultiple *= myRange;
    while ((rnum = random ()) >= maxMultiple)
        continue;
    return (rnum % myRange);
}

/*-----------------------------------------------------------------------------
 * Tcl_RandomCmd  --
 *     Implements the TCL random command:
 *     random limit | seed ?seedval?
 *
 * Results:
 *  Standard TCL results.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_RandomCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    long range;

    if ((argc < 2) || (argc > 3))
        goto invalidArgs;

    if (STREQU (argv [1], "seed")) {
        unsigned seed;

        if (argc == 3) {
            if (Tcl_GetUnsigned (interp, argv[2], &seed) != TCL_OK)
                return TCL_ERROR;
        } else
            seed = (unsigned) (getpid() + time((time_t *)NULL));

        srandom (seed);

    } else {
        if (argc != 2)
            goto invalidArgs;
        if (Tcl_GetLong (interp, argv[1], &range) != TCL_OK)
            return TCL_ERROR;
        if ((range <= 0) || (range > RANDOM_RANGE))
            goto outOfRange;

        sprintf (interp->result, "%ld", ReallyRandom (range));
    }
    return TCL_OK;

  invalidArgs:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                      " limit | seed ?seedval?", (char *) NULL);
    return TCL_ERROR;

  outOfRange:
    {
        char buf [18];

        sprintf (buf, "%ld", RANDOM_RANGE);
        Tcl_AppendResult (interp, "range must be > 0 and <= ",
                          buf, (char *) NULL);
        return TCL_ERROR;
    }
}

/*-----------------------------------------------------------------------------
 *  Tcl_InitMath --
 *
 *    Initialize the TclX math commands and functions.
 *-----------------------------------------------------------------------------
 */
void
Tcl_InitMath (interp)
    Tcl_Interp *interp;
{
    Tcl_ValueType minMaxArgTypes [2];

    minMaxArgTypes [0] = TCL_EITHER;
    minMaxArgTypes [1] = TCL_EITHER;

    Tcl_CreateCommand (interp, "max", Tcl_MaxCmd,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "min", Tcl_MinCmd,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "random", Tcl_RandomCmd,
                       (ClientData) NULL, (void (*)()) NULL);

    Tcl_CreateMathFunc (interp, "max",
                        2, minMaxArgTypes,
                        Tcl_MaxFunc,
                        (ClientData) NULL);

    Tcl_CreateMathFunc (interp, "min",
                        2, minMaxArgTypes,
                        Tcl_MinFunc,
                        (ClientData) NULL);

}
