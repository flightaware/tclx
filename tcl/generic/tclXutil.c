/*
 * tclXutil.c
 *
 * Utility functions for Extended Tcl.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1994 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXutil.c,v 4.2 1994/12/28 05:17:24 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

#ifndef _tolower
#  define _tolower tolower
#  define _toupper toupper
#endif

/*
 * Used to return argument messages by most commands.
 */
char *tclXWrongArgs = "wrong # args: ";


/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_StrToLong --
 *      Convert an Ascii string to an long number of the specified base.
 *
 * Parameters:
 *   o string (I) - String containing a number.
 *   o base (I) - The base to use for the number 8, 10 or 16 or zero to decide
 *     based on the leading characters of the number.  Zero to let the number
 *     determine the base.
 *   o longPtr (O) - Place to return the converted number.  Will be 
 *     unchanged if there is an error.
 *
 * Returns:
 *      Returns 1 if the string was a valid number, 0 invalid.
 *-----------------------------------------------------------------------------
 */
int
Tcl_StrToLong (string, base, longPtr)
    CONST char *string;
    int         base;
    long       *longPtr;
{
    char *end;
    long  num;

    num = strtol(string, &end, base);
    while ((*end != '\0') && ISSPACE(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        return FALSE;
    *longPtr = num;
    return TRUE;

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_StrToInt --
 *      Convert an Ascii string to an number of the specified base.
 *
 * Parameters:
 *   o string (I) - String containing a number.
 *   o base (I) - The base to use for the number 8, 10 or 16 or zero to decide
 *     based on the leading characters of the number.  Zero to let the number
 *     determine the base.
 *   o intPtr (O) - Place to return the converted number.  Will be 
 *     unchanged if there is an error.
 *
 * Returns:
 *      Returns 1 if the string was a valid number, 0 invalid.
 *-----------------------------------------------------------------------------
 */
int
Tcl_StrToInt (string, base, intPtr)
    CONST char *string;
    int         base;
    int        *intPtr;
{
    char *end;
    int   num;

    num = strtol(string, &end, base);
    while ((*end != '\0') && ISSPACE(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        return FALSE;
    *intPtr = num;
    return TRUE;

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_StrToUnsigned --
 *      Convert an Ascii string to an unsigned int of the specified base.
 *
 * Parameters:
 *   o string (I) - String containing a number.
 *   o base (I) - The base to use for the number 8, 10 or 16 or zero to decide
 *     based on the leading characters of the number.  Zero to let the number
 *     determine the base.
 *   o unsignedPtr (O) - Place to return the converted number.  Will be 
 *     unchanged if there is an error.
 *
 * Returns:
 *      Returns 1 if the string was a valid number, 0 invalid.
 *-----------------------------------------------------------------------------
 */
int
Tcl_StrToUnsigned (string, base, unsignedPtr)
    CONST char *string;
    int         base;
    unsigned   *unsignedPtr;
{
    char          *end;
    unsigned long  num;

    num = strtoul (string, &end, base);
    while ((*end != '\0') && ISSPACE(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        return FALSE;
    *unsignedPtr = num;
    return TRUE;

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_StrToDouble --
 *   Convert a string to a double percision floating point number.
 *
 * Parameters:
 *   string (I) - Buffer containing double value to convert.
 *   doublePtr (O) - The convert floating point number.
 * Returns:
 *   TRUE if the number is ok, FALSE if it is illegal.
 *-----------------------------------------------------------------------------
 */
int
Tcl_StrToDouble (string, doublePtr)
    CONST char *string;
    double     *doublePtr;
{
    char   *end;
    double  num;

    num = strtod (string, &end);
    while ((*end != '\0') && ISSPACE(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        return FALSE;

    *doublePtr = num;
    return TRUE;

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_StrToOffset --
 *      Convert an Ascii string to an off_t number of the specified base.
 *
 * Parameters:
 *   o string (I) - String containing a number.
 *   o base (I) - The base to use for the number 8, 10 or 16 or zero to decide
 *     based on the leading characters of the number.  Zero to let the number
 *     determine the base.
 *   o offsetPtr (O) - Place to return the converted number.  Will be 
 *     unchanged if there is an error.
 *
 * Returns:
 *      Returns 1 if the string was a valid number, 0 invalid.
 *-----------------------------------------------------------------------------
 */
int
Tcl_StrToOffset (string, base, offsetPtr)
    CONST char *string;
    int         base;
    off_t      *offsetPtr;
{
    char *end;
    long  num;
    off_t offset;

    num = strtol(string, &end, base);
    while ((*end != '\0') && ISSPACE(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        return FALSE;

    offset = (off_t) num;
    if (offset != num)
        return FALSE;

    *offsetPtr = offset;
    return TRUE;

}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_DownShift --
 *     Utility procedure to down-shift a string.  It is written in such
 *     a way as that the target string maybe the same as the source string.
 *
 * Parameters:
 *   o targetStr (I) - String to store the down-shifted string in.  Must
 *     have enough space allocated to store the string.  If NULL is specified,
 *     then the string will be dynamicly allocated and returned as the
 *     result of the function. May also be the same as the source string to
 *     shift in place.
 *   o sourceStr (I) - The string to down-shift.
 *
 * Returns:
 *   A pointer to the down-shifted string
 *-----------------------------------------------------------------------------
 */
char *
Tcl_DownShift (targetStr, sourceStr)
    char       *targetStr;
    CONST char *sourceStr;
{
    register char theChar;

    if (targetStr == NULL)
        targetStr = ckalloc (strlen ((char *) sourceStr) + 1);

    for (; (theChar = *sourceStr) != '\0'; sourceStr++) {
        if (isupper (theChar))
            theChar = _tolower (theChar);
        *targetStr++ = theChar;
    }
    *targetStr = '\0';
    return targetStr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_UpShift --
 *     Utility procedure to up-shift a string.
 *
 * Parameters:
 *   o targetStr (I) - String to store the up-shifted string in.  Must
 *     have enough space allocated to store the string.  If NULL is specified,
 *     then the string will be dynamicly allocated and returned as the
 *     result of the function. May also be the same as the source string to
 *     shift in place.
 *   o sourceStr (I) - The string to up-shift.
 *
 * Returns:
 *   A pointer to the up-shifted string
 *-----------------------------------------------------------------------------
 */
char *
Tcl_UpShift (targetStr, sourceStr)
    char       *targetStr;
    CONST char *sourceStr;
{
    register char theChar;

    if (targetStr == NULL)
        targetStr = ckalloc (strlen ((char *) sourceStr) + 1);

    for (; (theChar = *sourceStr) != '\0'; sourceStr++) {
        if (ISLOWER (theChar))
            theChar = _toupper (theChar);
        *targetStr++ = theChar;
    }
    *targetStr = '\0';
    return targetStr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_DStringGets --
 *
 *    Reads a line from a file into a dynamic string.  The string will be
 * expanded, if necessary and reads are done until EOL or EOF is reached.
 * The line is appended to any data already in the string.
 *
 * Parameter
 *   o filePtr (I) - File to read from.
 *   o dynStrPtr (I) - String to return the data in.
 * Returns:
 *    o TCL_BREAK - EOF
 *    o TCL_OK - If data was transfered.
 *    o TCL_ERROR - An error occured.
 *-----------------------------------------------------------------------------
 */
int
Tcl_DStringGets (filePtr, dynStrPtr)
    FILE         *filePtr;
    Tcl_DString  *dynStrPtr;
{
    char           buffer [128];
    register char *bufPtr, *bufEnd;
    register int   readVal;
    int            startLength = dynStrPtr->length;

    bufPtr = buffer;
    bufEnd = buffer + sizeof (buffer) - 1;

    clearerr (filePtr);  /* Clear previous error/EOF */

    while (TRUE) {
        readVal = getc (filePtr);
        if (readVal == '\n')      /* Is it a new-line? */
            break;
        if (readVal == EOF)
            break;
        *bufPtr++ = readVal;
        if (bufPtr > bufEnd) {
            Tcl_DStringAppend (dynStrPtr, buffer, sizeof (buffer));
            bufPtr = buffer;
        }
    }
    if ((readVal == EOF) && ferror (filePtr))
        return TCL_ERROR;   /* Error */

    if (bufPtr != buffer) {
        Tcl_DStringAppend (dynStrPtr, buffer, bufPtr - buffer);
    }

    if ((readVal == EOF) && dynStrPtr->length == startLength)
        return TCL_BREAK;
    else
        return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_GetLong --
 *
 *      Given a string, produce the corresponding long value.
 *
 * Results:
 *      The return value is normally TCL_OK;  in this case *longPtr
 *      will be set to the integer value equivalent to string.  If
 *      string is improperly formed then TCL_ERROR is returned and
 *      an error message will be left in interp->result.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetLong(interp, string, longPtr)
    Tcl_Interp *interp;
    CONST char *string;
    long       *longPtr;
{
    char *end;
    long  i;

    i = strtol(string, &end, 0);
    while ((*end != '\0') && ISSPACE(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0)) {
        Tcl_AppendResult (interp, "expected integer but got \"", string,
                          "\"", (char *) NULL);
        return TCL_ERROR;
    }
    *longPtr = i;
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_GetUnsigned --
 *
 *      Given a string, produce the corresponding unsigned integer value.
 *
 * Results:
 *      The return value is normally TCL_OK;  in this case *unsignedPtr
 *      will be set to the integer value equivalent to string.  If
 *      string is improperly formed then TCL_ERROR is returned and
 *      an error message will be left in interp->result.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetUnsigned(interp, string, unsignedPtr)
    Tcl_Interp *interp;
    CONST char *string;
    unsigned   *unsignedPtr;
{
    char          *end;
    unsigned long  i;

    /*
     * Since some strtoul functions don't detect negative numbers, check
     * in advance.
     */
    while (ISSPACE(*string))
        string++;
    if (string [0] == '-')
        goto badUnsigned;

    i = strtoul(string, &end, 0);
    while ((*end != '\0') && ISSPACE(*end))
        end++;

    if ((end == string) || (*end != '\0'))
        goto badUnsigned;

    *unsignedPtr = i;
    return TCL_OK;

  badUnsigned:
    Tcl_AppendResult (interp, "expected unsigned integer but got \"", 
                      string, "\"", (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_GetTime --
 *
 *      Given a string, produce the corresponding time_t value.
 *
 * Results:
 *      The return value is normally TCL_OK;  in this case *timepPtr
 *      will be set to the integer value equivalent to string.  If
 *      string is improperly formed then TCL_ERROR is returned and
 *      an error message will be left in interp->result.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetTime(interp, string, timePtr)
    Tcl_Interp *interp;
    CONST char *string;
    time_t     *timePtr;
{
    char   *end;
    long   i;
    time_t time;

    i = strtoul(string, &end, 0);
    while ((*end != '\0') && ISSPACE(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        goto badTime;

    time = (time_t) i;
    if (time != i)
        goto badTime;

    *timePtr = time;
    return TCL_OK;

  badTime:
    Tcl_AppendResult (interp, "expected unsigned time but got \"", 
                      string, "\"", (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_GetOffset --
 *
 *      Given a string, produce the corresponding off_t value.
 *
 * Results:
 *      The return value is normally TCL_OK;  in this case *timepPtr
 *      will be set to the integer value equivalent to string.  If
 *      string is improperly formed then TCL_ERROR is returned and
 *      an error message will be left in interp->result.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetOffset(interp, string, offsetPtr)
    Tcl_Interp *interp;
    CONST char *string;
    off_t      *offsetPtr;
{
    char   *end;
    long   i;
    off_t offset;

    i = strtol(string, &end, 0);
    while ((*end != '\0') && ISSPACE(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        goto badOffset;

    offset = (off_t) i;
    if (offset != i)
        goto badOffset;

    *offsetPtr = offset;
    return TCL_OK;

  badOffset:
    Tcl_AppendResult (interp, "expected integer offset but got \"", 
                      string, "\"", (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_RelativeExpr --
 *
 *    Evaluate an expression that may start with the magic words "end" or
 * "len".  These strings are replaced with either the end offset or the
 * length that is passed in.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter.
 *   o cstringExpr (I) - The expression to evaludate.
 *   o stringLen (I) - The length of the string.
 *   o exprResultPtr (O) - The result of the expression is returned here.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
Tcl_RelativeExpr (interp, cstringExpr, stringLen, exprResultPtr)
    Tcl_Interp  *interp;
    char        *cstringExpr;
    long         stringLen;
    long        *exprResultPtr;
{
    
    char *buf;
    int   exprLen, result;
    char  staticBuf [64];

    if (!(STRNEQU (cstringExpr, "end", 3) ||
          STRNEQU (cstringExpr, "len", 3))) {
        return Tcl_ExprLong (interp, cstringExpr, exprResultPtr);
    }

    sprintf (staticBuf, "%ld",
             stringLen - ((cstringExpr [0] == 'e') ? 1 : 0));
    exprLen = strlen (staticBuf) + strlen (cstringExpr) - 2;

    buf = staticBuf;
    if (exprLen > sizeof (staticBuf)) {
        buf = (char *) ckalloc (exprLen);
        strcpy (buf, staticBuf);
    }
    strcat (buf, cstringExpr + 3);

    result = Tcl_ExprLong (interp, buf, exprResultPtr);

    if (buf != staticBuf)
        ckfree (buf);
    return result;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_GetOpenFileStruct --
 *
 *    Convert a file handle to a pointer to the internal Tcl file structure.
 *
 * Parameters:
 *   o interp (I) - Current interpreter.
 *   o handle (I) - The file handle to convert.
 * Returns:
 *   A pointer to the open file structure for the file, or NULL if an error
 * occured.
 *-----------------------------------------------------------------------------
 */
OpenFile *
Tcl_GetOpenFileStruct (interp, handle)
    Tcl_Interp *interp;
    char       *handle;
{
    FILE   *filePtr;

    if (Tcl_GetOpenFile (interp, handle,
                         FALSE, FALSE,  /* No checking */
                         &filePtr) != TCL_OK)
        return NULL;

    return tclOpenFiles [fileno (filePtr)];
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_SetupFileEntry --
 *
 * Set up an entry in the Tcl file table for a file number, including the stdio
 * FILE structure.
 *
 * Parameters:
 *   o interp (I) - Current interpreter.
 *   o fileNum (I) - File number to set up the entry for.
 *   o permissions (I) - Flags consisting of TCL_FILE_READABLE,
 *     TCL_FILE_WRITABLE.
 * Returns:
 *   A pointer to the FILE structure for the file, or NULL if an error
 * occured.
 *-----------------------------------------------------------------------------
 */
FILE *
Tcl_SetupFileEntry (interp, fileNum, permissions)
    Tcl_Interp *interp;
    int         fileNum;
    int         permissions;
{
    Interp   *iPtr = (Interp *) interp;
    char     *mode;
    FILE     *filePtr;

    /*
     * Set up a stdio FILE control block for the new file.
     */
    if (permissions & TCL_FILE_WRITABLE) {
        if (permissions & TCL_FILE_READABLE)
            mode = "r+";
        else
            mode = "w";
    } else {
        mode = "r";
    }

    filePtr = fdopen (fileNum, mode);
    if (filePtr == NULL) {
        iPtr->result = Tcl_PosixError (interp);
        return NULL;
    }
    
    Tcl_EnterFile (interp, filePtr, permissions);

    return filePtr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_CloseForError --
 *
 *   Close a file number on error.  If the file is in the Tcl file table, clean
 * it up too. The variable errno, and interp->result and the errorCode variable
 * will be saved and not lost.
 *
 * Parameters:
 *   o interp (I) - Current interpreter.
 *   o fileNum (I) - File number to close.
 *-----------------------------------------------------------------------------
 */
void
Tcl_CloseForError (interp, fileNum)
    Tcl_Interp *interp;
    int         fileNum;
{
    static char *ERROR_CODE = "errorCode";
    int          saveErrNo = errno;
    Interp      *iPtr = (Interp *) interp;
    char        *saveResult, *errorCode, *saveErrorCode, *argv [2], buf [32];

    saveResult = ckstrdup (interp->result);

    if (iPtr->flags & ERROR_CODE_SET) {
        errorCode = Tcl_GetVar (interp, ERROR_CODE, TCL_GLOBAL_ONLY);
        saveErrorCode = ckstrdup (errorCode);
    } else {
        saveErrorCode = NULL;
    }

    sprintf (buf, "file%d", fileNum);

    argv [0] = "close";
    argv [1] = buf;
    Tcl_CloseCmd (NULL, interp, 2, argv);
    Tcl_ResetResult (interp);

    if (saveErrorCode != NULL) {
        Tcl_SetVar (interp, ERROR_CODE, saveErrorCode, TCL_GLOBAL_ONLY);
        iPtr->flags |= ERROR_CODE_SET;
        free (saveErrorCode);
    }
    Tcl_SetResult (interp, saveResult, TCL_DYNAMIC);

    close (fileNum);  /* In case Tcl didn't have it open */
    
    errno = saveErrNo;
}
     

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_TicksToMS --
 *
 *   Convert clock ticks to milliseconds.
 *
 * Parameters:
 *   o numTicks (I) - Number of ticks.
 * Returns:
 *   Milliseconds.
 *-----------------------------------------------------------------------------
 */
clock_t
Tcl_TicksToMS (numTicks)
     clock_t numTicks;
{
    static clock_t msPerTick = 0;

    /*
     * Some systems (SVR4) implement CLK_TCK as a call to sysconf, so lets only
     * reference it once in the life of this process.
     */
    if (msPerTick == 0)
        msPerTick = CLK_TCK;

    if (msPerTick <= 100) {
        /*
         * On low resolution systems we can do this all with integer math. Note
         * that the addition of half the clock hertz results in appoximate
         * rounding instead of truncation.
         */
        return (numTicks) * (1000 + msPerTick / 2) / msPerTick;
    } else {
        /*
         * On systems (Cray) where the question is ticks per millisecond, not
         * milliseconds per tick, we need to use floating point arithmetic.
         */
        return ((numTicks) * 1000.0 / msPerTick);
    }
}
