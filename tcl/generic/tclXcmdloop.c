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
 * $Id: tclXcmdloop.c,v 2.3 1993/04/03 23:23:43 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
int
IsSetVarCmd _ANSI_ARGS_((Tcl_Interp *interp,
                         char       *command));

void
OutFlush _ANSI_ARGS_((FILE *filePtr));

void
Tcl_PrintResult _ANSI_ARGS_((FILE   *fp,
                             int     returnval,
                             char   *resultText));

void
OutputPrompt _ANSI_ARGS_((Tcl_Interp *interp,
                          FILE       *outFP,
                          int         topLevel));

int
SetPromptVar _ANSI_ARGS_((Tcl_Interp  *interp,
                          char        *hookVarName,
                          char        *newHookValue,
                          char       **oldHookValuePtr));


/*
 *-----------------------------------------------------------------------------
 * IsSetVarCmd --
 *
 *      Determine if the current command is a `set' command that sets a
 * variable (i.e. two arguments).  This routine should only be called if the
 * command returned TCL_OK.  Returns TRUE if it sets a variable, FALSE if its
 * some other command.
 *-----------------------------------------------------------------------------
 */
static int
IsSetVarCmd (interp, command)
    Tcl_Interp *interp;
    char       *command;
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
 *   Flush a stdio file and check for errors.
 *
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
 *      Print a Tcl result
 *
 * Results:
 *
 *      Takes an open file pointer, a return value and some result
 *      text.  Prints the result text if the return value is TCL_OK,
 *      prints "Error:" and the result text if it's TCL_ERROR,
 *      else prints "Bad return code:" and the result text.
 *
 *-----------------------------------------------------------------------------
 */
static void
Tcl_PrintResult (fp, returnval, resultText)
    FILE   *fp;
    int     returnval;
    char   *resultText;
{

    if (returnval == TCL_OK) {
        if (resultText [0] != '\0') {
            fputs (resultText, fp);
            fputs ("\n", fp);
        }
    } else {
        OutFlush (fp);
        fputs ((returnval == TCL_ERROR) ? "Error" : "Bad return code", stderr);
        fputs (": ", stderr);
        fputs (resultText, stderr);
        fputs ("\n", stderr);
        OutFlush (stderr);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * OutputPromp --
 *     Outputs a prompt by executing either the command string in
 *     TCLENV(topLevelPromptHook) or TCLENV(downLevelPromptHook).
 *
 *-----------------------------------------------------------------------------
 */
static void
OutputPrompt (interp, outFP, topLevel)
    Tcl_Interp *interp;
    FILE       *outFP;
    int         topLevel;
{
    char *hookName;
    char *promptHook;
    int   result;
    int   promptDone = FALSE;

    hookName = topLevel ? "topLevelPromptHook"
                        : "downLevelPromptHook";

    promptHook = Tcl_GetVar2 (interp, "TCLENV", hookName, 1);
    if ((promptHook != NULL) && (promptHook [0] != '\0')) {
        result = Tcl_Eval (interp, promptHook, 0, (char **)NULL);
        if (!((result == TCL_OK) || (result == TCL_RETURN))) {
            fputs ("Error in prompt hook: ", stderr);
            fputs (interp->result, stderr);
            fputs ("\n", stderr);
            Tcl_PrintResult (outFP, result, interp->result);
        } else {
            fputs (interp->result, outFP);
            promptDone = TRUE;
        }
    } 
    if (!promptDone) {
        if (topLevel)
            fputs ("%", outFP);
        else
            fputs (">", outFP);
    }
    OutFlush (outFP);

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
 *   o inFile (I) - The file to read commands from.
 *   o outFile (I) - The file to write the prompts to. 
 *   o options (I) - Currently unused.
 *-----------------------------------------------------------------------------
 */
void
Tcl_CommandLoop (interp, inFile, outFile, options)
    Tcl_Interp *interp;
    FILE       *inFile;
    FILE       *outFile;
    unsigned    options;
{
    Tcl_DString cmdBuf;
    char        inputBuf [128];
    int         topLevel = TRUE;
    int         result;

    Tcl_DStringInit (&cmdBuf);

    while (TRUE) {
        /*
         * If a signal came in, process it and drop any pending command.
         */
        if (tclReceivedSignal) {
            Tcl_CheckForSignal (interp, TCL_OK); 
            Tcl_DStringFree (&cmdBuf);
            topLevel = TRUE;
        }
        /*
         * Output a prompt and input a command.
         */
        clearerr (inFile);
        clearerr (outFile);
        OutputPrompt (interp, outFile, topLevel);
        errno = 0;
        if (fgets (inputBuf, sizeof (inputBuf), inFile) == NULL) {
            if (!feof(inFile) && (errno == EINTR)) {
                putchar('\n');
                continue;  /* Next command */
            }
            if (ferror (inFile))
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
        if (result != TCL_OK || !IsSetVarCmd (interp, cmdBuf.string))
            Tcl_PrintResult (outFile, result, interp->result);
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

    Tcl_CommandLoop (interp, stdin, stdout, 0);

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
