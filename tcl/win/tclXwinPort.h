/*
 * tclXwinPort.h
 *
 * Portability include file for MS Windows systems.
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
 * $Id: tclExtdInt.h,v 5.14 1996/03/17 06:51:59 markd Exp $
 *-----------------------------------------------------------------------------
 */

#ifndef TCLXWINPORT_H
#define TCLXWINPORT_H

#include "tclWinPort.h"


#include <math.h>
#include <limits.h>

#ifndef MAXDOUBLE
#    define MAXDOUBLE HUGE_VAL
#endif
/*
 * Define a macro to call wait pid.  We don't use Tcl_WaitPid on Unix because
 * it delays signals.
 */
#define TCLX_WAITPID(pid, status, options) Tcl_WaitPid (pid, status, options)
#endif
