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
 * $Id: tclXflock.c,v 6.0 1996/05/10 16:15:33 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
ParseLockUnlockArgs _ANSI_ARGS_((Tcl_Interp    *interp,
                                 int            argc,
                                 char         **argv,
                                 int            argIdx,
                                 int            access,
                                 struct flock  *lockInfoPtr));

/*
 * If F_SETLKW is not defined, we assume file locking is not available.
 */
#ifdef F_SETLKW

/*-----------------------------------------------------------------------------
 * ParseLockUnlockArgs --
 *
 * Parse the positional arguments common to both the flock and funlock
 * commands:
 *   ... fileId ?start? ?length? ?origin?
 *
 * Parameters:
 *   o interp (I) - Pointer to the interpreter, errors returned in result.
 *   o argc (I) - Count of arguments supplied to the comment.
 *   o argv (I) - Commant argument vector.
 *   o argIdx (I) - Index of the first common agument to parse.
 *   o access (I) - Set of TCL_READABLE or TCL_WRITABLE or zero to
 *     not do error checking.
 *   o lockInfoPtr (O) - Fcntl info structure, start, length and whence
 *     are initialized by this routine.
 * Returns:
 *   The file number or -1 if an error occurs.
 *-----------------------------------------------------------------------------
 */
static int
ParseLockUnlockArgs (interp, argc, argv, argIdx, access, lockInfoPtr)
    Tcl_Interp    *interp;
    int            argc;
    char         **argv;
    int            argIdx;
    int            access;
    struct flock  *lockInfoPtr;
{
    int fnum;

    lockInfoPtr->l_start  = 0;
    lockInfoPtr->l_len    = 0;
    lockInfoPtr->l_whence = 0;

    fnum = TclX_GetOpenFnum (interp, argv [argIdx], access);
    if (fnum < 0)
	return -1;
    argIdx++;

    if ((argIdx < argc) && (argv [argIdx][0] != '\0')) {
        if (Tcl_GetOffset (interp, argv [argIdx],
                           &lockInfoPtr->l_start) != TCL_OK)
            return -1;
    }
    argIdx++;

    if ((argIdx < argc) && (argv [argIdx][0] != '\0')) {
        if (Tcl_GetOffset (interp, argv [argIdx],
                           &lockInfoPtr->l_len) != TCL_OK)
            return -1;
    }
    argIdx++;

    if (argIdx < argc) {
        if (STREQU (argv [argIdx], "start"))
            lockInfoPtr->l_whence = 0;
        else if (STREQU (argv [argIdx], "current"))
            lockInfoPtr->l_whence = 1;
        else if (STREQU (argv [argIdx], "end"))
            lockInfoPtr->l_whence = 2;
        else
            goto badOrgin;
    }

    return fnum;

  badOrgin:
    Tcl_AppendResult(interp, "bad origin \"", argv [argIdx],
                     "\": should be \"start\", \"current\", or \"end\"",
                     (char *) NULL);
    return -1;
}

/*-----------------------------------------------------------------------------
 * Tcl_FlockCmd --
 *
 * Implements the `flock' Tcl command:
 *    flock ?-read|-write? ?-nowait? fileId ?start? ?length? ?origin?
 *
 * Results:
 *      A standard Tcl result.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_FlockCmd (notUsed, interp, argc, argv)
    ClientData   notUsed;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int argIdx, stat, fnum;
    int readLock = FALSE, writeLock = FALSE, noWaitLock = FALSE;
    int access = 0;
    struct flock  lockInfo;

    if (argc < 2)
        goto invalidArgs;

    /*
     * Parse off the options.
     */
    for (argIdx = 1; (argIdx < argc) && (argv [argIdx][0] == '-'); argIdx++) {
        if (STREQU (argv [argIdx], "-read")) {
            access |= TCL_READABLE;
            continue;
        }
        if (STREQU (argv [argIdx], "-write")) {
            access |= TCL_WRITABLE;
            continue;
        }
        if (STREQU (argv [argIdx], "-nowait")) {
            noWaitLock = TRUE;
            continue;
        }
        goto invalidOption;
    }

    if (access == (TCL_READABLE | TCL_WRITABLE))
        goto bothReadAndWrite;
    if (access == 0)
        access = TCL_WRITABLE;

    /*
     * Make sure there are enough arguments left and then parse the 
     * positional ones.
     */
    if ((argIdx > argc - 1) || (argIdx < argc - 4))
        goto invalidArgs;

    fnum = ParseLockUnlockArgs (interp, argc, argv, argIdx, access, &lockInfo);
    if (fnum < 0)
        return TCL_ERROR;

    lockInfo.l_type = (access == TCL_WRITABLE) ? F_WRLCK : F_RDLCK;
    
    stat = fcntl (fnum, noWaitLock ? F_SETLK : F_SETLKW, 
                  &lockInfo);

    /*
     * Check to see if the lock failed due to it being locked or
     * an error.
     */
    if ((stat < 0) && !((errno == EACCES) || (errno == EAGAIN))) {
        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }
    
    if (noWaitLock)
        interp->result = (stat < 0) ? "0" : "1";

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
    interp->result = "can not specify both \"-read\" and \"-write\"";
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
    int fnum;
    struct flock  lockInfo;

    if ((argc < 2) || (argc > 5))
        goto invalidArgs;

    fnum = ParseLockUnlockArgs (interp, argc, argv, 1, 0, &lockInfo);
    if (fnum < 0)
        return TCL_ERROR;

    lockInfo.l_type = F_UNLCK;
    
    if (fcntl (fnum, F_SETLK, &lockInfo) < 0) {
        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }
    return TCL_OK;

  invalidArgs:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                      " fileId ?start? ?length? ?origin?", (char *) NULL);
    return TCL_ERROR;
}
#else

/*-----------------------------------------------------------------------------
 * Tcl_FlockCmd --
 * Tcl_FunlockCmd --
 *
 * Versions of the command that always returns an error on systems that
 * don't have file locking.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_FlockCmd (notUsed, interp, argc, argv)
    ClientData   notUsed;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    interp->result = "File locking is not available on this system";
    return TCL_ERROR;
}

int
Tcl_FunlockCmd (notUsed, interp, argc, argv)
    ClientData   notUsed;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    return Tcl_FlockCmd (notUsed, interp, argc, argv);
}
#endif
