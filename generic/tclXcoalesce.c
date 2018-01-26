/*
 * tclXcoalesce.c --
 *
 *  coalesce Tcl commands.
 *-----------------------------------------------------------------------------
 * Copyright 2017 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*-----------------------------------------------------------------------------
 * TclX_CoalesceObjCmd --
 *     Implements the TCL chmod command:
 *     coalesce var ?var...? defaultValue
 *
 * Results:
 *  The value of the first existing variable is returned.
 *  If no variables exist, the default value is returned.
 *
 *-----------------------------------------------------------------------------
 */
static int
TclX_CoalesceObjCmd (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int i;
    Tcl_Obj *val;

    if (objc < 3) {
        return TclX_WrongArgs (interp, objv [0], "var ?var...? defaultValue");
    }

    for (i = 1; i < objc - 1; i++) {
        if ((val = Tcl_ObjGetVar2 (interp, objv [i], NULL, 0)) != NULL) {
            Tcl_SetObjResult (interp, val);
            return TCL_OK;
        }
    }

    Tcl_SetObjResult (interp, objv[objc - 1]);
    return TCL_OK;
}


/*-----------------------------------------------------------------------------
 * TclX_CoalesceInit --
 *     Initialize the coalesce command.
 *-----------------------------------------------------------------------------
 */
void
TclX_CoalesceInit (Tcl_Interp *interp)
{
    Tcl_CreateObjCommand (interp,
			  "coalesce",
			  TclX_CoalesceObjCmd,
              (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "tcl::mathfunc::coalesce",
			  TclX_CoalesceObjCmd,
              (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);
}

/* vim: set ts=4 sw=4 sts=4 et : */
