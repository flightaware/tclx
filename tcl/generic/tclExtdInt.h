/*
 * tclExtdInt.h
 *
 * Standard internal include file for Extended Tcl.
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
 * $Id: tclExtdInt.h,v 8.1 1997/04/17 04:58:31 markd Exp $
 *-----------------------------------------------------------------------------
 */

#ifndef TCLEXTDINT_H
#define TCLEXTDINT_H

#include "tclExtend.h"

#if defined(__WIN32__) || defined(_WIN32)
#   include "tclXwinPort.h"
#else
#   include "tclXunixPort.h"
#endif

/*
 * After above, since some of our port definitions affect what gets defined
 * tclInt.h and the Tcl port file.
 */
#include "tclInt.h"

/*
 * Assert macro for use in TclX.  It is controlled by our own #define, not
 * NDEBUG, but other than that, it is a wrapper around assert.
 */
#ifdef TCLX_DEBUG
#   define TCLX_ASSERT(expr) (assert (expr))
#else
#   define TCLX_ASSERT(expr)
#endif

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
 * Flags used by TclX_RegExpCompileObj:
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

/*
 * Two values are always returned for translation, one for the read side and
 * one for the write.  They are returned masked into one word.
 */

#define TCLX_COPT_TRANSLATION      3
#define TCLX_TRANSLATE_READ_SHIFT  8
#define TCLX_TRANSLATE_READ_MASK   0xFF00
#define TCLX_TRANSLATE_WRITE_MASK  0x00FF

#define TCLX_TRANSLATE_UNSPECIFIED 0 /* For only one direction specified */
#define TCLX_TRANSLATE_AUTO        1
#define TCLX_TRANSLATE_LF          2
#define TCLX_TRANSLATE_BINARY      2  /* same as LF */
#define TCLX_TRANSLATE_CR          3
#define TCLX_TRANSLATE_CRLF        4
#define TCLX_TRANSLATE_PLATFORM    5

/*
 * Flags used by chown/chgrp.
 */
#define TCLX_CHOWN  0x1
#define TCLX_CHGRP  0x2

/*
 * Structure use to pass file locking information.  Parallels the Posix
 * struct flock, but use to pass info from the generic code to the system
 * dependent code.
 */
typedef struct {
    Tcl_Channel channel;      /* Channel to lock */
    int         access;       /* TCL_READABLE and/or TCL_WRITABLE */
    int         block;        /* Block if lock is not available */
    off_t       start;        /* Starting offset */
    off_t       len;          /* len = 0 means until end of file */
    pid_t       pid;          /* Lock owner */
    short       whence;       /* Type of start */
    int         gotLock;      /* Succeeded? */
} TclX_FlockInfo;

/*
 * Used to return argument messages by most commands.
 */
extern char *tclXWrongArgs;
extern Tcl_Obj *tclXWrongArgsObj;

/*
 * Macros to do string compares.  They pre-check the first character before
 * checking of the strings are equal.
 */

#define STREQU(str1, str2) \
        (((str1) [0] == (str2) [0]) && (strcmp (str1, str2) == 0))
#define STRNEQU(str1, str2, cnt) \
        (((str1) [0] == (str2) [0]) && (strncmp (str1, str2, cnt) == 0))

#define OBJSTREQU(obj1, str1) \
	(strcmp (Tcl_GetStringFromObj (obj1, NULL), str1) == 0)

#define OBJSTRNEQU(obj1, str1, cnt) \
	(strncmp (Tcl_GetStringFromObj (obj1, NULL), str1, cnt) == 0)
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
 * Callback type for walking directories.
 */
typedef int
(TclX_WalkDirProc) _ANSI_ARGS_((Tcl_Interp  *interp,
                                char        *path,
                                char        *fileName,
                                int          caseSensitive,
                                ClientData   clientData));

/*
 * Prototypes for utility procedures.
 */
extern int
TclX_StrToOffset _ANSI_ARGS_((CONST char *string,
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
TclX_GetChannelOption _ANSI_ARGS_((Tcl_Channel channel,
                                   int         option));

extern Tcl_Channel
TclX_GetOpenChannel _ANSI_ARGS_((Tcl_Interp *interp,
                                 char       *handle,
                                 int         direction));

extern Tcl_Channel
TclX_GetOpenChannelObj _ANSI_ARGS_((Tcl_Interp *interp,
                                    Tcl_Obj    *handle,
                                    int         direction));

extern int
TclX_GetOffset _ANSI_ARGS_((Tcl_Interp *interp,
                            CONST char *string,
                            off_t      *offsetPtr));

extern void
TclX_RegExpClean _ANSI_ARGS_((TclX_regexp *regExpPtr));

extern int
TclX_RegExpCompileObj _ANSI_ARGS_((Tcl_Interp   *interp,
                                   TclX_regexp  *regExpPtr,
                                   Tcl_Obj      *expression,
                                   int          flags));

extern int
TclX_RegExpExecute _ANSI_ARGS_((Tcl_Interp       *interp,
                                TclX_regexp      *regExpPtr,
                                char             *matchStrIn,
                                char             *matchStrLower,
                                Tcl_SubMatchInfo  subMatchInfo));


extern int
TclX_RelativeExpr _ANSI_ARGS_((Tcl_Interp  *interp,
                               Tcl_Obj     *exprPtr,
                               int          stringLen,
                               int         *exprResultPtr));

extern int
TclXRuntimeInit _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *which,
                             char       *defaultLib,
                             char       *version));

extern int
TclX_SetChannelOption _ANSI_ARGS_((Tcl_Interp  *interp,
                                   Tcl_Channel  channel,
                                   int          option,
                                   int          value));

extern char *
TclX_JoinPath _ANSI_ARGS_((char        *path1,
                           char        *path2,
                           Tcl_DString *joinedPath));

extern int  
TclX_WrongArgs _ANSI_ARGS_((Tcl_Interp *interp, 
                            Tcl_Obj    *commandNameObj, 
			    char       *string));

extern void
TclX_StringAppendObjResult _ANSI_ARGS_(TCL_VARARGS_DEF (Tcl_Interp *,arg1));

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
TclX_BsearchCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXchmod.c
 */
extern int 
TclX_ChmodObjCmd _ANSI_ARGS_((ClientData clientData, 
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_ChownObjCmd _ANSI_ARGS_((ClientData clientData, 
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_ChgrpObjCmd _ANSI_ARGS_((ClientData clientData, 
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
/*
 * from tclXcmdloop.c
 */
extern int 
TclX_CommandloopCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXdebug.c
 */
extern void
TclX_DebugInit _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXdup.c
 */
extern void
TclX_DupInit _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXfcntl.c
 */
extern int 
TclX_FcntlObjCmd _ANSI_ARGS_((ClientData clientData, 
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

/*
 * from tclXfilecmds.c
 */
extern int 
TclX_PipeObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_CopyfileObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_LgetsObjCmd _ANSI_ARGS_((ClientData clientData, 
                             Tcl_Interp   *interp, 
                             int           objc,
                             Tcl_Obj      *CONST objv[]));

extern int
TclX_FtruncateObjCmd _ANSI_ARGS_((ClientData clientData, 
                                 Tcl_Interp   *interp, 
				 int           objc,
				 Tcl_Obj      *CONST objv[]));

extern int
TclX_ReaddirObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

/*
 * from tclXfstat.c
 */
extern int 
TclX_FstatCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXflock.c
 */
extern int
TclX_FlockCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int
TclX_FunlockCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXfilescan.c
 */
extern void
TclX_InitFilescan _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXgeneral.c
 */

extern int 
TclX_EchoObjCmd _ANSI_ARGS_((ClientData clientData, 
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_InfoxObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_LoopObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

/*
 * from tclXid.c
 */
extern int 
TclX_IdObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

/*
 * from tclXkeylist.c
 */
extern void
TclX_KeyedListInit _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXlib.c
 */
extern int 
TclX_LibraryInit _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXlist.c
 */
extern void
TclX_ListInit _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXmath.c
 */
extern void
TclX_InitMath _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXmsgcat.c
 */
extern void
TclX_InitMsgCat _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXprocess.c
 */
extern int 
TclX_ExeclCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

extern int 
TclX_ForkObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_WaitCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXprofile.c
 */
void
TclX_ProfileInit _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXselect.c
 */
extern int 
TclX_SelectCmd _ANSI_ARGS_((ClientData, Tcl_Interp*, int, char**));

/*
 * from tclXsignal.c
 */
extern void
TclX_InitSignalHandling _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXstring.c
 */
extern void
TclX_StringInit _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXlib.c
 */
extern void
TclX_InitLibrary _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * from tclXoscmds.c
 */
extern int 
TclX_AlarmObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_LinkObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_NiceObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_SleepObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_SyncObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_SystemObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

extern int 
TclX_UmaskObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

/*
 * from tclXunixCmds.c or tclXwinCmds.c
 */
extern int 
TclX_ChrootObjCmd _ANSI_ARGS_((ClientData clientData,
                              Tcl_Interp *interp, 
			      int         objc,
			      Tcl_Obj     *CONST objv[]));

extern int 
TclX_TimesObjCmd _ANSI_ARGS_((ClientData   clientData,
                             Tcl_Interp  *interp,
			     int          objc,
			     Tcl_Obj      *CONST objv[]));


/*
 * from tclXsocket.c
 */
extern int
TclXGetHostInfo _ANSI_ARGS_((Tcl_Interp *interp,
                             Tcl_Channel channel,
                             int         remoteHost));

extern int
TclX_HostInfoObjCmd _ANSI_ARGS_((ClientData  clientData,
                                Tcl_Interp *interp,
                                int         objc,
                                Tcl_Obj   *CONST objv[]));

/*
 * from tclXunixSock.c and stubbed in tclXwinCmds.c.
 */
extern void
TclX_ServerInit _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * From TclXxxxDup.c
 */
Tcl_Channel
TclXOSDupChannel _ANSI_ARGS_((Tcl_Interp *interp,
                              Tcl_Channel srcChannel,
                              int         mode,
                              char       *targetChannelId));

Tcl_Channel
TclXOSBindOpenFile _ANSI_ARGS_((Tcl_Interp *interp,
                                int         fileNum));

/*
 * from tclXxxxOS.c
 */
extern int
TclXNotAvailableError _ANSI_ARGS_((Tcl_Interp *interp,
                                   char       *funcName));

extern clock_t
TclXOSTicksToMS _ANSI_ARGS_((clock_t numTicks));

extern int
TclXOSgetpriority _ANSI_ARGS_((Tcl_Interp *interp,
                               int        *priority,
                               char       *funcName));

extern int
TclXOSincrpriority _ANSI_ARGS_((Tcl_Interp *interp,
                                int         priorityIncr,
                                int        *priority,
                                char       *funcName));

extern int
TclXOSpipe _ANSI_ARGS_((Tcl_Interp *interp,
                        Tcl_Channel *channels));

extern int
TclXOSsetitimer _ANSI_ARGS_((Tcl_Interp *interp,
                             double     *seconds,
                             char       *funcName));

extern void
TclXOSsleep _ANSI_ARGS_((unsigned seconds));

extern void
TclXOSsync _ANSI_ARGS_((void));

extern int
TclXOSfsync _ANSI_ARGS_((Tcl_Interp *interp,
                         Tcl_Channel channel));

extern int
TclXOSsystem _ANSI_ARGS_((Tcl_Interp *interp,
                          char       *command,
                          int        *exitCode));

extern int
TclX_OSlink _ANSI_ARGS_((Tcl_Interp *interp,
                         char       *srcPath,
                         char       *destPath,
                         char       *funcName));
extern int
TclX_OSsymlink _ANSI_ARGS_((Tcl_Interp *interp,
                            char       *srcPath,
                            char       *destPath,
                            char       *funcName));

extern void
TclXOSElapsedTime _ANSI_ARGS_((clock_t *realTime,
                               clock_t *cpuTime));

extern int
TclXOSkill _ANSI_ARGS_((Tcl_Interp *interp,
                        pid_t       pid,
                        int         signal,
                        char       *funcName));

extern int
TclXOSFstat _ANSI_ARGS_((Tcl_Interp  *interp,
                         Tcl_Channel  channel,
                         int          direction,
                         struct stat *statBuf,
                         int         *ttyDev));
    
extern int
TclXOSSeekable _ANSI_ARGS_((Tcl_Interp  *interp,
                            Tcl_Channel  channel,
                            int         *seekablePtr));

extern int
TclXOSWalkDir _ANSI_ARGS_((Tcl_Interp       *interp,
                           char             *path,
                           int               hidden,
                           TclX_WalkDirProc *callback,
                           ClientData        clientData));

extern int
TclXOSGetFileSize _ANSI_ARGS_((Tcl_Channel  channel,
                               int          direction,
                               off_t       *fileSize));

extern int
TclXOSftruncate _ANSI_ARGS_((Tcl_Interp  *interp,
                             Tcl_Channel  channel,
                             off_t        newSize,
                             char        *funcName));

extern int
TclXOSfork _ANSI_ARGS_((Tcl_Interp *interp,
                        Tcl_Obj    *funcNameObj));

extern int
TclXOSexecl _ANSI_ARGS_((Tcl_Interp *interp,
                         char       *path,
                         char      **argList));

extern int
TclXOSInetAtoN _ANSI_ARGS_((Tcl_Interp     *interp,
                            char           *strAddress,
                            struct in_addr *inAddress));

extern int
TclXOSgetpeername _ANSI_ARGS_((Tcl_Interp *interp,
                               Tcl_Channel channel,
                               void       *sockaddr,
                               int         sockaddrSize));

extern int
TclXOSgetsockname _ANSI_ARGS_((Tcl_Interp *interp,
                               Tcl_Channel channel,
                               void       *sockaddr,
                               int         sockaddrSize));

extern int
TclXOSgetsockopt _ANSI_ARGS_((Tcl_Interp  *interp,
                              Tcl_Channel  channel,
                              int          option,
                              int         *valuePtr));

extern int
TclXOSsetsockopt _ANSI_ARGS_((Tcl_Interp  *interp,
                              Tcl_Channel  channel,
                              int          option,
                              int          value));

extern int
TclXOSchmod _ANSI_ARGS_((Tcl_Interp *interp,
                         char       *fileName,
                         int         mode));

extern int
TclXOSfchmod _ANSI_ARGS_((Tcl_Interp *interp,
                          Tcl_Channel channel,
                          int         mode,
                          char       *funcName));

extern int
TclXOSChangeOwnGrp _ANSI_ARGS_((Tcl_Interp  *interp,
                                unsigned     options,
                                char        *ownerStr,
                                char        *groupStr,
                                char       **files,
                                char       *funcName));
extern int  
TclXOSChangeOwnGrpObj _ANSI_ARGS_((Tcl_Interp  *interp,
                                   unsigned     options,
                                   char        *ownerStr,
                                   char        *groupStr,
                                   Tcl_Obj     *fileList,
                                   char        *funcName));

extern int
TclXOSFChangeOwnGrp _ANSI_ARGS_((Tcl_Interp *interp,
                                 unsigned    options,
                                 char       *ownerStr,
                                 char       *groupStr,
                                 char      **channelIds,
                                 char       *funcName));

extern int
TclXOSFChangeOwnGrpObj _ANSI_ARGS_((Tcl_Interp *interp,
                                    unsigned    options,
                                    char       *ownerStr,
                                    char       *groupStr,
                                    Tcl_Obj    *channelIdList,
                                    char       *funcName));

int
TclXOSGetSelectFnum _ANSI_ARGS_((Tcl_Interp *interp,
                                 Tcl_Channel channel,
                                 int        *readFnumPtr,
                                 int        *writeFnumPtr));
int
TclXOSHaveFlock _ANSI_ARGS_((void));

int
TclXOSFlock _ANSI_ARGS_((Tcl_Interp     *interp,
                         TclX_FlockInfo *lockInfoPtr));

int
TclXOSFunlock _ANSI_ARGS_((Tcl_Interp     *interp,
                           TclX_FlockInfo *lockInfoPtr));

int
TclXOSGetAppend _ANSI_ARGS_((Tcl_Interp *interp,
                             Tcl_Channel channel,
                             int        *valuePtr));

int
TclXOSSetAppend _ANSI_ARGS_((Tcl_Interp *interp,
                             Tcl_Channel channel,
                             int         value));

int
TclXOSGetCloseOnExec _ANSI_ARGS_((Tcl_Interp *interp,
                                  Tcl_Channel channel,
                                  int        *valuePtr));

int
TclXOSSetCloseOnExec _ANSI_ARGS_((Tcl_Interp *interp,
                                  Tcl_Channel channel,
                                  int         value));
#endif


