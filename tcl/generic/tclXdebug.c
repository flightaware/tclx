/*
 * tclXdebug.c --
 *
 * Tcl command execution trace command.
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
 * $Id: tclXdebug.c,v 2.3 1993/04/03 23:23:43 markd Exp markd $
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
    int         noEval;
    int         noTruncate;
    int         procCalls;
    int         depth;
    FILE       *filePtr;          /* File to output trace to. */
    } traceInfo_t, *traceInfo_pt;

/*
 * Prototypes of internal functions.
 */
void
PrintStr _ANSI_ARGS_((FILE *filePtr,
                      char *string,
                      int   numChars));

void
PrintArg _ANSI_ARGS_((FILE *filePtr,
                      char *argStr,
                      int   noTruncate));

void
TraceCode  _ANSI_ARGS_((traceInfo_pt traceInfoPtr,
                        int          level,
                        char        *command,
                        int          argc,
                        char       **argv));

void
CmdTraceRoutine _ANSI_ARGS_((ClientData    clientData,
                             Tcl_Interp   *interp,
                             int           level,
                             char         *command,
                             Tcl_CmdProc  *cmdProc,
                             ClientData    cmdClientData,
                             int           argc,
                             char        **argv));

void
DebugCleanUp _ANSI_ARGS_((ClientData  clientData,
                          Tcl_Interp *interp));


/*
 *-----------------------------------------------------------------------------
 * PrintStr --
 *
 *     Print an string, truncating it to the specified number of characters.
 * If the string contains newlines, \n is substituted.
 *-----------------------------------------------------------------------------
 */
static void
PrintStr (filePtr, string, numChars)
    FILE *filePtr;
    char *string;
    int   numChars;
{
    int idx;

    for (idx = 0; idx < numChars; idx++) {
        if (string [idx] == '\n') {
           putc ('\\', filePtr);
           putc ('n', filePtr);
        } else
           putc (string [idx], filePtr);
    }
    if (numChars < strlen (string))
        fprintf (filePtr, "...");
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
PrintArg (filePtr, argStr, noTruncate)
    FILE *filePtr;
    char *argStr;
    int   noTruncate;
{
    int idx, argLen, printLen;
    int quote_it;

    argLen = strlen (argStr);
    printLen = argLen;
    if ((!noTruncate) && (printLen > ARG_TRUNCATE_SIZE))
        printLen = ARG_TRUNCATE_SIZE;

    quote_it = (printLen == 0);

    for (idx = 0; idx < printLen; idx++)
        if (isspace (argStr [idx])) {
            quote_it = TRUE;
            break;
        }

    if (quote_it) 
        putc ('{', filePtr);
    PrintStr (filePtr, argStr, printLen);
    if (quote_it) 
        putc ('}', filePtr);
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
TraceCode (traceInfoPtr, level, command, argc, argv)
    traceInfo_pt traceInfoPtr;
    int          level;
    char        *command;
    int          argc;
    char       **argv;
{
    int idx, cmdLen, printLen;

    fprintf (traceInfoPtr->filePtr, "%2d:", level);

    if (level > 20)
        level = 20;
    for (idx = 0; idx < level; idx++) 
        fprintf (traceInfoPtr->filePtr, "  ");

    if (traceInfoPtr->noEval) {
        cmdLen = printLen = strlen (command);
        if ((!traceInfoPtr->noTruncate) && (printLen > CMD_TRUNCATE_SIZE))
            printLen = CMD_TRUNCATE_SIZE;

        PrintStr (traceInfoPtr->filePtr, command, printLen);
      } else {
          for (idx = 0; idx < argc; idx++) {
              if (idx > 0)
                  putc (' ', traceInfoPtr->filePtr);
              PrintArg (traceInfoPtr->filePtr, argv[idx], 
                        traceInfoPtr->noTruncate);
          }
    }

    putc ('\n', traceInfoPtr->filePtr);
    fflush (traceInfoPtr->filePtr);
  
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
    traceInfo_pt  traceInfoPtr = (traceInfo_pt) clientData;
    int           procLevel;

    if (!traceInfoPtr->procCalls) {
        TraceCode (traceInfoPtr, level, command, argc, argv);
    } else {
        if (TclFindProc (iPtr, argv [0]) != NULL) {
            procLevel = (iPtr->varFramePtr == NULL) ? 0 : 
                        iPtr->varFramePtr->level;
            TraceCode (traceInfoPtr, procLevel, command, argc, argv);
        }
    }
}

/*
 *-----------------------------------------------------------------------------
 * Tcl_CmdtraceCmd --
 *
 * Implements the TCL trace command:
 *     cmdtrace level|on ?noeval? ?notruncate? ?procs? ?fileid?
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
    Interp       *iPtr = (Interp *) interp;
    traceInfo_pt  infoPtr = (traceInfo_pt) clientData;
    int           idx;
    char         *fileHandle;

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
    if (infoPtr->traceHolder != NULL) {
        Tcl_DeleteTrace(interp, infoPtr->traceHolder);
        infoPtr->depth = 0;
        infoPtr->traceHolder = NULL;
    }

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
    infoPtr->filePtr    = stdout;
    fileHandle          = NULL;

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
            fileHandle = argv [idx];
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
    if (fileHandle != NULL) {
        FILE *filePtr;

        if (Tcl_GetOpenFile (interp, fileHandle, 
                             TRUE,   /* Write access */
                             TRUE,   /* Check access */
                             &filePtr) != TCL_OK)
	    return TCL_ERROR;
        infoPtr->filePtr = filePtr;
    }
    
    infoPtr->traceHolder = Tcl_CreateTrace (interp, infoPtr->depth,
                                            CmdTraceRoutine,
                                            (ClientData) infoPtr);
    return TCL_OK;

argumentError:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                      " level | on ?noeval? ?notruncate? ?procs?",
                      "?fileid? | off | depth", (char *) NULL);
    return TCL_ERROR;

invalidOption:
    Tcl_AppendResult (interp, "invalid option: expected ",
                      "one of \"noeval\", \"notruncate\", \"procs\", ",
                      "or a file id", (char *) NULL);
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

    if (infoPtr->traceHolder != NULL)
        Tcl_DeleteTrace (infoPtr->interp, infoPtr->traceHolder);
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
    infoPtr->noEval      = FALSE;
    infoPtr->noTruncate  = FALSE;
    infoPtr->procCalls   = FALSE;
    infoPtr->depth       = 0;

    Tcl_CallWhenDeleted (interp, DebugCleanUp, (ClientData) infoPtr);

    Tcl_CreateCommand (interp, "cmdtrace", Tcl_CmdtraceCmd, 
                       (ClientData) infoPtr, (void (*)()) NULL);
}


