/* 
 * tclXtest.c --
 *
 *  Test support functions for the Extended Tcl test program.
 *
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
 * $Id: tclXtest.c,v 7.3 1996/09/28 16:22:38 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

int
Tclxtest_Init _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * Error handler proc that causes errors to come out in the same format as
 * the standard Tcl test shell.  This keeps certain Tcl tests from reporting
 * errors.
 */
static char errorHandler [] =
    "proc tclx_errorHandler msg {global errorInfo; \
     if [lempty $errorInfo] {puts $msg} else {puts stderr $errorInfo}; \
     exit 1}";

/*
 * Prototypes of internal functions.
 */
static int
TclxTestEvalCmd _ANSI_ARGS_((ClientData    clientData,
                             Tcl_Interp   *interp,
                             int           argc,
                             char        **argv));


/*-----------------------------------------------------------------------------
 * DoTestEval --
 *   Evaluate a level/command pair.
 * Parameters:
 *   o interp - Errors are returned in result.
 *   o levelStr - Level string to parse.
 *   o command - Command to evaluate.
 *   o resultList - DString to append the two element eval code and result to.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
DoTestEval (interp, levelStr, command, resultList)
    Tcl_Interp  *interp;
    char        *levelStr;
    char        *command;
    Tcl_DString *resultList;
{
    Interp *iPtr = (Interp *) interp;
    int result;
    char numBuf [32];
    CallFrame *savedVarFramePtr, *framePtr;

    /*
     * Find the frame to eval in.
     */
    result = TclGetFrame (interp, levelStr, &framePtr);
    if (result <= 0) {
        if (result == 0)
            Tcl_AppendResult (interp, "invalid level \"", levelStr, "\"",
                              (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Evaluate in the new environment.
     */
    savedVarFramePtr = iPtr->varFramePtr;
    iPtr->varFramePtr = framePtr;

    result = Tcl_Eval (interp, command);

    iPtr->varFramePtr = savedVarFramePtr;

    /*
     * Append the two element list.
     */
    Tcl_DStringStartSublist (resultList);
    sprintf (numBuf, "%d", result);
    Tcl_DStringAppendElement (resultList, numBuf);
    Tcl_DStringAppendElement (resultList, interp->result);
    Tcl_DStringEndSublist (resultList);

    Tcl_ResetResult (interp);
    return TCL_OK;
}


/*-----------------------------------------------------------------------------
 * TclxTestEvalCmd --
 *    Command used in profile test.  It purpose is to evaluate a series of
 * commands at a specified level.  Its like uplevel, but can generate more
 * complex situations.  Level is specified in the same manner as uplevel,
 * with 0 being the current level.
 *     tclx_test_eval ?level cmd? ?level cmd? ...
 *
 * Results:
 *   A list contain a list entry for each command evaluated.  Each entry is
 * the eval code and result string.
 *-----------------------------------------------------------------------------
 */
static int
TclxTestEvalCmd (clientData, interp, argc, argv)
    ClientData    clientData;
    Tcl_Interp   *interp;
    int           argc;
    char        **argv;
{
    int idx;
    Tcl_DString resultList;

    if (((argc - 1) % 2) != 0) {
        Tcl_AppendResult (interp, "wrong # args: ", argv [0],
                          " ?level cmd? ?level cmd? ...", (char *) NULL);
        return TCL_ERROR;
    }

    Tcl_DStringInit (&resultList);

    for (idx = 1; idx < argc; idx += 2) {
        if (DoTestEval (interp, argv [idx], argv [idx + 1],
                        &resultList) == TCL_ERROR) {
            Tcl_DStringFree (&resultList);
            return TCL_ERROR;
        }
    }
        
    Tcl_DStringResult (interp, &resultList);
    Tcl_DStringFree (&resultList);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tclxtest_Init --
 *  Initialize TclX test support.
 *
 * Results:
 *   Returns a standard Tcl completion code, and leaves an error message in
 * interp->result if an error occurs.
 *-----------------------------------------------------------------------------
 */
int
Tclxtest_Init (interp)
    Tcl_Interp *interp;
{
    Tcl_CreateCommand (interp, "tclx_test_eval", TclxTestEvalCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    return Tcl_GlobalEval (interp, errorHandler);
}
