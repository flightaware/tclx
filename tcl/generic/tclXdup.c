/*
 * tclXdup.c
 *
 * Extended Tcl dup command.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1995 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXdup.c,v 5.1 1995/09/05 07:55:47 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Channel options and index of the names.
 */
static char *channelOptions [] = {
    "-blocking",
    "-linemode",
    "-translation",
    NULL};
#define COPT_BLOCKING    0
#define COPT_LINEMODE    1
#define COPT_TRANSLATION 2

/*
 * Prototypes of internal functions.
 */
static int
ConvertFileHandle _ANSI_ARGS_((Tcl_Interp *interp,
                               char       *handle));

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
    int mode, srcFileNum, isSocket, idx;
    int newFileNum = -1;
    off_t seekOffset = -1;

    /*
     * Get all the information about the file being duplicated.  On Unix,
     * the channels we can dup share the same file for the read and write
     * directions, so use either.
     */
    srcChannel = Tcl_GetChannel (interp, TclSubstChannelName (srcFileId),
                                 &mode);
    if (srcChannel == NULL)
	return TCL_ERROR;

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
        return TCL_ERROR;
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
            return TCL_ERROR;

        newChannel = Tcl_GetChannel (interp,
                                     TclSubstChannelName (targetFileId),
                                     NULL);
        if (newChannel != NULL) {
            Tcl_UnregisterChannel (interp, newChannel);
        }

        close (newFileNum);
        chkFileNum = fcntl (srcFileNum, F_DUPFD, newFileNum);
        if (chkFileNum < 0)
            goto unixError;
        if (chkFileNum != newFileNum) {
            Tcl_AppendResult (interp,
                              "dup: desired file number not returned",
                              (char *) NULL);
            goto error;
        }
    } else {
        newFileNum = dup (srcFileNum);
        if (newFileNum < 0)
            goto unixError;
    }
    
    newChannel = TclX_SetupFileEntry (interp, newFileNum, mode, isSocket);
    if (newChannel == NULL)
        goto unixError;

    if (seekOffset >= 0) {
        if (Tcl_Seek (newChannel, seekOffset, SEEK_SET) != 0)
            goto unixError;
    }
    
    /*
     * Set channel options.
     */
    for (idx = 0; channelOptions [idx] != NULL; idx++) {
        if (Tcl_SetChannelOption (interp,
                                  newChannel,
                                  channelOptions [idx],
                                  Tcl_GetChannelOption (srcChannel,
                                                        channelOptions [idx]))
                                  == TCL_ERROR)
            goto error;
    }

    Tcl_AppendResult (interp, Tcl_GetChannelName (newChannel),
                      (char *) NULL);
    return TCL_OK;

  unixError:
    Tcl_ResetResult (interp);
    Tcl_AppendResult (interp, "dup of \"", srcFileId, " failed: ",
                      Tcl_PosixError (interp), (char *) NULL);
  error:
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
    unsigned  fileNum;
    int fileMode, mode, isSocket;
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
    fileMode = fcntl (fileNum, F_GETFL, 0);
    if (fileMode == -1)
        goto unixError;

    switch (fileMode & O_ACCMODE) {
      case O_RDONLY:
        mode = TCL_READABLE;
        break;
      case O_WRONLY:
        mode = TCL_WRITABLE;
        break;
      case O_RDWR:
        mode = TCL_READABLE | TCL_WRITABLE;
        break;
    }

    if (fstat (fileNum, &fileStat) < 0)
        goto unixError;
    isSocket = ((fileStat.st_mode & S_IFMT) ==  S_IFSOCK);

    if (isSocket)
        sprintf (channelName, "sock%d", fileNum);
    else
        sprintf (channelName, "file%d", fileNum);

    if (Tcl_GetChannel (interp, TclSubstChannelName (channelName),
                        NULL) != NULL) {
        Tcl_AppendResult (interp, "file number \"", fileNumStr,
                          "\" is already bound to a Tcl file channel",
                          (char *) NULL);
        return TCL_ERROR;
    }
    
    channel = TclX_SetupFileEntry (interp, fileNum, mode, isSocket);
    if (channel == NULL)
        return TCL_ERROR;

    /*
     * Set channel options.
     */
    if (Tcl_SetChannelOption (interp,
                              channel,
                              channelOptions [COPT_BLOCKING],
                              fileMode & (O_NONBLOCK | O_NDELAY)  ? "1" : "0")
        == TCL_ERROR)
        goto error;
    if (Tcl_SetChannelOption (interp,
                              channel,
                              channelOptions [COPT_LINEMODE],
                              isatty (fileNum) ? "1" : "0")
        == TCL_ERROR)
        goto error;

    Tcl_AppendResult (interp, Tcl_GetChannelName (channel),
                      (char *) NULL);
    return TCL_OK;

  unixError:
    Tcl_ResetResult (interp);
    interp->result = Tcl_PosixError (interp);

  error:
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
