/*
 * tclXfcntl.c
 *
 * Extended Tcl fcntl command.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1995 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXfcntl.c,v 4.4 1995/04/30 05:46:07 markd Exp markd $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Macro to enable line buffering mode on a file.  The macros assure that the
 * resulting expression returns zero if the function call does not return
 * a value.  Try for setvbuf first, as setlinebuf seems to be a no-op on 
 * DEC Ultrix.
 */
#if defined(HAVE_SETVBUF) && defined(_IOLBF)
#   define SET_LINE_BUF(fp)  setvbuf (fp, NULL, _IOLBF, BUFSIZ)
#else
#   define SET_LINE_BUF(fp)  (setlinebuf (fp),0)
#endif

/*
 * If we don't have O_NONBLOCK, use O_NDELAY.
 */
#ifndef O_NONBLOCK
#   define O_NONBLOCK O_NDELAY
#endif

/*
 * Attributes sets used by fcntl command.  Also a structure to return parsed
 * attribute information in.
 */
#define ATTR_NONE     0  /* No attributes in this class */

#define ATTR_RDONLY   1  /* Access checks desired.      */
#define ATTR_WRONLY   2
#define ATTR_RDWR     3
#define ATTR_READ     4
#define ATTR_WRITE    5

#define ATTR_CLOEXEC  1  /* Other attribute sets */
#define ATTR_NOBUF    2
#define ATTR_LINEBUF  4

typedef struct {
    int  access;
    int  fcntl;
    int  other;
} fcntlAttr_t;

/*
 * The maximum length of any attribute name.
 */
#define MAX_ATTR_NAME_LEN  20


/*
 * Determine the field names and defines used to determine if a file is line
 * buffered or not buffered. This is nasty, _IONBF is the System V flag and
 * _SNBF is the BSD flag.  However some systems using BSD also define _IONBF
 * (yuk). Also some BSDs use __SNBF.
 */

#ifdef HAVE_STDIO_FLAGS
#   define STDIO_FLAGS _flags
#endif
#ifdef HAVE_STDIO__FLAGS
#   define STDIO_FLAGS __flags
#endif
#ifdef HAVE_STDIO_FLAG
#   define STDIO_FLAGS _flag
#endif
#ifdef HAVE_STDIO__FLAG
#   define STDIO_FLAGS __flag
#endif
#ifndef STDIO_FLAGS
    Unable to determine stdio flags;
#endif


#ifdef _STDIO_USES_IOSTREAM  /* GNU libc */
#   define STDIO_NBUF _IONBF
#   define STDIO_LBUF 0x200
#endif
#if (!defined(STDIO_NBUF)) && (defined(_IONBF) && !defined(_SNBF))
#   define STDIO_NBUF _IONBF
#   define STDIO_LBUF _IOLBF
#endif
#if !defined(STDIO_NBUF)
#   define STDIO_NBUF _SNBF
#   define STDIO_LBUF _SLBF
#endif
#ifndef STDIO_NBUF
    Unable to determine stdio defines;
#endif


/*
 * Prototypes of internal functions.
 */
static int
XlateFcntlAttr  _ANSI_ARGS_((Tcl_Interp  *interp,
                             char        *attrName,
                             fcntlAttr_t *attrPtr));

static int
GetFcntlAttr _ANSI_ARGS_((Tcl_Interp *interp,
                          OpenFile   *tclFilePtr,
                          char       *attrName));

static int
SetAttrOnFile _ANSI_ARGS_((Tcl_Interp *interp,
                           FILE       *filePtr,
                           fcntlAttr_t attrib,
                           int         value));

static int
SetFcntlAttr _ANSI_ARGS_((Tcl_Interp *interp,
                          OpenFile   *tclFilePtr,
                          char       *attrName,
                          char       *valueStr));

/*
 *-----------------------------------------------------------------------------
 * XlateFcntlAttr --
 *    Translate an fcntl attribute.
 *
 * Parameters:
 *   o interp (I) - Tcl interpreter.
 *   o attrName (I) - The attrbute name to translate, maybe upper or lower
 *     case.
 *   o attrPtr (O) - Structure containing the parsed attributes.  Only one of
 *     the fields will be set, the others will be set to ATTR_NONE.
 * Result:
 *   Returns TCL_OK if all is well, TCL_ERROR if there is an error.
 *-----------------------------------------------------------------------------
 */
static int
XlateFcntlAttr (interp, attrName, attrPtr)
    Tcl_Interp  *interp;
    char        *attrName;
    fcntlAttr_t *attrPtr;
{
    char attrNameUp [MAX_ATTR_NAME_LEN];

    attrPtr->access = ATTR_NONE;
    attrPtr->fcntl = ATTR_NONE;
    attrPtr->other = ATTR_NONE;

    if (strlen (attrName) >= MAX_ATTR_NAME_LEN)
        goto invalidAttrName;

    Tcl_UpShift (attrNameUp, attrName);

    if (STREQU (attrNameUp, "RDONLY")) {
        attrPtr->access = ATTR_RDONLY;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "WRONLY")) {
        attrPtr->access = ATTR_WRONLY;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "RDWR")) {
        attrPtr->access = ATTR_RDWR;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "READ")) {
        attrPtr->access = ATTR_READ;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "WRITE")) {
        attrPtr->access = ATTR_WRITE;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "NONBLOCK")) {
        attrPtr->fcntl = O_NONBLOCK;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "APPEND")) {
        attrPtr->fcntl = O_APPEND;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "CLOEXEC")) {
        attrPtr->other = ATTR_CLOEXEC;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "NOBUF")) {
        attrPtr->other = ATTR_NOBUF;
        return TCL_OK;
    }
    if (STREQU (attrNameUp, "LINEBUF")) {
        attrPtr->other = ATTR_LINEBUF;
        return TCL_OK;
    }

    /*
     * Error return code.
     */
  invalidAttrName:
    Tcl_AppendResult (interp, "unknown attribute name \"", attrName,
                      "\", expected one of APPEND, CLOEXEC, LINEBUF, ",
                      "NONBLOCK, NOBUF, READ, RDONLY, RDWR, WRITE, WRONLY",
                      (char *) NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * GetFcntlAttr --
 *    Return the value of a specified fcntl attribute.
 *
 * Parameters:
 *   o interp (I) - Tcl interpreter, value is returned in the result
 *   o tclFilePtr (I) - Pointer to the Tcl file descriptor.
 *   o attrName (I) - The attrbute name to translate, maybe upper or lower
 *     case.
 * Result:
 *   Returns TCL_OK if all is well, TCL_ERROR if fcntl returns an error.
 *-----------------------------------------------------------------------------
 */
static int
GetFcntlAttr (interp, tclFilePtr, attrName)
    Tcl_Interp *interp;
    OpenFile   *tclFilePtr;
    char       *attrName;
{
    fcntlAttr_t attrib;
    int         current;

    if (XlateFcntlAttr (interp, attrName, &attrib) != TCL_OK)
        return TCL_ERROR;

    /*
     * Special handling for read/write check on files that have more than one
     * FILE struct associated with them.  They are always read/write.
     */
    if ((attrib.access != ATTR_NONE) && (tclFilePtr->f2 != NULL)) {
        interp->result =
            (attrib.access & (ATTR_READ | ATTR_WRITE)) ? "1" : "0";
        return TCL_OK;
    }

    /*
     * Access check on single FILE files.  We access fcntl just to make sure.
     */
    if (attrib.access != ATTR_NONE) {
        current = fcntl (fileno (tclFilePtr->f), F_GETFL, 0);
        if (current == -1)
            goto unixError;
        interp->result = "0";
        switch (current & O_ACCMODE) {
          case O_RDONLY:
            if ((attrib.access == ATTR_RDONLY) ||
                (attrib.access == ATTR_READ))
                interp->result = "1";
            break;
          case O_WRONLY:
            if ((attrib.access == ATTR_WRONLY) ||
                (attrib.access == ATTR_WRITE))
                interp->result = "1";
            break;
          case O_RDWR:
            if ((attrib.access == ATTR_RDWR) ||
                (attrib.access == ATTR_READ) ||
                (attrib.access == ATTR_WRITE))
                interp->result = "1";
            break;
        }
        return TCL_OK;
    }

    /*
     * For fcntl attributes.  For dual FILE files, we assume the
     * second file has the same attributes as the first.  This would be
     * true if the attributes were set via this function.
     */
    if (attrib.fcntl != ATTR_NONE) {
        current = fcntl (fileno (tclFilePtr->f), F_GETFL, 0);
        if (current == -1)
            goto unixError;
        interp->result = (current & attrib.fcntl) ? "1" : "0";
        return TCL_OK;
    }

    if (attrib.other == ATTR_CLOEXEC) {
        current = fcntl (fileno (tclFilePtr->f), F_GETFD, 0);
        if (current == -1)
            goto unixError;
        interp->result = (current & 1) ? "1" : "0";
        return TCL_OK;
    }

    /*
     * Poke the stdio FILE structure to determine the buffering status.
     * Also assume both files are the same.
     */
    if (attrib.other == ATTR_NOBUF) {
        interp->result = (tclFilePtr->f->STDIO_FLAGS & STDIO_NBUF) ? "1" : "0";
        return TCL_OK;
    }
    if (attrib.other == ATTR_LINEBUF) {
        interp->result = (tclFilePtr->f->STDIO_FLAGS & STDIO_LBUF) ? "1" : "0";
        return TCL_OK;
    }

  unixError:
    interp->result = Tcl_PosixError (interp);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * SetAttrOnFile --
 *    Set the the attributes on a file.  This is called twice for dual FILE
 * struct files.
 *
 * Parameters:
 *   o interp (I) - Tcl interpreter, value is returned in the result
 *   o filePtr (I) - Pointer to the file descriptor.
 *   o attrib (I) - Structure describing attribute to set.
 *   o value (I) - Boolean value to set the attributes to.
 * Result:
 *   Returns TCL_OK if all is well, TCL_ERROR if there is an error.
 *-----------------------------------------------------------------------------
 */
static int
SetAttrOnFile (interp, filePtr, attrib, value)
    Tcl_Interp *interp;
    FILE       *filePtr;
    fcntlAttr_t attrib;
    int         value;
{
    int current;
 
    if (attrib.fcntl != ATTR_NONE) {
        current = fcntl (fileno (filePtr), F_GETFL, 0);
        if (current == -1)
            goto unixError;
        current &= ~attrib.fcntl;
        if (value)
            current |= attrib.fcntl;
        if (fcntl (fileno (filePtr), F_SETFL, current) == -1)
            goto unixError;

        return TCL_OK;
    }

    if (attrib.other == ATTR_CLOEXEC) {
        if (fcntl (fileno (filePtr), F_SETFD, value) == -1)
            goto unixError;
        return TCL_OK;
    }

    if (attrib.other == ATTR_NOBUF) {
        setbuf (filePtr, NULL);
        return TCL_OK;
    }

    if (attrib.other == ATTR_LINEBUF) {
        if (SET_LINE_BUF (filePtr) != 0)
            goto unixError;
        return TCL_OK;
    }

  unixError:
    interp->result = Tcl_PosixError (interp);
    return TCL_ERROR;
   
}

/*
 *-----------------------------------------------------------------------------
 * SetFcntlAttr --
 *    Set the specified fcntl attr to the given value.
 *
 * Parameters:
 *   o interp (I) - Tcl interpreter, value is returned in the result
 *   o tclFilePtr (I) - Pointer to the tcl file descriptor.
 *   o attrName (I) - The attrbute name to translate, maybe upper or lower
 *     case.
 *   o valueStr (I) - The string value to set the attribiute to.
 * Result:
 *   Returns TCL_OK if all is well, TCL_ERROR if there is an error.
 *-----------------------------------------------------------------------------
 */
static int
SetFcntlAttr (interp, tclFilePtr, attrName, valueStr)
    Tcl_Interp *interp;
    OpenFile   *tclFilePtr;
    char       *attrName;
    char       *valueStr;
{
    fcntlAttr_t attrib;
    int         value;

    if (XlateFcntlAttr (interp, attrName, &attrib) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetBoolean (interp, valueStr, &value) != TCL_OK)
        return TCL_ERROR;

    /*
     * Validate that this the attribute may be set (or cleared).
     */

    if (attrib.access != ATTR_NONE) {
        Tcl_AppendResult (interp, "Attribute \"", attrName, "\" may not be ",
                          "altered after open", (char *) NULL);
        return TCL_ERROR;
    }

    if (!value) {
        if ((attrib.other == ATTR_NOBUF) || (attrib.other == ATTR_LINEBUF)) {
            Tcl_AppendResult (interp, "Attribute \"", attrName,
                              "\" may not be cleared once set",
                              (char *) NULL);
            return TCL_ERROR;
        }
    }

    if (SetAttrOnFile (interp, tclFilePtr->f, attrib, value) == TCL_ERROR)
        return TCL_ERROR;

    if (tclFilePtr->f2 != NULL) {
        if (SetAttrOnFile (interp, tclFilePtr->f2, attrib, value) == TCL_ERROR)
            return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * Tcl_FcntlCmd --
 *     Implements the fcntl TCL command:
 *         fcntl handle attribute ?value?
 *-----------------------------------------------------------------------------
 */
int
Tcl_FcntlCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    OpenFile *tclFilePtr;
    FILE     *filePtr;

    if ((argc < 3) || (argc > 4)) {
        Tcl_AppendResult (interp, tclXWrongArgs, argv [0], 
                          " handle attribute ?value?", (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetOpenFile (interp, argv [1], 
                         FALSE, FALSE,   /* No access checking */
                         &filePtr) != TCL_OK)
	return TCL_ERROR;
    tclFilePtr = tclOpenFiles [fileno (filePtr)];

    /*
     * Get or set attributes.  These functions handle more than
     * one FILE struct in a single Tcl file.
     */
    if (argc == 3) {    
        if (GetFcntlAttr (interp, tclFilePtr, argv [2]) != TCL_OK)
            return TCL_ERROR;
    } else {
        if (SetFcntlAttr (interp, tclFilePtr, argv [2], argv [3]) != TCL_OK)
            return TCL_ERROR;
    }
    return TCL_OK;
}
