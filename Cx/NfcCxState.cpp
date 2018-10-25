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
// State/event handling (in pseudo-code):
//
//      INPUT: CurrentState, EventType
//
//      IF NFCCX_IS_USER_EVENT(Event)
//          NewState = g_NfcCxUsrEvent2StateMap[CurrentState][EventType]
//      ELSE
//          NewState = g_NfcCxIntEvent2StateMap[CurrentState][EventType]
//      ENDIF
//
//      IF NFCCX_IS_CONNECTOR_STATE(NewState)
//          NewState = <too-complicated>
//      ENDIF
//
//      TransitionFunction = g_NfcCxStateHandlers[CurrentState][NewState]
//      TransitionFunction()
//

static void
NfcCxStateInterfaceEventProcess(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2,
    _In_opt_ void* Param3
    );

static NTSTATUS
NfcCxStateInterfaceStateHandler(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2,
    _In_opt_ void* Param3
    );

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
                            /*EventInit*/           /*EventDeinit*/             /*EventConfigDiscovery*/    /*EventStopDiscovery*/  /*EventActivate*/               /*EventDeactivateSleep*/        /*EventConfig*/         /*EventDataXchg*/       /*EventSE*/
    /*StateIdle*/           { NfcCxStateInit,       NfcCxStateNone,             NfcCxStateNone,             NfcCxStateNone,         NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,             },
    /*StateInit*/           { NfcCxStateNone,       NfcCxStateNone,             NfcCxStateNone,             NfcCxStateNone,         NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,             },
    /*StateRfIdle*/         { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateConnChkDiscMode,  NfcCxStateRfIdle,       NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateRfIdle,       NfcCxStateNone,         NfcCxStateRfIdle,           },
    /*StateRfDiscovery*/    { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateConnChkDiscMode,  NfcCxStateRfIdle,       NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone,         NfcCxStateRfDiscovery,      },
    /*StateRfDiscovered*/   { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateConnChkDiscMode,  NfcCxStateRfIdle,       NfcCxStateConnChkDeviceType,    NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone,         NfcCxStateRfDiscovered,     },
    /*StateRfDataXchg*/     { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateConnChkDiscMode,  NfcCxStateRfIdle,       NfcCxStateNone,                 NfcCxStateConnChkDeviceType,    NfcCxStateNone,         NfcCxStateRfDataXchg,   NfcCxStateRfDataXchg,       },
    /*StateRecovery*/       { NfcCxStateNone,       NfcCxStateShutdown,         NfcCxStateNone,             NfcCxStateNone,         NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,             },
    /*StateShutdown*/       { NfcCxStateNone,       NfcCxStateNone,             NfcCxStateNone,             NfcCxStateNone,         NfcCxStateNone,                 NfcCxStateNone,                 NfcCxStateNone,         NfcCxStateNone,         NfcCxStateNone,             },
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

    InitializeListHead(&(*PPStateInterface)->PendingStateEvents);

    status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &(*PPStateInterface)->PendingStateEventsLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create PendingStateEventsLock. %!STATUS!", status);
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

    if (NULL != StateInterface->PendingStateEventsLock) {
        WdfObjectDelete(StateInterface->PendingStateEventsLock);
        StateInterface->PendingStateEventsLock = NULL;
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
NfcCxStateInterfaceQueueEvent(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2,
    _In_opt_ void* Param3
    )
/*++

Routine Description:

    Adds a new state event to the list of pending events.

Arguments:

    StateInterface - A pointer to the State interface
    Event - The event type
    Param - Parameters for the event

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;

    NFCCX_STATE_PENDING_EVENT* pendingStateEvent = new NFCCX_STATE_PENDING_EVENT{ {}, Event, Param1, Param2, Param3 };
    if (!pendingStateEvent)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate NFCCX_STATE_PENDING_EVENT. %!STATUS!", status);
        goto Done;
    }

    // Insert event onto list.
    WdfWaitLockAcquire(StateInterface->PendingStateEventsLock, NULL);
    InsertTailList(&StateInterface->PendingStateEvents, &pendingStateEvent->ListNodeHeader);
    WdfWaitLockRelease(StateInterface->PendingStateEventsLock);

    // Signal the LibNfc thread to process the event.
    NfcCxPostLibNfcThreadMessage(StateInterface->FdoContext->RFInterface, LIBNFC_STATE_PROCESS_NEXT_QUEUED_EVENT, 0, 0, 0, 0);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

void
NfcCxStateInterfaceProcessNextQueuedEvent(
    _In_ PNFCCX_STATE_INTERFACE StateInterface
    )
/*++

Routine Description:

    Checks to see if there are pending state events and no currently executing state transition. If there is, starts a new
    state transition.

Arguments:

    StateInterface - A pointer to the State interface

--*/
{
    // Check if there is a state transition already in progress.
    // Note: 'TransitionState' is only accessed on the LibNfc thread.
    if (StateInterface->TransitionState != NFCCX_STATE_TRANSITION_STATE::Idle)
    {
        return;
    }

    // Check if there is pending event.
    WdfWaitLockAcquire(StateInterface->PendingStateEventsLock, NULL);
    LIST_ENTRY* pendingStateEventItr = RemoveHeadList(&StateInterface->PendingStateEvents);
    WdfWaitLockRelease(StateInterface->PendingStateEventsLock);

    if (pendingStateEventItr == &StateInterface->PendingStateEvents)
    {
        // No pending events. So nothing to do.
        return;
    }

    NFCCX_STATE_PENDING_EVENT* pendingEvent = CONTAINING_RECORD(pendingStateEventItr, NFCCX_STATE_PENDING_EVENT, ListNodeHeader);

    // Mark that a state transition is now in progress.
    StateInterface->TransitionState = NFCCX_IS_USER_EVENT(pendingEvent->Event) ?
        NFCCX_STATE_TRANSITION_STATE::UserEventRunning :
        NFCCX_STATE_TRANSITION_STATE::InternalEventRunning;

    // Process the state transition.
    NfcCxStateInterfaceEventProcess(
        StateInterface,
        pendingEvent->Event,
        pendingEvent->Param1,
        pendingEvent->Param2,
        pendingEvent->Param3
        );

    delete pendingEvent;
}

void
NfcCxStateInterfaceChainEvent(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2,
    _In_opt_ void* Param3
    )
/*++

Routine Description:

    Chains a new state transition onto the currently executing state transition.

Arguments:

    StateInterface - A pointer to the State interface
    Event - The event type
    Param - Parameters for the event

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Sanity check that a state transition is already is progress.
    if (StateInterface->TransitionState == NFCCX_STATE_TRANSITION_STATE::Idle)
    {
        TRACE_LINE(LEVEL_ERROR, "No preexisting state transition.");
        goto Done;
    }

    // Ensure that the state handler is not called recursively to prevent reentrancy issues.
    if (StateInterface->InStateHandler)
    {
        // Queue the event to be processed.
        NfcCxPostLibNfcThreadMessage(StateInterface->FdoContext->RFInterface, LIBNFC_STATE_CHAIN_EVENT, (UINT_PTR)Event, (UINT_PTR)Param1, (UINT_PTR)Param2, (UINT_PTR)Param3);
        goto Done;
    }

    // Process the new state transition.
    NfcCxStateInterfaceEventProcess(StateInterface, Event, Param1, Param2, Param3);

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static void
NfcCxStateInterfaceEventProcess(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2,
    _In_opt_ void* Param3
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Start the state transition.
    StateInterface->InStateHandler = true;
    NTSTATUS status = NfcCxStateInterfaceStateHandler(StateInterface, Event, Param1, Param2, Param3);
    StateInterface->InStateHandler = false;

    if (status == STATUS_PENDING)
    {
        // State transition has an asynchronous operation pending.
        goto Done;
    }

    // State transition has completed.
    // Check if this state transition was a user operation.
    if (StateInterface->TransitionState == NFCCX_STATE_TRANSITION_STATE::UserEventRunning)
    {
        NfcCxRFInterfaceUserEventComplete(StateInterface->FdoContext->RFInterface, status);
    }

    StateInterface->TransitionState = NFCCX_STATE_TRANSITION_STATE::Idle;

    // Check if there are any more pending events.
    WdfWaitLockAcquire(StateInterface->PendingStateEventsLock, NULL);
    bool morePendingEventsExist = !IsListEmpty(&StateInterface->PendingStateEvents);
    WdfWaitLockRelease(StateInterface->PendingStateEventsLock);

    if (morePendingEventsExist)
    {
        // Signal the LibNfc thread to process the next event.
        NfcCxPostLibNfcThreadMessage(StateInterface->FdoContext->RFInterface, LIBNFC_STATE_PROCESS_NEXT_QUEUED_EVENT, 0, 0, 0, 0);
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static NTSTATUS
NfcCxStateInterfaceStateHandler(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2,
    _In_opt_ void* Param3
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
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;

    if (NFCCX_IS_USER_EVENT(Event))
    {
        // Get the target state given the current state and the type of event.
        NFCCX_CX_STATE targetState = NfcCxStateInterfaceFindTargetState(
            StateInterface,
            g_NfcCxUsrEvent2StateMap[StateInterface->CurrentState][Event]);

        TRACE_LINE(LEVEL_INFO, "CurrentState=%!NFCCX_CX_STATE! Event=%!NFCCX_CX_EVENT! TargetState=%!NFCCX_CX_STATE!",
            StateInterface->CurrentState, Event, targetState);

        if (targetState == NfcCxStateNone)
        {
            // Not a valid state transition.
            status = STATUS_INVALID_DEVICE_STATE;
            TRACE_LINE(LEVEL_ERROR, "Invalid state transition. %!STATUS!", status);
        }
        else
        {
            // Run the user event even if the state is already correct, as some user operations don't change the state of the state machine
            // but still must be processed (e.g. eSE transmit).

            // Get the state transition handler.
            PFN_NFCCX_STATE_HANDLER stateHandler = g_NfcCxStateHandlers[StateInterface->CurrentState][targetState];

            if (stateHandler)
            {
                // Run the state transition handler.
                status = stateHandler(StateInterface, Event, Param1, Param2, Param3);
            }

            StateInterface->CurrentState = targetState;
        }
    }
    else if (NFCCX_IS_INTERNAL_EVENT(Event))
    {
        // In the `NFCCX_CX_EVENT` enum, the internal events are placed after the user events. But in the `g_NfcCxIntEvent2StateMap`
        // array, the first internal event starts at index 0. So the value of the internal events must be offset to match the
        // `g_NfcCxIntEvent2StateMap` array.
        ULONG internalEventIndex = Event - NfcCxEventUserMax - 1;

        // Get the target state given the current state and the type of event.
        NFCCX_CX_STATE targetState = NfcCxStateInterfaceFindTargetState(
            StateInterface,
            g_NfcCxIntEvent2StateMap[StateInterface->CurrentState][internalEventIndex]);

        TRACE_LINE(LEVEL_INFO, "CurrentState=%!NFCCX_CX_STATE! Event=%!NFCCX_CX_EVENT! TargetState=%!NFCCX_CX_STATE!",
            StateInterface->CurrentState, Event, targetState);

        if (targetState == NfcCxStateNone)
        {
            // Not a valid state transition.
            status = STATUS_INVALID_DEVICE_STATE;
            TRACE_LINE(LEVEL_ERROR, "Invalid state transition. %!STATUS!", status);
        }
        // Don't bother with the transition if the state is already correct.
        else if (targetState != StateInterface->CurrentState)
        {
            // Get the state transition handler.
            PFN_NFCCX_STATE_HANDLER stateHandler = g_NfcCxStateHandlers[StateInterface->CurrentState][targetState];

            if (stateHandler)
            {
                // Run the state transition handler.
                status = stateHandler(StateInterface, Event, Param1, Param2, Param3);
            }

            StateInterface->CurrentState = targetState;
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Invalid event type. %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}
