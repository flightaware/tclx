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
 * $Id: tclXwinOS.c,v 1.3 1996/03/20 06:58:08 markd Exp $
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
static int
NotAvailableError _ANSI_ARGS_((Tcl_Interp *interp,
                               char       *funcName));

#if 0

/*-----------------------------------------------------------------------------
 * TclX_OS --
 *   Portability interface to functionallity.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OS (interp, funcName)
    Tcl_Interp *interp;
    char       *funcName;
#endif


/*-----------------------------------------------------------------------------
 * NotAvailableError --
 *   Return an error about functionality not being available under Windows.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
NotAvailableError (interp, funcName)
    Tcl_Interp *interp;
    char       *funcName;
{
    Tcl_AppendResult (interp, funcName, " is not available under MS Windows",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_OSchroot --
 *   Portability interface to chroot functionallity, which is not available
 * on windows.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o path (I) - Directory path to make root.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSchroot (interp, path, funcName)
    Tcl_Interp *interp;
    char       *path;
    char       *funcName;
{
    return NotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclX_OSgetpriority --
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
TclX_OSgetpriority (interp, priority, funcName)
    Tcl_Interp *interp;
    int        *priority;
    char       *funcName;
{
    return NotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclX_OSincrpriority--
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
TclX_OSincrpriority (interp, priorityIncr, priority, funcName)
    Tcl_Interp *interp;
    int         priorityIncr;
    int        *priority;
    char       *funcName;
{
    return NotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclX_OSpipe --
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
TclX_OSpipe (interp, fildes)
    Tcl_Interp *interp;
    int        *fildes;
{
    if (_pipe (fileNums, 16384, 0) < 0) {
        Tcl_AppendResult (interp, "pipe creation failed: ",
                          Tcl_PosixError (interp));
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*-----------------------------------------------------------------------------
 * TclX_OSsetitimer --
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
TclX_OSsetitimer (interp, seconds, funcName)
    Tcl_Interp *interp;
    double      *seconds;
    char       *funcName;
{
    return NotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclX_OSsleep --
 *   Portability interface to sleep functionallity.
 *
 * Parameters:
 *   o seconds (I) - Seconds to sleep.
 *-----------------------------------------------------------------------------
 */
void
TclX_OSsleep (seconds)
    unsigned seconds;
{
    Sleep (seconds*100);
}

/*-----------------------------------------------------------------------------
 * TclX_OSsync --
 *   Portability interface to sync functionality.
 *-----------------------------------------------------------------------------
 */
void
TclX_OSsync ()
{
    _flushall ();
}

/*-----------------------------------------------------------------------------
 * TclX_OSfsync --
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
TclX_OSfsync (interp, channelName)
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
 * TclX_OSsystem --
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
TclX_OSsystem (interp, command, exitCode)
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
 * TclX_OSmkdir --
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
TclX_OSmkdir (interp, path)
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
    return NotAvailableError (interp, funcName);
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
    return NotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclX_OSElapsedTime --
 *   Portability interface to get the elapsed CPU and real time.  CPU time
 * is not available under windows and zero is always returned.
 *
 * Parameters:
 *   o realTime (O) - Elapsed real time, in milliseconds is returned here.
 *   o cpuTime (O) - Elapsed CPU time, zero is always returned.
 *-----------------------------------------------------------------------------
 */
void
TclX_OSElapsedTime (realTime, cpuTime)
    clock_t *realTime;
    clock_t *cpuTime;
{
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
}

/*-----------------------------------------------------------------------------
 * TclX_OSkill --
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
TclX_OSkill (interp, pid, signal, funcName)
    Tcl_Interp *interp;
    pid_t       pid;
    int         signal;
    char       *funcName;
{
    return NotAvailableError (interp, funcName);
}

/*-----------------------------------------------------------------------------
 * TclX_OSGetOpenFileMode --
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
TclX_OSGetOpenFileMode (fileNum, mode, nonBlocking)
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
 * TclX_OSopendir --
 *   Portability interface to opendir functionallity.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.  Maybe NULL if errors text is
 *     not to be returned.
 *   o path (I) - Path to the directory.
 *   o handlePtr (O) - The handle to pass in other functions to access this
 *     directory is returned here.
 *   o caseSensitive (O) - Are the file names case sensitive?
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSopendir (interp, path, handlePtr, caseSensitive)
    Tcl_Interp     *interp;
    char           *path;
    TCLX_DIRHANDLE *handlePtr;
    int            *caseSensitive;
{
    Tcl_DString dirPath;
    char *rootEnd, *dummy1, *dummy2;
    int baseLength;
    Tcl_DString buffer;
    DWORD volFlags;
    HANDLE handle;
    WIN32_FIND_DATA data;
    BOOL found;
    char *newDir;

    /*
     * Local copy of the directory path.
     */
    Tcl_DStringInit (&dirPath);
    Tcl_DStringAppend (&dirPath, path, -1);
    baseLength = Tcl_DStringLength (&dirPath);

    /*
     * Ensure that the passed in path is actually a directory.
     */
    if (baseLength == 0) {
	newDir = ".";
    } else {
	newDir = dirPath.string;
    }

    /*
     * Next check the volume information for the directory to see whether
     * comparisons should be case sensitive or not.  If the root is null, then
     * we use the root of the current directory.  If the root is just a drive
     * specifier, we use the root directory of the given drive.
     */
    TclParseFileName (newDir, &rootEnd, &dummy1, &dummy2);
    if (rootEnd == NULL) {
	found = GetVolumeInformation (NULL, NULL, 0, NULL, NULL,
                                      &volFlags, NULL, 0);
    } else if (*rootEnd == ':') {
	char root[4] = "?:\\";
	root[0] = *newDir;
	found = GetVolumeInformation (root, NULL, 0, NULL, NULL,
                                      &volFlags, NULL, 0);
    } else {
	char *p;

	/*
	 * Since Win32 doesn't accept UNC names with // in
	 * GetVolumeInformation, substitute \ for / before the call.
	 */
	Tcl_DStringInit (&buffer);
	Tcl_DStringAppend (&buffer, newDir, (rootEnd - newDir) + 1);
	for (p = buffer.string; *p != '\0'; p++) {
	    if (*p == '/') {
		*p = '\\';
	    }
	}
	found = GetVolumeInformation (buffer.string, NULL, 0, NULL, NULL,
                                      &volFlags, NULL, 0);
	Tcl_DStringFree (&buffer);
    }
    if (!found) {
	TclWinConvertError (GetLastError ());
	Tcl_ResetResult (interp);
	Tcl_AppendResult (interp, "couldn't read volume information for \"",
                          dirPath.string, "\": ", Tcl_PosixError (interp),
                          (char *) NULL);
	goto errorExit;
    }
    *caseSensitive = (volFlags & FS_CASE_SENSITIVE) ? TRUE : FALSE;
    
    /*
     * We need to check all files in the directory, so append a *.*
     * to the path.  If the path ends in a non-separator character,
     * we also need to add a directory separator before the *.*.
     */
    if ((baseLength > 0)
	    && (strchr (":/\\", dirPath.string [baseLength-1]) == 0)) {
	Tcl_DStringAppend (&dirPath, "\\*.*", 4);
    } else {
	Tcl_DStringAppend (&dirPath, "*.*", 3);
    }

    /*
     * Now open the directory for reading.
     */
    handle = FindFirstFile (dirPath.string, &data);
    if (handle == INVALID_HANDLE_VALUE) {
	TclWinConvertError (GetLastError());
        if (interp != NULL)
            Tcl_AppendResult (interp, "open of directory \"", path,
                              "\" failed: ", Tcl_PosixError (interp),
                              (char *) NULL);
	goto errorExit;
    }
    
    *handlePtr = handle;
    Tcl_DStringFree (&dirPath);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&dirPath);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclX_OSreaddir --
 *   Portability interface to readdir functionallity.  The "." and ".." entries
 * are not returned.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o handle (I) - The handle returned by TclX_OSopendir.
 *   o hidden (I) - Include hidden files.
 *   o fileNamePtr (O) - A pointer to the filename is returned here.
 * Results:
 *   TCL_OK, TCL_ERROR or TCL_BREAK if there are no more directory entries.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSreaddir (interp, handle, fileNamePtr)
    Tcl_Interp     *interp;
    TCLX_DIRHANDLE  handle;
    int             hidden;
    char          **fileNamePtr;
{
    WIN32_FIND_DATA data;

    while (TRUE) {
        if (!FindNextFile (handle, &data))
            return TCL_BREAK;

	if ((data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !hidden)
            continue;
        if (data.cFileName [0] == '.') {
            if (data.cFileName [1] == '\0')
                continue;
            if ((data.cFileName [1] == '.') &&
                (data.cFileName [2] == '\0'))
                continue;
        }
        *fileNamePtr = data.cFileName;
        return TCL_OK;
    }
}

/*-----------------------------------------------------------------------------
 * TclX_OSclosedir --
 *   Portability interface to closedir functionallity.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.  Maybe NULL if error text is
 *     not to be returned.
 *   o handle (I) - The handle returned by TclX_OSopendir.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSclosedir (interp, handle)
    Tcl_Interp     *interp;
    TCLX_DIRHANDLE  handle;
{
    FindClose (handle);
    return TCL_OK;
}
