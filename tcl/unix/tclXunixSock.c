/*
 * tclXunixSock.c --
 *
 * Socket utility functions for Unix.  This also has the deprecated server
 * creation commands, which are not supported on platforms other than Unix.
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
 * $Id: tclXunixSock.c,v 6.0 1996/05/10 16:18:48 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

#ifndef INADDR_NONE
#    define INADDR_NONE  ((long) -1)
#endif

#ifdef NO_BCOPY
#    define bcopy(from, to, length)    memmove((to), (from), (length))
#endif

#ifndef NO_DATA
#   define NO_DATA NO_ADDRESS
#endif

extern int h_errno;

/*
 * Prototypes of internal functions.
 */
static int
ReturnGetHostError _ANSI_ARGS_((Tcl_Interp *interp,
                                char       *host));

static int
InetAtoN  _ANSI_ARGS_((Tcl_Interp     *interp,
                       char           *strAddress,
                       struct in_addr *inAddress));

static struct hostent *
InfoGetHost _ANSI_ARGS_((Tcl_Interp *interp,
                         int         argc,
                         char      **argv));


/*-----------------------------------------------------------------------------
 * ReturnGetHostError --
 *
 *   Return an error message when gethostbyname or gethostbyaddr fails.
 *
 * Parameters:
 *   o interp (O) - The error message is returned in the result.
 *   o host (I) - Host name or address that got the error.
 * Globals:
 *   o h_errno (I) - The list of file handles to parse, may be empty.
 * Returns:
 *   Always returns TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ReturnGetHostError (interp, host)
    Tcl_Interp *interp;
    char       *host;
{
    char  *errorMsg;
    char  *errorCode;

    switch (h_errno) {
      case HOST_NOT_FOUND:
        errorCode = "HOST_NOT_FOUND";
        errorMsg = "host not found";
        break;
      case TRY_AGAIN:
        errorCode = "TRY_AGAIN";
        errorMsg = "try again";
        break;
      case NO_RECOVERY:
        errorCode = "NO_RECOVERY";
        errorMsg = "unrecordable server error";
        break;
      case NO_DATA:
        errorCode = "NO_DATA";
        errorMsg = "no data";
        break;
    }
    Tcl_SetErrorCode (interp, "INET", errorCode, errorMsg, (char *)NULL);
    Tcl_AppendResult (interp, "host lookup failure: ",
                      host, " (", errorMsg, ")",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclXGetHostInfo --
 *    Return a host address, name (if it can be obtained) and port number.
 * Used by the fstat command.
 *     
 * Parameters:
 *   o interp (O) - List is returned in the result.
 *   o fileNum (I) - File number.  Should be a socket connection.
 *   o remoteHost (I) -  TRUE to get remote host information, FALSE to get 
 *     local host info.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXGetHostInfo (interp, fileNum, remoteHost)
    Tcl_Interp *interp;
    int         fileNum;
    int         remoteHost;
{
    int nameLen;
    struct sockaddr_in sockaddr;
    struct hostent *hostEntry;
    char *hostName;
    char portText [32];

    nameLen = sizeof (sockaddr);

    if (remoteHost) {
        if (getpeername (fileNum, (struct sockaddr *) &sockaddr,
                         &nameLen) < 0)
            goto unixError;
    } else {
        if (getsockname (fileNum, (struct sockaddr *) &sockaddr,
                         &nameLen) < 0)
            goto unixError;
    }

    hostEntry = gethostbyaddr ((char *) &(sockaddr.sin_addr),
                               sizeof (sockaddr.sin_addr),
                               AF_INET);
    if (hostEntry != NULL)
        hostName = hostEntry->h_name;
    else
        hostName = "";

    Tcl_AppendElement (interp, inet_ntoa (sockaddr.sin_addr));

    Tcl_AppendElement (interp, hostName);

    sprintf (portText, "%u", ntohs (sockaddr.sin_port));
    Tcl_AppendElement (interp, portText);
       
    return TCL_OK;

  unixError:
    Tcl_ResetResult (interp);
    interp->result = Tcl_PosixError (interp);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclXGetKeepLive --
 *    Get the keepalive option on a socket.
 *     
 * Parameters:
 *   o interp (O) - Errors are returned in the result.
 *   o channel (I) - Channel associated with the socket.
 *   o valuePtr (I) -  TRUE is return if keepalive is set, FALSE if its not.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXGetKeepAlive (interp, channel, valuePtr)
    Tcl_Interp  *interp;
    Tcl_Channel  channel;
    int         *valuePtr;
{
    int fileNum, value, valueLen = sizeof (value);

    fileNum = TclX_ChannelFnum (channel, 0);

    if (getsockopt (fileNum, SOL_SOCKET, SO_KEEPALIVE,
                    (void*) &value, &valueLen) != 0) {
        Tcl_AppendResult (interp, "error getting socket KEEPALIVE option: ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    *valuePtr = value;
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXSetKeepLive --
 *    Set the keepalive option on a socket.
 *     
 * Parameters:
 *   o interp (O) - Errors are returned in the result.
 *   o channel (I) - Channel associated with the socket.
 *   o value (I) -  TRUE or FALSE.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXSetKeepAlive (interp, channel, value)
    Tcl_Interp  *interp;
    Tcl_Channel  channel;
    int          value;
{
    int fileNum, valueLen = sizeof (value);

    fileNum = TclX_ChannelFnum (channel, 0);

    if (setsockopt (fileNum, SOL_SOCKET, SO_KEEPALIVE,
                    (void*) &value, valueLen) != 0) {
        Tcl_AppendResult (interp, "error setting socket KEEPALIVE option: ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * InetAtoN --
 *
 *   Convert an internet address to an "struct in_addr" representation.
 *
 * Parameters:
 *   o interp (O) - If not NULL, an error message is return in the result.
 *     If NULL, no error message is generated.
 *   o strAddress (I) - String address to convert.
 *   o inAddress (O) - Converted internet address is returned here.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
InetAtoN (interp, strAddress, inAddress)
    Tcl_Interp     *interp;
    char           *strAddress;
    struct in_addr *inAddress;
{
    int result = TCL_OK;

#ifndef NO_INET_ATON
    if (!inet_aton (strAddress, inAddress))
        result = TCL_ERROR;
#else
    inAddress->s_addr = inet_addr (strAddress);
    if (inAddress->s_addr == INADDR_NONE)
        result = TCL_ERROR;
#endif
    if ((result == TCL_ERROR) && (interp != NULL)) {
        Tcl_AppendResult (interp, "malformed address: \"",
                          strAddress, "\"", (char *) NULL);
    }
    return result;
}

/*-----------------------------------------------------------------------------
 * InfoGetHost --
 *
 *   Validate arguments and call gethostbyaddr for the host_info options
 * that return info about a host.  This looks up host information either by
 * name or address.
 *
 * Parameters:
 *   o interp (O) - The error message is returned in the result.
 *   o argc, argv (I) - Command argments.  Host name or IP address is expected
 *     in argv [2].
 * Returns:
 *   Pointer to the host entry or NULL if an error occured.
 *-----------------------------------------------------------------------------
 */
static struct hostent *
InfoGetHost (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    struct hostent *hostEntry;
    struct in_addr address;

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ", argv [1],
                          " host", (char *) NULL);
        return NULL;
    }

    if (InetAtoN (NULL, argv [2], &address) == TCL_OK) {
        hostEntry = gethostbyaddr ((char *) &address,
                                   sizeof (address),
                                   AF_INET);
    } else {
        hostEntry = gethostbyname (argv [2]);
    }
    if (hostEntry == NULL) {
        ReturnGetHostError (interp, argv [2]);
        return NULL;
    }
    return hostEntry;
}

/*-----------------------------------------------------------------------------
 * Tcl_HostInfoCmd --
 *     Implements the TCL host_info command:
 *
 *      host_info addresses host
 *      host_info official_name host
 *      host_info aliases host
 *
 * Results:
 *   For hostname, a list of address associated with the host.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ServerInfoCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    struct hostent *hostEntry;
    struct in_addr  inAddr;
    int             idx;

    if (argc < 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0],
                          " option ...", (char *) NULL);
        return TCL_ERROR;
    }

    if (STREQU (argv [1], "addresses")) {
        hostEntry = InfoGetHost (interp, argc, argv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        for (idx = 0; hostEntry->h_addr_list [idx] != NULL; idx++) {
            bcopy ((VOID *) hostEntry->h_addr_list [idx],
                   (VOID *) &inAddr,
                   hostEntry->h_length);
            Tcl_AppendElement (interp, inet_ntoa (inAddr));
        }
        return TCL_OK;
    }

    if (STREQU (argv [1], "address_name")) {
        hostEntry = InfoGetHost (interp, argc, argv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        for (idx = 0; hostEntry->h_addr_list [idx] != NULL; idx++) {
            bcopy ((VOID *) hostEntry->h_addr_list [idx],
                   (VOID *) &inAddr,
                   hostEntry->h_length);
            Tcl_AppendElement (interp, hostEntry->h_name);
        }
        return TCL_OK;
    }

    if (STREQU (argv [1], "official_name")) {
        hostEntry = InfoGetHost (interp, argc, argv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        Tcl_SetResult (interp, hostEntry->h_name, TCL_STATIC);
        return TCL_OK;
    }

    if (STREQU (argv [1], "aliases")) {
        hostEntry = InfoGetHost (interp, argc, argv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        for (idx = 0; hostEntry->h_aliases [idx] != NULL; idx++) {
            Tcl_AppendElement (interp, hostEntry->h_aliases [idx]);
        }
        return TCL_OK;
    }

    Tcl_AppendResult (interp, "invalid option \"", argv [1],
                      "\", expected one of \"addresses\", \"official_name\"",
                      " or \"aliases\"", (char *) NULL);
    return TCL_ERROR;
}

/*=============================================================================
 * The server_create and server_accept commands are deprecated in favor of the
 * Tcl socket -server functionallity, however they can't be implemented as
 * backwards compatibility procs.
 *=============================================================================
 */
#define SERVER_BUF      1
#define SERVER_NOBUF    2

#ifdef NO_BZERO
#    define bzero(to, length)          memset((to), '\0', (length))
#endif

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
            if (InetAtoN (interp, argv [nextArg],
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
 * TclX_SocketInit --
 *     
 *   Initialize the server commands in the specified interpreter.
 *-----------------------------------------------------------------------------
 */
void
TclX_SocketInit (interp)
    Tcl_Interp *interp;
{
    Tcl_CreateCommand (interp, "host_info", Tcl_ServerInfoCmd,
                       (ClientData) NULL, (void (*)()) NULL);

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
