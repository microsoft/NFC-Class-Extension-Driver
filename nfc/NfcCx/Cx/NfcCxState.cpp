/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NfcCxState.cpp

Abstract:

    This module implements state management in the NFC CX

--*/

#include "NfcCxPch.h"

#include "NFcCxState.tmh"

//
// State Connector
//
static const PFN_NFCCX_STATE_CONNECTOR g_NfcCxStateConnectorMap[] = {
    &NfcCxRFInterfaceConnChkDiscMode,
    &NfcCxRFInterfaceConnChkDeviceType,
};

//
// State Connector State Map
//
static const NFCCX_CX_STATE g_NfcCxState2StateMap[NfcCxStateMax][(NfcCxStateConnMax-NfcCxStateMax-1) * 2] = {
                                    /*StateConnChkDiscMode*/                            /*StateConnChkDeviceType*/
    /*StateIdle*/           { NfcCxStateNone,             NfcCxStateNone,       NfcCxStateNone,          NfcCxStateNone     },
    /*StateInit*/           { NfcCxStateNone,             NfcCxStateNone,       NfcCxStateNone,          NfcCxStateNone     },
    /*StateRfIdle*/         { NfcCxStateRfDiscovery,      NfcCxStateRfIdle,     NfcCxStateNone,          NfcCxStateNone     },
    /*StateRfDiscovery*/    { NfcCxStateRfDiscovery,      NfcCxStateRfIdle,     NfcCxStateNone,          NfcCxStateNone     },
    /*StateRfDiscovered*/   { NfcCxStateRfDiscovery,      NfcCxStateRfIdle,     NfcCxStateRfDataXchg,    NfcCxStateNone     },
    /*StateRfDataXchg*/     { NfcCxStateRfDiscovery,      NfcCxStateRfIdle,     NfcCxStateRfDiscovered,  NfcCxStateNone     },
    /*StateRecovery*/       { NfcCxStateRfDiscovery,      NfcCxStateRfIdle,     NfcCxStateNone,          NfcCxStateNone     },
    /*StateShutdown*/       { NfcCxStateNone,             NfcCxStateNone,       NfcCxStateNone,          NfcCxStateNone     },
};

//
// User Event to State Map
//
static const NFCCX_CX_STATE g_NfcCxUsrEvent2StateMap[NfcCxStateMax][NfcCxEventUserMax] = {
                            /*EventInit*/           /*EventDeinit*/             /*EventConfigDiscovery*/    /*EventStopDiscovery*/  /*EventActivate*/               /*EventDeactivateSleep*/        /*EventConfig*/         /*EventDataXchg*/
    /*StateIdle*/           { NfcCxStateInit,       NfcCxStateNone,             NfcCxStateNone,             NfcCxStateNone,         NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone          },
    /*StateInit*/           { NfcCxStateNone,       NfcCxStateNone,             NfcCxStateNone,             NfcCxStateNone,         NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone          },
    /*StateRfIdle*/         { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateConnChkDiscMode,  NfcCxStateRfIdle,       NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateRfIdle,       NfcCxStateNone          },
    /*StateRfDiscovery*/    { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateConnChkDiscMode,  NfcCxStateRfIdle,       NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateRfDiscovery,  NfcCxStateNone          },
    /*StateRfDiscovered*/   { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateConnChkDiscMode,  NfcCxStateRfIdle,       NfcCxStateConnChkDeviceType,    NfcCxStateNone,                 NfcCxStateRfDiscovered, NfcCxStateNone          },
    /*StateRfDataXchg*/     { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateConnChkDiscMode,  NfcCxStateRfIdle,       NfcCxStateNone,                 NfcCxStateConnChkDeviceType,    NfcCxStateRfDataXchg,   NfcCxStateRfDataXchg    },
    /*StateRecovery*/       { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateNone,             NfcCxStateNone,         NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone          },
    /*StateShutdown*/       { NfcCxStateNone,       NfcCxStateNone,             NfcCxStateNone,             NfcCxStateNone,         NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone          },
};

//
// Internal Event to State Map
//
static const NFCCX_CX_STATE g_NfcCxIntEvent2StateMap[NfcCxStateMax][NfcCxEventInternalMax-NfcCxEventUserMax-1] = {
                            /*EventReqCompleted*/           /*EventTimeout*/        /*EventRecovery*/       /*EventDiscovered*/     /*EventActivated*/      /*EventDeactivated*/        /*EventFailed*/
    /*StateIdle*/           { NfcCxStateIdle,               NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,             NfcCxStateNone            },
    /*StateInit*/           { NfcCxStateRfIdle,             NfcCxStateShutdown,     NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,             NfcCxStateShutdown        },
    /*StateRfIdle*/         { NfcCxStateRfIdle,             NfcCxStateRecovery,     NfcCxStateRecovery,     NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,             NfcCxStateRfIdle          },
    /*StateRfDiscovery*/    { NfcCxStateRfDiscovery,        NfcCxStateRecovery,     NfcCxStateRecovery,     NfcCxStateRfDiscovered, NfcCxStateNone,         NfcCxStateNone,             NfcCxStateRfDiscovery     },
    /*StateRfDiscovered*/   { NfcCxStateRfDiscovered,       NfcCxStateRecovery,     NfcCxStateRecovery,     NfcCxStateNone,         NfcCxStateRfDataXchg,   NfcCxStateConnChkDiscMode,  NfcCxStateConnChkDiscMode },
    /*StateRfDataXchg*/     { NfcCxStateRfDataXchg,         NfcCxStateRecovery,     NfcCxStateRecovery,     NfcCxStateNone,         NfcCxStateNone,         NfcCxStateConnChkDiscMode,  NfcCxStateRfDataXchg      },
    /*StateRecovery*/       { NfcCxStateConnChkDiscMode,    NfcCxStateIdle,         NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,             NfcCxStateIdle            },
    /*StateShutdown*/       { NfcCxStateIdle,               NfcCxStateShutdown,     NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,             NfcCxStateIdle            },
};

//
// State Handlers
//
static const PFN_NFCCX_STATE_HANDLER g_NfcCxStateHandlers[NfcCxStateMax][NfcCxStateMax] = {
                            /*StateIdle*/       /*StateInit*/                     /*StateRfIdle*/                          /*StateRfDiscovery*/                        /*StateRfDiscovered*/                            /*StateRfDataXchg*/                             /*StateRecovery*/               /*StateShutdown*/
    /*StateIdle*/           {  NULL,            NfcCxRFInterfaceStateInitialize,  NULL,                                    NULL,                                       NULL,                                            NULL,                                           NULL,                           NULL                            },
    /*StateInit*/           {  NULL,            NULL,                             NULL,                                    NULL,                                       NULL,                                            NULL,                                           NULL,                           NfcCxRFInterfaceStateShutdown   },
    /*StateRfIdle*/         {  NULL,            NULL,                             NfcCxRFInterfaceStateRfIdle,             NfcCxRFInterfaceStateRfIdle2RfDiscovery,    NULL,                                            NULL,                                           NfcCxRFInterfaceStateRecovery,  NfcCxRFInterfaceStateShutdown   },
    /*StateRfDiscovery*/    {  NULL,            NULL,                             NfcCxRFInterfaceStateRfDiscovery2RfIdle, NfcCxRFInterfaceStateRfDiscovery,           NfcCxRFInterfaceStateRfDiscovery2RfDiscovered,   NULL,                                           NfcCxRFInterfaceStateRecovery,  NfcCxRFInterfaceStateShutdown   },
    /*StateRfDiscovered*/   {  NULL,            NULL,                             NfcCxRFInterfaceStateRfActive2RfIdle,    NfcCxRFInterfaceStateRfActive2RfDiscovery,  NfcCxRFInterfaceStateRfDiscovered,               NfcCxRFInterfaceStateRfDiscovered2RfDataXchg,   NfcCxRFInterfaceStateRecovery,  NfcCxRFInterfaceStateShutdown   },
    /*StateRfDataXchg*/     {  NULL,            NULL,                             NfcCxRFInterfaceStateRfActive2RfIdle,    NfcCxRFInterfaceStateRfActive2RfDiscovery,  NfcCxRFInterfaceStateRfDataXchg2RfDiscovered,    NfcCxRFInterfaceStateRfDataXchg,                NfcCxRFInterfaceStateRecovery,  NfcCxRFInterfaceStateShutdown   },
    /*StateRecovery*/       {  NULL,            NULL,                             NULL,                                    NfcCxRFInterfaceStateRfIdle2RfDiscovery,    NULL,                                            NULL,                                           NULL,                           NfcCxRFInterfaceStateShutdown   },
    /*StateShutdown*/       {  NULL,            NULL,                             NULL,                                    NULL,                                       NULL,                                            NULL,                                           NULL,                           NULL                            },
};

NTSTATUS
NfcCxStateInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Out_ PNFCCX_STATE_INTERFACE * PPStateInterface
    )
/*++

Routine Description:

    This routine creates and initalizes the State Interface.

Arguments:

    FdoContext - A pointer to the FdoContext
    PPStateInterface - A pointer to a memory location to receive the allocated State interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(FdoContext);
    NT_ASSERT(PPStateInterface);

    *PPStateInterface = (PNFCCX_STATE_INTERFACE)malloc(sizeof(NFCCX_STATE_INTERFACE));
    if (NULL == *PPStateInterface) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    RtlZeroMemory(*PPStateInterface, sizeof(NFCCX_STATE_INTERFACE));

    (*PPStateInterface)->FdoContext = FdoContext;
    (*PPStateInterface)->CurrentState = NfcCxStateIdle;
    (*PPStateInterface)->TransitionFlag = NfcCxTransitionIdle;
    (*PPStateInterface)->CurrentUserEvent = NfcCxEventInvalid;

    InitializeListHead(&(*PPStateInterface)->DeferredEventList);

    (*PPStateInterface)->hUserEventCompleted = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (NULL == (*PPStateInterface)->hUserEventCompleted) {
        status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "Failed to create the user completion event, %!STATUS!", status);
        goto Done;
    }

Done:
    if (!NT_SUCCESS(status)) {
        if (NULL != *PPStateInterface) {
            NfcCxStateInterfaceDestroy(*PPStateInterface);
            *PPStateInterface = NULL;
        }
    }
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxStateInterfaceDestroy(
    _In_ PNFCCX_STATE_INTERFACE StateInterface
    )
/*++

Routine Description:

    This routine destroys the State Interface.

Arguments:

    StateInterface - A pointer to the State interface

Return Value:

    VOID

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NULL != StateInterface->hUserEventCompleted) {
        CloseHandle(StateInterface->hUserEventCompleted);
        StateInterface->hUserEventCompleted = NULL;
    }

    free(StateInterface);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NFCCX_CX_STATE FORCEINLINE
NfcCxStateInterfaceFindTargetState(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_STATE TargetState
    )
{
    while (NFCCX_IS_CONNECTOR_STATE(TargetState)) {
        ULONG Index = TargetState - NfcCxStateMax - 1;

        if (g_NfcCxStateConnectorMap[Index](StateInterface)) {
            TargetState = g_NfcCxState2StateMap[StateInterface->CurrentState][Index*2];
        }
        else {
            TargetState = g_NfcCxState2StateMap[StateInterface->CurrentState][Index*2 + 1];
        }
    }

    TRACE_LINE(LEVEL_INFO, "TargetState=%!NFCCX_CX_STATE!", TargetState);
    return TargetState;
}

NTSTATUS
NfcCxStateInterfaceAddDeferredEvent(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID * Param1,
    _In_opt_ VOID * Param2,
    _In_opt_ VOID * Param3
    )
/*++

Routine Description:

    This routine is used to add a deferred user event

Arguments:

    StateInterface - A pointer to the State interface
    Event - The event that is triggered
    Param - Parameters for the event

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_PENDING;
    PNFCCX_CX_EVENT_ENTRY pEventEntry = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    pEventEntry = (PNFCCX_CX_EVENT_ENTRY)malloc(sizeof(*pEventEntry));
    if (NULL == pEventEntry) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the event entry");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    InitializeListHead(&pEventEntry->ListEntry);

    pEventEntry->Event = Event;
    pEventEntry->Param1 = Param1;
    pEventEntry->Param2 = Param2;
    pEventEntry->Param3 = Param3;

    InsertTailList(&StateInterface->DeferredEventList, 
                   &pEventEntry->ListEntry);

    TRACE_LINE(LEVEL_INFO, "Event=%!NFCCX_CX_EVENT! added", Event);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

_Success_(return != FALSE)
BOOLEAN
NfcCxStateInterfaceRemoveDeferredEvent(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _Out_ NFCCX_CX_EVENT *Event,
    _Out_ VOID ** Param1,
    _Out_ VOID ** Param2,
    _Out_ VOID ** Param3
    )
/*++

Routine Description:

    This routine is used to remove a deferred user event

Arguments:

    StateInterface - A pointer to the State interface
    Event - The event that is triggered
    Param - Parameters for the event

Return Value:

    NTSTATUS

--*/
{
    PNFCCX_CX_EVENT_ENTRY pEventEntry = NULL;

    if (!IsListEmpty(&StateInterface->DeferredEventList)) {

        pEventEntry = (PNFCCX_CX_EVENT_ENTRY)
            RemoveHeadList(&StateInterface->DeferredEventList);

        *Event = pEventEntry->Event;
        *Param1 = pEventEntry->Param1;
        *Param2 = pEventEntry->Param2;
        *Param3 = pEventEntry->Param3;

        TRACE_LINE(LEVEL_INFO, "Event=%!NFCCX_CX_EVENT! removed", *Event);

        free(pEventEntry);

        return TRUE;
    }

    return FALSE;
}

NTSTATUS
NfcCxStateInterfaceStateHandler(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID * Param1,
    _In_opt_ VOID * Param2,
    _In_opt_ VOID * Param3
    )
/*++

Routine Description:

    This routine is used to handle state transitions based on
    specific events raised.

Arguments:

    StateInterface - A pointer to the State interface
    Event - The event that is triggered
    Param - Parameters for the event

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    NFCCX_CX_STATE targetState;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!StateInterface->bStateHandler) {

        StateInterface->bStateHandler = TRUE;
        TRACE_LINE(LEVEL_INFO, "CurrentState=%!NFCCX_CX_STATE! Event=%!NFCCX_CX_EVENT! TransitionFlag=%!NFCCX_CX_TRANSITION_FLAG!",
                                StateInterface->CurrentState, Event, StateInterface->TransitionFlag);

        if (NFCCX_IS_USER_EVENT(Event)) {
            TRACE_LINE(LEVEL_VERBOSE, "ResetEvent, handle %p", StateInterface->hUserEventCompleted);
            if (!ResetEvent(StateInterface->hUserEventCompleted))
            {
                NTSTATUS eventStatus = NTSTATUS_FROM_WIN32(GetLastError());
                TRACE_LINE(LEVEL_ERROR, "ResetEvent with handle %p failed, %!STATUS!", StateInterface->hUserEventCompleted, eventStatus);
            }

            if (StateInterface->TransitionFlag == NfcCxTransitionIdle) {
                StateInterface->CurrentUserEvent = Event;

                targetState = NfcCxStateInterfaceFindTargetState(StateInterface,
                                                                 g_NfcCxUsrEvent2StateMap[StateInterface->CurrentState][Event]);
                if (targetState == NfcCxStateNone) {
                    status = STATUS_INVALID_DEVICE_STATE;
                }
                else {
                    if (g_NfcCxStateHandlers[StateInterface->CurrentState][targetState] != NULL) {
                        StateInterface->TransitionFlag = NfcCxTransitionBusy;
                        status = g_NfcCxStateHandlers[StateInterface->CurrentState][targetState](StateInterface, Event, Param1, Param2, Param3);
                    }

                    StateInterface->CurrentState = targetState;
                }

                if (status != STATUS_PENDING) {
                    StateInterface->TransitionFlag = NfcCxTransitionIdle;
                    
                    if (NfcCxStateInterfaceRemoveDeferredEvent(StateInterface, &Event, &Param1, &Param2, &Param3)) {
                        NfcCxPostLibNfcThreadMessage(StateInterface->FdoContext->RFInterface, LIBNFC_STATE_HANDLER, (UINT_PTR)Event, (UINT_PTR)Param1, (UINT_PTR)Param2, (UINT_PTR)Param3);
                    }
                    else if (StateInterface->CurrentUserEvent != NfcCxEventInvalid) {
                        StateInterface->CurrentUserEvent = NfcCxEventInvalid;
                        
                        TRACE_LINE(LEVEL_VERBOSE, "SetEvent, handle %p", StateInterface->hUserEventCompleted);
                        if (!SetEvent(StateInterface->hUserEventCompleted))
                        {
                            NTSTATUS eventStatus = NTSTATUS_FROM_WIN32(GetLastError());
                            TRACE_LINE(LEVEL_ERROR, "SetEvent with handle %p failed, %!STATUS!", StateInterface->hUserEventCompleted, eventStatus);
                        }
                    }
                }
            }
            else {
                status = NfcCxStateInterfaceAddDeferredEvent(StateInterface, Event, Param1, Param2, Param3);
            }
        }
        else if (NFCCX_IS_INTERNAL_EVENT(Event)) {
            targetState = NfcCxStateInterfaceFindTargetState(StateInterface,
                                                             g_NfcCxIntEvent2StateMap[StateInterface->CurrentState][Event - NfcCxEventUserMax -1]);
            if (targetState == NfcCxStateNone) {
                status = STATUS_INVALID_DEVICE_STATE;
            }
            else if (targetState != StateInterface->CurrentState) {
                if (g_NfcCxStateHandlers[StateInterface->CurrentState][targetState] != NULL) {
                    StateInterface->TransitionFlag = NfcCxTransitionBusy;
                    status = g_NfcCxStateHandlers[StateInterface->CurrentState][targetState](StateInterface, Event, Param1, Param2, Param3);
                }

                StateInterface->CurrentState = targetState;
            }

            if (status != STATUS_PENDING) {
                StateInterface->TransitionFlag = NfcCxTransitionIdle;

                if (NfcCxStateInterfaceRemoveDeferredEvent(StateInterface, &Event, &Param1, &Param2, &Param3)) {
                    NfcCxPostLibNfcThreadMessage(StateInterface->FdoContext->RFInterface, LIBNFC_STATE_HANDLER, (UINT_PTR)Event, (UINT_PTR)Param1, (UINT_PTR)Param2, (UINT_PTR)Param3);
                }
                else if (StateInterface->CurrentUserEvent != NfcCxEventInvalid) {
                    StateInterface->CurrentUserEvent = NfcCxEventInvalid;

                    TRACE_LINE(LEVEL_VERBOSE, "SetEvent, handle %p", StateInterface->hUserEventCompleted);
                    if (!SetEvent(StateInterface->hUserEventCompleted))
                    {
                        NTSTATUS eventStatus = NTSTATUS_FROM_WIN32(GetLastError());
                        TRACE_LINE(LEVEL_ERROR, "SetEvent with handle %p failed, %!STATUS!", StateInterface->hUserEventCompleted, eventStatus);
                    }
                }
            }
        }
        else {
            NT_ASSERTMSG("Invalid Event", FALSE);
            status = STATUS_INVALID_PARAMETER;
        }

        StateInterface->bStateHandler = FALSE;
        TRACE_LINE(LEVEL_INFO, "CurrentState=%!NFCCX_CX_STATE! TransitionFlag=%!NFCCX_CX_TRANSITION_FLAG!",
                                StateInterface->CurrentState, StateInterface->TransitionFlag);
    }
    else {
        TRACE_LINE(LEVEL_INFO, "State handling in progress");
        NfcCxPostLibNfcThreadMessage(StateInterface->FdoContext->RFInterface, LIBNFC_STATE_HANDLER, (UINT_PTR)Event, (UINT_PTR)Param1, (UINT_PTR)Param2, (UINT_PTR)Param3);
        status = STATUS_PENDING;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

DWORD
NfcCxStateInterfaceWaitForUserEventToComplete(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ DWORD Timeout
    )
/*++

Routine Description:

    This routine is used to wait for the user event
    to complete

Arguments:

    StateInterface - A pointer to the State interface
    Timeout - The timeout for the user event to complete

Return Value:

    DWORD indicating the wait status

--*/
{
    TRACE_LINE(LEVEL_VERBOSE, "Wait on user event, handle %p", StateInterface->hUserEventCompleted);
    return WaitForSingleObject(StateInterface->hUserEventCompleted, Timeout);
}
