/* acconfig.h
   This file is used by Autoheader.  It is in the public domain.
*/

/* Define as the proper declaration for yytext.  */
#undef DECLARE_YYTEXT

/* Define if you have dirent.h.  */
#undef DIRENT

/* Define if you support file names longer than 14 characters.  */
#undef HAVE_LONG_FILE_NAMES

/* Define if you have netdb.h (and thus the rexec function).  */
#undef HAVE_NETDB_H

/* Define if you don't have sys/socket.h */
#undef NO_SYS_SOCKET_H

/* Define if you don't have sys/select.h */
#undef NO_SYS_SELECT_H

/* Define if you have and need sys/select.h */
#undef HAVE_SYS_SELECT_H

/* Define if don't you have values.h */
#undef NO_VALUES_H

/* Define if system calls automatically restart after interruption
   by a signal.  */
#undef HAVE_RESTARTABLE_SYSCALLS

/* Define if your struct tm has tm_zone.  */
#undef HAVE_TM_ZONE

/* Define if you have the timezone variable.  */
#undef HAVE_TIMEZONE_VAR

/* Define if your struct tm has tm_tzadj.  */
#undef HAVE_TM_TZADJ

/* Define if your struct tm has tm_gmtoff.  */
#undef HAVE_TM_GMTOFF

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
#undef HAVE_TZNAME

/* Define if you have unistd.h.  */
#undef HAVE_UNISTD_H

/* Define if int is 16 bits instead of 32.  */
#undef INT_16_BITS

/* Define if you have memory.h, and string.h doesn't declare the
   mem* functions.  */
#undef NEED_MEMORY_H

/* Define if you have neither a remote shell nor the rexec function.  */
#undef NO_REMOTE

/* Define as the return type of signal handlers (int or void).  */
#undef RETSIGTYPE

/* Define if the setvbuf function takes the buffering type as its second
   argument and the buffer pointer as the third, as on System V
   before release 3.  */
#undef SETVBUF_REVERSED

/* Define if you have the ANSI C header files.  */
#undef STDC_HEADERS

/* Define if you don't have dirent.h, but have sys/dir.h.  */
#undef SYSDIR

/* Define if you don't have dirent.h, but have sys/ndir.h.  */
#undef SYSNDIR

/* Define if your sys/time.h declares struct tm.  */
#undef TM_IN_SYS_TIME

/* Define if you do not have strings.h, index, bzero, etc..  */
#undef USG

/* Define if the closedir function returns void instead of int.  */
#undef VOID_CLOSEDIR

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
#undef _ALL_SOURCE
#endif

/* Define if you have a <sys/wait.h> with `union wait'.  */
#undef HAVE_SYS_WAIT

/* Define if you have the POSIX.1 `waitpid' function.  */
#undef HAVE_WAITPID

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef uid_t
#undef gid_t
#undef pid_t
#undef mode_t

/* Define to `long' if <sys/types.h> doesn't define.  */
#undef clock_t
#undef time_t

/* Define if the `getloadavg' function needs to be run setuid or setgid.  */
#undef GETLOADAVG_PRIVILEGED

/* Define if `union wait' is the type of the first arg to wait functions.  */
#undef HAVE_UNION_WAIT

/* Define if `sys_siglist' is declared by <signal.h>.  */
#undef SYS_SIGLIST_DECLARED

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
#undef GETGROUPS_T

/* Define if the `long double' type works.  */
#undef HAVE_LONG_DOUBLE

/* Define if you do not have dirent.h */
#undef NO_DIRENT_H

/* Define if you do not have readdir but you do have V7-style directories */
#undef USE_DIRENT2_H

/* Define if you do not have float.h */
#undef NO_FLOAT_H

/* Define if you do not have limits.h */
#undef NO_LIMITS_H

/* Define if you do not have stdlib.h */
#undef NO_STDLIB_H

/* Define if you do not have string.h */
#undef NO_STRING_H

/* Define if you do not have sys/time.h */
#undef NO_SYS_TIME_H

/* Define if you do not have sys/wait.h */
#undef NO_SYS_WAIT_H

/* Define if time.h can be included with sys/time.h */
#undef TIME_WITH_SYS_TIME

/* Define if wait does not return a union */
#undef NO_UNION_WAIT

/* Define if "times" returns the elasped real time */
#undef TIMES_RETS_REAL_TIME

/* Define if gettimeofday is not available */
#undef NO_GETTOD

/* Define if errno.h is not available */
#undef NO_ERRNO_H

/* Define if fd_set is not defined. */
#undef NO_FD_SET

/* Define if have gethostbyname (sockets). */
#undef HAVE_GETHOSTBYNAME

/* Define if stdlib.h defines random */
#undef STDLIB_DEFS_RANDOM

/* Define if we must use our own version of random */
#undef NO_RANDOM

/* Define if we have catgets and friends */
#undef HAVE_CATGETS

/* Define if catclose is type void instead of int */
#undef BAD_CATCLOSE

/* Define struct msghdr contains the msg_accrights field */
#undef HAVE_MSG_ACCRIGHTS

/* Define if inet_aton function is available. */
#undef HAVE_INET_ATON
