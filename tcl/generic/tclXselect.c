/*
 * tclXselect.c
 *
 * Extended Tcl file I/O commands.
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
 * $Id: tclXselect.c,v 7.2 1996/07/22 17:10:10 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

#ifndef NO_SELECT

#ifndef NO_SYS_SELECT_H
#   include <sys/select.h>
#endif

/*
 * A couple of systems (Xenix and older SCO unix) have bzero hidden away
 * in the X library that we don't use, but the select macros use bzero.
 * Make them use memset.
 */
#ifdef NO_BZERO
#    define bzero(to,length)  memset(to,'\0',length)
#endif

/*
 * A few systems (A/UX 2.0) have select but no macros, define em in this case.
 */
#ifndef FD_SET
#   define FD_SET(fd,fdset)     (fdset)->fds_bits[0] |= (1<<(fd))
#   define FD_CLR(fd,fdset)     (fdset)->fds_bits[0] &= ~(1<<(fd))
#   define FD_ZERO(fdset)       (fdset)->fds_bits[0] = 0
#   define FD_ISSET(fd,fdset)   (((fdset)->fds_bits[0]) & (1<<(fd)))
#endif

/*
 * Data kept about a file channel.
 */
typedef struct {
    Tcl_Channel channel;
    int         readFd;
    int         writeFd;
} channelData_t;

/*
 * Prototypes of internal functions.
 */
static int
ParseSelectFileList _ANSI_ARGS_((Tcl_Interp     *interp,
                                 char           *handleList,
                                 fd_set         *fileSetPtr,
                                 channelData_t **channelListPtr,
                                 int            *maxFileIdPtr));

static int
FindPendingData _ANSI_ARGS_((int            fileDescCnt,
                             channelData_t *channelList,
                             fd_set        *fileDescSetPtr));

static char *
ReturnSelectedFileList _ANSI_ARGS_((fd_set        *fileDescSetPtr,
                                    fd_set        *fileDescSet2Ptr,
                                    int            fileDescCnt,
                                    channelData_t *channelListPtr));


/*-----------------------------------------------------------------------------
 * ParseSelectFileList --
 *
 *   Parse a list of file handles for select.
 *
 * Parameters:
 *   o interp (O) - Error messages are returned in the result.
 *   o handleList (I) - The list of file handles to parse, may be empty.
 *   o fileSetPtr (O) - The select fd_set for the parsed handles is
 *     filled in.
 *   o channelListPtr (O) - A pointer to a dynamically allocated list of
 *     the channels that are in the set.  If the list is empty, NULL is
 *     returned.
 *   o maxFileIdPtr (I/O) - If a file id greater than the current value is
 *     encountered, it will be set to that file id.
 * Returns:
 *   The number of files in the list, or -1 if an error occured.
 *-----------------------------------------------------------------------------
 */
static int
ParseSelectFileList (interp, handleList, fileSetPtr, channelListPtr,
                     maxFileIdPtr)
    Tcl_Interp    *interp;
    char          *handleList;
    fd_set        *fileSetPtr;
    channelData_t **channelListPtr;
    int           *maxFileIdPtr;
{
    int handleCnt, idx;
    char **handleArgv;
    channelData_t *channelList;

    /*
     * Optimize empty list handling.
     */
    if (handleList [0] == '\0') {
        *channelListPtr = NULL;
        return 0;
    }

    if (Tcl_SplitList (interp, handleList, &handleCnt, &handleArgv) != TCL_OK)
        return -1;

    /*
     * Handle case of an empty list.
     */
    if (handleCnt == 0) {
        *channelListPtr = NULL;
        ckfree ((char *) handleArgv);
        return 0;
    }

    channelList =
        (channelData_t*) ckalloc (sizeof (channelData_t) * handleCnt);

    for (idx = 0; idx < handleCnt; idx++) {
        channelList [idx].channel =
            TclX_GetOpenChannel (interp,
                                 handleArgv [idx],
                                 0);
        if (channelList [idx].channel == NULL) {
            ckfree ((char *) handleArgv);
            ckfree ((char *) channelList);
            return -1;
        }
        
        channelList [idx].readFd =
            TclX_ChannelFnum (channelList [idx].channel, TCL_READABLE);
        if (channelList [idx].readFd >= 0) {
            FD_SET (channelList [idx].readFd, fileSetPtr);
            if (channelList [idx].readFd > *maxFileIdPtr)
                *maxFileIdPtr = channelList [idx].readFd;
        }
        channelList [idx].writeFd =
            TclX_ChannelFnum (channelList [idx].channel, TCL_WRITABLE);
        if (channelList [idx].writeFd >= 0) {
            FD_SET (channelList [idx].writeFd, fileSetPtr);
            if (channelList [idx].writeFd > *maxFileIdPtr)
                *maxFileIdPtr = channelList [idx].writeFd;
        }
    }

    *channelListPtr = channelList;
    ckfree ((char *) handleArgv);
    return handleCnt;
}

/*-----------------------------------------------------------------------------
 * FindPendingData --
 *
 *   Scan a list of read files to determine if any of them have data pending
 * in their buffers.
 *
 * Parameters:
 *   o fileDescCnt (I) - Number of descriptors in the list.
 *   o channelListPtr (I) - A pointer to a list of the channel data for
 *     the channels to check.
 *   o fileDescSetPtr (I) - A select fd_set with will have a bit set for
 *     every file that has data pending it its buffer.
 * Returns:
 *   TRUE if any where found that had pending data, FALSE if none were found.
 *-----------------------------------------------------------------------------
 */
static int
FindPendingData (fileDescCnt, channelList, fileDescSetPtr)
    int            fileDescCnt;
    channelData_t *channelList;
    fd_set        *fileDescSetPtr;
{
    int idx, found = FALSE;

    FD_ZERO (fileDescSetPtr);

    for (idx = 0; idx < fileDescCnt; idx++) {
        if (Tcl_InputBuffered (channelList [idx].channel)) {
            FD_SET (channelList [idx].readFd, fileDescSetPtr);
            found = TRUE;
        }
    }
    return found;
}

/*-----------------------------------------------------------------------------
 * ReturnSelectedFileList --
 *
 *   Take the resulting file descriptor sets from a select, and the
 *   list of file descritpors and build up a list of Tcl file handles.
 *
 * Parameters:
 *   o fileDescSetPtr (I) - The select fd_set.
 *   o fileDescSet2Ptr (I) - Pointer to a second descriptor to also check
 *     (their may be overlap).  NULL if no second set.
 *   o fileDescCnt (I) - Number of descriptors in the list.
 *   o channelListPtr (I) - A pointer to a list of the FILE pointers for
 *     files that are in the set.
 * Returns:
 *   A dynamicly allocated list of file handles.  If the handles are empty,
 *   it returns a constant empty string.  Do not free the empty string.
 *-----------------------------------------------------------------------------
 */
static char *
ReturnSelectedFileList (fileDescSetPtr, fileDescSet2Ptr, fileDescCnt,
                        channelList) 
    fd_set        *fileDescSetPtr;
    fd_set        *fileDescSet2Ptr;
    int            fileDescCnt;
    channelData_t *channelList;
{
    int idx, handleCnt;
    char *fileHandleList;
    char **fileHandleArgv;

    /*
     * Special case the empty list.
     */
    if (fileDescCnt == 0)
        return "";

    /*
     * Allocate enough room to hold the argv.
     */
    fileHandleArgv = (char **) ckalloc (fileDescCnt * sizeof (char *));

    handleCnt = 0;
    for (idx = 0; idx < fileDescCnt; idx++) {
        if (((channelList [idx].readFd >= 0) &&
             FD_ISSET (channelList [idx].readFd, fileDescSetPtr)) ||
            ((channelList [idx].writeFd >= 0) &&
             FD_ISSET (channelList [idx].writeFd, fileDescSetPtr))) {
            fileHandleArgv [handleCnt] =
                Tcl_GetChannelName (channelList [idx].channel);
            handleCnt++;
        } else if ((fileDescSet2Ptr != NULL) &&
                   (((channelList [idx].readFd >= 0) &&
                    FD_ISSET (channelList [idx].readFd, fileDescSet2Ptr)) ||
                   ((channelList [idx].writeFd >= 0) &&
                    FD_ISSET (channelList [idx].writeFd, fileDescSet2Ptr)))) {
            fileHandleArgv [handleCnt] =
                Tcl_GetChannelName (channelList [idx].channel);
            handleCnt++;
        }
    }

    fileHandleList = Tcl_Merge (handleCnt, fileHandleArgv);
    ckfree ((char *) fileHandleArgv);

    return fileHandleList;
}

/*-----------------------------------------------------------------------------
 * Tcl_SelectCmd --
 *  Implements the select TCL command:
 *      select readhandles ?writehandles? ?excepthandles? ?timeout?
 *
 *  This command is extra smart in the fact that it checks for read data
 * pending in the stdio buffer first before doing a select.
 *   
 * Results:
 *     A list in the form:
 *        {readhandles writehandles excepthandles}
 *     or {} it the timeout expired.
 *-----------------------------------------------------------------------------
 */
int
Tcl_SelectCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int idx;
    fd_set fdSets [3], readPendingFDSet;
    int descCnts [3];
    channelData_t *descLists [3];
    char *retListArgv [3];
    int numSelected, maxFileId = 0, pending;
    int result = TCL_ERROR;
    struct timeval  timeoutRec;
    struct timeval *timeoutRecPtr;


    if (argc < 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " readFileIds ?writeFileIds? ?exceptFileIds?",
                          " ?timeout?", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Initialize. 0 == read, 1 == write and 2 == exception.
     */
    for (idx = 0; idx < 3; idx++) {
        FD_ZERO (&fdSets [idx]);
        descCnts [idx] = 0;
        descLists [idx] = NULL;
    }

    /*
     * Parse the file handles and set everything up for the select call.
     */
    for (idx = 0; (idx < 3) && (idx < argc - 1); idx++) {
        descCnts [idx] = ParseSelectFileList (interp, argv [idx + 1],
                                              &fdSets [idx], &descLists [idx],
                                              &maxFileId);
        if (descCnts [idx] < 0)
            goto exitPoint;
    }

    /*
     * Get the time out.  Zero is different that not specified.
     */
    timeoutRecPtr = NULL;
    if ((argc > 4) && (argv [4][0] != '\0')) {
        double  timeout, seconds, microseconds;

        if (Tcl_GetDouble (interp, argv [4], &timeout) != TCL_OK)
            goto exitPoint;
        if (timeout < 0) {
            Tcl_AppendResult (interp, "timeout must be greater than or equal",
                              " to zero", (char *) NULL);
            goto exitPoint;
        }
        seconds = floor (timeout);
        microseconds = (timeout - seconds) * 1000000.0;
        timeoutRec.tv_sec = (long) seconds;
        timeoutRec.tv_usec = (long) microseconds;
        timeoutRecPtr = &timeoutRec;
    }

    /*
     * Check if any data is pending in the read buffers.  If there is,
     * then do the select, but don't block in it.
     */
    pending = FindPendingData (descCnts [0], descLists [0], &readPendingFDSet);
    if (pending) {
        timeoutRec.tv_sec = 0;
        timeoutRec.tv_usec = 0;
        timeoutRecPtr = &timeoutRec;
    }

    /*
     * All set, do the select.
     */
    numSelected = select (maxFileId + 1,
                          &fdSets [0], &fdSets [1], &fdSets [2],
                          timeoutRecPtr);
    if (numSelected < 0) {
        Tcl_AppendResult (interp, "select error: ", Tcl_PosixError (interp),
                          (char *) NULL);
        goto exitPoint;
    }

    /*
     * Return the result, either a 3 element list, or leave the result
     * empty if the timeout occured.
     */
    if (numSelected > 0 || pending) {
        for (idx = 0; idx < 3; idx++) {
            retListArgv [idx] =
                ReturnSelectedFileList (&fdSets [idx],
                                        (idx == 0) ? &readPendingFDSet : NULL,
                                        descCnts [idx],
                                        descLists [idx]);
        }

        Tcl_SetResult (interp, Tcl_Merge (3, retListArgv), TCL_DYNAMIC); 
        
        for (idx = 0; idx < 3; idx++) {
            if (retListArgv [idx][0] != '\0')
                ckfree ((char *) retListArgv [idx]);
        }
    }

    result = TCL_OK;

  exitPoint:
    for (idx = 0; idx < 3; idx++) {
        if (descLists [idx] != NULL)
            ckfree ((char *) descLists [idx]);
    }
    return result;
}
#else
/*-----------------------------------------------------------------------------
 * Tcl_SelectCmd --
 *     Dummy select command that returns an error for systems that don't
 *     have select.
 *-----------------------------------------------------------------------------
 */
int
Tcl_SelectCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    Tcl_AppendResult (interp, 
                      "select is not available on this version of Unix",
                      (char *) NULL);
    return TCL_ERROR;
}
#endif
