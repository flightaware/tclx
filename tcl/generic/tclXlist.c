/* 
 * tclXlist.c --
 *
 *  Extended Tcl list commands.
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
 * $Id: tclXlist.c,v 8.3 1997/06/25 09:07:52 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Only do look up of type once, its static.
 */
static Tcl_ObjType *listType;

static int
TclX_LvarcatObjCmd _ANSI_ARGS_((ClientData   clientData,
                                Tcl_Interp  *interp,
                                int          objc,
                                Tcl_Obj     *CONST objv[]));

static int
TclX_LvarpopObjCmd _ANSI_ARGS_((ClientData   clientData,
                                Tcl_Interp  *interp,
                                int          objc,
                                Tcl_Obj    *CONST objv[]));

static int
TclX_LvarpushObjCmd _ANSI_ARGS_((ClientData   clientData,
                                 Tcl_Interp  *interp,
                                 int          objc,
                                 Tcl_Obj    *CONST objv[]));

static int
TclX_LemptyObjCmd _ANSI_ARGS_((ClientData   clientData,
                               Tcl_Interp  *interp,
                               int          objc,
                               Tcl_Obj    *CONST objv[]));

static int
TclX_LassignObjCmd _ANSI_ARGS_((ClientData   clientData,
                                Tcl_Interp  *interp,
                                int          objc,
                                Tcl_Obj    *CONST objv[]));

static int
TclX_LmatchObjCmd _ANSI_ARGS_((ClientData   clientData,
                               Tcl_Interp  *interp,
                               int          objc,
                               Tcl_Obj    *CONST objv[]));

static int
TclX_LcontainObjCmd _ANSI_ARGS_((ClientData   clientData,
                                 Tcl_Interp  *interp,
                                 int          objc,
                                 Tcl_Obj    *CONST objv[]));


/*-----------------------------------------------------------------------------
 * TclX_LvarcatObjCmd --
 *   Implements the TclX lvarcat command:
 *      lvarcat var string ?string...?
 *-----------------------------------------------------------------------------
 */
static int
TclX_LvarcatObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    *CONST objv[];
{
    Tcl_Obj *strVarPtr, *strObjPtr;
    int strArgc, idx, argIdx, strLen;
    char **strArgv, *staticArgv [32], *newStr;

/*FIX: Not binary clean */
    if (objc < 3) {
        return TclX_WrongArgs (interp, objv [0], "var string ?string...?");
    }
    strArgv = staticArgv;

    /*
     * Get the variable that we are going to update.  If the var doesn't exist,
     * create it.  If it is shared by more than being a variable, duplicated
     * it.
     */
    strVarPtr = Tcl_ObjGetVar2 (interp, objv [1], NULL, TCL_PARSE_PART1);

    /*
     * FIX: Figure out how to do this without converting to strings, or if
     * that would even be compatible.  Maybe if all lists, then build a big
     * list, otherwise, this way.
     */
    if (strVarPtr != NULL) {
        strArgc = objc - 1;
    } else {
        strArgc = objc - 2;
    }

    if (strArgc >= (sizeof (staticArgv) / sizeof (char *))) {
        strArgv = (char **) ckalloc (strArgc * sizeof (char *));
    }
    
    if (strVarPtr != NULL) {
        strArgv [0] = Tcl_GetStringFromObj (strVarPtr, &strLen);
        argIdx = 1;
    } else {
        argIdx = 0;
    }
    for (idx = 2; idx < objc; idx++, argIdx++) {
        strArgv [argIdx] = Tcl_GetStringFromObj (objv [idx], &strLen);
    }

    newStr = Tcl_Concat (strArgc, strArgv);
    strObjPtr = Tcl_NewStringObj (newStr, -1);
    ckfree (newStr);

    if (strArgv != staticArgv)
        ckfree ((char *) strArgv);

    if (Tcl_ObjSetVar2 (interp, objv [1], NULL, strObjPtr,
                        TCL_PARSE_PART1 | TCL_LEAVE_ERR_MSG) == NULL) {
        Tcl_DecrRefCount (strObjPtr);
        return TCL_ERROR;
    }
    Tcl_SetObjResult (interp, strObjPtr);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_LvarpopObjCmd --
 *   Implements the TclX lvarpop command:
 *      lvarpop var ?indexExpr? ?string?
 *-----------------------------------------------------------------------------
 */
static int
TclX_LvarpopObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    *CONST objv[];
{
    Tcl_Obj *listVarPtr, *listPtr, *returnElemPtr = NULL;
    int listIdx, listLen;

    if ((objc < 2) || (objc > 4)) {
        return TclX_WrongArgs (interp, objv [0], "var ?indexExpr? ?string?");
    }

    listVarPtr = Tcl_ObjGetVar2 (interp, objv [1], NULL, 
                                 TCL_PARSE_PART1 | TCL_LEAVE_ERR_MSG);
    if (listVarPtr == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_IsShared (listVarPtr)) {
        listPtr = Tcl_DuplicateObj (listVarPtr);
        listVarPtr = Tcl_ObjSetVar2 (interp, objv [1], NULL, listPtr,
                                     TCL_PARSE_PART1 | TCL_LEAVE_ERR_MSG);
        if (listVarPtr == NULL) {
            Tcl_DecrRefCount (listPtr);
            return TCL_ERROR;
        }
        if (listVarPtr != listPtr)
            Tcl_DecrRefCount (listPtr);
    }
    listPtr = listVarPtr;

    /*
     * Get the index of the entry in the list we are doing to replace/delete.
     * Just ignore out-of bounds requests, like standard Tcl.
     */
    if (Tcl_ListObjLength (interp, listPtr, &listLen) != TCL_OK)
        goto errorExit;

    if (objc == 2) {
        listIdx = 0;
    } else if (TclX_RelativeExpr (interp, objv [2],
                                  listLen, &listIdx) != TCL_OK) {
        goto errorExit;
    }
    if ((listIdx < 0) || (listIdx >= listLen)) {
        goto okExit;
    }

    /*
     * Get the element that is doing to be deleted/replaced.
     */
    if (Tcl_ListObjIndex (interp, listPtr, listIdx, &returnElemPtr) != TCL_OK)
        goto errorExit;
    Tcl_IncrRefCount (returnElemPtr);

    /*
     * Either replace or delete the element.
     */
    if (objc == 4) {
        if (Tcl_ListObjReplace (interp, listPtr, listIdx, 1,
                                1, &(objv [3])) != TCL_OK)
            goto errorExit;
    } else {
        if (Tcl_ListObjReplace (interp, listPtr, listIdx, 1,
                                0, NULL) != TCL_OK)
            goto errorExit;
    }

    Tcl_SetObjResult (interp, returnElemPtr);

  okExit:
    if (returnElemPtr != NULL)
        Tcl_DecrRefCount (returnElemPtr);
    return TCL_OK;

  errorExit:
    if (returnElemPtr != NULL)
        Tcl_DecrRefCount (returnElemPtr);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_LvarpushObjCmd --
 *   Implements the TclX lvarpush command:
 *      lvarpush var string ?indexExpr?
 *-----------------------------------------------------------------------------
 */
static int
TclX_LvarpushObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    *CONST objv[];
{
    Tcl_Obj *listVarPtr, *listPtr;
    int listIdx, listLen;

    if ((objc < 3) || (objc > 4)) {
        return TclX_WrongArgs (interp, objv [0], "var string ?indexExpr?");
    }

    listVarPtr = Tcl_ObjGetVar2 (interp, objv [1], NULL, TCL_PARSE_PART1);
    if ((listVarPtr == NULL) || (Tcl_IsShared (listVarPtr))) {
        if (listVarPtr == NULL) {
            listPtr = Tcl_NewListObj (0, NULL);
        } else {
            listPtr = Tcl_DuplicateObj (listVarPtr);
        }
        listVarPtr = Tcl_ObjSetVar2 (interp, objv [1], NULL, listPtr,
                                     TCL_PARSE_PART1 | TCL_LEAVE_ERR_MSG);
        if (listVarPtr == NULL) {
            Tcl_DecrRefCount (listPtr);
            return TCL_ERROR;
        }
        if (listVarPtr != listPtr)
            Tcl_DecrRefCount (listPtr);
    }
    listPtr = listVarPtr;

    /*
     * Get the index of the entry in the list we are doing to replace/delete.
     * Out-of-bounds request go to the start or end, as with most of Tcl
     * commands.
     */
    if (Tcl_ListObjLength (interp, listPtr, &listLen) != TCL_OK)
        goto errorExit;

    if (objc == 3) {
        listIdx = 0;
    } else if (TclX_RelativeExpr (interp, objv [3],
                                  listLen, &listIdx) != TCL_OK) {
        goto errorExit;
    }
    if (listIdx < 0) {
        listIdx = 0;
    } else {
        if (listIdx > listLen)
            listIdx = listLen;
    }

    if (Tcl_ListObjReplace (interp, listPtr, listIdx, 0,
                            1, &(objv [2])) != TCL_OK)
        goto errorExit;

    return TCL_OK;

  errorExit:
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_LemptyObjCmd --
 *    Implements the TclX lempty command:
 *        lempty list
 *-----------------------------------------------------------------------------
 */
static int
TclX_LemptyObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    *CONST objv[];
{
    int isEmpty, length, idx;
    char *data;
    
    if (objc != 2) {
        return TclX_WrongArgs (interp, objv [0], "list");
    }

    /*
     * A null object.
     */
    if ((objv [1]->typePtr == NULL) && (objv [1]->bytes == NULL)) {
        Tcl_SetBooleanObj (Tcl_GetObjResult (interp), TRUE);
        return TCL_OK;
    }

    /*
     * This is a little tricky, because the pre-object lempty never checked
     * for a valid list, it just checked for a string of all white spaces.
     * If the object is already a list, go off of the length, otherwise scan
     * the string for while space.
     */
    if (objv [1]->typePtr == listType) {
        if (Tcl_ListObjLength (interp, objv [1], &length) != TCL_OK)
            return TCL_ERROR;
        isEmpty = (length == 0);
    } else {
        data = Tcl_GetStringFromObj (objv [1], &length);
        for (idx = 0; (idx < length) && ISSPACE (data [idx]); idx++)
            continue;
        isEmpty = (idx == length);
    }
    Tcl_SetBooleanObj (Tcl_GetObjResult (interp), isEmpty);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_LassignObjCmd --
 *    Implements the TclX assign_fields command:
 *       lassign list varname ?varname...?
 *-----------------------------------------------------------------------------
 */
static int
TclX_LassignObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    *CONST objv[];
{
    int listObjc, listIdx, idx, remaining;
    Tcl_Obj **listObjv, *elemPtr, *remainingObjPtr;
    Tcl_Obj *nullObjPtr = NULL;

    if (objc < 3) {
        return TclX_WrongArgs (interp, objv [0],
                               "list varname ?varname..?");
    }
    if (Tcl_ListObjGetElements (interp, objv [1],
                                &listObjc, &listObjv) != TCL_OK)
        return TCL_ERROR;

    /*
     * Assign elements to specified variables.  If there are not enough
     * elements, set the variables to a NULL object.
     */
    for (idx = 2, listIdx = 0; idx < objc; idx++, listIdx++) {
        if (listIdx < listObjc) {
            elemPtr = listObjv [listIdx];
        } else {
            if (nullObjPtr == NULL) {
                nullObjPtr = Tcl_NewObj ();
                Tcl_IncrRefCount (nullObjPtr);
            }
            elemPtr = nullObjPtr;
        }
        if (Tcl_ObjSetVar2 (interp, objv [idx], NULL, elemPtr,
                            TCL_PARSE_PART1) == NULL)
            goto error_exit;
    }

    /*
     * Return remaining elements as a list.
     */
    remaining = listObjc - objc + 2;
    if (remaining > 0) {
        remainingObjPtr = Tcl_NewListObj (remaining, &(listObjv [objc - 2]));
        Tcl_SetObjResult (interp, remainingObjPtr);
    }

    if (nullObjPtr != NULL)
        Tcl_DecrRefCount (nullObjPtr);
    return TCL_OK;

  error_exit:
    if (nullObjPtr != NULL)
        Tcl_DecrRefCount (nullObjPtr);
    return TCL_ERROR;
}

/*----------------------------------------------------------------------
 * TclX_LmatchObjCmd --
 *   Implements the TclX lmatch command:
 *       lmatch ?-exact|-glob|-regexp? list pattern
 *----------------------------------------------------------------------
 */
static int
TclX_LmatchObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    *CONST objv[];
{
#define EXACT   0
#define GLOB    1
#define REGEXP  2
    int listObjc, idx, match, mode, strLen;
    char *modeStr, *patternStr, *valueStr;
    Tcl_Obj **listObjv, *matchedListPtr = NULL;

/*FIX: Not binary clean (at least on -exact)*/
    mode = GLOB;
    if (objc == 4) {
        modeStr = Tcl_GetStringFromObj (objv [1], &strLen);
        if (STREQU (modeStr, "-exact")) {
            mode = EXACT;
        } else if (STREQU (modeStr, "-glob")) {
            mode = GLOB;
        } else if (STREQU (modeStr, "-regexp")) {
            mode = REGEXP;
        } else {
            TclX_StringAppendObjResult (interp,
                                        "bad search mode \"", modeStr,
                                        "\": must be -exact, -glob, or ",
                                        "-regexp", (char *) NULL);
            return TCL_ERROR;
        }
    } else if (objc != 3) {
        return TclX_WrongArgs (interp, objv [0], "?mode? list pattern");
    }

    if (Tcl_ListObjGetElements (interp, objv [objc - 2],
                                &listObjc, &listObjv) != TCL_OK)
        return TCL_ERROR;

    patternStr = Tcl_GetStringFromObj (objv [objc - 1], &strLen);

    for (idx = 0; idx < listObjc; idx++) {
        match = 0;
        valueStr = Tcl_GetStringFromObj (listObjv [idx], &strLen);
        switch (mode) {
            case EXACT:
                match = STREQU (valueStr, patternStr);
                break;
            case GLOB:
                match = Tcl_StringMatch (valueStr, patternStr);
                break;
            case REGEXP:
                match = Tcl_RegExpMatch (interp, valueStr, patternStr);
                if (match < 0) {
                    goto errorExit;
                }
                break;
        }
        if (match) {
            if (matchedListPtr == NULL)
                matchedListPtr = Tcl_NewListObj (0, NULL);
            if (Tcl_ListObjAppendElement (interp, matchedListPtr,
                                          listObjv [idx]) != TCL_OK)
                goto errorExit;
        }
    }
    if (matchedListPtr != NULL) {
        Tcl_SetObjResult (interp, matchedListPtr);
    }
    return TCL_OK;
    
  errorExit:
    if (matchedListPtr != NULL)
        Tcl_DecrRefCount (matchedListPtr);
    return TCL_ERROR;
}

/*----------------------------------------------------------------------
 * TclX_LcontainObjCmd --
 *   Implements the TclX lcontain command:
 *       lcontain list element
 *----------------------------------------------------------------------
 */
static int
TclX_LcontainObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    *CONST objv[];
{
    int listObjc, idx, strLen;
    Tcl_Obj **listObjv;
    char *elementStr, *checkStr;

    if (objc != 3) {
        return TclX_WrongArgs (interp, objv [0], 
                               "list element");
    }

/*FIX: Not binary clean */
/*FIX: Could optimize to not convert to string. */
    if (Tcl_ListObjGetElements (interp, objv [1],
                                &listObjc, &listObjv) != TCL_OK)
        return TCL_ERROR;

    checkStr = Tcl_GetStringFromObj (objv [2], &strLen);
    
    for (idx = 0; idx < listObjc; idx++) {
        elementStr = Tcl_GetStringFromObj (listObjv [idx], &strLen);
        if (STREQU (elementStr, checkStr))
            break;
    }
    Tcl_SetBooleanObj (Tcl_GetObjResult (interp), (idx < listObjc));
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_ListInit --
 *   Initialize the list commands in an interpreter.
 *
 * Parameters:
 *   o interp - Interpreter to add commands to.
 *-----------------------------------------------------------------------------
 */
void
TclX_ListInit (interp)
    Tcl_Interp *interp;
{
    listType = Tcl_GetObjType ("list");

    Tcl_CreateObjCommand(interp, 
			 "lvarcat", 
			 TclX_LvarcatObjCmd, 
                         (ClientData) NULL, 
			 (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp, 
			 "lvarpop", 
			 TclX_LvarpopObjCmd, 
                         (ClientData) NULL,
			 (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp, 
			 "lvarpush",
			 TclX_LvarpushObjCmd, 
                         (ClientData) NULL,
			 (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp,
                         "lempty",
			 TclX_LemptyObjCmd, 
                         (ClientData) NULL,
			 (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp, 
			 "lassign",
			 TclX_LassignObjCmd, 
                         (ClientData) NULL,
			 (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp,
			 "lmatch",
			 TclX_LmatchObjCmd, 
                         (ClientData) NULL,
			 (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateObjCommand(interp, 
			 "lcontain",
			 TclX_LcontainObjCmd, 
                         (ClientData) NULL,
			 (Tcl_CmdDeleteProc*) NULL);
}


