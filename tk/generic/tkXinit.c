/*
 * tkXinit.c --
 *
 * Initialization code for the wishx and other Tk & Extended Tcl based
 * applications.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1995 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tkXinit.c,v 4.5 1995/06/30 23:27:19 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"
#include "tk.h"

/*
 * Used to override the library and library environment variable used to
 * find the TkX startup file and runtime library.  The values of these
 * fields must be changed before TkX_Init is called.
 */
char *tkX_library    = TKX_LIBRARY;
char *tkX_libraryEnv = "TKX_LIBRARY";

/*
 * Procedure that is used to determine if Tk_Init has already been called.
 */
static char *tkInitProc = "tkScreenChanged";


/*
 *-----------------------------------------------------------------------------
 *
 * TkX_Init --
 *
 *   Do Tk initialization for wishx.  This includes overriding the value
 * of the variable "tk_library" so it points to our library instead of 
 * the standard one.
 *
 * Parameters:
 *   o interp - A pointer to the interpreter.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TkX_Init (interp)
    Tcl_Interp  *interp;
{
    static char initCmd[] =
	"if [file exists $tk_library/tk.tcl] {\n\
	    source $tk_library/tk.tcl\n\
	} else {\n\
	    set msg \"can't find $tk_library/tk.tcl; perhaps you \"\n\
	    append msg \"need to\\ninstall TkX (from TclX) or set your \"\n\
	    append msg \"TKX_LIBRARY environment variable?\"\n\
	    error $msg\n\
	}";

    char        *interact, *libDir;
    Tcl_CmdInfo  cmdInfo;

    /*
     * Make sure main window exists, or Tk_Init will fail in a confusing
     * way.
     */
    if (Tk_MainWindow (interp) == NULL) {
        Tcl_AppendResult (interp,
                          "TkX_Init called before calling Tk_CreateMainWindow",
                          (char *) NULL);
        return TCL_ERROR;
    }

    tclAppName     = "Wishx";
    tclAppLongname = "Extended Tk Shell - Wishx";
    tclAppVersion  = TK_VERSION;

    /*
     * If Tk_Init has been called, don't do anything else.
     */
    if (Tcl_GetCommandInfo (interp, tkInitProc, &cmdInfo)) {
        return TCL_OK;
    }

    /*
     * Set tk_library to point to the TkX library.  It maybe an empty
     * string if the path is not defined.
     */
    libDir = NULL;
    if (tkX_libraryEnv != NULL) {
        libDir = Tcl_GetVar2 (interp, "env", tkX_libraryEnv, TCL_GLOBAL_ONLY);
    }
    if (libDir == NULL) {
        libDir = Tcl_GetVar (interp, "tk_library", TCL_GLOBAL_ONLY);
    }
    if (libDir == NULL) {
        if (tkX_library != NULL)
            libDir = tkX_library;
        else
            libDir = "";
    }
    if (Tcl_SetVar (interp, "tk_library", libDir,
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;


    /*
     * Find and run the Tk initialization file.
     */
    return TclX_Eval(interp,
                     TCLX_EVAL_GLOBAL | TCLX_EVAL_ERR_HANDLER,
                     initCmd);
}
