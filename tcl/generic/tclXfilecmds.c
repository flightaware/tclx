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
 * $Id: tclXfilecmds.c,v 2.2 1993/04/03 23:23:43 markd Exp markd $
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
        interp->result = Tcl_UnixError (interp);
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
    OpenFile  *fromFilePtr, *toFilePtr;
    char       transferBuffer [2048];
    int        bytesRead;

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " fromFileId toFileId", (char *) NULL);
        return TCL_ERROR;
    }

    if (TclGetOpenFile (interp, argv[1], &fromFilePtr) != TCL_OK)
	return TCL_ERROR;
    if (TclGetOpenFile (interp, argv[2], &toFilePtr) != TCL_OK)
	return TCL_ERROR;

    if (!fromFilePtr->readable) {
        interp->result = "Source file is not open for read access";
	return TCL_ERROR;
    }
    if (!toFilePtr->writable) {
        interp->result = "Target file is not open for write access";
	return TCL_ERROR;
    }

    while (TRUE) {
        bytesRead = fread (transferBuffer, sizeof (char), 
                           sizeof (transferBuffer), fromFilePtr->f);
        if (bytesRead <= 0) {
            if (feof (fromFilePtr->f))
                break;
            else
                goto unixError;
        }
        if (fwrite (transferBuffer, sizeof (char), bytesRead, toFilePtr->f) != 
                    bytesRead)
            goto unixError;
    }

    return TCL_OK;

unixError:
    interp->result = Tcl_UnixError (interp);
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
    dynamicBuf_t  dynBuf;
    char          prevChar;
    int           bracesDepth, inQuotes, inChar;
    OpenFile     *filePtr;

    if ((argc != 2) && (argc != 3)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0],
                          " fileId ?varName?", (char *) NULL);
        return TCL_ERROR;
    }
    if (TclGetOpenFile(interp, argv[1], &filePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (!filePtr->readable) {
        Tcl_AppendResult (interp, "\"", argv[1],
                          "\" wasn't opened for reading", (char *) NULL);
        return TCL_ERROR;
    }

    Tcl_DynBufInit (&dynBuf);

    prevChar = '\0';
    bracesDepth = 0;
    inQuotes = FALSE;

    /*
     * Read in characters, keeping trace of if we are in the middle of a {}
     * or "" part of the list.
     */

    while (TRUE) {
        if (dynBuf.len + 1 == dynBuf.size)
            Tcl_ExpandDynBuf (&dynBuf, 0);
        inChar = getc (filePtr->f);
        if (inChar == EOF) {
            if (ferror (filePtr->f))
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
        dynBuf.ptr [dynBuf.len++] = inChar;
    }

    dynBuf.ptr [dynBuf.len] = '\0';

    if ((bracesDepth != 0) || inQuotes) {
        Tcl_AppendResult (interp, "miss-matched ",
                         (bracesDepth != 0) ? "braces" : "quote",
                         " in inputed list: ", dynBuf.ptr, (char *) NULL);
        goto errorExit;
    }

    if (argc == 2) {
        Tcl_DynBufReturn (interp, &dynBuf);
    } else {
        if (Tcl_SetVar (interp, argv[2], dynBuf.ptr, 
                        TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;
        if (feof (filePtr->f) && (dynBuf.len == 0))
            interp->result = "-1";
        else
            sprintf (interp->result, "%d", dynBuf.len);
        Tcl_DynBufFree (&dynBuf);
    }
    return TCL_OK;

readError:
    Tcl_ResetResult (interp);
    interp->result = Tcl_UnixError (interp);
    clearerr (filePtr->f);
    goto errorExit;

errorExit:
    Tcl_DynBufFree (&dynBuf);
    return TCL_ERROR;

}

