/* 
 * tclXchmod.c --
 *
 *    Chmod, chown and chgrp Tcl commands.
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
 * $Id: tclXchmod.c,v 5.3 1996/02/20 09:10:04 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Type used for returning parsed owner informtion.
 */
typedef struct {
    int    changeGroup;
    uid_t  userId;
    gid_t  groupId;
} ownerInfo_t;

/*
 * Type used for returning parsed mode informtion.
 */
typedef struct {
    char  *symMode;  /* Symbolic mode. If NULL, use absolute mode. */
    int    absMode;  /* Numeric mode. */
} modeInfo_t;

static char *FILE_ID_OPT = "-fileid";
static char *FILE_ID_NOT_AVAIL =
    "The -fileid option is not available on this system";

/*
 * Prototypes of internal functions.
 */
static int
ConvSymMode _ANSI_ARGS_((Tcl_Interp  *interp,
                         char        *symMode,
                         int          modeVal));

static int
ChmodFileName _ANSI_ARGS_((Tcl_Interp  *interp,
                           modeInfo_t   modeInfo,
                           char        *fileName));

static int
ChmodFileId _ANSI_ARGS_((Tcl_Interp  *interp,
                         modeInfo_t   modeInfo,
                         char        *fileId));

static int
ConvertGroupId _ANSI_ARGS_((Tcl_Interp  *interp,
                            char        *strId,
                            gid_t       *groupIdPtr));

static int
ConvertUserGroup _ANSI_ARGS_((Tcl_Interp  *interp,
                              char        *ownerGroupList,
                              ownerInfo_t *ownerInfoPtr));

static int
ChownFileName _ANSI_ARGS_((Tcl_Interp  *interp,
                           ownerInfo_t  ownerInfo,
                           char        *fileName));

static int
ChownFileId _ANSI_ARGS_((Tcl_Interp  *interp,
                         ownerInfo_t  ownerInfo,
                         char        *fileId));

static int
ChgrpFileName _ANSI_ARGS_((Tcl_Interp  *interp,
                           gid_t        groupId,
                           char        *fileName));

static int
ChgrpFileId _ANSI_ARGS_((Tcl_Interp  *interp,
                         gid_t        groupId,
                         char        *fileId));

/*
 *-----------------------------------------------------------------------------
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

/*
 *-----------------------------------------------------------------------------
 *
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
    if (chmod (filePath, (unsigned short) newMode) < 0)
        goto fileError;

    Tcl_DStringFree (&pathBuf);
    return TCL_OK;

  fileError:
    Tcl_AppendResult (interp, filePath, ": ",
                      Tcl_PosixError (interp), (char *) NULL);
  errorExit:
    Tcl_DStringFree (&pathBuf);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
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
#ifndef NO_FCHMOD
    int fnum;
    struct stat fileStat;
    int newMode;

    fnum = TclX_GetOpenFnum (interp, fileId, 0);
    if (fnum < 0)
        return TCL_ERROR;

    if (modeInfo.symMode != NULL) {
        if (fstat (fnum, &fileStat) != 0)
            goto fileError;
        newMode = ConvSymMode (interp, modeInfo.symMode,
                               fileStat.st_mode & 07777);
        if (newMode < 0)
            return TCL_ERROR;
    } else {
        newMode = modeInfo.absMode;
    }
    if (fchmod (fnum, (unsigned short) newMode) < 0)
        goto fileError;

    return TCL_OK;

  fileError:
    Tcl_AppendResult (interp, fileId, ": ",
                      Tcl_PosixError (interp), (char *) NULL);
    return TCL_ERROR;
#else
    interp->result = FILE_ID_NOT_AVAIL;
    return TCL_ERROR;
#endif
}

/*
 *-----------------------------------------------------------------------------
 *
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

/*
 *-----------------------------------------------------------------------------
 *
 * ConvertGroupId --
 *   Convert a group name of string id number to a group id.
 *-----------------------------------------------------------------------------
 */
static int
ConvertGroupId (interp, strId, groupIdPtr)
    Tcl_Interp  *interp;
    char        *strId;
    gid_t       *groupIdPtr;
{
    int             tmpId;
    struct group   *groupPtr;

    groupPtr = getgrnam (strId);
    if (groupPtr != NULL) {
        *groupIdPtr = groupPtr->gr_gid;
    } else {
        if (!Tcl_StrToInt (strId, 10, &tmpId))
            goto unknownGroup;
        *groupIdPtr = tmpId;
        if ((int) (*groupIdPtr) != tmpId)
            goto unknownGroup;
    }
    endpwent ();
    return TCL_OK;

  unknownGroup:
    Tcl_AppendResult (interp, "unknown group id: ", strId, (char *) NULL);
    endpwent ();
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConvertUserGroup --
 *   Convert the owner/group parameter for the chmod command.  If is one of
 *   {owner}. {owner group} or {owner {}}
 *-----------------------------------------------------------------------------
 */
static int
ConvertUserGroup (interp, userGroupList, ownerInfoPtr)
    Tcl_Interp  *interp;
    char        *userGroupList;
    ownerInfo_t *ownerInfoPtr;
{
    int             ownArgc, tmpId;
    char          **ownArgv;
    struct passwd  *passwdPtr;

    if (Tcl_SplitList (interp, userGroupList, &ownArgc, &ownArgv) != TCL_OK)
        return TCL_ERROR;

    if ((ownArgc < 1) || (ownArgc > 2)) {
        interp->result = "owner arg should be: user or {user group}";
        goto errorExit;
    }

    ownerInfoPtr->changeGroup = (ownArgc == 2);

    passwdPtr = getpwnam (ownArgv [0]);
    if (passwdPtr != NULL) {
        ownerInfoPtr->userId = passwdPtr->pw_uid;
    } else {
        if (!Tcl_StrToInt (ownArgv [0], 10, &tmpId))
            goto unknownUser;
        ownerInfoPtr->userId = tmpId;
        if ((int) ownerInfoPtr->userId != tmpId)
            goto unknownUser;
    }

    if (ownerInfoPtr->changeGroup && (ownArgv [1][0] != '\0')) {
        if (ConvertGroupId (interp, ownArgv [1],
                            &ownerInfoPtr->groupId) != TCL_OK)
            goto errorExit;
    } else {
        if (passwdPtr != NULL) {
            ownerInfoPtr->groupId = passwdPtr->pw_gid;
        } else {
            passwdPtr = getpwuid (ownerInfoPtr->userId);
            if (passwdPtr == NULL)
                goto noUserForGroup;
        }
    }
    ckfree ((char *) ownArgv);
    endpwent ();
    return TCL_OK;

  unknownUser:
    Tcl_AppendResult (interp, "unknown user id: ", ownArgv [0], (char *) NULL);
    goto errorExit;

  noUserForGroup:
    Tcl_AppendResult (interp, "can't find group for user id: ", ownArgv [0],
                      (char *) NULL);

  errorExit:
    ckfree ((char *) ownArgv);
    endpwent ();
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ChownFileName --
 *   Change the owner and/or group of a file by name.
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o ownerInfo - Parse infomation with the owner and/or group id to change.
 *   o fileName - Name of the file to change.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ChownFileName (interp, ownerInfo, fileName)
    Tcl_Interp  *interp;
    ownerInfo_t  ownerInfo;
    char        *fileName;
{
    char         *filePath;
    struct stat   fileStat;
    Tcl_DString   pathBuf;

    Tcl_DStringInit (&pathBuf);

    filePath = Tcl_TranslateFileName (interp, fileName, &pathBuf);
    if (filePath == NULL) {
        Tcl_DStringFree (&pathBuf);
        return TCL_ERROR;
    }

    if (!ownerInfo.changeGroup) {
        if (stat (filePath, &fileStat) != 0)
            goto fileError;
        if (chown (filePath, ownerInfo.userId, fileStat.st_gid) < 0)
            goto fileError;
    } else {
        if (chown (filePath, ownerInfo.userId, ownerInfo.groupId) < 0)
            goto fileError;
    }

    Tcl_DStringFree (&pathBuf);
    return TCL_OK;

  fileError:
    Tcl_AppendResult (interp, filePath, ": ",
                      Tcl_PosixError (interp), (char *) NULL);
    Tcl_DStringFree (&pathBuf);
    return TCL_ERROR;
}


/*
 *-----------------------------------------------------------------------------
 *
 * ChownFileId --
 *   Change the owner and/or group of a file by file id.
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o ownerInfo - Parse infomation with the owner and/or group id to change.
 *   o fileId - The Tcl file id.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ChownFileId (interp, ownerInfo, fileId)
    Tcl_Interp  *interp;
    ownerInfo_t  ownerInfo;
    char        *fileId;
{
#ifndef NO_FCHOWN
    int fnum;
    struct stat fileStat;

    fnum = TclX_GetOpenFnum (interp, fileId, 0);
    if (fnum < 0)
        return TCL_ERROR;

    if (!ownerInfo.changeGroup) {
        if (fstat (fnum, &fileStat) != 0)
            goto fileError;
        if (fchown (fnum, ownerInfo.userId, fileStat.st_gid) < 0)
            goto fileError;
    } else {
        if (fchown (fnum, ownerInfo.userId, ownerInfo.groupId) < 0)
            goto fileError;
    }

    return TCL_OK;

  fileError:
    Tcl_AppendResult (interp, fileId, ": ",
                      Tcl_PosixError (interp), (char *) NULL);
    return TCL_ERROR;
#else
    interp->result = FILE_ID_NOT_AVAIL;
    return TCL_ERROR;
#endif
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ChownCmd --
 *     Implements the TCL chown command:
 *     chown [-fileid] user filelist
 *     chown [-fileid] {user group} filelist
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ChownCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int            argIdx, idx, fileArgc, fileIds, result;
    char         **fileArgv;
    ownerInfo_t    ownerInfo;

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
    
    if (ConvertUserGroup (interp, argv [argIdx], &ownerInfo) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_SplitList (interp, argv [argIdx + 1], &fileArgc,
                       &fileArgv) != TCL_OK)
        return TCL_ERROR;

    result = TCL_OK;
    for (idx = 0; (idx < fileArgc) && (result == TCL_OK); idx++) {
        if (fileIds) {
            result = ChownFileId (interp, ownerInfo, fileArgv [idx]);
        } else {
            result = ChownFileName (interp, ownerInfo, fileArgv [idx]);
        }
    }

    ckfree ((char *) fileArgv);
    return result;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ChgrpFileName --
 *   Change the group of a file by name.
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o groupId - The new group id.
 *   o fileName - Name of the file to change.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ChgrpFileName (interp, groupId, fileName)
    Tcl_Interp  *interp;
    gid_t        groupId;
    char        *fileName;
{
    char         *filePath;
    struct stat   fileStat;
    Tcl_DString   pathBuf;

    Tcl_DStringInit (&pathBuf);

    filePath = Tcl_TranslateFileName (interp, fileName, &pathBuf);
    if (filePath == NULL) {
        Tcl_DStringFree (&pathBuf);
        return TCL_ERROR;
    }

    if ((stat (filePath, &fileStat) != 0) ||
        (chown (filePath, fileStat.st_uid, groupId) < 0)) {
        Tcl_AppendResult (interp, filePath, ": ",
                          Tcl_PosixError (interp), (char *) NULL);
        Tcl_DStringFree (&pathBuf);
        return TCL_ERROR;
    }
    Tcl_DStringFree (&pathBuf);
    return TCL_OK;
}


/*
 *-----------------------------------------------------------------------------
 *
 * ChgrpFileId --
 *   Change the group of a file by file id.
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o groupId - The new group id.
 *   o fileId - The Tcl file id.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ChgrpFileId (interp, groupId, fileId)
    Tcl_Interp  *interp;
    gid_t        groupId;
    char        *fileId;
{
#ifndef NO_FCHOWN
    int fnum;
    struct stat fileStat;

    fnum = TclX_GetOpenFnum (interp, fileId, 0);
    if (fnum < 0)
        return TCL_ERROR;

    if ((fstat (fnum, &fileStat) != 0) ||
        (fchown (fnum, fileStat.st_uid, groupId) < 0)) {
        Tcl_AppendResult (interp, fileId, ": ",
                          Tcl_PosixError (interp), (char *) NULL);
         return TCL_ERROR;
    }
    return TCL_OK;
#else
    interp->result = FILE_ID_NOT_AVAIL;
    return TCL_ERROR;
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
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
    int     argIdx, idx, fileArgc, fileIds, result;
    gid_t   groupId;
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

    if (ConvertGroupId (interp, argv [argIdx], &groupId) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_SplitList (interp, argv [argIdx + 1], &fileArgc,
                       &fileArgv) != TCL_OK)
        return TCL_ERROR;

    result = TCL_OK;
    for (idx = 0; (idx < fileArgc) && (result == TCL_OK); idx++) {
        if (fileIds) {
            result = ChgrpFileId (interp, groupId, fileArgv [idx]); 
        } else {
            result = ChgrpFileName (interp, groupId, fileArgv [idx]);
        }
    }

    ckfree ((char *) fileArgv);
    return result;
}
