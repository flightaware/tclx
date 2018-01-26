/*
 * tclXexists.c --
 *
 *  "exists" Tcl command and math function.
 *-----------------------------------------------------------------------------
 * Copyright 2018 Karl Lehenbauer and Mark Diekhans.
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
 * TclX_ExistsObjCmd --
 *     Implements the TCL exists command:
 *     exists var
 *
 * Results:
 *  true is returned if the variable exists, else false.
 *
 *-----------------------------------------------------------------------------
 */
static int
TclX_ExistsObjCmd (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    if (objc != 2) {
        return TclX_WrongArgs (interp, objv [0], "var");
    }

    Tcl_SetObjResult (interp, Tcl_NewBooleanObj (Tcl_ObjGetVar2 (interp, objv[1], NULL, 0) != NULL));
    return TCL_OK;
}



/*-----------------------------------------------------------------------------
 * TclX_ExistsInit --
 *     Initialize the exists command.
 *-----------------------------------------------------------------------------
 */
void
TclX_ExistsInit (Tcl_Interp *interp)
{
    Tcl_CreateObjCommand (interp,
			  "exists",
			  TclX_ExistsObjCmd,
              (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "tcl::mathfunc::exists",
			  TclX_ExistsObjCmd,
              (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);
}

/* vim: set ts=4 sw=4 sts=4 et : */
