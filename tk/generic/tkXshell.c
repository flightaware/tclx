/*
 * tkXshell.c
 *
 * Version of Tk main that is modified to build a wish shell with the Extended
 * Tcl command set and libraries.  This makes it easier to use a different
 * main.
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
 * $Id: tkXshell.c,v 5.6 1996/02/16 07:51:39 markd Exp $
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
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

static char sccsid[] = "@(#) tkMain.c 1.136 96/01/17 09:32:28";

#include <ctype.h>
#include <stdio.h>
#include <tclExtdInt.h>
#include "tclXpatchl.h"
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
static char *argv0;		/* Holds the name of the script file we're
				 * processing, if there was one.  Otherwise
				 * holds argv[0] from the command line.  This
				 * value is used as a default for the
				 * application name. */
static int gotPartial = 0;

/*
 * Forward declarations for procedures defined later in this file.
 * Note: TkDefaultAppName is declared here even though it's also
 * in tkInt.h.  This is because we don't include tkInt.h here:
 * people should be able to make a local copy of this file outside
 * the Tk source area to build personal main programs.
 */

static void		Prompt _ANSI_ARGS_((Tcl_Interp *interp, int partial));
static void		StdinProc _ANSI_ARGS_((ClientData clientData,
			    int mask));
static void		SignalProc _ANSI_ARGS_((int  signalNum));
EXTERN char *		TkDefaultAppName _ANSI_ARGS_((void));

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
    char *tkxVersion;

    tkxVersion = ckalloc (strlen (TK_VERSION) + 
                           strlen (TCL_EXTD_VERSION_SUFFIX) + 1);
    strcpy (tkxVersion, TK_VERSION);
    strcat (tkxVersion, TCL_EXTD_VERSION_SUFFIX);

    TclX_SetAppInfo (TRUE,
                     "wishx",
                     "Extended Wish",
                     tkxVersion,
                     TCLX_PATCHLEVEL);


    Tcl_FindExecutable(argv[0]);
    interp = Tcl_CreateInterp();
#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(interp);
#endif

    inChannel = Tcl_GetChannel(interp, "stdin", NULL);
    outChannel = Tcl_GetChannel(interp, "stdout", NULL);
    errChannel = Tcl_GetChannel(interp, "stderr", NULL);

    /*
     * Parse command-line arguments.  A leading "-file" argument is
     * ignored (a historical relic from the distant past).  If the
     * next argument doesn't start with a "-" then strip it off and
     * use it as the name of a script file to process.
     */

    argv0 = argv[0];
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
    if (fileName != NULL) {
	argv0 = fileName;
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
    Tcl_SetVar(interp, "argv0", argv0, TCL_GLOBAL_ONLY);

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
        Tcl_CreateChannelHandler(inChannel, TCL_READABLE, StdinProc,
                (ClientData) inChannel);
	if (tty) {
	    TclX_OutputPrompt (interp, 1);
	}
    }

    tclSignalBackgroundError = Tk_BackgroundError;
    Tcl_Flush(outChannel);
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
    Tcl_Write(errChannel, msg, -1);
    Tcl_Write(errChannel, "\n", 1);
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
    tclGotErrorSignal = 0;
    Tcl_DStringFree (&command);
    gotPartial = 0;
    if (tty) {
        Tcl_Write (TclX_Stdout (interp), "\n", 1);
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
    
    if (count != 0) {
	if (!Tcl_CommandComplete(cmd)) {
	    gotPartial = 1;
	    goto prompt;
	}
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

/*
 *----------------------------------------------------------------------
 *
 * TkDefaultAppName --
 *
 *	This procedure provides a default application name to use
 *	if one isn't specified explicitly for a Tk application.
 *
 * Results:
 *	Returns a string that can be used as the default application
 *	name.  This is either the last component of the file name
 *	from which the application was invoked, or "tk" if we don't
 *	know the name of the application's file (e.g. because Tk was
 *	loaded as a shared library).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TkDefaultAppName()
{
    char *p;

    if (argv0 == NULL) {
	return "tk";
    }
    p = strrchr(argv0, '/');
    if (p != NULL) {
	p++;
    } else {
	p = argv0;
    }
    return p;
}
