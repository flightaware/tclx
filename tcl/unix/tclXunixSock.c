/*
 * tclXunixSock.c --
 *
 * Deprecated server creation commands, which are not supported on platforms
 * other than Unix. These commands are deprecated in favor of the Tcl socket
 * functionallity, however they can't be implemented as backwards
 * compatibility procs.
 *---------------------------------------------------------------------------
 * Copyright 1991-1996 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXunixSock.c,v 7.0 1996/06/16 05:33:30 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

#define SERVER_BUF      1
#define SERVER_NOBUF    2

/*
 * Prototypes of internal functions.
 */
static int
BindFileHandles _ANSI_ARGS_((Tcl_Interp *interp,
                             unsigned    options,
                             int         socketFD));


/*-----------------------------------------------------------------------------
 * BindFileHandles --
 *
 *   Bind the file handles for a socket to one or two Tcl file channels.
 * Binding to two handles is for compatibility with older interfaces.
 * If an error occurs, both file descriptors will be closed and cleaned up.
 *
 * Parameters:
 *   o interp (O) - File handles or error messages are return in result.
 *   o options (I) - Options set controling buffering and handle allocation:
 *       o SERVER_BUF - Two file handle buffering.
 *       o SERVER_NOBUF - No buffering.
 *   o socketFD (I) - File number of the socket that was opened.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
BindFileHandles (interp, options, socketFD)
    Tcl_Interp *interp;
    unsigned    options;
    int         socketFD;
{
    int socketFD2 = -1;
    Tcl_Channel channel = NULL, channel2 = NULL;

    channel = TclX_SetupFileEntry (interp, socketFD, 
                                   TCL_READABLE | TCL_WRITABLE, TRUE);

    if (options & SERVER_NOBUF) {
        if (TclX_SetChannelOption (interp, channel, TCLX_COPT_BUFFERING,
                                   TCLX_BUFFERING_NONE) == TCL_ERROR)
            goto errorExit;
    }

    Tcl_AppendElement (interp, Tcl_GetChannelName (channel));
    return TCL_OK;

  errorExit:
    Tcl_CloseForError (interp, channel, socketFD);
    Tcl_CloseForError (interp, channel2, socketFD2);
    return TCL_ERROR;
}


/*-----------------------------------------------------------------------------
 * Tcl_ServerCreateCmd --
 *     Implements the TCL server_create command:
 *
 *        server_create ?options?
 *
 *  Creates a socket, binds the address and port on the local machine 
 * (optionally specified by the caller), and starts the port listening 
 * for connections by calling listen (2).
 *
 *  Options may be "-myip ip_address", "-myport port_number",
 * "-myport reserved", and "-backlog backlog".
 *
 * Results:
 *   If successful, a Tcl fileid is returned.
 *
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ServerCreateCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int socketFD = -1, nextArg;
    struct sockaddr_in  local;
    int myPort, value;
    int backlog = 5;
    int getReserved = FALSE;
    Tcl_Channel channel = NULL;

    /*
     * Parse arguments.
     */
    bzero ((VOID *) &local, sizeof (local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    nextArg = 1;

    while ((nextArg < argc) && (argv [nextArg][0] == '-')) {
        if (STREQU ("-myip", argv [nextArg])) {
            if (nextArg >= argc - 1)
                goto missingArg;
            nextArg++;
            if (TclXOSInetAtoN (interp, argv [nextArg],
                                &local.sin_addr) == TCL_ERROR)
                return TCL_ERROR;
        } else if (STREQU ("-myport", argv [nextArg])) {
            if (nextArg >= argc - 1)
                goto missingArg;
            nextArg++;
            if (STREQU (argv [nextArg], "reserved")) {
                getReserved = TRUE;
            } else {
                if (Tcl_GetInt (interp, argv [nextArg], &myPort) != TCL_OK)
                    return TCL_ERROR;
                local.sin_port = htons (myPort);
            }
        } else if (STREQU ("-backlog", argv [nextArg])) {
            if (nextArg >= argc - 1)
                goto missingArg;
            nextArg++;
            if (Tcl_GetInt (interp, argv [nextArg], &backlog) != TCL_OK)
                return TCL_ERROR;
        } else if (STREQU ("-reuseaddr", argv [nextArg])) {
            /* Ignore for compatibility */
        } else {
            Tcl_AppendResult (interp, "expected ",
                              "\"-myip\", \"-myport\", or \"-backlog\", ",
                              "got \"", argv [nextArg], "\"", (char *) NULL);
            return TCL_ERROR;
        }
        nextArg++;
    }

    if (nextArg != argc) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0],
                          " ?options?", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Allocate a reserved port if requested.
     */
    if (getReserved) {
        int port;
        if (rresvport (&port) < 0)
            goto unixError;
        local.sin_port = port;
    }

    /*
     * Open a socket and bind an address and port to it.
     */
    socketFD = socket (local.sin_family, SOCK_STREAM, 0);
    if (socketFD < 0)
        goto unixError;

    value = 1;
    if (setsockopt (socketFD, SOL_SOCKET, SO_REUSEADDR,
                    (void*) &value, sizeof (value)) < 0) {
        goto unixError;
    }
    if (bind (socketFD, (struct sockaddr *) &local, sizeof (local)) < 0) {
        goto unixError;
    }

    if (listen (socketFD, backlog) < 0)
        goto unixError;

    channel = TclX_SetupFileEntry (interp, socketFD, TCL_READABLE, TRUE);

    Tcl_AppendResult (interp, Tcl_GetChannelName (channel), (char *) NULL);
    return TCL_OK;

    /*
     * Exit points for errors.
     */
  missingArg:
    Tcl_AppendResult (interp, "missing argument for ", argv [nextArg],
                      (char *) NULL);
    return TCL_ERROR;

  unixError:
    interp->result = Tcl_PosixError (interp);

  errorExit:
    Tcl_CloseForError (interp, channel, socketFD);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_ServerAcceptCmd --
 *     Implements the TCL server_accept command:
 *
 *        server_accept ?options? file
 *
 *  Accepts an IP connection request to a socket created by server_create.
 *  Options maybe -buf orr -nobuf.
 *
 * Results:
 *   If successful, a Tcl fileid.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ServerAcceptCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    unsigned options;
    int acceptSocketFD, addrLen;
    int socketFD = -1, nextArg;
    struct sockaddr_in connectSocket;
    Tcl_File acceptFile;

    /*
     * Parse arguments.
     */
    nextArg = 1;
    options = SERVER_BUF;
    while ((nextArg < argc) && (argv [nextArg][0] == '-')) {
        if (STREQU ("-buf", argv [nextArg])) {
            options &= ~SERVER_NOBUF;
            options |= SERVER_BUF;
        } else if (STREQU ("-nobuf", argv [nextArg])) {
            options &= ~SERVER_BUF;
            options |= SERVER_NOBUF;
        } else {
            Tcl_AppendResult (interp, "expected \"-buf\" or \"-nobuf\", ",
                              "got \"", argv [nextArg], "\"", (char *) NULL);
            return TCL_ERROR;
        }
        nextArg++;
    }

    if (nextArg != argc - 1) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0], " ?options? fileid",
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Accept a socket connection on the socket created by server_create.
     */
    bzero ((VOID *) &connectSocket, sizeof (connectSocket));

    acceptSocketFD = TclX_GetOpenFnum (interp, argv [nextArg], 0);
    if (acceptSocketFD < 0)
        return TCL_ERROR;

    addrLen = sizeof (connectSocket);
    socketFD = accept (acceptSocketFD, 
                       (struct sockaddr *)&connectSocket, 
                       &addrLen);
    if (socketFD < 0)
        goto unixError;

    /*
     * Set up channels and we are done.
     */
    return BindFileHandles (interp, options, socketFD);

    /*
     * Exit points for errors.
     */
  unixError:
    interp->result = Tcl_PosixError (interp);
    if (socketFD >= 0)
        close (socketFD);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_ServerInit --
 *     
 *   Initialize the server commands in the specified interpreter.
 *-----------------------------------------------------------------------------
 */
void
TclX_ServerInit (interp)
    Tcl_Interp *interp;
{
    /*
     * These commands are deprecated in favor of the Tcl socket -server
     * functionallity, however they can't be implemented as backwards
     * compatibility procs.
     */
    Tcl_CreateCommand (interp, "server_accept", Tcl_ServerAcceptCmd,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "server_create", Tcl_ServerCreateCmd,
                       (ClientData) NULL, (void (*)()) NULL);
}
