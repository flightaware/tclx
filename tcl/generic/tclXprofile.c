/*
 * tclXprofile.c --
 *
 * Tcl performance profile monitor.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1996 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXprofile.c,v 7.3 1996/08/17 05:42:03 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * For when the level is not known.
 */
#define UNKNOWN_LEVEL -1

/*
 * Stack entry used to keep track of an profiling information for procedures
 * (and commands in command mode).  This stack mirrors the Tcl procedure stack.
 * A chain of variable scope entries is also kept.  This tracks the uplevel
 * chain kept in the Tcl stack.  Unlike the Tcl stack, an entry is also make
 * for the global context and for commands when in command mode.
 */

typedef struct profEntry_t {
    clock_t             cumRealTime;   /* Cumulative real time.         */
    clock_t             cumCpuTime;    /* Cumulative CPU.               */
    int                 isProc;        /* Procedure, not command.       */
    int                 procLevel;     /* Procedure level.              */ 
    int                 scopeLevel;    /* Varaible scope level.         */ 
    int                 evalLevel;     /* Tcl_Eval level.               */ 
    struct profEntry_t *prevEntryPtr;  /* Procedure call stack.         */
    struct profEntry_t *prevScopePtr;  /* Procedure var scope chain.    */
    char                cmdName [1];   /* Command name. MUST BE LAST!   */
} profEntry_t;


/*
 * Data keeped on a stack snapshot.
 */
typedef struct profDataEntry_t {
    clock_t count;
    clock_t realTime;
    clock_t cpuTime;
} profDataEntry_t;

/*
 * Client data structure for profile command.  This contains all global
 * profiling information for the interpreter.
 */

typedef struct profInfo_t { 
    Tcl_Interp     *interp;            /* Interpreter this is for.           */
    Tcl_Trace       traceHandle;       /* Handle to current trace.           */
    int             allCommands;       /* Prof all commands, not just procs  */
    clock_t         lastRealTime;      /* Real and CPU time of last trace    */
    clock_t         lastCpuTime;       /* from profiling routines.           */
    profEntry_t    *stackPtr;          /* Proc/command nesting stack .       */
    profEntry_t    *scopeChainPtr;     /* Variable scope chain.              */
    Tcl_HashTable   profDataTable;     /* Cumulative time table, Keyed by    */
                                       /* call stack list.                   */
} profInfo_t;

/*
 * Argument to panic on logic errors.  Takes an id number.
 */
static char *PROF_PANIC = "TclX profile bug id = %d\n";

/*
 * Prototypes of internal functions.
 */
static void
PushEntry _ANSI_ARGS_((profInfo_t *infoPtr,
                       char       *cmdName,
                       int         isProc,
                       int         procLevel,
                       int         scopeLevel,
                       int         evalLevel));

static void
RecordData _ANSI_ARGS_((profInfo_t  *infoPtr,
                        profEntry_t *entryPtr));

static void
PopEntry _ANSI_ARGS_((profInfo_t *infoPtr));

static void
ProfTraceRoutine _ANSI_ARGS_((ClientData    clientData,
                              Tcl_Interp   *interp,
                              int           evalLevel,
                              char         *command,
                              int           (*cmdProc)(),
                              ClientData    cmdClientData,
                              int           argc,
                              char        **argv));

static void
CleanDataTable _ANSI_ARGS_((profInfo_t *infoPtr));

static void
InitializeProcStack _ANSI_ARGS_((profInfo_t *infoPtr,
                                 CallFrame  *framePtr));

static void
TurnOnProfiling _ANSI_ARGS_((profInfo_t *infoPtr,
                             int         allCommands));

static void
DeleteProfTrace _ANSI_ARGS_((profInfo_t *infoPtr));

static int
TurnOffProfiling _ANSI_ARGS_((Tcl_Interp *interp,
                              profInfo_t *infoPtr,
                              char       *varName));

static void
ProfMonCleanUp _ANSI_ARGS_((ClientData  clientData,
                            Tcl_Interp *interp));


/*-----------------------------------------------------------------------------
 * PushEntry --
 *   Push a procedure or command entry onto the stack.
 *
 * Parameters:
 *   o infoPtr - The global profiling info.
 *   o cmdName  The procedure or command name.
 *   o isProc - TRUE if its a proc, FALSE if other command.
 *   o procLevel - The procedure call level that the procedure or command will
 *     execute at.
 *   o scopeLevel - The procedure variable scope level that the commands local
 *     variables are at.
 *   o evalLevel - The eval level the command was executed at.  For procedures
 *     this is the level it was called at, since if the procedure's commands
 *     are logged, they will be an the next eval level.  Maybe be
 *     UNKNOWN_LEVEL.
 *-----------------------------------------------------------------------------
 */
static void
PushEntry (infoPtr, cmdName, isProc, procLevel, scopeLevel, evalLevel)
    profInfo_t *infoPtr;
    char       *cmdName;
    int         isProc;
    int         procLevel;
    int         scopeLevel;
    int         evalLevel;
{
    profEntry_t *entryPtr, *scanPtr;

    /*
     * Calculate the size of an entry.  One byte for name is in the entry.
     */
    entryPtr = (profEntry_t *) ckalloc (sizeof (profEntry_t) +
                                        strlen (cmdName));
    
    /*
     * Fill it in and push onto the stack.  Note that the procedures frame has
     * not yet been layed down or the procedure body eval execute, so the value
     * they will be in the procedure is recorded.
     */
    entryPtr->cumRealTime = 0;
    entryPtr->cumCpuTime = 0;
    entryPtr->isProc = (TclFindProc ((Interp *) infoPtr->interp,
                                     cmdName) != NULL);
    entryPtr->procLevel = procLevel;
    entryPtr->scopeLevel = scopeLevel;
    entryPtr->evalLevel = evalLevel;
    strcpy (entryPtr->cmdName, cmdName);

    /*
     * Push onto the stack and set the variable scope chain.  The variable
     * scope entry is chained to the first entry who's scope is less than ours
     * if this is a proc or less than or equal to ours if this is a command.
     */
    entryPtr->prevEntryPtr = infoPtr->stackPtr;
    infoPtr->stackPtr = entryPtr;

    scanPtr = infoPtr->scopeChainPtr;
    while ((scanPtr != NULL) && (scanPtr->procLevel > 0) &&
           ((isProc && (scanPtr->scopeLevel >= scopeLevel)) ||
            ((!isProc) && (scanPtr->scopeLevel > scopeLevel)))) {
        scanPtr = scanPtr->prevScopePtr;
        /*
         * Only global level can be NULL.
         */
        if (scanPtr == NULL)
            panic (PROF_PANIC, 1);
    }
    entryPtr->prevScopePtr = scanPtr;
    infoPtr->scopeChainPtr = entryPtr;
}

/*-----------------------------------------------------------------------------
 * RecordData --
 *   Record an entries times in the data table.
 *
 * Parameters:
 *   o infoPtr - The global profiling info.
 *   o entryPtr - The entry to record.
 *-----------------------------------------------------------------------------
 */
static void
RecordData (infoPtr, entryPtr)
    profInfo_t  *infoPtr;
    profEntry_t *entryPtr;
{
    int idx, stackSize, newEntry;
    profEntry_t *scanPtr;
    char **stackArgv, *stackListPtr;
    Tcl_HashEntry *hashEntryPtr;
    profDataEntry_t *dataEntryPtr;

    /*
     * Build up a stack list.  Entry [0] is the top of the stack.
     */
    

    for (stackSize = 0, scanPtr = entryPtr; scanPtr != NULL;
         scanPtr = scanPtr->prevScopePtr) {
        stackSize++;
    }

    stackArgv = (char **) ckalloc (sizeof (char *) * stackSize);

    
    for (idx= 0, scanPtr = entryPtr; scanPtr != NULL;
         scanPtr = scanPtr->prevScopePtr) {
        stackArgv [idx++] = scanPtr->cmdName;
    }
    stackListPtr = Tcl_Merge (idx, stackArgv);
    ckfree ((char *) stackArgv);

    /*
     * Check the hash table for this entry, either finding an existing or
     * creating a new hash entry.
     */

    hashEntryPtr = Tcl_CreateHashEntry (&infoPtr->profDataTable,
                                        stackListPtr,
                                        &newEntry);
    ckfree (stackListPtr);

    /*
     * Either get the existing entry or create a new one.
     */
    if (newEntry) {
        dataEntryPtr = (profDataEntry_t *) ckalloc (sizeof (profDataEntry_t));
        Tcl_SetHashValue (hashEntryPtr, dataEntryPtr);
        dataEntryPtr->count = 0;
        dataEntryPtr->realTime = 0;
        dataEntryPtr->cpuTime  = 0;
    } else {
        dataEntryPtr = (profDataEntry_t *) Tcl_GetHashValue (hashEntryPtr);
    }

    /*
     * Increment the cumulative data.
     */
    dataEntryPtr->count++;
    dataEntryPtr->realTime += entryPtr->cumRealTime;
    dataEntryPtr->cpuTime += entryPtr->cumCpuTime;
}

/*-----------------------------------------------------------------------------
 * PopEntry --
 *   Pop the procedure entry from the top of the stack and record its
 * times in the data table.
 *
 * Parameters:
 *   o infoPtr - The global profiling info.
 *-----------------------------------------------------------------------------
 */
static void
PopEntry (infoPtr)
    profInfo_t *infoPtr;
{
    profEntry_t *entryPtr = infoPtr->stackPtr;

    RecordData (infoPtr, entryPtr);

    /*
     * Remove from the stack, reset the scope chain and free.
     */
    infoPtr->stackPtr = entryPtr->prevEntryPtr;
    infoPtr->scopeChainPtr = infoPtr->stackPtr;

    ckfree ((char *) entryPtr);
}

/*-----------------------------------------------------------------------------
 * ProfTraceRoutine --
 *  Routine called by Tcl_Eval to do profiling.
 *-----------------------------------------------------------------------------
 */
static void
ProfTraceRoutine (clientData, interp, evalLevel, command, cmdProc,
                  cmdClientData, argc, argv)
    ClientData    clientData;
    Tcl_Interp   *interp;
    int           evalLevel;
    char         *command;
    int           (*cmdProc)();
    ClientData    cmdClientData;
    int           argc;
    char        **argv;
{
    Interp *iPtr = (Interp *) interp;
    profInfo_t *infoPtr = (profInfo_t *) clientData;
    CallFrame *framePtr;
    int procLevel, scopeLevel, isProc;
    clock_t realTime, cpuTime, deltaRealTime, deltaCpuTime;
    profEntry_t *scanStackPtr;

    /*
     * Determine current proc and var levels.
     */
    procLevel = 0;
    for (framePtr = iPtr->framePtr; framePtr != NULL; framePtr =
             framePtr->callerPtr) {
        procLevel++;
    }
    scopeLevel = (iPtr->varFramePtr == NULL) ? 0 : iPtr->varFramePtr->level;

    /*
     * Calculate the time spent since the last trace.
     */
    TclXOSElapsedTime (&realTime, &cpuTime);
    deltaRealTime = (realTime - infoPtr->lastRealTime);
    deltaCpuTime = (cpuTime - infoPtr->lastCpuTime);

    /*
     * Update cumulative times for the scope chain.
     */
    for (scanStackPtr = infoPtr->scopeChainPtr; scanStackPtr != NULL; 
         scanStackPtr = scanStackPtr->prevScopePtr) {
        scanStackPtr->cumRealTime += deltaRealTime;
        scanStackPtr->cumCpuTime += deltaCpuTime;
    }

    /* 
     * Pop entries for commands or procedures that completed in the previous
     * eval.
     */
    while ((infoPtr->stackPtr->procLevel > procLevel) ||
           ((infoPtr->stackPtr->evalLevel >= evalLevel) &&
            (infoPtr->stackPtr->evalLevel != UNKNOWN_LEVEL))) {
        PopEntry (infoPtr);
    }

    /*
     * If this command is a procedure or if all commands are being traced,
     * handle the entry.
     */
     isProc = (TclFindProc (iPtr, argv [0]) != NULL);
#ifdef ITCL_NAMESPACES      
    if (infoPtr->allCommands || isProc) {
        char *commandNamesp;
        Tcl_DString commandWithNs;

        /*
         * Append the namespace of the command.
         */
        Tcl_DStringInit (&commandWithNs);
        commandNamesp = Itcl_GetNamespPath (Itcl_GetActiveNamesp (interp));
	if (commandNamesp != NULL) {
            Tcl_DStringAppend (&commandWithNs, commandNamesp, -1);

            /*
             * If the argument does not have a leading ::, then append one.
             */
            if (!STRNEQU (argv[0], "::", 2)) 
                Tcl_DStringAppend (&commandWithNs, "::", -1); 
	}
	Tcl_DStringAppend (&commandWithNs, argv[0], -1); 
        if (isProc) {
            PushEntry (infoPtr, Tcl_DStringValue (&commandWithNs), TRUE,
                       procLevel + 1, scopeLevel + 1, evalLevel);
        } else {
            PushEntry (infoPtr, Tcl_DStringValue (&commandWithNs), FALSE,
                       procLevel, scopeLevel, evalLevel);
        }
	Tcl_DStringFree (&commandWithNs);
    }
#else
    if (infoPtr->allCommands || isProc) {
        if (isProc) {
            PushEntry (infoPtr, argv [0], TRUE,
                       procLevel + 1, scopeLevel + 1, evalLevel);
        } else {
            PushEntry (infoPtr, argv [0], FALSE,
                       procLevel, scopeLevel, evalLevel);
        }
    }
#endif

    /*
     * Save the exit time of the profiling trace handler.
     */
    TclXOSElapsedTime (&infoPtr->lastRealTime,
                       &infoPtr->lastCpuTime);
}

/*-----------------------------------------------------------------------------
 * CleanDataTable --
 *    Clean up the hash data table, releasing all resources and setting it
 * to the empty state.
 *
 * Parameters:
 *   o infoPtr - The global profiling info.
 *-----------------------------------------------------------------------------
 */
static void
CleanDataTable (infoPtr)
    profInfo_t *infoPtr;
{
    Tcl_HashEntry    *hashEntryPtr;
    Tcl_HashSearch   searchCookie;

    hashEntryPtr = Tcl_FirstHashEntry (&infoPtr->profDataTable,
                                       &searchCookie);
    while (hashEntryPtr != NULL) {
        ckfree ((char *) Tcl_GetHashValue (hashEntryPtr));
        Tcl_DeleteHashEntry (hashEntryPtr);
        hashEntryPtr = Tcl_NextHashEntry (&searchCookie);
    }
}

/*-----------------------------------------------------------------------------
 * InitializeProcStack --
 *    Recursive procedure to initialize the procedure call stack so its in the
 * same state as the actual procedure  call stack.  If allCommands are enable,
 * they still are not initialized on the stack.
 *
 * Parameters:
 *   o infoPtr - The global profiling info.
 *   o framePtr - Pointer to the frame to push.  We recurse down to the bottom,
 *     then push on the way out.
 *-----------------------------------------------------------------------------
 */
static void
InitializeProcStack (infoPtr, framePtr)
    profInfo_t *infoPtr;
    CallFrame  *framePtr;
{
    if (framePtr == NULL)
        return;
    InitializeProcStack (infoPtr, framePtr->callerPtr);
    
    PushEntry (infoPtr, framePtr->argv[0], TRUE,
               infoPtr->stackPtr->procLevel + 1,
               framePtr->level,
               UNKNOWN_LEVEL);
}

/*-----------------------------------------------------------------------------
 * TurnOnProfiling --
 *    Turn on profiling.
 *
 * Parameters:
 *   o infoPtr - The global profiling info.
 *   o allCommands - TRUE if all commands are going to be logged, FALSE if just
 *     procs.
 *-----------------------------------------------------------------------------
 */
static void
TurnOnProfiling (infoPtr, allCommands)
    profInfo_t *infoPtr;
    int         allCommands;
{
    Interp *iPtr = (Interp *) infoPtr->interp;
    int scopeLevel;
    profEntry_t *scanPtr;

    CleanDataTable (infoPtr);

    infoPtr->traceHandle =
        Tcl_CreateTrace (infoPtr->interp, MAXINT,
                         (Tcl_CmdTraceProc *) ProfTraceRoutine,
                         (ClientData) infoPtr);
    TclXOSElapsedTime (&infoPtr->lastRealTime,
                       &infoPtr->lastCpuTime);
    infoPtr->allCommands = allCommands;
    
    /*
     * Add entry for global context, then add in current procedures.
     */
    PushEntry (infoPtr, "<global>", TRUE, 0, 0, 0);
    InitializeProcStack (infoPtr, ((Interp *) infoPtr->interp)->framePtr);

    /*
     * Find the current top of the scope stack.
     */
    scopeLevel = (iPtr->varFramePtr == NULL) ? 0 : iPtr->varFramePtr->level;
    scanPtr = infoPtr->scopeChainPtr;
    while ((scanPtr != NULL) && (scanPtr->scopeLevel >= scopeLevel) &&
           (scanPtr->procLevel > 0)) {
        scanPtr = scanPtr->prevScopePtr;
        /*
         * Only global level can be NULL.
         */
        if (scanPtr == NULL)
            panic (PROF_PANIC, 1);
    }
    infoPtr->scopeChainPtr = scanPtr;
}

/*-----------------------------------------------------------------------------
 * DeleteProfTrace --
 *   Delete the profile trace and clean up the stack, logging all procs
 * as if they had exited.  Data table must still be available.
 *
 * Parameters:
 *   o infoPtr - The global profiling info.
 *-----------------------------------------------------------------------------
 */
static void
DeleteProfTrace (infoPtr)
    profInfo_t *infoPtr;
{
    Tcl_DeleteTrace (infoPtr->interp, infoPtr->traceHandle);
    infoPtr->traceHandle = NULL;

    while (infoPtr->stackPtr != NULL) {
        PopEntry (infoPtr);
    }
}

/*-----------------------------------------------------------------------------
 * TurnOffProfiling --
 *   Turn off profiling.  Dump the table data to an array variable.  Entries
 * will be deleted as they are dumped to limit memory utilization.
 *
 * Parameters:
 *   o interp - Pointer to the interprer.
 *   o infoPtr - The global profiling info.
 *   o varName - The name of the variable to save the data in.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
TurnOffProfiling (interp, infoPtr, varName)
    Tcl_Interp *interp;
    profInfo_t *infoPtr;
    char       *varName;
{
    Tcl_HashEntry *hashEntryPtr;
    Tcl_HashSearch searchCookie;
    profDataEntry_t *dataEntryPtr;
    char *dataArgv [3], *dataListPtr;
    char countBuf [32], realTimeBuf [32], cpuTimeBuf [32];

    DeleteProfTrace (infoPtr);

    dataArgv [0] = countBuf;
    dataArgv [1] = realTimeBuf;
    dataArgv [2] = cpuTimeBuf;

    Tcl_UnsetVar (interp, varName, 0);
    hashEntryPtr = Tcl_FirstHashEntry (&infoPtr->profDataTable,
                                       &searchCookie);
    while (hashEntryPtr != NULL) {
        dataEntryPtr = 
            (profDataEntry_t *) Tcl_GetHashValue (hashEntryPtr);

        sprintf (countBuf, "%ld", dataEntryPtr->count);
        sprintf (realTimeBuf,"%ld", dataEntryPtr->realTime);
        sprintf (cpuTimeBuf, "%ld", dataEntryPtr->cpuTime);

        dataListPtr = Tcl_Merge (3, dataArgv);

        if (Tcl_SetVar2 (interp, varName,
                         Tcl_GetHashKey (&infoPtr->profDataTable,
                                         hashEntryPtr),
                         dataListPtr, TCL_LEAVE_ERR_MSG) == NULL) {
            ckfree (dataListPtr);
            return TCL_ERROR;
        }
        ckfree (dataListPtr);
        ckfree ((char *) dataEntryPtr);
        Tcl_DeleteHashEntry (hashEntryPtr);

        hashEntryPtr = Tcl_NextHashEntry (&searchCookie);
    }

    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_ProfileCmd --
 *   Implements the TCL profile command:
 *     profile ?-commands? on
 *     profile off arrayvar
 *-----------------------------------------------------------------------------
 */
static int
Tcl_ProfileCmd (clientData, interp, argc, argv)
    ClientData    clientData;
    Tcl_Interp   *interp;
    int           argc;
    char        **argv;
{
    profInfo_t  *infoPtr = (profInfo_t *) clientData;
    int          idx;
    int          cmdArgc,   optionsArgc = 0;
    char       **cmdArgv, **optionsArgv = &(argv [1]);

    /*
     * Scan for options (currently only one is supported).  Set cmdArgv to
     * contain the rest of the command following the options.
     */
    for (idx = 1; (idx < argc) && (argv [idx][0] == '-'); idx++)
        optionsArgc++;
    cmdArgc = argc - idx;
    cmdArgv = &(argv [idx]);

    if (cmdArgc < 1)
        goto wrongArgs;

    /*
     * Handle the on command.
     */
    if (STREQU (cmdArgv [0], "on")) {
        int allCommands = FALSE;

        if ((cmdArgc != 1) || (optionsArgc > 1))
            goto wrongArgs;

        if (optionsArgc == 1) {
            if (!STREQU (optionsArgv [0], "-commands")) {
                Tcl_AppendResult (interp, "expected option of \"-commands\", ",
                                  "got \"", optionsArgv [0], "\"",
                                  (char *) NULL);
                return TCL_ERROR;
            }
            allCommands = TRUE;
        }

        if (infoPtr->traceHandle != NULL) {
            Tcl_AppendResult (interp, "profiling is already enabled",
                              (char *) NULL);
            return TCL_ERROR; 
       }

        TurnOnProfiling (infoPtr, allCommands);
        return TCL_OK;
    }

    /*
     * Handle the off command.  Dump the hash table to a variable.
     */
    if (STREQU (cmdArgv [0], "off")) {

        if ((cmdArgc != 2) || (optionsArgc > 0))
            goto wrongArgs;

        if (infoPtr->traceHandle == NULL) {
            Tcl_AppendResult (interp, "profiling is not currently enabled",
                              (char *) NULL);
            return TCL_ERROR;
        }
            
        if (TurnOffProfiling (interp, infoPtr, argv [2]) != TCL_OK)
            return TCL_ERROR;
        return TCL_OK;
    }

    /*
     * Not a valid subcommand.
     */
    Tcl_AppendResult (interp, "expected one of \"on\" or \"off\", got \"",
                      argv [1], "\"", (char *) NULL);
    return TCL_ERROR;

  wrongArgs:
    Tcl_AppendResult (interp, tclXWrongArgs, argv [0],
                      " ?-commands? on|off arrayVar", (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * ProfMonCleanUp --
 *   Release the client data area when the interpreter is deleted.
 *-----------------------------------------------------------------------------
 */
static void
ProfMonCleanUp (clientData, interp)
    ClientData  clientData;
    Tcl_Interp *interp;
{
    profInfo_t *infoPtr = (profInfo_t *) clientData;

    if (infoPtr->traceHandle != NULL)
        DeleteProfTrace (infoPtr);
    CleanDataTable (infoPtr);
    Tcl_DeleteHashTable (&infoPtr->profDataTable);
    ckfree ((char *) infoPtr);
}

/*-----------------------------------------------------------------------------
 * Tcl_InitProfile --
 *   Initialize the Tcl profiling command.
 *-----------------------------------------------------------------------------
 */
void
Tcl_InitProfile (interp)
    Tcl_Interp *interp;
{
    profInfo_t *infoPtr;

    infoPtr = (profInfo_t *) ckalloc (sizeof (profInfo_t));

    infoPtr->interp = interp;
    infoPtr->traceHandle = NULL;
    infoPtr->stackPtr = NULL;
    infoPtr->scopeChainPtr = NULL;
    Tcl_InitHashTable (&infoPtr->profDataTable, TCL_STRING_KEYS);

    Tcl_CallWhenDeleted (interp, ProfMonCleanUp, (ClientData) infoPtr);

    Tcl_CreateCommand (interp, "profile", Tcl_ProfileCmd, 
                       (ClientData) infoPtr, (Tcl_CmdDeleteProc*) NULL);
}

