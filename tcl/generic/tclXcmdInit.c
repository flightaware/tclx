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
 * $Id: tclXcmdInit.c,v 8.0.4.1 1997/04/14 02:01:38 markd Exp $
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
    Tcl_CreateObjCommand (interp, "chgrp", -1,
			  Tcl_ChgrpObjCmd, (ClientData) NULL, 
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "chmod", -1,
                          Tcl_ChmodObjCmd, (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "chown", -1,
			  Tcl_ChownObjCmd, (ClientData) NULL, 
			  (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXcmdloop.c
     */
    Tcl_CreateCommand (interp, "commandloop", Tcl_CommandloopCmd, 
                      (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

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
    Tcl_CreateCommand (interp, "fcntl", Tcl_FcntlCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXfilecmds.c
     */
    Tcl_CreateObjCommand (interp, "pipe", -1,
		          Tcl_PipeObjCmd, (ClientData) 0,
		          (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "copyfile", -1,
			  Tcl_CopyfileObjCmd, (ClientData) 0,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateCommand (interp, "lgets", Tcl_LgetsCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "ftruncate", -1,
			  Tcl_FtruncateObjCmd, (ClientData) 0,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "readdir", -1,
			  Tcl_ReaddirObjCmd, (ClientData) 0, 
			  (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXmsgcat.c
     */
    Tcl_InitMsgCat (interp);

    /*
     * from tclXprocess.c
     */
    Tcl_CreateCommand (interp, "execl", Tcl_ExeclCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "fork", -1,
		          Tcl_ForkObjCmd, (ClientData) 0, 
		          (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateCommand (interp, "wait", Tcl_WaitCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXsignal.c
     */
    Tcl_InitSignalHandling (interp);

    /*
     * from tclXoscmds.c
     */
    Tcl_CreateObjCommand (interp, "alarm", -1,
		          Tcl_AlarmObjCmd, (ClientData) 0, 
	                  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "nice", -1,
		          Tcl_NiceObjCmd, (ClientData) 0,
	                  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "sleep", -1,
			  Tcl_SleepObjCmd, (ClientData) 0,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "sync", -1,
			  Tcl_SyncObjCmd, (ClientData) 0,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "system", -1,
			  Tcl_SystemObjCmd, (ClientData) 0, 
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "umask", -1,
			 Tcl_UmaskObjCmd, (ClientData) 0, 
			 (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXunixCmds.c or tclXwinCmds.c
     */
    Tcl_CreateObjCommand (interp, "chroot", -1,
		          Tcl_ChrootObjCmd, (ClientData) 0, 
		          (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand (interp, "link", -1,
		          Tcl_LinkObjCmd, (ClientData) 0, 
		          (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp, "times", -1,
			  Tcl_TimesObjCmd, (ClientData) 0,
			  (Tcl_CmdDeleteProc*) NULL);
    
    /*
     * from tclXsocket.c
     */
    Tcl_CreateObjCommand (interp, "host_info", -1,
		          Tcl_HostInfoObjCmd, (ClientData) 0, 
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
    Tcl_CreateCommand (interp, "bsearch", Tcl_BsearchCmd, 
                      (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXfstat.c
     */
    Tcl_CreateCommand (interp, "fstat", Tcl_FstatCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXflock.c
     */
    Tcl_CreateCommand (interp, "flock", Tcl_FlockCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand (interp, "funlock", Tcl_FunlockCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXfilescan.c
     */
    Tcl_InitFilescan (interp);

    /*
     * from tclXgeneral.c
     */
    Tcl_CreateObjCommand(interp, "echo", -1,
			 Tcl_EchoObjCmd, (ClientData) 0, 
		         (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp, "infox", -1,
			 Tcl_InfoxObjCmd, (ClientData) 0, 
			 (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp, "loop", -1,
		         Tcl_LoopObjCmd, (ClientData) 0, 
		         (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXid.c
     */
    Tcl_CreateCommand (interp, "id", Tcl_IdCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

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
    Tcl_InitMath (interp);

    /*
     * from tclXprofile.c
     */
    TclX_ProfileInit (interp);

    /*
     * from tclXselect.c
     */
    Tcl_CreateCommand (interp, "select", Tcl_SelectCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);

    /*
     * from tclXstring.c
     */
    TclX_StringInit (interp);

    return TCL_OK;
}


