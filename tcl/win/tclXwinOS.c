/*
 * tclXwinOS.c --
 *
 * OS portability interface for Windows systems.  The idea behind these
 * functions is to provide interfaces to various functions that vary on the
 * various platforms.  These functions either implement the call in a manner
 * approriate to the platform or return an error indicating the functionality
 * is not available on that platform.  This results in code with minimal
 * number of #ifdefs.
 *-----------------------------------------------------------------------------
 * Copyright 1996-1996 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXwinOS.c,v 7.6 1996/08/04 18:21:26 markd Exp $
 *-----------------------------------------------------------------------------
 * The code for reading directories is based on TclMatchFiles from the Tcl
 * distribution file win/tclWinFile.c
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static HANDLE
ChannelToHandle(Tcl_Channel channel,
                int         direction,
                int        *type);

static SOCKET
ChannelToSocket (Tcl_Interp  *interp
                 Tcl_Channel  channel)

static time_t
ConvertToUnixTime (FILETIME fileTime);


/*-----------------------------------------------------------------------------
 * TclXNotAvailableError --
 *   Return an error about functionality not being available under Windows.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o funcName - Command or other name to use in not available error.
 * Returns:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXNotAvailableError (Tcl_Interp *interp,
                       char       *funcName)
{
    Tcl_AppendResult (interp, funcName, " is not available on MS Windows",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_SplitWinCmdLine --
 *   Parse the window command line into arguments.
 *
 * Parameters:
 *   o argcPtr - Count of arguments is returned here.
 *   o argvPtr - Argument vector is returned here.
 * Notes:
 *   This code taken from the Tcl file tclAppInit.c: Copyright (c) 1996 by
 * Sun Microsystems, Inc.
 *-----------------------------------------------------------------------------
 */
void
TclX_SplitWinCmdLine (int    *argcPtr,
                      char ***argvPtr)
{
    char *args = GetCommandLine();
    char **argvlist, *p;
    int size, i;

    /*
     * Precompute an overly pessimistic guess at the number of arguments
     * in the command line by counting non-space spans.
     */
    for (size = 2, p = args; *p != '\0'; p++) {
        if (isspace (*p)) {
            size++;
            while (isspace (*p)) {
                p++;
            }
            if (*p == '\0') {
                break;
            }
        }
    }
    argvlist = (char **) ckalloc ((unsigned) (size * sizeof (char *)));
    *argvPtr = argvlist;

    /*
     * Parse the Windows command line string.  If an argument begins with a
     * double quote, then spaces are considered part of the argument until the
     * next double quote.  The argument terminates at the second quote.  Note
     * that this is different from the usual Unix semantics.
     */
    for (i = 0, p = args; *p != '\0'; i++) {
        while (isspace (*p)) {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (*p == '"') {
            p++;
            (*argvPtr) [i] = p;
            while ((*p != '\0') && (*p != '"')) {
                p++;
            }
        } else {
            (*argvPtr) [i] = p;
            while (*p != '\0' && !isspace(*p)) {
                p++;
            }
        }
        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }
    (*argvPtr) [i] = NULL;
    *argcPtr = i;
}

/*-----------------------------------------------------------------------------
 * ChannelToHandle --
 *
 *    Convert a channel to a handle.
 *
 * Parameters:
 *   o channel - Channel to get file number for.
 *   o direction - TCL_READABLE or TCL_WRITABLE, or zero.  If zero, then
 *     return the first of the read and write numbers.
 *   o type - The type of the file. TCL_WIN_FILE if an error occurs
 *     (so something is in the value).  Maybe NULL.
 * Returns:
 *   The file number or NULL if a file number is not associated with this
 * access direction.  Just go ahead and pass this to a system call to
 * get a useful error message, they should never happen.  If type is 
 * TCL_WIN_SOCK, the return value is not valid.
 *-----------------------------------------------------------------------------
 */
HANDLE
ChannelToHandle (Tcl_Channel channel,
                 int         direction,
                 int        *type)
{
    Tcl_File file;
    
    if (direction == 0) {
        file = Tcl_GetChannelFile (channel, TCL_READABLE);
        if (file == NULL)
            file = Tcl_GetChannelFile (channel, TCL_WRITABLE);
    } else {
        file = Tcl_GetChannelFile (channel, direction);
        if (file == NULL) {
            if (type != NULL)
                *type = TCL_WIN_FILE;
            return NULL;
        }
    }
    return (HANDLE) Tcl_GetFileInfo (file, type);
}

/*-----------------------------------------------------------------------------
 * ChannelToSocket --
 *
 *    Convert a channel to a socket.
 *
 * Parameters:
 *   o interp - An error is returned if the channel is not a socket.
 *   o channel - Channel to get file number for.
 * Returns:
 *   The socked number or -1 if an error occurs.
 *-----------------------------------------------------------------------------
 */
static SOCKET
ChannelToSocket (Tcl_Interp  *interp
                 Tcl_Channel  channel)
{
    Tcl_File file;
    SOCKET sock;
    int type;
    
    file = Tcl_GetChannelFile (channel, TCL_READABLE);
    if (file == NULL)
        file = Tcl_GetChannelFile (channel, TCL_WRITABLE);
    sock = (SOCKET) Tcl_GetFileInfo (file, &type);
    if (type != TCL_WIN_SOCK) {
        Tcl_AppendResult (interp, "channel \"", Tcl_GetChannelName (channel),
                          "\" is not a socket", (char *) NULL);
        return -1;

    }
    return sock;}

/*-----------------------------------------------------------------------------
 * ConvertToUnixTime --
 *
 *    Convert a FILETIME structure to Unix style time.
 *
 * Parameters:
 *   o fileTime - Time to convert.
 * Returns:
 *   Unix time: seconds since Jan 1, 1970.
 *-----------------------------------------------------------------------------
 */
static time_t
ConvertToUnixTime (FILETIME fileTime)
{
    /* FIX: Write me */
    return 0;
}

/*-----------------------------------------------------------------------------
 * TclXOSgetpriority --
 *   Portability interface to getpriority functionality, which is not available
 * on windows.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o priority - Process priority is returned here.
 *   o funcName - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSgetpriority (Tcl_Interp *interp,
                   int        *priority,
                   char       *funcName)
{
    /*FIX: this should work */
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSincrpriority--
 *   Portability interface to increment or decrement the current priority,
 * which is not available on windows.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o priorityIncr - Amount to adjust the priority by.
 *   o priority - The new priority..
 *   o funcName - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSincrpriority (Tcl_Interp *interp,
                    int         priorityIncr,
                    int        *priority,
                    char       *funcName)
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSpipe --
 *   Portability interface to pipe.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o fildes - Array to return file descriptors in.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSpipe (Tcl_Interp *interp,
            int        *fildes)
{
    if (_pipe (fildes, 16384, 0) < 0) {
        Tcl_AppendResult (interp, "pipe creation failed: ",
                          Tcl_PosixError (interp));
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*-----------------------------------------------------------------------------
 * TclXOSsetitimer --
 *   Portability interface to setitimer functionality, which is not available
 * on windows.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o seconds (I/O) - Seconds to pause for, it is updated with the time
 *     remaining on the last alarm.
 *   o funcName - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSsetitimer (Tcl_Interp *interp,
                 double      *seconds,
                 char       *funcName)
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSsleep --
 *   Portability interface to sleep functionallity.
 *
 * Parameters:
 *   o seconds - Seconds to sleep.
 *-----------------------------------------------------------------------------
 */
void
TclXOSsleep (unsigned seconds)
{
    Sleep (seconds*100);
}

/*-----------------------------------------------------------------------------
 * TclXOSsync --
 *   Portability interface to sync functionality.
 *-----------------------------------------------------------------------------
 */
void
TclXOSsync ()
{
    _flushall ();
}

/*-----------------------------------------------------------------------------
 * TclXOSfsync --
 *   Portability interface to fsync functionallity.  Does a _flushall, since
 * fsync is no available.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o channel - The channel to sync.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSfsync (Tcl_Interp *interp,
             Tcl_Channel channel)
{
    if (Tcl_Flush (channel) < 0)
        goto posixError;

    _flushall ();
    return TCL_OK;

  posixError:
    Tcl_AppendResult (interp, Tcl_PosixError (interp), (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclXOSsystem --
 *   Portability interface to system functionallity (executing a command
 * with the standard system shell).
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o command - Command to execute.
 *   o exitCode - Exit code of the child process.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSsystem (Tcl_Interp *interp,
              char       *command,
              int        *exitCode)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    BOOL bSuccess;

    memset (&si, 0, sizeof (si));

    bSuccess = CreateProcess (command,
                              NULL, NULL, NULL,
                              0,
                              CREATE_NEW_PROCESS_GROUP,
                              NULL, NULL,
                              &si, &pi);
    if (!bSuccess) {
        Tcl_AppendResult (interp, "process creation failed",
                          (char *) NULL);
        return TCL_ERROR;
    }
    CloseHandle (pi.hThread);
    WaitForSingleObject (pi.hProcess, INFINITE);
    GetExitCodeProcess (pi.hProcess, exitCode);
    CloseHandle (pi.hProcess);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSmkdir --
 *   Portability interface to mkdir functionallity.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o path - Directory to create.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSmkdir (Tcl_Interp *interp,
             char       *path)
{
    if (mkdir (path) < 0) {
        Tcl_AppendResult (interp, "creating directory \"", path,
                          "\" failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_OSlink --
 *   Portability interface to link functionallity, which is not available
 * on windows.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o srcPath - File to link.
 *   o targetPath - Path to new link.
 *   o funcName - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSlink (Tcl_Interp *interp,
             char       *srcPath,
             char       *targetPath,
             char       *funcName)
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclX_OSsymlink --
 *   Portability interface to symlink functionallity, which is not available
 * on windows.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o srcPath - Value of symbolic link.
 *   o targetPath - Path to new symbolic link.
 *   o funcName - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSsymlink (Tcl_Interp *interp,
                char       *srcPath,
                char       *targetPath,
                char       *funcName)
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSElapsedTime --
 *   Portability interface to get the elapsed CPU and real time.  CPU time
 * is not available under windows and zero is always returned.
 *
 * Parameters:
 *   o realTime - Elapsed real time, in milliseconds is returned here.
 *   o cpuTime - Elapsed CPU time, zero is always returned.
 *-----------------------------------------------------------------------------
 */
void
TclXOSElapsedTime (clock_t *realTime,
                   clock_t *cpuTime)
{
#if 0
???
    static struct timeval startTime = {0, 0};
    struct timeval currentTime;
    struct tms cpuTimes;

    /*
     * If this is the first call, get base time.
     */
    if ((startTime.tv_sec == 0) && (startTime.tv_usec == 0))
        gettimeofday (&startTime, NULL);
    
    gettimeofday (&currentTime, NULL);
    currentTime.tv_sec  = currentTime.tv_sec  - startTime.tv_sec;
    currentTime.tv_usec = currentTime.tv_usec - startTime.tv_usec;
    *realTime = (currentTime.tv_sec  * 1000) + (currentTime.tv_usec / 1000);
    *cpuTime = 0;
#else
    *realTime = 0;
    *cpuTime = 0;
#endif
}

/*-----------------------------------------------------------------------------
 * TclXOSkill --
 *   Portability interface to functionallity, which is not available
 * on windows.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o pid - Process id, negative process group, etc.
 *   o signal - Signal to send.
 *   o funcName - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSkill (Tcl_Interp *interp,
            pid_t       pid,
            int         signal,
            char       *funcName)
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSGetOpenFileMode --
 *   Portability interface to get the accessability on an open file number.
 *
 * Parameters:
 *   o fileNum - Number to return permissions on.
 *   o mode - Set of TCL_READABLE and TCL_WRITABLE.
 *   o nonBlocking - Non-blocking mode, always returns FALSE under windows.
 * Results:
 *   TCL_OK or TCL_ERROR.  The Posix error number describes the error.
 *-----------------------------------------------------------------------------
 */
int
TclXOSGetOpenFileMode (int  fileNum,
                       int *mode,
                       int *nonBlocking)
{
    struct stat fileStat;

    /* FIX: Probably not correct, how does one determine if a file is open
       for read/write.  also, might want to determine if its a socket here.*/
    if (fstat (fileNum, &fileStat) < 0)
        return TCL_ERROR;
    *mode = 0;
    if (fileStat.st_mode & _S_IREAD)
        *mode = TCL_READABLE;
    if (fileStat.st_mode & _S_IWRITE)
        *mode |= TCL_WRITABLE;
    *nonBlocking = FALSE;
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSFstat --
 *   Portability interface to get status information on an open file.
 *
 * Parameters:
 *   o interp - Errors are returned in result.
 *   o channel - Channel to get file number for.
 *   o direction - TCL_READABLE or TCL_WRITABLE, or zero.  If zero, then
 *     return the first of the read and write numbers.
 *   o statBuf - Status information, made to look as much like Unix as
 *     possible.
 *   o ttyDev - If not NULL, a boolean indicating if the device is
 *     associated with a tty. (Always FALSE on windows).
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSFstat (Tcl_Interp  *interp,
             Tcl_Channel  channel,
             int          direction,
             struct stat *statBuf,
             int         *ttyDev)
{
    HANDLE handle;
    int type;
    FILETIME creation, access, modify;
    
    handle = ChannelToHandle (channel, direction, &type);

    /*
     * These don't translate to windows.
     */
    statBuf->st_dev = 0;
    statBuf->st_ino = 0;
    statBuf->st_rdev = 0;

    statBuf->st_mode = 0;
    switch (type) {
      case TCL_WIN_PIPE:
        statBuf->st_mode |= S_IFIFO;
        break;
      case TCL_WIN_FILE:
        statBuf->st_mode |= S_IFREG;
        break;
      case TCL_WIN_SOCKET:
        statBuf->st_mode |= S_IFSOCK;
        break;
      case TCL_WIN_CONSOLE:
        statBuf->st_mode |= S_IFCHR;
        break;
    }        

    statBuf->st_nlink = (type == TCL_WIN_FILE) ? 1 : 0;
    statBuf->st_uid = 0;   /* FIX??? */
    statBuf->st_gid = 0;

    switch (type) {
      case TCL_WIN_FILE:
      case TCL_WIN_PIPE:
        statBuf->st_size = GetFileSize (handle, NULL);
        if (statBuf->st_size < 0)
            goto winError;

        if (!GetFileTime (handle, &creation, &access, &modify)) {
            goto winError;
        }
        statBuf->st_atime = ConvertToUnixTime (creation);
        statBuf->st_mtime = ConvertToUnixTime (access);
        statBuf->st_ctime = ConvertToUnixTime (modify);
        break;

      case TCL_WIN_SOCKET:
      case TCL_WIN_CONSOLE:
        statBuf->st_size = 0;
        statBuf->st_atime = 0;
        statBuf->st_mtime = 0;
        statBuf->st_ctime = 0;
        break;
    }        

    if (ttyDev != NULL)
        *ttyDev = (type == TCL_WIN_CONSOLE) ? 1 : 0;
    return TCL_OK;

  winError:
    TclWinConvertError (GetLastError ());
    Tcl_AppendResult (interp, Tcl_PosixError (interp), (char *) NULL);
    return TCL_ERROR;

}

/*-----------------------------------------------------------------------------
 * TclXOSWalkDir --
 *   Portability interface to reading the contents of a directory.  The
 * specified directory is walked and a callback is called on each entry.
 * The "." and ".." entries are skipped.
 *
 * Parameters:
 *   o interp - Interp to return errors in.
 *   o path - Path to the directory.
 *   o hidden - Include hidden files.  Ignored on Unix.
 *   o callback - Callback function to call on each directory entry.
 *     It should return TCL_OK to continue processing, TCL_ERROR if an
 *     error occured and TCL_BREAK to stop processing.  The parameters are:
 *        o interp - Interp is passed though.
 *        o path - Normalized path to directory.
 *        o fileName - Tcl normalized file name in directory.
 *        o caseSensitive - Are the file names case sensitive?
 *        o clientData - Client data that was passed.
 *   o clientData - Client data to pass to callback.
 * Results:
 *   TCL_OK if completed directory walk.  TCL_BREAK if callback returned
 * TCL_BREAK and TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
int
TclXOSWalkDir (Tcl_Interp       *interp,
               char             *path,
               int               hidden,
               TclX_WalkDirProc *callback,
               ClientData        clientData)
{
    char drivePattern[4] = "?:\\";
    char *p, *dir, *root, c;
    int result = TCL_OK;
    Tcl_DString pathBuf;
    DWORD atts, volFlags;
    HANDLE handle;
    WIN32_FIND_DATA data;
    BOOL found;

    /*
     * Convert the path to normalized form since some interfaces only
     * accept backslashes.  Also, ensure that the directory ends with a
     * separator character.
     */
    Tcl_DStringInit (&pathBuf);
    Tcl_DStringAppend (&pathBuf, path, -1);
    if (Tcl_DStringLength (&pathBuf) == 0) {
        Tcl_DStringAppend (&pathBuf, ".", 1);
    }
    for (p = Tcl_DStringValue( &pathBuf); *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\\';
        }
    }
    p--;
    if (*p != '\\' && *p != ':') {
        Tcl_DStringAppend(&pathBuf, "\\", 1);
    }
    dir = Tcl_DStringValue(&pathBuf);
    
    /*
     * First verify that the specified path is actually a directory.
     */
    atts = GetFileAttributes (dir);
    if ((atts == 0xFFFFFFFF) || ((atts & FILE_ATTRIBUTE_DIRECTORY) == 0)) {
        Tcl_DStringFree (&pathBuf);
        return TCL_OK;
    }

    /*
     * Next check the volume information for the directory to see whether
     * comparisons should be case sensitive or not.  If the root is null, then
     * we use the root of the current directory.  If the root is just a drive
     * specifier, we use the root directory of the given drive.
     */
    switch (Tcl_GetPathType (dir)) {
      case TCL_PATH_RELATIVE:
        found = GetVolumeInformation (NULL, NULL, 0, NULL,
                                      NULL, &volFlags, NULL, 0);
        break;
      case TCL_PATH_VOLUME_RELATIVE:
        if (*dir == '\\') {
            root = NULL;
        } else {
            root = drivePattern;
            *root = *dir;
        }
        found = GetVolumeInformation (root, NULL, 0, NULL,
                                      NULL, &volFlags, NULL, 0);
        break;
      case TCL_PATH_ABSOLUTE:
        if (dir[1] == ':') {
            root = drivePattern;
            *root = *dir;
            found = GetVolumeInformation (root, NULL, 0, NULL,
                                          NULL, &volFlags, NULL, 0);
        } else if (dir[1] == '\\') {
            p = strchr(dir+2, '\\');
            p = strchr(p+1, '\\');
            p++;
            c = *p;
            *p = 0;
            found = GetVolumeInformation (dir, NULL, 0, NULL,
                                          NULL, &volFlags, NULL, 0);
            *p = c;
        }
        break;
    }

    if (!found) {
        Tcl_DStringFree (&pathBuf);
        TclWinConvertError (GetLastError ());
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "couldn't read volume information for \"",
                          path, "\": ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * We need to check all files in the directory, so append a *.*
     * to the path. 
     */
    dir = Tcl_DStringAppend (&pathBuf, "*.*", 3);

    /*
     * Now open the directory for reading and iterate over the contents.
     */
    handle = FindFirstFile (dir, &data);
    Tcl_DStringFree (&pathBuf);

    if (handle == INVALID_HANDLE_VALUE) {
        TclWinConvertError (GetLastError ());
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "couldn't read directory \"",
                          path, "\": ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Now iterate over all of the files in the directory.
     */
    for (found = 1; found; found = FindNextFile (handle, &data)) {
        /*
         * Ignore hidden files if not requested.
         */
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !hidden)
            continue;

        /*
         * Skip "." and "..".
         */
        if (STREQU (data.cFileName, ".") || STREQU (data.cFileName, ".."))
            continue;

        /*
         * Call the callback with this file.
         */
        result = (*callback) (interp, path, data.cFileName,
                              (volFlags & FS_CASE_SENSITIVE), clientData);
        if (!((result == TCL_OK) || (result == TCL_CONTINUE)))
            break;
    }

    Tcl_DStringFree (&pathBuf);
    FindClose (handle);
    return result;
}

/*-----------------------------------------------------------------------------
 * TclXOSGetFileSize --
 *   Portability interface to get the size of an open file.
 *
 * Parameters:
 *   o channel - Channel.
 *   o fileSize - File size is returned here.
 *   o direction - TCL_READABLE or TCL_WRITABLE, or zero.  If zero, then
 *     return the first of the read and write numbers.
 * Results:
 *   TCL_OK or TCL_ERROR.  A POSIX error will be set.
 *-----------------------------------------------------------------------------
 */
int
TclXOSGetFileSize (Tcl_Channel  channel,
                   int          direction,
                   off_t       *fileSize)
{
    HANDLE handle; 
    int type;

    handle = ChannelToHandle (channel, direction, &type);
    
    switch (type) {
      case TCL_WIN_PIPE:
      case TCL_WIN_FILE:
        *fileSize = GetFileSize (handle, NULL);
        if (*fileSize < 0) {
            TclWinConvertError (GetLastError ());
            return TCL_ERROR;
        }
        break;
      case TCL_WIN_SOCKET:
      case TCL_WIN_CONSOLE:
        *fileSize = 0;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSftruncate --
 *   Portability interface to ftruncate functionality. 
 *
 * Parameters:
 *   o interp - Error messages are returned in the interpreter.
 *   o channel - Channel to truncate.
 *   o newSize - Size to truncate the file to.
 *   o funcName - Command or other name to use in not available error.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSftruncate (Tcl_Interp  *interp,
                 Tcl_channel  channel,
                 off_t        newSize,
                 char        *funcName)
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSfork --
 *   Portability interface to fork functionallity.  Not supported on windows.
 *
 * Parameters:
 *   o interp - An error  is returned in result.
 *   o funcName - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSfork (Tcl_Interp *interp,
            char       *funcName)
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSexecl --
 *   Portability interface to execl functionallity.  On windows, this is the
 * equivlant of a fork and an execl, so a process id is returned.
 *
 * Parameters:
 *   o interp - A process id or errors are returned in result.
 *   o path - Path to the program.
 *   o argList - NULL terminated argument vector.
 * Results:
 *   TCL_ERROR or does not return.
 *-----------------------------------------------------------------------------
 */
int
TclXOSexecl (Tcl_Interp *interp,
             char       *path,
             char      **argList)
{
    int pid;
    char numBuf [32];

    pid = spawnvp (_P_NOWAIT , path, argList);
    if (pid == -1) {
        Tcl_AppendResult (interp, "exec of \"", path, "\" failed: ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }

    sprintf (numBuf, "%d", pid);
    Tcl_SetResult (interp, numBuf, TCL_VOLATILE);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSInetAtoN --
 *
 *   Convert an internet address to an "struct in_addr" representation.
 *
 * Parameters:
 *   o interp - If not NULL, an error message is return in the result.
 *     If NULL, no error message is generated.
 *   o strAddress - String address to convert.
 *   o inAddress - Converted internet address is returned here.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSInetAtoN (Tcl_Interp     *interp,
                char           *strAddress,
                struct in_addr *inAddress)
{
    inAddress->s_addr = inet_addr (strAddress);
    if (inAddress->s_addr != INADDR_NONE)
        return TCL_OK;
    if (interp != NULL) {
        Tcl_AppendResult (interp, "malformed address: \"",
                          strAddress, "\"", (char *) NULL);
    }
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclXOSgetpeername --
 *   Portability interface to getpeername functionallity.
 *
 * Parameters:
 *   o interp - Errors are returned in result.
 *   o channel - Channel associated with the socket.
 *   o sockaddr - Pointer to sockaddr structure.
 *   o sockaddrSize - Size of the sockaddr struct.
 * Results:
 *   TCL_OK or TCL_ERROR, sets a posix error.
 *-----------------------------------------------------------------------------
 */
int
TclXOSgetpeername (Tcl_Interp *interp,
                   Tcl_Channel channel,
                   void       *sockaddr,
                   int         sockaddrSize)
{
    SOCKET sock;

    sock = ChannelToSocket (interp, channel);
    if (sock == -1)
        return TCL_ERROR;
    if (getpeername (sock, (struct sockaddr *) sockaddr, &sockaddrSize) < 0) {
        Tcl_AppendResult (interp, Tcl_GetChannelName (channel), ": ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSgetsockname --
 *   Portability interface to getsockname functionallity.
 *
 * Parameters:
 *   o interp - Errors are returned in result.
 *   o channel - Channel associated with the socket.
 *   o sockaddr - Pointer to sockaddr structure.
 *   o sockaddrSize - Size of the sockaddr struct.
 * Results:
 *   TCL_OK or TCL_ERROR, sets a posix error.
 *-----------------------------------------------------------------------------
 */
int
TclXOSgetsockname (Tcl_Interp *interp,
                   Tcl_Channel channel,
                   void       *sockaddr,
                   int         sockaddrSize)
{
    SOCKET sock;

    sock = ChannelToSocket (interp, channel);
    if (sock == -1)
        return TCL_ERROR;

    if (getsockname (sock, (struct sockaddr *) sockaddr, &sockaddrSize) < 0) {
        Tcl_AppendResult (interp, Tcl_GetChannelName (channel), ": ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSgetsockopt --
 *    Get the value of a integer socket option.
 *     
 * Parameters:
 *   o interp - Errors are returned in the result.
 *   o channel - Channel associated with the socket.
 *   o option - Socket option to get.
 *   o valuePtr -  Integer value is returned here.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSgetsockopt (interp, channel, option, valuePtr)
    Tcl_Interp  *interp;
    Tcl_Channel  channel;
    int          option;
    int         *valuePtr;
{
    int valueLen = sizeof (*valuePtr);
    SOCKET sock;

    sock = ChannelToSocket (interp, channel);
    if (sock == -1)
        return TCL_ERROR;

    if (getsockopt (sock, SOL_SOCKET, option, 
                    (void*) valuePtr, &valueLen) != 0) {
        Tcl_AppendResult (interp, Tcl_GetChannelName (channel), ": ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSsetsockopt --
 *    Set the value of a integer socket option.
 *     
 * Parameters:
 *   o interp - Errors are returned in the result.
 *   o channel - Channel associated with the socket.
 *   o option - Socket option to get.
 *   o value - Valid integer value for the option.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSsetsockopt (interp, channel, option, value)
    Tcl_Interp  *interp;
    Tcl_Channel  channel;
    int          option;
    int          value;
{
    int valueLen = sizeof (value);
    SOCKET sock;

    sock = ChannelToSocket (interp, channel);
    if (sock == -1)
        return TCL_ERROR;

    if (setsockopt (sock, SOL_SOCKET, option,
                    (void*) &value, valueLen) != 0) {
        Tcl_AppendResult (interp, Tcl_GetChannelName (channel), ": ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSchmod --
 *   Portability interface to chmod functionallity.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o fileName - Name of to set the mode on.
 *   o mode - New, unix style file access mode.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSchmod (interp, fileName, mode)
    Tcl_Interp *interp;
    char       *fileName;
    int         mode;
{
#if 0
  FIX:
    if (chmod (fileName, (unsigned short) mode) < 0) {
        Tcl_AppendResult (interp, "chmod failed on \"", fileName, "\": ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
#else
    Tcl_AppendResult (interp, funcName, " is not available on this system",
                      (char *) NULL);
    return TCL_ERROR;
#endif
}

/*-----------------------------------------------------------------------------
 * TclXOSfchmod --
 *   Portability interface to fchmod functionallity.
 *
 * Parameters:
 *   o interp - Errors returned in result.
 *   o channel - Channel to set the mode on.
 *   o mode - New, unix style file access mode.
 *   o funcName - Command or other string to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSfchmod (interp, channel, mode, funcName)
    Tcl_Interp *interp;
    Tcl_Channel channel;
    int         mode;
    char       *funcName;
{
#if 0
  FIX:
    if (fchmod (ChannelToFnum (channel, 0), (unsigned short) mode) < 0) {
        Tcl_AppendResult (interp, Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
#else
    Tcl_AppendResult (interp, funcName, " is not available on this system",
                      (char *) NULL);
    return TCL_ERROR;
#endif
}

/*-----------------------------------------------------------------------------
 * TclXOSChangeOwnGrp --
 *   Change the owner and/or group of a file by file name.
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o options - Option flags are:
 *     o TCLX_CHOWN - Change file's owner.
 *     o TCLX_CHGRP - Change file's group.
 *   o ownerStr - String containing owner name or id.  NULL if TCLX_CHOWN
 *     not specified.
 *   o groupStr - String containing owner name or id.  NULL if TCLX_CHOWN
 *     not specified.  If NULL and TCLX_CHOWN is specified, the user's group
 *     is used.
 *   o files - NULL terminated list of file names.
 *   o funcName - Command or other name to use in not available error.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSChangeOwnGrp (interp, options, ownerStr, groupStr, files, funcName)
    Tcl_Interp  *interp;
    unsigned     options;
    char        *ownerStr;
    char        *groupStr;
    char       **files;
    char       *funcName;
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSFChangeOwnGrp --
 *   Change the owner and/or group of a file by open channel.
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o options - Option flags are:
 *     o TCLX_CHOWN - Change file's owner.
 *     o TCLX_CHGRP - Change file's group.
 *   o ownerStr - String containing owner name or id.  NULL if TCLX_CHOWN
 *     not specified.
 *   o groupStr - String containing owner name or id.  NULL if TCLX_CHOWN
 *     not specified.  If NULL and TCLX_CHOWN is specified, the user's group
 *     is used.
 *   o channelIds - NULL terminated list of channel ids.
 *   o funcName - Command or other name to use in not available error.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSFChangeOwnGrp (interp, options, ownerStr, groupStr, channelIds, funcName)
    Tcl_Interp *interp;
    unsigned    options;
    char       *ownerStr;
    char       *groupStr;
    char      **channelIds;
    char       *funcName;
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSGetSelectFnum --
 *   Convert a channel its read and write file numbers for use in select.
 *
 * Parameters:
 *   o interp - Pointer to the current interpreter, error messages will be
 *     returned in the result.
 *   o channel - Channel to get the numbers for.
 *   o readFnumPtr - The read file number is returned here.  -1 is returned if
 *     the file is not open for reading.
 *   o writeFnumPtr - The write file number is returned here.  -1 is returned
 *     if the file is not open for writing.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSGetSelectFnum (Tcl_Interp *interp,
                     Tcl_Channel channel,
                     int        *readFnumPtr,
                     int        *writeFnumPtr)
{
    Tcl_File file;
    int fnum, type;

    file = Tcl_GetChannelFile (channel, TCL_READABLE);
    if (file != NULL) {
        *readFnumPtr = (int) ((SOCKET) Tcl_GetFileInfo (file, &type));
        if (type != TCL_WIN_SOCKET)
            goto badFileType;
    } else {
        *readFnumPtr = -1;
    }
    file = Tcl_GetChannelFile (channel, TCL_WRITABLE);
    if (file != NULL) {
        *writeFnumPtr = (int) ((SOCKET) Tcl_GetFileInfo (file, &type));
        if (type != TCL_WIN_SOCKET)
            goto badFileType;
    } else {
        *writeFnumPtr = -1;
    }
    return TCL_OK;
    
  badType:
    Tcl_AppendResult (interp, Tcl_GetChannelName (channel),
                      " is not a socket; select only works on sockets on ",
                      "Windows", (char *) NULL);
    return TCL_ERROR;
}
