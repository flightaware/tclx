/*
 * tclXserver.c --
 *
 * High level commands for connecting to TCP/IP based servers.
 *---------------------------------------------------------------------------
 * Copyright 1991-1993 Karl Lehenbauer and Mark Diekhans.
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

#ifndef HAVE_BCOPY
#    define bcopy(from, to, length)    memmove((to), (from), (length))
#endif
#ifndef HAVE_BZERO
#    define bzero(to, length)          memset((to), '\0', (length))
#endif

extern int h_errno;


/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerOpen --
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
    server.sin_family = AF_INET;

    if (ISDIGIT (*service)) {
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
    if (ISDIGIT (*host)) {
        server.sin_addr.s_addr = inet_addr (host);
    } else {
        struct hostent *hostEntry;

        hostEntry = gethostbyname (host);
        if (hostEntry == NULL) {
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
            Tcl_AppendResult (interp, "host name lookup failure: ",
                              host, " (", errorMsg, ")",
                              (char *) NULL);
            return TCL_ERROR;
        }
        bcopy (hostEntry->h_addr, &server.sin_addr, hostEntry->h_length);
    }

    /*
     * Open a socket and connect to the server.
     */
    socketFD = socket (AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0)
        goto unixError;

    if (connect (socketFD, &server, sizeof (server)) < 0)
        goto unixError;

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
#else

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ServerOpen --
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

#endif /* HAVE_GETHOSTBYNAME */
