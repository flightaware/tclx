/*
 * tclXfilescan.c --
 *
 * Tcl file scanning: regular expression matching on lines of a file.  
 * Implements awk.
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
 * $Id: tclXfilescan.c,v 5.1 1996/02/09 18:42:51 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * A scan context describes a collection of match patterns and commands,
 * along with a match default command to apply to a file on a scan.
 */
 
#define CONTEXT_A_CASE_INSENSITIVE_FLAG 2
#define MATCH_CASE_INSENSITIVE_FLAG 4

typedef struct matchDef_t {
    TclX_regexp         regExpInfo;
    char               *command;
    struct matchDef_t  *nextMatchDefPtr;
    short               matchflags;
} matchDef_t;

typedef struct scanContext_t {
    matchDef_t  *matchListHead;
    matchDef_t  *matchListTail;
    char        *defaultAction;
    short        flags;
    char         contextHandle [16];
    Tcl_Channel  copyFileChannel;
    int          fileOpen;
} scanContext_t;

/*
 * Prototypes of internal functions.
 */
static void
CleanUpContext _ANSI_ARGS_((void_pt         scanTablePtr,
                            scanContext_t  *contextPtr));

static int
ScanContextCreate _ANSI_ARGS_((Tcl_Interp  *interp,
                               void_pt      scanTablePtr));

static int
ScanContextDelete _ANSI_ARGS_((Tcl_Interp  *interp,
                               void_pt      scanTablePtr,
                               char        *contextHandle));

static int
ScanContextCopyFile _ANSI_ARGS_((Tcl_Interp  *interp,
                                 void_pt      scanTablePtr,
                                 char        *contextHandle,
                                 char        *fileHandle));

static void
CopyFileCloseHandler _ANSI_ARGS_((ClientData clientData));

static int
SetCopyFile _ANSI_ARGS_((Tcl_Interp    *interp,
                         scanContext_t *contextPtr,
                         char          *fileHandle));

static void
ClearCopyFile _ANSI_ARGS_((scanContext_t *contextPtr));

static int
SetMatchInfoVar _ANSI_ARGS_((Tcl_Interp       *interp,
                             int              *storedLinePtr,
                             scanContext_t    *contextPtr,
                             Tcl_Channel       channel,
                             char             *fileLine,
                             long              scanLineNum,
                             matchDef_t       *matchPtr,
                             Tcl_SubMatchInfo  subMatchInfo));

static int
ScanFile _ANSI_ARGS_((Tcl_Interp    *interp,
                      scanContext_t *contextPtr,
                      Tcl_Channel    channel));

static void
ScanFileCloseHandler _ANSI_ARGS_((ClientData clientData));

static void
FileScanCleanUp _ANSI_ARGS_((ClientData  clientData,
                             Tcl_Interp *interp));


/*-----------------------------------------------------------------------------
 * CleanUpContext --
 *
 *   Release all resources allocated to the specified scan context.  Doesn't
 * free the table entry.
 *-----------------------------------------------------------------------------
 */
static void
CleanUpContext (scanTablePtr, contextPtr)
    void_pt        scanTablePtr;
    scanContext_t *contextPtr;
{
    matchDef_t  *matchPtr, *oldMatchPtr;

    for (matchPtr = contextPtr->matchListHead; matchPtr != NULL;) {
        TclX_RegExpClean (&matchPtr->regExpInfo);
        if (matchPtr->command != NULL)
            ckfree (matchPtr->command);
        oldMatchPtr = matchPtr;
        matchPtr = matchPtr->nextMatchDefPtr;
        ckfree ((char *) oldMatchPtr);
        }
    if (contextPtr->defaultAction != NULL) {
        ckfree (contextPtr->defaultAction);
    }
    ClearCopyFile (contextPtr);
    ckfree ((char *) contextPtr);
}

/*-----------------------------------------------------------------------------
 * ScanContextCreate --
 *
 *   Create a new scan context, implements the subcommand:
 *         scancontext create
 *-----------------------------------------------------------------------------
 */
static int
ScanContextCreate (interp, scanTablePtr)
    Tcl_Interp  *interp;
    void_pt      scanTablePtr;
{
    scanContext_t *contextPtr, **tableEntryPtr;

    contextPtr = (scanContext_t *) ckalloc (sizeof (scanContext_t));
    contextPtr->flags = 0;
    contextPtr->matchListHead = NULL;
    contextPtr->matchListTail = NULL;
    contextPtr->defaultAction = NULL;
    contextPtr->copyFileChannel = NULL;

    tableEntryPtr = (scanContext_t **)
        Tcl_HandleAlloc (scanTablePtr,
                         contextPtr->contextHandle);
    *tableEntryPtr = contextPtr;

    Tcl_SetResult (interp, contextPtr->contextHandle, TCL_STATIC);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * ScanContextDelete --
 *
 *   Deletes the specified scan context, implements the subcommand:
 *         scancontext delete contexthandle
 *-----------------------------------------------------------------------------
 */
static int
ScanContextDelete (interp, scanTablePtr, contextHandle)
    Tcl_Interp  *interp;
    void_pt      scanTablePtr;
    char        *contextHandle;
{
    scanContext_t **tableEntryPtr;

    tableEntryPtr = (scanContext_t **) Tcl_HandleXlate (interp,
                                                        scanTablePtr,
                                                        contextHandle);
    if (tableEntryPtr == NULL)
        return TCL_ERROR;

    CleanUpContext (scanTablePtr, *tableEntryPtr);
    Tcl_HandleFree (scanTablePtr, tableEntryPtr);

    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * CopyFileCloseHandler --
 *   Close handler for the copyfile.  Turns off copying to the file.
 * Parameters:
 *   o clientData (I) - Pointer to the scan context.
 *-----------------------------------------------------------------------------
 */
static void
CopyFileCloseHandler (clientData)
    ClientData clientData;
{
    ((scanContext_t *) clientData)->copyFileChannel = NULL;
}

/*-----------------------------------------------------------------------------
 * SetCopyFile --
 *   Set the copy file handle for a context.
 * Parameters:
 *   o interp (O) - The Tcl interpreter, errors are returned in result.
 *   o contextPtr (I) - Pointer to the scan context.
 *   o fileHandle (I) - File handle of the copy file.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
SetCopyFile (interp, contextPtr, fileHandle)
    Tcl_Interp    *interp;
    scanContext_t *contextPtr;
    char          *fileHandle;
{
    Tcl_Channel copyFileChannel;

    copyFileChannel = TclX_GetOpenChannel (interp, fileHandle,
                                           TCL_WRITABLE);
    if (copyFileChannel == NULL)
        return TCL_ERROR;

    /*
     * Delete the old copyfile and set the new one.
     */
    if (contextPtr->copyFileChannel != NULL) {
        Tcl_DeleteCloseHandler (contextPtr->copyFileChannel,
                                CopyFileCloseHandler,
                                contextPtr);
    }
    Tcl_CreateCloseHandler (copyFileChannel,
                            CopyFileCloseHandler,
                            contextPtr);
    contextPtr->copyFileChannel = copyFileChannel;
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * ClearCopyFile --
 *   Clear the copy file handle for a context.
 * Parameters:
 *   o contextPtr (I) - Pointer to the scan context.
 *-----------------------------------------------------------------------------
 */
static void
ClearCopyFile (contextPtr)
    scanContext_t *contextPtr;
{
    if (contextPtr->copyFileChannel != NULL) {
        Tcl_DeleteCloseHandler (contextPtr->copyFileChannel,
                                CopyFileCloseHandler,
                                contextPtr);
        contextPtr->copyFileChannel = NULL;
    }
}

/*-----------------------------------------------------------------------------
 * ScanContextCopyFile --
 *
 *   Access or set the copy file handle for the specified scan context,
 * implements the subcommand:
 *         scancontext copyfile contexthandle ?filehandle?
 *-----------------------------------------------------------------------------
 */
static int
ScanContextCopyFile (interp, scanTablePtr, contextHandle, fileHandle)
    Tcl_Interp  *interp;
    void_pt      scanTablePtr;
    char        *contextHandle;
    char        *fileHandle;
{
    scanContext_t *contextPtr, **tableEntryPtr;
    Tcl_Channel    copyFileChannel;

    tableEntryPtr = (scanContext_t **) Tcl_HandleXlate (interp,
                                                        scanTablePtr,
                                                        contextHandle);
    if (tableEntryPtr == NULL)
        return TCL_ERROR;
    contextPtr = *tableEntryPtr;

    /*
     * Return the copy file handle if not specified.
     */
    if (fileHandle == NULL) {
        Tcl_SetResult (interp,
                       Tcl_GetChannelName (contextPtr->copyFileChannel),
                       TCL_STATIC);
        return TCL_OK;
    } else {
        return SetCopyFile (interp, contextPtr, fileHandle);
    }
}

/*-----------------------------------------------------------------------------
 * Tcl_ScancontextCmd --
 *
 *   Implements the TCL scancontext Tcl command, which has the following forms:
 *         scancontext create
 *         scancontext delete
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ScancontextCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    if (argc < 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " option ...",
                          (char *) NULL);
        return TCL_ERROR;
    }
    /*
     * Create a new scan context.
     */
    if (STREQU (argv [1], "create")) {
        if (argc != 2) {
            Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " create",
                              (char *) NULL);
            return TCL_ERROR;
        }
        return ScanContextCreate (interp,
                                  (void_pt) clientData);
    }
    
    /*
     * Delete a scan context.
     */
    if (STREQU (argv [1], "delete")) {
        if (argc != 3) {
            Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                              " delete contexthandle", (char *) NULL);
            return TCL_ERROR;
        }
        return ScanContextDelete (interp,
                                  (void_pt) clientData,
                                  argv [2]);
    }

    /*
     * Access or set the copyfile.
     */
    if (STREQU (argv [1], "copyfile")) {
        if ((argc < 3) || (argc > 4)) {
            Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                              " copyfile contexthandle ?filehandle?",
                              (char *) NULL);
            return TCL_ERROR;
        }
        return ScanContextCopyFile (interp,
                                    (void_pt) clientData,
                                    argv [2],
                                    (argc == 4) ? argv [3] : NULL);
    }

    Tcl_AppendResult (interp, "invalid argument, expected one of: ",
                      "\"create\", \"delete\", or \"copyfile\"",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_ScanmatchCmd --
 *
 *   Implements the TCL command:
 *         scanmatch ?-nocase? contexthandle ?regexp? command
 *
 *   This uses both Boyer_Moore and regular expressions matching.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ScanmatchCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    scanContext_t  *contextPtr, **tableEntryPtr;
    matchDef_t     *newmatch;
    int             compFlags = TCLX_REXP_BOTH_ALGORITHMS;
    int             firstArg = 1;

    if (argc < 3)
        goto argError;
    if (STREQU (argv[1], "-nocase")) {
        compFlags |= TCLX_REXP_NO_CASE;
        firstArg = 2;
    }
      
    /*
     * If firstArg == 2 (-nocase), the both a regular expression and a command
     * string must be specified, otherwise the regular expression is optional.
     */
    if (((firstArg == 2) && (argc != 5)) || ((firstArg == 1) && (argc > 4)))
        goto argError;

    tableEntryPtr = (scanContext_t **)
        Tcl_HandleXlate (interp,
                         (void_pt) clientData, 
                         argv [firstArg]);
    if (tableEntryPtr == NULL)
        return TCL_ERROR;
    contextPtr = *tableEntryPtr;

    /*
     * Handle the default case (no regular expression).
     */
    if (argc == 3) {
        if (contextPtr->defaultAction) {
            Tcl_AppendResult (interp, argv [0], ": default match already ",
                              "specified in this scan context", (char *) NULL);
            return TCL_ERROR;
        }
        contextPtr->defaultAction = ckstrdup (argv [2]);

        return TCL_OK;
    }

    /*
     * Add a regular expression to the context.
     */

    newmatch = (matchDef_t *) ckalloc(sizeof (matchDef_t));
    newmatch->matchflags = 0;

    if (compFlags & TCLX_REXP_NO_CASE) {
        newmatch->matchflags |= MATCH_CASE_INSENSITIVE_FLAG;
        contextPtr->flags |= CONTEXT_A_CASE_INSENSITIVE_FLAG;
    }

    if (TclX_RegExpCompile (interp,
                            &newmatch->regExpInfo,
                            argv [firstArg + 1], 
                           compFlags) != TCL_OK) {
        ckfree ((char *) newmatch);
        return (TCL_ERROR);
    }

    newmatch->command = ckstrdup (argv [firstArg + 2]);

    /*
     * Link in the new match.
     */
    newmatch->nextMatchDefPtr = NULL;
    if (contextPtr->matchListHead == NULL)
        contextPtr->matchListHead = newmatch;
    else
        contextPtr->matchListTail->nextMatchDefPtr = newmatch;
    contextPtr->matchListTail = newmatch;

    return TCL_OK;

argError:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                      " ?-nocase? contexthandle ?regexp? command",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * SetMatchInfoVar --
 *
 *   Sets the Tcl array variable "matchInfo" to contain information about the
 * current match.  This function is optimize to store per line information
 * only once.
 *
 * Parameters:
 *   o interp (O) - The Tcl interpreter to set the matchInfo variable in.
 *     Errors are returned in result.
 *   o storedLinePtr (I/O) - A flag that indicates if the current line has
 *     been stored.  Should be set to FALSE when a new line is read.
 *   o contextPtr (I) - The current scan context.
 *   o channel (I) - The file being scanned.
 *   o fileLine (I) - The current line.
 *   o scanLineNum (I) - Current scanned line in the file.
 *   o matchPtr (I) - The current match, or NULL fo the default.
 *   o subMatchInfo (I) - Information about the subexpressions that matched,
 *     or NULL if this is the default match.
 *-----------------------------------------------------------------------------
 */
static int
SetMatchInfoVar (interp, storedLinePtr, contextPtr, channel, fileLine,
                 scanLineNum, matchPtr, subMatchInfo)
    Tcl_Interp       *interp;
    int              *storedLinePtr;
    scanContext_t    *contextPtr;
    Tcl_Channel       channel;
    char             *fileLine;
    long              scanLineNum;
    matchDef_t       *matchPtr;
    Tcl_SubMatchInfo  subMatchInfo;
{
    static char *MATCHINFO = "matchInfo";
    off_t matchOffset;
    int idx, start, end;
    char key [32], buf [32], *varPtr, holdChar;

    /*
     * Save information about the current line, if it hasn't been saved.
     */
    if (!*storedLinePtr) {
        *storedLinePtr = TRUE;

        Tcl_UnsetVar (interp, MATCHINFO, 0);
        
        matchOffset = Tcl_Tell (channel) - (strlen (fileLine) + 1);

        if (Tcl_SetVar2 (interp, MATCHINFO, "line", fileLine, 
                         TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;

        sprintf (buf, "%ld", matchOffset);
        if (Tcl_SetVar2 (interp, MATCHINFO, "offset", buf,
                         TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;

        sprintf (buf, "%ld", scanLineNum);
        if (Tcl_SetVar2 (interp, MATCHINFO, "linenum", buf,
                         TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;

        if (Tcl_SetVar2 (interp, MATCHINFO, "context",
                         contextPtr->contextHandle,
                         TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;

        if (Tcl_SetVar2 (interp, MATCHINFO, "handle", 
                         Tcl_GetChannelName (channel),
                         TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;

    }

    if (contextPtr->copyFileChannel != NULL) {
        if (Tcl_SetVar2 (interp, MATCHINFO, "copyHandle", 
                         Tcl_GetChannelName (contextPtr->copyFileChannel),
                         TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;
    }

    if (subMatchInfo == NULL)
        return TCL_OK;

    for (idx = 0; idx < matchPtr->regExpInfo.numSubExprs; idx++) {
        start = subMatchInfo [idx].start;
        end = subMatchInfo [idx].end;

        sprintf (key, "subindex%d", idx);
        sprintf (buf, "%d %d", start, end);
        varPtr = Tcl_SetVar2 (interp, "matchInfo", key, buf,
                              TCL_LEAVE_ERR_MSG);
        if (varPtr == NULL)
            return TCL_ERROR;

        sprintf (key, "submatch%d", idx);
        if (start < 0) {
            varPtr = Tcl_SetVar2 (interp, "matchInfo", key,
                                  "",
                                  TCL_LEAVE_ERR_MSG);
        } else {
            holdChar = fileLine [end + 1];
            fileLine [end + 1] = '\0';
            varPtr = Tcl_SetVar2 (interp, "matchInfo", key,
                                  fileLine + start,
                                  TCL_LEAVE_ERR_MSG);
            fileLine [end + 1] = holdChar;
        }
        if (varPtr == NULL)
            return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * ScanFile --
 *
 *   Scan a file given a scancontext.
 *-----------------------------------------------------------------------------
 */
static int
ScanFile (interp, contextPtr, channel)
    Tcl_Interp    *interp;
    scanContext_t *contextPtr;
    Tcl_Channel    channel;
{
    Tcl_DString buffer, lowerBuffer;
    matchDef_t *matchPtr;
    int result, matchedAtLeastOne, status, storedThisLine;
    long scanLineNum = 0;
    Tcl_SubMatchInfo subMatchInfo;

    if (contextPtr->matchListHead == NULL) {
        Tcl_AppendResult (interp, "no patterns in current scan context",
                          (char *) NULL);
        return TCL_ERROR;
    }

    Tcl_DStringInit (&buffer);
    Tcl_DStringInit (&lowerBuffer);

    result = TCL_OK;
    while (TRUE) {
        Tcl_DStringSetLength (&buffer, 0);
        if (!contextPtr->fileOpen)
            goto scanExit;  /* Closed by a callback */

        if (Tcl_Gets (channel, &buffer) < 0) {
            if (Tcl_Eof (channel) || Tcl_InputBlocked (channel))
                goto scanExit;
            interp->result = Tcl_PosixError (interp);
            result = TCL_ERROR;
            goto scanExit;
        }

        scanLineNum++;
        storedThisLine = FALSE;
        matchedAtLeastOne = FALSE;

        if (contextPtr->flags & CONTEXT_A_CASE_INSENSITIVE_FLAG) {
            Tcl_DStringFree (&lowerBuffer);
            Tcl_DStringAppend (&lowerBuffer, buffer.string, -1);
            Tcl_DownShift (lowerBuffer.string, lowerBuffer.string);
        }

        for (matchPtr = contextPtr->matchListHead; matchPtr != NULL; 
                 matchPtr = matchPtr->nextMatchDefPtr) {

            if (!TclX_RegExpExecute (interp,
                                     &matchPtr->regExpInfo,
                                     buffer.string,
                                     lowerBuffer.string,
                                     subMatchInfo))
                continue;  /* Try next match pattern */

            matchedAtLeastOne = TRUE;

            result = SetMatchInfoVar(interp,
                                     &storedThisLine,
                                     contextPtr,
                                     channel,
                                     buffer.string,
                                     scanLineNum,
                                     matchPtr,
                                     subMatchInfo);
            if (result != TCL_OK)
                goto scanExit;

            result = Tcl_Eval(interp, matchPtr->command);
            if (result == TCL_ERROR) {
                Tcl_AddErrorInfo (interp, 
                    "\n    while executing a match command");
                goto scanExit;
            }
            if (result == TCL_CONTINUE) {
                /* 
                 * Don't process any more matches for this line.
                 */
                goto matchLineExit;
            }
            if ((result == TCL_BREAK) || (result == TCL_RETURN)) {
                /*
                 * Terminate scan.
                 */
                result = TCL_OK;
                goto scanExit;
            }
        }

      matchLineExit:
        /*
         * Process default action if required.
         */
        if ((contextPtr->defaultAction != NULL) && (!matchedAtLeastOne)) {
            result = SetMatchInfoVar(interp,
                                     &storedThisLine,
                                     contextPtr,
                                     channel,
                                     buffer.string,
                                     scanLineNum,
                                     NULL,
                                     NULL);
            if (result != TCL_OK)
                goto scanExit;

            result = Tcl_Eval (interp, contextPtr->defaultAction);
            if (result == TCL_ERROR) {
                Tcl_AddErrorInfo (interp, 
                    "\n    while executing a match default command");
                goto scanExit;
            }
            if ((result == TCL_BREAK) || (result == TCL_RETURN)) {
                /*
                 * Terminate scan.
                 */
                result = TCL_OK;
                goto scanExit;
            }
        }

	if ((contextPtr->copyFileChannel != NULL) && (!matchedAtLeastOne)) {
	    if ((Tcl_Write (contextPtr->copyFileChannel,
                            buffer.string, buffer.length) < 0) ||
                (TclX_WriteNL (contextPtr->copyFileChannel) < 0)) {
		interp->result = Tcl_PosixError (interp);
		return TCL_ERROR;
	    }
	}
    }

  scanExit:
    Tcl_DStringFree (&buffer);
    Tcl_DStringFree (&lowerBuffer);
    if (result == TCL_ERROR)
        return TCL_ERROR;
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * ScanFileCloseHandler --
 *   Close handler for the file being scanned.  Marks it as not open.
 * Parameters:
 *   o clientData (I) - Pointer to the scan context.
 *-----------------------------------------------------------------------------
 */
static void
ScanFileCloseHandler (clientData)
    ClientData clientData;
{
    ((scanContext_t *) clientData)->fileOpen = FALSE;
}

/*-----------------------------------------------------------------------------
 * Tcl_ScanfileCmd --
 *
 *   Implements the TCL command:
 *        scanfile ?-copyfile copyhandle? contexthandle filehandle
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ScanfileCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    scanContext_t *contextPtr, **tableEntryPtr;
    char *contextHandle, *fileHandle, *copyFileHandle;
    Tcl_Channel channel;
    int status;

    if ((argc != 3) && (argc != 5))
        goto argError;

    if (argc == 3) {
	contextHandle = argv [1];
	fileHandle = argv [2];
	copyFileHandle = NULL;
    } else {
	if (!STREQU (argv[1], "-copyfile"))
            goto argError;
	copyFileHandle = argv [2];
	contextHandle = argv [3];
	fileHandle = argv [4];
    }

    tableEntryPtr = (scanContext_t **)
        Tcl_HandleXlate (interp,
                         (void_pt) clientData, 
                         contextHandle);
    if (tableEntryPtr == NULL)
        return TCL_ERROR;
    contextPtr = *tableEntryPtr;

    channel = TclX_GetOpenChannel (interp, fileHandle, TCL_READABLE);
    if (channel == NULL)
        return TCL_ERROR;

    if (copyFileHandle != NULL) {
        if (SetCopyFile (interp, contextPtr, copyFileHandle) == TCL_ERROR)
            return TCL_ERROR;
    }

    /*
     * Scan the file, protecting it with a close handler.
     */
    contextPtr->fileOpen = TRUE;
    Tcl_CreateCloseHandler (channel,
                            ScanFileCloseHandler,
                            contextPtr);
    status = ScanFile (interp,
                       contextPtr,
                       channel);
    Tcl_DeleteCloseHandler (channel,
                            ScanFileCloseHandler,
                            contextPtr);

    /*
     * If we set the copyfile, disassociate it from the context.
     */
    if (copyFileHandle != NULL) {
        ClearCopyFile (contextPtr);
    }
    return status;

  argError:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
		      " ?-copyfile filehandle? contexthandle filehandle", 
		      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * FileScanCleanUp --
 *
 *    Called when the interpreter is deleted to cleanup all filescan
 * resources
 *-----------------------------------------------------------------------------
 */
static void
FileScanCleanUp (clientData, interp)
    ClientData  clientData;
    Tcl_Interp *interp;
{
    scanContext_t **tableEntryPtr;
    int             walkKey;
    
    walkKey = -1;
    while (TRUE) {
        tableEntryPtr =
            (scanContext_t **) Tcl_HandleWalk ((void_pt) clientData, 
                                               &walkKey);
        if (tableEntryPtr == NULL)
            break;
        CleanUpContext ((void_pt) clientData, *tableEntryPtr);
    }
    Tcl_HandleTblRelease ((void_pt) clientData);
}

/*-----------------------------------------------------------------------------
 *  Tcl_InitFilescan --
 *
 *    Initialize the TCL file scanning facility..
 *-----------------------------------------------------------------------------
 */
void
Tcl_InitFilescan (interp)
    Tcl_Interp *interp;
{
    void_pt  scanTablePtr;

    scanTablePtr = Tcl_HandleTblInit ("context",
                                      sizeof (scanContext_t *),
                                      10);

    Tcl_CallWhenDeleted (interp, FileScanCleanUp, (ClientData) scanTablePtr);

    /*
     * Initialize the commands.
     */
    Tcl_CreateCommand (interp, "scanfile", Tcl_ScanfileCmd, 
                       (ClientData) scanTablePtr, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "scanmatch", Tcl_ScanmatchCmd, 
                       (ClientData) scanTablePtr, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "scancontext", Tcl_ScancontextCmd,
                       (ClientData) scanTablePtr, (void (*)()) NULL);
}

