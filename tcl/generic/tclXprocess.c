/*
 * tclXprocess.c --
 *
 * Tcl command to create and manage processes.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1993 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXprocess.c,v 2.6 1993/07/18 15:19:48 markd Exp markd $
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
 * Tcl_ExeclCmd --
 *     Implements the TCL execl command:
 *     execl prog ?argList?
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
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
    char  *staticArgv [STATIC_ARG_SIZE];
    char **argInList = NULL;
    char **argList   = staticArgv;
    int    argInCnt, idx;

    if ((argc < 2) || (argc > 3)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " prog ?argList?", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * If arg list is supplied, split it and build up the arguments to pass.
     * otherwise, just supply argv[0].  Must be NULL terminated.
     */
    if (argc > 2) {
        if (Tcl_SplitList (interp, argv [2], &argInCnt, &argInList) != TCL_OK)
            return TCL_ERROR;

        if (argInCnt > STATIC_ARG_SIZE - 2)
            argList = (char **) ckalloc ((argInCnt + 1) * sizeof (char **));
            
        for (idx = 0; idx < argInCnt; idx++)
            argList [idx + 1] = argInList [idx];

        argList [argInCnt + 1] = NULL;
    } else {
        argList [1] = NULL;
    }

    argList [0] = argv [1];  /* Program name */

    if (execvp (argv[1], argList) < 0) {
        if (argInList != NULL)
            ckfree (argInList);
        if (argList != staticArgv)
            ckfree (argList);

        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ForkCmd --
 *     Implements the TCL fork command:
 *     fork
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ForkCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int pid;

    if (argc != 1) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], (char *) NULL);
        return TCL_ERROR;
    }

    pid = fork ();
    if (pid < 0) {
        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }

    sprintf(interp->result, "%d", pid);
    return TCL_OK;
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
    int   status, idx, tmpPid, options = 0, pgroup = FALSE;
    pid_t pid, returnedPid;
    
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
#if NO_WAITPID
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

    returnedPid = waitpid (pid, &status, options);

    if (returnedPid < 0) {
        interp->result = Tcl_PosixError (interp);
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
