/*
 * tclXserver.c --
 *
 * High level commands for connecting to TCP/IP based servers.
 *---------------------------------------------------------------------------
 * Copyright 1991-1995 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include "tclExtdInt.h"

#ifdef HAVE_SOCKET

#include <sys/types.h>
#ifndef NO_SYS_SOCKET_H
#    include <sys/socket.h>
#endif
#include <sys/uio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef INADDR_NONE
#    define INADDR_NONE  ((long) -1)
#endif

#ifndef HAVE_BCOPY
#    define bcopy(from, to, length)    memmove((to), (from), (length))
#endif
#ifndef HAVE_BZERO
#    define bzero(to, length)          memset((to), '\0', (length))
#endif

extern int h_errno;


/*
 * Buffering options.
 */
#define SERVER_BUF      1
#define SERVER_NOBUF    2
#define SERVER_TWOIDS   4

/*
 * Prototypes of internal functions.
 */
static int
ReturnGetHostError _ANSI_ARGS_((Tcl_Interp *interp,
                                char       *host));

static int
BindFileHandles _ANSI_ARGS_((Tcl_Interp *interp,
                             unsigned    options,
                             int         socketFD));

static struct hostent *
InfoGetHost _ANSI_ARGS_((Tcl_Interp *interp,
                         int         argc,
                         char      **argv));

static int
SendMessage _ANSI_ARGS_((Tcl_Interp *interp,
                         FILE       *filePtr,
                         int         flags,
                         int         nonewline,
                         char       *strmsg));

/*
 *-----------------------------------------------------------------------------
 *
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

/*
 *-----------------------------------------------------------------------------
 *
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

#ifdef HAVE_INET_ATON
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

/*
 *-----------------------------------------------------------------------------
 *
 * BindFileHandles --
 *
 *   Bind the file handles for a socket to one or two Tcl files and return
 * the files handles in the result.  If an error occurs, both file descriptors
 * will be closed and cleaned up.
 *
 * Parameters:
 *   o interp (O) - File handles or error messages are return in result.
 *   o options (I) - Options set controling buffering and handle allocation:
 *       o SERVER_BUF - Two file handle buffering.
 *       o SERVER_NOBUF - No buffering.
 *       o SERVER_TWOIDS - Allocate two file handles (ids), one for reading
 *         and one for writting.
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
    int      socketFD2 = -1;
    FILE     *filePtr, *file2Ptr;
    OpenFile *tclFilePtr;

    /*
     * Handle setting up two file handles, buffered or unbuffered.
     */
    if (options & SERVER_TWOIDS) {
        socketFD2 = dup (socketFD);
        if (socketFD2 < 0) {
            interp->result = Tcl_PosixError (interp);
            goto errorExit;
        }
        filePtr = Tcl_SetupFileEntry (interp, socketFD, TCL_FILE_READABLE);
        if (filePtr == NULL)
            goto errorExit;

        file2Ptr = Tcl_SetupFileEntry (interp, socketFD2, TCL_FILE_WRITABLE);
        if (filePtr == NULL)
            goto errorExit;

        if (options & SERVER_NOBUF) {
            setbuf (filePtr, NULL);
            setbuf (file2Ptr, NULL);
        }
        sprintf (interp->result, "file%d file%d", socketFD, socketFD2);
        return TCL_OK;
    }

    /*
     * Setup unbuffered, single handle access.
     */
    if (options & SERVER_NOBUF) {
        filePtr = Tcl_SetupFileEntry (interp, socketFD,
                                      TCL_FILE_READABLE | TCL_FILE_WRITABLE);
        if (filePtr == NULL)
            goto errorExit;

        setbuf (filePtr, NULL);
        sprintf (interp->result, "file%d", socketFD);
        return TCL_OK;
    }

    /*
     * Set up single handle, buffered access.  This requires creating
     * two FILE objects, but binding them to a single handle.  This is
     * something Tcl_GetOpenFile supports internally.
     */
    socketFD2 = dup (socketFD);
    if (socketFD2 < 0) {
        interp->result = Tcl_PosixError (interp);
        goto errorExit;
    }

    filePtr = Tcl_SetupFileEntry (interp, socketFD, TCL_FILE_READABLE);
    if (filePtr == NULL)
        goto errorExit;

    tclFilePtr = tclOpenFiles [fileno (filePtr)];

    file2Ptr = fdopen (socketFD2, "w");
    if (file2Ptr == NULL) {
        interp->result = Tcl_PosixError (interp);
        return NULL;
    }

    tclFilePtr->permissions |= TCL_FILE_WRITABLE;
    tclFilePtr->f2 = file2Ptr;

    sprintf (interp->result, "file%d", socketFD);
    return TCL_OK;

  errorExit:
    Tcl_CloseForError (interp, socketFD);
    if (socketFD2 >= 0)
        Tcl_CloseForError (interp, socketFD2);
    return TCL_ERROR;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerConnectCmd --
 *     Implements the TCL server_connect command:
 *
 *        server_connect ?options? host service
 *
 *  Opens a stream socket to the specified host (host name or IP address),
 *  service name or port number.  Options maybe -buf or -nobuf, -twoids, and
 *  -hostip hostname.
 *
 * Results:
 *   If successful, a pair Tcl fileids are returned for -buf or a single fileid
 * is returned for -nobuf.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ServerConnectCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    unsigned            options;
    char               *host, *service;
    int                 socketFD = -1, nextArg;
    struct hostent     *hostEntry;
    struct sockaddr_in  server, local;
    int                 idx, specifiedLocalIp = 0;
    int                 myPort;

    /*
     * Parse arguments.
     */
    if ((argc < 3) || (argc > 6)) {
        goto tclArgError;
    }

    bzero ((VOID *) &local, sizeof (local));
    nextArg = 1;
    options = SERVER_BUF;
    while ((nextArg < argc) && (argv [nextArg][0] == '-')) {
        if (STREQU ("-buf", argv [nextArg])) {
            options &= ~SERVER_NOBUF;
            options |= SERVER_BUF;
        } else if (STREQU ("-nobuf", argv [nextArg])) {
            options &= ~SERVER_BUF;
            options |= SERVER_NOBUF;
        } else if (STREQU ("-twoids", argv [nextArg])) {
            options |= SERVER_TWOIDS;
        } else if (STREQU ("-myip", argv [nextArg])) {
            if (nextArg >= argc - 1)
                goto missingArg;
            nextArg++;
            if (InetAtoN (interp, argv [nextArg],
                          &local.sin_addr) == TCL_ERROR)
                return TCL_ERROR;
            local.sin_family = AF_INET;
            specifiedLocalIp = 1;
        } else if (STREQU ("-myport", argv [nextArg])) {
            if (nextArg >= argc - 1)
                goto missingArg;
            nextArg++;
            if (Tcl_GetInt (interp, argv [nextArg], &myPort) != TCL_OK)
                return TCL_ERROR;
            local.sin_port = htons (myPort);
        } else {
            Tcl_AppendResult (interp, "expected one of \"-buf\", \"-nobuf\", ",
                              "\"-twoids\", \"-myip\" or \"-myport\", got \"",
                              argv [nextArg], "\"", (char *) NULL);
            return TCL_ERROR;
        }
        nextArg++;
    }

    if (nextArg != argc - 2)
        goto tclArgError;

    host = argv [nextArg];
    service = argv [nextArg + 1];

    /*
     * Convert service number or lookup the service name.
     */
    bzero ((VOID *) &server, sizeof (server));

    if (ISDIGIT (service [0])) {
        int  port;
        
        if (Tcl_GetInt (interp, service, &port) != TCL_OK)
            return TCL_ERROR;
        server.sin_port = htons (port);
    } else {
        struct servent *servEntry;

        servEntry = getservbyname (service, NULL);
        if (servEntry == NULL) {
            Tcl_AppendResult (interp, "unknown service: ", service,
                              (char *) NULL);
            Tcl_SetErrorCode (interp, "INET", "UNKNOWN_SERVICE",
                                "unknown service", (char *)NULL);
            return TCL_ERROR;
        }
        server.sin_port = servEntry->s_port;
    }

    /*
     * Convert IP address or lookup host name.
     */
    if (InetAtoN (NULL, host, &server.sin_addr) == TCL_OK) {
        server.sin_family = AF_INET;
        hostEntry = NULL;
    } else {
        hostEntry = gethostbyname (host);
        if (hostEntry == NULL)
            return ReturnGetHostError (interp, host);

        server.sin_family = hostEntry->h_addrtype;
    }

    /*
     * Open a socket and connect to the server.  If the connect fails and
     * other addresses are available, try them.
     */
    socketFD = socket (server.sin_family, SOCK_STREAM, 0);
    if (socketFD < 0)
        goto unixError;

    if (specifiedLocalIp) {
        if (bind (socketFD, (struct sockaddr *) &local, sizeof (local)) < 0) {
            goto unixError;
        }
    }

    /*
     * Connect to server.  If we started with a hostname, there might be
     * multiple addresses to try.
     */
    if (hostEntry == NULL) {
        if (connect (socketFD, (struct sockaddr *) &server,
                     sizeof (server)) < 0)
            goto unixError;
    } else {
        for (idx = 0; hostEntry->h_addr_list [idx] != NULL; idx++) {
            bcopy ((VOID *) hostEntry->h_addr_list [idx],
                   (VOID *) &server.sin_addr,
                   hostEntry->h_length);

            if (connect (socketFD, (struct sockaddr *) &server,
                         sizeof (server)) >= 0)
                break;  /* Got it */
        }
        if (hostEntry->h_addr_list [idx] == NULL)
            goto unixError;
    }

    /*
     * Set up stdio FILE structures and we are done.
     */
    return BindFileHandles (interp, options, socketFD);

    /*
     * Exit points for errors.
     */
  
  tclArgError:
    Tcl_AppendResult (interp, tclXWrongArgs, argv[0],
                      " ?options? host service|port", (char *) NULL);
    return TCL_ERROR;

  missingArg:
    Tcl_AppendResult (interp, "missing argument for ", argv [nextArg],
                      (char *) NULL);
    return TCL_ERROR;

  unixError:
    interp->result = Tcl_PosixError (interp);
    if (socketFD >= 0)
        close (socketFD);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
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
 *  and "-backlog backlog"
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
    int                 socketFD = -1, nextArg;
    struct sockaddr_in  local;
    int                 myPort;
    int                 backlog = 5;

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
            if (Tcl_GetInt (interp, argv [nextArg], &myPort) != TCL_OK)
                return TCL_ERROR;
            local.sin_port = htons (myPort);
        } else if (STREQU ("-backlog", argv [nextArg])) {
            if (nextArg >= argc - 1)
                goto missingArg;
            nextArg++;
            if (Tcl_GetInt (interp, argv [nextArg], &backlog) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_AppendResult (interp, "expected  ",
                              " \"-myip\", \"-myport\" or \"-backlog\"",
                              ", got \"", argv [nextArg], "\"", (char *) NULL);
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
     * Open a socket and bind an address and port to it.
     */
    socketFD = socket (local.sin_family, SOCK_STREAM, 0);
    if (socketFD < 0)
        goto unixError;

    if (bind (socketFD, (struct sockaddr *) &local, sizeof (local)) < 0) {
        goto unixError;
    }

    if (listen (socketFD, backlog) < 0)
        goto unixError;

    if (Tcl_SetupFileEntry (interp, socketFD, TCL_FILE_READABLE) == NULL)
        goto errorExit;

    sprintf (interp->result, "file%d", socketFD);
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
    if (socketFD >= 0)
        Tcl_CloseForError (interp, socketFD);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerAcceptCmd --
 *     Implements the TCL server_accept command:
 *
 *        server_accept ?options? file
 *
 *  Accepts an IP connection request to a socket created by server_create.
 *  Options maybe -buf or -nobuf and -twoids.
 *
 * Results:
 *   If successful, a pair Tcl fileids are returned for -buf or a single fileid
 * is returned for -nobuf.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ServerAcceptCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    unsigned            options;
    int                 acceptSocketFD, addrLen;
    int                 socketFD = -1, nextArg;
    struct sockaddr_in  connectSocket;
    FILE               *acceptFilePtr;

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
        } else if (STREQU ("-twoids", argv [nextArg])) {
            options |= SERVER_TWOIDS;
        } else {
            Tcl_AppendResult (interp, "expected \"-buf\", \"-nobuf\" or ",
                              "\"-twoids\", got \"", argv [nextArg],
                              "\"", (char *) NULL);
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

    if (Tcl_GetOpenFile (interp, argv [nextArg], 
                         FALSE, TRUE,  /* Read access */
                         &acceptFilePtr) == TCL_ERROR) {
        return TCL_ERROR;
    }
    acceptSocketFD = fileno (acceptFilePtr);
    addrLen = sizeof (connectSocket);
    socketFD = accept (acceptSocketFD, 
                       (struct sockaddr *)&connectSocket, 
                       &addrLen);
    if (socketFD < 0)
        goto unixError;

    /*
     * Set up stdio FILE structures and we are done.
     */
    return BindFileHandles (interp, options, socketFD);

    /*
     * Exit points for errors.
     */
  unixError:
    interp->result = Tcl_PosixError (interp);
    if (socketFD >= 0)
        Tcl_CloseForError (interp, socketFD);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * InfoGetHost --
 *
 *   Validate arguments and call gethostbyaddr for the server_info options
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

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerInfoCmd --
 *     Implements the TCL server_info command:
 *
 *      server_info addresses host
 *      server_info official_name host
 *      server_info aliases host
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

/*
 *-----------------------------------------------------------------------------
 *
 * SendMessage --
 *
 *   Send a message on a socket.  There are two versions of this function,
 * one that uses sendmsg and takes advantage of gather-write to send the new
 * line. The other is for systems that don't have sendmsg (Linux).
 *
 * Parameters:
 *   o interp (O) - Error messages are returned in result.
 *   o filePtr (I) - FILE associated with socket.
 *   o flags (I) - Flags to pass to sendmsg/send.
 *   o nonewline (I) - TRUE if a newline should not be sent.
 *   o strmsg (I) - String message to send.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
#ifdef HAVE_SENDMSG
static int
SendMessage (interp, filePtr, flags, nonewline, strmsg)
    Tcl_Interp *interp;
    FILE       *filePtr;
    int         flags;
    int         nonewline;
    char       *strmsg;
{
    struct msghdr msgHeader;
    struct iovec  dataVec [2];
    int           bytesAttempted;
    int           bytesSent;

    /*
     * Fill in the message header and write vector.  If a newline is
     * to be included, its the second element in the vector.
     */
    msgHeader.msg_name = NULL;
    msgHeader.msg_namelen = 0;
    msgHeader.msg_iov = dataVec;
    msgHeader.msg_iovlen = 1;
#ifdef HAVE_MSG_ACCRIGHTS
    msgHeader.msg_accrights = NULL;
    msgHeader.msg_accrightslen = 0;
#else
    msgHeader.msg_control = NULL;
    msgHeader.msg_controllen = 0;
#endif    

    bytesAttempted = strlen (strmsg);
    dataVec [0].iov_base = strmsg;
    dataVec [0].iov_len = bytesAttempted;

    if (!nonewline) {
        dataVec [1].iov_base = "\n";
        dataVec [1].iov_len = 1;
        msgHeader.msg_iovlen++;
        bytesAttempted++;
    }
   
    bytesSent = sendmsg (fileno (filePtr), &msgHeader, flags);
    if (bytesSent < 0) {
        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }

    if (bytesSent != bytesAttempted) {
        Tcl_AppendResult (interp, "sent fewer bytes than requested",
                          (char *)NULL);
    }
    return TCL_OK;
}
#else
static int
SendMessage (interp, filePtr, flags, nonewline, strmsg)
    Tcl_Interp *interp;
    FILE       *filePtr;
    int         flags;
    int         nonewline;
    char       *strmsg;
{
    Tcl_DString  buffer;
    char        *message;
    int          bytesAttempted;
    int          bytesSent;


    if (nonewline) {
        message = strmsg;
    } else {
        Tcl_DStringInit (&buffer);
        Tcl_DStringAppend (&buffer, strmsg, -1);
        Tcl_DStringAppend (&buffer, "\n", -1);
        message = Tcl_DStringValue (&buffer);
    }
    bytesAttempted = strlen (message);

    bytesSent = send (fileno (filePtr), message, bytesAttempted, flags);
    if (!nonewline) {
        Tcl_DStringFree (&buffer);
    }
    if (bytesSent < 0) {
        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }

    if (bytesSent != bytesAttempted) {
        Tcl_AppendResult (interp, "sent fewer bytes than requested",
                          (char *)NULL);
    }
    return TCL_OK;
}
#endif /* HAVE_SENDMSG */

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerSendCmd --
 *     Implements the TCL server_send command:
 *
 *        server_send [options] fileid message
 *
 *          -nonewline -outofband -dontroute
 *-----------------------------------------------------------------------------
 */
int
Tcl_ServerSendCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    FILE  *filePtr;
    int    flags = 0;
    int    nonewline = 0;
    int    nextArg;

    nextArg = 1;
    while ((nextArg < argc) && (argv [nextArg][0] == '-')) {
        if (STREQU (argv [nextArg], "-nonewline")) {
            nonewline = 1;
        } else if (STREQU (argv [nextArg], "-outofband")) {
            flags |= MSG_OOB;
        } else if (STREQU (argv [nextArg], "-dontroute")) {
            flags |= MSG_DONTROUTE;
        } else {
            Tcl_AppendResult (interp, "expected one of \"-nonewline\",",
                              " \"-outofband\", or \"-dontroute\" got \"",
                              argv [nextArg], "\"", (char *) NULL);
            return TCL_ERROR;
        }
        nextArg++;
    }
    if (nextArg != argc - 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0],
                          " [options] fileid message", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetOpenFile (interp, argv [nextArg++],
                         TRUE, TRUE, /* Write access */
                         &filePtr) != TCL_OK)
        return TCL_ERROR;

    return SendMessage (interp, filePtr,  flags, nonewline, argv [nextArg]);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerInit --
 *     
 *   Initialize the server commands in the specified interpreter.
 *-----------------------------------------------------------------------------
 */
void
Tcl_ServerInit (interp)
    Tcl_Interp *interp;
{
    Tcl_CreateCommand (interp, "server_connect", Tcl_ServerConnectCmd,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "server_info", Tcl_ServerInfoCmd,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "server_create", Tcl_ServerCreateCmd,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "server_accept", Tcl_ServerAcceptCmd,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "server_send", Tcl_ServerSendCmd,
                       (ClientData) NULL, (void (*)()) NULL);
}
#else

/*
 *-----------------------------------------------------------------------------
 *
 * ServerNotAvailable --
 *     Command stub that returns an error indicating the command is not 
 * available.
 *-----------------------------------------------------------------------------
 */
static int
ServerNotAvailable (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    Tcl_AppendResult (interp, argv [0], " not available on this system",
                      (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerInit --
 *     
 *   Initialize the stub server commands in the specified interpreter.  These
 * commands will return an error indicating the command is not available.
 *-----------------------------------------------------------------------------
 */
void
Tcl_ServerInit (interp)
    Tcl_Interp *interp;
{
    Tcl_CreateCommand (interp, "server_connect", ServerNotAvailable,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "server_info", ServerNotAvailable,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "server_create", ServerNotAvailable,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "server_accept", ServerNotAvailable,
                       (ClientData) NULL, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "server_send", ServerNotAvailable,
                       (ClientData) NULL, (void (*)()) NULL);
}
#endif /* HAVE_SOCKET */
