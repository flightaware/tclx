/*
 * tclExtdInt.h
 *
 * Standard internal include file for Extended Tcl library..
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
 * $Id: tclExtdInt.h,v 4.10 1995/03/30 05:26:13 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#ifndef TCLEXTDINT_H
#define TCLEXTDINT_H

#include "tclExtend.h"
#include "tclXconf.h"
#include "tclInt.h"

#include <sys/param.h>

/*
 * Use the real functions, not the Tcl interface that hides signals.
 */
#undef open
#undef read
#undef waitpid
#undef write

/*
 * If tclUnix.h has already included time.h, don't include it again, some
 * systems don't #ifdef inside of the file.
 */
#ifndef NO_SYS_TIME_H
#    include <time.h>
#endif

#include <sys/times.h>

/*
 * Make sure CLK_TCK is defined.
 */
#ifndef CLK_TCK
#    ifdef HZ
#        define CLK_TCK HZ
#    else
#        define CLK_TCK 60
#    endif
#endif

#include <math.h>

#ifdef NO_VALUES_H
#    include <limits.h>
#else
#    include <values.h>
#endif

#ifndef MAXDOUBLE
#    define MAXDOUBLE HUGE_VAL
#endif

#include <grp.h>

/*
 * Included the tcl file tclUnix.h after other system files, as it checks
 * if certain things are defined.
 */
#include "tclPort.h"

/*
 * These should be take from an include file, but it got to be such a mess
 * to get the include files right that they are here for good measure.
 */
struct tm *gmtime ();
struct tm *localtime ();

/*
 * Determine how a timezone is obtained from "struct tm".  If there is no
 * time zone in this struct (very lame) then use the timezone variable.
 * This is done in a way to make the timezone variable the method of
 * second to last resort, as some systems have it in addition to a field in
 * "struct tm".  The last resort is to use gettimeofday to determine the
 * time zone (this is all a big drag).
 */

#if defined(HAVE_TM_TZADJ)
#    define TCL_USE_TM_TZADJ
#    define TCL_GOT_TIMEZONE
#endif
#if defined(HAVE_TM_GMTOFF) && !defined (TCL_GOT_TIMEZONE)
#    define TCL_USE_TM_GMTOFF
#    define TCL_GOT_TIMEZONE
#endif
#if defined(HAVE_TIMEZONE_VAR) && !defined (TCL_GOT_TIMEZONE)
#    define TCL_USE_TIMEZONE_VAR
#    define TCL_GOT_TIMEZONE
#endif
#if defined(HAVE_GETTIMEOFDAY) && !defined (TCL_GOT_TIMEZONE)
#    define TCL_USE_GETTIMEOFDAY
#    define TCL_GOT_TIMEZONE
#endif


#ifndef MAXINT
#    ifdef INT_MAX	/* POSIX */
#        define MAXINT INT_MAX
#    else
#        define BITSPERBYTE   8
#        define BITS(type)    (BITSPERBYTE * (int)sizeof(type))
#        define HIBITI        (1 << BITS(int) - 1)
#        define MAXINT        (~HIBITI)
#    endif
#endif

#ifndef MININT
#    ifdef INT_MIN		/* POSIX */
#        define MININT INT_MIN
#    else
#        define MININT (-MAXINT)-1
#    endif
#endif

/*
 * If no MAXLONG, assume sizeof (long) == sizeof (int).
 */
#ifndef MAXLONG
#    ifdef LONG_MAX /* POSIX */
#        define MAXLONG LONG_MAX
#    else
#        define MAXLONG MAXINT  
#    endif
#endif

#ifndef TRUE
#    define TRUE   (1)
#    define FALSE  (0)
#endif

/*
 * Structure to hold a regular expression, plus a Boyer-Moore compiled
 * pattern.  Also structure to return submatch info.
 */

typedef struct {
    regexp *progPtr;
    char   *boyerMoorePtr;
    int     noCase;
    int     numSubExprs;
} TclX_regexp;

typedef struct {
    int start;
    int end;
} Tcl_SubMatchInfo [NSUBEXP];

/*
 * Flags used by TclX_RegExpCompile:
 */
#define TCLX_REXP_NO_CASE         1   /* Do matching regardless of case    */
#define TCLX_REXP_BOTH_ALGORITHMS 2   /* Use boyer-moore along with regexp */

/*
 * Flags used by TclX_Eval and friends.
 */
#define TCLX_EVAL_GLOBAL          1  /* Evaluate in the global environment.*/
#define TCLX_EVAL_FILE            2  /* Read and evaluate a file.          */
#define TCLX_EVAL_ERR_HANDLER     4  /* Call error handler on error.       */

/*
 * Used to return argument messages by most commands.
 */
extern char *tclXWrongArgs;

/*
 * Macros to do string compares.  They pre-check the first character before
 * checking of the strings are equal.
 */

#define STREQU(str1, str2) \
        (((str1) [0] == (str2) [0]) && (strcmp (str1, str2) == 0))
#define STRNEQU(str1, str2, cnt) \
        (((str1) [0] == (str2) [0]) && (strncmp (str1, str2, cnt) == 0))

/*
 * Macro to do ctype functions with 8 bit character sets.
 */
#define ISSPACE(c) (isspace ((unsigned char) c))
#define ISDIGIT(c) (isdigit ((unsigned char) c))
#define ISLOWER(c) (islower ((unsigned char) c))

/*
 * Macro that behaves like strdup, only uses ckalloc.
 */
#define ckstrdup(sourceStr) \
  (strcpy (ckalloc (strlen (sourceStr) + 1), sourceStr))


/*
 * Macros to get the stdin, stdout and stderr pointers out of the interpreter
 * when error status is not desired.
 */
#define TCL_STDIN \
    (((tclNumFiles < 1) || (tclOpenFiles [0] == NULL)) ? \
     stdin : tclOpenFiles [0]->f)

#define TCL_STDOUT \
    (((tclNumFiles < 2) || (tclOpenFiles [1] == NULL)) ? \
     stdout : \
      ((tclOpenFiles [1]->f2 != NULL) ? tclOpenFiles [1]->f2 : \
       tclOpenFiles [1]->f))

#define TCL_STDERR \
    (((tclNumFiles < 3) || (tclOpenFiles [2] == NULL)) ? \
     stderr : \
      ((tclOpenFiles [2]->f2 != NULL) ? tclOpenFiles [2]->f2 : \
       tclOpenFiles [2]->f))


/*
 * Prototypes for utility procedures.
 */
extern void
Tcl_CloseForError _ANSI_ARGS_((Tcl_Interp *interp,
                               int         fileNum));

extern int
Tcl_CommandLoop _ANSI_ARGS_((Tcl_Interp *interp,
                             int         interactive));

extern int
Tcl_StrToOffset _ANSI_ARGS_((CONST char *string,
                             int         base,
                             off_t      *offsetPtr));

extern int
Tcl_DStringGets _ANSI_ARGS_((FILE         *filePtr,
                             Tcl_DString  *dynStrPtr));

extern int
TclX_Eval _ANSI_ARGS_((Tcl_Interp  *interp,
                       unsigned     options,
                       char        *cmd));

extern int
TclX_VarEval ();

extern int
Tcl_GetDate _ANSI_ARGS_((char   *p,
                         time_t  now,
                         long    zone,
                         time_t *timePtr));

extern OpenFile *
Tcl_GetOpenFileStruct _ANSI_ARGS_((Tcl_Interp *interp,
                                   char       *handle));

extern int
Tcl_GetTime _ANSI_ARGS_((Tcl_Interp *interp,
                         CONST char *string,
                         time_t     *timePtr));

extern int
Tcl_GetOffset _ANSI_ARGS_((Tcl_Interp *interp,
                           CONST char *string,
                           off_t      *offsetPtr));

extern int
Tcl_ProcessSignal _ANSI_ARGS_((ClientData  clientData,
                               Tcl_Interp *interp,
                               int         cmdResultCode));

extern void
TclX_RegExpClean _ANSI_ARGS_((TclX_regexp *regExpPtr));

extern int
TclX_RegExpCompile _ANSI_ARGS_((Tcl_Interp   *interp,
                                TclX_regexp  *regExpPtr,
                                char         *expression,
                                int          flags));

extern int
TclX_RegExpExecute _ANSI_ARGS_((Tcl_Interp       *interp,
                                TclX_regexp      *regExpPtr,
                                char             *matchStrIn,
                                char             *matchStrLower,
                                Tcl_SubMatchInfo  subMatchInfo));


extern int
Tcl_RelativeExpr _ANSI_ARGS_((Tcl_Interp  *interp,
                              char        *cstringExpr,
                              long         stringLen,
                              long        *exprResultPtr));

extern void
Tcl_ResetSignals ();

extern FILE *
Tcl_SetupFileEntry _ANSI_ARGS_((Tcl_Interp *interp,
                                int         fileNum,
                                int         permissions));

extern clock_t
Tcl_TicksToMS _ANSI_ARGS_((clock_t numTicks));

/*
 * Definitions required to initialize all extended commands.  These are either
 * the command executors or initialization routines that do the command
 * initialization.  The initialization routines are used when there is more
 * to initializing the command that just binding the command name to the
 * executor.  Usually, this means initializing some command local data via
 * the ClientData mechanism.  The command executors should be declared to be of
 * type `Tcl_CmdProc', but this blows up some compilers, so they are declared
 * with an ANSI prototype.
 */

/*
 * from tclXbsearch.c
 */
extern int 
Tcl_BsearchCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXchmod.c
 */
extern int 
Tcl_ChmodCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_ChownCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_ChgrpCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXclock.c
 */
extern int 
Tcl_GetclockCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_FmtclockCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXcnvclock.c
 */
extern int 
Tcl_ConvertclockCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXcmdloop.c
 */
extern int 
Tcl_CommandloopCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXdebug.c
 */
extern void
Tcl_InitDebug _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXdup.c
 */
extern int 
Tcl_DupCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXfcntl.c
 */
extern int 
Tcl_FcntlCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXfilecmds.c
 */
extern int 
Tcl_PipeCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_CopyfileCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_LgetsCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_FrenameCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXfstat.c
 */
extern int 
Tcl_FstatCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXflock.c
 */
extern int
Tcl_FlockCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int
Tcl_FunlockCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int
Tcl_ReaddirCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXfilescan.c
 */
extern void
Tcl_InitFilescan _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXgeneral.c
 */

extern int 
Tcl_EchoCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_InfoxCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_LoopCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXid.c
 */
extern int 
Tcl_IdCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXkeylist.c
 */
extern int 
Tcl_KeyldelCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_KeylgetCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_KeylkeysCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_KeylsetCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXlib.c
 */
extern int 
TclX_LibraryInit _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXlist.c
 */
extern int 
Tcl_LvarpopCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_LvarcatCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_LvarpushCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_LemptyCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_LassignCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_LmatchCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXmath.c
 */
extern void
Tcl_InitMath _ANSI_ARGS_((Tcl_Interp*));

/*
 * from tclXmsgcat.c
 */
extern void
Tcl_InitMsgCat _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXprocess.c
 */
extern int 
Tcl_ExeclCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_ForkCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_WaitCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXprofile.c
 */
void
Tcl_InitProfile _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXselect.c
 */
extern int 
Tcl_SelectCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXsignal.c
 */
extern void
Tcl_InitSignalHandling _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXstring.c
 */
extern int 
Tcl_CindexCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_ClengthCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_CrangeCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_CcollateCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_ReplicateCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_TranslitCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_CtypeCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_CtokenCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_CexpandCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_CequalCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXlib.c
 */
extern void
Tcl_InitLibrary _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXunixcmds.c
 */
extern int 
Tcl_AlarmCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_ChrootCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_NiceCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_SleepCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_SyncCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_SystemCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_TimesCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_UmaskCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_LinkCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_UnlinkCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_MkdirCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
Tcl_RmdirCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXserver.c
 */
extern void
Tcl_ServerInit _ANSI_ARGS_((Tcl_Interp*));

#endif
