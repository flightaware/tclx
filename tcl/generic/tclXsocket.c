/*
 * tclXsocket.c --
 *
 * Socket utility functions and commands.
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
 * $Id: tclXsocket.c,v 1.1 1996/07/25 04:12:27 markd Exp $
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

/*
 * Prototypes of internal functions.
 */
static int
ReturnGetHostError _ANSI_ARGS_((Tcl_Interp *interp,
                                char       *host));

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
 *   o channel (I) - Channel associated with the socket.
 *   o remoteHost (I) -  TRUE to get remote host information, FALSE to get 
 *     local host info.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXGetHostInfo (interp, channel, remoteHost)
    Tcl_Interp *interp;
    Tcl_Channel channel;
    int         remoteHost;
{
    struct sockaddr_in sockaddr;
    struct hostent *hostEntry;
    char *hostName;
    char portText [32];

    if (remoteHost) {
        if (TclXOSgetpeername (channel, &sockaddr, sizeof (sockaddr)) < 0)
            goto unixError;
    } else {
        if (TclXOSgetsockname (channel,  &sockaddr, sizeof (sockaddr)) < 0)
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

    if (TclXOSInetAtoN (NULL, argv [2], &address) == TCL_OK) {
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
int
Tcl_HostInfoCmd (clientData, interp, argc, argv)
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
