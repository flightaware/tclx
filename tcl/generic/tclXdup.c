/*
 * tclXdup.c
 *
 * Extended Tcl dup command.
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
 * $Id: tclXdup.c,v 7.3 1996/08/06 07:15:27 markd Exp $
 *-----------------------------------------------------------------------------
 */
#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
DupChannelOptions _ANSI_ARGS_((Tcl_Interp  *interp,
                               Tcl_Channel  srcChannel,
                               Tcl_Channel  targetChannel));

static Tcl_Channel
DupFileChannel _ANSI_ARGS_((Tcl_Interp *interp,
                            char       *srcFileId,
                            char       *targetFileId));


/*-----------------------------------------------------------------------------
 * DupChannelOptions --
 *
 *   Set the channel options of one channel to those of another.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o srcChannel (I) - Channel to get the options.
 *   o targetChannel (I) - Channel to set the options on.
 * Result:
 *   TCL_OK or TCL_ERROR;
 *-----------------------------------------------------------------------------
 */
static int
DupChannelOptions (interp, srcChannel, targetChannel)
    Tcl_Interp  *interp;
    Tcl_Channel  srcChannel;
    Tcl_Channel  targetChannel;
{
    Tcl_DString strValues;
    char *scanPtr, *option, *value;
    int size, result;

    Tcl_DStringInit (&strValues);

    if (Tcl_GetChannelOption (srcChannel, NULL, &strValues) != TCL_OK)
        goto fatalError;

    /*
     * Walk (rather than split) the list for each name/value pair and set
     * the new channel.  Only modify blocking if its not the default, as
     * setting blocking on standard files generates an error on some systems.
     * Skip options that can't be set.
     */
    scanPtr = strValues.string;

    while (*scanPtr != '\0') {
        result = TclFindElement (interp, scanPtr, &option, &scanPtr, &size,
                                 NULL);
        if ((result != TCL_OK) || (*scanPtr == '\0'))
            goto fatalError;
        option [size] = '\0';
        result = TclFindElement (interp, scanPtr, &value, &scanPtr, &size,
                                 NULL);
        if (result != TCL_OK)
            goto fatalError;
        value [size] = '\0';

        if (STREQU (option, "-blocking") && (value [0] != '0'))
            continue;

        if (STREQU (option, "-peername") || STREQU (option, "-sockname"))
            continue;

        if (Tcl_SetChannelOption (interp, targetChannel, option,
                                  value) != TCL_OK) {
            Tcl_DStringFree (&strValues);
            return TCL_ERROR;
        }
    }

    Tcl_DStringFree (&strValues);
    return TCL_OK;

  fatalError:
    panic ("DupChannelOption bug");
    return TCL_ERROR;  /* Not reached */
}

/*-----------------------------------------------------------------------------
 * DupFileChannel --
 *   Do common work for all platforms for duplicate a channel
 *
 * Parameters:
 *   o interp (I) - If an error occures, the error message is in result.
 *   o srcChannelId (I) - The id of the channel to dup.
 *   o targetChannelId (I) - The id for the new file.  NULL if any id maybe
 *     used.
 * Returns:
 *   The unregistered channel, or NULL if an error occurs.
 *-----------------------------------------------------------------------------
 */
static Tcl_Channel
DupFileChannel (interp, srcChannelId, targetChannelId)
    Tcl_Interp *interp;
    char       *srcChannelId;
    char       *targetChannelId;
{
    Tcl_Channel srcChannel, newChannel = NULL;
    Tcl_ChannelType *channelType;
    int mode, seekable;

    srcChannel = Tcl_GetChannel (interp, srcChannelId, &mode);
    if (srcChannel == NULL)
        return NULL;

    channelType = Tcl_GetChannelType (srcChannel);
    if (STREQU (channelType->typeName, "pipe")) {
        Tcl_AppendResult (interp, "can not \"dup\" a Tcl command pipeline ",
                          " created with the \"open\" command",
                          (char *) NULL);
        return NULL;
    }
    
    /*
     * If writable, flush out the buffer.
     */
    if (mode & TCL_WRITABLE) {
        if (Tcl_Flush (srcChannel) == TCL_ERROR)
            goto posixError;
    }

    /*
     * Use OS dependent function to actually dup the channel.
     */
    newChannel = TclXOSDupChannel (interp, srcChannel, mode, targetChannelId);
    if (newChannel == NULL)
        return NULL;

    /*
     * If the channel is open for reading and seekable, seek the new channel
     * to the same position.
     */
    if (mode & TCL_READABLE) {
        if (TclXOSSeekable (interp, srcChannel, &seekable) == TCL_ERROR)
            goto errorExit;
        if (seekable) {
            off_t seekOffset = Tcl_Tell (srcChannel);
            if (seekOffset < 0)
                goto posixError;
            Tcl_Seek (newChannel, seekOffset, SEEK_SET);
        }
    }
    
    if (DupChannelOptions (interp, srcChannel, newChannel) != TCL_OK)
        goto errorExit;

    return newChannel;

  posixError:
    Tcl_ResetResult (interp);
    Tcl_AppendResult (interp, "dup of \"", srcChannelId, " failed: ",
                      Tcl_PosixError (interp), (char *) NULL);

  errorExit:
    if (newChannel != NULL) {
        Tcl_Close (NULL, newChannel);
    }
    return NULL;
}

/*-----------------------------------------------------------------------------
 * Tcl_DupCmd --
 *     Implements the dup TCL command:
 *         dup channelId ?targetChannelId?
 *
 * Results:
 *      Returns TCL_OK and interp->result containing a filehandle
 *      if the requested file or pipe was successfully duplicated.
 *
 *      Return TCL_ERROR and interp->result containing an
 *      explanation of what went wrong if an error occured.
 *-----------------------------------------------------------------------------
 */
int
Tcl_DupCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    Tcl_Channel newChannel;

    if ((argc < 2) || (argc > 3)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0], 
                          " channelId ?targetChannelId?", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * If a number is supplied, bind it to a file handle rather than doing
     * a dup.
     */
    if (ISDIGIT (argv [1][0])) {
        if (argc != 2) {
            Tcl_AppendResult (interp, "the second argument, targetChannelId, ",
                              "is not allow when binding a file number to ",
                              "a Tcl channel", (char *) NULL);
            return TCL_ERROR;
        }
        newChannel = TclXOSBindOpenFile (interp, argv [1]);
    } else {
        newChannel = DupFileChannel (interp, argv [1], argv [2]);
    }
    if (newChannel == NULL)
        return TCL_ERROR;

    Tcl_RegisterChannel (interp, newChannel);
    Tcl_SetResult (interp, Tcl_GetChannelName (newChannel), TCL_STATIC);
    return TCL_OK;
}
