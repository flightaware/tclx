/* 
 * tclXchmod.c --
 *
 *  Chmod, chown and chgrp Tcl commands.
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
 * $Id: tclXchmod.c,v 7.2 1996/08/04 07:29:58 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Type used for returning parsed mode informtion.
 */
typedef struct {
    char  *symMode;  /* Symbolic mode. If NULL, use absolute mode. */
    int    absMode;  /* Numeric mode. */
} modeInfo_t;

static char *FILE_ID_OPT = "-fileid";

/*
 * Prototypes of internal functions.
 */
static int
ConvSymMode _ANSI_ARGS_((Tcl_Interp  *interp,
                         char        *symMode,
                         int          modeVal));


/*-----------------------------------------------------------------------------
 * ConvSymMode --
 *   Parse and convert symbolic file permissions as specified by chmod(C).
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o symMode - The symbolic permissions to parse.
 *   o modeVal - The existing permissions value on a file.
 *
 * Returns:
 *   The new permissions, or -1 if invalid permissions where supplied.
 *-----------------------------------------------------------------------------
 */
static int
ConvSymMode (interp, symMode, modeVal)
    Tcl_Interp  *interp;
    char        *symMode;
    int          modeVal;
{
    int  user, group, other;
    char operator, *scanPtr;
    int  rwxMask, ugoMask, setUID, sticky, locking;
    int  newMode;

    scanPtr = symMode;

    while (*scanPtr != '\0') {
        user = group = other = FALSE;

        /* 
         * Scan who field.
         */
        while (! ((*scanPtr == '+') || 
                  (*scanPtr == '-') || 
                  (*scanPtr == '='))) {
            switch (*scanPtr) {
                case 'a':
                    user = group = other = TRUE;
                    break;
                case 'u':
                    user = TRUE;
                    break;
                case 'g':
                    group = TRUE;
                    break;
                case 'o':
                    other = TRUE;
                    break;
                default:
                    goto invalidMode;
            }
            scanPtr++;
        }

        /*
         * If none where specified, that means all.
         */

        if (! (user || group || other))
            user = group = other = TRUE;

        operator = *scanPtr++;

        /* 
         * Decode the permissions
         */

        rwxMask = 0;
        setUID = sticky = locking = FALSE;

        /* 
         * Scan permissions field
         */
        while (! ((*scanPtr == ',') || (*scanPtr == 0))) {
            switch (*scanPtr) {
                case 'r':
                    rwxMask |= 4;
                    break;
                case 'w':
                    rwxMask |= 2;
                    break;
                case 'x':
                    rwxMask |= 1;
                    break;
                case 's':
                    setUID = TRUE;
                    break;
                case 't':
                    sticky = TRUE;
                    break;
                case 'l':
                    locking = TRUE;
                    break;
                default:
                    goto invalidMode;
            }
            scanPtr++;
        }

        /*
         * Build mode map of specified values.
         */

        newMode = 0;
        ugoMask = 0;
        if (user) {
            newMode |= rwxMask << 6;
            ugoMask |= 0700;
        }
        if (group) {
            newMode |= rwxMask << 3;
            ugoMask |= 0070;
        }
        if (other) {
            newMode |= rwxMask;
            ugoMask |= 0007;
        }
        if (setUID && user)
            newMode |= 04000;
        if ((setUID || locking) && group)
            newMode |= 02000;
        if (sticky)
            newMode |= 01000;

        /* 
         * Add to cumulative mode based on operator.
         */

        if (operator == '+')
            modeVal |= newMode;
        else if (operator == '-')
            modeVal &= ~newMode;
        else if (operator == '=')
            modeVal |= (modeVal & ugoMask) | newMode;
        if (*scanPtr == ',')
            scanPtr++;
    }

    return modeVal;

  invalidMode:
    Tcl_AppendResult (interp, "invalid file mode \"", symMode, "\"",
                      (char *) NULL);
    return -1;
}

/*-----------------------------------------------------------------------------
 * ChmodFileName --
 *   Change the mode of a file by name.
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o modeInfo - Infomation with the mode to set the file to.
 *   o fileName - Name of the file to change.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ChmodFileName (interp, modeInfo, fileName)
    Tcl_Interp  *interp;
    modeInfo_t   modeInfo;
    char        *fileName;
{
    char         *filePath;
    struct stat   fileStat;
    Tcl_DString   pathBuf;
    int           newMode;

    Tcl_DStringInit (&pathBuf);

    filePath = Tcl_TranslateFileName (interp, fileName, &pathBuf);
    if (filePath == NULL) {
        Tcl_DStringFree (&pathBuf);
        return TCL_ERROR;
    }

    if (modeInfo.symMode != NULL) {
        if (stat (filePath, &fileStat) != 0)
            goto fileError;
        newMode = ConvSymMode (interp, modeInfo.symMode,
                               fileStat.st_mode & 07777);
        if (newMode < 0)
            goto errorExit;
    } else {
        newMode = modeInfo.absMode;
    }
    if (TclXOSchmod (interp, filePath, (unsigned short) newMode) < 0)
        return TCL_ERROR;

    Tcl_DStringFree (&pathBuf);
    return TCL_OK;

  fileError:
    Tcl_AppendResult (interp, filePath, ": ",
                      Tcl_PosixError (interp), (char *) NULL);
  errorExit:
    Tcl_DStringFree (&pathBuf);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * ChmodFileId --
 *   Change the mode of a file by file id.
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o modeInfo - Infomation with the mode to set the file to.
 *   o fileId - The Tcl file id.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ChmodFileId (interp, modeInfo, fileId)
    Tcl_Interp  *interp;
    modeInfo_t   modeInfo;
    char        *fileId;
{
    Tcl_Channel channel;
    struct stat fileStat;
    int newMode;

    channel = TclX_GetOpenChannel (interp, fileId, 0);
    if (channel == NULL)
        return TCL_ERROR;

    if (modeInfo.symMode != NULL) {
        if (TclXOSFstat (interp, channel, 0, &fileStat, NULL) != 0)
            return TCL_ERROR;
        newMode = ConvSymMode (interp, modeInfo.symMode,
                               fileStat.st_mode & 07777);
        if (newMode < 0)
            return TCL_ERROR;
    } else {
        newMode = modeInfo.absMode;
    }
    if (TclXOSfchmod (interp, channel, (unsigned short) newMode,
                      FILE_ID_OPT) == TCL_ERROR)
        return TCL_ERROR;

    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_ChmodCmd --
 *     Implements the TCL chmod command:
 *     chmod [fileid] mode filelist
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ChmodCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int           argIdx, idx, fileArgc, fileIds, result;
    modeInfo_t    modeInfo;
    char        **fileArgv;

    /*
     * Options are not parsable just looking for "-", since modes can
     * start with "-".
     */
    fileIds = FALSE;
    argIdx = 1;
    if ((argc > 1) && (STREQU (argv [argIdx], FILE_ID_OPT))) {
        fileIds = TRUE;
        argIdx++;
    }

    if (argIdx != argc - 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " [-fileid] mode filelist", (char *) NULL);
        return TCL_ERROR;
    }

    if (ISDIGIT (argv [argIdx][0])) {
        if (Tcl_GetInt (interp, argv [argIdx], &modeInfo.absMode) != TCL_OK)
            return TCL_ERROR;
        modeInfo.symMode = NULL;
    } else {
        modeInfo.symMode = argv [argIdx];
    }

    if (Tcl_SplitList (interp, argv [argIdx + 1], &fileArgc,
                       &fileArgv) != TCL_OK)
        return TCL_ERROR;

    result = TCL_OK;
    for (idx = 0; (idx < fileArgc) && (result == TCL_OK); idx++) {
        if (fileIds) {
            result = ChmodFileId (interp, modeInfo, fileArgv [idx]); 
        } else {
            result = ChmodFileName (interp, modeInfo, fileArgv [idx]);
        }
    }

    ckfree ((char *) fileArgv);
    return result;
}

/*-----------------------------------------------------------------------------
 * Tcl_ChownCmd --
 *     Implements the TCL chown command:
 *     chown [-fileid] userGrpSpec filelist
 *
 * The valid formats of userGrpSpec are:
 *   {owner}. {owner group} or {owner {}}
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *-----------------------------------------------------------------------------
 */
int
Tcl_ChownCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int argIdx, ownerArgc, fileArgc, fileIds;
    char **ownerArgv = NULL, **fileArgv = NULL, *owner, *group;
    unsigned options;

    /*
     * Parse options.
     */
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
                          " [-fileid] user|{user group} filelist",
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Parse the owner/group parameter.
     */
    if (Tcl_SplitList (interp, argv [argIdx],
                       &ownerArgc, &ownerArgv) != TCL_OK)
        return TCL_ERROR;

    if ((ownerArgc < 1) || (ownerArgc > 2)) {
        Tcl_AppendResult (interp, "owner arg should be: user or {user group}",
                          (char *) NULL);
        goto errorExit;
    }
    options = TCLX_CHOWN;
    owner = ownerArgv [0];
    group = NULL;
    if (ownerArgc == 2) {
        options |= TCLX_CHGRP;
        if (ownerArgv [1][0] != '\0')
            group = ownerArgv [1];
    }

    /*
     * Split the list of file names or channel ids.
     */
    if (Tcl_SplitList (interp, argv [argIdx + 1], &fileArgc,
                       &fileArgv) != TCL_OK)
        goto errorExit;

    /*
     * Finally, change ownership.
     */
    if (fileIds) {
        if (TclXOSFChangeOwnGrp (interp, options, owner, group,
                                 fileArgv, "chown -fileid") != TCL_OK)
            goto errorExit;
    } else {
        if (TclXOSChangeOwnGrp (interp, options, owner, group,
                                fileArgv, "chown") != TCL_OK)
            goto errorExit;
    }

    ckfree ((char *) ownerArgv);
    ckfree ((char *) fileArgv);
    return TCL_OK;

  errorExit:
    ckfree ((char *) ownerArgv);
    ckfree ((char *) fileArgv);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_ChgrpCmd --
 *     Implements the TCL chgrp command:
 *     chgrp [-fileid] group filelist
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ChgrpCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int argIdx, fileArgc, fileIds;
    char  **fileArgv;

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
                          " [-fileid] group filelist", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_SplitList (interp, argv [argIdx + 1], &fileArgc,
                       &fileArgv) != TCL_OK)
        return TCL_ERROR;
    
    if (fileIds) {
        if (TclXOSFChangeOwnGrp (interp, TCLX_CHGRP, NULL, argv [argIdx],
                                 fileArgv, "chgrp - fileid") != TCL_OK)
            goto errorExit;
    } else {
        if (TclXOSChangeOwnGrp (interp, TCLX_CHGRP, NULL, argv [argIdx],
                                fileArgv, "chgrp") != TCL_OK)
            goto errorExit;
    }

    ckfree ((char *) fileArgv);
    return TCL_OK;

  errorExit:
    ckfree ((char *) fileArgv);
    return TCL_ERROR;
}
