/*
 * tclXunixId.c --
 *
 * Tcl commands to access getuid, setuid, getgid, setgid and friends on Unix.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1997 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXunixId.c,v 1.3 1997/01/25 05:38:28 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Actually configured number of groups (from sysconf if we have it).
 */
#ifndef NO_SYSCONF
static int confNGroups = -1;
#else
#ifndef NGROUPS
#   ifdef NGROUPS_MAX
#       define NGROUPS NGROUPS_MAX
#   else
#       define NGROUPS 32
#   endif
#endif
static int confNGroups = NGROUPS;
#endif

/*
 * Prototypes of internal functions.
 */
static int
UseridToUsernameResult _ANSI_ARGS_((Tcl_Interp *interp,
                                    int         userId));

static int
UsernameToUseridResult _ANSI_ARGS_((Tcl_Interp *interp,
                                    char       *userName));

static int
GroupidToGroupnameResult _ANSI_ARGS_((Tcl_Interp *interp,
                                      int         groupId));

static int
GroupnameToGroupidResult _ANSI_ARGS_((Tcl_Interp *interp,
                                      char       *groupName));

static int
IdConvert _ANSI_ARGS_((Tcl_Interp *interp,
                       int         argc,
                       char      **argv));

static int
IdEffective  _ANSI_ARGS_((Tcl_Interp *interp,
                          int         argc,
                          char      **argv));

static int
IdProcess  _ANSI_ARGS_((Tcl_Interp *interp,
                        int         argc,
                        char      **argv));

static int
IdGroupids  _ANSI_ARGS_((Tcl_Interp *interp,
                         int         argc,
                         char      **argv,
                         int         symbolic));

static int
IdHost _ANSI_ARGS_((Tcl_Interp *interp,
                    int         argc,
                    char      **argv));

static int
GetSetWrongArgs _ANSI_ARGS_((Tcl_Interp *interp,
                             char      **argv));

static int
IdUser _ANSI_ARGS_((Tcl_Interp *interp,
                    int         argc,
                    char      **argv));

static int
IdUserId _ANSI_ARGS_((Tcl_Interp *interp,
                      int         argc,
                      char      **argv));

static int
IdGroup _ANSI_ARGS_((Tcl_Interp *interp,
                     int         argc,
                     char      **argv));

static int
IdGroupId _ANSI_ARGS_((Tcl_Interp *interp,
                       int         argc,
                       char      **argv));

/*-----------------------------------------------------------------------------
 * Tcl_IdCmd --
 *     Implements the TclX id command on Unix.
 *
 *        id user ?name?
 *        id convert user <name>
 *
 *        id userid ?uid?
 *        id convert userid <uid>
 *
 *        id group ?name?
 *        id convert group <name>
 *
 *        id groupid ?gid?
 *        id convert groupid <gid>
 *
 *        id groupids
 *
 *        id host
 *
 *        id process
 *        id process parent
 *        id process group
 *        id process group set
 *
 *        id effective user
 *        id effective userid
 *
 *        id effective group
 *        id effective groupid
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */

static int
UseridToUsernameResult (interp, userId)
    Tcl_Interp *interp;
    int         userId;
{
    uid_t          uid = (uid_t) userId;
    struct passwd *pw = getpwuid (userId);

    if ((pw == NULL) || ((int) uid != userId)) {
        sprintf (interp->result, "unknown user id: %d", userId);
        endpwent ();
        return TCL_ERROR;
    }
    Tcl_SetResult (interp, pw->pw_name, TCL_VOLATILE);
    endpwent ();
    return TCL_OK;
}

static int
UsernameToUseridResult (interp, userName)
    Tcl_Interp *interp;
    char       *userName;
{
    struct passwd *pw = getpwnam (userName);

    if (pw == NULL) {
        Tcl_AppendResult (interp, "unknown user id: ", userName, 
                          (char *) NULL);
        endpwent ();
        return TCL_ERROR;
    }
    sprintf (interp->result, "%d", pw->pw_uid);
    endpwent ();
    return TCL_OK;
}

static int
GroupidToGroupnameResult (interp, groupId)
    Tcl_Interp *interp;
    int         groupId;
{
    gid_t         gid = (gid_t) groupId;
    struct group *grp = getgrgid (groupId);

    if ((grp == NULL) || ((int) gid != groupId)) {
        sprintf (interp->result, "unknown group id: %d", groupId);
        endgrent ();
        return TCL_ERROR;
    }
    Tcl_SetResult (interp, grp->gr_name, TCL_VOLATILE);
    endgrent ();
    return TCL_OK;
}

static int
GroupnameToGroupidResult (interp, groupName)
    Tcl_Interp *interp;
    char       *groupName;
{
    struct group *grp = getgrnam (groupName);
    if (grp == NULL) {
        Tcl_AppendResult (interp, "unknown group id: ", groupName,
                          (char *) NULL);
        return TCL_ERROR;
    }
    sprintf (interp->result, "%d", grp->gr_gid);
    return TCL_OK;
}

/*
 * id convert type value
 */
static int
IdConvert (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int uid, gid;

    if (argc != 4) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " convert type value", (char *) NULL);
        return TCL_ERROR;
    }

    if (STREQU (argv[2], "user"))
        return UsernameToUseridResult (interp, argv[3]);
    
    if (STREQU (argv[2], "userid")) {
        if (Tcl_GetInt (interp, argv[3], &uid) != TCL_OK) 
            return TCL_ERROR;
        return UseridToUsernameResult (interp, uid);
    }
    
    if (STREQU (argv[2], "group"))
        return GroupnameToGroupidResult (interp, argv[3]);
    
    if (STREQU (argv[2], "groupid")) {
        if (Tcl_GetInt (interp, argv[3], &gid) != TCL_OK)
            return TCL_ERROR;
        return GroupidToGroupnameResult (interp, gid);
        
    }
    Tcl_AppendResult (interp, "third arg must be \"user\", \"userid\", ",
                      "\"group\" or \"groupid\", got \"", argv [2], "\"",
                      (char *) NULL);
    return TCL_ERROR;
}

/*
 * id effective type
 */
static int
IdEffective (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " effective type", (char *) NULL);
        return TCL_ERROR;
    }
    
    if (STREQU (argv[2], "user"))
        return UseridToUsernameResult (interp, geteuid ());
    
    if (STREQU (argv[2], "userid")) {
        sprintf (interp->result, "%d", geteuid ());
        return TCL_OK;
    }
    
    if (STREQU (argv[2], "group"))
        return GroupidToGroupnameResult (interp, getegid ());
    
    if (STREQU (argv[2], "groupid")) {
        sprintf (interp->result, "%d", getegid ());
        return TCL_OK;
    }

    Tcl_AppendResult (interp, "third arg must be \"user\", \"userid\", ",
                      "\"group\" or \"groupid\", got \"", argv [2], "\"",
                      (char *) NULL);
    return TCL_ERROR;
}

/*
 * id process ?parent|group? ?set?
 */
static int
IdProcess (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    pid_t  pid;

    if (argc > 4) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " process ?parent|group? ?set?",
                          (char *) NULL);
        return TCL_OK;
    }
    if (argc == 2) {
        sprintf (interp->result, "%d", getpid ());
        return TCL_OK;
    }

    if (STREQU (argv[2], "parent")) {
        if (argc != 3) {
            Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                              " process parent", (char *) NULL);
            return TCL_ERROR;
        }
        sprintf (interp->result, "%d", getppid ());
        return TCL_OK;
    }
    if (STREQU (argv[2], "group")) {
        if (argc == 3) {
            sprintf (interp->result, "%d", getpgrp ());
            return TCL_OK;
        }
        if ((argc != 4) || !STREQU (argv[3], "set")) {
            Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                              " process group ?set?", (char *) NULL);
            return TCL_ERROR;
        }

        if (Tcl_IsSafe (interp)) {
            Tcl_AppendResult (interp,
                              "can't set process group from a safe interpeter",
                              (char *) NULL);
            return TCL_ERROR;
        }
                        
#ifndef NO_SETPGID
        pid = getpid ();
        setpgid (pid, pid);
#else
        setpgrp ();
#endif
        return TCL_OK;
    }

    Tcl_AppendResult (interp, "expected one of \"parent\" or \"group\" ",
                      "got \"", argv [2], "\"", (char *) NULL);
    return TCL_ERROR;
}

/*
 * id groupids
 * id groups
 */
static int
IdGroupids (interp, argc, argv, symbolic)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
    int         symbolic;
{
#ifndef NO_GETGROUPS
    gid_t *groups;
    int nGroups, groupIndex;
    char numText [12];
    struct group *grp;

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ", argv [1],
                          (char *) NULL);
        return TCL_ERROR;
    }

#ifndef NO_SYSCONF
    if (confNGroups < 0)
        confNGroups = sysconf (_SC_NGROUPS_MAX);
#endif
    groups = (gid_t *) ckalloc (confNGroups * sizeof (gid_t));


    nGroups = getgroups (confNGroups, groups);
    if (nGroups < 0) {
        interp->result = Tcl_PosixError (interp);
        ckfree ((char *) groups);
        return TCL_ERROR;
    }

    for (groupIndex = 0; groupIndex < nGroups; groupIndex++) {
        if (symbolic) {
            grp = getgrgid (groups [groupIndex]);
            if (grp == NULL) {
                sprintf (interp->result, "unknown group id: %d", 
                         groups [groupIndex]);
                endgrent ();
                return TCL_ERROR;
            }
            Tcl_AppendElement (interp, grp->gr_name);
        } else {
            sprintf (numText, "%d", groups[groupIndex]);
            Tcl_AppendElement (interp, numText);
        }
    }
    if (symbolic)
        endgrent ();
    ckfree ((char *) groups);
    return TCL_OK;
#else
    Tcl_AppendResult (interp, "group id lists unavailable on this system ",
                      "(no getgroups function)", (char *) NULL);
    return TCL_ERROR;
#endif
}

/*
 * id host
 */
static int
IdHost (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
#ifndef NO_GETHOSTNAME
    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " host", (char *) NULL);
        return TCL_ERROR;
    }
	if (gethostname (interp->result, TCL_RESULT_SIZE) < 0) {
	    interp->result = Tcl_PosixError (interp);
	    return TCL_ERROR;
	}
	return TCL_OK;
#else
        Tcl_AppendResult (interp, "host name unavailable on this system ",
                          "(no gethostname function)", (char *) NULL);
        return TCL_ERROR;
#endif
}

/*
 * Return error when a get set function has too many args (2 or 3 expected).
 */
static int
GetSetWrongArgs (interp, argv)
    Tcl_Interp *interp;
    char      **argv;
{
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ", argv [1],
                      " ?value?", (char *) NULL);
    return TCL_ERROR;
}

/*
 * id user
 */
static int
IdUser (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    struct passwd *pw;

    if (argc > 3)
        return GetSetWrongArgs (interp, argv);

    if (argc == 2) {
        return UseridToUsernameResult (interp, getuid ());
    }

    pw = getpwnam (argv[2]);
    if (pw == NULL) {
        Tcl_AppendResult (interp, "user \"", argv[2], "\" does not exist",
                          (char *) NULL);
        goto errorExit;
    }
    if (setuid (pw->pw_uid) < 0) {
        interp->result = Tcl_PosixError (interp);
        goto errorExit;
    }
    endpwent ();
    return TCL_OK;

  errorExit:
    endpwent ();
    return TCL_OK;
}

/*
 * id userid
 */
static int
IdUserId (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int  uid;

    if (argc > 3)
        return GetSetWrongArgs (interp, argv);

    if (argc == 2) {
        sprintf (interp->result, "%d", getuid ());
        return TCL_OK;
    }

    if (Tcl_GetInt (interp, argv[2], &uid) != TCL_OK)
        return TCL_ERROR;

    if (setuid ((uid_t) uid) < 0) {
        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 * id group
 */
static int
IdGroup (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    struct group *grp;

    if (argc > 3)
        return GetSetWrongArgs (interp, argv);

    if (argc == 2) {
        return GroupidToGroupnameResult (interp, getgid ());
    }
     
    grp = getgrnam (argv[2]);
    if (grp == NULL) {
        Tcl_AppendResult (interp, "group \"", argv[2], "\" does not exist",
                          (char *) NULL);
        goto errorExit;
    }
    if (setgid (grp->gr_gid) < 0) {
        interp->result = Tcl_PosixError (interp);
        goto errorExit;
    }
    endgrent ();
    return TCL_OK;

  errorExit:
    endgrent ();
    return TCL_OK;
}

/*
 * id groupid
 */
static int
IdGroupId (interp, argc, argv)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int  gid;

    if (argc > 3)
        return GetSetWrongArgs (interp, argv);

    if (argc == 2) {
        sprintf (interp->result, "%d", getgid ());
        return TCL_OK;
    }

    if (Tcl_GetInt (interp, argv[2], &gid) != TCL_OK)
        return TCL_ERROR;

    if (setgid ((gid_t) gid) < 0) {
        interp->result = Tcl_PosixError (interp);
        return TCL_ERROR;
    }

    return TCL_OK;
}

int
Tcl_IdCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    if (argc < 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " arg ?arg...?",
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * If the first argument is "convert", handle the conversion.
     */
    if (STREQU (argv[1], "convert")) {
        return IdConvert (interp, argc, argv);
    }

    /*
     * If the first argument is "effective", return the effective user ID,
     * name, group ID or name.
     */
    if (STREQU (argv[1], "effective")) {
        return IdEffective (interp, argc, argv);
    }

    /*
     * If the first argument is "process", return the process ID, parent's
     * process ID, process group or set the process group depending on args.
     */
    if (STREQU (argv[1], "process")) {
        return IdProcess (interp, argc, argv);
    }

    /*
     * Handle getting list of groups the user is a member of.
     */
    if (STREQU (argv[1], "groups")) {
        return IdGroupids (interp, argc, argv, TRUE);
    }

    if (STREQU (argv[1], "groupids")) {
        return IdGroupids (interp, argc, argv, FALSE);
    }

    /*
     * Handle returning the host name if its available.
     */
    if (STREQU (argv[1], "host")) {
        return IdHost (interp, argc, argv);
    }

    /*
     * Handle setting or returning the user ID or group ID (by name or number).
     */
    if (STREQU (argv[1], "user")) {
        return IdUser (interp, argc, argv);
    }

    if (STREQU (argv[1], "userid")) {
        return IdUserId (interp, argc, argv);
    }

    if (STREQU (argv[1], "group")) {
        return IdGroup (interp, argc, argv);
    }

    if (STREQU (argv[1], "groupid")) {
        return IdGroupId (interp, argc, argv);
    }

    Tcl_AppendResult (interp, "second arg must be one of \"convert\", ",
                      "\"effective\", \"process\", ",
                      "\"user\", \"userid\", \"group\", \"groupid\", ",
                      "\"groups\", \"groupids\", ",
                      "or \"host\"", (char *) NULL);
    return TCL_ERROR;
}
