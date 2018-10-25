/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NfcCxState.h

Abstract:

    This module declares state management types in NFC CX

--*/

#pragma once

typedef enum _NFCCX_CX_STATE : ULONG {
    NfcCxStateIdle,
    NfcCxStateInit,
    NfcCxStateRfIdle,
    NfcCxStateRfDiscovery,
    NfcCxStateRfDiscovered,
    NfcCxStateRfDataXchg,
    NfcCxStateRecovery,
    NfcCxStateShutdown,
    NfcCxStateMax,
    NfcCxStateConnChkDiscMode,
    NfcCxStateConnChkDeviceType,
    NfcCxStateConnMax,
    NfcCxStateNone,
} NFCCX_CX_STATE, *PNFCCX_CX_STATE;

#define NFCCX_IS_CONNECTOR_STATE(State) ((State) > NfcCxStateMax && (State) < NfcCxStateNone)

typedef enum _NFCCX_CX_EVENT : ULONG {
    NfcCxEventInit,
    NfcCxEventDeinit,
    NfcCxEventConfigDiscovery,
    NfcCxEventStopDiscovery,
    NfcCxEventActivate,
    NfcCxEventDeactivateSleep,
    NfcCxEventConfig,
    NfcCxEventDataXchg,
    NfcCxEventSE,
    NfcCxEventUserMax,
    NfcCxEventReqCompleted,
    NfcCxEventTimeout,
    NfcCxEventRecovery,
    NfcCxEventDiscovered,
    NfcCxEventActivated,
    NfcCxEventDeactivated,
    NfcCxEventFailed,
    NfcCxEventInternalMax,
    NfcCxEventInvalid,
} NFCCX_CX_EVENT, *PNFCCX_CX_EVENT;

#define NFCCX_IS_USER_EVENT(Event) ((Event) < NfcCxEventUserMax)
#define NFCCX_IS_INTERNAL_EVENT(Event) ((Event) > NfcCxEventUserMax && (Event) < NfcCxEventInternalMax)

enum class NFCCX_STATE_TRANSITION_STATE
{
    // There is no active state transition (or user operation) pending.
    Idle,
    // A user operation is running. (See, NFCCX_RF_OPERATION.)
    // These are always started by a call to NfcCxRFInterfaceExecute.
    UserEventRunning,
    // An internal operation is running.
    // These are usually started in response to a hardware event. (e.g. tag arrived.)
    InternalEventRunning,
};

typedef
BOOLEAN
(FN_NFCCX_STATE_CONNECTOR)(
    _In_ PNFCCX_STATE_INTERFACE StateInterface
    );

typedef
NTSTATUS
(FN_NFCCX_STATE_HANDLER)(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2,
    _In_opt_ void* Param3
    );

typedef FN_NFCCX_STATE_CONNECTOR *PFN_NFCCX_STATE_CONNECTOR;
typedef FN_NFCCX_STATE_HANDLER *PFN_NFCCX_STATE_HANDLER;

struct NFCCX_STATE_PENDING_EVENT
{
    LIST_ENTRY ListNodeHeader;
    NFCCX_CX_EVENT Event;
    void* Param1;
    void* Param2;
    void* Param3;
};

typedef struct _NFCCX_STATE_INTERFACE {
    PNFCCX_FDO_CONTEXT FdoContext;
    NFCCX_CX_STATE CurrentState;
    NFCCX_STATE_TRANSITION_STATE TransitionState;
    // Stores deferred events, which are waiting for the current operation to complete.
    LIST_ENTRY PendingStateEvents;
    WDFWAITLOCK PendingStateEventsLock;
    // Used to detect when the state handler is called recursively.
    bool InStateHandler;
} NFCCX_STATE_INTERFACE, *PNFCCX_STATE_INTERFACE;

NTSTATUS
NfcCxStateInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Out_ PNFCCX_STATE_INTERFACE * PPStateInterface
    );

VOID
NfcCxStateInterfaceDestroy(
    _In_ PNFCCX_STATE_INTERFACE StateInterface
    );

NTSTATUS
NfcCxStateInterfaceQueueEvent(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2,
    _In_opt_ void* Param3
    );

void
NfcCxStateInterfaceProcessNextQueuedEvent(
    _In_ PNFCCX_STATE_INTERFACE StateInterface
    );

void
NfcCxStateInterfaceChainEvent(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2,
    _In_opt_ void* Param3
    );
