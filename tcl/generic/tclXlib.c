/*
 * tclXlib.c --
 *
 * Tcl commands to load libraries of Tcl code.
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
 * $Id: tclXlib.c,v 8.9 1997/06/30 17:21:40 markd Exp $
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
 * Command to pass to Tcl_GlobalEval to load the file loadouster.tcl.
 * This is a global rather than a local so it will work with K&R compilers.
 * Its writable so it works with gcc.
 */
static char loadOusterCmd [] = "source [file join $tclx_library loadouster.tcl]";

/*
 * Indicates the type of library index.
 */
typedef enum {
    TCLLIB_TNDX,       /* *.tndx                   */
    TCLLIB_TND,        /* *.tnd (.tndx in 8.3 land */
    TCLLIB_OUSTER      /* tclIndex                 */
} indexNameClass_t;

/*
 * Indicates if we have ITcl namespaces and the command to check for.
 */
static int haveNameSpaces = FALSE;
static char nameSpaceCheckCommand [] = "@scope";

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
EvalFilePart _ANSI_ARGS_((Tcl_Interp  *interp,
                          char        *fileName,
                          off_t        offset,
                          off_t        length));

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
                                  off_t      *offsetPtr,
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
LoadPackageIndex _ANSI_ARGS_((Tcl_Interp       *interp,
                              char             *tlibFilePath,
                              indexNameClass_t  indexNameClass));

static int
LoadOusterIndex _ANSI_ARGS_((Tcl_Interp *interp,
                             char       *indexFilePath));

static int
LoadDirIndexCallback _ANSI_ARGS_((Tcl_Interp  *interp,
                                  char        *path,
                                  char        *fileName,
                                  int          caseSensitive,
                                  ClientData   clientData));

static int
LoadDirIndexes _ANSI_ARGS_((Tcl_Interp  *interp,
                            char        *dirName));

static int
LoadPackageIndexes _ANSI_ARGS_((Tcl_Interp  *interp,
                                libInfo_t   *infoPtr,
                                char        *path));

static int
TclX_Auto_load_pkgObjCmd _ANSI_ARGS_((ClientData clientData, 
                                      Tcl_Interp *interp,
                                      int objc,
                                      Tcl_Obj *CONST objv[]));

static int
AddInProgress _ANSI_ARGS_((Tcl_Interp  *interp,
                           libInfo_t   *infoPtr,
                           char        *command));

static void
RemoveInProgress _ANSI_ARGS_((Tcl_Interp  *interp,
                              libInfo_t   *infoPtr,
                              char        *command));

static int
LoadChangedPathPackageIndexes _ANSI_ARGS_((Tcl_Interp  *interp,
                                           libInfo_t   *infoPtr,
                                           char        *oldPath,
                                           char        *path));

static int
LoadAutoPath _ANSI_ARGS_((Tcl_Interp  *interp,
                          libInfo_t   *infoPtr));

static int
LoadStdProc _ANSI_ARGS_((Tcl_Interp  *interp,
                         char        *command));

static int
LoadITclImportProc _ANSI_ARGS_((Tcl_Interp  *interp,
                                char        *command));

static int
LoadCommand _ANSI_ARGS_((Tcl_Interp  *interp,
                         char        *command));

static int
TclX_LoadlibindexObjCmd _ANSI_ARGS_((ClientData clientData, 
                                     Tcl_Interp *interp,
                                     int objc,
                                     Tcl_Obj *CONST objv[]));

static int
TclX_Auto_loadObjCmd _ANSI_ARGS_((ClientData clientData, 
                                  Tcl_Interp *interp,
                                  int objc,
                                  Tcl_Obj *CONST objv[]));

static void
TclLibCleanUp _ANSI_ARGS_((ClientData  clientData,
                           Tcl_Interp *interp));


/*-----------------------------------------------------------------------------
 * EvalFilePart --
 *
 *   Read in a byte range of a file and evaulate it.
 *
 * Parameters:
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o fileName (I) - The file to evaulate.
 *   o offset (I) - Byte offset into the file of the area to evaluate
 *   o length (I) - Number of bytes to evaulate.
 *-----------------------------------------------------------------------------
 */
static int
EvalFilePart (interp, fileName, offset, length)
    Tcl_Interp  *interp;
    char        *fileName;
    off_t        offset;
    off_t        length;
{
    Interp *iPtr = (Interp *) interp;
    int result;
    off_t fileSize;
    Tcl_DString pathBuf, cmdBuf;
    char *oldScriptFile, *buf;
    Tcl_Channel channel = NULL;

    Tcl_ResetResult (interp);
    Tcl_DStringInit (&pathBuf);
    Tcl_DStringInit (&cmdBuf);

    fileName = Tcl_TranslateFileName (interp, fileName, &pathBuf);
    if (fileName == NULL)
        goto errorExit;

    channel = Tcl_OpenFileChannel (interp, fileName, "r", 0);
    if (channel == NULL)
        goto errorExit;

    if (TclXOSGetFileSize (channel, TCL_READABLE, &fileSize) == TCL_ERROR)
        goto posixError;

    if ((fileSize < offset + length) || (offset < 0)) {
        TclX_AppendObjResult (interp,
                              "range to eval outside of file bounds in \"",
                              fileName, "\", index file probably corrupt",
                              (char *) NULL);
        goto errorExit;
    }

    if (Tcl_Seek (channel, offset, SEEK_SET) < 0)
        goto posixError;

    Tcl_DStringSetLength (&cmdBuf, length + 1);
    if (Tcl_Read (channel, cmdBuf.string, length) != length) {
        if (Tcl_Eof (channel))
            goto prematureEof;
        else
            goto posixError;
    }
    cmdBuf.string [length] = '\0';

    if (Tcl_Close (NULL, channel) != 0)
        goto posixError;
    channel = NULL;

    oldScriptFile = iPtr->scriptFile;
    iPtr->scriptFile = fileName;
    result = Tcl_GlobalEval (interp, cmdBuf.string);
    iPtr->scriptFile = oldScriptFile;
    
    Tcl_DStringFree (&pathBuf);
    Tcl_DStringFree (&cmdBuf);

    if (result != TCL_ERROR) {
        return TCL_OK;
    }

    /*
     * An error occured in the command, record information telling where it
     * came from.
     */
    buf = ckalloc (strlen (fileName) + 64);
    sprintf (buf, "\n    (file \"%s\" line %d)", fileName,
             interp->errorLine);
    Tcl_AddErrorInfo (interp, buf);
    ckfree (buf);
    goto errorExit;

    /*
     * Errors accessing the file once its opened are handled here.
     */
  posixError:
    TclX_AppendObjResult (interp, "error accessing: ", fileName, ": ",
                       Tcl_PosixError (interp), (char *) NULL);
    goto errorExit;

  prematureEof:
    TclX_AppendObjResult (interp, "premature EOF on: ", fileName,
                          (char *) NULL);
    goto errorExit;

  errorExit:
    if (channel != NULL)
        Tcl_Close (NULL, channel);
    Tcl_DStringFree (&pathBuf);
    Tcl_DStringFree (&cmdBuf);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * MakeAbsFile --
 *
 * Convert a file name to an absolute path.  This handles file name translation
 * and preappend the current directory name if the path is relative.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o fileName (I) - File name (should not start with a "/").
 *   o absNamePtr (O) - The name is returned in this dynamic string.  It
 *     should be initialized.
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
    char  *curDir;
    Tcl_DString joinBuf;

    Tcl_DStringSetLength (absNamePtr, 1);

    fileName = Tcl_TranslateFileName (interp, fileName, absNamePtr);
    if (fileName == NULL)
        goto errorExit;

    /*
     * If its already absolute.  If name translation didn't actually
     * copy the name to the buffer, we must do it now.
     */
    if (Tcl_GetPathType (fileName) == TCL_PATH_ABSOLUTE) {
        if (fileName != absNamePtr->string) {
            Tcl_DStringAppend (absNamePtr, fileName, -1);
        }
        return Tcl_DStringValue (absNamePtr);
    }

    /*
     * Otherwise its relative to the current directory, get the directory
     * and join into a path.
     */
    curDir = TclGetCwd (interp);
    if (curDir == NULL)
        goto errorExit;

    Tcl_DStringInit (&joinBuf);
    TclX_JoinPath (curDir, fileName, &joinBuf);
    Tcl_DStringSetLength (absNamePtr, 0);
    Tcl_DStringAppend (absNamePtr, joinBuf.string, -1);
    Tcl_DStringFree (&joinBuf);

    return Tcl_DStringValue (absNamePtr);

  errorExit:
    return NULL;
}

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
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
     off_t       *offsetPtr;
     unsigned   *lengthPtr;
{
    int   pkgDataArgc;
    char *dataStr, **pkgDataArgv = NULL;
    char *srcPtr, *destPtr;

    /*
     * Look up the package entry in the array.
     */
    dataStr = Tcl_GetVar2 (interp, AUTO_PKG_INDEX, packageName,
                           TCL_GLOBAL_ONLY);
    if (dataStr == NULL) {
        TclX_AppendObjResult (interp, "entry not found in \"auto_pkg_index \"",
                              "for package \"", packageName, "\"",
                              (char *) NULL);
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

    if (!TclX_StrToOffset (pkgDataArgv [1], 0, offsetPtr))
        goto invalidEntry;
    if (!TclX_StrToUnsigned (pkgDataArgv [2], 0, lengthPtr))
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
        ckfree ((char *) pkgDataArgv);
    Tcl_ResetResult (interp);
    TclX_AppendObjResult (interp, "invalid entry in \"auto_pkg_index \"",
                          "for package \"", packageName, "\"", (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
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
    Tcl_DStringAppendElement (&command, "auto_load_pkg");
    Tcl_DStringAppendElement (&command, package);

    result = Tcl_SetVar2 (interp, AUTO_INDEX, procName, command.string,
                          TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);

    Tcl_DStringFree (&command);

    return (result == NULL) ? TCL_ERROR : TCL_OK;
}

/*-----------------------------------------------------------------------------
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


/*-----------------------------------------------------------------------------
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
    Tcl_Channel  indexChannel = NULL;
    Tcl_DString  lineBuffer;
    int          lineArgc, idx, result;
    char       **lineArgv = NULL;

    Tcl_DStringInit (&lineBuffer);

    indexChannel = Tcl_OpenFileChannel (interp, tndxFilePath, "r", 0);
    if (indexChannel == NULL)
        return TCL_ERROR;
    
    while (TRUE) {
        Tcl_DStringFree (&lineBuffer);
        if (Tcl_Gets (indexChannel, &lineBuffer) < 0) {
            if (Tcl_Eof (indexChannel))
                goto reachedEOF;
            else
                goto fileError;
        }

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
        ckfree ((char *) lineArgv);
        lineArgv = NULL;
    }

  reachedEOF:
    Tcl_DStringFree (&lineBuffer);    
    if (Tcl_Close (NULL, indexChannel) != TCL_OK)
        goto fileError;

    return TCL_OK;

    /*
     * Handle format error in library input line.
     */
  formatError:
    Tcl_ResetResult (interp);
    TclX_AppendObjResult (interp, "format error in library index \"",
                          tndxFilePath, "\" (", lineBuffer.string, ")",
                          (char *) NULL);
    goto errorExit;


  fileError:
    TclX_AppendObjResult (interp, "error accessing package index file \"",
                          tndxFilePath, "\": ", Tcl_PosixError (interp),
                          (char *) NULL);
    goto errorExit;

    /*
     * Error exit here, releasing resources and closing the file.
     */
  errorExit:
    if (lineArgv != NULL)
        ckfree ((char *) lineArgv);
    Tcl_DStringFree (&lineBuffer);
    if (indexChannel != NULL)
        Tcl_Close (NULL, indexChannel);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * BuildPackageIndex --
 *
 * Call the "buildpackageindex" Tcl procedure to rebuild a package index.
 * This is found in the directory pointed to by the $tclx_library variable.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o tlibFilePath (I) - Absolute path name to the library file.
 * Returns:
 *   TCL_OK or TCL_ERROR.
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

    Tcl_DStringAppend (&command, "source [file join $tclx_library buildidx.tcl];", -1);
    Tcl_DStringAppend (&command, "buildpackageindex ", -1);
    Tcl_DStringAppend (&command, tlibFilePath, -1);

    result = Tcl_GlobalEval (interp, command.string);

    Tcl_DStringFree (&command);

    if (result == TCL_ERROR)
        return TCL_ERROR;
    Tcl_ResetResult (interp);
    return result;
}

/*-----------------------------------------------------------------------------
 * LoadPackageIndex --
 *
 * Load a package .tndx file.  Rebuild .tlib if non-existant or out of
 * date.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o tlibFilePath (I) - Absolute path name to the library file.
 *   o indexNameClass (I) - TCLLIB_TNDX if the index file should the suffix
 *     ".tndx" or TCLLIB_TND if it should have ".tnd".
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
LoadPackageIndex (interp, tlibFilePath, indexNameClass)
    Tcl_Interp       *interp;
    char             *tlibFilePath;
    indexNameClass_t  indexNameClass;
{
    Tcl_DString tndxFilePath;

    struct stat  tlibStat;
    struct stat  tndxStat;

    Tcl_DStringInit (&tndxFilePath);

    /*
     * Modify library file path to be the index file path.
     */
    Tcl_DStringAppend (&tndxFilePath, tlibFilePath, -1);
    tndxFilePath.string [tndxFilePath.length - 3] = 'n';
    tndxFilePath.string [tndxFilePath.length - 2] = 'd';
    if (indexNameClass == TCLLIB_TNDX)
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

/*-----------------------------------------------------------------------------
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
 * on the search path.  The function sources $tclx_library/loadouster.tcl
 * if it has not been loaded before.
 *-----------------------------------------------------------------------------
 */
static int
LoadOusterIndex (interp, indexFilePath)
     Tcl_Interp *interp;
     char       *indexFilePath;
{
    Tcl_DString  command;
    Tcl_CmdInfo  loadCmdInfo;
    
    Tcl_DStringInit (&command);

    /*
     * Check if "auto_load_ouster_index" is defined, if its not, source
     * the file that defines it.
     */
    if (!Tcl_GetCommandInfo (interp,
                             "auto_load_ouster_index",
                             &loadCmdInfo)) {
        if (Tcl_GlobalEval (interp, loadOusterCmd) == TCL_ERROR)
            goto errorExit;
    }

    /*
     * Now, actually do the load.
     */
    Tcl_DStringAppendElement (&command, "auto_load_ouster_index");
    Tcl_DStringAppendElement (&command, indexFilePath);

    if (Tcl_GlobalEval (interp, command.string) == TCL_ERROR)
        goto errorExit;

    Tcl_DStringFree (&command);
    Tcl_ResetResult (interp);
    return TCL_OK;

  errorExit:
    AddLibIndexErrorInfo (interp, indexFilePath);
    Tcl_DStringFree (&command);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * LoadDirIndexCallback --
 *
 *   Function called for every directory entry for LoadDirIndexes.
 *
 * Parameters
 *   o interp (I) - Interp is passed though.
 *   o path (I) - Normalized path to directory.
 *   o fileName (I) - Tcl normalized file name in directory.
 *   o caseSensitive (I) - Are the file names case sensitive?  Always
 *     TRUE on Unix.
 *   o clientData (I) - Pointer to a boolean that is set to TRUE if an error
 *     occures while porocessing the index file.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
LoadDirIndexCallback (interp, path, fileName, caseSensitive, clientData)
    Tcl_Interp  *interp;
    char        *path;
    char        *fileName;
    int          caseSensitive;
    ClientData   clientData;
{
    int *indexErrorPtr = (int *) clientData;
    int nameLen;
    char *chkName;
    indexNameClass_t indexNameClass;
    Tcl_DString chkNameBuf, filePath;

    /*
     * If the volume not case sensitive, convert the name to lower case.
     */
    Tcl_DStringInit (&chkNameBuf);
    chkName = fileName;
    if (!caseSensitive) {
        chkName = Tcl_DStringAppend (&chkNameBuf, fileName, -1);
        TclX_DownShift (chkName, chkName);
    }

    /*
     * Figure out if its an index file.
     */
    nameLen = strlen (chkName);
    if ((nameLen > 5) && STREQU (chkName + nameLen - 5, ".tlib")) {
        indexNameClass = TCLLIB_TNDX;
    } else if ((nameLen > 4) && STREQU (chkName + nameLen - 4, ".tli")) {
        indexNameClass = TCLLIB_TND;
    } else if ((caseSensitive && STREQU (chkName, "tclIndex")) ||
               ((!caseSensitive) && STREQU (chkName, "tclindex"))) {
        indexNameClass = TCLLIB_OUSTER;
    } else {
        Tcl_DStringFree (&chkNameBuf);
        return TCL_OK;  /* Not an index, skip */
    }
    Tcl_DStringFree (&chkNameBuf);

    /*
     * Assemble full path to library file.
     */
    Tcl_DStringInit (&filePath);
    TclX_JoinPath (path, fileName, &filePath);

    /*
     * Skip index it can't be accessed.
     */
    if (access (filePath.string, R_OK) < 0)
        goto exitPoint;

    /*
     * Process the index according to its type.
     */
    if (indexNameClass == TCLLIB_OUSTER) {
        if (LoadOusterIndex (interp, filePath.string) != TCL_OK)
            goto errorExit;
    } else {
        if (LoadPackageIndex (interp, filePath.string,
                              indexNameClass) != TCL_OK)
            goto errorExit;
    }

  exitPoint:
    Tcl_DStringFree (&filePath);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&filePath);
    *indexErrorPtr = TRUE;
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
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
    int indexError = FALSE;

    /*
     * This is a little tricky.  We want to skip directories we can't read,
     * read, but if we get an error processing an index, we want
     * to report it.  A boolean is passed in to indicate if the error
     * returned involved parsing the file.
     */
    if (TclXOSWalkDir (interp, dirName, FALSE, /* hidden */
                       LoadDirIndexCallback,
                       (ClientData) &indexError) == TCL_ERROR) {
        if (!indexError) {
            Tcl_ResetResult (interp);
            return TCL_OK;
        }
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
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
        TclX_AppendObjResult (interp, "recursive load of indexes ",
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

    ckfree ((char *) pathArgv);
    infoPtr->doingIdxSearch = FALSE;
    return result;
}

/*-----------------------------------------------------------------------------
 * TclX_Auto_load_pkgObjCmd --
 *
 *   Implements the command:
 *      auto_load_pkg package
 *
 * Which is called to load a .tlib package who's index has already been loaded.
 *-----------------------------------------------------------------------------
 */
static int
TclX_Auto_load_pkgObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj    *CONST objv[];
{
    char     *fileName;
    off_t     offset;
    unsigned  length;
    int       result;

    if (objc != 2) {
        return TclX_WrongArgs (interp, objv [0], "package");
    }

    if (GetPackageIndexEntry (interp,
                              Tcl_GetStringFromObj (objv [1], NULL),
                              &fileName, &offset, &length) != TCL_OK)
        return TCL_ERROR;

    result = EvalFilePart (interp, fileName, offset, length);
    ckfree (fileName);

    return result;
}

/*-----------------------------------------------------------------------------
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
        TclX_AppendObjResult (interp, "recursive auto_load of \"",
                              command, "\"", (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
 * LoadChangedPathPackageIndexes --
 *
 *   Determine what part of the path has changed and load the package indexes
 * just for the appended direcories.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o infoPtr (I) - Interpreter specific library info.
 *   o oldPath (I) - The previous value of auto_path.
 *   o path (I) - The current value of auto_path.
 * Returns:
 *   TCL_OK or TCL_ERROR
 *-----------------------------------------------------------------------------
 */
static int
LoadChangedPathPackageIndexes (interp, infoPtr, oldPath, path)
    Tcl_Interp  *interp;
    libInfo_t   *infoPtr;
    char        *oldPath;
    char        *path;
{
    char   *changedPath = NULL;
    int     pathArgc, oldPathArgc, idx;
    char  **pathArgv = NULL, **oldPathArgv = NULL;

    if (Tcl_SplitList (interp, oldPath, &oldPathArgc, &oldPathArgv) != TCL_OK)
        goto errorExit;
    if (Tcl_SplitList (interp, path, &pathArgc, &pathArgv) != TCL_OK)
        goto errorExit;

    for (idx = 0; (idx < oldPathArgc) && (idx < pathArgc); idx++) {
        if (!STREQU (oldPathArgv [idx], pathArgv [idx]))
            break;
    }
    changedPath = Tcl_Merge (pathArgc - idx, &pathArgv [idx]);

    if (LoadPackageIndexes (interp, infoPtr, changedPath) != TCL_OK)
        goto errorExit;

    ckfree (changedPath);
    ckfree ((char *) pathArgv);
    ckfree ((char *) oldPathArgv);
    return TCL_OK;

  errorExit:
    if (changedPath != NULL)
        ckfree (changedPath);
    if (pathArgv != NULL)
        ckfree ((char *) pathArgv);
    if (oldPathArgv != NULL)
        ckfree ((char *) oldPathArgv);
    return TCL_ERROR;

}

/*-----------------------------------------------------------------------------
 * LoadAutoPath --
 *
 *   Load all indexs on the auto_path variable.  If auto_path has not changed
 * or has just been appended to, since the last time libraries were
 * successfully loaded, this is a no-op.
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
    char   *path, *oldPath;

    path = Tcl_GetVar (interp, AUTO_PATH, TCL_GLOBAL_ONLY);
    if (path == NULL)
        return TCL_OK;

    oldPath = Tcl_GetVar (interp, AUTO_OLDPATH, TCL_GLOBAL_ONLY);

    /*
     * Check if the path has not changed at all.
     */
    if ((oldPath != NULL) && STREQU (path, oldPath))
        return TCL_OK;

    /*
     * If the old path exists, then load only the appended directories.
     * Otherwise, load the entire path.
     */
    if (oldPath != NULL) {
        if (LoadChangedPathPackageIndexes (interp, infoPtr, oldPath,
                                           path) != TCL_OK)
            return TCL_ERROR;
    } else {
        if (LoadPackageIndexes (interp, infoPtr, path) != TCL_OK)
            return TCL_ERROR;
    }

    if (Tcl_SetVar (interp, AUTO_OLDPATH, path,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
        return TCL_ERROR;

    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * LoadStdProc --
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
LoadStdProc (interp, command)
    Tcl_Interp  *interp;
    char        *command;
{
    char *loadCmd;

    loadCmd = Tcl_GetVar2 (interp, AUTO_INDEX, command, TCL_GLOBAL_ONLY);
    if (loadCmd == NULL)
        return TCL_CONTINUE;   /* Not found */

    if (Tcl_GlobalEval (interp, loadCmd) == TCL_ERROR)
        return TCL_ERROR;
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * LoadITclImportProc --
 *
 *   Check if the specified command is an auto load ITcl imported proc.  If so,
 * evaluated the load command it.  Don't call if namespaces are not available.
 *
 * Parameters
 *   o interp (I) - A pointer to the interpreter, error returned in result.
 *   o command (I) - The command to load.
 * Returns:
 *   TCL_OK if the command was loaded.
 *   TCL_CONTINUE if the command was not found.
 *   TCL_ERROR if an error occured.
 * FIX: Port to 8.0 namespaces/Itcl
 *-----------------------------------------------------------------------------
 */
static int
LoadITclImportProc (interp, command)
    Tcl_Interp  *interp;
    char        *command;
{
    static char    importCmd [] = "import all";
    Tcl_Obj       *cmdResult = NULL;
    Tcl_DString    fullName;
    Tcl_Obj      **nsSearchPathv;
    int            nsSearchPathc, idx, nameSpaceLen, searchPathLen;
    char          *searchPathStr, *nameSpace, *nextPtr; 
    char          *loadCmd = NULL;

    Tcl_DStringInit (&fullName);

    /*
     * Get the namespace search path.
     */
    if (Tcl_Eval (interp, importCmd) == TCL_ERROR)
        goto errorExit;

    cmdResult = Tcl_GetObjResult (interp);
    Tcl_IncrRefCount (cmdResult);
    
    if (Tcl_ListObjGetElements (interp, cmdResult,
                                &nsSearchPathc,
                                &nsSearchPathv) == TCL_ERROR)
        goto errorExit;

    /*
     * Search the path for the name.
     */
    for (idx = 0; idx < nsSearchPathc; idx++) {
        Tcl_DStringSetLength (&fullName, 0);
        searchPathStr = Tcl_GetStringFromObj (nsSearchPathv [idx], 
                                              &searchPathLen);
        /*FIX: Use list object  */
        if (TclFindElement (interp,
                            searchPathStr,
                            searchPathLen,
                            &nameSpace,
                            &nextPtr,
                            &nameSpaceLen,
                            NULL))
            goto errorExit;

        nameSpace [nameSpaceLen] = '\0';
        if (!STREQU (nameSpace, "::")) {
            Tcl_DStringAppend (&fullName, nameSpace, -1);
        }
        Tcl_DStringAppend (&fullName, "::", -1);
        Tcl_DStringAppend (&fullName, command, -1);
        loadCmd = Tcl_GetVar2 (interp, AUTO_INDEX,
                               fullName.string, TCL_GLOBAL_ONLY);
	if ((loadCmd == NULL) && (fullName.string[0] == ':') &&
		(fullName.string[1] == ':')) {
	    loadCmd = Tcl_GetVar2 (interp, AUTO_INDEX,
		    fullName.string+2, TCL_GLOBAL_ONLY);
	}
        if (loadCmd != NULL)
            break;
    }

    if (loadCmd == NULL) {
        Tcl_DecrRefCount (cmdResult);
        Tcl_DStringFree (&fullName);
        return TCL_CONTINUE;   /* Not found */
    }

    if (Tcl_GlobalEval (interp, loadCmd) == TCL_ERROR)
        goto errorExit;

    Tcl_DecrRefCount (cmdResult);
    Tcl_DStringFree (&fullName);
    return TCL_OK;

    
  errorExit:
    if (cmdResult != NULL)
        Tcl_DecrRefCount (cmdResult);
    Tcl_DStringFree (&fullName);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * LoadCommand --
 *
 *   Check the "auto_index" array for code to load a command and eval it.  Also
 * check ITcl import list.
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
    int result;
    Tcl_CmdInfo cmdInfo;

    result = LoadStdProc (interp, command);
    if (haveNameSpaces && (result == TCL_CONTINUE))
        result = LoadITclImportProc (interp, command);
    if (result == TCL_CONTINUE)
        return TCL_CONTINUE;
    if (result == TCL_ERROR)
        return TCL_ERROR;

    Tcl_ResetResult (interp);

    if (Tcl_GetCommandInfo (interp, command, &cmdInfo))
        return TCL_OK;  /* Found and loaded */

    TclX_AppendObjResult (interp, "command \"", command,
                          "\" was defined in a Tcl library index, ",
                          "but not in a Tcl library", (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_LoadlibindexObjCmd --
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
TclX_LoadlibindexObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj    *CONST objv[];
{
    char        *pathName;
    Tcl_DString  pathNameBuf;
    int          pathLen;

    Tcl_DStringInit (&pathNameBuf);

    if (objc != 2) {
        return TclX_WrongArgs (interp, objv [0], "libFile");
    }

    pathName = MakeAbsFile (interp,
                            Tcl_GetStringFromObj (objv [1], NULL),
                            &pathNameBuf);
    if (pathName == NULL)
        return TCL_ERROR;

    /*
     * Find the length of the directory name. Validate that we have a .tlib
     * extension or file name is "tclIndex" and call the routine to process
     * the specific type of index.
     */
    pathLen = strlen (pathName);

    if ((pathLen > 5) && STREQU (pathName + pathLen - 5, ".tlib")) {
        if (LoadPackageIndex (interp, pathName, TCLLIB_TNDX) != TCL_OK)
            goto errorExit;
    } else if ((pathLen > 4) && STREQU (pathName + pathLen - 4, ".tli")) {
        if (LoadPackageIndex (interp, pathName, TCLLIB_TND) != TCL_OK)
            goto errorExit;
    } else if (STREQU (pathName + pathLen - 9, "/tclIndex") ||
               STREQU (pathName + pathLen - 9, "/tclindex")) {
        /* FIX: Should we check for DOS before allowing lower case???*/
        if (LoadOusterIndex (interp, pathName) != TCL_OK)
            goto errorExit;
    } else {
        TclX_AppendObjResult (interp, "invalid library name, must have an ",
                              " extension of \".tlib\", \".tli\" or the name ",
                              "\"tclIndex\", got \"",
                              Tcl_GetStringFromObj (objv [1], NULL), "\"",
                              (char *) NULL);
        goto errorExit;
    }

    Tcl_DStringFree (&pathNameBuf);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&pathNameBuf);
    return TCL_ERROR;;
}

/*-----------------------------------------------------------------------------
 * TclX_auto_loadObjCmd --
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
TclX_Auto_loadObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj    *CONST objv[];
{
    libInfo_t *infoPtr = (libInfo_t *) clientData;
    int result;
    char *msg, *command;

    if (objc > 2) {
        return TclX_WrongArgs (interp, objv [0], "?command?");
    }

    /*
     * If no command is specified, just load the indexs.
     */
    if (objc == 1) {
        return LoadAutoPath (interp, infoPtr);
    }

    /*
     * Do checking for recursive auto_load of the same command.
     */
    command = Tcl_GetStringFromObj (objv [1], NULL);
    if (AddInProgress (interp, infoPtr, command) != TCL_OK)
        return TCL_ERROR;

    /*
     * First, attempt to load it from the indexes in memory.
     */
    result = LoadCommand (interp, command);
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
    result = LoadCommand (interp, command);
    if (result == TCL_ERROR)
        goto errorExit;
    if (result != TCL_OK)
        goto notFound;

  found:
    RemoveInProgress (interp, infoPtr, command);
    Tcl_SetBooleanObj (Tcl_GetObjResult (interp), TRUE);
    return TCL_OK;

  notFound:
    RemoveInProgress (interp, infoPtr, command);
    Tcl_SetBooleanObj (Tcl_GetObjResult (interp), FALSE);
    return TCL_OK;

  errorExit:
    msg = ckalloc (strlen (command) + 35);
    strcpy (msg, "\n    while auto loading \"");
    strcat (msg, command);
    strcat (msg, "\"");
    Tcl_AddObjErrorInfo (interp, msg, -1);
    ckfree (msg);

    RemoveInProgress (interp, infoPtr, command);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
 * TclX_LibraryInit --
 *
 *   Initialize the Extended Tcl library facility commands.
 *-----------------------------------------------------------------------------
 */
void
TclX_LibraryInit (interp)
    Tcl_Interp *interp;
{
    libInfo_t *infoPtr;
    Tcl_CmdInfo cmdInfo;

    infoPtr = (libInfo_t *) ckalloc (sizeof (libInfo_t));

    Tcl_InitHashTable (&infoPtr->inProgressTbl, TCL_STRING_KEYS);
    infoPtr->doingIdxSearch = FALSE;

    Tcl_CallWhenDeleted (interp, TclLibCleanUp, (ClientData) infoPtr);

    Tcl_CreateObjCommand (interp, "auto_load_pkg",
                          TclX_Auto_load_pkgObjCmd,
                          (ClientData) infoPtr,
                          (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateObjCommand (interp, "auto_load",
                          TclX_Auto_loadObjCmd,
                          (ClientData) infoPtr,
                          (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateObjCommand (interp, "loadlibindex",
                          TclX_LoadlibindexObjCmd,
                          (ClientData) infoPtr,
                          (Tcl_CmdDeleteProc*) NULL);

    Tcl_ResetResult (interp);
                                                                  

    /*
     * Check for ITcl namespaces.
     */
    haveNameSpaces = Tcl_GetCommandInfo (interp, 
                                         nameSpaceCheckCommand,
                                         &cmdInfo);
}
