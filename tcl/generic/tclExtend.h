/* 
 * tclExtend.h
 *
 *    External declarations for the extended Tcl library.
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
 * $Id: tclExtend.h,v 2.20 1993/11/05 05:33:10 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#ifndef TCLEXTEND_H
#define TCLEXTEND_H

#include <stdio.h>
#include "tcl.h"

/*
 * Version suffix for extended Tcl, this is appended to the standard Tcl
 * version to form the actual extended Tcl version.
 */

#define TCL_EXTD_VERSION_SUFFIX "a-B5"   /* 7.1a */

typedef void *void_pt;

/*
 * These globals are used by the infox command.  Should be set before
 * initializing the TclX shell.
 */

extern char *tclxVersion;        /* Extended Tcl version number.            */
extern int   tclxPatchlevel;     /* Extended Tcl patch level.               */

extern char *tclAppName;         /* Application name                        */
extern char *tclAppLongname;     /* Long, natural language application name */
extern char *tclAppVersion;      /* Version number of the application       */
extern int   tclAppPatchlevel;   /* Patchlevel of the application           */

/*
 * Flag user to indicate that a signal that was setup to return an error
 * occured (it may not have been processed yet).  This is used by interactive
 * command loops to flush input.  It should be explictly cleared by any routine
 * that cares about it.  Also an application-supplied function to call if a
 * error signal occurs.  This normally flushes command input.
 */
extern int tclGotErrorSignal;
extern void (*tclErrorSignalProc) _ANSI_ARGS_((int signalNum));

/*
 * Pointer to background error handler for signals handled while not in an
 * interpreter.
 */
extern void (*tclSignalBackgroundError) _ANSI_ARGS_((Tcl_Interp *interp));


/*
 * Exported Tcl initialization functions.
 */
EXTERN int
TclX_AppInit _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN int
TclX_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN int
TclXCmd_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN int
TclXLib_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void
TclX_ErrorExit _ANSI_ARGS_((Tcl_Interp  *interp,
                            int          exitCode));

extern void
TclX_OutputPrompt _ANSI_ARGS_((Tcl_Interp *interp,
                               int         topLevel));

EXTERN void 
TclX_ParseCmdLine _ANSI_ARGS_((Tcl_Interp   *interp,
                               int           argc,
                               char        **argv));

extern void
TclX_PrintResult _ANSI_ARGS_((Tcl_Interp *interp,
                              int         intResult,
                              char       *checkCmd));

EXTERN int
TclX_RunShell _ANSI_ARGS_((Tcl_Interp  *interp));

EXTERN void
Tcl_SetupSigInt _ANSI_ARGS_((void));

/*
 * Exported utility functions.
 */
EXTERN char * 
Tcl_DownShift _ANSI_ARGS_((char       *targetStr,
                           CONST char *sourceStr));

EXTERN int 
Tcl_GetLong _ANSI_ARGS_((Tcl_Interp  *interp,
                         CONST char *string,
                         long        *longPtr));

EXTERN int 
Tcl_GetUnsigned _ANSI_ARGS_((Tcl_Interp  *interp,
                             CONST char *string,
                             unsigned   *unsignedPtr));

EXTERN int
Tcl_ProcessSignals _ANSI_ARGS_((ClientData  clientData,
                                Tcl_Interp *interp,
                                int         cmdResultCode));

EXTERN int
Tcl_StrToLong _ANSI_ARGS_((CONST char *string,
                           int          base,
                           long        *longPtr));

EXTERN int
Tcl_StrToInt _ANSI_ARGS_((CONST char *string,
                          int         base,
                          int        *intPtr));

EXTERN int
Tcl_StrToUnsigned _ANSI_ARGS_((CONST char *string,
                               int         base,
                               unsigned   *unsignedPtr));

EXTERN int
Tcl_StrToDouble _ANSI_ARGS_((CONST char  *string,
                             double      *doublePtr));

EXTERN char * 
Tcl_UpShift _ANSI_ARGS_((char       *targetStr,
                         CONST char *sourceStr));

/*
 * Exported keyed list manipulation functions.
 */
EXTERN char *
Tcl_DeleteKeyedListField _ANSI_ARGS_((Tcl_Interp  *interp,
                                      CONST char  *fieldName,
                                      CONST char  *keyedList));
EXTERN int
Tcl_GetKeyedListField _ANSI_ARGS_((Tcl_Interp  *interp,
                                   CONST char  *fieldName,
                                   CONST char  *keyedList,
                                   char       **fieldValuePtr));

EXTERN int
Tcl_GetKeyedListKeys _ANSI_ARGS_((Tcl_Interp  *interp,
                                  CONST char  *subFieldName,
                                  CONST char  *keyedList,
                                  int         *keyesArgcPtr,
                                  char      ***keyesArgvPtr));

EXTERN char *
Tcl_SetKeyedListField _ANSI_ARGS_((Tcl_Interp  *interp,
                                   CONST char  *fieldName,
                                   CONST char  *fieldvalue,
                                   CONST char  *keyedList));

/*
 * Exported handle table manipulation functions.
 */
EXTERN void_pt  
Tcl_HandleAlloc _ANSI_ARGS_((void_pt   headerPtr,
                             char     *handlePtr));

EXTERN void 
Tcl_HandleFree _ANSI_ARGS_((void_pt  headerPtr,
                            void_pt  entryPtr));

EXTERN void_pt
Tcl_HandleTblInit _ANSI_ARGS_((CONST char *handleBase,
                               int         entrySize,
                               int         initEntries));

EXTERN void
Tcl_HandleTblRelease _ANSI_ARGS_((void_pt headerPtr));

EXTERN int
Tcl_HandleTblUseCount _ANSI_ARGS_((void_pt headerPtr,
                                   int     amount));

EXTERN void_pt
Tcl_HandleWalk _ANSI_ARGS_((void_pt   headerPtr,
                            int      *walkKeyPtr));

EXTERN void
Tcl_WalkKeyToHandle _ANSI_ARGS_((void_pt   headerPtr,
                                 int       walkKey,
                                 char     *handlePtr));

EXTERN void_pt
Tcl_HandleXlate _ANSI_ARGS_((Tcl_Interp  *interp,
                             void_pt      headerPtr,
                             CONST  char *handle));


#endif
