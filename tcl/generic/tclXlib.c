/*
 * tclXlib.c --
 *
 * Tcl commands to load libraries of Tcl code.
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
 * $Id: tclXlib.c,v 3.1 1993/12/13 04:36:14 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
 * The Extended Tcl library code is a super set of the standard Tcl libaries.
 * 
 * The following data structures are kept as Tcl variables so they can be
 * accessed from Tcl:
 *
 *   o auto_path - The directory path to search for libraries.
 *   o auto_oldpath - The previous value of auto_path.  Use to check if 
 *     the path needs to be searched again. Maybe unset to force searching
 *     of auto_path.
 *   o auto_index - An array indexed by command name and contains code to
 *     execute to make the command available.  Normally contains either:
 *       "source file"
 *       "auto_pkg_load package"
 *   o auto_pkg_index - Indexed by package name.
 *  
 *-----------------------------------------------------------------------------
 */
#include "tclExtdInt.h"

/*
 * Names of Tcl variables that are used.
 */
static char *AUTO_INDEX     = "auto_index";
static char *AUTO_PATH      = "auto_path";
static char *AUTO_OLDPATH   = "auto_oldpath";
static char *AUTO_PKG_INDEX = "auto_pkg_index";

/*
 * Per-interpreter structure used for managing the library.
 */
typedef struct libInfo_t {
    Tcl_HashTable inProgressTbl;     /* List of cmds being loaded.       */
    int           doingIdxSearch;    /* Loading indexes on a path now.   */
} libInfo_t;

/*
 * Prototypes of internal functions.
 */
static int
GlobalEvalFile _ANSI_ARGS_((Tcl_Interp *interp,
                            char       *file));

static int
EvalFilePart _ANSI_ARGS_((Tcl_Interp  *interp,
                          char        *fileName,
                          long         offset,
                          unsigned     length));

static char *
MakeAbsFile _ANSI_ARGS_((Tcl_Interp  *interp,
                         char        *fileName,
                         Tcl_DString *absNamePtr));

static int
SetPackageIndexEntry _ANSI_ARGS_((Tcl_Interp *interp,
                                  char       *packageName,
                                  char       *fileName,
                                  char       *offset,
                                  char       *length));

static int
GetPackageIndexEntry _ANSI_ARGS_((Tcl_Interp *interp,
                                  char       *packageName,
                                  char      **fileNamePtr,
                                  long       *offsetPtr,
                                  unsigned   *lengthPtr));

static int
SetProcIndexEntry _ANSI_ARGS_((Tcl_Interp *interp,
                               char       *procName,
                               char       *package));

static void
AddLibIndexErrorInfo _ANSI_ARGS_((Tcl_Interp *interp,
                                  char       *indexName));

static int
ProcessIndexFile _ANSI_ARGS_((Tcl_Interp *interp,
                              char       *tlibFilePath,
                              char       *tndxFilePath));

static int
BuildPackageIndex  _ANSI_ARGS_((Tcl_Interp *interp,
                                char       *tlibFilePath));

static int
LoadPackageIndex _ANSI_ARGS_((Tcl_Interp *interp,
                              char       *tlibFilePath));

static int
LoadOusterIndex _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *indexFilePath));

static int
LoadDirIndexes _ANSI_ARGS_((Tcl_Interp  *interp,
                            char        *dirName));

static int
LoadPackageIndexes _ANSI_ARGS_((Tcl_Interp  *interp,
                                libInfo_t   *infoPtr,
                                char        *path));

static int
AddInProgress _ANSI_ARGS_((Tcl_Interp  *interp,
                           libInfo_t   *infoPtr,
                           char        *command));

static void
RemoveInProgress _ANSI_ARGS_((Tcl_Interp  *interp,
                              libInfo_t   *infoPtr,
                              char        *command));

static int
LoadAutoPath _ANSI_ARGS_((Tcl_Interp  *interp,
                          libInfo_t   *infoPtr));

static int
LoadCommand _ANSI_ARGS_((Tcl_Interp  *interp,
                         char        *command));

static void
TclLibCleanUp _ANSI_ARGS_((ClientData  clientData,
                           Tcl_Interp *interp));

/*
 *-----------------------------------------------------------------------------
 * GlobalEvalFile --
 *
 *  Evaluate a file at global level in an interpreter.
 *-----------------------------------------------------------------------------
 */
static int
GlobalEvalFile(interp, file)
    Tcl_Interp *interp;
    char       *file;
{
    register Interp *iPtr = (Interp *) interp;
    int result;
    CallFrame *savedVarFramePtr;

    savedVarFramePtr = iPtr->varFramePtr;
    iPtr->varFramePtr = NULL;
    result = Tcl_EvalFile (interp, file);
    iPtr->varFramePtr = savedVarFramePtr;
    return result;
}

/*
 *-----------------------------------------------------------------------------
 * EvalFilePart --
 *
 *   Read in a byte range of a file and evaulate it.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o fileName (I) - The file to evaulate.
 *   o offset (I) - Byte offset into the file of the area to evaluate
 *   o length (I) - Number of bytes to evaulate..
 *-----------------------------------------------------------------------------
 */
static int
EvalFilePart (interp, fileName, offset, length)
    Tcl_Interp  *interp;
    char        *fileName;
    long         offset;
    unsigned     length;
{
    Interp       *iPtr = (Interp *) interp;
    int           fileNum, result;
    struct stat   statBuf;
    char         *oldScriptFile, *cmdBuffer, *buf;
    Tcl_DString   tildeBuf;

    Tcl_DStringInit (&tildeBuf);
    
    if (fileName [0] == '~') {
        if ((fileName = Tcl_TildeSubst (interp, fileName, &tildeBuf)) == NULL)
            return TCL_ERROR;
    }

    fileNum = open (fileName, O_RDONLY, 0);
    if (fileNum < 0) {
        Tcl_AppendResult (interp, "open failed on: ", fileName, ": ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }

    if (fstat (fileNum, &statBuf) == -1)
        goto accessError;

    if ((statBuf.st_size < offset + length) || (offset < 0)) {
        Tcl_AppendResult (interp, "range to eval outside of file bounds \"",
                          fileName, "\"", (char *) NULL);
        goto errorExit;
    }
    if (lseek (fileNum, offset, 0) < 0)
        goto accessError;

    cmdBuffer = ckalloc (length + 1);
    if (read (fileNum, cmdBuffer, length) != length)
        goto accessError;

    cmdBuffer [length] = '\0';

    if (close (fileNum) != 0)
        goto accessError;
    fileNum = -1;

    oldScriptFile = iPtr->scriptFile;
    iPtr->scriptFile = fileName;

    result = Tcl_GlobalEval (interp, cmdBuffer);

    iPtr->scriptFile = oldScriptFile;
    ckfree (cmdBuffer);
                         
    if (result != TCL_ERROR) {
        Tcl_DStringFree (&tildeBuf);
        return TCL_OK;
    }

    /*
     * An error occured in the command, record information telling where it
     * came from.
     */
    buf = ckalloc (sizeof (fileName) + 64);
    sprintf (buf, "\n    (file \"%s\" line %d)", fileName,
             interp->errorLine);
    Tcl_AddErrorInfo (interp, buf);
    ckfree (buf);
    goto errorExit;

    /*
     * Errors accessing the file once its opened are handled here.
     */
  accessError:
    Tcl_AppendResult (interp, "error accessing: ", fileName, ": ",
                      Tcl_PosixError (interp), (char *) NULL);

  errorExit:
    if (fileNum > 0)
        close (fileNum);
    Tcl_DStringFree (&tildeBuf);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * MakeAbsFile --
 *
 * Convert a file name to an absolute path.  This handles tilde substitution
 * and preappend the current directory name if the path is relative.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o fileName (I) - File name (should not start with a "/").
 *   o absNamePtr (O) - The name is returned in this dynamic string.
 * Returns:
 *   A pointer to the file name in the dynamic string or NULL if an error
 * occured.
 *-----------------------------------------------------------------------------
 */
static char *
MakeAbsFile (interp, fileName, absNamePtr)
    Tcl_Interp  *interp;
    char        *fileName;
    Tcl_DString *absNamePtr;
{
    char  curDir [MAXPATHLEN+1];

    Tcl_DStringFree (absNamePtr);

    /*
     * If its already absolute, just copy the name.
     */
    if (fileName [0] == '/') {
        Tcl_DStringAppend (absNamePtr, fileName, -1);
        return Tcl_DStringValue (absNamePtr);
    }

    /*
     * If it starts with a tilde, the substitution will make it
     * absolute.
     */
    if (fileName [0] == '~') {
        if (Tcl_TildeSubst (interp, fileName, absNamePtr) == NULL)
            return NULL;
        return Tcl_DStringValue (absNamePtr);
    }

    /*
     * Otherwise its relative to the current directory, get the directory
     * and go from here.
     */
#ifdef HAVE_GETCWD
    if (getcwd (curDir, MAXPATHLEN) == NULL) {
        Tcl_AppendResult (interp, "error getting working directory name: ",
                          Tcl_PosixError (interp), (char *) NULL);
    }
#else
    if (getwd (curDir) == NULL) {
        Tcl_AppendResult (interp, "error getting working directory name: ",
                          curDir, (char *) NULL);
    }
#endif
    Tcl_DStringAppend (absNamePtr, curDir,   -1);
    Tcl_DStringAppend (absNamePtr, "/",      -1);
    Tcl_DStringAppend (absNamePtr, fileName, -1);

    return Tcl_DStringValue (absNamePtr);
}

/*
 *-----------------------------------------------------------------------------
 * SetPackageIndexEntry --
 *
 * Set a package entry in the auto_pkg_index array in the form:
 *
 *     auto_pkg_index($packageName) [list $filename $offset $length]
 *
 * Duplicate package entries are overwritten.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o packageName (I) - Package name.
 *   o fileName (I) - Absolute file name of the file containing the package.
 *   o offset (I) - String containing the numeric start of the package.
 *   o length (I) - String containing the numeric length of the package.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
SetPackageIndexEntry (interp, packageName, fileName, offset, length)
     Tcl_Interp *interp;
     char       *packageName;
     char       *fileName;
     char       *offset;
     char       *length;
{
    char *pkgDataArgv [3], *dataStr, *setResult;

    /*
     * Build up the list of values to save.
     */
    pkgDataArgv [0] = fileName;
    pkgDataArgv [1] = offset;
    pkgDataArgv [2] = length;
    dataStr = Tcl_Merge (3, pkgDataArgv);

    setResult = Tcl_SetVar2 (interp, AUTO_PKG_INDEX, packageName, dataStr,
                             TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    ckfree (dataStr);

    return (setResult == NULL) ? TCL_ERROR : TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * GetPackageIndexEntry --
 *
 * Get a package entry from the auto_pkg_index array.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o packageName (I) - Package name to find.
 *   o fileNamePtr (O) - The file name for the library file is returned here.
 *     This should be freed by the caller.
 *   o offsetPtr (O) - Start of the package in the library.
 *   o lengthPtr (O) - Length of the package in the library.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
GetPackageIndexEntry (interp, packageName, fileNamePtr, offsetPtr, lengthPtr)
     Tcl_Interp *interp;
     char       *packageName;
     char      **fileNamePtr;
     long       *offsetPtr;
     unsigned   *lengthPtr;
{
    int   pkgDataArgc, idx;
    char *dataStr, **pkgDataArgv = NULL;
    char *srcPtr, *destPtr;

    /*
     * Look up the package entry in the array.
     */
    dataStr = Tcl_GetVar2 (interp, AUTO_PKG_INDEX, packageName,
                           TCL_GLOBAL_ONLY);
    if (dataStr == NULL) {
        Tcl_AppendResult (interp, "entry not found in \"auto_pkg_index \"",
                          "for package \"", packageName, "\"", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Extract the data from the array entry.  The file name will be copied
     * to the top of the memory area returned by Tcl_SplitList after the
     * other fields have been accessed.  Copied in a way allowing for overlap.
     */
    if (Tcl_SplitList (interp, dataStr, &pkgDataArgc, &pkgDataArgv) != TCL_OK)
        goto invalidEntry;
    if (pkgDataArgc != 3)
        goto invalidEntry;

    if (!Tcl_StrToLong (pkgDataArgv [1], 0, offsetPtr))
        goto invalidEntry;
    if (!Tcl_StrToUnsigned (pkgDataArgv [2], 0, lengthPtr))
        goto invalidEntry;

    *fileNamePtr = destPtr = (char *) pkgDataArgv;
    srcPtr = pkgDataArgv [0];

    while (*srcPtr != '\0') {
        *destPtr++ = *srcPtr++;
    }
    *destPtr = '\0';

    return TCL_OK;
    
    /*
     * Exit point when an invalid entry is found.
     */
  invalidEntry:
    if (pkgDataArgv != NULL)
        ckfree (pkgDataArgv);
    Tcl_ResetResult (interp);
    Tcl_AppendResult (interp, "invalid entry in \"auto_pkg_index \"",
                      "for package \"", packageName, "\"", (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * SetProcIndexEntry --
 *
 * Set the proc entry in the auto_index array.  These entry contains a command
 * to make the proc available from a package.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o procName (I) - The Tcl proc name.
 *   o package (I) - Pacakge containing the proc.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
SetProcIndexEntry (interp, procName, package)
    Tcl_Interp *interp;
    char       *procName;
    char       *package;
{
    Tcl_DString  command;
    char        *result;

    Tcl_DStringInit (&command);
    Tcl_DStringAppend (&command, "auto_load_pkg {", -1);
    Tcl_DStringAppend (&command, package, -1);
    Tcl_DStringAppend (&command, "}", -1);

    result = Tcl_SetVar2 (interp, AUTO_INDEX, procName, command.string,
                          TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);

    Tcl_DStringFree (&command);

    return (result == NULL) ? TCL_ERROR : TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * AddLibIndexErrorInfo --
 *
 * Add information to the error info stack about index that just failed.
 * This is generic for both tclIndex and .tlib indexs
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o indexName (I) - The name of the index.
 *-----------------------------------------------------------------------------
 */
static void
AddLibIndexErrorInfo (interp, indexName)
    Tcl_Interp *interp;
    char       *indexName;
{
    char *msg;

    msg = ckalloc (strlen (indexName) + 60);
    strcpy (msg, "\n    while loading Tcl library index \"");
    strcat (msg, indexName);
    strcat (msg, "\"");
    Tcl_AddErrorInfo (interp, msg);
    ckfree (msg);
}


/*
 *-----------------------------------------------------------------------------
 * ProcessIndexFile --
 *
 * Open and process a package library index file (.tndx).  Creates entries
 * in the auto_index and auto_pkg_index arrays.  Existing entries are over
 * written.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o tlibFilePath (I) - Absolute path name to the library file.
 *   o tndxFilePath (I) - Absolute path name to the library file index.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ProcessIndexFile (interp, tlibFilePath, tndxFilePath)
     Tcl_Interp *interp;
     char       *tlibFilePath;
     char       *tndxFilePath;
{
    FILE        *indexFilePtr = NULL;
    Tcl_DString  lineBuffer;
    int          lineArgc, idx, result, status;
    char       **lineArgv = NULL;

    indexFilePtr = fopen (tndxFilePath, "r");
    if (indexFilePtr == NULL)
        goto fileError;
    
    Tcl_DStringInit (&lineBuffer);

    while (TRUE) {
        Tcl_DStringFree (&lineBuffer);
        status = Tcl_DStringGets (indexFilePtr, &lineBuffer);
        if (status == TCL_BREAK)
            goto reachedEOF;
        if (status == TCL_ERROR)
            goto fileError;

        if ((Tcl_SplitList (interp, lineBuffer.string, &lineArgc,
                            &lineArgv) != TCL_OK) || (lineArgc < 4))
            goto formatError;
        
        /*
         * lineArgv [0] is the package name.
         * lineArgv [1] is the package offset in the library.
         * lineArgv [2] is the package length in the library.
         * lineArgv [3-n] are the entry procedures for the package.
         */
        result = SetPackageIndexEntry (interp, lineArgv [0], tlibFilePath,
                                       lineArgv [1], lineArgv [2]);
        if (result == TCL_ERROR)
            goto errorExit;

        /*
         * If the package is not duplicated, add the commands to load
         * the procedures.
         */
        if (result != TCL_CONTINUE) {
            for (idx = 3; idx < lineArgc; idx++) {
                if (SetProcIndexEntry (interp, lineArgv [idx],
                                       lineArgv [0]) != TCL_OK)
                    goto errorExit;
            }
        }
        ckfree (lineArgv);
        lineArgv = NULL;
    }

  reachedEOF:
    fclose (indexFilePtr);
    Tcl_DStringFree (&lineBuffer);

    return TCL_OK;

    /*
     * Handle format error in library input line.
     */
  formatError:
    Tcl_ResetResult (interp);
    Tcl_AppendResult (interp, "format error in library index \"",
                      tndxFilePath, "\" (", lineBuffer.string, ")",
                      (char *) NULL);
    goto errorExit;


  fileError:
    Tcl_AppendResult (interp, "error accessing package index file \"",
                      tndxFilePath, "\": ", Tcl_PosixError (interp),
                      (char *) NULL);
    goto errorExit;

    /*
     * Error exit here, releasing resources and closing the file.
     */
  errorExit:
    if (lineArgv != NULL)
        ckfree (lineArgv);
    Tcl_DStringFree (&lineBuffer);
    if (indexFilePtr != NULL)
        fclose (indexFilePtr);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * BuildPackageIndex --
 *
 * Call the "buildpackageindex" Tcl procedure to rebuild a package index.
 * This is found with the [info library] command.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o tlibFilePath (I) - Absolute path name to the library file.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *
 * ????Change name to auto something.
 *-----------------------------------------------------------------------------
 */
static int
BuildPackageIndex (interp, tlibFilePath)
     Tcl_Interp *interp;
     char       *tlibFilePath;
{
    Tcl_DString  command;
    int          result;

    Tcl_DStringInit (&command);

    Tcl_DStringAppend (&command, "source [info library]/buildidx.tcl;", -1);
    Tcl_DStringAppend (&command, "buildpackageindex ", -1);
    Tcl_DStringAppend (&command, tlibFilePath, -1);

    result = Tcl_GlobalEval (interp, command.string);

    Tcl_DStringFree (&command);

    if (result == TCL_ERROR)
        return TCL_ERROR;
    Tcl_ResetResult (interp);
    return result;
}

/*
 *-----------------------------------------------------------------------------
 * LoadPackageIndex --
 *
 * Load a package .tndx file.  Rebuild .tlib if non-existant or out of
 * date.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o tlibFilePath (I) - Absolute path name to the library file.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
LoadPackageIndex (interp, tlibFilePath)
    Tcl_Interp *interp;
    char       *tlibFilePath;
{
    Tcl_DString  tndxFilePath;
    struct stat  tlibStat;
    struct stat  tndxStat;

    Tcl_DStringInit (&tndxFilePath);

    Tcl_DStringAppend (&tndxFilePath, tlibFilePath, -1);
    tndxFilePath.string [tndxFilePath.length - 3] = 'n';
    tndxFilePath.string [tndxFilePath.length - 2] = 'd';
    tndxFilePath.string [tndxFilePath.length - 1] = 'x';

    /*
     * Get library's modification time.  If the file can't be accessed, set
     * time so the library does not get built.  Other code will report the
     * error.
     */
    if (stat (tlibFilePath, &tlibStat) < 0)
        tlibStat.st_mtime = MAXINT;

    /*
     * Get the time for the index.  If the file does not exists or is
     * out of date, rebuild it.
     */
    if ((stat (tndxFilePath.string, &tndxStat) < 0) ||
        (tndxStat.st_mtime < tlibStat.st_mtime)) {
        if (BuildPackageIndex (interp, tlibFilePath) != TCL_OK)
            goto errorExit;
    }

    if (ProcessIndexFile (interp, tlibFilePath, tndxFilePath.string) != TCL_OK)
        goto errorExit;
    Tcl_DStringFree (&tndxFilePath);
    return TCL_OK;

  errorExit:
    AddLibIndexErrorInfo (interp, tndxFilePath.string);
    Tcl_DStringFree (&tndxFilePath);

    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * LoadOusterIndex --
 *
 * Load a standard Tcl index (tclIndex).  A special proc is used so that the
 * "dir" variable can be set.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o indexFilePath (I) - Absolute path name to the tclIndex file.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 * Notes:
 *   The real work of loading an index is done by a procedure that is defined
 * in a seperate file.  Its not possible to put this file in the standard
 * tcl.tlib as a tclIndex might get loaded before the tcl.tndx file is found
 * on the search path.  The function sets up the auto_index array to load the
 * procedure if its not already defined. 
 *-----------------------------------------------------------------------------
 */
static int
LoadOusterIndex (interp, indexFilePath)
     Tcl_Interp *interp;
     char       *indexFilePath;
{
    Tcl_DString  command;
    
    Tcl_DStringInit (&command);
    Tcl_DStringAppend (&command, "set auto_index(auto_load_ouster_index) ",
                       -1);
    Tcl_DStringAppend (&command, "\"source [info library]/loadouster.tcl\";",
                       -1);
    Tcl_DStringAppend (&command, "auto_load_ouster_index {", -1);
    Tcl_DStringAppend (&command, indexFilePath, -1);
    Tcl_DStringAppend (&command, "}", -1);

    if (Tcl_GlobalEval (interp, command.string) == TCL_ERROR) {
        AddLibIndexErrorInfo (interp, indexFilePath);
        Tcl_DStringFree (&command);
        return TCL_ERROR;
    }
    Tcl_DStringFree (&command);
    Tcl_ResetResult (interp);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * LoadDirIndexes --
 *
 *     Load the indexes for all package library (.tlib) or a Ousterhout
 *  "tclIndex" file in a directory.  Nonexistent or unreadable directories
 *  are skipped.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o dirName (I) - The absolute path name of the directory to search for
 *     libraries.
 *-----------------------------------------------------------------------------
 */
static int
LoadDirIndexes (interp, dirName)
    Tcl_Interp  *interp;
    char        *dirName;
{
    DIR           *dirPtr;
    struct dirent *entryPtr;
    int            dirNameLen, nameLen;
    Tcl_DString    filePath;

    dirPtr = opendir (dirName);
    if (dirPtr == NULL)
        return TCL_OK;   /* Skip directory */

    Tcl_DStringInit (&filePath);
    Tcl_DStringAppend (&filePath, dirName, -1);
    Tcl_DStringAppend (&filePath, "/",     -1);

    dirNameLen = strlen (dirName) + 1;  /* Include `/' */

    while (TRUE) {
        entryPtr = readdir (dirPtr);
        if (entryPtr == NULL)
            break;
        nameLen = strlen (entryPtr->d_name);

        if ((nameLen > 5) && 
            ((STREQU (entryPtr->d_name + nameLen - 5, ".tlib")) ||
             (STREQU (entryPtr->d_name, "tclIndex")))) {

            /*
             * Append the file name on to the directory.
             */
            Tcl_DStringTrunc (&filePath, dirNameLen);
            Tcl_DStringAppend (&filePath, entryPtr->d_name, -1);

            /*
             * Skip index it can't be accessed.
             */
            if (access (filePath.string, R_OK) < 0)
                continue;

            /*
             * Process the index according to its type.
             */
            if (entryPtr->d_name [nameLen - 5] == '.') {
                if (LoadPackageIndex (interp, filePath.string) != TCL_OK)
                    goto errorExit;
            } else {
                if (LoadOusterIndex (interp, filePath.string) != TCL_OK)
                    goto errorExit;
            }
        }
    }

    Tcl_DStringFree (&filePath);
    closedir (dirPtr);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&filePath);
    closedir (dirPtr);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * LoadPackageIndexes --
 *
 * Loads the all indexes for all package libraries (.tlib) or a Ousterhout
 * "tclIndex" files found in all directories in the path.  The path is search
 * backwards so that index entries first in the path will override those 
 * later on in the path.
 *-----------------------------------------------------------------------------
 */
static int
LoadPackageIndexes (interp, infoPtr, path)
    Tcl_Interp  *interp;
    libInfo_t   *infoPtr;
    char        *path;
{
    char        *dirName;
    Tcl_DString  dirNameBuf;
    int          idx, pathArgc, result = TCL_OK;
    char       **pathArgv;

    Tcl_DStringInit (&dirNameBuf);

    if (infoPtr->doingIdxSearch) {
        Tcl_AppendResult (interp, "recursive load of indexes ",
                          "(probable invalid command while loading index)",
                          (char *) NULL);
        return TCL_ERROR;
    }
    infoPtr->doingIdxSearch = TRUE;

    if (Tcl_SplitList (interp, path, &pathArgc, &pathArgv) != TCL_OK) {
        infoPtr->doingIdxSearch = FALSE;
        return TCL_ERROR;
    }

    for (idx = pathArgc - 1; idx >= 0; idx--) {
        /*
         * Get the absolute dir name.  if the conversion fails (most likely
         * invalid "~") or the directory can't be read, skip it.
         */
        dirName = MakeAbsFile (interp, pathArgv [idx], &dirNameBuf);
        if (dirName == NULL)
            continue;
        
        if (access (dirName, X_OK) == 0)
            result = LoadDirIndexes (interp, dirName);
        else
            result = TCL_OK;

        Tcl_DStringFree (&dirNameBuf);
        if (result != TCL_OK)
            break;
    }

    ckfree (pathArgv);
    infoPtr->doingIdxSearch = FALSE;
    return result;
}

/*
 *-----------------------------------------------------------------------------
 * Tcl_Auto_load_pkgCmd --
 *
 *   Implements the command:
 *      auto_load_pkg package
 *
 * Which is called to load a .tlib package who's index has already been loaded.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_Auto_load_pkgCmd (dummy, interp, argc, argv)
    ClientData   dummy;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    char     *fileName;
    long      offset;
    unsigned  length;
    int       result;

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " package",
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (GetPackageIndexEntry (interp, argv [1], &fileName, &offset,
                              &length) != TCL_OK)
        return TCL_ERROR;

    result = EvalFilePart (interp, fileName, offset, length);
    ckfree (fileName);

    return result;
}

/*
 *-----------------------------------------------------------------------------
 * AddInProgress --
 *
 *   An a command to the table of in progress commands.  If the command is
 * already in the table, return an error.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o infoPtr (I) - Interpreter specific library info.
 *   o command (I) - The command to add.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
AddInProgress (interp, infoPtr, command)
    Tcl_Interp  *interp;
    libInfo_t   *infoPtr;
    char        *command;
{
    int  newEntry;

    Tcl_CreateHashEntry (&infoPtr->inProgressTbl, command, &newEntry);

    if (!newEntry) {
        Tcl_AppendResult (interp, "recursive auto_load of \"",
                          command, "\"", (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * RemoveInProgress --
 *
 *   Remove a command from the in progress table.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o infoPtr (I) - Interpreter specific library info.
 *   o command (I) - The command to remove.
 *-----------------------------------------------------------------------------
 */
static void
RemoveInProgress (interp, infoPtr, command)
    Tcl_Interp  *interp;
    libInfo_t   *infoPtr;
    char        *command;
{
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FindHashEntry (&infoPtr->inProgressTbl, command);
    if (entryPtr == NULL)
        panic ("lost in-progress command");

    Tcl_DeleteHashEntry (entryPtr);
}

/*
 *-----------------------------------------------------------------------------
 * LoadAutoPath --
 *
 *   Load all indexs on the auto_path variable.  If auto_path has not changed
 * since the last time libraries were successfully loaded, this is a no-op.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o infoPtr (I) - Interpreter specific library info.
 * Returns:
 *   TCL_OK or TCL_ERROR
 *-----------------------------------------------------------------------------
 */
static int
LoadAutoPath (interp, infoPtr)
    Tcl_Interp  *interp;
    libInfo_t   *infoPtr;
{
    char  *path, *oldPath;

    path = Tcl_GetVar (interp, AUTO_PATH, TCL_GLOBAL_ONLY);
    if (path == NULL)
        return TCL_OK;

    oldPath = Tcl_GetVar (interp, AUTO_OLDPATH, TCL_GLOBAL_ONLY);

    /*
     * Check if the path has changed.  If it has, load indexes, and
     * save the path if it succeeds.
     */
    if ((oldPath != NULL) && STREQU (path, oldPath))
        return TCL_OK;

    if (LoadPackageIndexes (interp, infoPtr, path) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_SetVar (interp, AUTO_OLDPATH, path,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * LoadCommand --
 *
 *   Check the "auto_index" array for code to load a command and eval it.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o command (I) - The command to load.
 * Returns:
 *   TCL_OK if the command was loaded.
 *   TCL_CONTINUE if the command is not in the index.
 *   TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
static int
LoadCommand (interp, command)
    Tcl_Interp  *interp;
    char        *command;
{
    char              *loadCmd;
    ClientData         clientData;
    Tcl_CmdDeleteProc *deleteProc;
    Tcl_CmdInfo        cmdInfo;

    loadCmd = Tcl_GetVar2 (interp, AUTO_INDEX, command, TCL_GLOBAL_ONLY);
    if (loadCmd == NULL)
        return TCL_CONTINUE;   /* Not found */

    if (Tcl_GlobalEval (interp, loadCmd) == TCL_ERROR)
        return TCL_ERROR;
    Tcl_ResetResult (interp);

    if (Tcl_GetCommandInfo (interp, command, &cmdInfo))
        return TCL_OK;  /* Found and loaded */

    Tcl_AppendResult (interp, "command \"", command, "\" was defined in a Tcl",
                      " library index, but not in a Tcl library",
                      (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * Tcl_LoadlibindexCmd --
 *
 *   This procedure is invoked to process the "Loadlibindex" Tcl command:
 *
 *      loadlibindex libfile
 *
 * which loads the index for a package library (.tlib) or a Ousterhout
 * "tclIndex" file.  New package definitions will override existing ones.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_LoadlibindexCmd (dummy, interp, argc, argv)
    ClientData   dummy;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    char        *pathName;
    Tcl_DString  pathNameBuf;
    int          pathLen;

    Tcl_DStringInit (&pathNameBuf);

    if (argc != 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " libFile",
                          (char *) NULL);
        return TCL_ERROR;
    }

    pathName = MakeAbsFile (interp, argv [1], &pathNameBuf);
    if (pathName == NULL)
        return TCL_ERROR;

    /*
     * Find the length of the directory name. Validate that we have a .tlib
     * extension or file name is "tclIndex" and call the routine to process
     * the specific type of index.
     */
    pathLen = strlen (pathName);

    if ((pathLen > 5) && (pathName [pathLen - 5] == '.')) {
        if (!STREQU (pathName + pathLen - 5, ".tlib"))
            goto invalidName;
        if (LoadPackageIndex (interp, pathName) != TCL_OK)
            goto errorExit;
    } else {
        if (!STREQU (pathName + pathLen - 9, "/tclIndex"))
            goto invalidName;
        if (LoadOusterIndex (interp, pathName) != TCL_OK)
            goto errorExit;
    }
    Tcl_DStringFree (&pathNameBuf);
    return TCL_OK;

  invalidName:
    Tcl_AppendResult (interp, "invalid library name, must have an extension ",
                      "of \".tlib\" or the name \"tclIndex\", got \"",
                      argv [1], "\"", (char *) NULL);

  errorExit:
    Tcl_DStringFree (&pathNameBuf);
    return TCL_ERROR;;
}

/*
 *-----------------------------------------------------------------------------
 * Tcl_auto_loadCmd --
 *
 *   This procedure is invoked to process the "auto_load" Tcl command:
 *
 *         auto_load ?command?
 *
 * which searchs the auto_load tables for the specified procedure.  If it
 * is not found, an attempt is made to load unloaded library indexes by
 * searching auto_path.
 *-----------------------------------------------------------------------------
 */
static int
Tcl_Auto_loadCmd (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    libInfo_t *infoPtr = (libInfo_t *) clientData;
    int        result;
    char      *msg;

    if (argc > 2) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], " ?command?",
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * If no command is specified, just load the indexs.
     */
    if (argc == 1)
        return LoadAutoPath (interp, infoPtr);

    /*
     * Do checking for recursive auto_load of the same command.
     */
    if (AddInProgress (interp, infoPtr, argv [1]) != TCL_OK)
        return TCL_ERROR;

    /*
     * First, attempt to load it from the indexes in memory.
     */
    result = LoadCommand (interp, argv [1]);
    if (result == TCL_ERROR)
        goto errorExit;
    if (result == TCL_OK)
        goto found;
    
    /*
     * Slow path, load the libraries indices on auto_path.
     */
    if (LoadAutoPath (interp, infoPtr) != TCL_OK)
        goto errorExit;

    /*
     * Try to load the command again.
     */
    result = LoadCommand (interp, argv [1]);
    if (result == TCL_ERROR)
        goto errorExit;
    if (result != TCL_OK)
        goto notFound;

  found:
    RemoveInProgress (interp, infoPtr, argv [1]);
    interp->result = "1";
    return TCL_OK;

  notFound:
    RemoveInProgress (interp, infoPtr, argv [1]);
    interp->result = "0";
    return TCL_OK;

  errorExit:
    msg = ckalloc (strlen (argv [1]) + 35);
    strcpy (msg, "\n    while auto loading \"");
    strcat (msg, argv [1]);
    strcat (msg, "\"");
    Tcl_AddErrorInfo (interp, msg);
    ckfree (msg);

    RemoveInProgress (interp, infoPtr, argv [1]);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * TclLibCleanUp --
 *
 *   Release the client data area when the interpreter is deleted.
 *-----------------------------------------------------------------------------
 */
static void
TclLibCleanUp (clientData, interp)
    ClientData  clientData;
    Tcl_Interp *interp;
{
    libInfo_t      *infoPtr = (libInfo_t *) clientData;
    Tcl_HashSearch  searchCookie;
    Tcl_HashEntry  *entryPtr;

    entryPtr = Tcl_FirstHashEntry (&infoPtr->inProgressTbl, &searchCookie);

    while (entryPtr != NULL) {
        Tcl_DeleteHashEntry (entryPtr);
        entryPtr = Tcl_NextHashEntry (&searchCookie);
    }

    Tcl_DeleteHashTable (&infoPtr->inProgressTbl);
    ckfree ((char *) infoPtr);
}

/*
 *-----------------------------------------------------------------------------
 * TclXLib_Init --
 *
 *   Initialize the Extended Tcl library facility commands.
 *-----------------------------------------------------------------------------
 */
int
TclXLib_Init (interp)
    Tcl_Interp *interp;
{
    libInfo_t *infoPtr;

    infoPtr = (libInfo_t *) ckalloc (sizeof (libInfo_t));

    Tcl_InitHashTable (&infoPtr->inProgressTbl, TCL_STRING_KEYS);
    infoPtr->doingIdxSearch = FALSE;

    Tcl_CallWhenDeleted (interp, TclLibCleanUp, (ClientData) infoPtr);

    Tcl_CreateCommand (interp, "auto_load_pkg", Tcl_Auto_load_pkgCmd,
                      (ClientData) infoPtr, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "auto_load", Tcl_Auto_loadCmd,
                      (ClientData) infoPtr, (void (*)()) NULL);
    Tcl_CreateCommand (interp, "loadlibindex", Tcl_LoadlibindexCmd,
                      (ClientData) infoPtr, (void (*)()) NULL);
    return TCL_OK;
}

