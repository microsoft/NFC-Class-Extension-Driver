/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NfcCxTml.h

Abstract:

    This module declares the Tml layer to adapt the NfcCoreLib to the
    class extension

--*/

#pragma once

typedef enum _INTERFACE_STATE {
    INTERFACE_STATE_STOPPED,
    INTERFACE_STATE_RUNNING
} INTERFACE_STATE;

//
// NfcLib transport mapping layer
//
typedef struct _NFCCX_TML_INTERFACE {
    //
    // Backlink to the FdoContext
    //
    PNFCCX_FDO_CONTEXT FdoContext;

    INTERFACE_STATE InterfaceState;

    _Guarded_by_(QueueLock)
    LIST_ENTRY      ReadQueue;

    _Guarded_by_(QueueLock)
    LIST_ENTRY      ReadNotificationQueue;
    WDFWAITLOCK     QueueLock;

    _Guarded_by_(QueueLock)
    LIST_ENTRY      WriteQueue;

    //
    // Flag to indicate if write pending with the lower transport layer.
    // If so, then do not pass any reads to the NfcCoreLib since it expects
    // this synchronization in the transport layer.
    //
    _Guarded_by_(QueueLock)
    BOOLEAN IsWriteCompletionPending;

} NFCCX_TML_INTERFACE, *PNFCCX_TML_INTERFACE;

typedef struct _NFCCX_TML_READ_QUEUE_ENTRY {
    LIST_ENTRY ListEntry;
    _Field_size_bytes_(BufferLength) PUCHAR Buffer;
    USHORT BufferLength;
    pphTmlNfc_TransactCompletionCb_t PfnReadCompletion;
    PVOID Context;
    HANDLE hCompletionEvent;
} NFCCX_TML_READ_QUEUE_ENTRY, *PNFCCX_TML_READ_QUEUE_ENTRY;

typedef struct _NFCCX_TML_WRITE_QUEUE_ENTRY {
    LIST_ENTRY ListEntry;
    WDFREQUEST Request;
    PNFCCX_TML_INTERFACE TmlInterface;
} NFCCX_TML_WRITE_QUEUE_ENTRY, *PNFCCX_TML_WRITE_QUEUE_ENTRY;

typedef struct _NFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY {
    LIST_ENTRY ListEntry;
    UCHAR Buffer[NCI_PACKET_MAX_SIZE];
    USHORT BufferLength;
} NFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY, *PNFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY;

typedef struct _NFCCX_TML_WRITE_CONTEXT {
    WDFDEVICE Device;
    pphTmlNfc_TransactCompletionCb_t PfnWriteCompletion;
    PVOID Context;
    WDFMEMORY Memory;
    USHORT Length;
    NTSTATUS Status;
    HANDLE hCompletionEvent;
    ULONG ReferenceCount;
} NFCCX_TML_WRITE_CONTEXT, *PNFCCX_TML_WRITE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NFCCX_TML_WRITE_CONTEXT, NfcCxTmlWriteGetContext);

NTSTATUS
NfcCxTmlInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Out_ PNFCCX_TML_INTERFACE * PPRFInterface
    );

VOID
NfcCxTmlInterfaceDestroy(
    _In_ PNFCCX_TML_INTERFACE TmlInterface
    );

VOID
NfcCxTml_DrainQueue(
    _In_ PNFCCX_TML_INTERFACE TmlInterface
    );

NTSTATUS
NfcCxTmlInterfaceStart(
    _In_ PNFCCX_TML_INTERFACE TmlInterface
    );

NTSTATUS
NfcCxTmlInterfaceStop(
    _In_ PNFCCX_TML_INTERFACE TmlInterface
    );

NTSTATUS
NfcCxTmlDispatchReadNotification(
    _In_ PNFCCX_TML_INTERFACE TmlInterface,
    _In_opt_bytecount_(BufferSize) PUCHAR Buffer,
    _In_ USHORT BufferSize
    );

VOID
NfcCxTml_Write(
    _In_ PNFCCX_TML_WRITE_QUEUE_ENTRY WriteQueueEntry
    );

EVT_WDF_REQUEST_COMPLETION_ROUTINE NfcCxTml_WriteCompletion;
