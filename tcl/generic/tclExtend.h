/* 
 * tclExtend.h
 *
 *    External declarations for the extended Tcl library.
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
 * $Id: tclExtend.h,v 7.7 1996/10/27 00:34:09 markd Exp $
 *-----------------------------------------------------------------------------
 */

#ifndef TCLEXTEND_H
#define TCLEXTEND_H

#include <stdio.h>
#include "tcl.h"

/*
 * The versions for TclX and TkX.  This is based on the versions of Tcl and Tk
 * that TclX was released against.  Its possible that TclX maybe running with
 * a different version of Tcl or Tk.  The basic versions are used for package
 * provide, the full versions as used for file names and include beta release
 * information and patch information.  The shared library versions are the
 * version suffixes for libraries (if supported).  It is made by using the Tcl
 * or Tk version as the major version and the TclX suffix, converted to a
 * number, as a minor.
 *
 * Examples:
 *   Release        _VERSION  _FULL_VERSION  _SHLIB_VERSION
 *   7.5.0           7.5.0     7.5.0         75.0
 *   7.5.1 beta 1    7.5.1     7.5.1b1       75.1
 *   7.5.1 patch 1   7.5.1.1   7.5.1p1       75.1
 */

#define TCLX_PATCHLEVEL      0

#define TCLX_VERSION        "7.6.0"
#define TCLX_FULL_VERSION   "7.6.0"
#define TCLX_SHLIB_VERSION  "76.0"

#define TKX_VERSION         "4.2.0"
#define TKX_FULL_VERSION    "4.2.0"
#define TKX_SHLIB_VERSION   "42.0"

/*
 * Generic void pointer.
 */
typedef void *void_pt;

/*
 * Flags to command loop functions.
 */
#define TCLX_CMDL_INTERACTIVE  (1<<0)
#define TCLX_CMDL_EXIT_ON_EOF  (1<<1)

/*
 * If this variable is non-zero, the TclX shell will delete the interpreter
 * at the end of a script instead of exiting immediately.  This is for
 * applications that want to track down memory leaks.  This does not alter the
 * behavior of explicit calls to exit.
 */
extern int tclDeleteInterpAtEnd;

/*
 * Application signal error handler.  Called after normal signal processing,
 * when a signal results in an error.   Its main purpose in life is to allow
 * interactive command loops to clear their input buffer on SIGINT.  This is
 * not currently a generic interface, but should be. Only one maybe active.
 * This is an undocumented interface.  Its in the external file in case
 * someone needs this facility.  It might change in the future.  Let us
 * know if you need this functionallity.
 */
typedef int
(*TclX_AppSignalErrorHandler) _ANSI_ARGS_((Tcl_Interp *interp,
                                           ClientData  clientData,
                                           int         background,
                                           int         signalNum));
/*
 * Exported TclX initialization functions.
 */
EXTERN void
TclX_Main _ANSI_ARGS_((int              argc,
                       char           **argv,
                       Tcl_AppInitProc *appInitProc));

EXTERN int
Tclx_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN int
Tclx_SafeInit _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN int
Tclxcmd_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN int
Tclxcmd_SafeInit _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN int
Tclxlib_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void
TclX_ErrorExit _ANSI_ARGS_((Tcl_Interp  *interp,
                            int          exitCode));

EXTERN void
TclX_EvalRCFile _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void
TclX_PrintResult _ANSI_ARGS_((Tcl_Interp *interp,
                              int         intResult,
                              char       *checkCmd));

EXTERN void
TclX_SetupSigInt _ANSI_ARGS_((void));

EXTERN void
TclX_SetAppSignalErrorHandler _ANSI_ARGS_((
    TclX_AppSignalErrorHandler errorFunc,
    ClientData                 clientData));

EXTERN void
TclX_SetAppInfo _ANSI_ARGS_((int   defaultValues,
                             char *appName,
                             char *appLongName,
                             char *appVersion,
                             int   appPatchlevel));

EXTERN void
TclX_SplitWinCmdLine _ANSI_ARGS_((int    *argcPtr,
                                  char ***argvPtr));

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
                                  int         *keysArgcPtr,
                                  char      ***keysArgvPtr));

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
 * Command loop functions.
 */
EXTERN int
TclX_CommandLoop _ANSI_ARGS_((Tcl_Interp *interp,
                              int         options,
                              char       *endCommand,
                              char       *prompt1,
                              char       *prompt2));

EXTERN int
TclX_AsyncCommandLoop _ANSI_ARGS_((Tcl_Interp *interp,
                                   int         options,
                                   char       *endCommand,
                                   char       *prompt1,
                                   char       *prompt2));

/*
 * Tk with TclX initialization.
 */

EXTERN int
Tkx_Init _ANSI_ARGS_((Tcl_Interp  *interp));

EXTERN void
TkX_Main _ANSI_ARGS_((int               argc,
                      char            **argv,
                      Tcl_AppInitProc  *appInitProc));

/*
 * These are for Windows only.
 */
EXTERN int
TkX_ConsoleInit _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void
TkX_Panic _ANSI_ARGS_(TCL_VARARGS_DEF(char *,fmt));

#endif
