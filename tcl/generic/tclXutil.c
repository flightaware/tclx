/*
 * tclXutil.c
 *
 * Utility functions for Extended Tcl.
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
 * $Id: tclXutil.c,v 5.10 1996/03/13 08:30:21 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

#ifndef _tolower
#  define _tolower tolower
#  define _toupper toupper
#endif

/*
 * Prototypes of internal functions.
 */
static int
ReturnOverflow _ANSI_ARGS_((Tcl_Interp *interp));

static int
CallEvalErrorHandler _ANSI_ARGS_((Tcl_Interp  *interp));

static Tcl_ChannelType *
UnixFileChannelType _ANSI_ARGS_((Tcl_Interp *interp));

/*
 * Used to return argument messages by most commands.
 */
char *tclXWrongArgs = "wrong # args: ";

/*-----------------------------------------------------------------------------
 * ReturnOverflow --
 *    
 *   Return an error message about an numeric overflow.
 *
 * Parameters:
 *   o interp (O) - Interpreter to set the error message in.
 * Returns:
 *   TCL_ERROR;
 *-----------------------------------------------------------------------------
 */
static int
ReturnOverflow (interp)
    Tcl_Interp *interp;
{
    interp->result = "integer value too large to represent";
    Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW",
                     interp->result, (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
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
    char *end, *p;
    long  i;

    /*
     * Note: use strtoul instead of strtol for integer conversions
     * to allow full-size unsigned numbers, but don't depend on strtoul
     * to handle sign characters;  it won't in some implementations.
     */

    errno = 0;
    for (p = (char *) string; isspace(UCHAR(*p)); p++) {
        /* Empty loop body. */
    }
    if (*p == '-') {
        p++;
        i = -strtoul(p, &end, base);
    } else if (*p == '+') {
        p++;
        i = strtoul(p, &end, base);
    } else {
        i = strtoul(p, &end, base);
    }
    if (end == p) {
        return FALSE;
    }
    if (errno == ERANGE) {
        return FALSE;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
        end++;
    }
    if (*end != '\0') {
        return FALSE;
    }
    *longPtr = i;
    return TRUE;
}

/*-----------------------------------------------------------------------------
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
    char *end, *p;
    int   i;

    /*
     * Note: use strtoul instead of strtol for integer conversions
     * to allow full-size unsigned numbers, but don't depend on strtoul
     * to handle sign characters;  it won't in some implementations.
     */

    errno = 0;
    for (p = (char *) string; isspace(UCHAR(*p)); p++) {
        /* Empty loop body. */
    }
    if (*p == '-') {
        p++;
        i = -strtoul(p, &end, base);
    } else if (*p == '+') {
        p++;
        i = strtoul(p, &end, base);
    } else {
        i = strtoul(p, &end, base);
    }
    if (end == p) {
        return FALSE;
    }
    if (errno == ERANGE) {
        return FALSE;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
        end++;
    }
    if (*end != '\0') {
        return FALSE;
    }
    *intPtr = i;
    return TRUE;
}

/*-----------------------------------------------------------------------------
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
    char *end, *p;
    unsigned i;

    errno = 0;
    for (p = (char *) string; isspace(UCHAR(*p)); p++) {
        /* Empty loop body. */
    }
    i = strtoul(p, &end, base);
    if (end == p) {
        return FALSE;
    }
    if (errno == ERANGE) {
        return FALSE;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
        end++;
    }
    if (*end != '\0') {
        return FALSE;
    }
    *unsignedPtr = i;
    return TRUE;
}

/*-----------------------------------------------------------------------------
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
    char   *end, *p;
    double  i;

    errno = 0;
    for (p = (char *) string; isspace(UCHAR(*p)); p++) {
        /* Empty loop body. */
    }
    i = strtod (string, &end);
    if (end == p) {
        return FALSE;
    }
    *doublePtr = i;
    return TRUE;
}

/*-----------------------------------------------------------------------------
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
    char *end, *p;
    off_t i;

    /*
     * Note: use strtoul instead of strtol for integer conversions
     * to allow full-size unsigned numbers, but don't depend on strtoul
     * to handle sign characters;  it won't in some implementations.
     */

    errno = 0;
    for (p = (char *) string; isspace(UCHAR(*p)); p++) {
        /* Empty loop body. */
    }
    if (*p == '-') {
        p++;
        i = -strtoul(p, &end, base);
    } else if (*p == '+') {
        p++;
        i = strtoul(p, &end, base);
    } else {
        i = strtoul(p, &end, base);
    }
    if (end == p) {
        return FALSE;
    }
    if (errno == ERANGE) {
        return FALSE;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
        end++;
    }
    if (*end != '\0') {
        return FALSE;
    }
    *offsetPtr = i;
    return TRUE;
}

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
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
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetLong(interp, string, longPtr)
    Tcl_Interp *interp;
    CONST char *string;
    long       *longPtr;
{
    char *end, *p;
    long i;

    errno = 0;
    for (p = (char *) string; isspace(UCHAR(*p)); p++) {
        /* Empty loop body. */
    }
    i = strtol(p, &end, 0);
    if (end == p) {
        badInteger:
        Tcl_AppendResult(interp, "expected integer but got \"", string,
                "\"", (char *) NULL);
        return TCL_ERROR;
    }
    if (errno == ERANGE) {
        return ReturnOverflow (interp);
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
        end++;
    }
    if (*end != '\0') {
        goto badInteger;
    }
    *longPtr = i;
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
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
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetUnsigned(interp, string, unsignedPtr)
    Tcl_Interp *interp;
    CONST char *string;
    unsigned   *unsignedPtr;
{
    char          *end, *p;
    unsigned long  i;

    /*
     * Since some strtoul functions don't detect negative numbers, check
     * in advance.
     */
    errno = 0;
    for (p = (char *) string; isspace(UCHAR(*p)); p++) {
        /* Empty loop body. */
    }
    if (*p == '-')
        goto badUnsigned;
    if (*p == '+') {
        p++;
    }
    i = strtoul(p, &end, 0);
    if (end == p) {
        goto badUnsigned;
    }
    if (errno == ERANGE) {
        return ReturnOverflow (interp);
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
        end++;
    }
    if (*end != '\0') {
        goto badUnsigned;
    }

    *unsignedPtr = i;
    return TCL_OK;

  badUnsigned:
    Tcl_AppendResult (interp, "expected unsigned integer but got \"", 
                      string, "\"", (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
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
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetOffset(interp, string, offsetPtr)
    Tcl_Interp *interp;
    CONST char *string;
    off_t      *offsetPtr;
{
    char *end, *p;
    long i;

    errno = 0;
    for (p = (char *) string; isspace(UCHAR(*p)); p++) {
        /* Empty loop body. */
    }
    i = strtol(p, &end, 0);
    if (end == p) {
        goto badOffset;
    }
    if (errno == ERANGE) {
        return ReturnOverflow (interp);
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
        end++;
    }
    if (*end != '\0') {
        goto badOffset;
    }
    *offsetPtr = (off_t) i;
    if (*offsetPtr != i)
        goto badOffset;
    return TCL_OK;

  badOffset:
    Tcl_AppendResult (interp, "expected integer offset but got \"", 
                      string, "\"", (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
 * TclX_GetOpenChannel --
 *
 *    Convert a file handle to a channel with error checking.
 *
 * Parameters:
 *   o interp (I) - Current interpreter.
 *   o handle (I) - The file handle to convert.
 *   o accessMode (I) - Set of TCL_READABLE or TCL_WRITABLE or zero to
 *     not do error checking.
 * Returns:
 *   A the channel or NULL if an error occured.
 *-----------------------------------------------------------------------------
 */
Tcl_Channel
TclX_GetOpenChannel (interp, handle, accessMode)
    Tcl_Interp *interp;
    char       *handle;
    int         accessMode;
{
    Tcl_Channel chan;
    int mode;

    chan = Tcl_GetChannel (interp, handle, &mode);
    if (chan == (Tcl_Channel) NULL) {
	return NULL;
    }
    if ((accessMode & TCL_READABLE) && ((mode & TCL_READABLE) == 0)) {
        Tcl_AppendResult(interp, "channel \"", handle,
                "\" wasn't opened for reading", (char *) NULL);
        return NULL;
    }
    if ((accessMode & TCL_WRITABLE) && ((mode & TCL_WRITABLE) == 0)) {
        Tcl_AppendResult(interp, "channel \"", handle,
                "\" wasn't opened for writing", (char *) NULL);
        return NULL;
    }

    return chan;
}

/*-----------------------------------------------------------------------------
 * TclX_GetOpenFnum --
 *
 *    Convert a file handle to a file number with error checking.
 *
 * Parameters:
 *   o interp (I) - Current interpreter.
 *   o handle (I) - The file handle to convert.
 *   o accessMode (I) - Set of TCL_READABLE or TCL_WRITABLE or zero to
 *     not do error checking.
 * Returns:
 *   The file number of < 0 if an error occured.
 *-----------------------------------------------------------------------------
 */
int
TclX_GetOpenFnum (interp, handle, accessMode)
    Tcl_Interp *interp;
    char       *handle;
    int         accessMode;
{
    Tcl_Channel channel;
    Tcl_File file;

    channel = TclX_GetOpenChannel (interp, handle, accessMode);
    if (channel == NULL)
        return -1;

    switch (accessMode) {
      case 0:
      case TCL_READABLE | TCL_WRITABLE:
        file = Tcl_GetChannelFile (channel, TCL_READABLE);
        if (file == NULL)
            file = Tcl_GetChannelFile (channel, TCL_WRITABLE);
        break;
      case TCL_READABLE:
        file = Tcl_GetChannelFile (channel, TCL_READABLE);
        break;
      case TCL_WRITABLE:
        file = Tcl_GetChannelFile (channel, TCL_WRITABLE);
        break;
    }
    if (file == NULL)
        panic ("TclX_GetOpenFnum: file not available");

    return (int) Tcl_GetFileInfo (file, NULL);
}

/*-----------------------------------------------------------------------------
 * TclX_ChannelFnum --
 *
 *    Convert a channel to a file number.
 *
 * Parameters:
 *   o channel (I) - Channel to get file number for.
 *   o direction (I) - TCL_READABLE or TCL_WRITABLE, or zero.  If zero, then
 *     return the first of the read and write numbers.
 * Returns:
 *   The file number or -1 if a file number is not associated with this access
 * direction.
 *-----------------------------------------------------------------------------
 */
int
TclX_ChannelFnum (channel, direction)
    Tcl_Channel channel;
    int         direction;
{
    Tcl_File file;

    if (direction == 0) {
        file = Tcl_GetChannelFile (channel, TCL_READABLE);
        if (file == NULL)
            file = Tcl_GetChannelFile (channel, TCL_WRITABLE);
    } else {
        file = Tcl_GetChannelFile (channel, direction);
        if (file == NULL)
            return -1;
    }
    return (int) Tcl_GetFileInfo (file, NULL);
}

/*-----------------------------------------------------------------------------
 * TclX_SetupFileEntry --
 *
 * Set up an entry in the Tcl file table for a file number, including the stdio
 * FILE structure.
 *
 * Parameters:
 *   o interp (I) - Current interpreter.  The file handle is NOT returned
 *     in result, it is cleared.  This is done because the file handles
 *     are often returned in different ways.
 *   o fileNum (I) - File number to set up the entry for.
 *   o mode (I) - Flags consisting of TCL_READABLE, TCL_WRITABLE.
 *   o isSocket (I) - TRUE if its a socket, FALSE otherwise.
 * Returns:
 *   The channel.  No error can occur.
 *-----------------------------------------------------------------------------
 */
Tcl_Channel
TclX_SetupFileEntry (interp, fileNum, mode, isSocket)
    Tcl_Interp *interp;
    int         fileNum;
    int         mode;
    int         isSocket;
{
    Tcl_Channel channel;

    if (isSocket) {
        channel = Tcl_MakeTcpClientChannel ((ClientData) fileNum);
    } else {
        channel = Tcl_MakeFileChannel ((ClientData) fileNum,
                                       (ClientData) fileNum,
                                       mode);
    }
        
    Tcl_RegisterChannel (interp, channel);
    return channel;
}

/*-----------------------------------------------------------------------------
 * Tcl_CloseForError --
 *
 *   Close a file on error.  If the file is associated with a channel, close
 * it too. The error number will be saved and not lost.
 *
 * Parameters:
 *   o interp (I) - Current interpreter.
 *   o channel (I) - Channel to close if not NULL.
 *   o fileNum (I) - File number to close if >= 0.
 *-----------------------------------------------------------------------------
 */
void
Tcl_CloseForError (interp, channel, fileNum)
    Tcl_Interp *interp;
    Tcl_Channel channel;
    int         fileNum;
{
    int saveErrNo = Tcl_GetErrno ();

    /*
     * Always close fileNum, even if channel close is done, as it doesn't
     * close stdin, stdout or stderr numbers.
     */
    if (channel != NULL)
        Tcl_UnregisterChannel (interp, channel);
    if (fileNum >= 0)
        close (fileNum);
     Tcl_SetErrno (saveErrNo);
}

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
 * CallEvalErrorHandler --
 *
 *   Call the error handler specified in tclx_errorHandler.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter.
 * Returns:
 *   The Tcl result code from the handler.  TCL_ERROR is returned and
 * result unchanged if not handler is available.
 *-----------------------------------------------------------------------------
 */
static int
CallEvalErrorHandler (interp)
    Tcl_Interp  *interp;
{
    char         *errorHandler, *msgArg;
    char         *msgArgv [2];
    Tcl_DString   command;
    int           result;

    errorHandler = Tcl_GetVar (interp, "tclx_errorHandler", TCL_GLOBAL_ONLY);
    if (errorHandler == NULL)
        return TCL_ERROR;
    
    Tcl_DStringInit (&command);
    Tcl_DStringAppend (&command, errorHandler, -1);

    /*
     * Use list merge to quote the argument, as it probably contains spaces.
     */
    msgArgv [0] = interp->result;
    msgArgv [1] = NULL;
    msgArg = Tcl_Merge (1, msgArgv);

    Tcl_DStringAppend (&command, " ", -1);
    Tcl_DStringAppend (&command, msgArg, -1);
    ckfree (msgArg);

    result = Tcl_GlobalEval (interp, Tcl_DStringValue (&command));
    if (result == TCL_ERROR) {
        Tcl_AddErrorInfo (interp,
                          "\n    (while processing tclx_errorHandler)");
    }

    Tcl_DStringFree (&command);
    return result;
}

/*-----------------------------------------------------------------------------
 * TclX_Eval --
 *
 *   Evaluate a Tcl command string with various options.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter.
 *   o options (I) - Options controling the evaluation:
 *     o TCLX_EVAL_GLOBAL - Evaulate in the global context.
 *     o TCLX_EVAL_FILE - Treat the string as the name of a file to eval.
 *     o TCLX_EVAL_ERR_HANDLER - Call the user-specified error callback 
 *       specified in the global variable tclx_errorHandler if an error
 *       occurs.
 *   o string (I) - The command or name of file to evaluate.
 * Returns:
 *   The Tcl result code.
 *-----------------------------------------------------------------------------
 */
int
TclX_Eval (interp, options, string)
    Tcl_Interp  *interp;
    unsigned     options;
    char        *string;
{
    Interp      *iPtr = (Interp *) interp;
    CallFrame   *savedVarFramePtr;
    int          result;

    if (options & TCLX_EVAL_GLOBAL) {
        savedVarFramePtr = iPtr->varFramePtr;
        iPtr->varFramePtr = NULL;
    }

    if (options & TCLX_EVAL_FILE) {
        result = Tcl_EvalFile (interp, string);
    } else {
        result = Tcl_Eval (interp, string);
    }

    if ((result == TCL_ERROR) && (options & TCLX_EVAL_ERR_HANDLER)) {
        result = CallEvalErrorHandler (interp);
    }

    if (options & TCLX_EVAL_GLOBAL) {
        iPtr->varFramePtr = savedVarFramePtr;
    }
    return result;
}

/*-----------------------------------------------------------------------------
 * TclX_VarEval --
 *
 *   Evaluate a Tcl command string with various options.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter.
 *   o options (I) - Options controling the evaluation, see TclX_Eval.
 *   o str, ... (I) - String arguments, terminated by a NULL.  They will
 *     be concatenated together to form a single string.
 *-----------------------------------------------------------------------------
 */
int
TclX_VarEval TCL_VARARGS_DEF(Tcl_Interp *, arg1)
{
    va_list      argList;
    Tcl_Interp  *interp;
    unsigned     options;
    char        *str;
    Tcl_DString  cmdBuffer;
    int          result;

    Tcl_DStringInit (&cmdBuffer);

    interp = TCL_VARARGS_START (Tcl_Interp * ,arg1, argList);
    options = va_arg (argList, unsigned);

    while (1) {
        str = va_arg (argList, char *);
        if (str == NULL)
            break;
        Tcl_DStringAppend (&cmdBuffer, str, -1);
    }
    va_end (argList);

    result = TclX_Eval (interp, options, Tcl_DStringValue (&cmdBuffer));
    Tcl_DStringFree (&cmdBuffer);
    
    return result;
}

/*-----------------------------------------------------------------------------
 * TclX_WriteStr --
 *
 *   Write a string to a channel.
 *
 * Parameters:
 *   o channel (I) - Channel to write to.
 *   o str (I) - The string to write.
 * Returns:
 *   Same as for Tcl_Write, -1 is an error.
 *-----------------------------------------------------------------------------
 */
int
TclX_WriteStr (channel, str)
    Tcl_Channel  channel;
    char        *str;
{
    return Tcl_Write (channel, str, strlen (str));
}

/*-----------------------------------------------------------------------------
 * TclX_GetChannelOption --
 *
 *   C-friendly front end to Tcl_GetChannelOption.
 *
 * Parameters:
 *   o channel (I) - Channel to get the option for.
 *   o optionName (I) - One of the TCLX_COPT_* defines.
 * Returns:
 *   The interger option value define.
 *-----------------------------------------------------------------------------
 */
int
TclX_GetChannelOption (channel, option)
    Tcl_Channel channel;
    int         option;
{
    char *strOption;
    Tcl_DString strValue;
    int value;

    Tcl_DStringInit (&strValue);

    switch (option) {
      case TCLX_COPT_BLOCKING:
        strOption = "-blocking";
        break;

      case TCLX_COPT_BUFFERING:
        strOption = "-buffering";
        break;

      case TCLX_COPT_TRANSLATION:
        strOption = "-translation";
        break;

      default:
        goto fatalError;
    }

    if (Tcl_GetChannelOption (channel, strOption, &strValue) != TCL_OK)
        goto fatalError;

    switch (option) {
      case TCLX_COPT_BLOCKING:
        if (strValue.string [0] == '0') {
            value = TCLX_MODE_NONBLOCKING;
        } else {
            value = TCLX_MODE_BLOCKING;
        }
        break;

      case TCLX_COPT_BUFFERING:
        if (STREQU (strValue.string, "full")) {
            value = TCLX_BUFFERING_FULL;
        } else if (STREQU (strValue.string, "line")) {
            value = TCLX_BUFFERING_LINE;
        } else if (STREQU (strValue.string, "none")) {
            value = TCLX_BUFFERING_NONE;
        } else {
            goto fatalError;
        }
        break;

      case TCLX_COPT_TRANSLATION:
        if (STREQU (strValue.string, "auto")) {
            value = TCLX_TRANSLATE_AUTO;
        } else if (STREQU (strValue.string, "lf")) {
            value = TCLX_TRANSLATE_LF;
        } else if (STREQU (strValue.string, "binary")) {
            value = TCLX_TRANSLATE_BINARY;
        } else if (STREQU (strValue.string, "cr")) {
            value = TCLX_TRANSLATE_CR;
        } else if (STREQU (strValue.string, "crlf")) {
            value = TCLX_TRANSLATE_CRLF;
        } else if (STREQU (strValue.string, "platform")) {
            value = TCLX_TRANSLATE_PLATFORM;
        } else {
            goto fatalError;
        }
        break;
    }
    Tcl_DStringFree (&strValue);
    return value;

  fatalError:
    panic ("TclX_GetChannelOption bug");
}

/*-----------------------------------------------------------------------------
 * TclX_SetChannelOption --
 *
 *   C-friendly front end to Tcl_SetChannelOption.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o channel (I) - Channel to set the option for.
 *   o option (I) - One of the TCLX_COPT_* defines.
 *   o value (I) - Value to set the option too (integer define).
 * Result:
 *   TCL_OK or TCL_ERROR;
 *-----------------------------------------------------------------------------
 */
int
TclX_SetChannelOption (interp, channel, option, value)
    Tcl_Interp  *interp;
    Tcl_Channel  channel;
    int          option;
    int          value;
{
    char *strOption, *strValue;

    switch (option) {
      case TCLX_COPT_BLOCKING:
        strOption = "-blocking";
        switch (value) {
          case TCLX_MODE_BLOCKING:
            strValue = "1";
            break;
          case TCLX_MODE_NONBLOCKING:
            strValue = "0";
            break;
          default:
            goto fatalError;
        }
        break;

      case TCLX_COPT_BUFFERING:
        strOption = "-buffering";
        switch (value) {
          case TCLX_BUFFERING_FULL:
            strValue = "full";
            break;
          case TCLX_BUFFERING_LINE:
            strValue = "line";
            break;
          case TCLX_BUFFERING_NONE:
            strValue = "none";
            break;
          default:
            goto fatalError;
        }
        break;

      case TCLX_COPT_TRANSLATION:
        strOption = "-translation";
        switch (value) {
          case TCLX_TRANSLATE_AUTO:
            strValue = "auto";
            break;
          case TCLX_TRANSLATE_LF:  /* Also binary */
            strValue = "lf";
            break;
          case TCLX_TRANSLATE_CR:
            strValue = "cr";
            break;
          case TCLX_TRANSLATE_CRLF:
            strValue = "crlf";
            break;
          case TCLX_TRANSLATE_PLATFORM:
            strValue = "platform";
            break;
          default:
            goto fatalError;
        }
        break;

      default:
        goto fatalError;
    }

    return Tcl_SetChannelOption (interp, channel, strOption, strValue);

  fatalError:
    panic ("TclX_SetChannelOption bug");
}
