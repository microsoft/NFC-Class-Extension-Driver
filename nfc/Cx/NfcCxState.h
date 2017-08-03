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

typedef enum _NFCCX_CX_TRANSITION_FLAG {
    NfcCxTransitionIdle,
    NfcCxTransitionBusy,
} NFCCX_CX_TRANSITION_FLAG, *PNFCCX_CX_TRANSITION_FLAG;

#define NFCCX_IS_STATE_TRANSITION_IDLE(Flag) ((Flag) == NfcCxTransitionIdle)
#define NFCCX_IS_STATE_TRANSITION_BUSY(Flag) ((Flag) == NfcCxTransitionBusy)

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
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    );

typedef FN_NFCCX_STATE_CONNECTOR *PFN_NFCCX_STATE_CONNECTOR;
typedef FN_NFCCX_STATE_HANDLER *PFN_NFCCX_STATE_HANDLER;

typedef struct _NFCCX_CX_EVENT_ENTRY {
    LIST_ENTRY                  ListEntry;
    NFCCX_CX_EVENT              Event;
    VOID*                       Param1;
    VOID*                       Param2;
    VOID*                       Param3;
} NFCCX_CX_EVENT_ENTRY, *PNFCCX_CX_EVENT_ENTRY;

typedef struct _NFCCX_STATE_INTERFACE {
    PNFCCX_FDO_CONTEXT          FdoContext;
    NFCCX_CX_STATE              CurrentState;
    NFCCX_CX_TRANSITION_FLAG    TransitionFlag;
    LIST_ENTRY                  DeferredEventList;
    NFCCX_CX_EVENT              CurrentUserEvent;
    HANDLE                      hUserEventCompleted;
    BOOLEAN                     bStateHandler;
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
NfcCxStateInterfaceStateHandler(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID * Param1,
    _In_opt_ VOID * Param2,
    _In_opt_ VOID * Param3
    );

NTSTATUS
NfcCxStateInterfaceAddDeferredEvent(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID * Param1,
    _In_opt_ VOID * Param2,
    _In_opt_ VOID * Param3
    );

NTSTATUS
NfcCxStateInterfaceClearAllDeferredEvents(
    _In_ PNFCCX_STATE_INTERFACE StateInterface
    );

DWORD
NfcCxStateInterfaceWaitForUserEventToComplete(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ DWORD Timeout
    );
