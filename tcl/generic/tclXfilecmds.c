/*
 * tclXfilecmds.c
 *
 * Extended Tcl pipe, copyfile and lgets commands.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1993 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXfilecmds.c,v 2.4 1993/06/21 06:08:05 markd Exp markd $
 *-----------------------------------------------------------------------------
 */
/* 
 *-----------------------------------------------------------------------------
 * Note: List parsing code stolen from Tcl distribution file tclUtil.c.
 *-----------------------------------------------------------------------------
 * Copyright (c) 1987-1993 The Regents of the University of California.
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

/*
 * Prototypes of internal functions.
 */
static int
GetsListElement _ANSI_ARGS_((Tcl_Interp    *interp,
                             FILE          *filePtr,
                             Tcl_DString   *bufferPtr,
                             int           *idxPtr));

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_PipeCmd --
 *     Implements the pipe TCL command:
 *         pipe ?fileId_var_r fileId_var_w?
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Locates and creates entries in the file table
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_PipeCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    Interp    *iPtr = (Interp *) interp;
    int        fileNums [2];
    char       fileIds [12];

    if (!((argc == 1) || (argc == 3))) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0], 
                          " ?fileId_var_r fileId_var_w?", (char*) NULL);
        return TCL_ERROR;
    }

    if (pipe (fileNums) < 0) {
        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }

    if (Tcl_SetupFileEntry (interp, fileNums [0], TRUE,  FALSE) == NULL)
        goto errorExit;
    if (Tcl_SetupFileEntry (interp, fileNums [1], FALSE, TRUE) == NULL)
        goto errorExit;

    if (argc == 1)      
        sprintf (interp->result, "file%d file%d", fileNums [0], fileNums [1]);
    else {
        sprintf (fileIds, "file%d", fileNums [0]);
        if (Tcl_SetVar (interp, argv[1], fileIds, TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;

        sprintf (fileIds, "file%d", fileNums [1]);
        if (Tcl_SetVar (interp, argv[2], fileIds, TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;
    }
        
    return TCL_OK;

  errorExit:
    Tcl_CloseForError (interp, fileNums [0]);
    Tcl_CloseForError (interp, fileNums [1]);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CopyfileCmd --
 *     Implements the copyfile TCL command:
 *         copyfile fromFileId1 fromFileId
 *
 * Results:
 *      Nothing if it worked, else an error.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_CopyfileCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    FILE  *fromFilePtr, *toFilePtr;
    char   transferBuffer [2048];
    int    bytesRead;

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " fromFileId toFileId", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetOpenFile (interp, argv[1],
                         FALSE,  /* Read access  */
                         TRUE,   /* Check access */  
                         &fromFilePtr) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetOpenFile (interp, argv[2],
                         TRUE,   /* Write access */
                         TRUE,   /* Check access */
                         &toFilePtr) != TCL_OK)
        return TCL_ERROR;

    while (TRUE) {
        bytesRead = fread (transferBuffer, sizeof (char), 
                           sizeof (transferBuffer), fromFilePtr);
        if (bytesRead <= 0) {
            if (feof (fromFilePtr))
                break;
            else
                goto unixError;
        }
        if (fwrite (transferBuffer, sizeof (char), bytesRead, toFilePtr) != 
                    bytesRead)
            goto unixError;
    }

    return TCL_OK;

unixError:
    interp->result = Tcl_PosixError (interp);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * GetsListElement --
 *
 *   Parse through a line read from a file for a list element.  If the end of
 * the string is reached while still in the list element, read another line.
 *
 * Paramaters:
 *   o interp (I) - Errors are returned in result.
 *   o filePtr (I) - The file to read from.
 *   o bufferPtr (I) - Buffer that file is read into.  The first line of the
 *     list should already have been read in.
 *   o idxPtr (I/O) - Pointer to the index of the next element in the buffer.
 *     initialize to zero before the first call.
 * Returns:
 *   o TCL_OK if a element was validated but there are more in the buffer.
 *   o TCL_BREAK if the end of the list was reached.
 *   o TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
static int
GetsListElement (interp, filePtr, bufferPtr, idxPtr)
    Tcl_Interp    *interp;
    FILE          *filePtr;
    Tcl_DString   *bufferPtr;
    int           *idxPtr;
{
    register char *p;
    int openBraces = 0;
    int inQuotes = 0;
    int cnt = 0; /*??????*/

    p = bufferPtr->string + *idxPtr;

    /*
     * Skim off leading white space and check for an opening brace or
     * quote.
     */
    
    while (isascii(*p) && isspace(*p)) {
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
        cnt ++;
        if (cnt > 0x1063)
            sin (1.0);
        if (p < bufferPtr->string || p >
            (bufferPtr->string + bufferPtr->length))
            abort();  /*????*/
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
                    if ((isascii(*p) && isspace(*p)) || (*p == 0)) {
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
                    if ((isascii(*p) && isspace(*p)) || (*p == 0)) {
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

                stat = Tcl_DStringGets (filePtr, bufferPtr); 
                if (stat == TCL_ERROR)
                    goto fileError;
                
                p = bufferPtr->string + (p - oldString);
                if (stat == TCL_OK)
                    break;  /* Got some data */
                /*
                 * EOF in list error.
                 */
                if (openBraces != 0) {
                    Tcl_SetResult(interp,
                            "unmatched open brace in list read from file (at EOF)",
                            TCL_STATIC);
                    return TCL_ERROR;
                } else {
                    Tcl_SetResult(interp,
                            "unmatched open quote in list read from file (at EOF)",
                            TCL_STATIC);
                    return TCL_ERROR;
                }
            }
        }
        p++;
    }

    done:
    while (isascii(*p) && isspace(*p)) {
        p++;
    }
    *idxPtr = p - bufferPtr->string;
    return (*p == '\0') ? TCL_BREAK : TCL_OK;

    fileError:
    Tcl_AppendResult (interp, "error reading list from file: ",
                      Tcl_PosixError (interp), (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
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
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_LgetsCmd (notUsed, interp, argc, argv)
    ClientData   notUsed;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    FILE        *filePtr;
    Tcl_DString  buffer;
    int          stat, bufIdx = 0;

    if ((argc != 2) && (argc != 3)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0],
                          " fileId ?varName?", (char *) NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetOpenFile (interp, argv[1],
                         FALSE,  /* Read access  */
                         TRUE,   /* Check access */
                         &filePtr) != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * Read the list, parsing off each element until the list is read.
     * More lines are read if newlines are encountered in the middle of
     * a list.
     */
    Tcl_DStringInit (&buffer);

    stat = Tcl_DStringGets (filePtr, &buffer);
    if (stat == TCL_ERROR)
        goto readError;

    while (stat != TCL_BREAK) {
        stat = GetsListElement (interp, filePtr, &buffer, &bufIdx);
        if (stat == TCL_ERROR)
            goto errorExit;
    }

    /*
     * Return the string as a result or in a variable.
     */
    if (argc == 2) {
        Tcl_DStringResult (interp, &buffer);
    } else {
        if (Tcl_SetVar (interp, argv[2], buffer.string,
                        TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;

        if (feof (filePtr) && (buffer.length == 0))
            interp->result = "-1";
        else
            sprintf (interp->result, "%d", buffer.length);

        Tcl_DStringFree (&buffer);
    }
    return TCL_OK;

readError:
    Tcl_AppendResult (interp, "error reading list from file: ",
                      Tcl_PosixError (interp), (char *) NULL);
    clearerr (filePtr);

errorExit:
    Tcl_DStringFree (&buffer);
    return TCL_ERROR;

}

