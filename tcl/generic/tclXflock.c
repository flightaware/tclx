/*
 * tclXflock.c
 *
 * Extended Tcl flock and funlock commands.
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
 * $Id: tclXflock.c,v 7.2 1996/08/19 16:20:18 markd Exp $
 *-----------------------------------------------------------------------------
 */
/* FIX: Need to add an interface to F_GETLK */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
ParseLockUnlockArgs _ANSI_ARGS_((Tcl_Interp     *interp,
                                 int             argc,
                                 char          **argv,
                                 int             argIdx,
                                 TclX_FlockInfo *lockInfoPtr));


/*-----------------------------------------------------------------------------
 * ParseLockUnlockArgs --
 *
 * Parse the positional arguments common to both the flock and funlock
 * commands:
 *   ... fileId ?start? ?length? ?origin?
 *
 * Parameters:
 *   o interp - Pointer to the interpreter, errors returned in result.
 *   o argc - Count of arguments supplied to the comment.
 *   o argv - Commant argument vector.
 *   o argIdx - Index of the first common agument to parse.
 *   o access - Set of TCL_READABLE or TCL_WRITABLE or zero to
 *     not do error checking.
 *   o lockInfoPtr - Lock info structure, start, length and whence are
 *     initialized by this routine.  The access and block fields should already
 *     be filled in.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ParseLockUnlockArgs (interp, argc, argv, argIdx, lockInfoPtr)
    Tcl_Interp     *interp;
    int             argc;
    char          **argv;
    int             argIdx;
    TclX_FlockInfo *lockInfoPtr;
{
    lockInfoPtr->start  = 0;
    lockInfoPtr->len    = 0;
    lockInfoPtr->whence = 0;

    lockInfoPtr->channel = TclX_GetOpenChannel (interp, argv [argIdx],
                                                lockInfoPtr->access);
    if (lockInfoPtr->channel == NULL)
        return TCL_ERROR;
    argIdx++;

    if ((argIdx < argc) && (argv [argIdx][0] != '\0')) {
        if (Tcl_GetOffset (interp, argv [argIdx],
                           &lockInfoPtr->start) != TCL_OK)
            return TCL_ERROR;
    }
    argIdx++;

    if ((argIdx < argc) && (argv [argIdx][0] != '\0')) {
        if (Tcl_GetOffset (interp, argv [argIdx],
                           &lockInfoPtr->len) != TCL_OK)
            return TCL_ERROR;
    }
    argIdx++;

    if (argIdx < argc) {
        if (STREQU (argv [argIdx], "start"))
            lockInfoPtr->whence = 0;
        else if (STREQU (argv [argIdx], "current"))
            lockInfoPtr->whence = 1;
        else if (STREQU (argv [argIdx], "end"))
            lockInfoPtr->whence = 2;
        else
            goto badOrgin;
    }

    return TCL_OK;

  badOrgin:
    Tcl_AppendResult(interp, "bad origin \"", argv [argIdx],
                     "\": should be \"start\", \"current\", or \"end\"",
                     (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_FlockCmd --
 *
 * Implements the `flock' Tcl command:
 *    flock ?-read|-write? ?-nowait? fileId ?start? ?length? ?origin?
 *
 * Results:
 *      A standard Tcl result.
 *-----------------------------------------------------------------------------
 */
int
Tcl_FlockCmd (notUsed, interp, argc, argv)
    ClientData   notUsed;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int argIdx;
    TclX_FlockInfo lockInfo;

    if (argc < 2)
        goto invalidArgs;

    lockInfo.access = 0;
    lockInfo.block = TRUE;

    /*
     * Parse off the options.
     */
    for (argIdx = 1; (argIdx < argc) && (argv [argIdx][0] == '-'); argIdx++) {
        if (STREQU (argv [argIdx], "-read")) {
            lockInfo.access |= TCL_READABLE;
            continue;
        }
        if (STREQU (argv [argIdx], "-write")) {
            lockInfo.access |= TCL_WRITABLE;
            continue;
        }
        if (STREQU (argv [argIdx], "-nowait")) {
            lockInfo.block = FALSE;
            continue;
        }
        goto invalidOption;
    }

    if (lockInfo.access == (TCL_READABLE | TCL_WRITABLE))
        goto bothReadAndWrite;
    if (lockInfo.access == 0)
        lockInfo.access = TCL_WRITABLE;

    /*
     * Make sure there are enough arguments left and then parse the 
     * positional ones.
     */
    if ((argIdx > argc - 1) || (argIdx < argc - 4))
        goto invalidArgs;

    if (ParseLockUnlockArgs (interp, argc, argv, argIdx, &lockInfo) != TCL_OK)
        return TCL_ERROR;

    if (TclXOSFlock (interp, &lockInfo) != TCL_OK)
        return TCL_ERROR;

    if (!lockInfo.block)
        Tcl_SetResult (interp, lockInfo.gotLock ? "1" : "0", TCL_STATIC);

    return TCL_OK;

    /*
     * Code to return error messages.
     */
  invalidArgs:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ?-read|-write? ",
                      "?-nowait? fileId ?start? ?length? ?origin?",
                      (char *) NULL);
    return TCL_ERROR;

    /*
     * Invalid option found at argv [argIdx].
     */
  invalidOption:
    Tcl_AppendResult (interp, "invalid option \"", argv [argIdx],
                      "\" expected one of \"-read\", \"-write\", or ",
                      "\"-nowait\"", (char *) NULL);
    return TCL_ERROR;

  bothReadAndWrite:
    Tcl_AppendResult (interp, "can not specify both \"-read\" and \"-write\"",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_FunlockCmd --
 *
 * Implements the `funlock' Tcl command:
 *    funlock fileId ?start? ?length? ?origin?
 *
 * Results:
 *      A standard Tcl result.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_FunlockCmd (notUsed, interp, argc, argv)
    ClientData   notUsed;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    TclX_FlockInfo lockInfo;

    if ((argc < 2) || (argc > 5))
        goto invalidArgs;

    lockInfo.access = 0;  /* Read or write */
    if (ParseLockUnlockArgs (interp, argc, argv, 1, &lockInfo) != TCL_OK)
        return TCL_ERROR;

    return TclXOSFunlock (interp, &lockInfo);

  invalidArgs:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                      " fileId ?start? ?length? ?origin?", (char *) NULL);
    return TCL_ERROR;
}
