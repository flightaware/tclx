/*
 * tclXfstat.c
 *
 * Extended Tcl fstat command.
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
 * $Id: tclXfstat.c,v 7.2 1996/07/22 17:10:02 markd Exp $
 *-----------------------------------------------------------------------------
 */
#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static char *
StrFileType _ANSI_ARGS_((struct stat  *statBufPtr));

static void
ReturnStatList _ANSI_ARGS_((Tcl_Interp   *interp,
                            int           ttyDev,
                            struct stat  *statBufPtr));

static int
ReturnStatArray _ANSI_ARGS_((Tcl_Interp   *interp,
                             int           ttyDev,
                             struct stat  *statBufPtr,
                             char         *arrayName));

static int
ReturnStatItem _ANSI_ARGS_((Tcl_Interp   *interp,
                            Tcl_Channel   channel,
                            int           ttyDev,
                            struct stat  *statBufPtr,
                            char         *itemName));


/*-----------------------------------------------------------------------------
 * StrFileType --
 *
 *   Looks at stat mode and returns a text string indicating what type of
 * file it is.
 *
 * Parameters:
 *   o statBufPtr (I) - Pointer to a buffer initialized by stat or fstat.
 * Returns:
 *   A pointer static text string representing the type of the file.
 *-----------------------------------------------------------------------------
 */
static char *
StrFileType (statBufPtr)
    struct stat  *statBufPtr;
{
    char *typeStr;

    /*
     * Get a string representing the type of the file.
     */
    if (S_ISREG (statBufPtr->st_mode)) {
        typeStr = "file";
    } else if (S_ISDIR (statBufPtr->st_mode)) {
        typeStr = "directory";
    } else if (S_ISCHR (statBufPtr->st_mode)) {
        typeStr = "characterSpecial";
    } else if (S_ISBLK (statBufPtr->st_mode)) {
        typeStr = "blockSpecial";
    } else if (S_ISFIFO (statBufPtr->st_mode)) {
        typeStr = "fifo";
    } else if (S_ISLNK (statBufPtr->st_mode)) {
        typeStr = "link";
    } else if (S_ISSOCK (statBufPtr->st_mode)) {
        typeStr = "socket";
    } else {
        typeStr = "unknown";
    }

    return typeStr;
}

/*-----------------------------------------------------------------------------
 * ReturnStatList --
 *
 *   Return file stat infomation as a keyed list.
 *
 * Parameters:
 *   o interp (I) - The list is returned in result.
 *   o ttyDev (O) - A boolean indicating if the device is associated with a
 *     tty.
 *   o statBufPtr (I) - Pointer to a buffer initialized by stat or fstat.
 *-----------------------------------------------------------------------------
 */
static void
ReturnStatList (interp,ttyDev, statBufPtr)
    Tcl_Interp   *interp;
    int           ttyDev;
    struct stat  *statBufPtr;
{
    char statList [200];

    sprintf (statList, 
             "{atime %ld} {ctime %ld} {dev %ld} {gid %ld} {ino %ld} {mode %ld} ",
              (long) statBufPtr->st_atime, (long) statBufPtr->st_ctime,
              (long) statBufPtr->st_dev,   (long) statBufPtr->st_gid,
              (long) statBufPtr->st_ino,   (long) statBufPtr->st_mode);
    Tcl_AppendResult (interp, statList, (char *) NULL);

    sprintf (statList, 
             "{mtime %ld} {nlink %ld} {size %ld} {uid %ld} {tty %d} {type %s}",
             (long) statBufPtr->st_mtime,  (long) statBufPtr->st_nlink,
             (long) statBufPtr->st_size,   (long) statBufPtr->st_uid,
             (int) ttyDev, StrFileType (statBufPtr));
    Tcl_AppendResult (interp, statList, (char *) NULL);

}

/*-----------------------------------------------------------------------------
 * ReturnStatArray --
 *
 *   Return file stat infomation in an array.
 *
 * Parameters:
 *   o interp (I) - Current interpreter, error return in result.
 *   o ttyDev (O) - A boolean indicating if the device is associated with a
 *     tty.
 *   o statBufPtr (I) - Pointer to a buffer initialized by stat or fstat.
 *   o arrayName (I) - The name of the array to return the info in.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ReturnStatArray (interp, ttyDev, statBufPtr, arrayName)
    Tcl_Interp   *interp;
    int           ttyDev;
    struct stat  *statBufPtr;
    char         *arrayName;
{
    char numBuf [30];

    sprintf (numBuf, "%ld", (long) statBufPtr->st_dev);
    if  (Tcl_SetVar2 (interp, arrayName, "dev", numBuf, 
                      TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    sprintf (numBuf, "%ld", (long) statBufPtr->st_ino);
    if  (Tcl_SetVar2 (interp, arrayName, "ino", numBuf,
                         TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    sprintf (numBuf, "%ld", (long) statBufPtr->st_mode);
    if  (Tcl_SetVar2 (interp, arrayName, "mode", numBuf, 
                      TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    sprintf (numBuf, "%ld", (long) statBufPtr->st_nlink);
    if  (Tcl_SetVar2 (interp, arrayName, "nlink", numBuf,
                      TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    sprintf (numBuf, "%ld", (long) statBufPtr->st_uid);
    if  (Tcl_SetVar2 (interp, arrayName, "uid", numBuf,
                      TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    sprintf (numBuf, "%ld", (long) statBufPtr->st_gid);
    if  (Tcl_SetVar2 (interp, arrayName, "gid", numBuf,
                      TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    sprintf (numBuf, "%ld", (long) statBufPtr->st_size);
    if  (Tcl_SetVar2 (interp, arrayName, "size", numBuf,
                      TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    sprintf (numBuf, "%ld", (long) statBufPtr->st_atime);
    if  (Tcl_SetVar2 (interp, arrayName, "atime", numBuf,
                      TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    sprintf (numBuf, "%ld", (long) statBufPtr->st_mtime);
    if  (Tcl_SetVar2 (interp, arrayName, "mtime", numBuf,
                      TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    sprintf (numBuf, "%ld", (long) statBufPtr->st_ctime);
    if  (Tcl_SetVar2 (interp, arrayName, "ctime", numBuf,
                      TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    if (Tcl_SetVar2 (interp, arrayName, "tty", 
                     ttyDev ? "1" : "0",
                     TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    if (Tcl_SetVar2 (interp, arrayName, "type", StrFileType (statBufPtr),
                     TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    return TCL_OK;

}

/*-----------------------------------------------------------------------------
 * ReturnStatItem --
 *
 *   Return a single file status item.
 *
 * Parameters:
 *   o interp (I) - Item or error returned in result.
 *   o channel (I) - Channel the file is assoicated with.
 *   o ttyDev (O) - A boolean indicating if the device is associated with a
 *     tty.
 *   o statBufPtr (I) - Pointer to a buffer initialized by stat or fstat.
 *   o itemName (I) - The name of the desired item.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ReturnStatItem (interp, channel, ttyDev, statBufPtr, itemName)
    Tcl_Interp   *interp;
    Tcl_Channel   channel;
    int           ttyDev;
    struct stat  *statBufPtr;
    char         *itemName;
{
    char numBuf [32];

    numBuf [0] = '\0';
    if (STREQU (itemName, "dev"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_dev);
    else if (STREQU (itemName, "ino"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_ino);
    else if (STREQU (itemName, "mode"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_mode);
    else if (STREQU (itemName, "nlink"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_nlink);
    else if (STREQU (itemName, "uid"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_uid);
    else if (STREQU (itemName, "gid"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_gid);
    else if (STREQU (itemName, "size"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_size);
    else if (STREQU (itemName, "atime"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_atime);
    else if (STREQU (itemName, "mtime"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_mtime);
    else if (STREQU (itemName, "ctime"))
        sprintf (numBuf, "%ld", (long) statBufPtr->st_ctime);
    else if (STREQU (itemName, "type"))
        strcpy (numBuf, StrFileType (statBufPtr));
    else if (STREQU (itemName, "tty"))
        strcpy (numBuf, ttyDev ? "1" : "0");
    else if (STREQU (itemName, "remotehost")) {
        if (TclXGetHostInfo (interp, channel, TRUE) != TCL_OK)
            return TCL_ERROR;
    } else if (STREQU (itemName, "localhost")) {
        if (TclXGetHostInfo (interp, channel, FALSE) != TCL_OK)
            return TCL_ERROR;
    } else {
        Tcl_AppendResult (interp, "Got \"", itemName, "\", expected one of ",
                          "\"atime\", \"ctime\", \"dev\", \"gid\", \"ino\", ",
                          "\"mode\", \"mtime\", \"nlink\", \"size\", ",
                          "\"tty\", \"type\", \"uid\", \"remotehost\", or ",
                          "\"localhost\"", (char *) NULL);
        return TCL_ERROR;
    }

    if (numBuf [0] != '\0')
        Tcl_SetResult (interp, numBuf, TCL_VOLATILE);

    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_FstatCmd --
 *     Implements the fstat TCL command:
 *         fstat fileId ?item?|?stat arrayvar?
 *-----------------------------------------------------------------------------
 */
int
Tcl_FstatCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    Tcl_Channel channel;
    struct stat statBuf;
    int ttyDev;

    /* FIX: modify to use channel and pass to OS routines. */
    if ((argc < 2) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " fileId ?item?|?stat arrayVar?", (char *) NULL);
        return TCL_ERROR;
    }
    
    channel = TclX_GetOpenChannel (interp, argv [1], 0);
    if (channel == NULL)
        return TCL_ERROR;
    
    if (TclXOSFstat (interp, channel, 0, &statBuf, &ttyDev)) {
        return TCL_ERROR;
    }

    /*
     * Return data in the requested format.
     */
    if (argc == 4) {
        if (!STREQU (argv [2], "stat")) {
            Tcl_AppendResult (interp, "expected item name of \"stat\" when ",
                              "using array name", (char *) NULL);
            return TCL_ERROR;
        }
        return ReturnStatArray (interp, ttyDev, &statBuf, argv [3]);
    }
    if (argc == 3)
        return ReturnStatItem (interp, channel, ttyDev, &statBuf, argv [2]);

    ReturnStatList (interp, ttyDev, &statBuf);
    return TCL_OK;
}
