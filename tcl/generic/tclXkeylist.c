/* 
 * tclXkeylist.c --
 *
 *  Extended Tcl keyed list commands and interfaces.
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
 * $Id: tclXkeylist.c,v 8.0.4.1 1997/04/14 02:01:47 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Keyed lists are stored as arrays recursively defined objects.  The data
 * portion of a keyed list entry is a Tcl_Obj which may be a keyed list object
 * or any other Tcl object.  Since determine the structure of a keyed list is
 * lazy (you don't know if an element is data or another keyed list) until it
 * is accessed, the object can be transformed into a keyed list from a Tcl
 * string or list.
 */

/*
 * An entry in a keyed list array.
 */
typedef struct {
    char    *key;
    Tcl_Obj *valuePtr;
} keylEntry_t;

/*
 * Internal representation of a keyed list object.
 */
typedef struct {
    int          arraySize;   /* Current slots available in the array.  */
    int          numEntries;  /* Number of actual entries in the array. */
    keylEntry_t *entries;     /* Array of keyed list entries.           */
} keylIntObj_t;

/*
 * Amount to increment array size by when it needs to grow.
 */
#define KEYEDLIST_ARRAY_INCR_SIZE 16

/*
 * Invalidate the string representation (FIX: should be in Tcl).
 */
#define TclX_InvalidateStringRep(objPtr) \
    if (objPtr->bytes != NULL) { \
        ckfree ((char *) objPtr->bytes); \
        keylPtr->bytes = NULL; \
    }

/*
 * Macro to duplicate a child entry of a keyed list if it is share by more
 * than the parent.
 */
#define DupSharedKeyListChild(keylIntPtr, idx) \
    if (keylIntPtr->entries [idx].valuePtr->refCount > 1) { \
        Tcl_Obj *oldValuePtr = keylIntPtr->entries [idx].valuePtr; \
        keylIntPtr->entries [idx].valuePtr = \
            Tcl_DuplicateObj (oldValuePtr); \
        Tcl_DecrRefCount (oldValuePtr); \
    }

/*
 * Macro to validate an keyed list object.
 */
#if TCLX_DEBUG
#   define KEYL_OBJ_ASSERT(keylAIntPtr) \
    {assert (keylAIntPtr->arraySize >= keylAIntPtr->numEntries); \
     assert (keylAIntPtr->arraySize >= 0); \
     assert (keylAIntPtr->numEntries >= 0); \
     assert ((keylAIntPtr->arraySize > 0) ? \
                 (keylAIntPtr->entries != NULL) : TRUE); \
     assert ((keylAIntPtr->numEntries > 0) ? \
                 (keylAIntPtr->entries != NULL) : TRUE);}
#else
#  define KEYL_OBJ_ASSERT(keylPtr)
#endif


/*
 * Prototypes of internal functions.
 */
static keylIntObj_t *
AllocKeyedListIntRep _ANSI_ARGS_((void));

static void
FreeKeyedListData _ANSI_ARGS_((keylIntObj_t *keylIntPtr));

static void
EnsureKeyedListSpace _ANSI_ARGS_((keylIntObj_t *keylIntPtr,
                                  int           newNumEntries));

static void
DeleteKeyedListEntry _ANSI_ARGS_((keylIntObj_t *keylIntPtr,
                                  int           entryIdx));

static int
FindKeyedListEntry _ANSI_ARGS_((keylIntObj_t *keylIntPtr,
                                char         *key,
                                int          *keyLenPtr,
                                char        **nextSubKeyPtr));

static int
StringToKeyedListEntry _ANSI_ARGS_((Tcl_Interp *interp,
                                    char        *str,
                                    keylEntry_t *entryPtr));

static void
DupKeyedListInternalRep _ANSI_ARGS_((Tcl_Obj *srcPtr,
                                     Tcl_Obj *copyPtr));

static void
FreeKeyedListInternalRep _ANSI_ARGS_((Tcl_Obj *keylPtr));

static int
SetKeyedListFromAny _ANSI_ARGS_((Tcl_Interp *interp,
                                 Tcl_Obj    *objPtr));

static void
UpdateStringOfKeyedList _ANSI_ARGS_((Tcl_Obj *keylPtr));

static int 
TclX_KeylgetObjCmd _ANSI_ARGS_((ClientData   clientData,
                                Tcl_Interp  *interp,
                                int          objc,
                                Tcl_Obj    **objv));

static int
TclX_KeylsetObjCmd _ANSI_ARGS_((ClientData   clientData,
                                Tcl_Interp  *interp,
                                int          objc,
                                Tcl_Obj    **objv));

static int 
TclX_KeyldelObjCmd _ANSI_ARGS_((ClientData   clientData,
                                Tcl_Interp  *interp,
                                int          objc,
                                Tcl_Obj    **objv));

static int 
TclX_KeylkeysObjCmd _ANSI_ARGS_((ClientData   clientData,
                                 Tcl_Interp  *interp,
                                 int          objc,
                                 Tcl_Obj    **objv));

/*
 * Type definition.
 */
static Tcl_ObjType keyedListType = {
    "keyedList",              /* name */
    FreeKeyedListInternalRep, /* freeIntRepProc */
    DupKeyedListInternalRep,  /* dupIntRepProc */
    UpdateStringOfKeyedList,  /* updateStringProc */
    SetKeyedListFromAny       /* setFromAnyProc */
};


/*-----------------------------------------------------------------------------
 * AllocKeyedListIntRep --
 *   Allocate an and initialize the keyed list internal representation.
 *
 * Returns:
 *    A pointer to the keyed list internal structure.
 *-----------------------------------------------------------------------------
 */
static keylIntObj_t *
AllocKeyedListIntRep ()
{
    keylIntObj_t *keylIntPtr;

    keylIntPtr = (keylIntObj_t *) ckalloc (sizeof (keylIntObj_t));

    keylIntPtr->arraySize = 0;
    keylIntPtr->numEntries = 0;
    keylIntPtr->entries = NULL;

    return keylIntPtr;
}

/*-----------------------------------------------------------------------------
 * FreeKeyedListData --
 *   Free the internal representation of a keyed list.
 *
 * Parameters:
 *   o keylIntPtr - Keyed list internal structure to free.
 *-----------------------------------------------------------------------------
 */
static void
FreeKeyedListData (keylIntPtr)
    keylIntObj_t *keylIntPtr;
{
    int idx;

    for (idx = 0; idx < keylIntPtr->numEntries ; idx++) {
        ckfree (keylIntPtr->entries [idx].key);
        Tcl_DecrRefCount (keylIntPtr->entries [idx].valuePtr);
    }
    if (keylIntPtr->entries != NULL)
        ckfree ((VOID*) keylIntPtr->entries);
    ckfree ((VOID*) keylIntPtr);
}

/*-----------------------------------------------------------------------------
 * EnsureKeyedListSpace --
 *   Ensure there is enough room in a keyed list array for a certain number
 * of entries, expanding if necessary.
 *
 * Parameters:
 *   o keylIntPtr - Keyed list internal representation.
 *   o newNumEntries - The number of entries that are going to be added to
 *     the keyed list.
 *-----------------------------------------------------------------------------
 */
static void
EnsureKeyedListSpace (keylIntPtr, newNumEntries)
    keylIntObj_t *keylIntPtr;
    int           newNumEntries;
{
    KEYL_OBJ_ASSERT (keylIntPtr);

    if ((keylIntPtr->arraySize - keylIntPtr->numEntries) < newNumEntries) {
        int newSize = keylIntPtr->arraySize + newNumEntries +
            KEYEDLIST_ARRAY_INCR_SIZE;
        if (keylIntPtr->entries == NULL) {
            keylIntPtr->entries = (keylEntry_t *)
                ckalloc (newSize * sizeof (keylEntry_t));
        } else {
            keylIntPtr->entries = (keylEntry_t *)
                ckrealloc ((VOID *) keylIntPtr->entries,
                           newSize * sizeof (keylEntry_t));
        }
        keylIntPtr->arraySize = newSize;
    }

    KEYL_OBJ_ASSERT (keylIntPtr);
}

/*-----------------------------------------------------------------------------
 * DeleteKeyedListEntry --
 *   Delete an entry from a keyed list.
 *
 * Parameters:
 *   o keylIntPtr - Keyed list internal representation.
 *   o entryIdx - Index of entry to delete.
 *-----------------------------------------------------------------------------
 */
static void
DeleteKeyedListEntry (keylIntPtr, entryIdx)
    keylIntObj_t *keylIntPtr;
    int           entryIdx;
{
    ckfree (keylIntPtr->entries [entryIdx].key);
    Tcl_DecrRefCount (keylIntPtr->entries [entryIdx].valuePtr);

    memcpy ((VOID *) &(keylIntPtr->entries [entryIdx]),
            (VOID *) &(keylIntPtr->entries [entryIdx + 1]),
            ((keylIntPtr->numEntries - 1) - entryIdx) * sizeof (keylEntry_t));
    keylIntPtr->numEntries--;

    KEYL_OBJ_ASSERT (keylIntPtr);
}

/*-----------------------------------------------------------------------------
 * FindKeyedListEntry --
 *   Find an entry in keyed list.
 *
 * Parameters:
 *   o keylIntPtr - Keyed list internal representation.
 *   o key - Name of key to search for.
 *   o keyLenPtr - In not NULL, the length of the key for this
 *     level is returned here.  This excludes subkeys and the `.' delimiters.
 *   o nextSubKeyPtr - If not NULL, the start of the name of the next
 *     sub-key within key is returned.
 * Returns:
 *   Index of the entry or -1 if not found.
 *-----------------------------------------------------------------------------
 */
static int
FindKeyedListEntry (keylIntPtr, key, keyLenPtr, nextSubKeyPtr)
    keylIntObj_t *keylIntPtr;
    char         *key;
    int          *keyLenPtr;
    char        **nextSubKeyPtr;
{
    char *keySeparPtr;
    int keyLen, findIdx;

    keySeparPtr = strchr (key, '.');
    if (keySeparPtr != NULL) {
        keyLen = keySeparPtr - key;
    } else {
        keyLen = strlen (key);
    }

    for (findIdx = 0; findIdx < keylIntPtr->numEntries; findIdx++) {
        if (strncmp (keylIntPtr->entries [findIdx].key, key,
                     keyLen) == 0)
            break;
    }

    if (nextSubKeyPtr != NULL) {
        if (keySeparPtr == NULL) {
            *nextSubKeyPtr = NULL;
        } else {
            *nextSubKeyPtr = keySeparPtr + 1;
        }
    }
    if (keyLenPtr != NULL) {
        *keyLenPtr = keyLen;
    }
    
    if (findIdx >= keylIntPtr->numEntries) {
        return -1;
    }
    
    return findIdx;
}

/*-----------------------------------------------------------------------------
 * StringToKeyedListEntry --
 *   Convert an object to a keyed entry.
 *
 * Parameters:
 *   o interp - Used to return error messages, if not NULL.
 *   o str - String to convert.  Each entry must be a two element list,
 *     with the first element being the key and the second being the
 *     value.
 *   o entryPtr - The keyed list entry to initialize from the object.
 * Returns:
 *    TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
StringToKeyedListEntry (interp, str, entryPtr)
    Tcl_Interp  *interp;
    char        *str;
    keylEntry_t *entryPtr;
{
    int listArgc;
    char **listArgv;

    if (Tcl_SplitList (interp, str, &listArgc, &listArgv) != TCL_OK) {
        Tcl_ResetObjResult (interp);
        TclX_StringAppendObjResult (interp,
                                    "keyed list entry not a valid list, ",
                                    "found \"", str, "\"",
                                    (char *) NULL);
        return TCL_ERROR;
    }

    if (listArgc != 2) {
        TclX_StringAppendObjResult (interp,
                                    "keyed list entry must be a two ",
                                    "element list, found \"", str, "\"",
                                    (char *) NULL);
        ckfree ((VOID*) listArgv);
        return TCL_ERROR;
    }
    if (listArgv [0][0] == '\0') {
        TclX_StringAppendObjResult (interp,
                                    "keyed list key may not be an ",
                                    "empty string", (char *) NULL);
        ckfree ((VOID*) listArgv);
        return TCL_ERROR;
    }

    entryPtr->key = ckstrdup (listArgv [0]);
    entryPtr->valuePtr = Tcl_NewStringObj (listArgv [1], -1);

    ckfree ((VOID*) listArgv);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * FreeKeyedListInternalRep --
 *   Free the internal representation of a keyed list.
 *
 * Parameters:
 *   o keylPtr - Keyed list object being deleted.
 *-----------------------------------------------------------------------------
 */
static void
FreeKeyedListInternalRep (keylPtr)
    Tcl_Obj *keylPtr;
{
    FreeKeyedListData ((keylIntObj_t *) keylPtr->internalRep.otherValuePtr);
}

/*-----------------------------------------------------------------------------
 * DupKeyedListInternalRep --
 *   Duplicate the internal representation of a keyed list.
 *
 * Parameters:
 *   o srcPtr - Keyed list object to copy.
 *   o copyPtr - Target object to copy internal representation to.
 *-----------------------------------------------------------------------------
 */
static void
DupKeyedListInternalRep (srcPtr, copyPtr)
    Tcl_Obj *srcPtr;
    Tcl_Obj *copyPtr;
{
    keylIntObj_t *srcIntPtr =
        (keylIntObj_t *) srcPtr->internalRep.otherValuePtr;
    keylIntObj_t *copyIntPtr;
    int idx;

    KEYL_OBJ_ASSERT (srcIntPtr);

    copyIntPtr = (keylIntObj_t *) ckalloc (sizeof (keylIntObj_t));
    copyIntPtr->arraySize = srcIntPtr->arraySize;
    copyIntPtr->numEntries = srcIntPtr->numEntries;
    copyIntPtr->entries = (keylEntry_t *)
        ckalloc (copyIntPtr->arraySize * sizeof (keylEntry_t));

    for (idx = 0; idx < srcIntPtr->numEntries ; idx++) {
        copyIntPtr->entries [idx].key = 
            ckstrdup (srcIntPtr->entries [idx].key);
        copyIntPtr->entries [idx].valuePtr = srcIntPtr->entries [idx].valuePtr;
        Tcl_IncrRefCount (copyIntPtr->entries [idx].valuePtr);
    }

    copyPtr->internalRep.otherValuePtr = (VOID *) copyIntPtr;
    copyPtr->typePtr = &keyedListType;

    KEYL_OBJ_ASSERT (copyIntPtr);
}

/*-----------------------------------------------------------------------------
 * SetKeyedListFromAny --
 *   Convert an object to a keyed list from its string representation.  Only
 * the first level is converted, as there is no way of knowing how far down
 * the keyed list recurses until lower levels are accessed.
 *
 * Parameters:
 *   o objPtr - Object to convert to a keyed list.
 *-----------------------------------------------------------------------------
 */
static int
SetKeyedListFromAny (interp, objPtr) 
    Tcl_Interp *interp;
    Tcl_Obj    *objPtr;
{
    keylIntObj_t *keylIntPtr;
    int idx, listArgc, strLen;
    char  **listArgv;

    if (Tcl_SplitList (interp, Tcl_GetStringFromObj (objPtr, &strLen),
                       &listArgc, &listArgv) != TCL_OK)
        return TCL_ERROR;

    keylIntPtr = AllocKeyedListIntRep ();
    EnsureKeyedListSpace (keylIntPtr, listArgc);

    for (idx = 0; idx < listArgc; idx++) {
        if (StringToKeyedListEntry (interp, listArgv [idx], 
                &(keylIntPtr->entries [keylIntPtr->numEntries])) != TCL_OK)
            goto errorExit;
        keylIntPtr->numEntries++;
    }
    ckfree ((VOID*) listArgv);
    if ((objPtr->typePtr != NULL) &&
        (objPtr->typePtr->freeIntRepProc != NULL)) {
        (*objPtr->typePtr->freeIntRepProc) (objPtr);
    }
    objPtr->internalRep.otherValuePtr = (VOID *) keylIntPtr;
    objPtr->typePtr = &keyedListType;

    KEYL_OBJ_ASSERT (keylIntPtr);
    return TCL_OK;
    
  errorExit:
    ckfree ((VOID*) listArgv);
    FreeKeyedListData (keylIntPtr);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * UpdateStringOfKeyedList --
 *    Update the string representation of a keyed list.
 *
 * Parameters:
 *   o objPtr - Object to convert to a keyed list.
 *-----------------------------------------------------------------------------
 */
static void
UpdateStringOfKeyedList (keylPtr)
    Tcl_Obj  *keylPtr;
{
#define UPDATE_STATIC_SIZE 32
    int idx, strLen;
    char **listArgv, *entryArgv [2];
    char *staticListArgv [UPDATE_STATIC_SIZE];
    keylIntObj_t *keylIntPtr =
        (keylIntObj_t *) keylPtr->internalRep.otherValuePtr;

    if (keylIntPtr->numEntries > UPDATE_STATIC_SIZE) {
        listArgv =
            (char **) ckalloc (keylIntPtr->numEntries * sizeof (char *));
    } else {
        listArgv = staticListArgv;
    }

    for (idx = 0; idx < keylIntPtr->numEntries; idx++) {
        entryArgv [0] = keylIntPtr->entries [idx].key;
        entryArgv [1] =
            Tcl_GetStringFromObj (keylIntPtr->entries [idx].valuePtr,
                                  &strLen);
        listArgv [idx] = Tcl_Merge (2, entryArgv);
    }

    keylPtr->bytes = Tcl_Merge (keylIntPtr->numEntries, listArgv);
    keylPtr->length = strlen (keylPtr->bytes);


    for (idx = 0; idx < keylIntPtr->numEntries; idx++) {
        ckfree (listArgv [idx]);
    }
    if (listArgv != staticListArgv)
        ckfree ((VOID*) listArgv);
}

/*-----------------------------------------------------------------------------
 * TclX_NewKeyedListObj --
 *   Create and initialize a new keyed list object.
 *
 * Returns:
 *    A pointer to the object.
 *-----------------------------------------------------------------------------
 */
Tcl_Obj *
TclX_NewKeyedListObj ()
{
    Tcl_Obj *keylPtr = Tcl_NewObj ();
    keylIntObj_t *keylIntPtr = AllocKeyedListIntRep ();

    keylPtr->internalRep.otherValuePtr = (VOID *) keylIntPtr;
    keylPtr->typePtr = &keyedListType;
    return keylPtr;
}

/*-----------------------------------------------------------------------------
 * TclX_KeyedListGet --
 *   Retrieve a key value from a keyed list.
 *
 * Parameters:
 *   o interp - Error message will be return in result if there is an error.
 *   o keylPtr - Keyed list object to get key from.
 *   o key - The name of the key to extract.  Will recusively process sub-keys
 *     seperated by `.'.
 *   o valueObjPtrPtr - If the key is found, a pointer to the key object
 *     is returned here.  NULL is returned if the key is not present.
 * Returns:
 *   o TCL_OK - If the key value was returned.
 *   o TCL_BREAK - If the key was not found.
 *   o TCL_ERROR - If an error occured.
 *-----------------------------------------------------------------------------
 */
int
TclX_KeyedListGet (interp, keylPtr, key, valuePtrPtr)
    Tcl_Interp *interp;
    Tcl_Obj    *keylPtr;
    char       *key;
    Tcl_Obj   **valuePtrPtr;
{
    keylIntObj_t *keylIntPtr;
    char *nextSubKey;
    int findIdx;

    if (key [0] == '\0') {
        TclX_StringAppendObjResult (interp, "empty key", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_ConvertToType (interp, keylPtr, &keyedListType) != TCL_OK)
        return TCL_ERROR;
    keylIntPtr = (keylIntObj_t *) keylPtr->internalRep.otherValuePtr;

    findIdx = FindKeyedListEntry (keylIntPtr, key, NULL, &nextSubKey);

    /*
     * If not found, return status.
     */
    if (findIdx < 0) {
        *valuePtrPtr = NULL;
        return TCL_BREAK;
    }

    /*
     * If we are at the last subkey, return the entry, otherwise recurse
     * down looking for the entry.
     */
    if (nextSubKey == NULL) {
        *valuePtrPtr = keylIntPtr->entries [findIdx].valuePtr;
        return TCL_OK;
    } else {
        return TclX_KeyedListGet (interp, 
                                       keylIntPtr->entries [findIdx].valuePtr,
                                       nextSubKey,
                                       valuePtrPtr);
    }
}

/*-----------------------------------------------------------------------------
 * TclX_KeyedListSet --
 *   Set a key value in keyed list object.
 *
 * Parameters:
 *   o interp - Error message will be return in result object.
 *   o keylPtr - Keyed list object to update.
 *   o key - The name of the key to extract.  Will recusively process
 *     sub-key seperated by `.'.
 *   o valueObjPtr - The value to set for the key.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_KeyedListSet (interp, keylPtr, key, valuePtr)
    Tcl_Interp *interp;
    Tcl_Obj    *keylPtr;
    char       *key;
    Tcl_Obj    *valuePtr;
{
    keylIntObj_t *keylIntPtr;
    char *nextSubKey;
    int findIdx, keyLen, status;
    Tcl_Obj *newKeylPtr;

    if (key [0] == '\0') {
        TclX_StringAppendObjResult (interp, "empty key", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_ConvertToType (interp, keylPtr, &keyedListType) != TCL_OK)
        return TCL_ERROR;
    keylIntPtr = (keylIntObj_t *) keylPtr->internalRep.otherValuePtr;

    findIdx = FindKeyedListEntry (keylIntPtr, key,
                                  &keyLen, &nextSubKey);

    /*
     * If we are at the last subkey, either update or add an entry.
     */
    if (nextSubKey == NULL) {
        if (findIdx < 0) {
            EnsureKeyedListSpace (keylIntPtr, 1);
            findIdx = keylIntPtr->numEntries;
            keylIntPtr->numEntries++;
        } else {
            ckfree (keylIntPtr->entries [findIdx].key);
            Tcl_DecrRefCount (keylIntPtr->entries [findIdx].valuePtr);
        }
        keylIntPtr->entries [findIdx].key =
            (char *) ckalloc (keyLen + 1);
        strncpy (keylIntPtr->entries [findIdx].key, key, keyLen);
        keylIntPtr->entries [findIdx].key [keyLen] = '\0';
        keylIntPtr->entries [findIdx].valuePtr = valuePtr;
        Tcl_IncrRefCount (valuePtr);
        TclX_InvalidateStringRep (keylPtr);

        KEYL_OBJ_ASSERT (keylIntPtr);
        return TCL_OK;
    }

    /*
     * If we are not at the last subkey, recurse down, creating new
     * entries if neccessary.  If this level key was not found, it
     * means we must build new subtree. Don't insert the new tree until we
     * come back without error.
     */
    if (findIdx >= 0) {
        DupSharedKeyListChild (keylIntPtr, findIdx);
        status =
            TclX_KeyedListSet (interp, 
                               keylIntPtr->entries [findIdx].valuePtr,
                               nextSubKey, valuePtr);
        if (status == TCL_OK) {
            TclX_InvalidateStringRep (keylPtr);
        }

        KEYL_OBJ_ASSERT (keylIntPtr);
        return status;
    } else {
        newKeylPtr = TclX_NewKeyedListObj ();
        if (TclX_KeyedListSet (interp, newKeylPtr,
                               nextSubKey, valuePtr) != TCL_OK) {
            Tcl_DecrRefCount (newKeylPtr);
            return TCL_ERROR;
        }
        EnsureKeyedListSpace (keylIntPtr, 1);
        findIdx = keylIntPtr->numEntries++;
        keylIntPtr->entries [findIdx].key =
            (char *) ckalloc (keyLen + 1);
        strncpy (keylIntPtr->entries [findIdx].key, key, keyLen);
        keylIntPtr->entries [findIdx].key [keyLen] = '\0';
        keylIntPtr->entries [findIdx].valuePtr = newKeylPtr;
        TclX_InvalidateStringRep (keylPtr);

        KEYL_OBJ_ASSERT (keylIntPtr);
        return TCL_OK;
    }
}

/*-----------------------------------------------------------------------------
 * TclX_KeyedListDelete --
 *   Delete a key value from keyed list.
 *
 * Parameters:
 *   o interp - Error message will be return in result if there is an error.
 *   o keylPtr - Keyed list object to update.
 *   o key - The name of the key to extract.  Will recusively process
 *     sub-key seperated by `.'.
 * Returns:
 *   o TCL_OK - If the key was deleted.
 *   o TCL_BREAK - If the key was not found.
 *   o TCL_ERROR - If an error occured.
 *-----------------------------------------------------------------------------
 */
int
TclX_KeyedListDelete (interp, keylPtr, key)
    Tcl_Interp *interp;
    Tcl_Obj    *keylPtr;
    char       *key;
{
    keylIntObj_t *keylIntPtr, *subKeylIntPtr;
    char *nextSubKey;
    int findIdx, status;

    if (key [0] == '\0') {
        TclX_StringAppendObjResult (interp, "empty key", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_ConvertToType (interp, keylPtr, &keyedListType) != TCL_OK)
        return TCL_ERROR;
    keylIntPtr = (keylIntObj_t *) keylPtr->internalRep.otherValuePtr;

    findIdx = FindKeyedListEntry (keylIntPtr, key, NULL, &nextSubKey);

    /*
     * If not found, return status.
     */
    if (findIdx < 0) {
        KEYL_OBJ_ASSERT (keylIntPtr);
        return TCL_BREAK;
    }

    /*
     * If we are at the last subkey, delete the entry.
     */
    if (nextSubKey == NULL) {
        DeleteKeyedListEntry (keylIntPtr, findIdx);
        TclX_InvalidateStringRep (keylPtr);

        KEYL_OBJ_ASSERT (keylIntPtr);
        return TCL_OK;
    }

    /*
     * If we are not at the last subkey, recurse down.  If the entry is
     * deleted and the sub-keyed list is empty, delete it as well.  Must
     * invalidate string, as it caches all representations below it.
     */
    DupSharedKeyListChild (keylIntPtr, findIdx);

    status = TclX_KeyedListDelete (interp,
                                   keylIntPtr->entries [findIdx].valuePtr,
                                   nextSubKey);
    if (status == TCL_OK) {
        subKeylIntPtr = (keylIntObj_t *)
            keylIntPtr->entries [findIdx].valuePtr->internalRep.otherValuePtr;
        if (subKeylIntPtr->numEntries == 0) {
            DeleteKeyedListEntry (keylIntPtr, findIdx);
        }
        TclX_InvalidateStringRep (keylPtr);
    }

    KEYL_OBJ_ASSERT (keylIntPtr);
    return status;
}

/*-----------------------------------------------------------------------------
 * TclX_KeyedListGetKeys --
 *   Retrieve a list of keyed list keys.
 *
 * Parameters:
 *   o interp - Error message will be return in result if there is an error.
 *   o keylPtr - Keyed list object to get key from.
 *   o key - The name of the key to get the sub keys for.  NULL or empty
 *     to retrieve all top level keys.
 *   o listObjPtrPtr - List object is returned here with key as values.
 * Returns:
 *   o TCL_OK - If the zero or more key where returned.
 *   o TCL_BREAK - If the key was not found.
 *   o TCL_ERROR - If an error occured.
 *-----------------------------------------------------------------------------
 */
int
TclX_KeyedListGetKeys (interp, keylPtr, key, listObjPtrPtr)
    Tcl_Interp *interp;
    Tcl_Obj    *keylPtr;
    char       *key;
    Tcl_Obj   **listObjPtrPtr;
{
    keylIntObj_t *keylIntPtr;
    Tcl_Obj *nameObjPtr, *listObjPtr;
    char *nextSubKey;
    int idx, findIdx;

    if (Tcl_ConvertToType (interp, keylPtr, &keyedListType) != TCL_OK)
        return TCL_ERROR;
    keylIntPtr = (keylIntObj_t *) keylPtr->internalRep.otherValuePtr;

    /*
     * If key is not NULL or empty, then recurse down until we go past
     * the end of all of the elements of the key.
     */
    if ((key != NULL) && (key [0] != '\0')) {
        findIdx = FindKeyedListEntry (keylIntPtr, key, NULL, &nextSubKey);
        if (findIdx < 0) {
            assert (keylIntPtr->arraySize >= keylIntPtr->numEntries);  /*FIX:*/
            return TCL_BREAK;
        }
        assert (keylIntPtr->arraySize >= keylIntPtr->numEntries);  /*FIX:*/
        return TclX_KeyedListGetKeys (interp, 
                                      keylIntPtr->entries [findIdx].valuePtr,
                                      nextSubKey,
                                      listObjPtrPtr);
    }

    /*
     * Reached the end of the full key, return all keys at this level.
     */
    listObjPtr = Tcl_NewListObj (0, NULL);
    for (idx = 0; idx < keylIntPtr->numEntries; idx++) {
        nameObjPtr = Tcl_NewStringObj (keylIntPtr->entries [idx].key,
                                       -1);
        if (Tcl_ListObjAppendElement (interp, listObjPtr,
                                      nameObjPtr) != TCL_OK) {
            Tcl_DecrRefCount (nameObjPtr);
            Tcl_DecrRefCount (listObjPtr);
            return TCL_ERROR;
        }
        Tcl_DecrRefCount (nameObjPtr);
    }
    *listObjPtrPtr = listObjPtr;
    assert (keylIntPtr->arraySize >= keylIntPtr->numEntries);  /*FIX:*/
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_KeylgetObjCmd --
 *     Implements the TCL keylget command:
 *         keylget listvar ?key? ?retvar | {}?
 *-----------------------------------------------------------------------------
 */
static int
TclX_KeylgetObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    **objv;
{
    Tcl_Obj *keylPtr, *valuePtr;
    int strLen, status;

    if ((objc < 2) || (objc > 4)) {
        return TclX_WrongArgs (interp, objv [0],
                               "listvar ?key? ?retvar | {}?");
    }

    /*
     * Handle request for list of keys, use keylkeys command.
     */
    if (objc == 2)
        return TclX_KeylkeysObjCmd (clientData, interp, objc, objv);

    keylPtr = Tcl_ObjGetVar2 (interp, objv [1], NULL, TCL_LEAVE_ERR_MSG);
    if (keylPtr == NULL) {
        return TCL_ERROR;
    }

    /*
     * Handle retrieving a value for a specified key.
     */
    status = TclX_KeyedListGet (interp, keylPtr,
                                Tcl_GetStringFromObj (objv [2], &strLen),
                                &valuePtr);
    if (status == TCL_ERROR)
        return TCL_ERROR;

    /*
     * Handle key not found.
     */
    if (status == TCL_BREAK) {
        if (objc == 3) {
            TclX_StringAppendObjResult (interp, "key \"", 
                                        Tcl_GetStringFromObj (objv [2],
                                                              &strLen),
                                        "\" not found in keyed list",
                                        (char *) NULL);
            return TCL_ERROR;
        } else {
            Tcl_SetIntObj (Tcl_GetObjResult (interp), FALSE);
            return TCL_OK;
        }
    }

    /*
     * No variable specified, so return value in the result.
     */
    if (objc == 3) {
        Tcl_SetObjResult (interp, valuePtr);
        return TCL_OK;
    }

    /*
     * Variable (or empty variable name) specified.
     */
    Tcl_GetStringFromObj (objv [3], &strLen);
    if (strLen != 0) {
        if (Tcl_ObjSetVar2 (interp, objv [3], NULL, valuePtr,
                            TCL_LEAVE_ERR_MSG) == NULL)
            return TCL_ERROR;
    }
    Tcl_SetIntObj (Tcl_GetObjResult (interp), TRUE);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_KeylsetObjCmd --
 *     Implements the TCL keylset command:
 *         keylset listvar key value ?key value...?
 *-----------------------------------------------------------------------------
 */
static int
TclX_KeylsetObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    **objv;
{
    Tcl_Obj *keylVarPtr, *keylPtr;
    int idx, strLen;

    if ((objc < 4) || ((objc % 2) != 0)) {
        return TclX_WrongArgs (interp, objv [0],
                               "listvar key value ?key value...?");
    }

    /*
     * Get the variable that we are going to update.  If the var doesn't exist,
     * create it.  If it is shared by more than being a variable, duplicated
     * it.
     */
    keylVarPtr = Tcl_ObjGetVar2 (interp, objv [1], NULL, 0);
    if (keylVarPtr == NULL) {
        keylPtr = TclX_NewKeyedListObj ();
    } else if (keylVarPtr->refCount > 1) {
        keylPtr = Tcl_DuplicateObj (keylVarPtr);
    } else {
        keylPtr = keylVarPtr;
        Tcl_IncrRefCount (keylPtr);
    }
    if (keylPtr != keylVarPtr) {
        if (Tcl_ObjSetVar2 (interp, objv [1], NULL, keylPtr,
                            TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;
    }

    for (idx = 2; idx < objc; idx += 2) {
        if (TclX_KeyedListSet (interp, keylPtr,
                               Tcl_GetStringFromObj (objv [idx], &strLen),
                               objv [idx+1]) != TCL_OK)
            goto errorExit;
    }

    Tcl_DecrRefCount (keylPtr);
    return TCL_OK;

  errorExit:
    Tcl_DecrRefCount (keylPtr);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_KeyldelObjCmd --
 *     Implements the TCL keyldel command:
 *         keyldel listvar key ?key ...?
 *----------------------------------------------------------------------------
 */
static int
TclX_KeyldelObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    **objv;
{
    Tcl_Obj *keylVarPtr, *keylPtr;
    char *key;
    int idx, strLen, status;

    if (objc < 3) {
        return TclX_WrongArgs (interp, objv [0], "listvar key ?key ...?");
    }

    /*
     * Get the variable that we are going to update.  If it is shared by more
     * than being a variable, duplicated it.
     */
    keylVarPtr = Tcl_ObjGetVar2 (interp, objv [1], NULL, TCL_LEAVE_ERR_MSG);
    if (keylVarPtr == NULL) {
        return TCL_ERROR;
    }
    if (keylVarPtr->refCount > 1) {
        keylPtr = Tcl_DuplicateObj (keylVarPtr);
        if (Tcl_ObjSetVar2 (interp, objv [1], NULL, keylPtr,
                            TCL_LEAVE_ERR_MSG) == NULL)
            goto errorExit;
    } else {
        keylPtr = keylVarPtr;
        Tcl_IncrRefCount (keylPtr);
    }

    for (idx = 2; idx < objc; idx++) {
        key = Tcl_GetStringFromObj (objv [idx], &strLen);

        status = TclX_KeyedListDelete (interp, keylPtr, key);
        switch (status) {
          case TCL_BREAK:
            TclX_StringAppendObjResult (interp, "key not found: \"",
                                        key, "\"", (char *) NULL);
            goto errorExit;
          case TCL_ERROR:
            goto errorExit;
        }
    }

    Tcl_DecrRefCount (keylPtr);
    return TCL_OK;

  errorExit:
    Tcl_DecrRefCount (keylPtr);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_KeylkeysObjCmd --
 *     Implements the TCL keylkeys command:
 *         keylkeys listvar ?key?
 *-----------------------------------------------------------------------------
 */
static int
TclX_KeylkeysObjCmd (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    **objv;
{
    Tcl_Obj *keylPtr, *listObjPtr;
    char *key;
    int strLen, status;

    if ((objc < 2) || (objc > 3)) {
        return TclX_WrongArgs (interp, objv [0], "listvar ?key?");
    }
    keylPtr = Tcl_ObjGetVar2 (interp, objv [1], NULL, TCL_LEAVE_ERR_MSG);
    if (keylPtr == NULL) {
        return TCL_ERROR;
    }

    /*
     * If key argument is not specified, then objv [2] is NULL or empty,
     * meaning get top level keys.
     */
    if (objc < 3) {
        key = NULL;
    } else {
        key = Tcl_GetStringFromObj (objv [2], &strLen);
    }
    status = TclX_KeyedListGetKeys (interp, keylPtr, key, &listObjPtr);

    switch (status) {
      case TCL_BREAK:
        TclX_StringAppendObjResult (interp, "key not found: \"",
                                    key, "\"", (char *) NULL);
        return TCL_ERROR;
      case TCL_ERROR:
        return TCL_ERROR;
    }

    Tcl_SetObjResult (interp, listObjPtr);
    Tcl_DecrRefCount (listObjPtr);

    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_KeyedListInit --
 *   Initialize the keyed list commands for this interpreter.
 *
 * Parameters:
 *   o interp - Interpreter to add commands to.
 *-----------------------------------------------------------------------------
 */
void
TclX_KeyedListInit (interp)
    Tcl_Interp *interp;
{
    Tcl_RegisterObjType (&keyedListType);

    Tcl_CreateObjCommand (interp, "keylget", -1, TclX_KeylgetObjCmd,
                          (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateObjCommand (interp, "keylset", -1, TclX_KeylsetObjCmd,
                          (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateObjCommand (interp, "keyldel", -1, TclX_KeyldelObjCmd,
                          (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateObjCommand (interp, "keylkeys", -1, TclX_KeylkeysObjCmd,
                          (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL);
}


