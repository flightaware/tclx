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
 * $Id: tclXfcntl.c,v 5.3 1996/02/12 07:21:15 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Attributes sets used by fcntl command.  Also a structure to return parsed
 * attribute information in.
 */
#define ATTR_NONE     0  /* No attributes in this class */

#define ATTR_RDONLY   1  /* Access checks desired.      */
#define ATTR_WRONLY   2
#define ATTR_RDWR     3
#define ATTR_READ     4
#define ATTR_WRITE    5

#define ATTR_CLOEXEC  1  /* Other attribute sets */
#define ATTR_NOBUF    2
#define ATTR_LINEBUF  4

typedef struct {
    int  access;
    int  fcntl;
    int  other;
} fcntlAttr_t;

/*
 * The maximum length of any attribute name.
 */
#define MAX_ATTR_NAME_LEN  20

/*
 * Prototypes of internal functions.
 */
static int
XlateFcntlAttr  _ANSI_ARGS_((Tcl_Interp  *interp,
                             char        *attrName,
                             fcntlAttr_t *attrPtr));

static int
GetFcntlAttr _ANSI_ARGS_((Tcl_Interp  *interp,
                          Tcl_Channel  channel,
                          int          readFileNum,
                          int          writeFileNum,
                          char        *attrName));

static int
SetAttrOnFile _ANSI_ARGS_((Tcl_Interp *interp,
                           Tcl_Channel  channel,
                           int          fileNum,
                           fcntlAttr_t attrib,
                           int         value));

static int
SetFcntlAttr _ANSI_ARGS_((Tcl_Interp  *interp,
                          Tcl_Channel  channel,
                          int          readFileNum,
                          int          writeFileNum,
                          char        *attrName,
                          char        *valueStr));

/*-----------------------------------------------------------------------------
 * XlateFcntlAttr --
 *    Translate an fcntl attribute.
 *
 * Parameters:
 *   o interp (I) - Tcl interpreter.
 *   o attrName (I) - The attrbute name to translate, maybe upper or lower
 *     case.
 *   o attrPtr (O) - Structure containing the parsed attributes.  Only one of
 *     the fields will be set, the others will be set to ATTR_NONE.
 * Result:
 *   Returns TCL_OK if all is well, TCL_ERROR if there is an error.
 *-----------------------------------------------------------------------------
 */
static int
XlateFcntlAttr (interp, attrName, attrPtr)
    Tcl_Interp  *interp;
    char        *attrName;
    fcntlAttr_t *attrPtr;
{
    char attrNameUp [MAX_ATTR_NAME_LEN];

    attrPtr->access = ATTR_NONE;
    attrPtr->fcntl = ATTR_NONE;
    attrPtr->other = ATTR_NONE;

    if (strlen (attrName) >= MAX_ATTR_NAME_LEN)
        goto invalidAttrName;

    Tcl_UpShift (attrNameUp, attrName);

    if (STREQU (attrNameUp, "RDONLY")) {
        attrPtr->access = ATTR_RDONLY;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "WRONLY")) {
        attrPtr->access = ATTR_WRONLY;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "RDWR")) {
        attrPtr->access = ATTR_RDWR;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "READ")) {
        attrPtr->access = ATTR_READ;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "WRITE")) {
        attrPtr->access = ATTR_WRITE;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "NONBLOCK")) {
        attrPtr->fcntl = O_NONBLOCK;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "APPEND")) {
        attrPtr->fcntl = O_APPEND;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "CLOEXEC")) {
        attrPtr->other = ATTR_CLOEXEC;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "NOBUF")) {
        attrPtr->other = ATTR_NOBUF;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "LINEBUF")) {
        attrPtr->other = ATTR_LINEBUF;
        return TCL_OK;
    }

    /*
     * Error return code.
     */
  invalidAttrName:
    Tcl_AppendResult (interp, "unknown attribute name \"", attrName,
                      "\", expected one of APPEND, CLOEXEC, LINEBUF, ",
                      "NONBLOCK, NOBUF, READ, RDONLY, RDWR, WRITE, WRONLY",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * GetFcntlAttr --
 *    Return the value of a specified fcntl attribute.
 *
 * Parameters:
 *   o interp (I) - Tcl interpreter, value is returned in the result
 *   o channel (I) - The channel to check.
 *   o readFileNum (I) - The read file number associated with the channel or
 *     -1 if no channel.
 *   o writeFileNum (I) - The read file number associated with the channel or
 *     -1 if no channel.
 *   o attrName (I) - The attrbute name to translate, maybe upper or lower
 *     case.
 * Result:
 *   Returns TCL_OK if all is well, TCL_ERROR if fcntl returns an error.
 *-----------------------------------------------------------------------------
 */
static int
GetFcntlAttr (interp, channel, readFileNum, writeFileNum, attrName)
    Tcl_Interp  *interp;
    Tcl_Channel  channel;
    int          readFileNum;
    int          writeFileNum;
    char        *attrName;
{
    fcntlAttr_t attrib;
    int aFileNum, current, value;
    char *option;

    if (XlateFcntlAttr (interp, attrName, &attrib) != TCL_OK)
        return TCL_ERROR;

    /*
     * If both file numbers are specified, pick one for the checking.  They
     * will be in sync for most options if the attributes were set by us.
     */
    aFileNum = (readFileNum >= 0) ? readFileNum : writeFileNum;

    /*
     * Access check.  Assumes Tcl channel is configured correctly.
     */
    if (attrib.access != ATTR_NONE) {
        switch (attrib.access) {
          case ATTR_RDONLY:
            value = (readFileNum >= 0) && (writeFileNum < 0);
            break;
          case ATTR_WRONLY:
            value = (readFileNum < 0) && (writeFileNum >= 0);
            break;
          case ATTR_RDWR:
            value = (readFileNum >= 0) && (writeFileNum >= 0);
            break;
          case ATTR_READ:
            value = (readFileNum >= 0);
            break;
          case ATTR_WRITE:
            value = (writeFileNum >= 0);
            break;
        default:
            panic ("fcntl bad attrib");
        }
        interp->result =  value ? "1" : "0";
        return TCL_OK;
    }

    /*
     * Get fcntl attributes.
     */
    if (attrib.fcntl != ATTR_NONE) {
        current = fcntl (aFileNum, F_GETFL, 0);
        if (current == -1)
            goto unixError;
        interp->result = (current & attrib.fcntl) ? "1" : "0";
        return TCL_OK;
    }

    if (attrib.other == ATTR_CLOEXEC) {
        current = fcntl (aFileNum, F_GETFD, 0);
        if (current == -1)
            goto unixError;
        interp->result = (current & 1) ? "1" : "0";
        return TCL_OK;
    }

    /*
     * Get attributes maintained by the channel.
     */
    if (attrib.other == ATTR_NOBUF) {
        interp->result = Tcl_GetChannelOption (channel, "-unbuffered");
        return TCL_OK;
    }
    if (attrib.other == ATTR_LINEBUF) {
        interp->result = Tcl_GetChannelOption (channel, "-linemode");
        return TCL_OK;
    }

  unixError:
    interp->result = Tcl_PosixError (interp);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * SetAttrOnFile --
 *    Set the the attributes on a file.  This is called twice for dual file
 * channel.
 *
 * Parameters:
 *   o interp (I) - Tcl interpreter, value is returned in the result
 *   o channel (I) - The channel to check.
 *   o fileNum (I) - The file number associated with the channel direction.
 *     -1 if no channel.
 *   o attrib (I) - Structure describing attribute to set.
 *   o value (I) - Boolean value to set the attributes to.
 * Result:
 *   Returns TCL_OK if all is well, TCL_ERROR if there is an error.
 *-----------------------------------------------------------------------------
 */
static int
SetAttrOnFile (interp, channel, fileNum, attrib, value)
    Tcl_Interp  *interp;
    fcntlAttr_t  attrib;
    Tcl_Channel  channel;
    int          fileNum;
    int          value;
{
    int current;
 
    if (attrib.fcntl != ATTR_NONE) {
        current = fcntl (fileNum, F_GETFL, 0);
        if (current == -1)
            goto unixError;
        current &= ~attrib.fcntl;
        if (value)
            current |= attrib.fcntl;
        if (fcntl (fileNum, F_SETFL, current) == -1)
            goto unixError;

        return TCL_OK;
    }

    if (attrib.other == ATTR_CLOEXEC) {
        if (fcntl (fileNum, F_SETFD, value) == -1)
            goto unixError;
        return TCL_OK;
    }

    if (attrib.other == ATTR_NOBUF) {
        return Tcl_SetChannelOption (interp, channel, "-unbuffered",
                                     value ? "1" : "0");
        return TCL_OK;
    }

    if (attrib.other == ATTR_LINEBUF) {
        return Tcl_SetChannelOption (interp, channel, "-linemode",
                                     value ? "1" : "0");
        return TCL_OK;
    }

  unixError:
    interp->result = Tcl_PosixError (interp);
    return TCL_ERROR;
   
}

/*-----------------------------------------------------------------------------
 * SetFcntlAttr --
 *    Set the specified fcntl attr to the given value.
 *
 * Parameters:
 *   o interp (I) - Tcl interpreter, value is returned in the result
 *   o channel (I) - The channel to check.
 *   o readFileNum (I) - The read file number associated with the channel or
 *     -1 if no channel.
 *   o writeFileNum (I) - The read file number associated with the channel or
 *     -1 if no channel.
 *   o valueStr (I) - The string value to set the attribiute to.
 * Result:
 *   Returns TCL_OK if all is well, TCL_ERROR if there is an error.
 *-----------------------------------------------------------------------------
 */
static int
SetFcntlAttr (interp, channel, readFileNum, writeFileNum, attrName, valueStr)
    Tcl_Interp  *interp;
    Tcl_Channel  channel;
    int          readFileNum;
    int          writeFileNum;
    char        *attrName;
    char        *valueStr;
{
    fcntlAttr_t attrib;
    int         value;

    if (XlateFcntlAttr (interp, attrName, &attrib) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetBoolean (interp, valueStr, &value) != TCL_OK)
        return TCL_ERROR;

    /*
     * Validate that this the attribute may be set (or cleared).
     */

    if (attrib.access != ATTR_NONE) {
        Tcl_AppendResult (interp, "Attribute \"", attrName, "\" may not be ",
                          "altered after open", (char *) NULL);
        return TCL_ERROR;
    }
    
    if (readFileNum >= 0) {
        if (SetAttrOnFile (interp, channel, readFileNum, attrib,
                           value) == TCL_ERROR)
            return TCL_ERROR;
    }
    if (writeFileNum >= 0) {
        if (SetAttrOnFile (interp, channel, writeFileNum, attrib,
                           value) == TCL_ERROR)
            return TCL_ERROR;
    }
    return TCL_OK;
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
    int readFileNum, writeFileNum;

    if ((argc < 3) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " handle attribute ?value?", (char *) NULL);
        return TCL_ERROR;
    }

    channel = TclX_GetOpenChannel (interp, argv [1], 0);
    if (channel == NULL)
	return TCL_ERROR;

    readFileNum = TclX_ChannelFnum (channel, TCL_READABLE);
    writeFileNum = TclX_ChannelFnum (channel, TCL_WRITABLE);

    /*
     * Get or set attributes.
     */
    if (argc == 3) {    
        if (GetFcntlAttr (interp, channel, readFileNum, writeFileNum,
                          argv [2]) != TCL_OK)
            return TCL_ERROR;
    } else {
        if (SetFcntlAttr (interp, channel, readFileNum, writeFileNum,
                          argv [2], argv [3]) != TCL_OK)
            return TCL_ERROR;
    }
    return TCL_OK;
}
