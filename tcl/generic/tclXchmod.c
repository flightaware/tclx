/* 
 * tclXchmod.c --
 *
 *    Chmod, chown and chgrp Tcl commands.
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
 * $Id: tclXchmod.c,v 2.4 1993/07/27 07:42:35 markd Exp markd $
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
 * Prototypes of internal functions.
 */
static int
ConvSymMode _ANSI_ARGS_((Tcl_Interp  *interp,
                         char        *symMode,
                         int          modeVal));

static int
ConvertGroupId _ANSI_ARGS_((Tcl_Interp  *interp,
                            char        *strId,
                            gid_t       *groupIdPtr));

static int
ConvertUserGroup _ANSI_ARGS_((Tcl_Interp  *interp,
                              char        *ownerGroupList,
                              ownerInfo_t *ownerInfoPtr));

/*
 *-----------------------------------------------------------------------------
 *
 * ConvSymMode --
 *      Parse and convert symbolic file permissions as specified by chmod(C).
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o symMode - The symbolic permissions to parse.
 *   o modeVal - The existing permissions value on a file.
 *
 * Results:
 *      The new permissions, or -1 if invalid permissions where supplied.
 *
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
 * Tcl_ChmodCmd --
 *     Implements the TCL chmod command:
 *     chmod mode filelist
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
    int           idx, modeVal, fileArgc, absMode;
    char        **fileArgv, *fileName;
    struct stat   fileStat;
    Tcl_DString   tildeBuf;

    Tcl_DStringInit (&tildeBuf);

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " mode filelist", (char *) NULL);
        return TCL_ERROR;
    }

    if (isdigit (argv [1][0])) {
        if (Tcl_GetInt (interp, argv [1], &modeVal) != TCL_OK)
            return TCL_ERROR;
        absMode = TRUE;
    } else
        absMode = FALSE;

    if (Tcl_SplitList (interp, argv [2], &fileArgc, &fileArgv) != TCL_OK)
        return TCL_ERROR;

    for (idx = 0; idx < fileArgc; idx++) {
        fileName = Tcl_TildeSubst (interp, fileArgv [idx], &tildeBuf);
        if (fileName == NULL)
            goto errorExit;
        
        if (!absMode) {
            if (stat (fileName, &fileStat) != 0)
                goto fileError;
            modeVal = ConvSymMode (interp, argv [1], fileStat.st_mode & 07777);
            if (modeVal < 0)
                goto errorExit;
        }
        if (chmod (fileName, (unsigned short) modeVal) < 0)
            goto fileError;

        Tcl_DStringFree (&tildeBuf);
    }

    ckfree ((char *) fileArgv);
    return TCL_OK;

  fileError:
    /*
     * Error accessing file, assumes file name is fileArgv [idx].
     */
    Tcl_AppendResult (interp, fileArgv [idx], ": ", Tcl_PosixError (interp),
                      (char *) NULL);

  errorExit:
    Tcl_DStringFree (&tildeBuf);
    ckfree ((char *) fileArgv);
    return TCL_ERROR;
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
    ckfree (ownArgv);
    endpwent ();
    return TCL_OK;

  unknownUser:
    Tcl_AppendResult (interp, "unknown user id: ", ownArgv [0], (char *) NULL);
    goto errorExit;

  noUserForGroup:
    Tcl_AppendResult (interp, "can't find group for user id: ", ownArgv [0],
                      (char *) NULL);

  errorExit:
    ckfree (ownArgv);
    endpwent ();
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ChownCmd --
 *     Implements the TCL chown command:
 *     chown user filelist
 *     chown {user group} filelist
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
    int            idx, fileArgc;
    char         **fileArgv, *fileName;
    ownerInfo_t    ownerInfo;
    struct stat    fileStat;
    Tcl_DString    tildeBuf;

    Tcl_DStringInit (&tildeBuf);

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " user|{user group} filelist", (char *) NULL);
        return TCL_ERROR;
    }
    
    if (ConvertUserGroup (interp, argv [1], &ownerInfo) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_SplitList (interp, argv [2], &fileArgc, &fileArgv) != TCL_OK)
        return TCL_ERROR;

    for (idx = 0; idx < fileArgc; idx++) {
        fileName = Tcl_TildeSubst (interp, fileArgv [idx], &tildeBuf);
        if (fileName == NULL)
            goto errorExit;
        
        if (!ownerInfo.changeGroup) {
            if (stat (fileName, &fileStat) != 0)
                goto fileError;
            ownerInfo.groupId = fileStat.st_gid;
        }
        if (chown (fileName, ownerInfo.userId, ownerInfo.groupId) < 0)
                goto fileError;

        Tcl_DStringFree (&tildeBuf);
    }

    ckfree ((char *) fileArgv);
    return TCL_OK;

  fileError:
    Tcl_AppendResult (interp, fileArgv [idx], ": ",
                      Tcl_PosixError (interp), (char *) NULL);
  errorExit:
    Tcl_DStringFree (&tildeBuf);
    ckfree ((char *) fileArgv);
    return TCL_ERROR;;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ChgrpCmd --
 *     Implements the TCL chgrp command:
 *     chgrp group filelist
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
    int            idx, fileArgc;
    gid_t          groupId;
    char         **fileArgv, *fileName;
    struct stat    fileStat;
    Tcl_DString    tildeBuf;

    Tcl_DStringInit (&tildeBuf);

    if (argc < 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " group filelist", (char *) NULL);
        return TCL_ERROR;
    }

    if (ConvertGroupId (interp, argv [1], &groupId) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_SplitList (interp, argv [2], &fileArgc, &fileArgv) != TCL_OK)
        return TCL_ERROR;

    for (idx = 0; idx < fileArgc; idx++) {
        fileName = Tcl_TildeSubst (interp, fileArgv [idx], &tildeBuf);
        if (fileName == NULL)
            goto errorExit;
        
        if ((stat (fileName, &fileStat) != 0) ||
                (chown (fileArgv[idx], fileStat.st_uid, groupId) < 0)) {
            Tcl_AppendResult (interp, fileArgv [idx], ": ",
                              Tcl_PosixError (interp), (char *) NULL);
            goto fileError;
        }

        Tcl_DStringFree (&tildeBuf);
    }

    ckfree ((char *) fileArgv);
    return TCL_OK;

  fileError:
    Tcl_AppendResult (interp, fileArgv [idx], ": ",
                      Tcl_PosixError (interp), (char *) NULL);
  errorExit:
    Tcl_DStringFree (&tildeBuf);
    ckfree ((char *) fileArgv);
    return TCL_ERROR;;
}
