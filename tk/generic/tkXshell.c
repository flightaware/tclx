/*
 * tkXshell.c
 *
 * Version of Tk main that is modified to build a wish shell with the Extended
 * Tcl command set and libraries.  This makes it easier to use a different
 * main.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1995 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tkXshell.c,v 4.8 1995/06/30 23:27:19 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

/* 
 * tkMain.c --
 *
 *	This file contains the main program for "wish", a windowing
 *	shell based on Tk and Tcl.  It also provides a template that
 *	can be used as the basis for main programs for other Tk
 *	applications.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 * All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclExtdInt.h"
#include "tk.h"

/*
 * Global variables used by the main program:
 */

static Tk_Window mainWindow;	/* The main window for the application.  If
				 * NULL then the application no longer
				 * exists. */
static Tcl_Interp *interp;	/* Interpreter for this application. */
static Tcl_DString command;	/* Used to assemble lines of terminal input
				 * into Tcl commands. */
static int gotPartial = 0;      /* Partial command in buffer. */
static int tty;			/* Non-zero means standard input is a
				 * terminal-like device.  Zero means it's
				 * a file. */
static char exitCmd[] = "exit";
static char errorExitCmd[] = "exit 1";

/*
 * Command-line options:
 */

static int synchronize = 0;
static char *fileName = NULL;
static char *name = NULL;
static char *display = NULL;
static char *geometry = NULL;

static Tk_ArgvInfo argTable[] = {
    {"-display", TK_ARGV_STRING, (char *) NULL, (char *) &display,
	"Display to use"},
    {"-geometry", TK_ARGV_STRING, (char *) NULL, (char *) &geometry,
	"Initial geometry for window"},
    {"-name", TK_ARGV_STRING, (char *) NULL, (char *) &name,
	"Name to use for application"},
    {"-sync", TK_ARGV_CONSTANT, (char *) 1, (char *) &synchronize,
	"Use synchronous mode for display server"},
    {(char *) NULL, TK_ARGV_END, (char *) NULL, (char *) NULL,
	(char *) NULL}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		StdinProc _ANSI_ARGS_((ClientData clientData,
			    int mask));
static void		SignalProc _ANSI_ARGS_((int signalNum));

/*
 *----------------------------------------------------------------------
 *
 * TkX_Main --
 *
 *	Main program for Wish.
 *
 * Results:
 *	None. This procedure never returns (it exits the process when
 *	it's done
 *
 * Side effects:
 *	This procedure initializes the wish world and then starts
 *	interpreting commands;  almost anything could happen, depending
 *	on the script being interpreted.
 *
 *----------------------------------------------------------------------
 */

void
TkX_Main (argc, argv, appInitProc)
    int argc;				/* Number of arguments. */
    char **argv;			/* Array of argument strings. */
    Tcl_AppInitProc *appInitProc;	/* Application-specific initialization
					 * procedure to call after most
					 * initialization but before starting
					 * to execute commands. */
{
    char *args, *p, *msg, *argv0, *class;
    char buf[20];
    int code;
    size_t length;
    FILE *stderrPtr;

    stderrPtr = TCL_STDERR;

    interp = Tcl_CreateInterp();
#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(interp);
#endif

    /*
     * Parse command-line arguments.  A leading "-file" argument is
     * ignored (a historical relic from the distant past).  If the
     * next argument doesn't start with a "-" then strip it off and
     * use it as the name of a script file to process.  Also check
     * for other standard arguments, such as "-geometry", anywhere
     * in the argument list.
     */

    argv0 = argv[0];
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
    if (Tk_ParseArgv(interp, (Tk_Window) NULL, &argc, argv, argTable, 0)
	    != TCL_OK) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }
    if (name == NULL) {
	if (fileName != NULL) {
	    p = fileName;
	} else {
	    p = argv[0];
	}
	name = strrchr(p, '/');
	if (name != NULL) {
	    name++;
	} else {
	    name = p;
	}
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
    Tcl_SetVar(interp, "argv0", (fileName != NULL) ? fileName : argv0,
	    TCL_GLOBAL_ONLY);

    /*
     * If a display was specified, put it into the DISPLAY
     * environment variable so that it will be available for
     * any sub-processes created by us.
     */

    if (display != NULL) {
	Tcl_SetVar2(interp, "env", "DISPLAY", display, TCL_GLOBAL_ONLY);
    }

    /*
     * Initialize the Tk application.  If a -name option was provided,
     * use it;  otherwise, if a file name was provided, use the last
     * element of its path as the name of the application; otherwise
     * use the last element of the program name.  For the application's
     * class, capitalize the first letter of the name.
     */

    if (name == NULL) {
	p = (fileName != NULL) ? fileName : argv0;
	name = strrchr(p, '/');
	if (name != NULL) {
	    name++;
	} else {
	    name = p;
	}
    }
    class = (char *) ckalloc((unsigned) (strlen(name) + 1));
    strcpy(class, name);
    class[0] = toupper((unsigned char) class[0]);
    mainWindow = Tk_CreateMainWindow(interp, display, name, class);
    ckfree(class);
    if (mainWindow == NULL) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }
    if (synchronize) {
	XSynchronize(Tk_Display(mainWindow), True);
    }

    /*
     * Set the "tcl_interactive" variable.  If we are going to be interactive,
     * set up SIGINT handling.
     */
    tty = isatty(0);
    Tcl_SetVar(interp, "tcl_interactive",
 	    ((fileName == NULL) && tty) ? "1" : "0", TCL_GLOBAL_ONLY);
    if ((fileName == NULL) && tty)
        Tcl_SetupSigInt ();

    /*
     * Set the geometry of the main window, if requested.  Put the
     * requested geometry into the "geometry" variable.
     */

    if (geometry != NULL) {
	Tcl_SetVar(interp, "geometry", geometry, TCL_GLOBAL_ONLY);
	code = Tcl_VarEval(interp, "wm geometry . ", geometry, (char *) NULL);
	if (code != TCL_OK) {
	    fprintf(stderr, "%s\n", interp->result);
	}
    }

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

        tclErrorSignalProc = SignalProc;
	Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) 0);
	if (tty) {
	    TclX_OutputPrompt (interp, 1);
	}
    }
    tclSignalBackgroundError = Tk_BackgroundError;

    fflush(TCL_STDOUT);
    Tcl_DStringInit(&command);

    /*
     * Set the geometry of the main window, if requested.  If the "geometry"
     * variable has gone away, this means that the application doesn't want
     * us to set the geometry after all.
     */

    if (geometry != NULL) {
	p = Tcl_GetVar(interp, "geometry", TCL_GLOBAL_ONLY);
	if (p != NULL) {
	    code = Tcl_VarEval(interp, "wm geometry . ", p, (char *) NULL);
	    if (code != TCL_OK) {
		fprintf(stderr, "%s\n", interp->result);
	    }
	}
    }

    /*
     * Loop infinitely, waiting for commands to execute.  When there
     * are no windows left, Tk_MainLoop returns and we exit.
     */

    Tk_MainLoop();

    Tcl_DStringFree(&command);

    /*
     * Don't exit directly, but rather invoke the Tcl "exit" command.
     * This gives the application the opportunity to redefine "exit"
     * to do additional cleanup.
     */

    if (!tclDeleteInterpAtEnd) {
        Tcl_GlobalEval(interp, exitCmd);
    } else {
        Tcl_DeleteInterp (interp);
    }
    exit(1);

error:
    msg = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    if (msg == NULL) {
	msg = interp->result;
    }
    fprintf(stderrPtr, "%s\n", msg);

    if (!tclDeleteInterpAtEnd) {
        Tcl_GlobalEval(interp, errorExitCmd);
    } else {
        Tcl_DeleteInterp (interp);
    }
    exit (1);
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
        fputc ('\n', TCL_STDOUT);
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

static void
StdinProc(clientData, mask)
    ClientData clientData;		/* Not used. */
    int mask;				/* Not used. */
{
#define BUFFER_SIZE 4000
    char input[BUFFER_SIZE+1];
    char *cmd;
    int code, count;

    count = read(0, input, BUFFER_SIZE);
    if (count <= 0) {
	if (!gotPartial) {
	    if (tty) {
		Tcl_VarEval(interp, "exit", (char *) NULL);
		exit(1);
	    } else {
		Tk_DeleteFileHandler(0);
	    }
	    return;
	} else {
	    count = 0;
	}
    }
    cmd = Tcl_DStringAppend(&command, input, count);
    if (count != 0) {
	if ((input[count-1] != '\n') && (input[count-1] != ';')) {
	    gotPartial = 1;
	    goto exitPoint;
	}
	if (!Tcl_CommandComplete(cmd)) {
	    gotPartial = 1;
	    goto exitPoint;
	}
    }
    gotPartial = 0;

    /*
     * Disable the stdin file handler;  otherwise if the command
     * re-enters the event loop we might process commands from
     * stdin before the current command is finished.  Among other
     * things, this will trash the text of the command being evaluated.
     */

    Tk_CreateFileHandler(0, 0, StdinProc, (ClientData) 0);
    code = Tcl_RecordAndEval(interp, cmd, TCL_EVAL_GLOBAL);
    Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) 0);
    if ((code != TCL_OK) || tty)
        TclX_PrintResult (interp, code, cmd);
    Tcl_DStringFree(&command);

  exitPoint:
    if (tty) {
        TclX_OutputPrompt (interp, !gotPartial);
    }
}
