/*
 * tclXcmdInit.c
 *
 * Function to add the Extented Tcl commands into an interpreter.  The TclX
 * library commands are not added here, to make it easier to build applications
 * that don't use the extended libraries.
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
 * $Id: tclXcmdInit.c,v 8.1 1997/04/17 04:58:34 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*-----------------------------------------------------------------------------
 * Tclxcmd_Init --
 *
 *   Add the Extended Tcl commands to the specified interpreter (except for
 * the library commands that override that standard Tcl procedures).  This
 * does no other startup.
 *-----------------------------------------------------------------------------
 */
int
Tclxcmd_Init (interp)
    Tcl_Interp *interp;
{
    if (Tclxcmd_SafeInit (interp) == TCL_ERROR)
        return TCL_ERROR;

    /*
     * from tclCkalloc.c (now part of the UCB Tcl).
     */
#ifdef TCL_MEM_DEBUG    
    Tcl_InitMemory (interp);
#endif

    /*
     * from tclXchmod.c
     */
    Tcl_CreateObjCommand (interp, 
			  "chgrp",
			  TclX_ChgrpObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "chmod",
			  TclX_ChmodObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
                          "chown",
			  TclX_ChownObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXcmdloop.c
     */
    Tcl_CreateCommand (interp,
		       "commandloop",
		       TclX_CommandloopCmd, 
                       (ClientData) NULL,
		       (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXdebug.c
     */
    TclX_DebugInit (interp);

    /*
     * from tclXdup.c
     */
    TclX_DupInit (interp);

    /*
     * from tclXfcntl.c
     */
    Tcl_CreateObjCommand (interp, 
			  "fcntl",
			  TclX_FcntlObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXfilecmds.c
     */
    Tcl_CreateObjCommand (interp,
			  "pipe",
			  TclX_PipeObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "copyfile",
			  TclX_CopyfileObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
                          "lgets",
                          TclX_LgetsObjCmd,
                          (ClientData) NULL,
                          (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "ftruncate",
			  TclX_FtruncateObjCmd,
			  (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
                          "readdir",
			  TclX_ReaddirObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXmsgcat.c
     */
    TclX_InitMsgCat (interp);

    /*
     * from tclXprocess.c
     */
    Tcl_CreateCommand (interp,
		       "execl",
		       TclX_ExeclCmd,
                       (ClientData) NULL,
		       (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
                          "fork",
			  TclX_ForkObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateCommand (interp,
		       "wait",
		       TclX_WaitCmd,
                       (ClientData) NULL,
		       (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXsignal.c
     */
    TclX_InitSignalHandling (interp);

    /*
     * from tclXoscmds.c
     */
    Tcl_CreateObjCommand (interp,
			  "alarm",
			  TclX_AlarmObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "nice",
			  TclX_NiceObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "sleep",
			  TclX_SleepObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
                          "sync",
			  TclX_SyncObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
                          "system",
			  TclX_SystemObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "umask",
			  TclX_UmaskObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXunixCmds.c or tclXwinCmds.c
     */
    Tcl_CreateObjCommand (interp,
			  "chroot",
			  TclX_ChrootObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand (interp,
			  "link",
			  TclX_LinkObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, 
			  "times",
			  TclX_TimesObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);
    
    /*
     * from tclXsocket.c
     */
    Tcl_CreateObjCommand (interp, 
			  "host_info",
			  TclX_HostInfoObjCmd,
                          (ClientData) NULL, 
			  (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXunixSock.c and stubbed in tclXwinCmds.c.
     */
    TclX_ServerInit (interp);

    return TCL_OK;
}


/*-----------------------------------------------------------------------------
 * Tclxcmd_SafeInit --
 *
 *   Add the safe Extended Tcl commands to the specified interpreter.
 *-----------------------------------------------------------------------------
 */
int
Tclxcmd_SafeInit (interp)
    Tcl_Interp *interp;
{
    TclX_SetAppInfo (TRUE,
                     "TclX",
                     "Extended Tcl",
                     TCLX_FULL_VERSION,
                     TCLX_PATCHLEVEL);


    /*
     * from tclXbsearch.c
     */
    Tcl_CreateCommand (interp, 
		       "bsearch",
		       TclX_BsearchCmd, 
                      (ClientData) NULL,
		      (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXfstat.c
     */
    Tcl_CreateCommand (interp,
		       "fstat",
		       TclX_FstatCmd,
                       (ClientData) NULL,
		       (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXflock.c
     */
    Tcl_CreateCommand (interp,
		       "flock",
		       TclX_FlockCmd,
                       (ClientData) NULL,
		       (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateCommand (interp, 
		       "funlock",
		       TclX_FunlockCmd,
                       (ClientData) NULL,
		       (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXfilescan.c
     */
    TclX_InitFilescan (interp);

    /*
     * from tclXgeneral.c
     */
    Tcl_CreateObjCommand (interp, 
			  "echo",
			  TclX_EchoObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp, 
			 "infox",
			 TclX_InfoxObjCmd,
                         (ClientData) NULL,
			 (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp, 
			 "loop",
			 TclX_LoopObjCmd,
                         (ClientData) NULL,
			 (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXid.c
     */
    Tcl_CreateObjCommand (interp,
			  "id",
			  TclX_IdObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXkeylist.c
     */
    TclX_KeyedListInit (interp);

    /*
     * from tclXlist.c
     */
    TclX_ListInit (interp);

    /*
     * from tclXmath.c
     */
    TclX_InitMath (interp);

    /*
     * from tclXprofile.c
     */
    TclX_ProfileInit (interp);

    /*
     * from tclXselect.c
     */
    Tcl_CreateCommand (interp, 
		       "select",
		       TclX_SelectCmd,
                       (ClientData) NULL,
		       (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXstring.c
     */
    TclX_StringInit (interp);

    return TCL_OK;
}


