/*
 * tclXunixPort.h
 *
 * Portability include file for Unix systems.
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
 * $Id: tclXunixPort.h,v 5.2 1996/03/19 07:53:03 markd Exp $
 *-----------------------------------------------------------------------------
 */

#ifndef TCLXUNIXPORT_H
#define TCLXUNIXPORT_H

#include <sys/param.h>

#include <math.h>

#ifdef NO_LIMITS_H
#    include <values.h>
#else
#    include <limits.h>
#endif

#ifndef MAXDOUBLE
#    define MAXDOUBLE HUGE_VAL
#endif

#include <sys/times.h>
#include <grp.h>

/*
 * Included the tcl file tclUnixPort.h after other system files, as it checks
 * if certain things are defined.
 */
#include "tclUnixPort.h"

/*
 * Define O_ACCMODE if <fcntl.h> does not define it.
 */
#ifndef O_ACCMODE
#    define O_ACCMODE  (O_RDONLY|O_WRONLY|O_RDWR)
#endif

/*
 * Make sure we have both O_NONBLOCK and O_NDELAY defined.
 */
#ifndef O_NONBLOCK
#   define O_NONBLOCK O_NDELAY
#endif
#ifndef O_NDELAY
#   define O_NDELAY O_NONBLOCK
#endif

/*
 * Make sure CLK_TCK is defined.
 */
#ifndef CLK_TCK
#    ifdef HZ
#        define CLK_TCK HZ
#    else
#        define CLK_TCK 60
#    endif
#endif

/*
 * Define a macro to call wait pid.  We don't use Tcl_WaitPid on Unix because
 * it delays signals.
 */
#define TCLX_WAITPID(pid, status, options) waitpid (pid, status, options)

/*
 * Handle used to access directories.
 */
#define TCLX_DIRHANDLE DIR *

#endif
