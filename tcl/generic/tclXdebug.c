/*
 * tclXdebug.c --
 *
 * Tcl command execution trace command.
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
 * $Id: tclXdebug.c,v 7.1 1996/07/18 19:36:16 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Client data structure for the cmdtrace command.
 */
#define ARG_TRUNCATE_SIZE 40
#define CMD_TRUNCATE_SIZE 60

typedef struct traceInfo_t {
    Tcl_Interp *interp;
    Tcl_Trace   traceHolder;
    int         inTrace;
    int         noEval;
    int         noTruncate;
    int         procCalls;
    int         depth;
    char       *callback;
    Tcl_Channel channel;
    } traceInfo_t, *traceInfo_pt;

/*
 * Prototypes of internal functions.
 */
static void
TraceDelete _ANSI_ARGS_((Tcl_Interp   *interp,
                         traceInfo_pt  infoPtr));

static void
PrintStr _ANSI_ARGS_((Tcl_Channel  channel,
                      char        *string,
                      int          numChars,
                      int          quoted));

static void
PrintArg _ANSI_ARGS_((Tcl_Channel  channel,
                      char        *argStr,
                      int          noTruncate));

static void
TraceCode  _ANSI_ARGS_((traceInfo_pt infoPtr,
                        int          level,
                        char        *command,
                        int          argc,
                        char       **argv));

static void
TraceCallBack _ANSI_ARGS_((Tcl_Interp   *interp,
                           traceInfo_pt  infoPtr,
                           int           level,
                           char         *command,
                           int           argc,
                           char        **argv));

static void
CmdTraceRoutine _ANSI_ARGS_((ClientData    clientData,
                             Tcl_Interp   *interp,
                             int           level,
                             char         *command,
                             Tcl_CmdProc  *cmdProc,
                             ClientData    cmdClientData,
                             int           argc,
                             char        **argv));

static int
Tcl_CmdtraceCmd _ANSI_ARGS_((ClientData    clientData,
                             Tcl_Interp   *interp,
                             int           argc,
                             char        **argv));

static void
DebugCleanUp _ANSI_ARGS_((ClientData  clientData,
                          Tcl_Interp *interp));


/*
 *-----------------------------------------------------------------------------
 * TraceDelete --
 *
 *   Delete the trace if active, reseting the structure.
 *-----------------------------------------------------------------------------
 */
static void
TraceDelete (interp, infoPtr)
    Tcl_Interp   *interp;
    traceInfo_pt  infoPtr;
{
    if (infoPtr->traceHolder != NULL) {
        Tcl_DeleteTrace (interp, infoPtr->traceHolder);
        infoPtr->depth = 0;
        infoPtr->traceHolder = NULL;
        if (infoPtr->callback != NULL) {
            ckfree (infoPtr->callback);
            infoPtr->callback = NULL;
        }
    }
}

/*
 *-----------------------------------------------------------------------------
 * PrintStr --
 *
 *     Print an string, truncating it to the specified number of characters.
 * If the string contains newlines, \n is substituted.
 *-----------------------------------------------------------------------------
 */
static void
PrintStr (channel, string, numChars, quoted)
    Tcl_Channel  channel;
    char        *string;
    int          numChars;
    int          quoted;
{
    int idx;

    if (quoted) 
        Tcl_Write (channel, "{", 1);
    for (idx = 0; idx < numChars; idx++) {
        if (string [idx] == '\n') {
            Tcl_Write (channel, "\\n", 2);
        } else {
            Tcl_Write (channel, &(string [idx]), 1);
        }
    }
    if (numChars < (int) strlen (string))
        Tcl_Write (channel, "...", 3);
    if (quoted) 
        Tcl_Write (channel, "}", 1);
}

/*
 *-----------------------------------------------------------------------------
 * PrintArg --
 *
 *   Print an argument string, truncating and adding "..." if its longer
 * then ARG_TRUNCATE_SIZE.  If the string contains white spaces, quote
 * it with angle brackets.
 *-----------------------------------------------------------------------------
 */
static void
PrintArg (channel, argStr, noTruncate)
    Tcl_Channel  channel;
    char        *argStr;
    int          noTruncate;
{
    int idx, argLen, printLen;
    int quoted;

    argLen = strlen (argStr);
    printLen = argLen;
    if ((!noTruncate) && (printLen > ARG_TRUNCATE_SIZE))
        printLen = ARG_TRUNCATE_SIZE;

    quoted = (printLen == 0);

    for (idx = 0; idx < printLen; idx++)
        if (ISSPACE (argStr [idx])) {
            quoted = TRUE;
            break;
        }

    PrintStr (channel, argStr, printLen, quoted);
}

/*
 *-----------------------------------------------------------------------------
 * TraceCode --
 *
 *   Print out a trace of a code line.  Level is used for indenting
 * and marking lines and may be eval or procedure level.
 *-----------------------------------------------------------------------------
 */
static void
TraceCode (infoPtr, level, command, argc, argv)
    traceInfo_pt infoPtr;
    int          level;
    char        *command;
    int          argc;
    char       **argv;
{
    int idx, cmdLen, printLen;
    char buf [32];

    sprintf (buf, "%2d:", level);
    TclX_WriteStr (infoPtr->channel, buf); 

    if (level > 20)
        level = 20;
    for (idx = 0; idx < level; idx++) 
        Tcl_Write (infoPtr->channel, "  ", 2);

    if (infoPtr->noEval) {
        cmdLen = printLen = strlen (command);
        if ((!infoPtr->noTruncate) && (printLen > CMD_TRUNCATE_SIZE))
            printLen = CMD_TRUNCATE_SIZE;

        PrintStr (infoPtr->channel, command, printLen, FALSE);
      } else {
          for (idx = 0; idx < argc; idx++) {
              if (idx > 0)
                  Tcl_Write (infoPtr->channel, " ", 1);
              PrintArg (infoPtr->channel,
                        argv [idx], 
                        infoPtr->noTruncate);
          }
    }

    TclX_WriteNL (infoPtr->channel);
    Tcl_Flush (infoPtr->channel);
}

/*
 *-----------------------------------------------------------------------------
 * TraceCallBack --
 *
 *   Build and call a callback for the command that was just executed. The
 * following arguments are appended to the command:
 *   1) command - A string containing the text of the command, before any
 *      argument substitution.
 *   2) argv - A list of the final argument information that will be passed to
 *     the command after command, variable, and backslash substitution.
 *   3) evalLevel - The Tcl_Eval level.
 *   4) procLevel - The procedure level.
 * The code should allow for additional substitution of arguments in future
 * versions (such as a procedure with args as the last argument).  The value
 * of result, errorInfo and errorCode are preserved.  All other state must be
 * preserved by the procedure.  A error will result in information being dumped
 * to stderr and the trace deleted.
 *-----------------------------------------------------------------------------
 */
static void
TraceCallBack (interp, infoPtr, level, command, argc, argv)
    Tcl_Interp   *interp;
    traceInfo_pt  infoPtr;
    int           level;
    char         *command;
    int           argc;
    char        **argv;
{
    Interp       *iPtr = (Interp *) interp;
    Tcl_DString   callback, resultBuf, errorInfoBuf, errorCodeBuf;
    char         *cmdList, *errorInfo, *errorCode;
    char          numBuf [32];

    Tcl_DStringInit (&callback);
    Tcl_DStringInit (&resultBuf);
    Tcl_DStringInit (&errorInfoBuf);
    Tcl_DStringInit (&errorCodeBuf);

    /*
     * Build the command to evaluate.
     */
    Tcl_DStringAppend (&callback, infoPtr->callback, -1);

    Tcl_DStringStartSublist (&callback);
    Tcl_DStringAppendElement (&callback, command);
    Tcl_DStringEndSublist (&callback);

    Tcl_DStringStartSublist (&callback);
    cmdList = Tcl_Merge (argc, argv);
    Tcl_DStringAppendElement (&callback, cmdList);
    ckfree (cmdList);
    Tcl_DStringEndSublist (&callback);

    sprintf (numBuf, "%d", level);
    Tcl_DStringAppendElement (&callback, numBuf);

    sprintf (numBuf, "%d",  ((iPtr->varFramePtr == NULL) ? 0 : 
             iPtr->varFramePtr->level));
    Tcl_DStringAppendElement (&callback, numBuf);

    /*
     * Save errorInfo and errorCode.
     */
    Tcl_DStringGetResult (interp, &resultBuf);
    errorInfo = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
    if (errorInfo != NULL)
        Tcl_DStringAppend (&errorInfoBuf, errorInfo, -1);
    errorCode = Tcl_GetVar (interp, "errorCode", TCL_GLOBAL_ONLY);
    if (errorCode != NULL)
        Tcl_DStringAppend (&errorCodeBuf, errorCode, -1);

    /*
     * Evaluate the command.  If an error occurs, dump something to stderr
     * and delete the trace.  There is no way to return an error at this
     * point.
     */
    if (Tcl_Eval (interp, Tcl_DStringValue (&callback)) == TCL_ERROR) {
        Tcl_Channel stderrChan;

        Tcl_AddErrorInfo (interp, "\n    (\"cmdtrace\" callback command)");
        
        stderrChan = Tcl_GetStdChannel (TCL_STDERR);
        if (stderrChan != NULL) {
            TclX_WriteStr (stderrChan, 
                           "cmdtrace callback command error: errorCode = ");
            TclX_WriteStr (stderrChan,
                           Tcl_GetVar (interp, "errorCode", TCL_GLOBAL_ONLY));
            TclX_WriteNL (stderrChan);
            TclX_WriteStr (stderrChan,
                           Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
            TclX_WriteNL (stderrChan);
            Tcl_Flush (stderrChan);
        }
        TraceDelete (interp, infoPtr);
    }

    /*
     * Restore errorInfo and errorCode.
     */
    if (errorInfo != NULL)
        Tcl_SetVar (interp, "errorInfo", Tcl_DStringValue (&errorInfoBuf),
                    TCL_GLOBAL_ONLY);
    if (errorCode != NULL)
        Tcl_SetVar (interp, "errorCode", Tcl_DStringValue (&errorCodeBuf),
                    TCL_GLOBAL_ONLY);
    Tcl_DStringResult (interp, &resultBuf);

    Tcl_DStringFree (&callback);
    Tcl_DStringFree (&resultBuf);
    Tcl_DStringFree (&errorInfoBuf);
    Tcl_DStringFree (&errorCodeBuf);
}

/*
 *-----------------------------------------------------------------------------
 * CmdTraceRoutine --
 *
 *  Routine called by Tcl_Eval to trace a command.
 *-----------------------------------------------------------------------------
 */
static void
CmdTraceRoutine (clientData, interp, level, command, cmdProc, cmdClientData, 
                 argc, argv)
    ClientData    clientData;
    Tcl_Interp   *interp;
    int           level;
    char         *command;
    Tcl_CmdProc  *cmdProc;
    ClientData    cmdClientData;
    int           argc;
    char        **argv;
{
    Interp       *iPtr = (Interp *) interp;
    traceInfo_pt  infoPtr = (traceInfo_pt) clientData;
    int           procLevel;

    if (infoPtr->inTrace)
        return;
    infoPtr->inTrace = TRUE;

    if (infoPtr->procCalls) {
        if (TclFindProc (iPtr, argv [0]) != NULL) {
            if (infoPtr->callback != NULL) {
                TraceCallBack (interp, infoPtr, level, command, argc, argv);
            } else {
                procLevel = (iPtr->varFramePtr == NULL) ? 0 : 
                    iPtr->varFramePtr->level;
                TraceCode (infoPtr, procLevel, command, argc, argv);
            }
        }
    } else {
        if (infoPtr->callback != NULL) {
            TraceCallBack (interp, infoPtr, level, command, argc, argv);
        } else {
            TraceCode (infoPtr, level, command, argc, argv);
        }
    }
    infoPtr->inTrace = FALSE;
}

/*
 *-----------------------------------------------------------------------------
 * Tcl_CmdtraceCmd --
 *
 * Implements the TCL trace command:
 *     cmdtrace level|on ?noeval? ?notruncate? ?procs? ?fileid? ?command cmd?
 *     cmdtrace off
 *     cmdtrace depth
 *-----------------------------------------------------------------------------
 */
static int
Tcl_CmdtraceCmd (clientData, interp, argc, argv)
    ClientData    clientData;
    Tcl_Interp   *interp;
    int           argc;
    char        **argv;
{
    traceInfo_pt  infoPtr = (traceInfo_pt) clientData;
    int           idx;
    char         *fileHandle, *callback;

    if (argc < 2)
        goto argumentError;

    /*
     * Handle `depth' sub-command.
     */
    if (STREQU (argv[1], "depth")) {
        if (argc != 2)
            goto argumentError;
        sprintf(interp->result, "%d", infoPtr->depth);
        return TCL_OK;
    }

    /*
     * If a trace is in progress, delete it now.
     */
    TraceDelete (interp, infoPtr);

    /*
     * Handle off sub-command.
     */
    if (STREQU (argv[1], "off")) {
        if (argc != 2)
            goto argumentError;
        return TCL_OK;
    }

    infoPtr->noEval     = FALSE;
    infoPtr->noTruncate = FALSE;
    infoPtr->procCalls  = FALSE;
    infoPtr->channel    = NULL;
    fileHandle          = NULL;
    callback            = NULL;

    for (idx = 2; idx < argc; idx++) {
        if (STREQU (argv[idx], "notruncate")) {
            if (infoPtr->noTruncate)
                goto argumentError;
            infoPtr->noTruncate = TRUE;
            continue;
        }
        if (STREQU (argv[idx], "noeval")) {
            if (infoPtr->noEval)
                goto argumentError;
            infoPtr->noEval = TRUE;
            continue;
        }
        if (STREQU (argv[idx], "procs")) {
            if (infoPtr->procCalls)
                goto argumentError;
            infoPtr->procCalls = TRUE;
            continue;
        }
        if (STRNEQU (argv [idx], "std", 3) || 
                STRNEQU (argv [idx], "file", 4)) {
            if (fileHandle != NULL)
                goto argumentError;
            if (callback != NULL)
                goto mixCommandAndFile;
            fileHandle = argv [idx];
            continue;
        }
        if (STREQU (argv [idx], "command")) {
            if (callback != NULL)
                goto argumentError;
            if (fileHandle != NULL)
                goto mixCommandAndFile;
            if (idx == argc - 1)
                goto missingCommand;
            callback = argv [++idx];
            continue;
        }
        goto invalidOption;
    }

    if (STREQU (argv[1], "on")) {
        infoPtr->depth = MAXINT;
    } else {
        if (Tcl_GetInt (interp, argv[1], &(infoPtr->depth)) != TCL_OK)
            return TCL_ERROR;
    }

    if (callback != NULL) {
        infoPtr->callback = ckstrdup (callback);
    } else {
        if (fileHandle == NULL)
            fileHandle = "stdout";
        infoPtr->channel = TclX_GetOpenChannel (interp,
                                                fileHandle,
                                                TCL_WRITABLE);
        if (infoPtr->channel == NULL)
            return TCL_ERROR;
    }
    infoPtr->traceHolder = Tcl_CreateTrace (interp,
                                            infoPtr->depth,
                                            CmdTraceRoutine,
                                            (ClientData) infoPtr);
    return TCL_OK;

  argumentError:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                      " level | on ?noeval? ?notruncate? ?procs?",
                      "?fileid? ?command cmd? | off | depth", (char *) NULL);
    return TCL_ERROR;

  missingCommand:
    Tcl_AppendResult (interp, "command option requires an argument",
                      (char *) NULL);
    return TCL_ERROR;

  mixCommandAndFile:
    Tcl_AppendResult (interp, "can not specify both the command option and ",
                      "a file handle", (char *) NULL);
    return TCL_ERROR;

  invalidOption:
    Tcl_AppendResult (interp, "invalid option: expected ",
                      "one of \"noeval\", \"notruncate\", \"procs\", ",
                      "\"command\", or a file id", (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * DebugCleanUp --
 *
 *  Release the debug data area when the interpreter is deleted.
 *-----------------------------------------------------------------------------
 */
static void
DebugCleanUp (clientData, interp)
    ClientData  clientData;
    Tcl_Interp *interp;
{
    traceInfo_pt infoPtr = (traceInfo_pt) clientData;

    TraceDelete (interp, infoPtr);
    ckfree ((char *) infoPtr);
}

/*
 *-----------------------------------------------------------------------------
 * Tcl_InitDebug --
 *
 *  Initialize the TCL debugging commands.
 *-----------------------------------------------------------------------------
 */
void
Tcl_InitDebug (interp)
    Tcl_Interp *interp;
{
    traceInfo_pt infoPtr;

    infoPtr = (traceInfo_pt) ckalloc (sizeof (traceInfo_t));

    infoPtr->interp      = interp;
    infoPtr->traceHolder = NULL;
    infoPtr->inTrace     = FALSE;
    infoPtr->noEval      = FALSE;
    infoPtr->noTruncate  = FALSE;
    infoPtr->procCalls   = FALSE;
    infoPtr->depth       = 0;
    infoPtr->callback    = NULL;
    infoPtr->channel     = NULL;

    Tcl_CallWhenDeleted (interp, DebugCleanUp, (ClientData) infoPtr);

    Tcl_CreateCommand (interp, "cmdtrace", Tcl_CmdtraceCmd, 
                       (ClientData) infoPtr, (Tcl_CmdDeleteProc*) NULL);
}


