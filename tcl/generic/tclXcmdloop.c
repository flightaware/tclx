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
 * $Id: tclXcmdloop.c,v 2.5 1993/07/12 05:26:12 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
IsSetVarCmd _ANSI_ARGS_((char  *command));

static void
OutFlush _ANSI_ARGS_((FILE *filePtr));

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

    if ((!STRNEQU (command, "set", 3)) || (!isspace (command [3])))
        return FALSE;  /* Quick check */

    /*
     * Loop to count the words in the command.
     */
    wordCnt = 0;
    nextPtr = command;
    while (*nextPtr != '\0') {
        nextPtr = TclWordEnd (nextPtr, FALSE);
        nextPtr++;  /* Character after the word */
        while ((*nextPtr != '\0') && (isspace (*nextPtr)))
            nextPtr++;
        wordCnt++;
    }
    return wordCnt > 2 ? TRUE : FALSE;
}

/*
 *-----------------------------------------------------------------------------
 *
 * OutFlush --
 *
 *   Flush a stdio file and check for errors.  Keeps us from going on if
 * we are really not outputting anything.
 *-----------------------------------------------------------------------------
 */
static void
OutFlush (filePtr)
    FILE *filePtr;
{
    int stat;

    stat = fflush (filePtr);
    if (ferror (filePtr)) {
        if (errno != EINTR)
            panic ("command loop: error writing to output file: %s\n",
                   strerror (errno));
        clearerr (filePtr);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_PrintResult --
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
Tcl_PrintResult (interp, intResult, checkCmd)
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
        OutFlush (stdout);
        if (intResult == TCL_ERROR)  
            fputs ("Error: ", stderr);
        else
            fprintf (stderr, "Bad return code (%d): ", intResult);
        fputs (interp->result, stderr);
        fputs ("\n", stderr);
        OutFlush (stderr);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_OutputPrompt --
 *     Outputs a prompt by executing either the command string in
 *     TCLENV(topLevelPromptHook) or TCLENV(downLevelPromptHook).
 *
 *-----------------------------------------------------------------------------
 */
void
Tcl_OutputPrompt (interp, topLevel)
    Tcl_Interp *interp;
    int         topLevel;
{
    char *hookName;
    char *promptHook;
    int   result;
    int   promptDone = FALSE;

    hookName = topLevel ? "topLevelPromptHook" : "downLevelPromptHook";

    promptHook = Tcl_GetVar2 (interp, "TCLENV", hookName, 1);
    if (promptHook != NULL) {
        result = Tcl_Eval (interp, promptHook);
        if (result == TCL_ERROR) {
            fputs ("Error in prompt hook: ", stderr);
            fputs (interp->result, stderr);
            fputs ("\n", stderr);
            Tcl_PrintResult (interp, result, NULL);
        } else {
            fputs (interp->result, stdout);
            promptDone = TRUE;
        }
    } 
    if (!promptDone) {
        if (topLevel)
            fputs ("%", stdout);
        else
            fputs (">", stdout);
    }
    OutFlush (stdout);
    Tcl_ResetResult (interp);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CommandLoop --
 *
 *   Run a Tcl command loop.  The command loop interactively prompts for,
 * reads and executes commands. Two entries in the global array TCLENV
 * contain prompt hooks.  A prompt hook is Tcl code that is executed and
 * its result is used as the prompt string.  The element `topLevelPromptHook'
 * is the hook that generates the main prompt.  The element
 * `downLevelPromptHook' is the hook to generate the prompt for reading
 * continuation lines for incomplete commands.  If a signal occurs while
 * in the command loop, it is reset and ignored.  EOF terminates the loop.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter
 *-----------------------------------------------------------------------------
 */
void
Tcl_CommandLoop (interp)
    Tcl_Interp *interp;
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
        if (tclReceivedSignal) {
            Tcl_ProcessSignals (interp, TCL_OK); 
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
        Tcl_OutputPrompt (interp, topLevel);
        errno = 0;
        if (fgets (inputBuf, sizeof (inputBuf), stdin) == NULL) {
            if (!feof(stdin) && (errno == EINTR)) {
                putchar('\n');
                continue;  /* Next command */
            }
            if (ferror (stdin))
                panic ("command loop: error on input file: %s\n",
                       strerror (errno));
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

        Tcl_PrintResult (interp, result, cmdBuf.string);

        topLevel = TRUE;
        Tcl_DStringFree (&cmdBuf);
    }
endOfFile:
    Tcl_DStringFree (&cmdBuf);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SetPromptVar --
 *     Set one of the prompt hook variables, saving a copy of the old
 *     value, if it exists.
 *
 * Parameters:
 *   o hookVarName (I) - The name of the prompt hook, which is an element
 *     of the TCLENV array.  One of topLevelPromptHook or downLevelPromptHook.
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
        hookValue = Tcl_GetVar2 (interp, "TCLENV", hookVarName, 
                                 TCL_GLOBAL_ONLY);
        if (hookValue != NULL)
            oldHookPtr =  ckstrdup (hookValue);
    }
    if (Tcl_SetVar2 (interp, "TCLENV", hookVarName, newHookValue, 
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
 *       commandloop ?prompt? ?prompt2?
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
                          " ?prompt? ?prompt2?", (char *) NULL);
        return TCL_ERROR;
    }
    if (argc > 1) {
        if (SetPromptVar (interp, "topLevelPromptHook", argv[1],
                          &oldTopLevelHook) != TCL_OK)
            goto exitPoint;
    }
    if (argc > 2) {
        if (SetPromptVar (interp, "downLevelPromptHook", argv[2], 
                          &oldDownLevelHook) != TCL_OK)
            goto exitPoint;
    }

    Tcl_CommandLoop (interp);

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
