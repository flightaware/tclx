/* 
 * tclExtend.h
 *
 *    External declarations for the extended Tcl library.
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
 * $Id: tclExtend.h,v 4.13 1995/07/05 01:17:36 markd Exp markd $
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

#define TCL_EXTD_VERSION_SUFFIX "a-b6"   /* 7.4a-b6 */

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
 * Used to override the init file name, which is either library relative or
 * absolute.  The value fields must be changed before TclX_Init or
 * TclXLib_Init is called.  To change the library location,
 */
extern char *tclX_library;     /* Library directory to use.        */
extern char *tclX_libraryEnv;  /* Override environment variable.   */
extern char *tclX_initFile;    /* Init file name or absolute path. */

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
 * If this variable is non-zero, the TclX shell will delete the interpreter
 * at the end of a script instead of evaluating the "exit" command.  This is
 * for applications that want to track down memory leaks.  This does not
 * alter the behavior of explicit calls to exit.
 */
extern int tclDeleteInterpAtEnd;


/*
 * Exported TclX initialization functions.
 */
EXTERN void
TclX_Main _ANSI_ARGS_((int              argc,
                       char           **argv,
                       Tcl_AppInitProc *appInitProc));

EXTERN int
TclX_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN int
TclXCmd_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN int
TclXLib_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void
TclX_ErrorExit _ANSI_ARGS_((Tcl_Interp  *interp,
                            int          exitCode));

EXTERN void
TclX_EvalRCFile _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void
TclX_OutputPrompt _ANSI_ARGS_((Tcl_Interp *interp,
                               int         topLevel));

EXTERN void
TclX_PrintResult _ANSI_ARGS_((Tcl_Interp *interp,
                              int         intResult,
                              char       *checkCmd));

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
Tcl_GetTimeZone _ANSI_ARGS_((long  currentTime));

EXTERN int 
Tcl_GetUnsigned _ANSI_ARGS_((Tcl_Interp  *interp,
                             CONST char *string,
                             unsigned   *unsignedPtr));

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

/*
 * Tk with TclX initialization.
 *
 * The variables are Used to override the library and library environment
 * variable used to find the TkX startup file and runtime library.  The values
 * of these fields must be changed before TkX_Init is called.
 */
extern char *tkX_library;
extern char *tkX_libraryEnv;

EXTERN int
TkX_Init _ANSI_ARGS_((Tcl_Interp  *interp));

EXTERN void
TkX_Main _ANSI_ARGS_((int               argc,
                      char            **argv,
                      Tcl_AppInitProc  *appInitProc));

#endif
