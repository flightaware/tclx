/* 
 * tclXgeneral.c --
 *
 *      Contains general extensions to the basic TCL command set.
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
 * $Id: tclXgeneral.c,v 5.9 1996/03/21 06:35:49 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Values returned by the infox command.
 */

static char *tclxVersion       = TCLX_FULL_VERSION;
static int   tclxPatchlevel    = TCLX_PATCHLEVEL;
static char *tclAppName        = NULL;
static char *tclAppLongName    = NULL;
static char *tclAppVersion     = NULL;
static int   tclAppPatchlevel  = -1;


/*-----------------------------------------------------------------------------
 * TclX_SetAppInfo --
 *   Store the application information returned by infox.
 *
 * Parameters:
 *   o defaultValues (I) - If true, then the values are assigned only if they
 *     are not already defined (defaulted).  If false, the values are always
 *     set.
 *   o appName (I) - Application symbolic name.  
 *   o appLongName (I) - Long, natural language application name.
 *   o appVersion (I) - Version number of the application.
 *   o appPatchlevel (I) - Patch level of the application.  If less than
 *     zero, don't change.
 * Notes:
 *   String pointers are saved without copying, don't release the memory.
 * If the arguments are NULL, don't change the values.
 *-----------------------------------------------------------------------------
 */
void
TclX_SetAppInfo (defaultValues, appName, appLongName, appVersion,
                 appPatchlevel)
    int   defaultValues;
    char *appName;
    char *appLongName;
    char *appVersion;
    int   appPatchlevel;
{
    if ((appName != NULL) &&
        ((!defaultValues) || (tclAppName == NULL))) {
        tclAppName = appName;
    }
    if ((appLongName != NULL) &&
        ((!defaultValues) || (tclAppLongName == NULL))) {
        tclAppLongName = appLongName;
    }
    if ((appVersion != NULL) &&
        ((!defaultValues) || (tclAppVersion == NULL))) {
        tclAppVersion = appVersion;
    }
    if ((appPatchlevel >= 0) &&
        ((!defaultValues) || (tclAppPatchlevel < 0))) {
        tclAppPatchlevel = appPatchlevel;
    }
}


/*-----------------------------------------------------------------------------
 * Tcl_EchoCmd --
 *    Implements the TCL echo command:
 *        echo ?str ...?
 *
 * Results:
 *      Always returns TCL_OK.
 *-----------------------------------------------------------------------------
 */
int
Tcl_EchoCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int   idx;
    Tcl_Channel channel;

    channel = TclX_GetOpenChannel (interp, "stdout", TCL_WRITABLE);
    if (channel == NULL)
        return TCL_ERROR;

    for (idx = 1; idx < argc; idx++) {
        if (TclX_WriteStr (channel, argv [idx]) < 0)
            goto posixError;
        if (idx < (argc - 1)) {
            if (Tcl_Write (channel, " ", 1) < 0)
                goto posixError;
        }
    }
    if (TclX_WriteNL (channel) < 0)
        goto posixError;
    return TCL_OK;

  posixError:
    interp->result = Tcl_PosixError (interp);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_InfoxCmd --
 *    Implements the TCL infox command:
 *        infox option
 *-----------------------------------------------------------------------------
 */
int
Tcl_InfoxCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    char numBuf [32];

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " option", (char *) NULL);
        return TCL_ERROR;
    }

    if (STREQU ("version", argv [1])) {
        if (tclxVersion != NULL)
            Tcl_SetResult (interp, tclxVersion, TCL_STATIC);
        return TCL_OK;
    }
    if (STREQU ("patchlevel", argv [1])) {
        sprintf (numBuf, "%d", tclxPatchlevel);
        Tcl_SetResult (interp, numBuf, TCL_VOLATILE);
        return TCL_OK;
    }
    if (STREQU ("have_fchown", argv [1])) {
#       ifndef NO_FCHOWN
        interp->result = "1";
#       else
        interp->result = "0";
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_fchmod", argv [1])) {
#       ifndef NO_FCHMOD
        interp->result = "1";
#       else
        interp->result = "0";
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_flock", argv [1])) {
#       ifdef F_SETLKW
        interp->result = "1";
#       else
        interp->result = "0";
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_fsync", argv [1])) {
#       ifndef NO_FSYNC
        interp->result = "1";
#       else
        interp->result = "0";
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_ftruncate", argv [1])) {
#       if (!defined(NO_FTRUNCATE)) || defined(HAVE_CHSIZE)
        interp->result = "1";
#       else
        interp->result = "0";
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_msgcats", argv [1])) {
#       ifndef NO_CATGETS
        interp->result = "1";
#       else
        interp->result = "0";
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_posix_signals", argv [1])) {
#       ifndef NO_SIGACTION
        interp->result = "1";
#       else
        interp->result = "0";
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_truncate", argv [1])) {
#       ifndef NO_TRUNCATE
        interp->result = "1";
#       else
        interp->result = "0";
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_waitpid", argv [1])) {
#       ifndef NO_WAITPID
        interp->result = "1";
#       else
        interp->result = "0";
#       endif        
        return TCL_OK;
    }
    if (STREQU ("appname", argv [1])) {
        if (tclAppName != NULL)
            Tcl_SetResult (interp, tclAppName, TCL_STATIC);
        return TCL_OK;
    }
    if (STREQU ("applongname", argv [1])) {
        if (tclAppLongName != NULL)
            Tcl_SetResult (interp, tclAppLongName, TCL_STATIC);
        return TCL_OK;
    }
    if (STREQU ("appversion", argv [1])) {
        if (tclAppVersion != NULL)
            Tcl_SetResult (interp, tclAppVersion, TCL_STATIC);
        return TCL_OK;
    }
    if (STREQU ("apppatchlevel", argv [1])) {
        sprintf (numBuf, "%d", (tclAppPatchlevel >= 0) ? tclAppPatchlevel : 0);
        Tcl_SetResult (interp, numBuf, TCL_VOLATILE);
        return TCL_OK;
    }

    Tcl_AppendResult (interp, "illegal option \"", argv [1], 
                      "\" expect one of: version, patchlevel, ",
                      "have_fchown, have_fchmod, have_flock, ",
                      "have_fsync, have_ftruncate, have_msgcats, ",
                      "have_truncate, have_posix_signals, ",
                      "have_waitpid, ",
                      "appname, applongname, appversion, or apppatchlevel",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_LoopCmd --
 *     Implements the TCL loop command:
 *         loop var start end ?increment? command
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
int
Tcl_LoopCmd (dummy, interp, argc, argv)
    ClientData  dummy;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int   result = TCL_OK;
    long  i, first, limit, incr = 1;
    char *command;
    char  itxt [32];

    if ((argc < 5) || (argc > 6)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " var first limit ?incr? command", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_ExprLong (interp, argv[2], &first) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_ExprLong (interp, argv[3], &limit) != TCL_OK)
        return TCL_ERROR;
    if (argc == 5) {
        command = argv[4];
    } else {
        if (Tcl_ExprLong (interp, argv[4], &incr) != TCL_OK)
            return TCL_ERROR;
        command = argv[5];
    }

    for (i = first;
             (((i < limit) && (incr >= 0)) || ((i > limit) && (incr < 0)));
             i += incr) {

        sprintf (itxt,"%ld",i);
        if (Tcl_SetVar (interp, argv [1], itxt, TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;

        result = Tcl_Eval (interp, command);
        if (result == TCL_CONTINUE) {
            result = TCL_OK;
        } else if (result != TCL_OK) {
            if (result == TCL_BREAK) {
                result = TCL_OK;
            } else if (result == TCL_ERROR) {
                char buf [64];
                
                sprintf (buf, "\n    (\"loop\" body line %d)", 
                         interp->errorLine);
                Tcl_AddErrorInfo (interp, buf);
            }
            break;
        }
    }

    /*
     * Set variable to its final value.
     */
    sprintf (itxt,"%ld",i);
    if (Tcl_SetVar (interp, argv [1], itxt, TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    return result;
}
