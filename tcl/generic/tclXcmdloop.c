/* 
 * tclXcmdloop --
 *
 *   Interactive command loop, C and Tcl callable.
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
 * $Id: tclXcmdloop.c,v 3.0 1993/11/19 06:58:26 markd Rel markd $
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
 *   Print the result of a Tcl.  It can optionally not echo "set" commands
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
    /*
     * If the command was supplied and it was a successful set of a variable,
     * don't output the result.
     */
    if ((checkCmd != NULL) && (intResult == TCL_OK) && IsSetVarCmd (checkCmd))
        return;

    if (intResult == TCL_OK) {
        if (interp->result [0] != '\0') {
            fputs (interp->result, stdout);
            fputs ("\n", stdout);
        }
    } else {
        fflush (stdout);
        if (intResult == TCL_ERROR)  
            fputs ("Error: ", stderr);
        else
            fprintf (stderr, "Bad return code (%d): ", intResult);
        fputs (interp->result, stderr);
        fputs ("\n", stderr);
        fflush (stderr);
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
    int   result;
    int   promptDone = FALSE;

    /*
     * If a signal came in, process it.  This prevents signals that are queued
     * from generating prompt hook errors.
     */
    if (tcl_AsyncReady) {
        Tcl_AsyncInvoke (interp, TCL_OK); 
    }

    hookName = topLevel ? "tcl_prompt1" : "tcl_prompt2";

    promptHook = Tcl_GetVar (interp, hookName, 1);
    if (promptHook != NULL) {
        result = Tcl_Eval (interp, promptHook);
        if (result == TCL_ERROR) {
            fputs ("Error in prompt hook: ", stderr);
            fputs (interp->result, stderr);
            fputs ("\n", stderr);
            TclX_PrintResult (interp, result, NULL);
        } else {
            promptDone = TRUE;
        }
    } 
    if (!promptDone) {
        if (topLevel)
            fputs ("%", stdout);
        else
            fputs (">", stdout);
    }
    fflush (stdout);
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
    char        inputBuf [128];
    int         topLevel = TRUE;
    int         result;

    Tcl_DStringInit (&cmdBuf);

    while (TRUE) {
        /*
         * If a signal came in, process it. Drop any pending command
         * if a "error" signal occured since the last time we were
         * through here.
         */
        if (tcl_AsyncReady) {
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
        clearerr (stdin);
        clearerr (stdout);
        if (interactive)
            TclX_OutputPrompt (interp, topLevel);
        errno = 0;
        if (fgets (inputBuf, sizeof (inputBuf), stdin) == NULL) {
            if (!feof(stdin) && (errno == EINTR)) {
                putchar('\n');
                continue;  /* Next command */
            }
            if (ferror (stdin)) {
                Tcl_AppendResult (interp, "command input error on stdin: ",
                                  Tcl_PosixError (interp), (char *) NULL);
                return TCL_ERROR;
            }
            goto endOfFile;
        }
        Tcl_DStringAppend (&cmdBuf, inputBuf, -1);

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
