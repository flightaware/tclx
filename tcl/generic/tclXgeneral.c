/* 
 * tclXgeneral.c --
 *
 * A collection of general commands: echo, infox and loop.
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
 * $Id: tclXgeneral.c,v 8.2 1997/06/12 21:08:19 markd Exp $
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
 * TclX_EchoObjCmd --
 *    Implements the TCL echo command:
 *        echo ?str ...?
 *
 * Results:
 *      Always returns TCL_OK.
 *-----------------------------------------------------------------------------
 */
int
TclX_EchoObjCmd (dummy, interp, objc, objv)
     ClientData  dummy;
     Tcl_Interp *interp;
     int         objc;
     Tcl_Obj    *CONST objv[];
{
    int   idx;
    Tcl_Channel channel;
    char *stringPtr;
    int stringPtrLen;

    channel = TclX_GetOpenChannel (interp, "stdout", TCL_WRITABLE);
    if (channel == NULL)
        return TCL_ERROR;

    for (idx = 1; idx < objc; idx++) {
	stringPtr = Tcl_GetStringFromObj (objv [idx], &stringPtrLen);
        if (Tcl_Write (channel, stringPtr, stringPtrLen) < 0)
            goto posixError;
        if (idx < (objc - 1)) {
            if (Tcl_Write (channel, " ", 1) < 0)
                goto posixError;
        }
    }
    if (TclX_WriteNL (channel) < 0)
        goto posixError;
    return TCL_OK;

  posixError:
    Tcl_SetStringObj (Tcl_GetObjResult (interp), Tcl_PosixError (interp), -1);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_InfoxObjCmd --
 *    Implements the TCL infox command:
 *        infox option
 *-----------------------------------------------------------------------------
 */
int
TclX_InfoxObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj    *CONST objv[];
{
    Tcl_Obj *resultPtr = Tcl_GetObjResult (interp);
    char *optionPtr;

    /*
     * FIX: Need a way to get the have_ functionality from the OS-dependent
     * code.
     */
    if (objc != 2)
	return TclX_WrongArgs (interp, objv[0], "option");

    optionPtr = Tcl_GetStringFromObj (objv[1], NULL);

    if (STREQU ("version", optionPtr)) {
        if (tclxVersion != NULL) {
            Tcl_SetStringObj (resultPtr, tclxVersion, -1);
        }
        return TCL_OK;
    }
    if (STREQU ("patchlevel", optionPtr)) {
	Tcl_SetIntObj (resultPtr, tclxPatchlevel);
        return TCL_OK;
    }
    if (STREQU ("have_fchown", optionPtr)) {
#       ifndef NO_FCHOWN
        Tcl_SetBooleanObj (resultPtr, TRUE);
#       else
        Tcl_SetBooleanObj (resultPtr, FALSE);
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_fchmod", optionPtr)) {
#       ifndef NO_FCHMOD
        Tcl_SetBooleanObj (resultPtr, TRUE);
#       else
        Tcl_SetBooleanObj (resultPtr, FALSE);
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_flock", optionPtr)) {
	if (TclXOSHaveFlock ())
	    Tcl_SetBooleanObj (resultPtr, TRUE);
	else
	    Tcl_SetBooleanObj (resultPtr, FALSE);
        return TCL_OK;
    }
    if (STREQU ("have_fsync", optionPtr)) {
#       ifndef NO_FSYNC
        Tcl_SetBooleanObj (resultPtr, TRUE);
#       else
        Tcl_SetBooleanObj (resultPtr, FALSE);
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_ftruncate", optionPtr)) {
#       if (!defined(NO_FTRUNCATE)) || defined(HAVE_CHSIZE)
        Tcl_SetBooleanObj (resultPtr, TRUE);
#       else
        Tcl_SetBooleanObj (resultPtr, FALSE);
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_msgcats", optionPtr)) {
#       ifndef NO_CATGETS
        Tcl_SetBooleanObj (resultPtr, TRUE);
#       else
        Tcl_SetBooleanObj (resultPtr, FALSE);
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_posix_signals", optionPtr)) {
#       ifndef NO_SIGACTION
        Tcl_SetBooleanObj (resultPtr, TRUE);
#       else
        Tcl_SetBooleanObj (resultPtr, FALSE);
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_truncate", optionPtr)) {
#       ifndef NO_TRUNCATE
        Tcl_SetBooleanObj (resultPtr, TRUE);
#       else
        Tcl_SetBooleanObj (resultPtr, FALSE);
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_symlink", optionPtr)) {
#       ifdef S_IFLNK
        Tcl_SetBooleanObj (resultPtr, TRUE);
#       else
        Tcl_SetBooleanObj (resultPtr, FALSE);
#       endif        
        return TCL_OK;
    }
    if (STREQU ("have_waitpid", optionPtr)) {
#       ifndef NO_WAITPID
        Tcl_SetBooleanObj (resultPtr, TRUE);
#       else
        Tcl_SetBooleanObj (resultPtr, FALSE);
#       endif        
        return TCL_OK;
    }
    if (STREQU ("appname", optionPtr)) {
        if (tclAppName != NULL) {
            Tcl_SetStringObj (resultPtr, tclAppName, -1);
        }
        return TCL_OK;
    }
    if (STREQU ("applongname", optionPtr)) {
        if (tclAppLongName != NULL)
            Tcl_SetStringObj (resultPtr, tclAppLongName, -1);
        return TCL_OK;
    }
    if (STREQU ("appversion", optionPtr)) {
        if (tclAppVersion != NULL)
            Tcl_SetStringObj (resultPtr, tclAppVersion, -1);
        return TCL_OK;
    }
    if (STREQU ("apppatchlevel", optionPtr)) {
	if (tclAppPatchlevel >= 0)
	    Tcl_SetIntObj (resultPtr, tclAppPatchlevel);
	else
	    Tcl_SetIntObj (resultPtr, 0);
        return TCL_OK;
    }
    TclX_StringAppendObjResult (interp, 
                                "illegal option \"",
                                optionPtr,
                                "\", expect one of: version, patchlevel, ",
                                "have_fchown, have_fchmod, have_flock, ",
                                "have_fsync, have_ftruncate, have_msgcats, ",
                                "have_symlink, have_truncate, ",
                                "have_posix_signals, have_waitpid, appname, ",
                                "applongname, appversion, or apppatchlevel",
                                (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_LoopObjCmd --
 *     Implements the TCL loop command:
 *         loop var start end ?increment? command
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
int
TclX_LoopObjCmd (dummy, interp, objc, objv)
     ClientData  dummy;
     Tcl_Interp *interp;
     int         objc;
     Tcl_Obj    *CONST objv[];
{
    int       result = TCL_OK;
    int      i, first, limit, incr = 1;
    Tcl_Obj  *command, *iObj;

    if ((objc < 5) || (objc > 6))
	return TclX_WrongArgs (interp, objv [0], 
                               "var first limit ?incr? command");

    if (Tcl_ExprStringObj (interp, objv [2]) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetIntFromObj (interp, Tcl_GetObjResult (interp),
                           &first) != TCL_OK)
	return TCL_ERROR;

    if (Tcl_ExprStringObj (interp, objv [3]) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetIntFromObj (interp, Tcl_GetObjResult (interp),
                           &limit) != TCL_OK)
	return TCL_ERROR;

    if (objc == 5) {
        command = objv [4];
    } else {
	if (Tcl_ExprStringObj (interp, objv [4]) != TCL_OK)
	    return TCL_ERROR;
	if (Tcl_GetIntFromObj (interp, Tcl_GetObjResult (interp), 
			       &incr) != TCL_OK)
	    return TCL_ERROR;
        command = objv [5];
    }

    for (i = first;
         (((i < limit) && (incr >= 0)) || ((i > limit) && (incr < 0)));
         i += incr) {
        
        
	iObj = Tcl_ObjGetVar2 (interp, objv [1], (Tcl_Obj *) NULL,
                               TCL_PARSE_PART1);
        if ((iObj == NULL) || (Tcl_IsShared (iObj))) {
            iObj = Tcl_NewIntObj (first);
            if (Tcl_ObjSetVar2 (interp, objv [1], (Tcl_Obj *) NULL, iObj,
                                TCL_PARSE_PART1 | TCL_LEAVE_ERR_MSG) == NULL) {
                Tcl_DecrRefCount (iObj);
                return TCL_ERROR;
            }
        }
        Tcl_SetIntObj (iObj, i);

        result = Tcl_EvalObj (interp, command);
        if (result == TCL_CONTINUE) {
            result = TCL_OK;
        } else if (result != TCL_OK) {
            if (result == TCL_BREAK) {
                result = TCL_OK;
            } else if (result == TCL_ERROR) {
                char buf [64];
                
                sprintf (buf, "\n    (\"loop\" body line %d)", 
                         interp->errorLine);
		Tcl_AppendStringsToObj (Tcl_GetObjResult (interp), buf, 
                                        (char *) NULL);
            }
            break;
        }
    }

    /*
     * Set variable to its final value.
     */
    iObj = Tcl_ObjGetVar2 (interp, objv [1], (Tcl_Obj *) NULL,
                           TCL_PARSE_PART1);
    if ((iObj == NULL) || (Tcl_IsShared (iObj))) {
        iObj = Tcl_NewIntObj (first);
        if (Tcl_ObjSetVar2 (interp, objv [1], (Tcl_Obj *) NULL, iObj,
                            TCL_PARSE_PART1 | TCL_LEAVE_ERR_MSG) == NULL) {
            Tcl_DecrRefCount (iObj);
            return TCL_ERROR;
        }
    }
    Tcl_SetIntObj (iObj, i);

    return result;
}


