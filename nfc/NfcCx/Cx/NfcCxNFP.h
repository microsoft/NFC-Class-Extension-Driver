/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxNFP.h

Abstract:

    Nfp Interface declaration
    
Environment:

    User-mode Driver Framework

--*/

#pragma once

#define NFP_MINIMUM_MESSAGE_PAYLOAD_SIZE    (1)
#define NFP_MAXIMUM_MESSAGE_PAYLOAD_SIZE    (10240) //10KB

//
// According to the NFP documentation, NFC should
// report a transmit speed of about 50 Kbps
//
#define NFP_AVERAGE_TRANSMIT_SPEED_KBPS     (50)    //KBps

#define NFP_MAX_PUB_SUB_QUEUE_LENGTH        (50)
#define NFP_UNRESPONSIVE_CLIENT_TIMER_MS    (10000)

typedef struct _NFP_INTERFACE {

    //
    // Back link to the fdo context
    //
    PNFCCX_FDO_CONTEXT FdoContext;

    //
    // Interface Created
    //
    BOOLEAN InterfaceCreated;

    //
    // Subscriptions
    //
    WDFWAITLOCK SubsLock;
    _Guarded_by_(SubsLock)
    LIST_ENTRY SubsList;
    _Guarded_by_(SubsLock)
    LIST_ENTRY ArrivalSubsList;
    _Guarded_by_(SubsLock)
    LIST_ENTRY RemovalSubsList;

    //
    // Publications
    //
    WDFWAITLOCK PubsLock;
    _Guarded_by_(PubsLock)
    LIST_ENTRY PubsList;

    //
    // Sending List, each sends are associated with a publication client
    // This list is protected by the Publication lock (PubsLock)
    //
    _Guarded_by_(PubsLock)
    LIST_ENTRY SendList;

    //
    // The current client for which the SendWorker is in the process of sending
    // a publication. This pointer value is protected by the Publication Lock.
    //
    _Guarded_by_(PubsLock)
    PNFCCX_FILE_CONTEXT SendWorkerCurrentClient;

    WDFWORKITEM SendWorker;
    WDFWAITLOCK SendWorkerLock;

    //
    // Device Connection state and attributes
    //
    BOOLEAN DeviceConnected;
    BOOLEAN TagConnected;
    ULONG TagMaxWriteSize;
} NFP_INTERFACE, *PNFP_INTERFACE;

NTSTATUS
NfcCxNfpInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT DeviceContext,
    _Out_ PNFP_INTERFACE * PPNfpInterface
    );

VOID
NfcCxNfpInterfaceDestroy(
    _In_ PNFP_INTERFACE NfpInterface
    );

NTSTATUS
NfcCxNfpInterfaceStart(
    _In_ PNFP_INTERFACE NfpInterface
    );

NTSTATUS
NfcCxNfpInterfaceStop(
    _In_ PNFP_INTERFACE NfpInterface
    );

FN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED 
NfcCxNfpInterfaceIsIoctlSupported;


FN_NFCCX_DDI_MODULE_IODISPATCH 
NfcCxNfpInterfaceIoDispatch;

VOID
NfcCxNfpInterfaceAddSubscriptionClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID
NfcCxNfpInterfaceRemoveSubscriptionClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID
NfcCxNfpInterfaceHandleTagConnectionEstablished(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ DWORD ArrivalBitMask
    );

VOID
NfcCxNfpInterfaceHandleTagConnectionLost(
    _In_ PNFP_INTERFACE NfpInterface
    );

BOOLEAN
NfcCxNfpInterfaceCheckIfDriverDiscoveryEnabled(
    _In_ PNFP_INTERFACE NfpInterface
    );

VOID
NfcCxNfpInterfaceHandleWriteableTagEvent(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ ULONG cbMaxWriteable
    );

VOID
NfcCxNfpInterfaceVerifyAndSendPublication(
    _In_ PNFP_INTERFACE NfpInterface
    );

BOOLEAN
NfcCxNfpInterfaceCheckIfWriteTagPublicationsExist(
    _In_ PNFP_INTERFACE NfpInterface
    );

BOOLEAN
NfcCxNfpInterfaceCheckIfReadOnlyTagPublicationsExist(
    _In_ PNFP_INTERFACE NfpInterface
    );

VOID 
NfcCxNfpInterfaceHandleP2pConnectionEstablished(
    _In_ PNFP_INTERFACE NfpInterface
    );

VOID
NfcCxNfpInterfaceHandleP2pConnectionLost(
    _In_ PNFP_INTERFACE NfpInterface
    );

VOID
NfcCxNfpInterfaceHandleReceivedNdefMessage(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ CNFCProximityBuffer* MessageBuffer
    );

VOID
NfcCxNfpInterfaceAddPublicationClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

_Requires_lock_held_(NfpInterface->PubsLock)
VOID
NfcCxNfpInterfaceAddPublicationClientLocked(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID
NfcCxNfpInterfaceRemovePublicationClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

BOOLEAN
NfcCxNfpInterfaceAddSendClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

_Requires_lock_held_(NfpInterface->PubsLock)
BOOLEAN
NfcCxNfpInterfaceAddSendClientLocked(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID
NfcCxNfpInterfaceRemoveSendClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

_Requires_lock_held_(NfpInterface->PubsLock)
VOID
NfcCxNfpInterfaceRemoveSendClientLocked(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

_Requires_lock_held_(NfpInterface->PubsLock)
NTSTATUS
NfcCxNfpInterfaceVerifyStateAndSendPublicationLocked(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileConext
    );

NTSTATUS
NfcCxNfpInterfaceScheduleSendWorker(
    _In_ PNFP_INTERFACE NfpInterface
    );

EVT_WDF_WORKITEM
NfcCxNfpInterfaceSendWorker;

NTSTATUS
NfcCxNfpInterfaceValidateRequest (
    _In_ ULONG        IoControlCode,
    _In_ size_t       InputBufferLength,
    _In_ size_t       OutputBufferLength
    );

NTSTATUS
NfcCxNfpInterfaceDispatchRequest(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxNfpInterfaceDispatchEnable(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxNfpInterfaceDispatchDisable(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxNfpInterfaceDispatchGetNextSubscribedMessage(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxNfpInterfaceDispatchSetPayload(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxNfpInterfaceDispatchGetNextTransmittedMessage(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxNfpInterfaceDispatchGetMaxMessageBytes(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxNfpInterfaceDispatchGetKbps(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );
