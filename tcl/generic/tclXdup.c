/*
 * tclXdup.c
 *
 * Extended Tcl dup command.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1994 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXdup.c,v 3.1 1994/05/28 03:38:22 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
ConvertFileHandle _ANSI_ARGS_((Tcl_Interp *interp,
                               char       *handle));

static FILE *
DoNormalDup _ANSI_ARGS_((Tcl_Interp *interp,
                         OpenFile   *oldFilePtr));

static FILE *
DoSpecifiedDup _ANSI_ARGS_((Tcl_Interp *interp,
                            OpenFile   *oldFilePtr,
                            char       *newFileId));


/*
 *-----------------------------------------------------------------------------
 *
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
    }
    if (fileId < 0)
        Tcl_AppendResult (interp, "invalid file handle: ", handle,
                          (char *) NULL);
    return fileId;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DoNormalDup --
 *   Process a normal dup command (i.e. the new file is not specified).
 *
 * Parameters:
 *   o interp (I) - If an error occures, the error message is in result.
 *   o oldFilePtr (I) - Tcl file control block for the file to dup.
 * Returns:
 *   A pointer to the file structure for the new file, or NULL if an
 * error occured.
 *-----------------------------------------------------------------------------
 */
static FILE *
DoNormalDup (interp, oldFilePtr)
    Tcl_Interp *interp;
    OpenFile   *oldFilePtr;
{
    int    newFileId;
    FILE  *filePtr;

    newFileId = dup (fileno (oldFilePtr->f));
    if (newFileId < 0)
        goto unixError;

    filePtr = Tcl_SetupFileEntry (interp, newFileId, oldFilePtr->permissions);

    return filePtr;

unixError:
    interp->result = Tcl_PosixError (interp);
    return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DoSpecifiedDup --
 *   Process a dup command where the file is dupped to a specified fileid.
 * The new file may or be open or closed, but its better if is open 
 * if stdin, stdout or stderr are being used, otherwise the a different
 * stdio file descriptior maybe bound to these descriptors.
 *
 * Parameters:
 *   o interp (I) - If an error occures, the error message is in result.
 *   o oldFilePtr (I) - Tcl file control block for the file to dup.
 *   o targetFileId (I) - The id (handle) name for the new file.
 * Returns:
 *   A pointer to the open structure for the new file, or NULL if an
 * error occured.
 *-----------------------------------------------------------------------------
 */
static FILE *
DoSpecifiedDup (interp, oldFilePtr, targetFileId)
    Tcl_Interp *interp;
    OpenFile   *oldFilePtr;
    char       *targetFileId;
{
    int    targetFileNum = -1;
    FILE  *targetFilePtr;
    char  *mode;

    /*
     * Determine if the target file is currently open.  Also get the file
     * number for the file.  Also flush the file.
     */
    if (Tcl_GetOpenFile (interp, targetFileId, 
                         FALSE, FALSE,  /* No checking */
                         &targetFilePtr) != TCL_OK) {
        Tcl_ResetResult (interp);
        targetFilePtr = NULL;

        targetFileNum = ConvertFileHandle (interp, targetFileId);
        if (targetFileNum < 0)
            return NULL;

    } else {
        targetFileNum = fileno (targetFilePtr);
        fflush (targetFilePtr);
    }

    /*
     * If this is not one of the standard files, close it.  This will do all
     * Tcl cleanup in case its a pipeline, etc.
     */
    if ((targetFilePtr != NULL) && (targetFileNum > 2)) {
        char *argv [2];

        argv [0] = "dup";
        argv [1] = targetFileId;
        if (Tcl_CloseCmd (NULL, interp, 2, argv) != TCL_OK)
            return NULL;
        targetFilePtr = NULL;
    }

    /*
     * Duplicate the old file to the specified file id.  This functionallity
     * could be obtained with dup2 on most systems.
     */
    close (targetFileNum);
    if (fcntl (fileno (oldFilePtr->f), F_DUPFD, targetFileNum) < 0)
        goto unixError;

    /*
     * If the file is not open, setup a FILE structure and tell Tcl about it.
     */
    if (targetFilePtr == NULL) {
        targetFilePtr = Tcl_SetupFileEntry (interp, targetFileNum,
                                            oldFilePtr->permissions);
        if (targetFilePtr == NULL)
            goto unixError;
    }

    return targetFilePtr;

unixError:
    interp->result = Tcl_PosixError (interp);
    if (targetFileNum >= 0)
        close (targetFileNum);
    return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
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
 *
 * Side effects:
 *      Locates and creates an entry in the handles table
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_DupCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    OpenFile *oldFilePtr;
    FILE     *newFilePtr;
    long      seekOffset = -1;

    if ((argc < 2) || (argc > 3)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv[0], 
                          " fileId ?targetFileId?", (char *) NULL);
        return TCL_ERROR;
    }

    oldFilePtr = Tcl_GetOpenFileStruct (interp, argv[1]);
    if (oldFilePtr == NULL)
	return TCL_ERROR;

    if (oldFilePtr->numPids > 0) {
        Tcl_AppendResult (interp, "can not `dup' a pipeline", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * If writable, flush out the buffer.  If readable, remember were we are
     * so the we can set it up for the next stdio read to come from the same
     * place.  The location is only recorded if the file is a reqular file,
     * since you cann't seek on other types of files.
     */
    if (oldFilePtr->permissions  & TCL_FILE_WRITABLE) {
        if (fflush (oldFilePtr->f) != 0)
            goto unixError;
    }
    if (oldFilePtr->permissions & TCL_FILE_READABLE) {
        struct stat statBuf;
        
        if (fstat (fileno (oldFilePtr->f), &statBuf) < 0)
            goto unixError;
        if ((statBuf.st_mode & S_IFMT) == S_IFREG) {
            seekOffset = ftell (oldFilePtr->f);
            if (seekOffset < 0)
                goto unixError;
        }
    }

    /*
     * Process the dup depending on if dup-ing to a new file or a target
     * file handle.
     */
    if (argc == 2)
        newFilePtr = DoNormalDup (interp, oldFilePtr);
    else
        newFilePtr = DoSpecifiedDup (interp, oldFilePtr, argv [2]);

    if (newFilePtr == NULL)
        return TCL_ERROR;

    if (seekOffset >= 0) {
        if (fseek (newFilePtr, seekOffset, SEEK_SET) != 0)
            goto unixError;
    }
    Tcl_ResetResult (interp);
    sprintf (interp->result, "file%d", fileno (newFilePtr));
    return TCL_OK;

unixError:
    Tcl_ResetResult (interp);
    interp->result = Tcl_PosixError (interp);
    return TCL_ERROR;
}
