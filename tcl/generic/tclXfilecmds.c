/*
 * tclXfilecmds.c
 *
 * Extended Tcl pipe, copyfile and lgets commands.
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
 * $Id: tclXfilecmds.c,v 5.8 1996/03/10 04:42:34 markd Exp $
 *-----------------------------------------------------------------------------
 */
/* 
 *-----------------------------------------------------------------------------
 * Note: List parsing code stolen from Tcl distribution file tclUtil.c,
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
static char *FILE_ID_NOT_AVAIL =
    "the -fileid option is not available on this system";

/*
 * Prototypes of internal functions.
 */
static int
CopyOpenFile _ANSI_ARGS_((Tcl_Interp *interp,
                          long        maxBytes,
                          Tcl_Channel inChan,
                          Tcl_Channel outChan));

static int
GetsListElement _ANSI_ARGS_((Tcl_Interp    *interp,
                             Tcl_Channel    channel,
                             Tcl_DString   *bufferPtr,
                             int           *idxPtr));

static int
TruncateByPath  _ANSI_ARGS_((Tcl_Interp  *interp,
                             char        *filePath,
                             off_t        newSize));

static int
TruncateByHandle  _ANSI_ARGS_((Tcl_Interp  *interp,
                               char        *fileHandle,
                               off_t        newSize));

/*-----------------------------------------------------------------------------
 * Tcl_PipeCmd --
 *     Implements the pipe TCL command:
 *         pipe ?fileId_var_r fileId_var_w?
 *
 * Results:
 *      Standard TCL result.
 *-----------------------------------------------------------------------------
 */
int
Tcl_PipeCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int fileNums [2];
    Tcl_File file;
    Tcl_Channel chans [2];

    chans [0] = chans [1] = NULL;

    if (!((argc == 1) || (argc == 3))) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0], 
                          " ?fileId_var_r fileId_var_w?", (char*) NULL);
        return TCL_ERROR;
    }

    if (pipe (fileNums) < 0) {
        Tcl_AppendResult (interp, "pipe creation failed: ",
                          Tcl_PosixError (interp));
        return TCL_ERROR;
    }

    chans [0] = TclX_SetupFileEntry (interp,  fileNums [0], TCL_READABLE,
                                     FALSE);
    chans [1] = TclX_SetupFileEntry (interp,  fileNums [1], TCL_WRITABLE,
                                     FALSE);

    if (argc == 1)
        Tcl_AppendResult (interp,
                          Tcl_GetChannelName (chans [0]), " ",
                          Tcl_GetChannelName (chans [1]),
                          (char *) NULL);
    else {
        if (Tcl_SetVar (interp, argv[1], 
                        Tcl_GetChannelName (chans [0]),
                        TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;

        if (Tcl_SetVar (interp, argv[2], 
                        Tcl_GetChannelName (chans [1]),
                        TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;
    }
        
    return TCL_OK;

  errorExit:
    if (chans [0] != NULL) 
        Tcl_Close (NULL, chans [0]);
    else
        close (fileNums [0]);
    if (chans [1] != NULL) 
        Tcl_Close (NULL, chans [1]);
    else
        close (fileNums [1]);
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
 * Tcl_CopyfileCmd --
 *     Implements the copyfile TCL command:
 *         copyfile ?-bytes num|-maxbytes num? -translate fromFileId toFileId
 *
 * Results:
 *      The number of bytes transfered or an error.
 *-----------------------------------------------------------------------------
 */
int
Tcl_CopyfileCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
#define TCLX_COPY_ALL        0
#define TCLX_COPY_BYTES      1
#define TCLX_COPY_MAX_BYTES  2

    Tcl_Channel inChan, outChan;
    int inTrans, outTrans;
    long totalBytesToRead, totalBytesRead;
    int argIdx, copyMode, translate, saveErrno;

    /*
     * Parse arguments.
     */
    copyMode = TCLX_COPY_ALL;
    totalBytesToRead = MAXLONG;
    translate = FALSE;

    for (argIdx = 1; (argIdx < argc) && (argv [argIdx][0] == '-'); argIdx++) {
        if (STREQU (argv [argIdx], "-bytes")) {
            copyMode = TCLX_COPY_BYTES;
            argIdx++;
            if (argIdx >= argc) {
                Tcl_AppendResult (interp,
                                  "argument required for -bytes option",
                                  (char *) NULL);
                return TCL_ERROR;
            }
            if (Tcl_GetLong (interp, argv [argIdx],
                             &totalBytesToRead) != TCL_OK)
                return TCL_ERROR;
        } else if (STREQU (argv [1], "-maxbytes")) {
            copyMode = TCLX_COPY_MAX_BYTES;
            argIdx++;
            if (argIdx >= argc) {
                Tcl_AppendResult (interp,
                                  "argument required for -maxbytes option",
                                  (char *) NULL);
                return TCL_ERROR;
            }
            if (Tcl_GetLong (interp, argv [argIdx],
                             &totalBytesToRead) != TCL_OK)
                return TCL_ERROR;
        } else if (STREQU (argv [1], "-translate")) {
            translate = TRUE;
        } else {
            Tcl_AppendResult (interp, "invalid argument \"", argv [argIdx],
                              "\", expected \"-bytes\", \"-maxbytes\", or ",
                              "\"-translate\"", (char *) NULL);
            return TCL_ERROR;
        }
    }
    
    if (argIdx != argc - 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " ?-bytes num|-maxbytes num? ?-translate? ",
                          "fromFileId toFileId", (char *) NULL);
        return TCL_ERROR;
    }


    /*
     * Get the channels.  Unless translation is enabled, save the current
     * translation mode and put the files in binary mode.
     */
    inChan = TclX_GetOpenChannel (interp, argv [argIdx], TCL_READABLE);
    if (inChan == NULL)
        return TCL_ERROR;
    outChan = TclX_GetOpenChannel (interp, argv [argIdx + 1], TCL_WRITABLE);
    if (outChan == NULL)
        return TCL_ERROR;

    if (!translate) {
        inTrans = TclX_GetChannelOption (inChan, TCLX_COPT_TRANSLATION);
        if (TclX_SetChannelOption (interp, inChan, TCLX_COPT_TRANSLATION,
                                   TCLX_TRANSLATE_BINARY) != TCL_OK)
            return TCL_ERROR;

        outTrans = TclX_GetChannelOption (outChan, TCLX_COPT_TRANSLATION);
        if (TclX_SetChannelOption (interp, outChan, TCLX_COPT_TRANSLATION,
                                   TCLX_TRANSLATE_BINARY) != TCL_OK)
            return TCL_ERROR;
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
        if (TclX_SetChannelOption (interp, inChan, TCLX_COPT_TRANSLATION,
                                   inTrans) != TCL_OK)
            return TCL_ERROR;

        if (TclX_SetChannelOption (interp, outChan, TCLX_COPT_TRANSLATION,
                                   outTrans) != TCL_OK)
            return TCL_ERROR;
    }

    if (totalBytesRead < 0) {
        Tcl_SetErrno (saveErrno);
        Tcl_AppendResult (interp, "copyfile failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Return an error if -bytes were specified and not that many were
     * available.
     */
    if ((copyMode == TCLX_COPY_BYTES) &&
        (totalBytesToRead > 0) &&
        (totalBytesRead != totalBytesToRead)) {
        sprintf (interp->result,
                 "premature EOF, %ld bytes expected, %ld bytes actually read",
                 totalBytesToRead, totalBytesRead);
        return TCL_ERROR;
    }

    sprintf (interp->result, "%ld", totalBytesRead);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * GetsListElement --
 *
 *   Parse through a line read from a file for a list element.  If the end of
 * the string is reached while still in the list element, read another line.
 *
 * Paramaters:
 *   o interp (I) - Errors are returned in result.
 *   o channel (I) - The channel to read from.
 *   o bufferPtr (I) - Buffer that file is read into.  The first line of the
 *     list should already have been read in.
 *   o idxPtr (I/O) - Pointer to the index of the next element in the buffer.
 *     initialize to zero before the first call.
 * Returns:
 *   o TCL_OK if an element was validated but there is more in the buffer.
 *   o TCL_BREAK if the end of the list was reached.
 *   o TCL_ERROR if an error occured.
 * Notes:
 *   Code is a modified version of UCB procedure tclUtil.c:TclFindElement
 *-----------------------------------------------------------------------------
 */
static int
GetsListElement (interp, channel, bufferPtr, idxPtr)
    Tcl_Interp    *interp;
    Tcl_Channel    channel;
    Tcl_DString   *bufferPtr;
    int           *idxPtr;
{
    register char *p;
    int openBraces = 0;
    int inQuotes = 0;

    p = bufferPtr->string + *idxPtr;

    /*
     * If an EOF is pending even though data was read, it indicates that
     * a newline was not read.
     */
    if (Tcl_Eof (channel))
        goto errorOrEof;

    /*
     * Skim off leading white space and check for an opening brace or
     * quote.
     */
    
    while (ISSPACE(*p)) {
        p++;
    }
    if (*p == '{') {
        openBraces = 1;
        p++;
    } else if (*p == '"') {
        inQuotes = 1;
        p++;
    }

    /*
     * Find the end of the element (either a space or a close brace or
     * the end of the string).
     */

    while (1) {
        switch (*p) {

            /*
             * Open brace: don't treat specially unless the element is
             * in braces.  In this case, keep a nesting count.
             */

            case '{':
                if (openBraces != 0) {
                    openBraces++;
                }
                break;

            /*
             * Close brace: if element is in braces, keep nesting
             * count and quit when the last close brace is seen.
             */

            case '}':
                if (openBraces == 1) {
                    char *p2;

                    p++;
                    if (ISSPACE(*p) || (*p == 0)) {
                        goto done;
                    }
                    for (p2 = p; (*p2 != 0) && (!isspace(*p2)) && (p2 < p+20);
                            p2++) {
                        /* null body */
                    }
                    Tcl_ResetResult(interp);
                    sprintf(interp->result,
                            "list element in braces followed by \"%.*s\" instead of space in list read from file",
                            p2-p, p);
                    return TCL_ERROR;
                } else if (openBraces != 0) {
                    openBraces--;
                }
                break;

            /*
             * Backslash:  skip over everything up to the end of the
             * backslash sequence.
             */

            case '\\': {
                int size;

                (void) Tcl_Backslash(p, &size);
                p += size - 1;
                break;
            }

            /*
             * Space: ignore if element is in braces or quotes;  otherwise
             * terminate element.
             */

            case ' ':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '\v':
                if ((openBraces == 0) && !inQuotes) {
                    goto done;
                }
                break;

            /*
             * Double-quote:  if element is in quotes then terminate it.
             */

            case '"':
                if (inQuotes) {
                    char *p2;

                    p++;
                    if (ISSPACE(*p) || (*p == 0)) {
                        goto done;
                    }
                    for (p2 = p; (*p2 != 0) && (!isspace(*p2)) && (p2 < p+20);
                            p2++) {
                        /* null body */
                    }
                    Tcl_ResetResult(interp);
                    sprintf(interp->result,
                            "list element in quotes followed by \"%.*s\" %s",
                            p2-p, p, "instead of space in list read from file");
                    return TCL_ERROR;
                }
                break;

            /*
             * End of line, read and append another line if in braces or
             * quotes. Replace the '\0' with the newline that was in the sting.
             * Reset scan pointer (p) in case of buffer realloc.
             */

            case 0: {
                char *oldString;
                int   stat;
                
                if ((openBraces == 0) && (inQuotes == 0))
                    goto done;

                oldString = bufferPtr->string;
                Tcl_DStringAppend (bufferPtr, "\n", -1);

                stat = Tcl_Gets (channel, bufferPtr);
                if (stat < 0)
                    goto errorOrEof;
                p = bufferPtr->string + (p - oldString);
            }
        }
        p++;
    }

  done:
    while (ISSPACE(*p)) {
        p++;
    }
    *idxPtr = p - bufferPtr->string;
    return (*p == '\0') ? TCL_BREAK : TCL_OK;

  errorOrEof:
    if (Tcl_Eof (channel)) {
        Tcl_SetResult (interp,
                       "eof encountered while reading list from channel",
                       TCL_STATIC);
    } else {
        Tcl_AppendResult (interp,
                          "error reading list from : ",
                          Tcl_PosixError (interp), (char *) NULL);
    }
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_LgetsCmd --
 *
 * Implements the `lgets' Tcl command:
 *    lgets fileId ?varName?
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *-----------------------------------------------------------------------------
 */
int
Tcl_LgetsCmd (notUsed, interp, argc, argv)
    ClientData   notUsed;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    Tcl_Channel channel;
    Tcl_DString buffer;
    int mode;
    int stat, bufIdx = 0;

    if ((argc != 2) && (argc != 3)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0],
                          " fileId ?varName?", (char *) NULL);
        return TCL_ERROR;
    }
    channel = TclX_GetOpenChannel (interp, argv [1], TCL_READABLE);
    if (channel == NULL)
        return TCL_ERROR;

    /*
     * If the channel is non-blocking, temporarily set blocking mode, since
     * the channel I/O system doesn't give control over leaving data in the
     * buffer (yet).
     */
    
    mode = TclX_GetChannelOption (channel, TCLX_COPT_BLOCKING);

    if (mode == TCLX_MODE_NONBLOCKING) {
        if (TclX_SetChannelOption (interp, channel, TCLX_COPT_BLOCKING,
                                   TCLX_MODE_BLOCKING) == TCL_ERROR)
            return TCL_ERROR;
    }

    /*
     * Read the list, parsing off each element until the list is read.
     * More lines are read if newlines are encountered in the middle of
     * a list.
     */
    Tcl_DStringInit (&buffer);

    if (Tcl_Gets (channel, &buffer) < 0) {
        if (Tcl_Eof (channel) || Tcl_InputBlocked (channel))
            goto done;
        goto readError;
    }

    do {
        stat = GetsListElement (interp, channel, &buffer, &bufIdx);
        if (stat == TCL_ERROR)
            goto errorExit;
    } while (stat == TCL_OK);

    /*
     * Return the string as a result or in a variable.
     */
  done:
    if (mode == TCLX_MODE_NONBLOCKING) {
        if (TclX_SetChannelOption (interp, channel, TCLX_COPT_BLOCKING,
                                   TCLX_MODE_NONBLOCKING) == TCL_ERROR)
            return TCL_ERROR;
    }
    if (argc == 2) {
        Tcl_DStringResult (interp, &buffer);
    } else {
        if (Tcl_SetVar (interp, argv[2], buffer.string,
                        TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;

        if (Tcl_Eof (channel) || Tcl_InputBlocked (channel)) {
            interp->result = "-1";
        } else {
            sprintf (interp->result, "%d", buffer.length);
        }
        Tcl_DStringFree (&buffer);
    }
    return TCL_OK;

  readError:
    Tcl_AppendResult (interp, "error reading list from file: ",
                      Tcl_PosixError (interp), (char *) NULL);

  errorExit:
    /* 
     * If a variable is supplied, return whatever data we have.
     */
    if (argc > 2) {
        Tcl_SetVar (interp, argv[2], buffer.string, TCL_LEAVE_ERR_MSG);
    }
    Tcl_DStringFree (&buffer);
    if (mode == TCLX_MODE_NONBLOCKING) {
        if (TclX_SetChannelOption (interp, channel, TCLX_COPT_BLOCKING,
                                   TCLX_MODE_NONBLOCKING) == TCL_ERROR)
            return TCL_ERROR;
    }
    return TCL_ERROR;
}


/*-----------------------------------------------------------------------------
 * Tcl_FrenameCmd --
 *     Implements the frename TCL command:
 *         frename oldPath newPath
 *
 * Results:
 *      Standard TCL result.
 *-----------------------------------------------------------------------------
 */
int
Tcl_FrenameCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    Tcl_DString tildeBuf1, tildeBuf2;
    char *oldPath, *newPath;

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " oldPath newPath", (char *) NULL);
        return TCL_ERROR;
    }

    Tcl_DStringInit (&tildeBuf1);
    Tcl_DStringInit (&tildeBuf2);
    
    oldPath = argv [1];
    if (oldPath [0] == '~') {
        oldPath = Tcl_TranslateFileName (interp, oldPath, &tildeBuf1);
        if (oldPath == NULL)
            goto errorExit;
    }

    newPath = argv [2];
    if (newPath [0] == '~') {
        newPath = Tcl_TranslateFileName (interp, newPath, &tildeBuf2);
        if (newPath == NULL)
            goto errorExit;
    }

    if (rename (oldPath, newPath) != 0) {
        Tcl_AppendResult (interp, "rename \"", argv [1], "\" to \"", argv [2],
                          "\" failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    
    Tcl_DStringFree (&tildeBuf1);
    Tcl_DStringFree (&tildeBuf2);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&tildeBuf1);
    Tcl_DStringFree (&tildeBuf2);
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
    Tcl_DString  tildeBuf;

    Tcl_DStringInit (&tildeBuf);

    filePath = Tcl_TranslateFileName (interp, filePath, &tildeBuf);
    if (filePath == NULL) {
        Tcl_DStringFree (&tildeBuf);
        return TCL_ERROR;
    }
    if (truncate (filePath, newSize) != 0) {
        Tcl_AppendResult (interp, filePath, ": ", Tcl_PosixError (interp), (char *) NULL);
        Tcl_DStringFree (&tildeBuf);
        return TCL_ERROR;
    }

    Tcl_DStringFree (&tildeBuf);
    return TCL_OK;
#else
    Tcl_AppendResult (interp, "truncating files by path is not available on this system",
                      (char *) NULL);
    return TCL_ERROR;
#endif
}

/*-----------------------------------------------------------------------------
 * TruncateByHandle --
 * 
 *  Truncate a file via a open file handle, if this is available on this
 * system.
 *
 * Parameters:
 *   o interp (I) - Error messages are returned in the interpreter.
 *   o fileHandle (I) - Path to file.
 *   o newSize (I) - Size to truncate the file to.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
TruncateByHandle (interp, fileHandle, newSize)
    Tcl_Interp  *interp;
    char        *fileHandle;
    off_t        newSize;
{
#if (!defined(NO_FTRUNCATE)) || defined(HAVE_CHSIZE)
    int fileNum, stat;

    fileNum = TclX_GetOpenFnum (interp, fileHandle, TCL_WRITABLE);
    if (fileNum < 0)
        return TCL_ERROR;

#ifndef NO_FTRUNCATE
    stat = ftruncate (fileNum, newSize);
#else
    stat = chsize (fileNum, newSize);
#endif
    if (stat != 0) {
        Tcl_AppendResult (interp, fileHandle, ": ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
#else
    Tcl_AppendResult (interp, FILE_ID_NOT_AVAIL, (char *) NULL);
    return TCL_ERROR;
#endif
}

/*-----------------------------------------------------------------------------
 *
 * Tcl_FtruncateCmd --
 *     Implements the Tcl ftruncate command:
 *     ftruncate [-fileid] file newsize
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_FtruncateCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int    argIdx, fileIds;
    off_t  newSize;

    fileIds = FALSE;
    for (argIdx = 1; (argIdx < argc) && (argv [argIdx] [0] == '-'); argIdx++) {
        if (STREQU (argv [argIdx], FILE_ID_OPT)) {
            fileIds = TRUE;
        } else {
            Tcl_AppendResult (interp, "Invalid option \"", argv [argIdx],
                              "\", expected \"", FILE_ID_OPT, "\"",
                              (char *) NULL);
            return TCL_ERROR;
        }
    }

    if (argIdx != argc - 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " [-fileid] file newsize", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetOffset (interp,
                       argv [argIdx + 1],
                       &newSize) != TCL_OK)
        return TCL_ERROR;


    if (fileIds) {
        return TruncateByHandle (interp, argv [argIdx], newSize);
    } else {
        return TruncateByPath (interp, argv [argIdx], newSize);
    }
}

/*-----------------------------------------------------------------------------
 *
 * Tcl_ReaddirCmd --
 *     Implements the rename TCL command:
 *         readdir dirPath
 *
 * Results:
 *      Standard TCL result.
 *-----------------------------------------------------------------------------
 */
int
Tcl_ReaddirCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    Tcl_DString    tildeBuf;
    char          *dirPath;
    DIR           *dirPtr;
    struct dirent *entryPtr;

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " dirPath", (char *) NULL);
        return TCL_ERROR;
    }

    Tcl_DStringInit (&tildeBuf);

    dirPath = argv [1];
    if (dirPath [0] == '~') {
        dirPath = Tcl_TranslateFileName (interp, dirPath, &tildeBuf);
        if (dirPath == NULL)
            goto errorExit;
    }

    dirPtr = opendir (dirPath);
    if (dirPtr == NULL)  {
        Tcl_AppendResult (interp, dirPath, ": ", Tcl_PosixError (interp),
                          (char *) NULL);
        goto errorExit;
    }

    while (TRUE) {
        entryPtr = readdir (dirPtr);
        if (entryPtr == NULL)
            break;
        if (entryPtr->d_name [0] == '.') {
            if (entryPtr->d_name [1] == '\0')
                continue;
            if ((entryPtr->d_name [1] == '.') &&
                (entryPtr->d_name [2] == '\0'))
                continue;
        }
        Tcl_AppendElement (interp, entryPtr->d_name);
    }
    closedir (dirPtr);
    Tcl_DStringFree (&tildeBuf);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&tildeBuf);
    return TCL_ERROR;
}

