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
 * $Id: tclXunixOS.c,v 5.4 1996/03/20 06:58:02 markd Exp $
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
 * TclX_OSTicksToMS --
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
TclX_OSTicksToMS (numTicks)
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
 * TclX_OSchroot --
 *   Portability interface to chroot functionallity.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o path (I) - Directory path to make root.
 *   o funcName (I) - Command or other name to use in not available error.
 * Results:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSchroot (interp, path, funcName)
    Tcl_Interp *interp;
    char       *path;
    char       *funcName;
{
    if (chroot (path) < 0) {
        Tcl_AppendResult (interp, "changing root to \"", path,
                          "\" failed: ", Tcl_PosixError (interp),
                          (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_OSgetpriority --
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
TclX_OSgetpriority (interp, priority, funcName)
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
 * TclX_OSincrpriority--
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
TclX_OSincrpriority (interp, priorityIncr, priority, funcName)
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
    if (pipe (fildes) < 0) {
        Tcl_AppendResult (interp, "pipe creation failed: ",
                          Tcl_PosixError (interp));
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*-----------------------------------------------------------------------------
 * TclX_OSsetitimer --
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
TclX_OSsetitimer (interp, seconds, funcName)
    Tcl_Interp *interp;
    double      *seconds;
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

    useconds = ceil (seconds);
    *seconds = alarm (useconds);

    return TCL_OK;
#endif
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
    sleep (seconds);
}

/*-----------------------------------------------------------------------------
 * TclX_OSsync --
 *   Portability interface to sync functionality.
 *-----------------------------------------------------------------------------
 */
void
TclX_OSsync ()
{
    sync ();
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
 * TclX_OSElapsedTime --
 *   Portability interface to get the elapsed CPU and real time. 
 *
 * Parameters:
 *   o realTime (O) - Elapsed real time, in milliseconds is returned here.
 *   o cpuTime (O) - Elapsed CPU time, in milliseconds is returned here.
 *-----------------------------------------------------------------------------
 */
void
TclX_OSElapsedTime (realTime, cpuTime)
    clock_t *realTime;
    clock_t *cpuTime;
{
/*
 * If times returns elapsed real time, this is easy.  If it returns a status,
 * real time must be obtained in other ways.
 */
#ifndef TIMES_RETS_STATUS
    struct tms cpuTimes;

    *realTime = TclX_OSTicksToMS (times (&cpuTimes));
    *cpuTime = TclX_OSTicksToMS (cpuTimes.tms_utime + cpuTimes.tms_stime);
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
    *cpuTime = TclX_OSTicksToMS (cpuTimes.tms_utime + cpuTimes.tms_stime);
#endif
}

/*-----------------------------------------------------------------------------
 * TclX_OSkill --
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
TclX_OSkill (interp, pid, signal, funcName)
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
 * TclX_OSGetOpenFileMode --
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
TclX_OSGetOpenFileMode (fileNum, mode, nonBlocking)
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
 * TclX_OSopendir --
 *   Portability interface to opendir functionallity.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.  Maybe NULL if error text is
 *     not to be returned.
 *   o path (I) - Path to the directory.
 *   o handlePtr (O) - The handle to pass in other functions to access this
 *     directory is returned here.
 *   o caseSensitive (O) - Are the file names case sensitive?  Always TRUE
 *     on Unix.
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
    DIR *handle;
    
    handle = opendir (path);
    if (handle == NULL)  {
        if (interp != NULL)
            Tcl_AppendResult (interp, "open of directory \"", path,
                              "\" failed: ", Tcl_PosixError (interp),
                              (char *) NULL);
        return TCL_ERROR;
    }
    *handlePtr = handle;
    *caseSensitive = TRUE;
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * TclX_OSreaddir --
 *   Portability interface to readdir functionallity.  The "." and ".." entries
 * are not returned.
 *
 * Parameters:
 *   o interp (I) - Errors returned in result.
 *   o handle (I) - The handle returned by TclX_OSopendir.
 *   o hidden (I) - Include hidden files.  Ignored on Unix.
 *   o fileNamePtr (O) - A pointer to the filename is returned here.
 * Results:
 *   TCL_OK, TCL_ERROR or TCL_BREAK if there are no more directory entries.
 *-----------------------------------------------------------------------------
 */
int
TclX_OSreaddir (interp, handle, hidden, fileNamePtr)
    Tcl_Interp     *interp;
    TCLX_DIRHANDLE  handle;
    int             hidden;
    char          **fileNamePtr;
{
    struct dirent *entryPtr;
   
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
        *fileNamePtr = entryPtr->d_name;
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
    if (closedir (handle) < 0) {
        if (interp != NULL)
            Tcl_AppendResult (interp, "close of directory failed: ",
                              Tcl_PosixError (interp), (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}
