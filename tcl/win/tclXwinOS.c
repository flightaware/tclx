/*
 * tclXunixOS.c --
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
 * $Id: tclXwinOS.c,v 7.1 1996/07/18 19:36:36 markd Exp $
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
ChannelToHandle _ANSI_ARGS_((Tcl_Channel channel,
                             int         direction));


/*-----------------------------------------------------------------------------
 * TclXNotAvailableError --
 *   Return an error about functionality not being available under Windows.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o funcName (I) - Command or other name to use in not available error.
 * Returns:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXNotAvailableError (interp, funcName)
    Tcl_Interp *interp;
    char       *funcName;
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
 *   o argcPtr (O) - Count of arguments is returned here.
 *   o argvPtr (O) - Argument vector is returned here.
 * Notes:
 *   This code taken from the Tcl file tclAppInit.c: Copyright (c) 1996 by
 * Sun Microsystems, Inc.
 *-----------------------------------------------------------------------------
 */
void
TclX_SplitWinCmdLine (argcPtr, argvPtr)
    int    *argcPtr;
    char ***argvPtr;
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
 *   o channel (I) - Channel to get file number for.
 *   o direction (I) - TCL_READABLE or TCL_WRITABLE, or zero.  If zero, then
 *     return the first of the read and write numbers.
 * Returns:
 *   The file number or NULL if a file number is not associated with this
 * access direction.
 *-----------------------------------------------------------------------------
 */
HANDLE
ChannelToHandle (channel, direction)
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
            return NULL;
    }
    return (HANDLE) Tcl_GetFileInfo (file, NULL);
}

/*-----------------------------------------------------------------------------
 * TclXOSgetpriority --
 *   Portability interface to getpriority functionality, which is not available
 * on windows.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o priority (O) - Process priority is returned here.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSgetpriority (interp, priority, funcName)
    Tcl_Interp *interp;
    int        *priority;
    char       *funcName;
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
 *   o interp (I) - Errors returned in result.
 *   o priorityIncr (I) - Amount to adjust the priority by.
 *   o priority (O) - The new priority..
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSincrpriority (interp, priorityIncr, priority, funcName)
    Tcl_Interp *interp;
    int         priorityIncr;
    int        *priority;
    char       *funcName;
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSpipe --
 *   Portability interface to pipe.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o fildes (O) - Array to return file descriptors in.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSpipe (interp, fildes)
    Tcl_Interp *interp;
    int        *fildes;
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
 *   o interp (I) - Errors returned in result.
 *   o seconds (I/O) - Seconds to pause for, it is updated with the time
 *     remaining on the last alarm.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSsetitimer (interp, seconds, funcName)
    Tcl_Interp *interp;
    double      *seconds;
    char       *funcName;
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSsleep --
 *   Portability interface to sleep functionallity.
 *
 * Parameters:
 *   o seconds (I) - Seconds to sleep.
 *-----------------------------------------------------------------------------
 */
void
TclXOSsleep (seconds)
    unsigned seconds;
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
 *   Portability interface to fsync functionallity.  Does a sync if fsync is
 * nor available.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o channelName (I) - Name fo channel to sync.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSfsync (interp, channelName)
    Tcl_Interp *interp;
    char       *channelName;
{
    Tcl_Channel channel;
    int fileNum;

    channel = TclX_GetOpenChannel (interp, channelName, TCL_WRITABLE);
    if (channel == NULL)
        return TCL_ERROR;

    fileNum = TclX_GetOpenFnum (interp, channelName, TCL_WRITABLE);
    if (fileNum < 0)
        return TCL_ERROR;

    if (Tcl_Flush (channel) < 0)
        goto posixError;

    _flushall ();
    return TCL_OK;

  posixError:
    interp->result = Tcl_PosixError (interp);
    return TCL_ERROR;

}

/*-----------------------------------------------------------------------------
 * TclXOSsystem --
 *   Portability interface to system functionallity (executing a command
 * with the standard system shell).
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o command (I) - Command to execute.
 *   o exitCode (O) - Exit code of the child process.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSsystem (interp, command, exitCode)
    Tcl_Interp *interp;
    char       *command;
    int        *exitCode;
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
 *   o interp (I) - Errors returned in result.
 *   o path (I) - Directory to create.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSmkdir (interp, path)
    Tcl_Interp *interp;
    char       *path;
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
 *   o interp (I) - Errors returned in result.
 *   o srcPath (I) - File to link.
 *   o targetPath (I) - Path to new link.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSlink (interp, srcPath, targetPath, funcName)
    Tcl_Interp *interp;
    char       *srcPath;
    char       *targetPath;
    char       *funcName;
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclX_OSsymlink --
 *   Portability interface to symlink functionallity, which is not available
 * on windows.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o srcPath (I) - Value of symbolic link.
 *   o targetPath (I) - Path to new symbolic link.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSsymlink (interp, srcPath, targetPath, funcName)
    Tcl_Interp *interp;
    char       *srcPath;
    char       *targetPath;
    char       *funcName;
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSElapsedTime --
 *   Portability interface to get the elapsed CPU and real time.  CPU time
 * is not available under windows and zero is always returned.
 *
 * Parameters:
 *   o realTime (O) - Elapsed real time, in milliseconds is returned here.
 *   o cpuTime (O) - Elapsed CPU time, zero is always returned.
 *-----------------------------------------------------------------------------
 */
void
TclXOSElapsedTime (realTime, cpuTime)
    clock_t *realTime;
    clock_t *cpuTime;
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
 *   o interp (I) - Errors returned in result.
 *   o pid (I) - Process id, negative process group, etc.
 *   o signal (I) - Signal to send.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSkill (interp, pid, signal, funcName)
    Tcl_Interp *interp;
    pid_t       pid;
    int         signal;
    char       *funcName;
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSGetOpenFileMode --
 *   Portability interface to get the accessability on an open file number.
 *
 * Parameters:
 *   o fileNum (I) - Number to return permissions on.
 *   o mode (O) - Set of TCL_READABLE and TCL_WRITABLE.
 *   o nonBlocking (O) - Non-blocking mode, always returns FALSE under windows.
 * Results:
 *   TCL_OK or TCL_ERROR.  The Posix error number describes the error.
 *-----------------------------------------------------------------------------
 */
int
TclXOSGetOpenFileMode (fileNum, mode, nonBlocking)
    int  fileNum;
    int *mode;
    int *nonBlocking;
{
    struct stat fileStat;

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
 * TclXOSWalkDir --
 *   Portability interface to reading the contents of a directory.  The
 * specified directory is walked and a callback is called on each entry.
 * The "." and ".." entries are skipped.
 *
 * Parameters:
 *   o interp (I) - Interp to return errors in.
 *   o path (I) - Path to the directory.
 *   o hidden (I) - Include hidden files.  Ignored on Unix.
 *   o callback (I) - Callback function to call on each directory entry.
 *     It should return TCL_OK to continue processing, TCL_ERROR if an
 *     error occured and TCL_BREAK to stop processing.  The parameters are:
 *        o interp (I) - Interp is passed though.
 *        o path (I) - Normalized path to directory.
 *        o fileName (I) - Tcl normalized file name in directory.
 *        o caseSensitive (I) - Are the file names case sensitive?
 *        o clientData (I) - Client data that was passed.
 *   o clientData (I) - Client data to pass to callback.
 * Results:
 *   TCL_OK if completed directory walk.  TCL_BREAK if callback returned
 * TCL_BREAK and TCL_ERROR if an error occured.
 *-----------------------------------------------------------------------------
 */
int
TclXOSWalkDir (interp, path, hidden, callback, clientData)
    Tcl_Interp       *interp;
    char             *path;
    int               hidden;
    TclX_WalkDirProc *callback;
    ClientData        clientData;
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
 *   o channel (I) - Channel.
 *   o fileSize (O) - File size is returned here.
 *   o direction (I) - TCL_READABLE or TCL_WRITABLE, or zero.  If zero, then
 *     return the first of the read and write numbers.
 * Results:
 *   TCL_OK or TCL_ERROR.  A POSIX error will be set.
 *-----------------------------------------------------------------------------
 */
int
TclXOSGetFileSize (channel, direction, fileSize)
    Tcl_Channel  channel;
    int          direction;
    off_t       *fileSize;
{
    *fileSize = GetFileSize (ChannelToHandle (channel, direction), NULL);
    if (*fileSize < 0) {
        TclWinConvertError (GetLastError ());
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSftruncate --
 *   Portability interface to ftruncate functionality.  Coded to be part of
 * the ftruncate command to allow for good error messages.
 *
 * Parameters:
 *   o interp (I) - Error messages are returned in the interpreter.
 *   o fileHandle (I) - Path to file.
 *   o newSize (I) - Size to truncate the file to.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSftruncate (interp, fileHandle, newSize)
    Tcl_Interp  *interp;
    char        *fileHandle;
    off_t        newSize;
{
    Tcl_AppendResult (interp,
                      "the -fileid option is not available on MS Windows";
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclXOSfork --
 *   Portability interface to fork functionallity.  Not supported on windows.
 *
 * Parameters:
 *   o interp (I) - An error  is returned in result.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSfork (interp, funcName)
    Tcl_Interp *interp;
    char       *funcName;
{
    return TclXNotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclXOSexecl --
 *   Portability interface to execl functionallity.  On windows, this is the
 * equivlant of a fork and an execl, so a process id is returned.
 *
 * Parameters:
 *   o interp (I) - A process id or errors are returned in result.
 *   o path (I) - Path to the program.
 *   o argList (I) - NULL terminated argument vector.
 * Results:
 *   TCL_ERROR or does not return.
 *-----------------------------------------------------------------------------
 */
int
TclXOSexecl (interp, path, argList)
    Tcl_Interp *interp;
    char       *path;
    char      **argList;
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
 *   o interp (O) - If not NULL, an error message is return in the result.
 *     If NULL, no error message is generated.
 *   o strAddress (I) - String address to convert.
 *   o inAddress (O) - Converted internet address is returned here.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSInetAtoN (interp, strAddress, inAddress)
    Tcl_Interp     *interp;
    char           *strAddress;
    struct in_addr *inAddress;
{
    if (inet_aton (strAddress, inAddress))
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
 *   o channel (I) - Channel associated with the socket.
 *   o sockaddr (I) - Pointer to sockaddr structure.
 *   o sockaddrSize (I) - Size of the sockaddr struct.
 * Results:
 *   TCL_OK or TCL_ERROR, sets a posix error.
 *-----------------------------------------------------------------------------
 */
int
TclXOSgetpeername (channel, sockaddr, sockaddrSize)
    Tcl_Channel channel;
    void       *sockaddr;
    int         sockaddrSize;
{
    int fnum;

    fnum = TclX_ChannelFnum (channel, 0);
    if (getpeername (fnum, (struct sockaddr *) sockaddr, &sockaddrSize) < 0)
        return TCL_ERROR;
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSgetsockname --
 *   Portability interface to getsockname functionallity.
 *
 * Parameters:
 *   o channel (I) - Channel associated with the socket.
 *   o sockaddr (I) - Pointer to sockaddr structure.
 *   o sockaddrSize (I) - Size of the sockaddr struct.
 * Results:
 *   TCL_OK or TCL_ERROR, sets a posix error.
 *-----------------------------------------------------------------------------
 */
int
TclXOSgetsockname (channel, sockaddr, sockaddrSize)
    Tcl_Channel channel;
    void       *sockaddr;
    int         sockaddrSize;
{
    int fnum;

    fnum = TclX_ChannelFnum (channel, 0);
    if (getsockname (fnum, (struct sockaddr *) sockaddr, &sockaddrSize) < 0)
        return TCL_ERROR;
    return TCL_OK;
}
