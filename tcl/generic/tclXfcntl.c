/*
 * tclXfcntl.c
 *
 * Extended Tcl fcntl command.
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
 * $Id: tclXfcntl.c,v 7.2 1996/08/06 07:15:27 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Attributes sets used by fcntl command.
 */
#define ATTR_ERROR           -1  /* Error parsing attributes.   */

#define ATTR_RDONLY           1  /* Access checks desired.      */
#define ATTR_WRONLY           2
#define ATTR_RDWR             3
#define ATTR_READ             4
#define ATTR_WRITE            5
#define ATTR_APPEND           6
#define ATTR_CLOEXEC          7
#define ATTR_NOBUF            8
#define ATTR_LINEBUF          9
#define ATTR_NONBLOCK        10
#define ATTR_KEEPALIVE       11

/*
 * The maximum length of any attribute name.
 */
#define MAX_ATTR_NAME_LEN  20

/*
 * Table of attribute names and values.
 */
struct {
    char *name;
    int   id;
    int   modifiable;
} attrNames [] = {
    {"RDONLY",    ATTR_RDONLY,    FALSE},
    {"WRONLY",    ATTR_WRONLY,    FALSE},
    {"RDWR",      ATTR_RDWR,      FALSE},
    {"READ",      ATTR_READ,      FALSE},
    {"WRITE",     ATTR_WRITE,     FALSE},
    {"APPEND",    ATTR_APPEND,    TRUE},
    {"CLOEXEC",   ATTR_CLOEXEC,   TRUE},
    {"NONBLOCK",  ATTR_NONBLOCK,  TRUE},
    {"LINEBUF",   ATTR_LINEBUF,   TRUE},
    {"NOBUF",     ATTR_NOBUF,     TRUE},
    {"KEEPALIVE", ATTR_KEEPALIVE, TRUE},
    {NULL,        0,              FALSE}};

/*
 * Prototypes of internal functions.
 */
static int
XlateFcntlAttr  _ANSI_ARGS_((Tcl_Interp  *interp,
                             char        *attrName,
                             int          modify));

static int
GetFcntlAttr _ANSI_ARGS_((Tcl_Interp  *interp,
                          Tcl_Channel  channel,
                          int          mode,
                          int          attrib));

static int
SetFcntlAttr _ANSI_ARGS_((Tcl_Interp  *interp,
                          Tcl_Channel  channel,
                          int          attrib,
                          char        *valueStr));

/*-----------------------------------------------------------------------------
 * XlateFcntlAttr --
 *    Translate an fcntl attribute to an numberic id.
 *
 * Parameters:
 *   o interp - Tcl interp, errors in result
 *   o attrName - The attrbute name to translate, maybe upper or lower case.
 *   o modify - Will the attribute be modified
 * Result:
 *   The number associated with the attirbute, or ATTR_ERROR is an error
 * occures.
 *-----------------------------------------------------------------------------
 */
static int
XlateFcntlAttr (interp, attrName, modify)
    Tcl_Interp  *interp;
    char        *attrName;
    int          modify;
{
    char attrNameUp [MAX_ATTR_NAME_LEN];
    int idx;

    if (strlen (attrName) >= MAX_ATTR_NAME_LEN)
        goto invalidAttrName;
    
    Tcl_UpShift (attrNameUp, attrName);
    
    for (idx = 0; attrNames [idx].name != NULL; idx++) {
        if (STREQU (attrNameUp, attrNames [idx].name)) {
            if (modify && !attrNames [idx].modifiable) {
                Tcl_AppendResult (interp, "Attribute \"", attrName,
                                  "\" may not be altered after open",
                                  (char *) NULL);
                return ATTR_ERROR;
            }
            return attrNames [idx].id;
        }
    }

    /*
     * Invalid attribute.
     */
  invalidAttrName:
    Tcl_AppendResult (interp, "unknown attribute name \"", attrName,
                      "\", expected one of ", (char *) NULL);
    for (idx = 0; attrNames [idx + 1].name != NULL; idx++) {
        Tcl_AppendResult (interp, attrNames [idx].name, ", ", (char *) NULL);
    }
    Tcl_AppendResult (interp, "or ", attrNames [idx].name, (char *) NULL);
    return ATTR_ERROR;
}

/*-----------------------------------------------------------------------------
 * GetFcntlAttr --
 *    Return the value of a specified fcntl attribute.
 *
 * Parameters:
 *   o interp - Tcl interpreter, value is returned in the result
 *   o channel - The channel to check.
 *   o mode - Channel access mode.
 *   o attrib - Attribute to get.
 * Result:
 *   TCL_OK or TCL_ERROR
 *-----------------------------------------------------------------------------
 */
static int
GetFcntlAttr (interp, channel, mode, attrib)
    Tcl_Interp  *interp;
    Tcl_Channel  channel;
    int          mode;
    int          attrib;
{
    int current, value;

    switch (attrib) {
      case ATTR_RDONLY:
        value = (mode & TCL_READABLE) && !(mode & TCL_WRITABLE);
        break;
      case ATTR_WRONLY:
        value = (mode & TCL_WRITABLE) && !(mode & TCL_READABLE);
        break;
      case ATTR_RDWR:
        value = (mode & TCL_READABLE) && (mode & TCL_WRITABLE);
        break;
      case ATTR_READ:
        value =  (mode & TCL_READABLE);
        break;
      case ATTR_WRITE:
        value = (mode & TCL_WRITABLE);
        break;
      case ATTR_APPEND:
        current = fcntl (TclX_ChannelFnum (channel, 0), F_GETFL, 0);
        if (current == -1)
            goto unixError;
        value = ((current & O_APPEND) != 0);
        break;
      case ATTR_CLOEXEC:
        current = fcntl (TclX_ChannelFnum (channel, 0), F_GETFD, 0);
        if (current == -1)
            goto unixError;
        value = ((current & 1) != 0);
        break;
      case ATTR_NONBLOCK:
        value  = (TclX_GetChannelOption (channel, TCLX_COPT_BLOCKING) ==
                  TCLX_MODE_NONBLOCKING);
        break;
      case ATTR_NOBUF:
        value = (TclX_GetChannelOption (channel, TCLX_COPT_BUFFERING) ==
                 TCLX_BUFFERING_NONE);
        break;
      case ATTR_LINEBUF:
        value = (TclX_GetChannelOption (channel, TCLX_COPT_BUFFERING) ==
                 TCLX_BUFFERING_LINE);
        break;
      case ATTR_KEEPALIVE:
        if (TclXOSgetsockopt (interp, channel, SO_KEEPALIVE, &value) != TCL_OK)
            return TCL_ERROR;
      default:
        panic ("fcntl get attrib");
    }

    Tcl_SetResult (interp, (value ? "1" : "0"), TCL_STATIC);
    return TCL_OK;

  unixError:
    Tcl_AppendResult (interp, Tcl_PosixError (interp), (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * SetFcntlAttr --
 *    Set the the attributes on a channel.
 *
 * Parameters:
 *   o interp - Tcl interpreter, value is returned in the result
 *   o channel - The channel to check.
 *   o attrib - Atrribute to set.
 *   o valueStr - String value (all are boolean now).
 * Result:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
SetFcntlAttr (interp, channel, attrib, valueStr)
    Tcl_Interp  *interp;
    Tcl_Channel  channel;
    int          attrib;
    char        *valueStr;
{
    int value, current, readFnum, writeFnum;

    if (Tcl_GetBoolean (interp, valueStr, &value) != TCL_OK)
        return TCL_ERROR;

    if ((attrib == ATTR_APPEND) || (attrib == ATTR_CLOEXEC)) {
        readFnum = TclX_ChannelFnum (channel, TCL_READABLE);
        writeFnum = TclX_ChannelFnum (channel, TCL_WRITABLE);
    }

    switch (attrib) {
      case ATTR_APPEND:
        if (writeFnum < 0)
            goto notWriteAccess;
        current = fcntl (writeFnum, F_GETFL, 0);
        if (current == -1)
            goto unixError;
        current = (current & ~O_APPEND) | (value ? O_APPEND : 0);
        if (fcntl (writeFnum, F_SETFL, current) == -1)
            goto unixError;

      case ATTR_CLOEXEC:
        if (readFnum > 0) {
            if (fcntl (readFnum, F_SETFD, value) == -1)
                goto unixError;
        }
        if ((writeFnum > 0) && (readFnum != writeFnum)) {
            if (fcntl (writeFnum, F_SETFD, value) == -1)
                goto unixError;
        }
        return TCL_OK;
      case ATTR_NONBLOCK:
        return TclX_SetChannelOption (interp, channel, TCLX_COPT_BLOCKING,
                                      value ? TCLX_MODE_NONBLOCKING :
                                              TCLX_MODE_BLOCKING);
      case ATTR_NOBUF:
        return TclX_SetChannelOption (interp, channel, TCLX_COPT_BUFFERING,
                                      value ? TCLX_BUFFERING_NONE :
                                              TCLX_BUFFERING_FULL);
      case ATTR_LINEBUF:
        return TclX_SetChannelOption (interp, channel, TCLX_COPT_BUFFERING,
                                      value ? TCLX_BUFFERING_LINE :
                                              TCLX_BUFFERING_FULL);
      case ATTR_KEEPALIVE:
        return TclXOSsetsockopt (interp, channel, SO_KEEPALIVE, value);
      default:
        panic ("fcntl set attrib");
    }

  unixError:
    Tcl_AppendResult (interp, Tcl_PosixError (interp), (char *) NULL);
    return TCL_ERROR;

  notWriteAccess:
    Tcl_AppendResult (interp, Tcl_GetChannelName (channel),
                      " not open for write access", (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_FcntlCmd --
 *     Implements the fcntl TCL command:
 *         fcntl handle attribute ?value?
 *-----------------------------------------------------------------------------
 */
int
Tcl_FcntlCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    Tcl_Channel channel;
    int mode, attrib;

    if ((argc < 3) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " handle attribute ?value?", (char *) NULL);
        return TCL_ERROR;
    }

    channel = Tcl_GetChannel (interp, argv [1], &mode);
    if (channel == NULL)
	return TCL_ERROR;

    attrib = XlateFcntlAttr (interp, argv [2], (argc == 4));
    if (attrib == ATTR_ERROR)
        return TCL_ERROR;

    if (argc == 3) {    
        if (GetFcntlAttr (interp, channel, mode, attrib) != TCL_OK)
            return TCL_ERROR;
    } else {
        if (SetFcntlAttr (interp, channel, attrib, argv [3]) != TCL_OK)
            return TCL_ERROR;
    }
    return TCL_OK;
}
