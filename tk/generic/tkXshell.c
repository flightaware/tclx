/*
 * tkXshell.c
 *
 * Version of Tk main modified for TclX to support SIGINT and use some of
 * the TclX utility procedures.
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
 * $Id: tkXshell.c,v 5.9 1996/03/11 05:43:59 markd Exp $
 *-----------------------------------------------------------------------------
 */
/* 
 * tkMain.c --
 *
 *	This file contains a generic main program for Tk-based applications.
 *	It can be used as-is for many applications, just by supplying a
 *	different appInitProc procedure for each specific application.
 *	Or, it can be used as a template for creating new main programs
 *	for Tk applications.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tkMain.c 1.145 96/03/08 17:13:44
 */

#include <ctype.h>
#include <stdio.h>
#include <tclExtdInt.h>
#include <tk.h>

/*
 * Declarations for various library procedures and variables (don't want
 * to include tkInt.h or tkPort.h here, because people might copy this
 * file out of the Tk source directory to make their own modified versions).
 * Note: don't declare "exit" here even though a declaration is really
 * needed, because it will conflict with a declaration elsewhere on
 * some systems.
 */

extern char *		strrchr _ANSI_ARGS_((CONST char *string, int c));

/*
 * Global variables used by the main program:
 */

static Tcl_Interp *interp;	/* Interpreter for this application. */
static Tcl_DString command;	/* Used to assemble lines of terminal input
				 * into Tcl commands. */
static Tcl_DString line;	/* Used to read the next line from the
                                 * terminal input. */
static int tty;			/* Non-zero means standard input is a
				 * terminal-like device.  Zero means it's
				 * a file. */
static int gotPartial = 0;


/*
 * Forward declarations for procedures defined later in this file.
 */

static void		StdinProc _ANSI_ARGS_((ClientData clientData,
			    int mask));
static void		SignalProc _ANSI_ARGS_((int  signalNum));


/*
 *----------------------------------------------------------------------
 *
 * TkX_Main --
 *
 *	Main program for Wish and most other Tk-based applications.
 *
 * Results:
 *	None. This procedure never returns (it exits the process when
 *	it's done.
 *
 * Side effects:
 *	This procedure initializes the Tk world and then starts
 *	interpreting commands;  almost anything could happen, depending
 *	on the script being interpreted.
 *
 *----------------------------------------------------------------------
 */

void
TkX_Main(argc, argv, appInitProc)
    int argc;				/* Number of arguments. */
    char **argv;			/* Array of argument strings. */
    Tcl_AppInitProc *appInitProc;	/* Application-specific initialization
					 * procedure to call after most
					 * initialization but before starting
					 * to execute commands. */
{
    char *args, *msg, *fileName;
    char buf[20];
    int code;
    size_t length;
    Tcl_Channel inChannel, outChannel, errChannel, chan;

    TclX_SetAppInfo (TRUE,
                     "wishx",
                     "Extended Wish",
                     TKX_VERSION,
                     TCLX_PATCHLEVEL);

    Tcl_FindExecutable(argv[0]);
    interp = Tcl_CreateInterp();
#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(interp);
#endif

    /*
     * Parse command-line arguments.  A leading "-file" argument is
     * ignored (a historical relic from the distant past).  If the
     * next argument doesn't start with a "-" then strip it off and
     * use it as the name of a script file to process.
     */

    fileName = NULL;
    if (argc > 1) {
	length = strlen(argv[1]);
	if ((length >= 2) && (strncmp(argv[1], "-file", length) == 0)) {
	    argc--;
	    argv++;
	}
    }
    if ((argc > 1) && (argv[1][0] != '-')) {
	fileName = argv[1];
	argc--;
	argv++;
    }

    /*
     * Make command-line arguments available in the Tcl variables "argc"
     * and "argv".
     */

    args = Tcl_Merge(argc-1, argv+1);
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    ckfree(args);
    sprintf(buf, "%d", argc-1);
    Tcl_SetVar(interp, "argc", buf, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "argv0", (fileName != NULL) ? fileName : argv[0],
	    TCL_GLOBAL_ONLY);

    /*
     * Set the "tcl_interactive" variable.
     */

    /*
     * For now, under Windows, we assume we are not running as a console mode
     * app, so we need to use the GUI console.  In order to enable this, we
     * always claim to be running on a tty.  This probably isn't the right
     * way to do it.
     */

#ifdef __WIN32__
    tty = 1;
#else
    tty = isatty(0);
#endif
    Tcl_SetVar(interp, "tcl_interactive",
	    ((fileName == NULL) && tty) ? "1" : "0", TCL_GLOBAL_ONLY);

    if ((fileName == NULL) && tty)
        Tcl_SetupSigInt ();

    /*
     * Invoke application-specific initialization.
     */

    if ((*appInitProc)(interp) != TCL_OK) {
        TclX_ErrorExit (interp, 255);
    }

    /*
     * Invoke the script specified on the command line, if any.
     */

    if (fileName != NULL) {
	code = TclX_Eval (interp,
                          TCLX_EVAL_GLOBAL | TCLX_EVAL_FILE |
                          TCLX_EVAL_ERR_HANDLER,
                          fileName);
	if (code != TCL_OK) {
	    goto error;
	}
	tty = 0;
    } else {

	/*
	 * Commands will come from standard input, so set up an event
	 * handler for standard input.  Evaluate the .rc file, if one
	 * has been specified, set up an event handler for standard
	 * input, and print a prompt if the input device is a terminal.
	 */

        TclX_EvalRCFile (interp);

	/*
	 * Establish a channel handler for stdin.
	 */

        tclErrorSignalProc = SignalProc;
	inChannel = Tcl_GetStdChannel(TCL_STDIN);
	if (inChannel) {
	    Tcl_CreateChannelHandler(inChannel, TCL_READABLE, StdinProc,
		    (ClientData) inChannel);
	}
	if (tty) {
	    TclX_OutputPrompt (interp, 1);
	}
    }

    tclSignalBackgroundError = Tk_BackgroundError;
    outChannel = Tcl_GetStdChannel(TCL_STDOUT);
    if (outChannel) {
	Tcl_Flush(outChannel);
    }
    Tcl_DStringInit(&command);
    Tcl_DStringInit(&line);
    Tcl_ResetResult(interp);

    /*
     * Loop infinitely, waiting for commands to execute.  When there
     * are no windows left, Tk_MainLoop returns and we exit.
     */

    Tk_MainLoop();
    Tcl_Exit(0);

error:
    msg = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    if (msg == NULL) {
	msg = interp->result;
    }
    errChannel = Tcl_GetStdChannel(TCL_STDERR);
    if (errChannel) {
        Tcl_Write(errChannel, msg, -1);
        Tcl_Write(errChannel, "\n", 1);
    }
    if (!tclDeleteInterpAtEnd) {
        Tcl_Exit(1);
    } else {
        Tcl_DeleteInterp (interp);
        Tcl_Exit(1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SignalProc --
 *
 *	Function called on a signal generating an error to clear the stdin
 *   	buffer.
 *----------------------------------------------------------------------
 */

static void
SignalProc (signalNum)
    int  signalNum;
{
    Tcl_Channel stdoutChan = Tcl_GetStdChannel (TCL_STDOUT);
    tclGotErrorSignal = 0;
    Tcl_DStringFree (&command);
    gotPartial = 0;
    if (tty) {
        if (stdoutChan != NULL) {
            Tcl_Write (stdoutChan, "\n", 1);
        }
        TclX_OutputPrompt (interp, !gotPartial);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * StdinProc --
 *
 *	This procedure is invoked by the event dispatcher whenever
 *	standard input becomes readable.  It grabs the next line of
 *	input characters, adds them to a command being assembled, and
 *	executes the command if it's complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Could be almost arbitrary, depending on the command that's
 *	typed.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
static void
StdinProc(clientData, mask)
    ClientData clientData;		/* Not used. */
    int mask;				/* Not used. */
{
    char *cmd;
    int code, count;
    Tcl_Channel chan = (Tcl_Channel) clientData;

    count = Tcl_Gets(chan, &line);

    if (count < 0) {
	if (!gotPartial) {
	    if (tty) {
		Tcl_Exit(0);
	    } else {
		Tcl_DeleteChannelHandler(chan, StdinProc, (ClientData) chan);
	    }
	    return;
	} else {
	    count = 0;
	}
    }

    (void) Tcl_DStringAppend(&command, Tcl_DStringValue(&line), -1);
    cmd = Tcl_DStringAppend(&command, "\n", -1);
    Tcl_DStringFree(&line);
    
    if (!Tcl_CommandComplete(cmd)) {
        gotPartial = 1;
        goto prompt;
    }
    gotPartial = 0;

    /*
     * Disable the stdin channel handler while evaluating the command;
     * otherwise if the command re-enters the event loop we might
     * process commands from stdin before the current command is
     * finished.  Among other things, this will trash the text of the
     * command being evaluated.
     */

    Tcl_CreateChannelHandler(chan, 0, StdinProc, (ClientData) chan);
    code = Tcl_RecordAndEval(interp, cmd, TCL_EVAL_GLOBAL);
    Tcl_CreateChannelHandler(chan, TCL_READABLE, StdinProc,
	    (ClientData) chan);
    Tcl_DStringFree(&command);
    if (*interp->result != 0) {
	if ((code != TCL_OK) || (tty)) {
            TclX_PrintResult (interp, code, cmd);
	}
    }

    /*
     * Output a prompt.
     */

    prompt:
    if (tty) {
        TclX_OutputPrompt (interp, !gotPartial);
    }
    Tcl_ResetResult(interp);
}
