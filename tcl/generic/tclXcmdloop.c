/* 
 * tclXcmdloop --
 *
 *   Interactive command loop, C and Tcl callable.
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
 * $Id: tclXcmdloop.c,v 5.3 1996/02/12 18:15:34 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
IsSetVarCmd _ANSI_ARGS_((char  *command));

static int
SetPromptVar _ANSI_ARGS_((Tcl_Interp  *interp,
                          char        *hookVarName,
                          char        *newHookValue,
                          char       **oldHookValuePtr));


/*
 *-----------------------------------------------------------------------------
 * IsSetVarCmd --
 *
 *    Determine if a command is a `set' command that sets a variable
 * (i.e. two arguments).  This routine should only be called if the command
 * returned TCL_OK.  Returns TRUE if it sets a variable, FALSE if its some
 * other command.
 *-----------------------------------------------------------------------------
 */
static int
IsSetVarCmd (command)
    char  *command;
{
    char  *nextPtr;
    int    wordCnt;

    if ((!STRNEQU (command, "set", 3)) || (!ISSPACE (command [3])))
        return FALSE;  /* Quick check */

    /*
     * Loop to count the words in the command.
     */
    wordCnt = 0;
    nextPtr = command;
    while (*nextPtr != '\0') {
        nextPtr = TclWordEnd (nextPtr, FALSE, NULL);
        nextPtr++;  /* Character after the word */
        while ((*nextPtr != '\0') && (ISSPACE (*nextPtr)))
            nextPtr++;
        wordCnt++;
    }
    return wordCnt > 2 ? TRUE : FALSE;
}

/*
 *-----------------------------------------------------------------------------
 *
 * TclX_PrintResult --
 *
 *   Print the result of a Tcl_Eval.  It can optionally not echo "set" commands
 * that successfully set a variable.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter.  Result of command should be
 *     in interp->result.
 *   o intResult (I) - The integer result returned by Tcl_Eval.
 *   o checkCmd (I) - If not NULL and the command was sucessful, check to
 *     set if this is a "set" command setting a variable.  If so, don't echo
 *     the result. 
 *-----------------------------------------------------------------------------
 */
void
TclX_PrintResult (interp, intResult, checkCmd)
    Tcl_Interp *interp;
    int         intResult;
    char       *checkCmd;
{
    Tcl_Channel stdoutChan;

    /*
     * If the command was supplied and it was a successful set of a variable,
     * don't output the result.
     */
    if ((checkCmd != NULL) && (intResult == TCL_OK) && IsSetVarCmd (checkCmd))
        return;

    stdoutChan = Tcl_GetStdChannel (TCL_STDIN);

    if (intResult == TCL_OK) {
        if (stdoutChan == NULL)
            return;
        if (interp->result [0] != '\0') {
            TclX_WriteStr (stdoutChan, interp->result);
            TclX_WriteNL (stdoutChan);
        }
    } else {
        Tcl_Channel stderrChan;
        char        msg [64];

        stderrChan = Tcl_GetStdChannel (TCL_STDERR);
        if (stderrChan == NULL)
            return;
       
        if (stdoutChan != NULL)
            Tcl_Flush (stdoutChan);
        if (intResult == TCL_ERROR) {
            strcpy (msg, "Error: ");
        } else {
            sprintf (msg, "Bad return code (%d): ", intResult);
        }
        TclX_WriteStr (stderrChan, msg);
        TclX_WriteStr (stderrChan, interp->result);
        TclX_WriteNL (stderrChan);
        Tcl_Flush (stderrChan);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * TclX_OutputPrompt --
 *     Outputs a prompt by executing either the command string in
 *     tcl_prompt1 or tcl_prompt2.
 *
 *-----------------------------------------------------------------------------
 */
void
TclX_OutputPrompt (interp, topLevel)
    Tcl_Interp *interp;
    int         topLevel;
{
    char *hookName;
    char *promptHook;
    int result;
    int promptDone = FALSE;
    Tcl_Channel stdoutChan;

    /*
     * If a signal came in, process it.  This prevents signals that are queued
     * from generating prompt hook errors.
     */
    if (Tcl_AsyncReady ()) {
        Tcl_AsyncInvoke (interp, TCL_OK); 
    }

    hookName = topLevel ? "tcl_prompt1" : "tcl_prompt2";

    promptHook = Tcl_GetVar (interp, hookName, 1);
    if (promptHook != NULL) {
        result = Tcl_Eval (interp, promptHook);
        if (result == TCL_ERROR) {
            Tcl_Channel stderrChan = Tcl_GetStdChannel (TCL_STDERR);
            if (stderrChan != NULL) {
                TclX_WriteStr (stderrChan, "Error in prompt hook: ");
                TclX_WriteStr (stderrChan, interp->result);
                TclX_WriteNL (stderrChan);
            }
            TclX_PrintResult (interp, result, NULL);
        } else {
            promptDone = TRUE;
        }
    } 

    stdoutChan = Tcl_GetStdChannel (TCL_STDOUT);
    if (stdoutChan != NULL) {
        if (!promptDone) {
            if (topLevel)
                Tcl_Write (stdoutChan, "%", 1);
            else
                Tcl_Write (stdoutChan, ">", 1);
        }
        Tcl_Flush (stdoutChan);
    }
    Tcl_ResetResult (interp);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CommandLoop --
 *
 *   Run a Tcl command loop.  The command loop interactively prompts for,
 * reads and executes commands. Two global variables, "tcl_prompt1" and
 * "tcl_prompt2" contain prompt hooks.  A prompt hook is Tcl code that is
 * executed and its result is used as the prompt string. If a error generating
 * signal occurs while in the command loop, it is reset and ignored.  EOF
 * terminates the loop.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter
 *   o interactive (I) - If TRUE print prompts and non-error results.
 * Returns:
 *   TCL_OK or TCL_ERROR;
 *-----------------------------------------------------------------------------
 */
int
Tcl_CommandLoop (interp, interactive)
    Tcl_Interp *interp;
    int         interactive;
{
    Tcl_DString cmdBuf;
    int result, topLevel = TRUE;
    Tcl_Channel stdinChan;

    Tcl_DStringInit (&cmdBuf);

    while (TRUE) {
        /*
         * If a signal came in, process it. Drop any pending command
         * if a "error" signal occured since the last time we were
         * through here.
         */
        if (Tcl_AsyncReady ()) {
            Tcl_AsyncInvoke (interp, TCL_OK); 
        }
        if (tclGotErrorSignal) {
            tclGotErrorSignal = FALSE;
            Tcl_DStringFree (&cmdBuf);
            topLevel = TRUE;
        }

        /*
         * Output a prompt and input a command.
         */
        stdinChan = Tcl_GetStdChannel (TCL_STDIN);
        if (stdinChan == NULL)
            goto endOfFile;

        if (interactive) {
            TclX_OutputPrompt (interp, topLevel);
        }
        Tcl_SetErrno (0);
        result = Tcl_Gets (stdinChan, &cmdBuf);

        if (result < 0) {
            if (Tcl_Eof (stdinChan) || Tcl_InputBlocked (stdinChan))
                goto endOfFile;
            if (Tcl_GetErrno () == EINTR) {
                Tcl_Channel stdoutChan = Tcl_GetStdChannel (TCL_STDOUT);
                if (stdoutChan != NULL)
                    TclX_WriteNL (stdoutChan);
                continue;  /* Next command */
            }
            Tcl_AppendResult (interp, "command input error on stdin: ",
                              Tcl_PosixError (interp), (char *) NULL);
            return TCL_ERROR;
        }

        /*
         * Newline was stripped by Tcl_DStringGets, but is needed for
         * command-complete checking, add it back in.  If the command is
         * not complete, get the next line.
         */
        Tcl_DStringAppend (&cmdBuf, "\n", 1);

        if (!Tcl_CommandComplete (cmdBuf.string)) {
            topLevel = FALSE;
            continue;  /* Next line */
        }

        /*
         * Finally have a complete command, go eval it and maybe output the
         * result.
         */
        result = Tcl_RecordAndEval (interp, cmdBuf.string, 0);

        if (interactive || result != TCL_OK)
            TclX_PrintResult (interp, result, cmdBuf.string);

        topLevel = TRUE;
        Tcl_DStringFree (&cmdBuf);
    }
  endOfFile:
    Tcl_DStringFree (&cmdBuf);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SetPromptVar --
 *     Set one of the prompt hook variables, saving a copy of the old
 *     value, if it exists.
 *
 * Parameters:
 *   o hookVarName (I) - The name of the global variable containing prompt
 *     hook.
 *   o newHookValue (I) - The new value for the prompt hook.
 *   o oldHookValuePtr (O) - If not NULL, then a pointer to a copy of the
 *     old prompt value is returned here.  NULL is returned if there was not
 *     old value.  This is a pointer to a malloc-ed string that must be
 *     freed when no longer needed.
 * Result:
 *   TCL_OK if the hook variable was set ok, TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
static int
SetPromptVar (interp, hookVarName, newHookValue, oldHookValuePtr)
    Tcl_Interp *interp;
    char       *hookVarName;
    char       *newHookValue;
    char      **oldHookValuePtr;
{
    char *hookValue;    
    char *oldHookPtr = NULL;

    if (oldHookValuePtr != NULL) {
        hookValue = Tcl_GetVar (interp, hookVarName, TCL_GLOBAL_ONLY);
        if (hookValue != NULL)
            oldHookPtr =  ckstrdup (hookValue);
    }
    if (Tcl_SetVar (interp, hookVarName, newHookValue, 
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
        if (oldHookPtr != NULL)
            ckfree (oldHookPtr);
        return TCL_ERROR;
    }    
    if (oldHookValuePtr != NULL)
        *oldHookValuePtr = oldHookPtr;
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CommandloopCmd --
 *     Implements the TCL commandloop command:
 *       commandloop ?prompt1? ?prompt2?
 *
 * Results:
 *     Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_CommandloopCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    char *oldTopLevelHook  = NULL;
    char *oldDownLevelHook = NULL;
    int   result = TCL_ERROR;

    if (argc > 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0],
                          " ?prompt1? ?prompt2?", (char *) NULL);
        return TCL_ERROR;
    }
    if (argc > 1) {
        if (SetPromptVar (interp, "tcl_prompt1", argv[1],
                          &oldTopLevelHook) != TCL_OK)
            goto exitPoint;
    }
    if (argc > 2) {
        if (SetPromptVar (interp, "tcl_prompt2", argv[2], 
                          &oldDownLevelHook) != TCL_OK)
            goto exitPoint;
    }

    Tcl_CommandLoop (interp, TRUE);

    if (oldTopLevelHook != NULL)
        SetPromptVar (interp, "topLevelPromptHook", oldTopLevelHook, NULL);
    if (oldDownLevelHook != NULL)
        SetPromptVar (interp, "downLevelPromptHook", oldDownLevelHook, NULL);
        
    result = TCL_OK;
  exitPoint:
    if (oldTopLevelHook != NULL)
        ckfree (oldTopLevelHook);
    if (oldDownLevelHook != NULL)
        ckfree (oldDownLevelHook);
    return result;
}
