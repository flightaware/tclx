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
 * $Id: tclExtend.h,v 2.6 1993/06/21 06:08:05 markd Exp markd $
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

#define TCL_EXTD_VERSION_SUFFIX "a-B1"   /* 7.0a */

typedef void *void_pt;

/*
 * Flags for Tcl shell startup.
 */
#define TCLSH_QUICK_STARTUP       1   /* Don't process Tcl init file.        */
#define TCLSH_ABORT_STARTUP_ERR   2   /* Abort on an error.                  */
#define TCLSH_NO_STACK_DUMP       8   /* Don't dump the proc stack on error. */
#define TCLSH_INTERACTIVE        16   /* Set interactiveSession to 1.        */

/*
 * These globals are used by the infox command.  Should be set by main.
 */

extern char *tclxVersion;        /* Extended Tcl version number.            */
extern int   tclxPatchlevel;     /* Extended Tcl patch level.               */

extern char *tclAppName;         /* Application name                        */
extern char *tclAppLongname;     /* Long, natural language application name */
extern char *tclAppVersion;      /* Version number of the application       */

/*
 * If non-zero, a signal was received.  Normally signals are handled in
 * Tcl_Eval, but if an application does not return to eval for some period
 * of time, then this should be checked and Tcl_CheckForSignal called if
 * this is set.
 */
extern int tclReceivedSignal;

/*
 * Exported Extended Tcl functions.
 */

EXTERN int
Tcl_CheckForSignal _ANSI_ARGS_((Tcl_Interp *interp,
                                int         cmdResultCode));

EXTERN Tcl_Interp * 
Tcl_CreateExtendedInterp ();

EXTERN char *
Tcl_DeleteKeyedListField _ANSI_ARGS_((Tcl_Interp  *interp,
                                      CONST char  *fieldName,
                                      CONST char  *keyedList));
EXTERN char * 
Tcl_DownShift _ANSI_ARGS_((char       *targetStr,
                           CONST char *sourceStr));
EXTERN char * 
Tcl_UpShift _ANSI_ARGS_((char       *targetStr,
                         CONST char *sourceStr));

EXTERN int
Tcl_GetKeyedListField _ANSI_ARGS_((Tcl_Interp  *interp,
                                   CONST char  *fieldName,
                                   CONST char  *keyedList,
                                   char       **fieldValuePtr));

int
Tcl_GetKeyedListKeys _ANSI_ARGS_((Tcl_Interp  *interp,
                                  CONST char  *subFieldName,
                                  CONST char  *keyedList,
                                  int         *keyesArgcPtr,
                                  char      ***keyesArgvPtr));

EXTERN int 
Tcl_GetLong _ANSI_ARGS_((Tcl_Interp  *interp,
                         CONST char *string,
                         long        *longPtr));

EXTERN int 
Tcl_GetUnsigned _ANSI_ARGS_((Tcl_Interp  *interp,
                             CONST char *string,
                             unsigned   *unsignedPtr));

int
Tcl_ProcessInitFile _ANSI_ARGS_((Tcl_Interp *interp,
                                 char       *dirEnvVar,
                                 char       *dir1,
                                 char       *dir2,
                                 char       *dir3,
                                 char       *initFile));

EXTERN char *
Tcl_SetKeyedListField _ANSI_ARGS_((Tcl_Interp  *interp,
                                   CONST char  *fieldName,
                                   CONST char  *fieldvalue,
                                   CONST char  *keyedList));

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

EXTERN void 
Tcl_Startup _ANSI_ARGS_((Tcl_Interp     *interp,
                         unsigned        options,
                         int             argc,
                         CONST char    **argv));

EXTERN int
Tcl_ShellEnvInit _ANSI_ARGS_((Tcl_Interp  *interp,
                              unsigned     options));
#endif
