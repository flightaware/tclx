/* 
 * tclXstring.c --
 *
 *      Extended TCL string and character manipulation commands.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1995 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXstring.c,v 4.1 1994/08/11 03:49:41 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static unsigned int
ExpandString _ANSI_ARGS_((unsigned char *s,
                          unsigned char  buf[]));


/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CindexCmd --
 *     Implements the cindex TCL command:
 *         cindex string indexExpr
 *
 * Results:
 *      Returns the character indexed by  index  (zero  based)  from
 *      string. 
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_CindexCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    long index, len;

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                          " string indexExpr", (char *) NULL);
        return TCL_ERROR;
    }
    
    len = strlen (argv [1]);
    if (Tcl_RelativeExpr (interp, argv[2], len, &index) != TCL_OK)
        return TCL_ERROR;
    if (index >= len)
        return TCL_OK;

    interp->result [0] = argv[1][index];
    interp->result [1] = 0;
    return TCL_OK;

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ClengthCmd --
 *     Implements the clength TCL command:
 *         clength string
 *
 * Results:
 *      Returns the length of string in characters. 
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ClengthCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " string", 
                          (char *) NULL);
        return TCL_ERROR;
    }

    sprintf (interp->result, "%d", strlen (argv[1]));
    return TCL_OK;

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CrangeCmd --
 *     Implements the crange and csubstr TCL commands:
 *         crange string firstExpr lastExpr
 *         csubstr string firstExpr lengthExpr
 *
 * Results:
 *      Standard Tcl result.
 * Notes:
 *   If clientData is TRUE its the range command, if its FALSE its csubstr.
 *-----------------------------------------------------------------------------
 */
int
Tcl_CrangeCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    long      fullLen, first;
    long      subLen;
    char     *strPtr;
    char      holdChar;
    int       isRange = (int) clientData;

    if (argc != 4) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " string firstExpr ", 
                          (isRange) ? "lastExpr" : "lengthExpr",
                          (char *) NULL);
        return TCL_ERROR;
    }

    fullLen = strlen (argv [1]);

    if (Tcl_RelativeExpr (interp, argv[2], fullLen, &first) != TCL_OK)
        return TCL_ERROR;

    if (first >= fullLen)
        return TCL_OK;

    if (Tcl_RelativeExpr (interp, argv[3], fullLen, &subLen) != TCL_OK)
        return TCL_ERROR;
        
    if (isRange) {
        if (subLen < first) {
            Tcl_AppendResult (interp, "last is before first",
                              (char *) NULL);
            return TCL_ERROR;
        }
        subLen = subLen - first +1;
    }

    if (first + subLen > fullLen)
        subLen = fullLen - first;

    strPtr = argv [1] + first;

    holdChar = strPtr [subLen];
    strPtr [subLen] = '\0';
    Tcl_SetResult (interp, strPtr, TCL_VOLATILE);
    strPtr [subLen] = holdChar;

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_Ccollate Cmd --
 *     Implements the crange and csubstr TCL commands:
 *         ccollate [-local] string1 string2
 *
 * Results:
 *      Standard Tcl result.
 *-----------------------------------------------------------------------------
 */
int
Tcl_CcollateCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int argIndex, result, local = FALSE;

    if ((argc < 3) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " ?options? string1 string2", 
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (argc == 4) {
        if (!STREQU (argv [1], "-local")) {
            Tcl_AppendResult (interp, "Invalid option \"", argv [1],
                              "\", expected \"-local\"",
                              (char *) NULL);
            return TCL_ERROR;
        }
        local = TRUE;
    }
    argIndex = argc - 2;
    
    if (local) {
#if HAVE_STRCOLL
        result = strcoll (argv [argIndex], argv [argIndex + 1]);
#else
        result = strcmp (argv [argIndex], argv [argIndex + 1]);
#endif
    } else {
        result = strcmp (argv [argIndex], argv [argIndex + 1]);
    }

    if (result < 0) {
        interp->result = "-1";
    } else if (result == 0) {
        interp->result = "0";
    } else {
        interp->result = "1";
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_ReplicateCmd --
 *     Implements the replicate TCL command:
 *         replicate string countExpr
 *
 * Results:
 *      Returns string replicated count times.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_ReplicateCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    long           repCount;
    register char *srcPtr, *scanPtr, *newPtr;
    register long  newLen, cnt;

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " string countExpr", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_ExprLong (interp, argv[2], &repCount) != TCL_OK)
        return TCL_ERROR;

    srcPtr = argv [1];
    newLen = strlen (srcPtr) * repCount;
    if (newLen >= TCL_RESULT_SIZE)
        Tcl_SetResult (interp, ckalloc ((unsigned) newLen + 1), TCL_DYNAMIC);

    newPtr = interp->result;
    for (cnt = 0; cnt < repCount; cnt++) {
        for (scanPtr = srcPtr; *scanPtr != 0; scanPtr++)
            *newPtr++ = *scanPtr;
    }
    *newPtr = 0;

    return TCL_OK;

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CtokenCmd --
 *     Implements the clength TCL command:
 *         ctoken strvar separators
 *
 * Results:
 *      Returns the first token and removes it from the string variable.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_CtokenCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    Tcl_DString  string;
    char        *varValue, *startPtr;
    int          tokenLen;

    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                          " strvar separators", (char *) NULL);
        return TCL_ERROR;
    }
    
    varValue = Tcl_GetVar (interp, argv [1], TCL_LEAVE_ERR_MSG);
    if (varValue == NULL)
        return TCL_ERROR;

    Tcl_DStringInit (&string);
    Tcl_DStringAppend (&string, varValue, -1);

    startPtr = string.string + strspn (string.string, argv [2]);
    tokenLen = strcspn (startPtr, argv [2]);

    if (Tcl_SetVar (interp, argv [1], startPtr + tokenLen,
                    TCL_LEAVE_ERR_MSG) == NULL) {
        Tcl_DStringFree (&string);
        return TCL_ERROR;
    }
    startPtr [tokenLen] = '\0';
    Tcl_SetResult (interp, startPtr, TCL_VOLATILE);
    Tcl_DStringFree (&string);

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CexpandCmd --
 *     Implements the cexpand TCL command:
 *         cexpand string
 *
 * Results:
 *   Returns the character with backslash sequences expanded into actual
 * characters.
 *-----------------------------------------------------------------------------
 */
int
Tcl_CexpandCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    Tcl_DString    expanded;
    register char *scanPtr, *lastPtr;
    char           buf [1];
    int            count;

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                          " string", (char *) NULL);
        return TCL_ERROR;
    }

    Tcl_DStringInit (&expanded);
    scanPtr = lastPtr = argv [1];

    while (*scanPtr != '\0') {
        if (*scanPtr != '\\') {
            scanPtr++;
            continue;
        }
        /*
         * Found a backslash.
         */
        Tcl_DStringAppend (&expanded, lastPtr, scanPtr - lastPtr);
        buf [0] = Tcl_Backslash (scanPtr, &count);
        Tcl_DStringAppend (&expanded, buf, 1);
        scanPtr += count;
        lastPtr = scanPtr;
    }
    Tcl_DStringAppend (&expanded, lastPtr, scanPtr - lastPtr);
    
    Tcl_DStringResult (interp, &expanded);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CequalCmd --
 *     Implements the cexpand TCL command:
 *         cequal string1 string2
 *
 * Results:
 *   "0" or "1".
 *-----------------------------------------------------------------------------
 */
int
Tcl_CequalCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    if (argc != 3) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                          " string1 string2", (char *) NULL);
        return TCL_ERROR;
    }
    interp->result = (strcmp (argv [1], argv [2]) == 0) ? "1" : "0";
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ExpandString --
 *  Build an expand version of a translit range specification.
 *
 * Results:
 *  TRUE it the expansion is ok, FALSE it its too long.
 *
 *-----------------------------------------------------------------------------
 */
#define MAX_EXPANSION 255

static unsigned int
ExpandString (s, buf)
    unsigned char *s;
    unsigned char  buf[];
{
    int i, j;

    i = 0;
    while((*s !=0) && i < MAX_EXPANSION) {
        if(s[1] == '-' && s[2] > s[0]) {
            for(j = s[0]; j <= s[2]; j++)
                buf[i++] = j;
            s += 3;
        } else
            buf[i++] = *s++;
    }
    buf[i] = 0;
    return (i < MAX_EXPANSION);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_TranslitCmd --
 *     Implements the TCL translit command:
 *     translit inrange outrange string
 *
 * Results:
 *  Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_TranslitCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    unsigned char from [MAX_EXPANSION+1];
    unsigned char to   [MAX_EXPANSION+1];
    unsigned char map  [MAX_EXPANSION+1];
    unsigned char *s, *t;
    int idx;

    if (argc != 4) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " from to string", (char *) NULL);
        return TCL_ERROR;
    }

    if (!ExpandString ((unsigned char *) argv[1], from)) {
        interp->result = "inrange expansion too long";
        return TCL_ERROR;
    }

    if (!ExpandString ((unsigned char *) argv[2], to)) {
        interp->result = "outrange expansion too long";
        return TCL_ERROR;
    }

    for (idx = 0; idx <= MAX_EXPANSION; idx++)
        map [idx] = idx;

    for (idx = 0; to [idx] != '\0'; idx++) {
        if (from [idx] != '\0')
            map [from [idx]] = to [idx];
        else
            break;
    }
    if (to [idx] != '\0') {
        interp->result = "inrange longer than outrange";
        return TCL_ERROR;
    }

    for (; from [idx] != '\0'; idx++)
        map [from [idx]] = 0;

    for (s = t = (unsigned char *) argv[3]; *s != '\0'; s++) {
        if (map[*s] != '\0')
            *t++ = map [*s];
    }
    *t = '\0';

    Tcl_SetResult (interp, argv[3], TCL_VOLATILE);

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CtypeCmd --
 *
 *      This function implements the 'ctype' command:
 *      ctype ?-failindex? class string ?failIndexVar?
 *
 *      Where class is one of the following:
 *        digit, xdigit, lower, upper, alpha, alnum,
 *        space, cntrl,  punct, print, graph, ascii, char or ord.
 *
 * Results:
 *       One or zero: Depending if all the characters in the string are of
 *       the desired class.  Char and ord provide conversions and return the
 *       converted value.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_CtypeCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int             failIndex = FALSE;
    char           *failVar;
    register char  *class;
    char           *string;
    register char  *scanPtr;

    if (argc < 3)
	goto wrongNumArgs;

    if (argv [1][0] == '-') {
	if (STREQU (argv [1], "-failindex")) {
	    failIndex = TRUE;
	} else {
	    Tcl_AppendResult(interp, "invalid option \"", argv [1],
		"\", must be -failindex", (char *) NULL);
	    return TCL_ERROR;
	}
    }
    if (failIndex) {
        if (argc != 5) 
            goto wrongNumArgs;
        failVar = argv [2];
        class = argv [3];
        string = argv [4];
    } else {
        if (argc != 3) 
            goto wrongNumArgs;
        class = argv [1];
        string = argv [2];
    }
    scanPtr = string;

    /*
     * Handle conversion requests.
     */
    if (STREQU (class, "char")) {
        int number;

        if (failIndex) 
          goto failInvalid;
        if (Tcl_GetInt (interp, scanPtr, &number) != TCL_OK)
            return TCL_ERROR;
        if ((number < 0) || (number > 255)) {
            Tcl_AppendResult (interp, "number must be in the range 0..255",
                              (char *) NULL);
            return TCL_ERROR;
        }

        interp->result [0] = number;
        interp->result [1] = 0;
        return TCL_OK;
    }

    if (STREQU (class, "ord")) {
        int value;

        if (failIndex) 
          goto failInvalid;

        value = 0xff & scanPtr[0];  /* Prevent sign extension */
        sprintf (interp->result, "%u", value);
        return TCL_OK;
    }

    if (STREQU (class, "alnum")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!isalnum (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "alpha")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!isalpha (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "ascii")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!isascii (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "cntrl")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!iscntrl (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "digit")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!isdigit (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "graph")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!isgraph (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "lower")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!islower (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "print")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!isprint (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "punct")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!ispunct (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "space")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!isspace (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "upper")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!isupper (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    if (STREQU (class, "xdigit")) {
        for (; *scanPtr != 0; scanPtr++) {
            if (!isxdigit (UCHAR (*scanPtr)))
                break;
        }
        goto returnResult;
    }
    /*
     * No match on class.
     */
    Tcl_AppendResult (interp, "unrecognized class specification: \"", class,
                      "\", expected one of: alnum, alpha, ascii, char, ",
                      "cntrl, digit, graph, lower, ord, print, punct, space, ",
                      "upper or xdigit", (char *) NULL);
    return TCL_ERROR;

    /*
     * Return true or false, depending if the end was reached.  Always return 
     * false for a null string.  Optionally return the failed index if there
     * is no match.
     */
  returnResult:
    if ((*scanPtr == 0) && (scanPtr != string))
        interp->result = "1";
    else {
        /*
         * If the fail index was requested, set the variable here.
         */
        if (failIndex) {
            char indexStr [50];

            sprintf (indexStr, "%d", scanPtr - string);
            if (Tcl_SetVar(interp, failVar, indexStr,
                           TCL_LEAVE_ERR_MSG) == NULL)
                return TCL_ERROR;
        }
        interp->result = "0";
    }
    return TCL_OK;

  wrongNumArgs:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                      " ?-failindex var? class string",
                      (char *) NULL);
    return TCL_ERROR;
    
  failInvalid:
    Tcl_AppendResult (interp, "-failindex option is invalid for class \"",
                      class, "\"", (char *) NULL);
    return TCL_ERROR;
}
