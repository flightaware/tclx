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
 * $Id: tclXdup.c,v 7.0 1996/06/16 05:30:12 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
ConvertFileHandle _ANSI_ARGS_((Tcl_Interp *interp,
                               char       *handle));

static int
DupChannelOptions _ANSI_ARGS_((Tcl_Interp  *interp,
                               Tcl_Channel  srcChannel,
                               Tcl_Channel  targetChannel));

static int
DupFileHandle _ANSI_ARGS_((Tcl_Interp *interp,
                           char       *srcFileId,
                           char       *targetFileId));


/*-----------------------------------------------------------------------------
 * ConvertFileHandle --
 *
 * Convert a file handle to its file number. The file handle maybe one 
 * of "stdin", "stdout" or "stderr" or "fileNNN", were NNN is the file
 * number.  If the handle is invalid, -1 is returned and a error message
 * will be returned in interp->result.  This is used when the file may
 * not be currently open.
 *
 *-----------------------------------------------------------------------------
 */
static int
ConvertFileHandle (interp, handle)
    Tcl_Interp *interp;
    char       *handle;
{
    int fileId = -1;

    if (handle [0] == 's') {
        if (STREQU (handle, "stdin"))
            fileId = 0;
        else if (STREQU (handle, "stdout"))
            fileId = 1;
        else if (STREQU (handle, "stderr"))
            fileId = 2;
    } else {
       if (STRNEQU (handle, "file", 4))
           Tcl_StrToInt (&handle [4], 10, &fileId);
       if (STRNEQU (handle, "sock", 4))
           Tcl_StrToInt (&handle [4], 10, &fileId);
    }
    if (fileId < 0)
        Tcl_AppendResult (interp, "invalid file handle: ", handle,
                          (char *) NULL);
    return fileId;
}

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
}

/*-----------------------------------------------------------------------------
 * DupFileHandle --
 *   Duplicate a Tcl file handle.
 *
 * Parameters:
 *   o interp (I) - If an error occures, the error message is in result.
 *   o srcFileId (I) - The id  of the file to dup.
 *   o targetFileId (I) - The id for the new file.  NULL if any id maybe
 *     used.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
DupFileHandle (interp, srcFileId, targetFileId)
    Tcl_Interp *interp;
    char       *srcFileId;
    char       *targetFileId;
{
    Tcl_Channel srcChannel, newChannel = NULL;
    Tcl_ChannelType *channelType;
    int mode, srcFileNum, isSocket;
    int newFileNum = -1;
    off_t seekOffset = -1;

    /*
     * Get all the information about the file being duplicated.  On Unix,
     * the channels we can dup share the same file for the read and write
     * directions, so use either.
     */
    srcChannel = Tcl_GetChannel (interp, srcFileId, &mode);
    if (srcChannel == NULL)
	goto errorExit;

    if (mode & TCL_READABLE) {
        srcFileNum = TclX_ChannelFnum (srcChannel, TCL_READABLE);
    } else {
        srcFileNum = TclX_ChannelFnum (srcChannel, TCL_WRITABLE);
    }
    channelType = Tcl_GetChannelType (srcChannel);

    if (STREQU (channelType->typeName, "pipe")) {
        Tcl_AppendResult (interp, "can not \"dup\" a Tcl command pipeline ",
                          " created with the \"open\" command",
                          (char *) NULL);
	goto errorExit;
    }
    
    isSocket = STREQU (channelType->typeName, "tcp");

    /*
     * If writable, flush out the buffer.  If readable, remember were we are
     * so the we can set it up for the next stdio read to come from the same
     * place.  The location is only recorded if the file is a reqular file,
     * since you can't seek on other types of files.
     */
    if (mode  & TCL_WRITABLE) {
        if (Tcl_Flush (srcChannel) == TCL_ERROR)
            goto unixError;
    }

    if (mode & TCL_READABLE) {
        struct stat fileStat;

        if (fstat (srcFileNum, &fileStat) < 0)
            goto unixError;
        if ((fileStat.st_mode & S_IFMT) == S_IFREG) {
            seekOffset = Tcl_Tell (srcChannel);
            if (seekOffset < 0)
                goto unixError;
        }
    }

    /*
     * If a target id is specified, close that channel if its open.  Also,
     * do the actual dup now.
     */
    if (targetFileId != NULL) {
        int chkFileNum;

        newFileNum = ConvertFileHandle (interp, targetFileId);
        if (newFileNum < 0)
            goto errorExit;

        newChannel = Tcl_GetChannel (interp, targetFileId, NULL);
        if (newChannel != NULL) {
            Tcl_UnregisterChannel (interp, newChannel);
        }

        chkFileNum = dup2 (srcFileNum, newFileNum);
        if (chkFileNum < 0)
            goto unixError;
        if (chkFileNum != newFileNum) {
            Tcl_AppendResult (interp,
                              "dup: desired file number not returned",
                              (char *) NULL);
            goto errorExit;
        }
    } else {
        newFileNum = dup (srcFileNum);
        if (newFileNum < 0)
            goto unixError;
    }
    
    newChannel = TclX_SetupFileEntry (interp, newFileNum, mode, isSocket);

    if (seekOffset >= 0) {
        if (Tcl_Seek (newChannel, seekOffset, SEEK_SET) != 0)
            goto unixError;
    }
    
    if (DupChannelOptions (interp, srcChannel, newChannel) != TCL_OK)
        goto errorExit;

    Tcl_AppendResult (interp, Tcl_GetChannelName (newChannel),
                      (char *) NULL);
    return TCL_OK;

  unixError:
    Tcl_ResetResult (interp);
    Tcl_AppendResult (interp, "dup of \"", srcFileId, " failed: ",
                      Tcl_PosixError (interp), (char *) NULL);

  errorExit:
    if (newChannel != NULL) {
        Tcl_UnregisterChannel (interp, newChannel);
    } else {
        if (newFileNum >= 0)
            close (newFileNum);
    }
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * BindOpenFile --
 *   Bind a open file number of a channel.
 *
 * Parameters:
 *   o interp (I) - If an error occures, the error message is in result.
 *   o fileNumStr (I) - The string number of the open file.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
BindOpenFile (interp, fileNumStr)
    Tcl_Interp *interp;
    char       *fileNumStr;
{
    unsigned fileNum;
    int mode, nonBlocking, isSocket;
    struct stat fileStat;
    char channelName[20];
    Tcl_Channel channel = NULL;

    if (!Tcl_StrToUnsigned (fileNumStr, 0, &fileNum)) {
        Tcl_AppendResult (interp, "invalid integer file number \"", fileNumStr,
                          "\", expected unsigned integer or Tcl file id",
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Make sure file is open and determine the access mode and file type.
     */
    if (TclXOSGetOpenFileMode (fileNum, &mode, &nonBlocking) != TCL_OK)
        goto unixError;

    if (fstat (fileNum, &fileStat) < 0)
        goto unixError;

    /*
     * If its a socket but RDONLY or WRONLY, enter it as a file.  This is
     * a pipe under BSD.
     */
    isSocket = S_ISSOCK (fileStat.st_mode) &&
        (mode == (TCL_READABLE | TCL_WRITABLE)) ;

    if (isSocket)
        sprintf (channelName, "sock%d", fileNum);
    else
        sprintf (channelName, "file%d", fileNum);

    if (Tcl_GetChannel (interp, channelName, NULL) != NULL) {
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "file number \"", fileNumStr,
                          "\" is already bound to a Tcl file channel",
                          (char *) NULL);
        return TCL_ERROR;
    }
    Tcl_ResetResult (interp);

    channel = TclX_SetupFileEntry (interp, fileNum, mode, isSocket);

    /*
     * Set channel options.
     */
    if (nonBlocking) {
        if (TclX_SetChannelOption (interp,
                                   channel,
                                   TCLX_COPT_BLOCKING,
                                   TCLX_MODE_NONBLOCKING) == TCL_ERROR)
            goto errorExit;
    }
    if (isatty (fileNum)) {
        if (TclX_SetChannelOption (interp,
                                   channel,
                                   TCLX_COPT_BUFFERING,
                                   TCLX_BUFFERING_LINE) == TCL_ERROR)
            goto errorExit;
    }

    Tcl_AppendResult (interp, Tcl_GetChannelName (channel),
                      (char *) NULL);
    return TCL_OK;

  unixError:
    Tcl_AppendResult (interp, "binding open file to Tcl channel failed: ",
                      Tcl_PosixError (interp), (char *) NULL);

  errorExit:
    if (channel != NULL) {
        Tcl_UnregisterChannel (interp, channel);
    }
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_DupCmd --
 *     Implements the dup TCL command:
 *         dup fileId ?targetFileId?
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
    if ((argc < 2) || (argc > 3)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0], 
                          " fileId ?targetFileId?", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * If a number is supplied, bind it to a file handle rather than doing
     * a dup.
     */
    if (ISDIGIT (argv [1][0])) {
        if (argc != 2) {
            Tcl_AppendResult (interp, "the second argument, targetFileId, is ",
                              "not allow when binding a file number to a Tcl ",
                              "file id", (char *) NULL);
            return TCL_ERROR;
        }
        return BindOpenFile (interp, argv [1]);
    } else {
        return DupFileHandle (interp, argv [1], argv [2]);
    }
}
