/*
 * tclXwinDup.c
 *
 * Support for the dup command on Windows.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1997 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id:$
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*-----------------------------------------------------------------------------
 * ConvertChannelName --
 *
 *   Convert a requested channel name to one of the standard channel ids.
 * 
 * Parameters:
 *   o interp - Errors are returned in result.
 *   o channelName - Desired channel, one of "stdin", "stdout" or "stderr".
 *   o handleIdPtr - One of STD_{INPUT|OUTPUT|ERROR}_HANDLE is returned.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 * FIX: Make Unix version parallel this one.
 *-----------------------------------------------------------------------------
 */
static int
ConvertChannelName (Tcl_Interp *interp,
                    char       *channelName,
                    DWORD      *handleIdPtr)
{
    if (channelName [0] == 's') {
        if (STREQU (channelName, "stdin"))
            *handleIdPtr = STD_INPUT_HANDLE;
        else if (STREQU (channelName, "stdout"))
            *handleIdPtr = STD_OUTPUT_HANDLE;
        else if (STREQU (channelName, "stderr"))
            *handleIdPtr = STD_ERROR_HANDLE;
    } else if (STRNEQU (channelName, "file", 4) ||
               STRNEQU (channelName, "sock", 4)) {
        TclX_StringAppendObjResult (interp, "on MS Windows, only stdin, ",
                                    "stdout, or stderr maybe the dup target",
                                    (char *) NULL);
        return TCL_ERROR;
    } else {
        TclX_StringAppendObjResult (interp, "invalid channel id: ",
                                    channelName, (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSDupChannel --
 *   OS dependent duplication of a channel.
 *
 * Parameters:
 *   o interp (I) - If an error occures, the error message is in result.
 *   o srcChannel (I) - The channel to dup.
 *   o mode (I) - The channel mode.
 *   o targetChannelId (I) - The id for the new file.  NULL if any id maybe
 *     used.
 * Returns:
 *   The unregistered new channel, or NULL if an error occured.
 *-----------------------------------------------------------------------------
 */
Tcl_Channel
TclXOSDupChannel (interp, srcChannel, mode, targetChannelId)
    Tcl_Interp *interp;
    Tcl_Channel srcChannel;
    int         mode;
    char       *targetChannelId;
{
    Tcl_File channelFile;
    Tcl_Channel newChannel = NULL;
    int type;
    HANDLE srcFileHand, newFileHand = NULL;

    /*
     * On Windows, the channels we can dup share the same file for the read and
     * write directions, so use either.
     */
    if (mode & TCL_READABLE) {
        channelFile = Tcl_GetChannelFile (srcChannel, TCL_READABLE);
    } else {
        channelFile = Tcl_GetChannelFile (srcChannel, TCL_WRITABLE);
    }
    srcFileHand = (HANDLE) Tcl_GetFileInfo (channelFile, &type);
    
    if (type == TCL_WIN_SOCKET) {
        TclXNotAvailableError (interp,
                               "duping a socket");
        goto errorExit;
    }
    
    /*
     * Duplicate the channel's file.
     */
    if (!DuplicateHandle (GetCurrentProcess (),
                          srcFileHand,
                          GetCurrentProcess (),
                          &newFileHand,
                          0, FALSE,
                          DUPLICATE_SAME_ACCESS)) {
	TclWinConvertError (GetLastError ());
        TclX_StringAppendObjResult (interp, "dup failed: ",
                                    Tcl_PosixError (interp), (char *) NULL);
        goto errorExit;
    }

    /*
     * If a standard target channel is specified, close the target if its open
     * and make the new channel one of the std channels.
     */
    if (targetChannelId != NULL) {
        Tcl_Channel oldChannel;
        DWORD stdHandleId;

        if (ConvertChannelName (interp, targetChannelId,
                                &stdHandleId) != TCL_OK)
            goto errorExit;

        oldChannel = Tcl_GetChannel (interp, targetChannelId, NULL);
        if (oldChannel != NULL) {
            Tcl_UnregisterChannel (interp, oldChannel);
        }
        SetStdHandle (stdHandleId, newFileHand);
     }
    
    newChannel = Tcl_MakeFileChannel ((ClientData) newFileHand,
                                      (ClientData) newFileHand,
                                      mode);
    return newChannel;

  errorExit:
    if (newFileHand != NULL)
        CloseHandle (newFileHand);
    return NULL;
}

/*-----------------------------------------------------------------------------
 * TclXOSBindOpenFile --
 *   Bind a open file number of a channel.
 *
 * Parameters:
 *   o interp (I) - If an error occures, the error message is in result.
 *   o fileNum (I) - The file number of the open file.
 * Returns:
 *   The unregistered channel or NULL if an error occurs.
 *-----------------------------------------------------------------------------
 */
Tcl_Channel
TclXOSBindOpenFile (interp, fileNum)
    Tcl_Interp *interp;
    int         fileNum;
{
    unsigned fileNum;
    HANDLE fileHandle;
    DWORD fileType;
    int mode, isSocket;
    char channelName[20];
    Tcl_Channel channel = NULL;

    if (!Tcl_StrToUnsigned (fileNumStr, 0, &fileNum)) {
        Tcl_AppendResult (interp, "invalid integer file number \"", fileNumStr,
                          "\", expected unsigned integer or Tcl file id",
                          (char *) NULL);
        return NULL;
    }

    /*
     * Make sure file is open and determine the access mode and file type.
     * Currently, we just make sure it's open, and assume both read and write.
     * FIX: find an API under Windows that returns the read/write info.
     */
    fileHandle = (HANDLE) fileNum;
    fileType = GetFileType (fileHandle);
    if (fileType == FILE_TYPE_UNKNOWN) {
        TclWinConvertError (GetLastError ());
        goto posixError;
    }
    mode = TCL_READABLE | TCL_WRITABLE;

    /*
     * FIX: we don't deal with sockets.
     */
    isSocket = 0;

    if (isSocket)
        sprintf (channelName, "sock%d", fileNum);
    else
        sprintf (channelName, "file%d", fileNum);

    if (Tcl_GetChannel (interp, channelName, NULL) != NULL) {
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "file number \"", fileNumStr,
                          "\" is already bound to a Tcl file channel",
                          (char *) NULL);
        return NULL;
    }
    Tcl_ResetResult (interp);

    if (isSocket) {
        channel = Tcl_MakeTcpClientChannel ((ClientData) fileNum);
    } else {
        channel = Tcl_MakeFileChannel ((ClientData) fileNum,
                                       (ClientData) fileNum,
                                       mode);
    }
    Tcl_RegisterChannel (interp, channel);

    return channel;

  posixError:
    Tcl_AppendResult (interp, "binding open file ", fileNumStr,
                      " to Tcl channel failed: ", Tcl_PosixError (interp),
                      (char *) NULL);

    if (channel != NULL) {
        Tcl_UnregisterChannel (interp, channel);
    }
    return NULL;
}


