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
 * $Id: tclXfilecmds.c,v 2.3 1993/04/07 05:58:36 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


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
    Tcl_DString  dynBuf;
    char         buffer [128];
    char        *bufPtr, *bufEnd;
    char         prevChar;
    int          bracesDepth, inQuotes, inChar;
    FILE        *filePtr;

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

    Tcl_DStringInit (&dynBuf);

    bufPtr = buffer;
    bufEnd = buffer + sizeof (buffer) - 1;

    prevChar = '\0';
    bracesDepth = 0;
    inQuotes = FALSE;

    /*
     * Read in characters, keeping trace of if we are in the middle of a {}
     * or "" part of the list.
     */

    while (TRUE) {
        inChar = getc (filePtr);
        if (inChar == EOF) {
            if (ferror (filePtr))
                goto readError;
            break;
        }
        if (prevChar != '\\') {
            switch (inChar) {
                case '{':
                    bracesDepth++;
                    break;
                case '}':
                    if (bracesDepth == 0)
                        break;
                    bracesDepth--;
                    break;
                case '"':
                    if (bracesDepth == 0)
                        inQuotes = !inQuotes;
                    break;
            }
        }
        prevChar = inChar;
        if ((inChar == '\n') && (bracesDepth == 0) && !inQuotes)
            break;
        *bufPtr++ = inChar;
        if (bufPtr > bufEnd) {
            Tcl_DStringAppend (&dynBuf, buffer, sizeof (buffer));
            bufPtr = buffer;
        }
    }

    if (bufPtr != buffer) {
        Tcl_DStringAppend (&dynBuf, buffer, bufPtr - buffer);
    }
    if ((bracesDepth != 0) || inQuotes) {
        Tcl_AppendResult (interp, "miss-matched ",
                         (bracesDepth != 0) ? "braces" : "quote",
                         " in inputed list: ", dynBuf.string, (char *) NULL);
        goto errorExit;
    }

    /*
     * Return the string as a result or in a variable.
     */
    if (argc == 2) {
        Tcl_DStringResult (interp, &dynBuf);
    } else {
        if (Tcl_SetVar (interp, argv[2], dynBuf.string,
                        TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;

        if (feof (filePtr) && (dynBuf.length == 0))
            interp->result = "-1";
        else
            sprintf (interp->result, "%d", dynBuf.length);

        Tcl_DStringFree (&dynBuf);
    }
    return TCL_OK;

readError:
    Tcl_ResetResult (interp);
    interp->result = Tcl_PosixError (interp);
    clearerr (filePtr);

errorExit:
    Tcl_DStringFree (&dynBuf);
    return TCL_ERROR;

}

