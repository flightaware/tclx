/* 
 * tclXlist.c --
 *
 *  Extended Tcl list commands.
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
 * $Id: tclXlist.c,v 5.1 1996/02/12 18:16:02 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*-----------------------------------------------------------------------------
 * Tcl_LvarcatCmd --
 *     Implements the TCL lvarcat command:
 *         lvarcat var string ?string...?
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
int
Tcl_LvarcatCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int        listArgc, idx, listIdx;
    char     **listArgv;
    char      *staticArgv [12];
    char      *varContents, *newStr, *result;

    if (argc < 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " var string ?string...?", (char *) NULL);
        return TCL_ERROR;
    }

    varContents = Tcl_GetVar (interp, argv[1], 0);

    if (varContents != NULL)
        listArgc = argc - 1;
    else
        listArgc = argc - 2;

    if (listArgc < (sizeof (staticArgv) / sizeof (char *))) {
        listArgv = staticArgv;
    } else {
        listArgv = (char **) ckalloc (listArgc * sizeof (char *));
    }
    
    if (varContents != NULL) {
        listArgv [0] = varContents;
        listIdx = 1;
    } else {
        listIdx = 0;
    }
    for (idx = 2; idx < argc; idx++, listIdx++)
        listArgv [listIdx] = argv [idx];

    newStr = Tcl_Concat (listArgc, listArgv);
    result = Tcl_SetVar (interp, argv [1], newStr, TCL_LEAVE_ERR_MSG);

    ckfree (newStr);
    if (listArgv != staticArgv)
        ckfree ((char *) listArgv);

    /*
     * If all is ok, return the variable contents as a "static" result.
     */
    if (result != NULL) {
        interp->result = result;
        return TCL_OK;
    } else {
        return TCL_ERROR;
    }
}

/*-----------------------------------------------------------------------------
 * Tcl_LvarpopCmd --
 *     Implements the TCL lvarpop command:
 *         lvarpop var ?indexExpr? ?string?
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
int
Tcl_LvarpopCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int        listArgc, idx;
    long       listIdx;
    char     **listArgv;
    char      *varContents, *resultList, *returnElement;

    if ((argc < 2) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " var ?indexExpr? ?string?", (char *) NULL);
        return TCL_ERROR;
    }

    varContents = Tcl_GetVar (interp, argv[1], TCL_LEAVE_ERR_MSG);
    if (varContents == NULL)
        return TCL_ERROR;

    if (Tcl_SplitList (interp, varContents, &listArgc, &listArgv) == TCL_ERROR)
        return TCL_ERROR;

    if (argc == 2) {
        listIdx = 0;
    } else if (Tcl_RelativeExpr (interp, argv[2], listArgc, &listIdx)
               != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * Just ignore out-of bounds requests, like standard Tcl.
     */
    if ((listIdx < 0) || (listIdx >= listArgc)) {
        goto okExit;
    }
    returnElement = listArgv [listIdx];

    if (argc == 4)
        listArgv [listIdx] = argv [3];
    else {
        listArgc--;
        for (idx = listIdx; idx < listArgc; idx++)
            listArgv [idx] = listArgv [idx+1];
    }

    resultList = Tcl_Merge (listArgc, listArgv);
    if (Tcl_SetVar (interp, argv [1], resultList, TCL_LEAVE_ERR_MSG) == NULL) {
        ckfree (resultList);
        goto errorExit;
    }
    ckfree (resultList);

    Tcl_SetResult (interp, returnElement, TCL_VOLATILE);
  okExit:
    ckfree((char *) listArgv);
    return TCL_OK;

  errorExit:
    ckfree((char *) listArgv);
    return TCL_ERROR;;
}

/*-----------------------------------------------------------------------------
 * Tcl_LvarpushCmd --
 *     Implements the TCL lvarpush command:
 *         lvarpush var string ?indexExpr?
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
int
Tcl_LvarpushCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int        listArgc, idx;
    long       listIdx;
    char     **listArgv;
    char      *varContents, *resultList;

    if ((argc < 3) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " var string ?indexExpr?", (char *) NULL);
        return TCL_ERROR;
    }

    varContents = Tcl_GetVar (interp, argv[1], 0);
    if (varContents == NULL)
        varContents = "";

    if (Tcl_SplitList (interp, varContents, &listArgc, &listArgv) == TCL_ERROR)
        return TCL_ERROR;

    if (argc == 3) {
        listIdx = 0;
    } else if (Tcl_RelativeExpr (interp, argv[3], listArgc, &listIdx)
               != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * Out-of-bounds request go to the start or end, as with most of Tcl.
     */
    if (listIdx < 0)
        listIdx = 0;
    else
        if (listIdx > listArgc)
            listIdx = listArgc;

    /*
     * This code takes advantage of the fact that a NULL entry is always
     * returned by Tcl_SplitList, but not required by Tcl_Merge.
     */
    for (idx = listArgc; idx > listIdx; idx--)
        listArgv [idx] = listArgv [idx - 1];

    listArgv [listIdx] = argv [2];

    resultList = Tcl_Merge (listArgc + 1, listArgv);

    if (Tcl_SetVar (interp, argv [1], resultList, TCL_LEAVE_ERR_MSG) == NULL) {
        ckfree (resultList);
        goto errorExit;
    }

    ckfree (resultList);
    ckfree((char *) listArgv);
    return TCL_OK;

  errorExit:
    ckfree((char *) listArgv);
    return TCL_ERROR;;
}

/*-----------------------------------------------------------------------------
 * Tcl_LemptyCmd --
 *     Implements the lempty TCL command:
 *         lempty list
 *
 * Results:
 *     Standard TCL result.
 *-----------------------------------------------------------------------------
 */
int
Tcl_LemptyCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    char *scanPtr;

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " list",
                          (char *) NULL);
        return TCL_ERROR;
    }

    scanPtr = argv [1];
    while ((*scanPtr != '\0') && (ISSPACE (*scanPtr)))
        scanPtr++;
    sprintf (interp->result, "%d", (*scanPtr == '\0'));
    return TCL_OK;

}

/*-----------------------------------------------------------------------------
 * Tcl_LassignCmd --
 *     Implements the TCL assign_fields command:
 *         lassign list varname ?varname...?
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
int
Tcl_LassignCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    int        listArgc, listIdx, idx, remaining;
    char     **listArgv;
    char      *varValue;

    if (argc < 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " list varname ?varname..?", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_SplitList (interp, argv[1], &listArgc, &listArgv) == TCL_ERROR)
        return TCL_ERROR;

    for (idx = 2, listIdx = 0; idx < argc; idx++, listIdx++) {
        varValue = (listIdx < listArgc) ? listArgv[listIdx] : "" ;
        if (Tcl_SetVar (interp, argv[idx], varValue,
                        TCL_LEAVE_ERR_MSG) == NULL) {
            goto error_exit;
        }
    }
    remaining = listArgc - argc + 2;
    if (remaining > 0) {
        Tcl_SetResult (interp,
                       Tcl_Merge (remaining, listArgv + argc - 2),
                       TCL_DYNAMIC);
    }
    ckfree((char *) listArgv);
    return TCL_OK;

  error_exit:
    ckfree((char *) listArgv);
    return TCL_ERROR;
}

/*----------------------------------------------------------------------
 * Tcl_LmatchCmd --
 *
 *      This procedure is invoked to process the "lmatch" Tcl command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *----------------------------------------------------------------------
 */
int
Tcl_LmatchCmd(notUsed, interp, argc, argv)
    ClientData notUsed;                 /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{
#define EXACT   0
#define GLOB    1
#define REGEXP  2
    int listArgc;
    char **listArgv;
    Tcl_DString resultList;
    int i, match, mode;

    mode = GLOB;
    if (argc == 4) {
        if (STREQU(argv[1], "-exact")) {
            mode = EXACT;
        } else if (STREQU(argv[1], "-glob")) {
            mode = GLOB;
        } else if (STREQU(argv[1], "-regexp")) {
            mode = REGEXP;
        } else {
            Tcl_AppendResult(interp, "bad search mode \"", argv[1],
                    "\": must be -exact, -glob, or -regexp", (char *) NULL);
            return TCL_ERROR;
        }
    } else if (argc != 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " ?mode? list pattern\"", (char *) NULL);
        return TCL_ERROR;
    }
    if (Tcl_SplitList(interp, argv[argc-2], &listArgc, &listArgv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (listArgc == 0) {
        ckfree ((char *) listArgv);
        return TCL_OK;
    }
    
    Tcl_DStringInit (&resultList);
    for (i = 0; i < listArgc; i++) {
        match = 0;
        switch (mode) {
            case EXACT:
                match = (STREQU (listArgv [i], argv [argc-1]));
                break;
            case GLOB:
                match = Tcl_StringMatch (listArgv [i], argv [argc-1]);
                break;
            case REGEXP:
                match = Tcl_RegExpMatch (interp, listArgv [i], argv [argc-1]);
                if (match < 0) {
                    ckfree ((char *) listArgv);
                    Tcl_DStringFree (&resultList);
                    return TCL_ERROR;
                }
                break;
        }
        if (match) {
            Tcl_DStringAppendElement (&resultList, listArgv [i]);
        }
    }
    ckfree ((char *) listArgv);
    Tcl_DStringResult (interp, &resultList);
    return TCL_OK;
}
