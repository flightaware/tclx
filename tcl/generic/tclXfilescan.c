/*
 * tclXfilescan.c --
 *
 * Tcl file scanning: regular expression matching on lines of a file.  
 * Implements awk.
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
 * $Id: tclXfilescan.c,v 4.1 1994/11/23 01:58:04 markd Exp markd $
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
} scanContext_t;

/*
 * Prototypes of internal functions.
 */
static void
CleanUpContext _ANSI_ARGS_((void_pt         scanTablePtr,
                            scanContext_t  *contextPtr));

static int
CreateScanContext _ANSI_ARGS_((Tcl_Interp  *interp,
                               void_pt      scanTablePtr));

static int
SelectScanContext _ANSI_ARGS_((Tcl_Interp  *interp,
                               void_pt      scanTablePtr,
                               char        *contextHandle));

static int
Tcl_Delete_scancontextCmd _ANSI_ARGS_((Tcl_Interp  *interp,
                                       void_pt      scanTablePtr,
                                       char        *contextHandle));

static int
SetMatchVar _ANSI_ARGS_((Tcl_Interp *interp,
                         char       *fileLine,
                         long        scanLineNum,
                         FILE       *filePtr,
                         FILE       *copyFilePtr));

static int
SetSubMatchVar _ANSI_ARGS_((Tcl_Interp       *interp,
                            char             *fileLine,
                            Tcl_SubMatchInfo  subMatchInfo));

static void
FileScanCleanUp _ANSI_ARGS_((ClientData  clientData,
                             Tcl_Interp *interp));


/*
 *-----------------------------------------------------------------------------
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
    ckfree ((char *) contextPtr);
}

/*
 *-----------------------------------------------------------------------------
 * CreateScanContext --
 *
 *   Create a new scan context, implements the subcommand:
 *         scancontext create
 *-----------------------------------------------------------------------------
 */
static int
CreateScanContext (interp, scanTablePtr)
    Tcl_Interp  *interp;
    void_pt      scanTablePtr;
{
    scanContext_t *contextPtr, **tableEntryPtr;
    char           curName [16];

    contextPtr = (scanContext_t *) ckalloc (sizeof (scanContext_t));
    contextPtr->flags = 0;
    contextPtr->matchListHead = NULL;
    contextPtr->matchListTail = NULL;
    contextPtr->defaultAction = NULL;

    tableEntryPtr = (scanContext_t **) Tcl_HandleAlloc (scanTablePtr,
                                                        curName);
    *tableEntryPtr = contextPtr;

    Tcl_SetResult (interp, curName, TCL_VOLATILE);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * DeleteScanContext --
 *
 *   Deletes the specified scan context, implements the subcommand:
 *         scancontext delete contexthandle
 *-----------------------------------------------------------------------------
 */
static int
DeleteScanContext (interp, scanTablePtr, contextHandle)
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

/*
 *-----------------------------------------------------------------------------
 * Tcl_ScancontextCmd --
 *
 *   Implements the TCL scancontext Tcl command, which has the following forms:
 *         scancontext create
 *         scancontext delete
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ScancontextCmd (clientData, interp, argc, argv)
    char       *clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    if (argc < 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " option",
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
        return CreateScanContext (interp, (void_pt) clientData);
    }
    
    /*
     * Delete a scan context.
     */
    if (STREQU (argv [1], "delete")) {
        if (argc != 3) {
            Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                              "delete contexthandle", (char *) NULL);
            return TCL_ERROR;
        }
        return DeleteScanContext (interp, (void_pt) clientData, argv [2]);
    }
    
    Tcl_AppendResult (interp, "invalid argument, expected one of: ",
                      "create or delete", (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
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
    char       *clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    scanContext_t  *contextPtr, **tableEntryPtr;
    char           *result;
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

/*
 *-----------------------------------------------------------------------------
 * SetMatchVar --
 *
 *   Sets the TCL array variable matchInfo to contain information 
 * about the line that is matched.
 *-----------------------------------------------------------------------------
 */
static int
SetMatchVar (interp, fileLine, scanLineNum, filePtr, copyFilePtr)
    Tcl_Interp *interp;
    char       *fileLine;
    long        scanLineNum;
    FILE       *filePtr;
    FILE       *copyFilePtr;
{
    static char *MATCHINFO = "matchInfo";
    static char *FILEFMT   = "file%d";
    int          matchOffset;
    char         buf [32];

    Tcl_UnsetVar (interp, MATCHINFO, 0);

    matchOffset = ftell (filePtr) - (strlen (fileLine) + 1);

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

    sprintf (buf, FILEFMT, fileno (filePtr));
    if (Tcl_SetVar2 (interp, MATCHINFO, "handle", buf, 
                     TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;
    if (copyFilePtr != NULL) {
        sprintf (buf, FILEFMT, fileno (copyFilePtr));
        if (Tcl_SetVar2 (interp, MATCHINFO, "copyHandle", buf, 
                         TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * SetSubMatchVar --
 *
 *   Sets the TCL array variable matchInfo entries to describe the matching
 * subexpressions.
 *-----------------------------------------------------------------------------
 */
static int
SetSubMatchVar (interp, fileLine, subMatchInfo)
    Tcl_Interp       *interp;
    char             *fileLine;
    Tcl_SubMatchInfo  subMatchInfo;
{
    int  idx, start, end;
    char key [32], buf [32], *varPtr, holdChar;
    
    for (idx = 0; subMatchInfo [idx].start >= 0; idx++) {
        start = subMatchInfo [idx].start;
        end = subMatchInfo [idx].end;

        sprintf (key, "subindex%d", idx);
        sprintf (buf, "%d %d", start, end);
        varPtr = Tcl_SetVar2 (interp, "matchInfo", key, buf,
                              TCL_LEAVE_ERR_MSG);
        if (varPtr == NULL)
            return TCL_ERROR;

        sprintf (key, "submatch%d", idx);
        holdChar = fileLine [end + 1];
        fileLine [end + 1] = '\0';
        varPtr = Tcl_SetVar2 (interp, "matchInfo", key,
                              fileLine + start,
                              TCL_LEAVE_ERR_MSG);
        fileLine [end + 1] = holdChar;
        if (varPtr == NULL)
            return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * Tcl_ScanfileCmd --
 *
 *   Implements the TCL command:
 *        scanfile contexthandle filehandle
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ScanfileCmd (clientData, interp, argc, argv)
    char       *clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    scanContext_t    *contextPtr, **tableEntryPtr;
    Tcl_DString       dynBuf, lowerDynBuf;
    FILE             *filePtr;
    matchDef_t       *matchPtr;
    int               contextHandleIndex, fileHandleIndex;
    int               result, matchedAtLeastOne, status, storedThisLine;
    long              scanLineNum = 0;
    char             *fileHandle;
    Tcl_SubMatchInfo  subMatchInfo;
    FILE             *copytoFilePtr;

    if ((argc != 3) && (argc != 5)) goto argError;

    if (argc == 3) {
	contextHandleIndex = 1;
	fileHandleIndex = 2;
	copytoFilePtr = NULL;
    } else {
	if (!STREQU (argv[1], "-copyfile")) goto argError;
	contextHandleIndex = 3;
	fileHandleIndex = 4;

        if (Tcl_GetOpenFile (interp, argv [2],
			     TRUE,  /* Write access  */
			     TRUE,   /* Check access */
			     &copytoFilePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    tableEntryPtr = (scanContext_t **)
        Tcl_HandleXlate (interp,
                         (void_pt) clientData, 
                         argv [contextHandleIndex]);
    if (tableEntryPtr == NULL)
        return TCL_ERROR;
    contextPtr = *tableEntryPtr;

    if (Tcl_GetOpenFile (interp, argv [fileHandleIndex],
                         FALSE,  /* Read access  */
                         TRUE,   /* Check access */
                         &filePtr) != TCL_OK)
            return TCL_ERROR;

    if (contextPtr->matchListHead == NULL) {
        Tcl_AppendResult (interp, "no patterns in current scan context",
                          (char *) NULL);
        return TCL_ERROR;
    }

    Tcl_DStringInit (&dynBuf);
    Tcl_DStringInit (&lowerDynBuf);

    result = TCL_OK;
    while (TRUE) {
        Tcl_DStringFree (&dynBuf);
        status = Tcl_DStringGets (filePtr, &dynBuf) ;

        if (status == TCL_ERROR) {
            interp->result = Tcl_PosixError (interp);
            result = TCL_ERROR;
            goto scanExit;
        }
        if (status == TCL_BREAK)
            goto scanExit;  /* EOF */

        scanLineNum++;
        storedThisLine = FALSE;
        matchedAtLeastOne = FALSE;

        if (contextPtr->flags & CONTEXT_A_CASE_INSENSITIVE_FLAG) {
            Tcl_DStringFree (&lowerDynBuf);
            Tcl_DStringAppend (&lowerDynBuf, dynBuf.string, -1);
            Tcl_DownShift (lowerDynBuf.string, lowerDynBuf.string);
        }

        for (matchPtr = contextPtr->matchListHead; matchPtr != NULL; 
                 matchPtr = matchPtr->nextMatchDefPtr) {

            if (!TclX_RegExpExecute (interp,
                                     &matchPtr->regExpInfo,
                                     dynBuf.string,
                                     lowerDynBuf.string,
                                     subMatchInfo))
                continue;  /* Try next match pattern */

            matchedAtLeastOne = TRUE;
            if (!storedThisLine) {
                result = SetMatchVar (interp,
                                      dynBuf.string,
                                      scanLineNum,
                                      filePtr,
                                      copytoFilePtr);
                if (result != TCL_OK)
                    goto scanExit;
                storedThisLine = TRUE;
            }
            result = SetSubMatchVar (interp, dynBuf.string, subMatchInfo);
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
            if (result == TCL_BREAK) {
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
            result = SetMatchVar (interp,
                                  dynBuf.string,
                                  scanLineNum,
                                  filePtr,
                                  copytoFilePtr);
            if (result != TCL_OK)
                goto scanExit;

            result = Tcl_Eval (interp, contextPtr->defaultAction);
            if (result == TCL_ERROR)
                Tcl_AddErrorInfo (interp, 
                    "\n    while executing a match default command");
            if (result == TCL_BREAK)
                goto scanExit;
        }

	if ((copytoFilePtr != NULL) && (!matchedAtLeastOne)) {
	    if (fputs (dynBuf.string, copytoFilePtr) == EOF) {
		interp->result = Tcl_PosixError (interp);
		return TCL_ERROR;
	    }
	    if (fputs ("\n", copytoFilePtr) == EOF) {
		interp->result = Tcl_PosixError (interp);
		return TCL_ERROR;
	    }
	}
    }
scanExit:
    Tcl_DStringFree (&dynBuf);
    Tcl_DStringFree (&lowerDynBuf);
    if (result == TCL_ERROR)
        return TCL_ERROR;
    return TCL_OK;

argError:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
		      " ?-copyfile filehandle? contexthandle filehandle", 
		      (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
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

/*
 *-----------------------------------------------------------------------------
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

