/*
 * tclXfilecmds.c
 *
 * Extended Tcl pipe and copyfile commands.
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
 * $Id: tclXfilecmds.c,v 8.8 1997/06/30 07:57:45 markd Exp $
 *-----------------------------------------------------------------------------
 */
/* 
 *-----------------------------------------------------------------------------
 * Note: The list parsing code is from Tcl distribution file tclUtil.c,
 * procedure TclFindElement.
 *-----------------------------------------------------------------------------
 * Copyright (c) 1987-1994 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

static char *FILE_ID_OPT = "-fileid";
static char *TCL_TRANSLATION_OPT = "-translation";
static char *TCL_EOFCHAR_OPT = "-eofchar";

/*
 * Prototypes of internal functions.
 */
static int
CopyOpenFile _ANSI_ARGS_((Tcl_Interp *interp,
                          long        maxBytes,
                          Tcl_Channel inChan,
                          Tcl_Channel outChan));

static int
TruncateByPath  _ANSI_ARGS_((Tcl_Interp  *interp,
                             char        *filePath,
                             off_t        newSize));

static int
ReadDirCallback _ANSI_ARGS_((Tcl_Interp  *interp,
                             char        *path,
                             char        *fileName,
                             int          caseSensitive,
                             ClientData   clientData));

static int 
TclX_PipeObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

static int 
TclX_CopyfileObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));

static int
TclX_FtruncateObjCmd _ANSI_ARGS_((ClientData clientData, 
                                 Tcl_Interp   *interp, 
				 int           objc,
				 Tcl_Obj      *CONST objv[]));

static int
TclX_ReaddirObjCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));


/*-----------------------------------------------------------------------------
 * Tcl_PipeObjCmd --
 *     Implements the pipe TCL command:
 *         pipe ?fileId_var_r fileId_var_w?
 *
 * Results:
 *      Standard TCL result.
 *-----------------------------------------------------------------------------
 */
int
TclX_PipeObjCmd (clientData, interp, objc, objv)
     ClientData  clientData;
     Tcl_Interp *interp;
     int         objc;
     Tcl_Obj    *CONST objv[];
{
    Tcl_Channel  channels [2];
    char        *channelNames [2];

    if (!((objc == 1) || (objc == 3)))
	return TclX_WrongArgs (interp, objv [0], "?fileId_var_r fileId_var_w?");

    if (TclXOSpipe (interp, channels) != TCL_OK)
        return TCL_ERROR;


    channelNames [0] = Tcl_GetChannelName (channels [0]);
    channelNames [1] = Tcl_GetChannelName (channels [1]);
    
    if (objc == 1) {
        TclX_AppendResult (interp, channelNames [0], " ",
                           channelNames [1], (char *) NULL);
    } else {
        if (Tcl_ObjSetVar2 (interp, objv [1], (Tcl_Obj *) NULL,
                            Tcl_NewStringObj (channelNames [0], -1),
                            TCL_PARSE_PART1 | TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;

        if (Tcl_ObjSetVar2 (interp, objv [2], (Tcl_Obj *) NULL,
                            Tcl_NewStringObj (channelNames [1], -1),
                            TCL_PARSE_PART1 | TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;
    }

    return TCL_OK;

  errorExit:
    Tcl_Close (NULL, channels [0]);
    Tcl_Close (NULL, channels [1]);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * CopyOpenFile --
 * 
 *  Utility function to copy an open file to another open file.  Handles
 * non-blocking I/O in the same manner as gets.  It doesn't return an
 * error when EWOULDBLOCK or EAGAIN is returned if some data has been read.
 *
 * Parameters:
 *   o interp (I) - Error messages are returned in the interpreter.
 *   o maxBytes (I) - Maximum number of bytes to copy.
 *   o inChan (I) - Input channel.
 *   o outChan (I) - Output channel.
 * Returns:
 *    The number of bytes transfered or -1 on an error with the error in
 * errno.
 *-----------------------------------------------------------------------------
 */
static int
CopyOpenFile (interp, maxBytes, inChan, outChan)
    Tcl_Interp *interp;
    long        maxBytes;
    Tcl_Channel inChan;
    Tcl_Channel outChan;
{
    char   buffer [2048];
    long   bytesToRead, bytesRead, totalBytesRead, bytesLeft;

    bytesLeft = maxBytes;
    totalBytesRead = 0;

    while (bytesLeft > 0) {
        bytesToRead = sizeof (buffer);
        if (bytesToRead > bytesLeft)
            bytesToRead = bytesLeft;

        bytesRead = Tcl_Read (inChan, buffer, bytesToRead);
        if (bytesRead <= 0) {
            if (Tcl_Eof (inChan) || Tcl_InputBlocked (inChan)) {
                break;
            }
            return -1;
        }
        if (Tcl_Write (outChan, buffer, bytesRead) != bytesRead)
            return -1;

        bytesLeft -= bytesRead;
        totalBytesRead += bytesRead;
    }

    if (Tcl_Flush (outChan) == TCL_ERROR)
        return -1;

    return totalBytesRead;
}

/*-----------------------------------------------------------------------------
 * Tcl_CopyfileObjCmd --
 *     Implements the copyfile TCL command:
 *         copyfile ?-bytes num|-maxbytes num? -translate fromFileId toFileId
 *
 * Results:
 *      The number of bytes transfered or an error.
 *-----------------------------------------------------------------------------
 */
int
TclX_CopyfileObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj     *CONST objv[];
{
#define TCLX_COPY_ALL        0
#define TCLX_COPY_BYTES      1
#define TCLX_COPY_MAX_BYTES  2

    Tcl_Channel    inChan, outChan;
    long           totalBytesToRead, totalBytesRead;
    int            objIdx, copyMode, translate;
    int            saveErrno = 0;
    Tcl_DString    inChanTrans, outChanTrans;
    Tcl_DString    inChanEOF, outChanEOF;
    char          *switchString;

    /*
     * Parse arguments.
     */
    copyMode = TCLX_COPY_ALL;
    totalBytesToRead = MAXLONG;
    translate = FALSE;

    for (objIdx = 1; objIdx < objc; objIdx++) {
	switchString = Tcl_GetStringFromObj (objv [objIdx], NULL);
	if (*switchString != '-')
            break;
        if (STREQU (switchString, "-bytes")) {
            copyMode = TCLX_COPY_BYTES;
            objIdx++;
            if (objIdx >= objc) {
                TclX_AppendResult (interp,
                                   "argument required for -bytes option",
                                   (char *) NULL);
                return TCL_ERROR;
            }
	    if (Tcl_GetLongFromObj (interp, objv [objIdx],
                                   &totalBytesToRead) != TCL_OK) 
		    return TCL_ERROR;
        } else if (STREQU (switchString, "-maxbytes")) {
            copyMode = TCLX_COPY_MAX_BYTES;
            objIdx++;
            if (objIdx >= objc) {
                TclX_AppendResult (interp,
                                  "argument required for -maxbytes option",
                                  (char *) NULL);
                return TCL_ERROR;
            }
	    if (Tcl_GetLongFromObj (interp, objv [objIdx], &totalBytesToRead)
		!= TCL_OK)
                return TCL_ERROR;
        } else if (STREQU (switchString, "-translate")) {
            translate = TRUE;
        } else {
            TclX_AppendResult (interp, "invalid argument \"", 
                               Tcl_GetStringFromObj (objv [objIdx], NULL),
                              "\", expected \"-bytes\", \"-maxbytes\", or ",
                              "\"-translate\"", (char *) NULL);
            return TCL_ERROR;
        }
    }
    
    if (objIdx != objc - 2) {
        TclX_WrongArgs (interp, objv [0], 
	    "?-bytes num|-maxbytes num? ?-translate? fromFileId toFileId");
        return TCL_ERROR;
    }

    Tcl_DStringInit (&inChanTrans);
    Tcl_DStringInit (&outChanTrans);
    Tcl_DStringInit (&inChanEOF);
    Tcl_DStringInit (&outChanEOF);

    /*
     * Get the channels.  Unless translation is enabled, save the current
     * translation mode and put the files in binary mode and clear the
     * EOF character.
     */
    inChan = TclX_GetOpenChannelObj (interp, objv [objIdx], TCL_READABLE);
    if (inChan == NULL)
        goto errorExit;
    outChan = TclX_GetOpenChannelObj (interp, objv [objIdx + 1], TCL_WRITABLE);
    if (outChan == NULL)
        goto errorExit;

    if (!translate) {
        Tcl_GetChannelOption (inChan, TCL_TRANSLATION_OPT, &inChanTrans);
        Tcl_GetChannelOption (outChan, TCL_TRANSLATION_OPT, &outChanTrans);
        Tcl_GetChannelOption (inChan, TCL_EOFCHAR_OPT, &inChanEOF);
        Tcl_GetChannelOption (outChan, TCL_EOFCHAR_OPT, &outChanEOF);
        
        if (Tcl_SetChannelOption (interp, inChan, TCL_TRANSLATION_OPT,
                                  "binary") != TCL_OK)
            goto errorExit;
        if (Tcl_SetChannelOption (interp, outChan, TCL_TRANSLATION_OPT,
                                  "binary") != TCL_OK)
            goto errorExit;
        if (Tcl_SetChannelOption (interp, inChan, TCL_EOFCHAR_OPT,
                                  "") != TCL_OK)
            goto errorExit;
        if (Tcl_SetChannelOption (interp, outChan, TCL_EOFCHAR_OPT,
                                  "") != TCL_OK)
            goto errorExit;
    }

    /*
     * Copy the file, it an error occurs, save it until translation is
     * restored.
     */
    totalBytesRead = CopyOpenFile (interp,
                                   totalBytesToRead,
                                   inChan, outChan);
    if (totalBytesRead < 0)
        saveErrno = Tcl_GetErrno ();

    if (!translate) {
        if (Tcl_SetChannelOption (interp, inChan, TCL_TRANSLATION_OPT,
                                  inChanTrans.string) != TCL_OK)
            goto errorExit;
        if (Tcl_SetChannelOption (interp, outChan, TCL_TRANSLATION_OPT,
                                  inChanTrans.string) != TCL_OK)
            goto errorExit;
        if (Tcl_SetChannelOption (interp, inChan, TCL_EOFCHAR_OPT,
                                  inChanEOF.string) != TCL_OK)
            goto errorExit;
        if (Tcl_SetChannelOption (interp, outChan, TCL_EOFCHAR_OPT,
                                  outChanEOF.string) != TCL_OK)
            goto errorExit;
    }

    if (totalBytesRead < 0) {
        Tcl_SetErrno (saveErrno);
        Tcl_AppendResult (interp, "copyfile failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        goto errorExit;
    }

    /*
     * Return an error if -bytes were specified and not that many were
     * available.
     */
    if ((copyMode == TCLX_COPY_BYTES) &&
        (totalBytesToRead > 0) &&
        (totalBytesRead != totalBytesToRead)) {

	/* FIX: if there is ever a printf-style object text appender,
	 * convert this code to use it.
	 */

	char bytesToReadString[16];
	char totalBytesReadString[16];

	sprintf (bytesToReadString, "%ld", totalBytesToRead);
	sprintf (totalBytesReadString, "%ld", totalBytesRead);

	TclX_AppendResult (interp, "premature EOF, ", bytesToReadString,
                           " bytes expected, ", totalBytesReadString,
                           " bytes actually read", (char *)NULL);
        goto errorExit;
    }

    Tcl_SetIntObj (Tcl_GetObjResult (interp), totalBytesRead);
    Tcl_DStringFree (&inChanTrans);
    Tcl_DStringFree (&outChanTrans);
    Tcl_DStringFree (&inChanEOF);
    Tcl_DStringFree (&outChanEOF);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&inChanTrans);
    Tcl_DStringFree (&outChanTrans);
    Tcl_DStringFree (&inChanEOF);
    Tcl_DStringFree (&outChanEOF);

    return TCL_ERROR;
}


/*-----------------------------------------------------------------------------
 * TruncateByPath --
 * 
 *  Truncate a file via path, if this is available on this system.
 *
 * Parameters:
 *   o interp (I) - Error messages are returned in the interpreter.
 *   o filePath (I) - Path to file.
 *   o newSize (I) - Size to truncate the file to.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
TruncateByPath (interp, filePath, newSize)
    Tcl_Interp  *interp;
    char        *filePath;
    off_t        newSize;
{
#ifndef NO_TRUNCATE
    Tcl_DString  pathBuf;

    Tcl_DStringInit (&pathBuf);

    filePath = Tcl_TranslateFileName (interp, filePath, &pathBuf);
    if (filePath == NULL) {
        Tcl_DStringFree (&pathBuf);
        return TCL_ERROR;
    }
    if (truncate (filePath, newSize) != 0) {
        TclX_AppendResult (interp, filePath, ": ", Tcl_PosixError (interp),
                           (char *) NULL);
        Tcl_DStringFree (&pathBuf);
        return TCL_ERROR;
    }

    Tcl_DStringFree (&pathBuf);
    return TCL_OK;
#else
    Tcl_AppendResult (interp, "truncating files by path is not available on this system",
                      (char *) NULL);
    return TCL_ERROR;
#endif
}

/*-----------------------------------------------------------------------------
 * Tcl_FtruncateObjCmd --
 *     Implements the Tcl ftruncate command:
 *     ftruncate [-fileid] file newsize
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
TclX_FtruncateObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj     *CONST objv[];
{
    int           objIdx, fileIds;
    off_t         newSize;
    long          convSize;
    Tcl_Channel   channel;
    char         *switchString;
    char         *pathString;

    fileIds = FALSE;
    for (objIdx = 1; objIdx < objc ; objIdx++) {
        switchString = Tcl_GetStringFromObj (objv [objIdx], NULL);
	if (*switchString != '-')
            break;
        if (STREQU (switchString, FILE_ID_OPT)) {
            fileIds = TRUE;
        } else {
            TclX_AppendResult (interp, "Invalid option \"", switchString,
                               "\", expected \"", FILE_ID_OPT, "\"",
                               (char *) NULL);
            return TCL_ERROR;
        }
    }

    if (objIdx != objc - 2)
        return TclX_WrongArgs (interp, objv [0], "[-fileid] file newsize");

    if (Tcl_GetLongFromObj (interp, objv [objIdx + 1], &convSize) != TCL_OK)
        return TCL_ERROR;

    newSize = convSize;
    if (fileIds) {
        channel = TclX_GetOpenChannelObj (interp, objv [objIdx], 0);
        if (channel == NULL)
            return TCL_ERROR;
        return TclXOSftruncate (interp, channel, newSize,
                                "-fileid option");
    } else {
	pathString = Tcl_GetStringFromObj (objv [objIdx], NULL);
        return TruncateByPath (interp, pathString, newSize);
    }
}

/*-----------------------------------------------------------------------------
 * ReadDirCallback --
 *
 *   Callback procedure for walking directories.
 * Parameters:
 *   o interp (I) - Interp is passed though.
 *   o path (I) - Normalized path to directory.
 *   o fileName (I) - Tcl normalized file name in directory.
 *   o caseSensitive (I) - Are the file names case sensitive?  Always
 *     TRUE on Unix.
 *   o clientData (I) - Tcl_DString to append names to.
 * Returns:
 *   TCL_OK.
 *-----------------------------------------------------------------------------
 */
static int
ReadDirCallback (interp, path, fileName, caseSensitive, clientData)
    Tcl_Interp  *interp;
    char        *path;
    char        *fileName;
    int          caseSensitive;
    ClientData   clientData;
{
    Tcl_Obj *fileListObj = (Tcl_Obj *) clientData;
    Tcl_Obj *fileNameObj;
    int      result;

    fileNameObj = Tcl_NewStringObj (fileName, -1);
    result = Tcl_ListObjAppendElement (interp, fileListObj, fileNameObj);
    return result;
}

/*-----------------------------------------------------------------------------
 * Tcl_ReaddirObjCmd --
 *     Implements the rename TCL command:
 *         readdir ?-hidden? dirPath
 *
 * Results:
 *      Standard TCL result.
 *-----------------------------------------------------------------------------
 */
int
TclX_ReaddirObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj    *CONST objv[];
{
    Tcl_DString  pathBuf;
    char        *dirPath;
    int          hidden, status;
    Tcl_Obj     *fileListObj;
    char        *switchString;
    int          dirPathLen;
    
    if ((objc < 2) || (objc > 3))
        return TclX_WrongArgs (interp, objv [0], "?-hidden? dirPath");

    if (objc == 2) {
        dirPath = Tcl_GetStringFromObj (objv [1], &dirPathLen);
        hidden = FALSE;
    } else {
        switchString = Tcl_GetStringFromObj (objv [1], NULL);
        if (!STREQU (switchString, "-hidden")) {
            TclX_AppendResult (interp,
                               "expected option of \"-hidden\", got \"",
                               switchString, "\"", (char *) NULL);
            return TCL_ERROR;
        }
        dirPath = Tcl_GetStringFromObj (objv [2], NULL);
        hidden = TRUE;
    }

    Tcl_DStringInit (&pathBuf);

    fileListObj = Tcl_NewObj ();

    dirPath = Tcl_TranslateFileName (interp, dirPath, &pathBuf);
    if (dirPath == NULL) {
        goto errorExit;
    }

    status = TclXOSWalkDir (interp,
                            dirPath,
                            hidden,
                            ReadDirCallback,
                            (ClientData) fileListObj);
    if (status == TCL_ERROR)
        goto errorExit;

    Tcl_DStringFree (&pathBuf);
    Tcl_SetObjResult (interp, fileListObj);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&pathBuf);
    Tcl_DecrRefCount (fileListObj);
    return TCL_ERROR;
}


/*-----------------------------------------------------------------------------
 * TclX_FilecmdsInit --
 *     Initialize the file commands.
 *-----------------------------------------------------------------------------
 */
void
TclX_FilecmdsInit (interp)
    Tcl_Interp *interp;
{
    Tcl_CreateObjCommand (interp,
			  "pipe",
			  TclX_PipeObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "copyfile",
			  TclX_CopyfileObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
			  "ftruncate",
			  TclX_FtruncateObjCmd,
			  (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand (interp,
                          "readdir",
			  TclX_ReaddirObjCmd,
                          (ClientData) NULL,
			  (Tcl_CmdDeleteProc*) NULL);
}

