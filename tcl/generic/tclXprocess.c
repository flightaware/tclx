/*
 * tclXprocess.c --
 *
 * Tcl command to create and manage processes.
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
 * $Id: tclXprocess.c,v 1.3 1997/01/02 05:44:51 karl Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * These are needed for wait command even if waitpid is not available.
 */
#ifndef  WNOHANG
#    define  WNOHANG    1
#endif
#ifndef  WUNTRACED
#    define  WUNTRACED  2
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ForkObjCmd --
 *     Implements the TCL fork command:
 *     fork
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ForkObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    if (objc != 1)
	return TclX_WrongArgs (interp, objv [0], "");

    return TclXOSfork (interp, objv [0]);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ExeclCmd --
 *     Implements the TCL execl command:
 *     execl prog ?argList?
 *
 * Results:
 *   Standard TCL results, may return the UNIX system error message.  On Win32,
 * a process id is returned.
 *-----------------------------------------------------------------------------
 */
int
Tcl_ExeclCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
#define STATIC_ARG_SIZE   12
    char          *staticArgv [STATIC_ARG_SIZE];
    char         **argInList   = NULL;
    char         **argList     = staticArgv;
    char          *path;
    char          *argv0       = NULL;
    int            nextArg     = 1;
    int            argInCnt, idx, status;
    Tcl_DString    pathBuf;


    status = TCL_ERROR;  /* assume the worst */

    if (argc < 2)
        goto wrongArgs;

    if (STREQU ("-argv0", argv [1])) {
        if (argc < 4)
            goto wrongArgs;
        argv0 = argv [2];
        nextArg = 3;
    }
    if ((argc - nextArg) > 2)
        goto wrongArgs;

    Tcl_DStringInit (&pathBuf);

    /*
     * If arg list is supplied, split it and build up the arguments to pass.
     * otherwise, just supply argv[0].  Must be NULL terminated.
     */
    if (argc - 1 > nextArg) {
        if (Tcl_SplitList (interp, argv [nextArg + 1],
                           &argInCnt, &argInList) != TCL_OK)
            goto exitPoint;

        if (argInCnt > STATIC_ARG_SIZE - 2)
            argList = (char **) ckalloc ((argInCnt + 1) * sizeof (char **));
            
        for (idx = 0; idx < argInCnt; idx++)
            argList [idx + 1] = argInList [idx];

        argList [argInCnt + 1] = NULL;
    } else {
        argList [1] = NULL;
    }

    path = Tcl_TranslateFileName (interp, argv [nextArg], &pathBuf);
    if (path == NULL)
        goto exitPoint;

    if (argv0 != NULL) {
        argList [0] = argv0;
    } else {
	argList [0] = path;  /* Program name */
    }

    status = TclXOSexecl (interp, path, argList);

  exitPoint:
    if (argInList != NULL)
        ckfree ((char *) argInList);
    if (argList != staticArgv)
        ckfree ((char *) argList);
    Tcl_DStringFree (&pathBuf);
    return status;

  wrongArgs:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                      " ?-argv0 argv0? prog ?argList?",
                      (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_WaitCmd --
 *   Implements the TCL wait command:
 *     wait ?-nohang? ?-untraced? ?-pgroup? ?pid?
 *
 * Results:
 *   Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_WaitCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int    idx, tmpPid, options = 0, pgroup = FALSE;
    pid_t  pid, returnedPid;

    WAIT_STATUS_TYPE status;    

    for (idx = 1; idx < argc; idx++) {
        if (argv [idx][0] != '-')
            break;
        if (STREQU ("-nohang", argv [idx])) {
            if (options & WNOHANG)
                goto usage;
            options |= WNOHANG;
            continue;
        }
        if (STREQU ("-untraced", argv [idx])) {
            if (options & WUNTRACED)
                goto usage;
            options |= WUNTRACED;
            continue;
        }
        if (STREQU ("-pgroup", argv [idx])) {
            if (pgroup)
                goto usage;
            pgroup = TRUE;
            continue;
        }
        goto usage;  /* None match */
    }
    /*
     * Check for more than one non-minus argument.  If ok, convert pid,
     * if supplied.
     */
    if (idx < argc - 1)
        goto usage;  
    if (idx < argc) {
        if (!Tcl_StrToInt (argv [idx], 10, &tmpPid))
            goto invalidPid;
        if (tmpPid <= 0)
            goto negativePid;
        pid = (pid_t) tmpPid;
        if ((int) pid != tmpPid)
            goto invalidPid;
    } else {
        pid = -1;  /* pid or pgroup not supplied */
    }

    /*
     * Versions that don't have real waitpid have limited functionality.
     */
#ifdef NO_WAITPID
    if ((options != 0) || pgroup) {
        Tcl_AppendResult (interp, "The \"-nohang\", \"-untraced\" and ",
                          "\"-pgroup\" options are not available on this ",
                          "system", (char *) NULL);
        return TCL_ERROR;
    }
#endif

    if (pgroup) {
        if (pid > 0)
            pid = -pid;
        else
            pid = 0;
    }

    returnedPid = TCLX_WAITPID (pid, (int *) &status, options);

    if (returnedPid < 0) {
        Tcl_AppendResult (interp, "wait for process failed: ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * If no process was available, return an empty status.  Otherwise return
     * a list contain the PID and why it stopped.
     */
    if (returnedPid == 0)
        return TCL_OK;
    
    if (WIFEXITED (status))
        sprintf (interp->result, "%d %s %d", returnedPid, "EXIT", 
                 WEXITSTATUS (status));
    else if (WIFSIGNALED (status))
        sprintf (interp->result, "%d %s %s", returnedPid, "SIG", 
                 Tcl_SignalId (WTERMSIG (status)));
    else if (WIFSTOPPED (status))
        sprintf (interp->result, "%d %s %s", returnedPid, "STOP", 
                 Tcl_SignalId (WSTOPSIG (status)));

    return TCL_OK;

  usage:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ", 
                      "?-nohang? ?-untraced? ?-pgroup? ?pid?",
                      (char *) NULL);
    return TCL_ERROR;

  invalidPid:
    Tcl_AppendResult (interp, "invalid pid or process group id \"",
                      argv [idx], "\"", (char *) NULL);
    return TCL_ERROR;

  negativePid:
    Tcl_AppendResult (interp, "pid or process group id must be greater ",
                      "than zero", (char *) NULL);
    return TCL_ERROR;
}
