/*
 * tclXunixOS.c --
 *
 * OS portability interface for Unix systems.  The idea behind these functions
 * is to provide interfaces to various functions that vary on the various
 * platforms.  These functions either implement the call in a manner approriate
 * to the platform or return an error indicating the functionality is not
 * available on that platform.  This results in code with minimal number of
 * #ifdefs.
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
 * $Id: tclXunixOS.c,v 7.1 1996/07/18 19:36:33 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

#ifndef NO_GETPRIORITY
#include <sys/resource.h>
#endif

/*
 * Cheat a little to avoid configure checking for floor and ceil being
 * This breaks with GNU libc headers...really should check with autoconf.
 */
#ifndef __GNU_LIBRARY__
extern
double floor ();

extern
double ceil ();
#endif

/*
 * Prototypes of internal functions.
 */
static int
ChannelToFnum _ANSI_ARGS_((Tcl_Channel channel,
                           int         direction));


/*-----------------------------------------------------------------------------
 * ChannelToFnum --
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
static int
ChannelToFnum (channel, direction)
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
 * TclXOSTicksToMS --
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
TclXOSTicksToMS (numTicks)
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
 * TclXOSgetpriority --
 *   Portability interface to getpriority functionality.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o priority (O) - Process priority is returned here.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSgetpriority (interp, priority, funcName)
    Tcl_Interp *interp;
    int        *priority;
    char       *funcName;
{
#ifndef NO_GETPRIORITY
    *priority = getpriority (PRIO_PROCESS, 0);
#else
    *priority = nice (0);
#endif
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSincrpriority--
 *   Portability interface to increment or decrement the current priority.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o priorityIncr (I) - Amount to adjust the priority by.
 *   o priority (O) - The new priority..
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSincrpriority (interp, priorityIncr, priority, funcName)
    Tcl_Interp *interp;
    int         priorityIncr;
    int        *priority;
    char       *funcName;
{
    errno = 0;  /* Old priority might be -1 */

#ifndef NO_GETPRIORITY
    *priority = getpriority (PRIO_PROCESS, 0) + priorityIncr;
    if (errno == 0) {
        setpriority (PRIO_PROCESS, 0, *priority);
    }
#else
    *priority = nice (priorityIncr);
#endif
    if (errno != 0) {
        Tcl_AppendResult (interp, "failed to increment priority: ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
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
    if (pipe (fildes) < 0) {
        Tcl_AppendResult (interp, "pipe creation failed: ",
                          Tcl_PosixError (interp));
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*-----------------------------------------------------------------------------
 * TclXOSsetitimer --
 *   Portability interface to setitimer functionality.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o seconds (I/O) - Seconds to pause for, it is updated with the time
 *     remaining on the last alarm.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSsetitimer (interp, seconds, funcName)
    Tcl_Interp *interp;
    double     *seconds;
    char       *funcName;
{
/*
 * A million microseconds per seconds.
 */
#define TCL_USECS_PER_SEC (1000L * 1000L)

#ifndef NO_SETITIMER
    double secFloor;
    struct itimerval  timer, oldTimer;

    secFloor = floor (*seconds);

    timer.it_value.tv_sec     = secFloor;
    timer.it_value.tv_usec    = (long) ((*seconds - secFloor) *
                                        (double) TCL_USECS_PER_SEC);
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = 0;  

    if (setitimer (ITIMER_REAL, &timer, &oldTimer) < 0) {
        Tcl_AppendResult (interp, "unable to obtain timer: ",
                          Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    *seconds  = oldTimer.it_value.tv_sec;
    *seconds += ((double) oldTimer.it_value.tv_usec) /
        ((double) TCL_USECS_PER_SEC);

    return TCL_OK;
#else
    unsigned useconds;

    useconds = ceil (*seconds);
    *seconds = alarm (useconds);

    return TCL_OK;
#endif
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
    sleep (seconds);
}

/*-----------------------------------------------------------------------------
 * TclXOSsync --
 *   Portability interface to sync functionality.
 *-----------------------------------------------------------------------------
 */
void
TclXOSsync ()
{
    sync ();
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

#ifndef NO_FSYNC
    if (fsync (fileNum) < 0)
        goto posixError;
#else
    sync ();
#endif
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
    int errPipes [2], childErrno;
    pid_t pid;
    WAIT_STATUS_TYPE waitStatus;

    errPipes [0] = errPipes [1] = -1;

    /*
     * Create a close on exec pipe to get status back from the child if
     * the exec fails.
     */
    if (pipe (errPipes) != 0) {
        Tcl_AppendResult (interp, "couldn't create pipe: ",
                          Tcl_PosixError (interp), (char *) NULL);
        goto errorExit;
    }
    if (fcntl (errPipes [1], F_SETFD, FD_CLOEXEC) != 0) {
        Tcl_AppendResult (interp, "couldn't set close on exec for pipe: ",
                          Tcl_PosixError (interp), (char *) NULL);
        goto errorExit;
    }

    pid = fork ();
    if (pid == -1) {
        Tcl_AppendResult (interp, "couldn't fork child process: ",
                          Tcl_PosixError (interp), (char *) NULL);
        goto errorExit;
    }
    if (pid == 0) {
        close (errPipes [0]);
        execl ("/bin/sh", "sh", "-c", command, (char *) NULL);
        write (errPipes [1], &errno, sizeof (errno));
        _exit (127);
    }

    close (errPipes [1]);
    if (read (errPipes [0], &childErrno, sizeof (childErrno)) > 0) {
        errno = childErrno;
        Tcl_AppendResult (interp, "couldn't execing /bin/sh: ",
                          Tcl_PosixError (interp), (char *) NULL);
        waitpid (pid, (int *) &waitStatus, 0);
        goto errorExit;
    }
    close (errPipes [0]);

    waitpid (pid, (int *) &waitStatus, 0);
    
    /*
     * Return status based on wait result.
     */
    if (WIFEXITED (waitStatus)) {
        *exitCode = WEXITSTATUS (waitStatus);
        return TCL_OK;
    }

    if (WIFSIGNALED (waitStatus)) {
        Tcl_SetErrorCode (interp, "SYSTEM", "SIG",
                          Tcl_SignalId (WTERMSIG (waitStatus)), (char *) NULL);
        Tcl_AppendResult (interp, "system command terminate with signal ",
                          Tcl_SignalId (WTERMSIG (waitStatus)), (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Should never get this status back unless the implementation is
     * really brain-damaged.
     */
    if (WIFSTOPPED (waitStatus)) {
        Tcl_AppendResult (interp, "system command child stopped",
                          (char *) NULL);
        return TCL_ERROR;
    }

  errorExit:
    close (errPipes [0]);
    close (errPipes [1]);
    return TCL_ERROR;
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
    if (mkdir (path, S_IFDIR | 0777) < 0) {
        Tcl_AppendResult (interp, "creating directory \"", path,
                          "\" failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_OSlink --
 *   Portability interface to link functionallity.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o srcPath (I) - File to link.
 *   o targetPath (I) - Path to new link.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSlink (interp, srcPath, targetPath, funcName)
    Tcl_Interp *interp;
    char       *srcPath;
    char       *targetPath;
    char       *funcName;
{
    if (link (srcPath, targetPath) != 0) {
        Tcl_AppendResult (interp, "linking \"", srcPath, "\" to \"",
                          targetPath, "\" failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_OSsymlink --
 *   Portability interface to symlink functionallity.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o srcPath (I) - Value of symbolic link.
 *   o targetPath (I) - Path to new symbolic link.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSsymlink (interp, srcPath, targetPath, funcName)
    Tcl_Interp *interp;
    char       *srcPath;
    char       *targetPath;
    char       *funcName;
{
#ifdef S_IFLNK
    if (symlink (srcPath, targetPath) != 0) {
        Tcl_AppendResult (interp, "creating symbolic link \"",
                          targetPath, "\" failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
#else
    Tcl_AppendResult (interp, "symbolic links are not supported on this",
                          " Unix system", (char *) NULL);
    return TCL_ERROR;
#endif
}

/*-----------------------------------------------------------------------------
 * TclXOSElapsedTime --
 *   Portability interface to get the elapsed CPU and real time. 
 *
 * Parameters:
 *   o realTime (O) - Elapsed real time, in milliseconds is returned here.
 *   o cpuTime (O) - Elapsed CPU time, in milliseconds is returned here.
 *-----------------------------------------------------------------------------
 */
void
TclXOSElapsedTime (realTime, cpuTime)
    clock_t *realTime;
    clock_t *cpuTime;
{
/*
 * If times returns elapsed real time, this is easy.  If it returns a status,
 * real time must be obtained in other ways.
 */
#ifndef TIMES_RETS_STATUS
    struct tms cpuTimes;

    *realTime = TclXOSTicksToMS (times (&cpuTimes));
    *cpuTime = TclXOSTicksToMS (cpuTimes.tms_utime + cpuTimes.tms_stime);
#else
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
    times (&cpuTimes);
    *cpuTime = TclXOSTicksToMS (cpuTimes.tms_utime + cpuTimes.tms_stime);
#endif
}

/*-----------------------------------------------------------------------------
 * TclXOSkill --
 *   Portability interface to functionallity.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o pid (I) - Process id, negative process group, etc.
 *   o signal (I) - Signal to send.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXOSkill (interp, pid, signal, funcName)
    Tcl_Interp *interp;
    pid_t       pid;
    int         signal;
    char       *funcName;
{
    if (kill (pid, signal) < 0) {
        char pidStr [32];

        Tcl_AppendResult (interp, "sending signal ",
                          (signal == 0) ? 0 : Tcl_SignalId (signal),
                          (char *) NULL);
        if (pid > 0) {
            sprintf (pidStr, "%d", pid);
            Tcl_AppendResult (interp, " to process ", pidStr, (char *) NULL);
        } else if (pid == 0) {
            sprintf (pidStr, "%d", getpgrp ());
            Tcl_AppendResult (interp, " to current process group (", 
                              pidStr, ")", (char *) NULL);
        } else if (pid == -1) {
            Tcl_AppendResult (interp, " to all processess ", (char *) NULL);
        } else if (pid < -1) {
            sprintf (pidStr, "%d", -pid);
            Tcl_AppendResult (interp, " to process group ", 
                              pidStr, (char *) NULL);
        }
        Tcl_AppendResult (interp, " failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSGetOpenFileMode --
 *   Portability interface to get the accessability on an open file number.
 *
 * Parameters:
 *   o fileNum (I) - Number to return permissions on.
 *   o mode (O) - Set of TCL_READABLE and TCL_WRITABLE.
 *   o nonBlocking (O) - TRUE if the file is in non-blocking mode, FALSE if
 *     its normal.
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
    int fileMode;

    fileMode = fcntl (fileNum, F_GETFL, 0);
    if (fileMode == -1)
        return TCL_ERROR;

    switch (fileMode & O_ACCMODE) {
      case O_RDONLY:
        *mode = TCL_READABLE;
        break;
      case O_WRONLY:
        *mode = TCL_WRITABLE;
        break;
      case O_RDWR:
        *mode = TCL_READABLE | TCL_WRITABLE;
        break;
    }
    if (fileMode & (O_NONBLOCK | O_NDELAY))
        *nonBlocking = TRUE;
    else
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
 *        o caseSensitive (I) - Are the file names case sensitive?  Always
 *          TRUE on Unix.
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
    DIR *handle;
    struct dirent *entryPtr;
    int result = TCL_OK;
    
    handle = opendir (path);
    if (handle == NULL)  {
        if (interp != NULL)
            Tcl_AppendResult (interp, "open of directory \"", path,
                              "\" failed: ", Tcl_PosixError (interp),
                              (char *) NULL);
        return TCL_ERROR;
    }
   
    while (TRUE) {
        entryPtr = readdir (handle);
        if (entryPtr == NULL)
            return TCL_BREAK;
        if (entryPtr->d_name [0] == '.') {
            if (entryPtr->d_name [1] == '\0')
                continue;
            if ((entryPtr->d_name [1] == '.') &&
                (entryPtr->d_name [2] == '\0'))
                continue;
        }
        result = (*callback) (interp, path, entryPtr->d_name,
                              TRUE, clientData);
        if (!((result == TCL_OK) || (result == TCL_CONTINUE)))
            break;
    }
    if (result == TCL_ERROR) {
        closedir (handle);
        return TCL_ERROR;
    }
    if (closedir (handle) < 0) {
        if (interp != NULL)
            Tcl_AppendResult (interp, "close of directory failed: ",
                              Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
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
    struct stat statBuf;

    if (fstat (ChannelToFnum (channel, direction), &statBuf)) {
        return TCL_ERROR;
    }
    *fileSize = statBuf.st_size;
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
#if (!defined(NO_FTRUNCATE)) || defined(HAVE_CHSIZE) 
    int fileNum, stat;
    Tcl_File file;

    fileNum = TclX_GetOpenFnum (interp, fileHandle, TCL_WRITABLE);
    if (fileNum < 0)
        return TCL_ERROR;

#ifndef NO_FTRUNCATE
    stat = ftruncate (fileNum, newSize);
#else
    stat = chsize (fileNum, newSize);
#endif
    if (stat != 0) {
        Tcl_AppendResult (interp, fileHandle, ": ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
#else
    Tcl_AppendResult (interp,
                      "the -fileid option is not available on this system";
                      (char *) NULL);
    return TCL_ERROR;
#endif
}

/*-----------------------------------------------------------------------------
 * TclXOSfork --
 *   Portability interface to fork functionallity.
 *
 * Parameters:
 *   o interp (I) - A format process id or errors are returned in result.
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
    pid_t pid;
    char numBuf [32];
    
    pid = fork ();
    if (pid < 0) {
        Tcl_AppendResult (interp, "fork failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }

    sprintf (numBuf, "%d", pid);
    Tcl_SetResult (interp, numBuf, TCL_VOLATILE);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclXOSexecl --
 *   Portability interface to execl functionallity.
 *
 * Parameters:
 *   o interp (I) - Errors are returned in result.
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
    execvp (path, argList);

    /*
     * Can only make it here on an error.
     */
    Tcl_AppendResult (interp, "exec of \"", path, "\" failed: ",
                      Tcl_PosixError (interp), (char *) NULL);
    return TCL_ERROR;
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
#ifndef NO_INET_ATON
    if (inet_aton (strAddress, inAddress))
        return TCL_OK;
#else
    inAddress->s_addr = inet_addr (strAddress);
    if (inAddress->s_addr != INADDR_NONE)
        return TCL_OK;
#endif
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
