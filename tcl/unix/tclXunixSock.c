/*
 * tclXserver.c --
 *
 * High level commands for connecting to TCP/IP based servers.
 *---------------------------------------------------------------------------
 * Copyright 1991-1994 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include "tclExtdInt.h"

#ifdef HAVE_GETHOSTBYNAME

#include <sys/types.h>
#ifndef NO_SYS_SOCKET_H
#    include <sys/socket.h>
#endif
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
 * Prototypes of internal functions.
 */
static int
ReturnGetHostError _ANSI_ARGS_((Tcl_Interp *interp,
                                char       *host));

static struct hostent *
InfoGetHostByName _ANSI_ARGS_((Tcl_Interp *interp,
                               int         argc,
                               char      **argv));

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

    switch (h_errno) {
      case HOST_NOT_FOUND:
        errorMsg = "host not found";
        break;
      case TRY_AGAIN:
        errorMsg = "try again";
        break;
      case NO_RECOVERY:
        errorMsg = "unrecordable server error";
        break;
      case NO_DATA:
        errorMsg = "no data";
        break;
    }
    Tcl_AppendResult (interp, "host lookup failure: ",
                      host, " (", errorMsg, ")",
                      (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerOpenCmd --
 *     Implements the TCL server_open command:
 *
 *        server_open ?option? host service
 *
 *  Opens a stream socket to the specified host (host name or IP address),
 *  service name or port number.  Options maybe -buf or -nobuf.
 *
 * Results:
 *   If successful, a pair Tcl fileids are returned for -buf or a single fileid
 * is returned for -nobuf.
 *-----------------------------------------------------------------------------
 */
int
Tcl_ServerOpenCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    char               *host, *service;
    int                 socketFD = -1, socketFD2 = -1, nextArg, buffered;
    struct hostent     *hostEntry;
    struct sockaddr_in  server;
    FILE               *filePtr;

    /*
     * Parse arguments.
     */
    if ((argc < 3) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0],
                          " ?option? host service|port", (char *) NULL);
        return TCL_ERROR;
    }

    if (argc == 4) {
        if (STREQU ("-buf", argv [1])) {
            buffered = TRUE;
        } else if (STREQU ("-nobuf", argv [1])) {
            buffered = FALSE;
        } else {
            Tcl_AppendResult (interp, "expected one of \"-buf\" or \"-nobuf\"",
                              ", got \"", argv [1], "\"", (char *) NULL);
            return TCL_ERROR;
        }
        nextArg = 2;
    } else {
        buffered = TRUE;
        nextArg = 1;
    }

    host = argv [nextArg];
    service = argv [nextArg + 1];

    /*
     * Convert service number or lookup the service name.
     */
    bzero (&server, sizeof (server));

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
            return TCL_ERROR;
        }
        server.sin_port = servEntry->s_port;
    }

    /*
     * Convert IP address or lookup host name.
     */
    server.sin_addr.s_addr = inet_addr (host);
    if (server.sin_addr.s_addr != INADDR_NONE) {
        server.sin_family = AF_INET;
        hostEntry = NULL;
    } else {
        hostEntry = gethostbyname (host);
        if (hostEntry == NULL)
            return ReturnGetHostError (interp, host);

        server.sin_family = hostEntry->h_addrtype;
        bcopy (hostEntry->h_addr_list [0], &server.sin_addr,
               hostEntry->h_length);
        hostEntry->h_addr_list++;
    }

    /*
     * Open a socket and connect to the server.  If the connect fails and
     * other addresses are available, try them.
     */
    socketFD = socket (server.sin_family, SOCK_STREAM, 0);
    if (socketFD < 0)
        goto unixError;

    while (TRUE) {
        if (connect (socketFD, (struct sockaddr *) &server,
                     sizeof (server)) >= 0)
            break;  /* Got it */

        if ((hostEntry == NULL) || (hostEntry->h_addr_list [0] == NULL))
            goto unixError;

        /*
         * Try next address.
         */
        bcopy (hostEntry->h_addr_list [0], &server.sin_addr,
               hostEntry->h_length);
        hostEntry->h_addr_list++;
    }

    /*
     * Set up stdio FILE structures.  If buffered, a pair (read/write) is 
     * returned.  If not buffered, a single one is returned.
     */
    if (!buffered) {
        filePtr = Tcl_SetupFileEntry (interp, socketFD,
                                      TCL_FILE_READABLE | TCL_FILE_WRITABLE);
        if (filePtr == NULL)
            goto errorExit;

        setbuf (filePtr, NULL);
        sprintf (interp->result, "file%d", socketFD);
        return TCL_OK;
    }

    if (Tcl_SetupFileEntry (interp, socketFD, TCL_FILE_READABLE) == NULL)
        goto errorExit;

    socketFD2 = dup (socketFD);
    if (socketFD2 < 0)
        goto unixError;

    if (Tcl_SetupFileEntry (interp, socketFD2, TCL_FILE_WRITABLE) == NULL)
        goto errorExit;

    sprintf (interp->result, "file%d file%d", socketFD, socketFD2);
    return TCL_OK;

    /*
     * Exit points for errors.
     */
  unixError:
    interp->result = Tcl_PosixError (interp);

  errorExit:
    if (socketFD >= 0)
        Tcl_CloseForError (interp, socketFD);
    if (socketFD2 >= 0)
        Tcl_CloseForError (interp, socketFD2);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * InfoGetHostByName --
 *
 *   Validate arguments and call gethostbyname for the server_info options
 * that return info about a host name.
 *
 * Parameters:
 *   o interp (O) - The error message is returned in the result.
 *   o argc, argv (I) - Command argments.
 * Returns:
 *   Pointer to the host entry or NULL if an error occured.
 *-----------------------------------------------------------------------------
 */
static struct hostent *
InfoGetHostByName (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    struct hostent *hostEntry;

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ", argv [1],
                          " hostname", (char *) NULL);
        return NULL;
    }

    hostEntry = gethostbyname (argv [2]);
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
 *        server_info addresses hostname
 *        server_info addresses hostname
 *
 * Results:
 *   For hostname, a list of address associated with the host.
 *-----------------------------------------------------------------------------
 */
int
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
        hostEntry = InfoGetHostByName (interp, argc, argv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        for (idx = 0; hostEntry->h_addr_list [idx] != NULL; idx++) {
            bcopy (hostEntry->h_addr_list [idx], &inAddr,
                   hostEntry->h_length);
            Tcl_AppendElement (interp, inet_ntoa (inAddr));
        }
        return TCL_OK;
    }

    if (STREQU (argv [1], "official_name")) {
        hostEntry = InfoGetHostByName (interp, argc, argv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        Tcl_SetResult (interp, hostEntry->h_name, TCL_STATIC);
        return TCL_OK;
    }

    if (STREQU (argv [1], "aliases")) {
        hostEntry = InfoGetHostByName (interp, argc, argv);
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
#else

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerOpenCmd --
 *     Version of this command that return an error on systems that don't have
 * sockets.
 *-----------------------------------------------------------------------------
 */
int
Tcl_ServerOpenCmd (clientData, interp, argc, argv)
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
 * Tcl_ServerInfoCmd --
 *     Version of this command that return an error on systems that don't have
 * sockets.
 *-----------------------------------------------------------------------------
 */
int
Tcl_ServerInfoCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_ServerOpenCmd (clientData, interp, argc, argv);
}

#endif /* HAVE_GETHOSTBYNAME */
