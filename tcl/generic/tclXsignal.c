/*
 * tclXsignal.c --
 *
 * Tcl Unix signal support routines and the signal and commands.  The #ifdefs
 * around several common Unix signals existing are for Windows.
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
 * $Id: tclXsignal.c,v 8.2 1997/06/12 21:08:31 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * If either SIGCLD or SIGCHLD are defined, define them both.  This makes
 * the interchangeable.  Windows doesn't have this signal.
 */

#if defined(SIGCLD) || defined(SIGCHLD)
#   ifndef SIGCLD
#      define SIGCLD SIGCHLD
#   endif
#   ifndef SIGCHLD
#      define SIGCHLD SIGCLD
#   endif
#endif

#ifndef MAXSIG
#  ifdef NSIG
#    define MAXSIG NSIG
#  else
#    define MAXSIG 32
#  endif
#endif

/*
 * Encore UMAX doesn't define SIG_ERR!.
 */
#ifndef SIG_ERR
#   define SIG_ERR  (void (*)())-1
#endif

/*
 * Value returned by Tcl_SignalId when an invalid signal is passed in.
 * Pointer is used as a quick check of a valid signal number.
 */
static char *unknownSignalIdMsg;

/*
 * Signal name table maps name to number.  Note, it is possible to have
 * more than MAXSIG entries in this table if the system defines multiple
 * symbols that have the same value.
 */

#define SIG_NAME_MAX 9  /* Maximum length of any signal name */

static struct {
    char *name;
    short num;
} sigNameTable [] = {
#ifdef SIGABRT
    {"ABRT",    SIGABRT},
#endif
#ifdef SIGALRM
    {"ALRM",    SIGALRM},
#endif
#ifdef SIGBUS
    {"BUS",     SIGBUS},
#endif
#ifdef SIGCHLD
    {"CHLD",    SIGCHLD},
#endif
#ifdef SIGCLD
    {"CLD",     SIGCLD},
#endif
#ifdef SIGCONT
    {"CONT",    SIGCONT},
#endif
#ifdef SIGEMT
    {"EMT",     SIGEMT},
#endif
#ifdef SIGFPE
    {"FPE",     SIGFPE},
#endif
#ifdef SIGHUP
    {"HUP",     SIGHUP},
#endif
#ifdef SIGILL
    {"ILL",     SIGILL},
#endif
#ifdef SIGINT
    {"INT",     SIGINT},
#endif
#ifdef SIGIO
    {"IO",      SIGIO},
#endif
#ifdef SIGIOT
    {"IOT",     SIGIOT},
#endif
#ifdef SIGKILL
    {"KILL",    SIGKILL},
#endif
#ifdef SIGLOST
    {"LOST",    SIGLOST},
#endif
#ifdef SIGPIPE
    {"PIPE",    SIGPIPE},
#endif
#ifdef SIGPOLL
    {"POLL",    SIGPOLL},
#endif
#ifdef SIGPROF
    {"PROF",    SIGPROF},
#endif
#ifdef SIGPWR
    {"PWR",     SIGPWR},
#endif
#ifdef SIGQUIT
    {"QUIT",    SIGQUIT},
#endif
#ifdef SIGSEGV
    {"SEGV",    SIGSEGV},
#endif
#ifdef SIGSTOP
    {"STOP",    SIGSTOP},
#endif
#ifdef SIGSYS
    {"SYS",     SIGSYS},
#endif
#ifdef SIGTERM
    {"TERM",    SIGTERM},
#endif
#ifdef SIGTRAP
    {"TRAP",    SIGTRAP},
#endif
#ifdef SIGTSTP
    {"TSTP",    SIGTSTP},
#endif
#ifdef SIGTTIN
    {"TTIN",    SIGTTIN},
#endif
#ifdef SIGTTOU
    {"TTOU",    SIGTTOU},
#endif
#ifdef SIGURG
    {"URG",     SIGURG},
#endif
#ifdef SIGUSR1
    {"USR1",    SIGUSR1},
#endif
#ifdef SIGUSR2
    {"USR2",    SIGUSR2},
#endif
#ifdef SIGVTALRM
    {"VTALRM",  SIGVTALRM},
#endif
#ifdef SIGWINCH
    {"WINCH",   SIGWINCH},
#endif
#ifdef SIGXCPU
    {"XCPU",    SIGXCPU},
#endif
#ifdef SIGXFSZ
    {"XFSZ",    SIGXFSZ},
#endif
    {NULL,         -1}};

#ifndef RETSIGTYPE
#   define RETSIGTYPE void
#endif

typedef RETSIGTYPE (*signalProcPtr_t) _ANSI_ARGS_((int));


/*
 * Defines if this is not Posix.
 */
#ifndef SIG_BLOCK
#   define SIG_BLOCK       1
#   define SIG_UNBLOCK     2
#endif

/*
 * Symbolic signal actions that can be associated with a signal.
 */
static char *SIGACT_DEFAULT = "default";
static char *SIGACT_IGNORE  = "ignore";
static char *SIGACT_ERROR   = "error";
static char *SIGACT_TRAP    = "trap";
static char *SIGACT_UNKNOWN = "unknown";

/*
 * Structure used to save error state of the interpreter.
 */
typedef struct {
    char  *result;
    char  *errorInfo;
    char  *errorCode;
} errState_t;

/*
 * Table containing a interpreters and there Async handler cookie.
 */
typedef struct {
    Tcl_Interp       *interp;
    Tcl_AsyncHandler  handler;
} interpHandler_t;

static interpHandler_t *interpTable = NULL;
static int              interpTableSize  = 0;
static int              numInterps  = 0;

/*
 * Application signal error handler.  Called after normal signal processing,
 * when a signal results in an error.   Its main purpose in life is to allow
 * interactive command loops to clear their input buffer on SIGINT.  This is
 * not currently a generic interface, but should be. Only one maybe active.
 */
static TclX_AppSignalErrorHandler appSigErrorHandler = NULL;
static ClientData                 appSigErrorClientData = NULL;

/*
 * Counters of signals that have occured but have not been processed.
 */
static unsigned signalsReceived [MAXSIG];

/*
 * Table of commands to evaluate when a signal occurs.  If the command is
 * NULL and the signal is received, an error is returned.
 */
static char *signalTrapCmds [MAXSIG];

/*
 * Prototypes of internal functions.
 */
static char *
GetSignalName _ANSI_ARGS_((int signalNum));

static int
GetSignalState _ANSI_ARGS_((int              signalNum,
                            signalProcPtr_t *sigProcPtr));

static int
SetSignalState _ANSI_ARGS_((int             signalNum,
                            signalProcPtr_t sigFunc));
static int
TclX_SignalCmd _ANSI_ARGS_((ClientData  clientData,
                            Tcl_Interp *interp,
                            int         argc,
                            char      **argv));

static int
BlockSignals _ANSI_ARGS_((Tcl_Interp    *interp,
                          int            action,
                          unsigned char  signals [MAXSIG]));

static char *
SignalBlocked _ANSI_ARGS_((Tcl_Interp  *interp,
                           int          signalNum));

static int
SigNameToNum _ANSI_ARGS_((Tcl_Interp *interp,
                          char       *sigName,
                          int        *sigNumPtr));

static int
ParseSignalSpec _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *signalStr,
                             int         allowZero));

static RETSIGTYPE
SignalTrap _ANSI_ARGS_((int signalNum));

static int
FormatTrapCode  _ANSI_ARGS_((Tcl_Interp  *interp,
                             int          signalNum,
                             Tcl_DString *command));

static errState_t *
SaveErrorState _ANSI_ARGS_((Tcl_Interp *interp));

static void
RestoreErrorState _ANSI_ARGS_((Tcl_Interp *interp,
                               errState_t *errStatePtr));

static int
EvalTrapCode _ANSI_ARGS_((Tcl_Interp *interp,
                          int         signalNum));

static int
ProcessASignal _ANSI_ARGS_((Tcl_Interp *interp,
                            int         background,
                            int         signalNum));

static int
ProcessSignals _ANSI_ARGS_((ClientData  clientData,
                            Tcl_Interp *interp,
                            int         cmdResultCode));

static int
ParseSignalList _ANSI_ARGS_((Tcl_Interp    *interp,
                             char          *signalListStr,
                             unsigned char  signals [MAXSIG]));

static int
SetSignalActions _ANSI_ARGS_((Tcl_Interp      *interp,
                              unsigned char    signals [MAXSIG],
                              signalProcPtr_t  actionFunc,
                              char            *command));

static char *
FormatSignalListEntry _ANSI_ARGS_((Tcl_Interp *interp,
                                   int         signalNum));

static int
ProcessSignalListEntry _ANSI_ARGS_((Tcl_Interp *interp,
                                    char       *signalEntry));

static int
GetSignalStates _ANSI_ARGS_((Tcl_Interp    *interp,
                             unsigned char  signals [MAXSIG]));

static int
SetSignalStates _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *signalKeyedList));

static void
SignalCmdCleanUp _ANSI_ARGS_((ClientData  clientData,
                              Tcl_Interp *interp));
static int
TclX_KillCmd _ANSI_ARGS_((ClientData  clientData,
                          Tcl_Interp *interp,
                          int         argc,
                          char      **argv));


/*-----------------------------------------------------------------------------
 * GetSignalName --
 *     Get the name for a signal.  This normalized SIGCHLD.
 * Parameters:
 *   o signalNum (I) - Signal number convert.
 * Results
 *   Static signal name.
 *-----------------------------------------------------------------------------
 */
static char *
GetSignalName (signalNum)
    int signalNum;
{
#ifdef SIGCHLD
    /*
     * Force name to always be SIGCHLD, even if system defines only SIGCLD.
     */
    if (signalNum == SIGCHLD)
        return "SIGCHLD";
#endif

    return Tcl_SignalId (signalNum);
}

/*-----------------------------------------------------------------------------
 * GetSignalState --
 *     Get the current state of the specified signal.
 * Parameters:
 *   o signalNum (I) - Signal number to query.
 *   o sigProcPtr (O) - The signal function is returned here.
 * Results
 *   TCL_OK or TCL_ERROR (check errno).
 *-----------------------------------------------------------------------------
 */
static int
GetSignalState (signalNum, sigProcPtr)
    int              signalNum;
    signalProcPtr_t *sigProcPtr;
{
#ifndef NO_SIGACTION
    struct sigaction currentState;

    if (sigaction (signalNum, NULL, &currentState) < 0)
        return TCL_ERROR;
    *sigProcPtr = currentState.sa_handler;
    return TCL_OK;
#else
    signalProcPtr_t  actionFunc;

#ifdef SIGKILL
    if (signalNum == SIGKILL) {
        *sigProcPtr = SIG_DFL;
        return TCL_OK;
    }
#endif
    actionFunc = signal (signalNum, SIG_DFL);
    if (actionFunc == SIG_ERR)
        return TCL_ERROR;
    if (actionFunc != SIG_DFL)
        signal (signalNum, actionFunc);  /* reset */
    *sigProcPtr = actionFunc;
    return TCL_OK;
#endif
}

/*-----------------------------------------------------------------------------
 * SetSignalState --
 *     Set the state of a signal.
 * Parameters:
 *   o signalNum (I) - Signal number to query.
 *   o sigFunc (O) - The signal function or SIG_DFL or SIG_IGN.
 * Results
 *   TCL_OK or TCL_ERROR (check errno).
 *-----------------------------------------------------------------------------
 */
static int
SetSignalState (signalNum, sigFunc)
    int             signalNum;
    signalProcPtr_t sigFunc;
{
#ifndef NO_SIGACTION
    struct sigaction newState;
    
    newState.sa_handler = sigFunc;
    sigfillset (&newState.sa_mask);
    newState.sa_flags = 0;

    if (sigaction (signalNum, &newState, NULL) < 0)
        return TCL_ERROR;

    return TCL_OK;
#else
    if (signal (signalNum, sigFunc) == SIG_ERR)
        return TCL_ERROR;
    else
        return TCL_OK;
#endif
}

/*-----------------------------------------------------------------------------
 * BlockSignals --
 *     
 *    Block or unblock the specified signals.  Returns an error if not a Posix
 * system.
 *
 * Parameters::
 *   o interp (O) - Error messages are returned in result.
 *   o action (I) - SIG_BLOCK or SIG_UNBLOCK.
 *   o signals (I) - Boolean array indexed by signal number that indicates
 *     the requested signals.
 * Returns:
 *   TCL_OK or TCL_ERROR, with error message in interp.
 *-----------------------------------------------------------------------------
 */
static int
BlockSignals (interp, action, signals)
    Tcl_Interp    *interp;
    int            action;
    unsigned char  signals [];
{
#ifndef NO_SIGACTION
    int      signalNum;
    sigset_t sigBlockSet;

    sigemptyset (&sigBlockSet);

    for (signalNum = 0; signalNum < MAXSIG; signalNum++) {
        if (signals [signalNum])
            sigaddset (&sigBlockSet, signalNum);
    }

    if (sigprocmask (action, &sigBlockSet, NULL)) {
        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }

    return TCL_OK;
#else
    interp->result =
       "Posix signals are not available on this system, can not block signals";
    return TCL_ERROR;
#endif
}

/*-----------------------------------------------------------------------------
 * SignalBlocked --
 *     
 *    Determine if a signal is blocked.  On non-Posix systems, always returns
 * "0".
 *
 * Parameters::
 *   o interp (O) - Error messages are returned in result.
 *   o signalNum (I) - The signal to determine the state for.
 * Returns:
 *   NULL if an error occured, or a pointer to a static string of "1" if the
 * signal is block, and a static string of "0" if it is not blocked.
 *-----------------------------------------------------------------------------
 */
static char *
SignalBlocked (interp, signalNum)
    Tcl_Interp  *interp;
    int          signalNum;
{
#ifndef NO_SIGACTION
    sigset_t sigBlockSet;

    if (sigprocmask (SIG_BLOCK, NULL, &sigBlockSet)) {
        interp->result = Tcl_PosixError (interp);
        return NULL;
    }
    return sigismember (&sigBlockSet, signalNum) ? "1" : "0";
#else
    return "0";
#endif
}

/*-----------------------------------------------------------------------------
 * SigNameToNum --
 *    Converts a UNIX signal name to its number, returns -1 if not found.
 * the name may be upper or lower case and may optionally have the leading
 * "SIG" omitted.
 *
 * Parameters:
 *   o interp (I) - Errors are returned in result.
 *   o sigName (I) - Name of signal to convert.
 *   o sigNumPtr (O) - Signal number is returned here.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
SigNameToNum (interp, sigName, sigNumPtr)
    Tcl_Interp *interp;
    char       *sigName;
    int        *sigNumPtr;
{
    char  sigNameUp [SIG_NAME_MAX+1];  /* Upshifted signal name */
    char *sigNamePtr; 
    int   idx;

    /*
     * Copy and upshift requested name.
     */
    if (strlen (sigName) > SIG_NAME_MAX)
        goto invalidSignal;   /* Name too long */

    TclX_UpShift (sigNameUp, sigName);

    if (STRNEQU (sigNameUp, "SIG", 3))
        sigNamePtr = &sigNameUp [3];
    else
        sigNamePtr = sigNameUp;

    for (idx = 0; sigNameTable [idx].num != -1; idx++) {
        if (STREQU (sigNamePtr, sigNameTable [idx].name)) {
            *sigNumPtr = sigNameTable [idx].num;
            return TCL_OK;
        }
    }

  invalidSignal:
    Tcl_AppendResult (interp, "invalid signal \"", sigName, "\"",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * ParseSignalSpec --
 *  
 *   Parse a signal specified as either a name or a number.
 * 
 * Parameters:
 *   o interp (O) - Interpreter for returning errors.
 *   o signalStr (I) - The signal name or number string.
 *   o allowZero (I) - Allow zero as a valid signal number (for kill).
 * Returns:
 *   The signal number converted, or -1 if an error occures.
 *-----------------------------------------------------------------------------
 */
static int
ParseSignalSpec (interp, signalStr, allowZero)
    Tcl_Interp *interp;
    char       *signalStr;
    int         allowZero;
{
    int  signalNum;

    /*
     * If its a number, validate that number is actual a valid signal number
     * for this system.  If either fail, try it as a name.  Just let
     * SigNameToNum generate the error message if its a number, but not a
     * valid signal.
     */
    if (TclX_StrToInt (signalStr, 0, &signalNum)) {
        if (allowZero && (signalNum == 0))
            return 0;
        if (Tcl_SignalId (signalNum) != unknownSignalIdMsg)
            return signalNum;
    }
    if (SigNameToNum (interp, signalStr, &signalNum) != TCL_OK)
        return -1;
    return signalNum;
}

/*-----------------------------------------------------------------------------
 * SignalTrap --
 *
 *   Trap handler for UNIX signals.  Sets tells all registered interpreters
 * that a trap has occured and saves the trap info.  The first interpreter to
 * call it's async signal handler will process all pending signals.
 *-----------------------------------------------------------------------------
 */
static RETSIGTYPE
SignalTrap (signalNum)
    int signalNum;
{
    int idx;

    /*
     * Record the count of the number of this type of signal that has occured
     * and tell all the interpreters to call the async handler when safe.
     */
    signalsReceived [signalNum]++;

    for (idx = 0; idx < numInterps; idx++)
        Tcl_AsyncMark (interpTable [idx].handler);

#ifdef NO_SIGACTION
    /*
     * For old-style Unix signals, the signal must be explictly re-enabled.
     * Not done for SIGCHLD, as we would continue to the signal until the
     * wait is done.  This is fixed by Posix signals and is not necessary under
     * BSD, but it done this way for consistency.
     */
#ifdef SIGCHLD
    if (signalNum != SIGCHLD) {
        if (SetSignalState (signalNum, SignalTrap) == TCL_ERROR)
            panic ("SignalTrap bug");
    }
#else
    if (SetSignalState (signalNum, SignalTrap) == TCL_ERROR)
        panic ("SignalTrap bug");
#endif /* SIGCHLD */
#endif /* NO_SIGACTION */
}

/*-----------------------------------------------------------------------------
 * SaveErrorState --
 *  
 *   Save the error state of the interpreter (result, errorInfo and errorCode).
 *
 * Parameters:
 *   o interp (I) - The interpreter to save. Result will be reset.
 * Returns:
 *   A dynamically allocated structure containing all three strings,  Its
 * allocated as a single malloc block.
 *-----------------------------------------------------------------------------
 */
static errState_t *
SaveErrorState (interp)
    Tcl_Interp *interp;
{
    errState_t *errStatePtr;
    char       *errorInfo, *errorCode, *nextPtr;
    int         len;

    errorInfo = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
    errorCode = Tcl_GetVar (interp, "errorCode", TCL_GLOBAL_ONLY);

    len = sizeof (errState_t) + strlen (interp->result) + 1;
    if (errorInfo != NULL)
        len += strlen (errorInfo) + 1;
    if (errorCode != NULL)
        len += strlen (errorCode) + 1;


    errStatePtr = (errState_t *) ckalloc (len);
    nextPtr = ((char *) errStatePtr) + sizeof (errState_t);

    errStatePtr->result = nextPtr;
    strcpy (errStatePtr->result, interp->result);
    nextPtr += strlen (interp->result) + 1;

    errStatePtr->errorInfo = NULL;
    if (errorInfo != NULL) {
        errStatePtr->errorInfo = nextPtr;
        strcpy (errStatePtr->errorInfo, errorInfo);
        nextPtr += strlen (errorInfo) + 1;
    }

    errStatePtr->errorCode = NULL;
    if (errorCode != NULL) {
        errStatePtr->errorCode = nextPtr;
        strcpy (errStatePtr->errorCode, errorCode);
        nextPtr += strlen (errorCode) + 1;
    }

    Tcl_ResetResult (interp);
    return errStatePtr;
}

/*-----------------------------------------------------------------------------
 * RestoreErrorState --
 *  
 *   Restore the error state of the interpreter that was saved by
 * SaveErrorState.
 *
 * Parameters:
 *   o interp (I) - The interpreter to save.
 *   o errStatePtr (I) - Error state from SaveErrorState.  This structure will
 *     be freed. 
 * Returns:
 *   A dynamically allocated structure containing all three strings,  Its
 * allocated as a single malloc block.
 *-----------------------------------------------------------------------------
 */
static void
RestoreErrorState (interp, errStatePtr)
    Tcl_Interp *interp;
    errState_t *errStatePtr;
{
    Tcl_SetResult (interp, errStatePtr->result, TCL_VOLATILE);
    if (errStatePtr->errorInfo != NULL)
        Tcl_SetVar (interp, "errorInfo", errStatePtr->errorInfo,
                    TCL_GLOBAL_ONLY);
    if (errStatePtr->errorCode != NULL)
        Tcl_SetVar (interp, "errorCode", errStatePtr->errorCode,
                    TCL_GLOBAL_ONLY);

    ckfree ((char *) errStatePtr);
}

/*-----------------------------------------------------------------------------
 * FormatTrapCode --
 *     Format the signal name into the signal trap command.  Replacing %S with
 * the signal name.
 *
 * Parameters:
 *   o interp (I/O) - The interpreter to return errors in.
 *   o signalNum (I) - The signal number of the signal that occured.
 *   o command (O) - The resulting command adter the formatting.
 *-----------------------------------------------------------------------------
 */
static int
FormatTrapCode (interp, signalNum, command)
    Tcl_Interp  *interp;
    int          signalNum;
    Tcl_DString *command;
{
    char *copyPtr, *scanPtr;

    Tcl_DStringInit (command);

    copyPtr = scanPtr = signalTrapCmds [signalNum];

    while (*scanPtr != '\0') {
        if (*scanPtr != '%') {
            scanPtr++;
            continue;
        }
        if (scanPtr [1] == '%') {
            scanPtr += 2;
            continue;
        }
        Tcl_DStringAppend (command, copyPtr, (scanPtr - copyPtr));

        switch (scanPtr [1]) {
          case 'S': {
              Tcl_DStringAppend (command, GetSignalName (signalNum), -1);
              break;
          }
          default:
            goto badSpec;
        }
        scanPtr += 2;
        copyPtr = scanPtr;
    }
    Tcl_DStringAppend (command, copyPtr, copyPtr - scanPtr);

    return TCL_OK;

    /*
     * Handle bad % specification currently pointed to by scanPtr.
     */
  badSpec:
    {
        char badSpec [2];
        
        badSpec [0] = scanPtr [1];
        badSpec [1] = '\0';
        Tcl_AppendResult (interp, "bad signal trap command formatting ",
                          "specification \"%", badSpec,
                          "\", expected one of \"%%\" or \"%S\"",
                          (char *) NULL);
        return TCL_ERROR;
    }
}

/*-----------------------------------------------------------------------------
 * EvalTrapCode --
 *     Run code as the result of a signal.  The symbolic signal name is
 * formatted into the command replacing %S with the symbolic signal name.
 *
 * Parameters:
 *   o interp (I) - The interpreter to run the signal in. If an error
 *     occures, then the result will be left in the interp.
 *   o signalNum (I) - The signal number of the signal that occured.
 * Return:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
EvalTrapCode (interp, signalNum)
    Tcl_Interp *interp;
    int         signalNum;
{
    int          result;
    Tcl_DString  command;

    Tcl_ResetResult (interp);

    /*
     * Format the signal name into the command.  This also allows the signal
     * to be reset in the command.
     */

    result = FormatTrapCode (interp,
                             signalNum,
                             &command);
    if (result == TCL_OK)
        result = Tcl_GlobalEval (interp, 
                                 command.string);

    Tcl_DStringFree (&command);

    if (result == TCL_ERROR) {
        char errorInfo [64];

        sprintf (errorInfo, "\n    while executing signal trap code for %s%s",
                 Tcl_SignalId (signalNum), " signal");
        Tcl_AddErrorInfo (interp, errorInfo);

        return TCL_ERROR;
    }
    
    Tcl_ResetResult (interp);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * ProcessASignal --
 *  
 *   Do processing on the specified signal.
 *
 * Parameters:
 *   o interp (I) - Result will contain the result of the signal handling
 *     code that was evaled.
 *   o background (I) - Signal handler was called from the event loop with
 *     no current interp.
 *   o signalNum (I) - The signal to process.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ProcessASignal (interp, background, signalNum)
    Tcl_Interp *interp;
    int         background;
    int         signalNum;
{
    int result = TCL_OK;

    /*
     * Either return an error or evaluate code associated with this signal.
     * If evaluating code, call it for each time the signal occured.
     */
    if (signalTrapCmds [signalNum] == NULL) {
        char *signalName = GetSignalName (signalNum);

        signalsReceived [signalNum] = 0;
        Tcl_SetErrorCode (interp, "POSIX", "SIG", signalName, (char*) NULL);
        Tcl_AppendResult (interp, signalName, " signal received", 
                          (char *)NULL);
        Tcl_SetVar (interp, "errorInfo", "", TCL_GLOBAL_ONLY);
        result = TCL_ERROR;

        /*
         * Let the application at signals that generate errors.
         */
        if (appSigErrorHandler != NULL)
            result = (*appSigErrorHandler) (interp,
                                            appSigErrorClientData,
                                            background,
                                            signalNum);
    } else {
        while (signalsReceived [signalNum] > 0) {
            (signalsReceived [signalNum])--;
            result = EvalTrapCode (interp, signalNum);
            if (result == TCL_ERROR)
                break;
        }
    }
    return result;
}

/*-----------------------------------------------------------------------------
 * ProcessSignals --
 *  
 *   Called by the async handler to process pending signals in a safe state
 * interpreter state.  This is often called just after a command completes.
 * The results of the command are passed to this procedure and may be altered
 * by it.  If trap code is specified for the signal that was received, then
 * the trap will be executed, otherwise an error result will be returned
 * indicating that the signal occured.  If an error is returned, clear the
 * errorInfo variable.  This makes sure it exists and that it is empty,
 * otherwise bogus or non-existant information will be returned if this
 * routine was called somewhere besides Tcl_Eval.  If a signal was received
 * multiple times and a trap is set on it, then that trap will be executed for
 * each time the signal was received.
 * 
 * Parameters:
 *   o clientData (I) - Not used.
 *   o interp (I/O) - interp->result should contain the result for
 *     the command that just executed.  This will either be restored or
 *     replaced with a new result.  If this is NULL, then no interpreter
 *     is directly available (i.e. event loop).  In this case, the first
 *     interpreter in internal interpreter table is used.  If an error results
 *     from signal processing, it is handled via Tcl_BackgroundError.
 *   o cmdResultCode (I) - The integer result returned by the command that
 *     Tcl_Eval just completed.  Should be TCL_OK if not called from
 *     Tcl_Eval.
 * Returns:
 *   Either the original result code, an error result if one of the
 *   trap commands returned an error, or an error indicating the
 *   a signal occured.
 *-----------------------------------------------------------------------------
 */
static int
ProcessSignals (clientData, interp, cmdResultCode)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         cmdResultCode;
{
    Tcl_Interp *sigInterp;
    errState_t *errStatePtr;
    int         signalNum, result, idx;

    /*
     * Get the interpreter is it wasn't supplied, if none is available,
     * bail out.
     */
    if (interp == NULL) {
        if (numInterps == 0)
            return cmdResultCode;
        sigInterp = interpTable [0].interp;
    } else {
        sigInterp = interp;
    }

    errStatePtr = SaveErrorState (sigInterp);

    /*
     * Process all signals.  Don't process any more if one returns an error.
     */
    result = TCL_OK;

    for (signalNum = 1; signalNum < MAXSIG; signalNum++) {
        if (signalsReceived [signalNum] == 0)
            continue;
        result = ProcessASignal (sigInterp,
                                 (interp == NULL),
                                 signalNum);
        if (result == TCL_ERROR)
            break;
    }

    /*
     * Restore result and error state if we didn't get an error in signal
     * handling.
     */
    if (result != TCL_ERROR) {
        RestoreErrorState (sigInterp, errStatePtr);
    } else {
        ckfree ((char *) errStatePtr);
        cmdResultCode = TCL_ERROR;
    }

    /*
     * Reset the signal received flag in case more signals are pending.
     */
    for (signalNum = 1; signalNum < MAXSIG; signalNum++) {
        if (signalsReceived [signalNum] != 0)
            break;
    }
    if (signalNum < MAXSIG) {
        for (idx = 0; idx < numInterps; idx++)
            Tcl_AsyncMark (interpTable [idx].handler);
    }

    /*
     * If a signal handler returned an error and an interpreter was not
     * supplied, call the background error handler.
     */
    if ((result == TCL_ERROR) && (interp == NULL)) {
        Tcl_BackgroundError (sigInterp);
    }
    return cmdResultCode;
}

/*-----------------------------------------------------------------------------
 * ParseSignalList --
 *  
 *   Parse a list of signal names or numbers.  Also handles the special case
 * of the signal being a single entry of "*".
 * 
 * Parameters:
 *   o interp (O) - Interpreter for returning errors.
 *   o signalListStr (I) - The Tcl list of signals to convert.
 *   o signals (O) - Boolean array indexed by signal number that indicates
 *     which signals are set.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ParseSignalList (interp, signalListStr, signals)
    Tcl_Interp    *interp;
    char          *signalListStr;
    unsigned char  signals [MAXSIG];
{
    char  **signalListArgv;
    int     signalListSize, signalNum, idx, cnt;

    if (Tcl_SplitList (interp, signalListStr, &signalListSize, 
                       &signalListArgv) != TCL_OK)
        return TCL_ERROR;

    if (signalListSize == 0) {
        Tcl_AppendResult (interp, "signal list may not be empty",
                          (char *) NULL);
        goto errorExit;
    }

    memset (signals, FALSE, sizeof (unsigned char) * MAXSIG);

    /*
     * Handle the wild card signal.  Don't return signals that can't be
     * modified.
     */
    if (STREQU (signalListArgv [0], "*")) {
        if (signalListSize != 1)
            goto wildMustBeAlone;
        cnt = 0;
        for (idx = 0; sigNameTable [idx].name != NULL; idx++) {
            signalNum = sigNameTable [idx].num;
#ifdef SIGKILL
            if ((signalNum == SIGKILL) || (signalNum == SIGSTOP))
                continue;
#endif
            signals [signalNum] = TRUE;
        }
        goto okExit;
    }

    /*
     * Handle individually specified signals.
     */
    for (idx = 0; idx < signalListSize; idx++) {
        if (STREQU (signalListArgv [idx], "*"))
            goto wildMustBeAlone;

        signalNum = ParseSignalSpec (interp,
                                     signalListArgv [idx],
                                     FALSE);  /* Zero not valid */
        if (signalNum < 0)
            goto errorExit;
        signals [signalNum] = TRUE;
    }

  okExit:
    ckfree ((char *) signalListArgv);
    return TCL_OK;

  wildMustBeAlone:
    Tcl_AppendResult (interp, "when \"*\" is specified in the signal list, ",
                      "no other signals may be specified", (char *) NULL);

  errorExit:
    ckfree ((char *) signalListArgv);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * SetSignalActions --
 *     
 *    Set the signal state for the specified signals.  
 *
 * Parameters::
 *   o interp (O) - The list is returned in the result.
 *   o signals (I) - Boolean array indexed by signal number that indicates
 *     the requested signals.
 *   o actionFunc (I) - The function to run when the signal is received.
 *   o command (I) - If the function is the "trap" function, this is the
 *     Tcl command to run when the trap occurs.  Otherwise, NULL.
 * Returns:
 *   TCL_OK or TCL_ERROR, with error message in interp.
 *-----------------------------------------------------------------------------
 */
static int
SetSignalActions (interp, signals, actionFunc, command)
    Tcl_Interp      *interp;
    unsigned char    signals [MAXSIG];
    signalProcPtr_t  actionFunc;
    char            *command;
{
    int signalNum;

    for (signalNum = 0; signalNum < MAXSIG; signalNum++) {
        if (!signals [signalNum])
            continue;

        if (signalTrapCmds [signalNum] != NULL) {
            ckfree (signalTrapCmds [signalNum]);
            signalTrapCmds [signalNum] = NULL;
        }
        if (command != NULL)
            signalTrapCmds [signalNum] = ckstrdup (command);

        if (SetSignalState (signalNum, actionFunc) == TCL_ERROR) {
            Tcl_AppendResult (interp, Tcl_PosixError (interp),
                              " while setting ", Tcl_SignalId (signalNum),
                              (char *) NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * FormatSignalListEntry --
 *     
 *    Retrieve a signal's state and format a keyed list entry used to describe
 * a that state.
 *
 * Parameters::
 *   o interp (O) - Error messages are returned here.
 *   o signalNum (I) - The signal to get the state for.
 * Returns:
 *   A pointer to the dynamically allocate list entry, or NULL if an error
 * occurs.
 *-----------------------------------------------------------------------------
 */
static char *
FormatSignalListEntry (interp, signalNum)
    Tcl_Interp *interp;
    int         signalNum;
{
    char            *sigState [3], *sigEntry [2], *entry;
    signalProcPtr_t  actionFunc;

    if (GetSignalState (signalNum, &actionFunc) == TCL_ERROR)
        goto unixSigError;

    sigState [2] = NULL;
    if (actionFunc == SIG_DFL) {
        sigState [0]  = SIGACT_DEFAULT;
    } else if (actionFunc == SIG_IGN) {
        sigState [0] = SIGACT_IGNORE;
    } else if (actionFunc == SignalTrap) {
        if (signalTrapCmds [signalNum] == NULL)
            sigState [0] = SIGACT_ERROR;
        else {
            sigState [0] = SIGACT_TRAP;
            sigState [2] = signalTrapCmds [signalNum];
        }
    } else {
        sigState [0] = SIGACT_UNKNOWN;
    }
    
    sigState [1] = SignalBlocked (interp, signalNum);
    if (sigState [1] == NULL)
        goto unixSigError;
    
    sigEntry [0] = Tcl_SignalId (signalNum);
    sigEntry [1] = Tcl_Merge ((sigState [2] == NULL) ? 2 : 3,
                              sigState);

    entry = Tcl_Merge (2, sigEntry);
    ckfree (sigEntry [1]);
    return entry;

  unixSigError:
    Tcl_AppendResult (interp, Tcl_PosixError (interp),
                      " while getting ", Tcl_SignalId (signalNum),
                      (char *) NULL);
    return NULL;
}

/*-----------------------------------------------------------------------------
 * ProcessSignalListEntry --
 *     
 *    Parse a keyed list entry used to describe a signal state and set the
 * signal to that state.  If the signal action is specified as "unknown",
 * it is ignored.
 *
 * Parameters::
 *   o interp (O) - Error messages are returned here.
 *   o signalEntry (I) - Entry from the keyed list.
 * Returns:
 *   TCL_OK or TCL_ERROR;
 *-----------------------------------------------------------------------------
 */
static int
ProcessSignalListEntry (interp, signalEntry)
    Tcl_Interp *interp;
    char       *signalEntry;
{
    char           **sigEntry = NULL, **sigState = NULL;
    int              sigEntrySize, sigStateSize;
    int              signalNum, blocked;
    signalProcPtr_t  actionFunc = NULL;
    unsigned char    signals [MAXSIG];

    /*
     * Split the list.
     */
    if (Tcl_SplitList (interp, signalEntry,
                       &sigEntrySize, &sigEntry) != TCL_OK)
        return TCL_ERROR;
    if (sigEntrySize != 2)
        goto invalidEntry;
    if (Tcl_SplitList (interp, sigEntry [1],
                       &sigStateSize, &sigState) != TCL_OK)
        goto errorExit;
    if (sigStateSize < 2 || sigStateSize > 3)
        goto invalidEntry;
    
    /*
     * Parse the signal name and state.
     */
    if (SigNameToNum (interp, sigEntry [0], &signalNum) != TCL_OK)
        return TCL_ERROR;

    if (STREQU (sigState [0], SIGACT_DEFAULT)) {
        actionFunc = SIG_DFL;
        if (sigStateSize != 2)
            goto invalidEntry;
    } else if (STREQU (sigState [0], SIGACT_IGNORE)) {
        actionFunc = SIG_IGN;
        if (sigStateSize != 2)
            goto invalidEntry;
    } else if (STREQU (sigState [0], SIGACT_ERROR)) {
        actionFunc = SignalTrap;
        if (sigStateSize != 2)
            goto invalidEntry;
    } else if (STREQU (sigState [0], SIGACT_TRAP)) {
        actionFunc = SignalTrap;
        if (sigStateSize != 3)    /* Must have command */
            goto invalidEntry;
    } else if (STREQU (sigState [0], SIGACT_UNKNOWN)) {
        if (sigStateSize != 2)
            goto invalidEntry;
        goto exitPoint;  /* Ignore non-Tcl signals */
    }

    if (Tcl_GetBoolean (interp, sigState [1], &blocked) != TCL_OK)
        goto errorExit;
    
    memset (signals, FALSE, sizeof (unsigned char) * MAXSIG);
    signals [signalNum] = TRUE;

    /*
     * Set signal actions and handle blocking if its supported on this
     * system.  If the signal is to be blocked, we do it before setting up
     * the handler.  If its to be unblocked, we do it after.
     */
#ifndef NO_SIGACTION
    if (blocked) {
        if (BlockSignals (interp, SIG_BLOCK, signals) != TCL_OK)
            goto errorExit;
    }
#endif
    if (SetSignalActions (interp, signals, actionFunc,
                        sigState [2]) != TCL_OK)
        goto errorExit;
#ifndef NO_SIGACTION
    if (!blocked) {
        if (BlockSignals (interp, SIG_UNBLOCK, signals) != TCL_OK)
            goto errorExit;
    }
#endif
    
  exitPoint:
    if (sigEntry != NULL)
        ckfree ((char *) sigEntry);
    if (sigState != NULL)
        ckfree ((char *) sigState);
    return TCL_OK;

  invalidEntry:
    Tcl_AppendResult (interp, "invalid signal keyed list entry \"",
                      signalEntry, "\"", (char *) NULL);

  errorExit:
    if (sigEntry != NULL)
        ckfree ((char *) sigEntry);
    if (sigState != NULL)
        ckfree ((char *) sigState);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * GetSignalStates --
 *     
 *    Return a keyed list containing the signal states for the specified
 * signals.
 *
 * Parameters::
 *   o interp (O) - The list is returned in the result.
 *   o signals (I) - Boolean array indexed by signal number that indicates
 *     the requested signals.
 * Returns:
 *   TCL_OK or TCL_ERROR, with error message in interp.
 *-----------------------------------------------------------------------------
 */
static int
GetSignalStates (interp, signals)
    Tcl_Interp    *interp;
    unsigned char  signals [MAXSIG];
{
    int    signalNum, idx, cnt;
    char  *stateKeyedList [MAXSIG];

    cnt = 0;
    for (signalNum = 0; signalNum < MAXSIG; signalNum++) {
        if (!signals [signalNum])
            continue;
        stateKeyedList [cnt] =
            FormatSignalListEntry (interp, 
                                   signalNum);
        if (stateKeyedList [cnt] == NULL)
            goto errorExit;
        cnt++;
    }

    Tcl_SetResult (interp,
                   Tcl_Merge (cnt,
                              stateKeyedList),
                   TCL_DYNAMIC);
    for (idx = 0; idx < cnt; idx++)
        ckfree (stateKeyedList [idx]);
    return TCL_OK;

  errorExit:
    for (idx = 0; idx < cnt; idx++)
        ckfree (stateKeyedList [idx]);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * SetSignalStates --
 *     
 *    Set signal states from keyed list in the format returned by
 * GetSignalStates.
 *
 * Parameters::
 *   o interp (O) - Errors are returned in the result.
 *   o signalKeyedList (I) - Keyed list describing signal states.
 * Returns:
 *   TCL_OK or TCL_ERROR, with error message in interp.
 *-----------------------------------------------------------------------------
 */
static int
SetSignalStates (interp, signalKeyedList)
    Tcl_Interp *interp;
    char       *signalKeyedList;
{
    int     idx, signalListSize;
    char  **signalList;

    if (Tcl_SplitList (interp, signalKeyedList,
                       &signalListSize, &signalList) != TCL_OK)
        return TCL_ERROR;

    for (idx = 0; idx < signalListSize; idx++) {
        if (ProcessSignalListEntry (interp, signalList [idx]) != TCL_OK)
            goto errorExit;
    }
    ckfree ((char *) signalList);
    return TCL_OK;

  errorExit:
    ckfree ((char *) signalList);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_SignalCmd --
 *     Implements the TCL signal command:
 *         signal action siglist ?command?
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 * Side effects:
 *	Signal handling states may be changed.
 *-----------------------------------------------------------------------------
 */
static int
TclX_SignalCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    unsigned char  signals [MAXSIG];

    if ((argc < 3) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " action signalList ?command?", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Do the specified action on the signals.  "set" has a special format
     * for the signal list, so do it first.
     */
    if (STREQU (argv [1], "set")) {
        if (argc != 3)
            goto cmdNotValid;
        return SetSignalStates (interp, argv [2]);
    }

    if (ParseSignalList (interp,
                         argv [2],
                         signals) != TCL_OK)
        return TCL_ERROR;

    if (STREQU (argv [1], SIGACT_TRAP)) {
        if (argc != 4) {
            Tcl_AppendResult (interp, "command required for ",
                             "trapping signals", (char *) NULL);
            return TCL_ERROR;
        }
        return SetSignalActions (interp,
                                 signals,
                                 SignalTrap,
                                 argv [3]);
    }

    if (argc != 3)
        goto cmdNotValid;
    
    if (STREQU (argv [1], SIGACT_DEFAULT)) {
        return SetSignalActions (interp,
                                 signals,
                                 SIG_DFL,
                                 NULL);
    }

    if (STREQU (argv [1], SIGACT_IGNORE)) {
        return SetSignalActions (interp,
                                 signals,
                                 SIG_IGN,
                                 NULL);
    }

    if (STREQU (argv [1], SIGACT_ERROR)) {
        return SetSignalActions (interp,
                                 signals,
                                 SignalTrap,
                                 NULL);
    }

    if (STREQU (argv [1], "get")) {
        return GetSignalStates (interp,
                                signals);
    }

    if (STREQU (argv [1], "block")) {
        return BlockSignals (interp,
                             SIG_BLOCK,
                             signals);
    }

    if (STREQU (argv [1], "unblock")) {
        return BlockSignals (interp,
                             SIG_UNBLOCK,
                             signals);
    }

    /*
     * Not a valid action.
     */
    Tcl_AppendResult (interp, "invalid signal action specified: ", 
                      argv [1], ": expected one of \"default\", ",
                      "\"ignore\", \"error\", \"trap\", \"get\", ",
                      "\"set\", \"block\", or \"unblock\"", (char *) NULL);
    return TCL_ERROR;


  cmdNotValid:
    Tcl_AppendResult (interp, "command may not be ",
                      "specified for \"", argv [1], "\" action",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_KillCmd --
 *     Implements the TCL kill command:
 *        kill ?-pgroup? ?signal? idlist
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *-----------------------------------------------------------------------------
 */
static int
TclX_KillCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int     argc;
    char      **argv;
{
    int    signalNum, nextArg, idx, procId, procArgc;
    int    pgroup = FALSE;
    char **procArgv;
    
#ifdef SIGTERM
#   define DEFAULT_KILL_SIGNAL SIGTERM
#else
#   define DEFAULT_KILL_SIGNAL SIGINT
#endif

    if (argc < 2)
        goto usage;

    nextArg = 1;
    if (STREQU (argv [nextArg], "-pgroup")) {
        pgroup = TRUE;
        nextArg++;
    }
        
    if (((argc - nextArg) < 1) || ((argc - nextArg) > 2))
        goto usage;

    /*
     * Get the signal.
     */
    if ((argc - nextArg) == 1) {
        signalNum = DEFAULT_KILL_SIGNAL;
    } else {
        signalNum = ParseSignalSpec (interp,
                                     argv [nextArg],
                                     TRUE);  /* Allow zero */
        if (signalNum < 0)
            return TCL_ERROR;
        nextArg++;
    }

    if (Tcl_SplitList (interp, argv [nextArg], &procArgc, 
                       &procArgv) != TCL_OK)
        return TCL_ERROR;

    for (idx = 0; idx < procArgc; idx++) {
        if (Tcl_GetInt (interp, procArgv [idx], &procId) != TCL_OK)
            goto errorExit;
        
        if (pgroup)
            procId = -procId;

        if (TclXOSkill (interp, procId, signalNum, argv [0]) != TCL_OK)
            goto errorExit;
    }

    ckfree ((char *) procArgv);
    return TCL_OK;
        
  errorExit:
    ckfree ((char *) procArgv);
    return TCL_ERROR;;

  usage:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                      " ?-pgroup? ?signal? idlist", (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * SignalCmdCleanUp --
 *
 *   Clean up the signal data structure when an interpreter is deleted. If
 * this is the last interpreter, clean up all tables.
 *
 * Parameters:
 *   o clientData (I) - Not used.
 *   o interp (I) - Interp that is being deleted.
 *-----------------------------------------------------------------------------
 */
static void
SignalCmdCleanUp (clientData, interp)
    ClientData  clientData;
    Tcl_Interp *interp;
{
    int  idx;

    for (idx = 0; idx < numInterps; idx++) {
        if (interpTable [idx].interp == interp)
            break;
    }
    if (idx == numInterps)
        panic ("signal interp lost");

    Tcl_AsyncDelete(interpTable[idx].handler);
    interpTable [idx] = interpTable [--numInterps];

    /*
     * If there are no more interpreters, clean everything up.
     */
    if (numInterps == 0) {
        ckfree ((char *) interpTable);
        interpTable = NULL;
        interpTableSize = 0;

        for (idx = 0; idx < MAXSIG; idx++) {
            if (signalTrapCmds [idx] != NULL) {
                ckfree (signalTrapCmds [idx]);
                signalTrapCmds [idx] = NULL;
            }
        }
    }
}

/*-----------------------------------------------------------------------------
 * TclX_SetupSigInt --
 *    Set up SIGINT to the "error" state if the current state is default.
 * This is done because shells set SIGINT to ignore for background processes
 * so that they don't die on signals generated by the user at the keyboard.
 * Tcl only enables SIGINT catching if it is an interactive session.
 *-----------------------------------------------------------------------------
 */
void
TclX_SetupSigInt ()
{
    signalProcPtr_t  actionFunc;

    if ((GetSignalState (SIGINT, &actionFunc) == TCL_OK) &&
        (actionFunc == SIG_DFL))
        SetSignalState (SIGINT, SignalTrap);
}

/*-----------------------------------------------------------------------------
 * TclX_SetAppSignalErrorHandler --
 *
 *   Set the current application signal error handler.  This is kind of a
 * hack.  It just saves the handler and client data in globals.
 *
 * Parameters:
 *   o errorFunc (I) - Error handling function.
 *   o clientData (I) - Client data to pass to function
 *-----------------------------------------------------------------------------
 */
void
TclX_SetAppSignalErrorHandler (errorFunc, clientData)
    TclX_AppSignalErrorHandler errorFunc;
    ClientData                 clientData;
{
    appSigErrorHandler = errorFunc;
    appSigErrorClientData = clientData;
}

/*-----------------------------------------------------------------------------
 * TclX_SignalInit --
 *      Initializes singal handling for a interpreter.
 *-----------------------------------------------------------------------------
 */
void
TclX_SignalInit (interp)
    Tcl_Interp *interp;
{
    int              idx;
    interpHandler_t *newTable;

    /*
     * If this is the first interpreter, set everything up.
     */
    if (numInterps == 0) {
        interpTableSize = 4;
        interpTable = (interpHandler_t *)
            ckalloc (sizeof (interpHandler_t) * interpTableSize);

        for (idx = 0; idx < MAXSIG; idx++) {
            signalsReceived [idx] = 0;
            signalTrapCmds [idx] = NULL;
        }
        /*
         * Get address of "unknown signal" message.
         */
        unknownSignalIdMsg = Tcl_SignalId (20000);
    }

    /*
     * If there is not room in this table for another interp, expand it.
     */
    if (numInterps == interpTableSize) {
        newTable = (interpHandler_t *)
            ckalloc (sizeof (interpHandler_t) * interpTableSize * 2);
        memcpy (newTable, interpTable,
                sizeof (interpHandler_t) * interpTableSize);
        ckfree ((char *) interpTable);
        interpTable = newTable;
        interpTableSize *= 2;
    }

    /*
     * Add this interpreter to the list and set up a async handler.
     * Arange for clean up on the interpreter being deleted.
     */
    interpTable [numInterps].interp = interp;
    interpTable [numInterps].handler =
        Tcl_AsyncCreate (ProcessSignals, (ClientData) NULL);
    numInterps++;

    Tcl_CallWhenDeleted (interp, SignalCmdCleanUp, (ClientData) NULL);

    Tcl_CreateCommand (interp, "signal", TclX_SignalCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand (interp, "kill", TclX_KillCmd,
                       (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);
}


