/*
 * tclXwinDup.c
 *
 * Support for the dup command on Windows.
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
 * $Id: tclXwinDup.c,v 7.1 1996/09/03 23:39:10 markd Exp $
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
        Tcl_AppendResult (interp, "on MS Windows, only stdin, stdout or ",
                          "stderr maybe the dup target", (char *) NULL);
        return TCL_ERROR;
    } else {
        Tcl_AppendResult (interp, "invalid channel id: ", channelName,
                          (char *) NULL);
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
        Tcl_AppendResult (interp, "dup failed: ",
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
 *   o fileNumStr (I) - The string number of the open file.
 * Returns:
 *   The unregistered channel or NULL if an error occurs.
 *-----------------------------------------------------------------------------
 */
Tcl_Channel
TclXOSBindOpenFile (interp, fileNumStr)
    Tcl_Interp *interp;
    char       *fileNumStr;
{
    /* FIX: Can probably make something work */
    TclXNotAvailableError (interp,
                           "binding an open file to a Tcl channel");
    return NULL;
}
