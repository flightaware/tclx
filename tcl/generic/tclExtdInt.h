/*
 * tclExtdInt.h
 *
 * Standard internal include file for Extended Tcl library..
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
 * $Id: tclExtdInt.h,v 5.11 1996/03/11 06:15:53 markd Exp $
 *-----------------------------------------------------------------------------
 */

#ifndef TCLEXTDINT_H
#define TCLEXTDINT_H

#include "tclExtend.h"
#include "tclInt.h"

#include <sys/param.h>

#include <math.h>

#ifdef NO_LIMITS_H
#    include <values.h>
#else
#    include <limits.h>
#endif

#ifndef MAXDOUBLE
#    define MAXDOUBLE HUGE_VAL
#endif

#include <grp.h>

/*
 * Included the tcl file tclUnixPort.h after other system files, as it checks
 * if certain things are defined.
 */
#include "tclUnixPort.h"

/*
 * Define O_ACCMODE if <fcntl.h> does not define it.
 */
#ifndef O_ACCMODE
#    define O_ACCMODE  (O_RDONLY|O_WRONLY|O_RDWR)
#endif

/*
 * Make sure we have both O_NONBLOCK and O_NDELAY defined.
 */
#ifndef O_NONBLOCK
#   define O_NONBLOCK O_NDELAY
#endif
#ifndef O_NDELAY
#   define O_NDELAY O_NONBLOCK
#endif


/*
 * If tclPort.h has already included time.h, don't include it again, some
 * systems don't #ifdef inside of the file.  Also, on some of these systems,
 * time.h is included by sys/time.h, so don't include it again if that is the
 * case.
 */
#if !defined (NO_SYS_TIME_H) && defined(TIME_WITH_SYS_TIME)
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

/*
 * These should be take from an include file, but it got to be such a mess
 * to get the include files right that they are here for good measure.
 */
struct tm *gmtime ();
struct tm *localtime ();


/*
 * Get ranges of integers and longs.
 * If no MAXLONG, assume sizeof (long) == sizeof (int).
 */

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

#ifndef MAXLONG
#    ifdef LONG_MAX /* POSIX */
#        define MAXLONG LONG_MAX
#    else
#        define MAXLONG MAXINT  
#    endif
#endif

/*
 * Boolean constants.
 */
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
 * Defines used by TclX_Get/SetChannelOption.  Defines name TCLX_COPT_ are the
 * options and the others are the value
 */
#define TCLX_COPT_BLOCKING      1
#define TCLX_MODE_BLOCKING      0
#define TCLX_MODE_NONBLOCKING   1

#define TCLX_COPT_BUFFERING     2
#define TCLX_BUFFERING_FULL     0
#define TCLX_BUFFERING_LINE     1
#define TCLX_BUFFERING_NONE     2

#define TCLX_COPT_TRANSLATION   3
#define TCLX_TRANSLATE_AUTO     0
#define TCLX_TRANSLATE_LF       1
#define TCLX_TRANSLATE_BINARY   1  /* same as LF */
#define TCLX_TRANSLATE_CR       2
#define TCLX_TRANSLATE_CRLF     3
#define TCLX_TRANSLATE_PLATFORM 4



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
 * Prototypes for utility procedures.
 */
extern void
Tcl_CloseForError _ANSI_ARGS_((Tcl_Interp *interp,
                               Tcl_Channel channel,
                               int         fileNum));

extern int
Tcl_CommandLoop _ANSI_ARGS_((Tcl_Interp *interp,
                             int         interactive));

extern int
Tcl_StrToOffset _ANSI_ARGS_((CONST char *string,
                             int         base,
                             off_t      *offsetPtr));

extern int
TclX_Eval _ANSI_ARGS_((Tcl_Interp  *interp,
                       unsigned     options,
                       char        *cmd));

extern int
TclX_VarEval _ANSI_ARGS_(TCL_VARARGS(Tcl_Interp *, arg1));

extern int
TclX_WriteStr _ANSI_ARGS_((Tcl_Channel  channel,
                           char        *str));

#define TclX_WriteNL(channel) (Tcl_Write (channel, "\n", 1))


extern int
TclX_ChannelFnum _ANSI_ARGS_((Tcl_Channel channel,
                              int         direction));

extern int
TclX_GetChannelOption _ANSI_ARGS_((Tcl_Channel channel,
                                   int         option));

extern Tcl_Channel
TclX_GetOpenChannel _ANSI_ARGS_((Tcl_Interp *interp,
                                 char       *handle,
                                 int         accessMode));

extern int
TclX_GetOpenFnum _ANSI_ARGS_ ((Tcl_Interp *interp,
                               char       *handle,
                               int         accessMode));

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
Tcl_ResetSignals _ANSI_ARGS_((void));

extern int
TclX_RuntimeInit _ANSI_ARGS_((Tcl_Interp *interp,
                              char       *tclLibVarName,
                              char       *tclEnvVarName,
                              char       *defaultEnvVar,
                              char       *defaultDir,
                              char       *tclInitVarName,
                              char       *defaultInitFile));

extern int
TclX_SetChannelOption _ANSI_ARGS_((Tcl_Interp  *interp,
                                   Tcl_Channel  channel,
                                   int          option,
                                   int          value));

extern Tcl_Channel
TclX_SetupFileEntry _ANSI_ARGS_((Tcl_Interp *interp,
                                 int         fileNum,
                                 int         mode,
                                 int         isSocket));

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

extern int
Tcl_FtruncateCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int
Tcl_ReaddirCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

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
Tcl_ServerInit _ANSI_ARGS_((Tcl_Interp *interp));

extern int
TclXGetHostInfo _ANSI_ARGS_((Tcl_Interp *interp,
                             int         fileNum,
                             int         remoteHost));

extern int
TclXGetKeepAlive _ANSI_ARGS_((Tcl_Interp  *interp,
                              Tcl_Channel  channel,
                              int         *valuePtr));

extern int
TclXSetKeepAlive _ANSI_ARGS_((Tcl_Interp  *interp,
                              Tcl_Channel  channel,
                              int          value));

#endif
