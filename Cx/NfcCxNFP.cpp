/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxNFP.cpp

Abstract:

    Nfp Interface Implementation
    
Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxNFP.tmh"

typedef
NTSTATUS
(*PFNNFP_DISPATCH_HANDLER) (
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

typedef struct _NFP_DISPATCH_ENTRY {
    ULONG IoControlCode;
    size_t MinimumInputBufferLength;
    size_t MinimumOutputBufferLength;
    PFNNFP_DISPATCH_HANDLER DispatchHandler;
} NFP_DISPATCH_ENTRY, *PNFP_DISPATCH_ENTRY;

NFP_DISPATCH_ENTRY 
g_NfpDispatch [] = {
    {IOCTL_NFP_ENABLE,                      0,          0,                      NfcCxNfpInterfaceDispatchEnable},
    {IOCTL_NFP_DISABLE,                     0,          0,                      NfcCxNfpInterfaceDispatchDisable},
    {IOCTL_NFP_GET_NEXT_SUBSCRIBED_MESSAGE, 0,          sizeof(ULONG),          NfcCxNfpInterfaceDispatchGetNextSubscribedMessage},
    {IOCTL_NFP_SET_PAYLOAD,                 0,          0,                      NfcCxNfpInterfaceDispatchSetPayload},
    {IOCTL_NFP_GET_NEXT_TRANSMITTED_MESSAGE,0,          0,                      NfcCxNfpInterfaceDispatchGetNextTransmittedMessage},
    {IOCTL_NFP_GET_MAX_MESSAGE_BYTES,       0,          sizeof(ULONG),          NfcCxNfpInterfaceDispatchGetMaxMessageBytes},
    {IOCTL_NFP_GET_KILO_BYTES_PER_SECOND,   0,          sizeof(ULONG),          NfcCxNfpInterfaceDispatchGetKbps}
};


NTSTATUS
NfcCxNfpInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Out_ PNFP_INTERFACE * PPNfpInterface
    )
/*++

Routine Description:

    This routine creates and initalizes the Nfp Interface.

Arguments:

    FdoContext - A pointer to the FdoContext
    NfpInterface - A pointer to a memory location to receive the allocated Nfp interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFP_INTERFACE nfpInterface = NULL;
    WDF_OBJECT_ATTRIBUTES objectAttrib;
    WDF_WORKITEM_CONFIG workItemConfig = {};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    nfpInterface = (PNFP_INTERFACE)malloc(sizeof(*nfpInterface));
    if (NULL == nfpInterface) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the nfc interface");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    ZeroMemory(nfpInterface, sizeof(*nfpInterface));

    nfpInterface->FdoContext = FdoContext;

    //
    // Subscriptions
    //
    InitializeListHead(&nfpInterface->SubsList);
    InitializeListHead(&nfpInterface->ArrivalSubsList);
    InitializeListHead(&nfpInterface->RemovalSubsList);

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = FdoContext->Device;
    status = WdfWaitLockCreate(&objectAttrib,
                                &nfpInterface->SubsLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the Subscription WaitLock, %!STATUS!", status);
        goto Done;
    }

    //
    // Publications
    //
    InitializeListHead(&nfpInterface->PubsList);
    InitializeListHead(&nfpInterface->SendList);

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = FdoContext->Device;
    status = WdfWaitLockCreate(&objectAttrib,
                                &nfpInterface->PubsLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the Publication WaitLock, %!STATUS!", status);
        goto Done;
    }

    //
    // Send Worker
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = FdoContext->Device;

    WDF_WORKITEM_CONFIG_INIT(&workItemConfig,
                             NfcCxNfpInterfaceSendWorker);
    workItemConfig.AutomaticSerialization = TRUE;

    status = WdfWorkItemCreate(&workItemConfig,
                               &objectAttrib,
                               &nfpInterface->SendWorker);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the SendWorker, %!STATUS!", status);
        goto Done;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = FdoContext->Device;
    status = WdfWaitLockCreate(&objectAttrib,
                               &nfpInterface->SendWorkerLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the SendWorker WaitLock, %!STATUS!", status);
        goto Done;
    }

Done:

    if (!NT_SUCCESS(status)) {
        if (NULL != nfpInterface) {
            NfcCxNfpInterfaceDestroy(nfpInterface);
            nfpInterface = NULL;
        }
    }

    *PPNfpInterface = nfpInterface;

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxNfpInterfaceDestroy(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    This routine cleans up the Nfp Interface

Arguments:

    NfpInterface - A pointer to the NfpInterface to cleanup.

Return Value:

    None

--*/
{
    //
    // Perform any cleanup associated with the NfpInterface
    //

    //
    // Since the lock objects are parented to the device,
    // there are no needs to manually delete them here
    //
    
    if (NfpInterface->InterfaceCreated) {
        //
        // Disable the NFP interface
        //
        WdfDeviceSetDeviceInterfaceState(NfpInterface->FdoContext->Device,
                                         &GUID_DEVINTERFACE_NFP,
                                         NULL,
                                         FALSE);

        TRACE_LINE(LEVEL_VERBOSE, "NFP interface disabled");
    }

    free(NfpInterface);
}

NTSTATUS
NfcCxNfpInterfaceStart(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    Start the Nfp Interface

Arguments:

    NfpInterface - The Nfp Interface

Return Value:

    NTSTATUS

--*/
{
    static const wchar_t nfpCapabilitiesStringList[] = L"StandardNfc\0"; // Extra null terminator for multi-string list (DEVPROP_TYPE_STRING_LIST)

    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext = NfpInterface->FdoContext;
    WDF_DEVICE_INTERFACE_PROPERTY_DATA propertyData = {};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!fdoContext->NfpRadioInterfaceCreated &&
        !fdoContext->DisableRfInterfaces) {
        //
        // Create and publish the NFP RM Interface
        //
        status = WdfDeviceCreateDeviceInterface(fdoContext->Device,
                                                &GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE,
                                                NULL);
        if (!NT_SUCCESS (status)) {
            TRACE_LINE(LEVEL_ERROR, "WdfDeviceCreateDeviceInterface failed %!STATUS!", status);
            goto Done;
        }

        WdfDeviceSetDeviceInterfaceState(fdoContext->Device,
                                         &GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE,
                                         NULL,
                                         TRUE);

        fdoContext->NfpRadioInterfaceCreated = TRUE;
    }

    if (fdoContext->Power->NfpRadioState &&
        !fdoContext->DisableRfInterfaces)
    {
        //
        // Publish the NFP interface
        //
        if (!NfpInterface->InterfaceCreated) {
            status = WdfDeviceCreateDeviceInterface(fdoContext->Device,
                                                    &GUID_DEVINTERFACE_NFP,
                                                    NULL);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to create the NFP device interface, %!STATUS!", status);
                goto Done;
            }

            WDF_DEVICE_INTERFACE_PROPERTY_DATA_INIT(&propertyData,
                                                    &GUID_DEVINTERFACE_NFP,
                                                    &DEVPKEY_NFP_Capabilities);

            status = WdfDeviceAssignInterfaceProperty(fdoContext->Device,
                                                      &propertyData,
                                                      DEVPROP_TYPE_STRING_LIST,
                                                      sizeof(nfpCapabilitiesStringList),
                                                      const_cast<wchar_t*>(nfpCapabilitiesStringList));
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to assign property for the NFP device interface, %!STATUS!", status);
                goto Done;
            }

            NfpInterface->InterfaceCreated = TRUE;
        }

        WdfDeviceSetDeviceInterfaceState(fdoContext->Device,
                                         &GUID_DEVINTERFACE_NFP,
                                         NULL,
                                         TRUE);
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
NfcCxNfpInterfaceStop(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    Stop the Nfp Interface

Arguments:

    NfpInterface - The Nfp Interface

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NfpInterface->InterfaceCreated) {

        WdfDeviceSetDeviceInterfaceState(NfpInterface->FdoContext->Device,
                                         &GUID_DEVINTERFACE_NFP,
                                         NULL,
                                         FALSE);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

BOOLEAN
NfcCxNfpInterfaceIsIoctlSupported(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This routine returns true if the provided IOCTL is supported by the
    module.

    For NFP, this routine will return true for all NFP IOCTLs

Arguments:

    FdoContext - The Fdo context
    IoControlCode - The IOCTL code to check

Return Value:

    TRUE - The IOCTL is supported by this module
    FALSE - The IOCTL is not supported by this module

--*/
{
    ULONG i;

    UNREFERENCED_PARAMETER(FdoContext);

    for (i=0; i < ARRAYSIZE(g_NfpDispatch);i++) {
        if (g_NfpDispatch[i].IoControlCode == IoControlCode) {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
NfcCxNfpInterfaceAddSubscriptionClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine adds the subscription client (represented by the FileContext)
    to the proper subscriptionlist

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - Client to add

Return Value:

    NONE

--*/
{
    NT_ASSERTMSG("Client not for subscriptions",
                ROLE_SUBSCRIPTION == FileContext->Role);

    WdfWaitLockAcquire(NfpInterface->SubsLock, NULL);

    if (NfcCxFileObjectIsNormalSubscription(FileContext)) {
        InsertHeadList(&NfpInterface->SubsList, 
                       &FileContext->ListEntry);
    } else if (NfcCxFileObjectIsArrivalSubscription(FileContext)) {
        InsertHeadList(&NfpInterface->ArrivalSubsList, 
                       &FileContext->ListEntry);
    } else if (NfcCxFileObjectIsRemovalSubscription(FileContext)) {
        InsertHeadList(&NfpInterface->RemovalSubsList, 
                        &FileContext->ListEntry);
    } else if (NfcCxFileObjectIsWritableTagSubscription(FileContext)) {
        InsertHeadList(&NfpInterface->SubsList, 
                       &FileContext->ListEntry);
    } else {
        NT_ASSERTMSG("Invalid client subscription type", FALSE);
    }

    WdfWaitLockRelease(NfpInterface->SubsLock);
}


VOID
NfcCxNfpInterfaceRemoveSubscriptionClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine removes the subscription client (represented by the FileContext)
    from the proper subscriptionlist

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - Client to remove

Return Value:

    NONE

--*/
{
    NT_ASSERTMSG("Client not for subscriptions",
                ROLE_SUBSCRIPTION == FileContext->Role);

    WdfWaitLockAcquire(NfpInterface->SubsLock, NULL);

    RemoveEntryList(&FileContext->ListEntry);

    WdfWaitLockRelease(NfpInterface->SubsLock);
}

NTSTATUS
NfcCxNfpInterfaceCopySubcriptionData(
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ ULONG DataLength,
    _Out_ PULONG BufferUsed
    )
/*++

Routine Description:

   Copies the message data into the subscribed output buffer.

Arguments:

    OutputBuffer - The Output buffer
    OutputBufferLength - The output buffer length
    Data - The message data buffer
    DataLength - The message data buffer length
    BufferUsed - A pointer to a ULONG to receive how many bytes of the output buffer was used.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG requiredBufferSize = 0;

    *BufferUsed = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Since the output buffer has already been validated
    // to contain 4 bytes, we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(ULONG) <= OutputBufferLength);

    //
    // The first 4 bytes will provide the payload size to the client
    // in the event the buffer is not large enough to contain the full
    // payload, the client can allocate a buffer large enough using
    // this size on future requests
    //
    CopyMemory(OutputBuffer, &DataLength, sizeof(ULONG));
    *BufferUsed = sizeof(ULONG);

    status = RtlULongAdd(DataLength, sizeof(ULONG), &requiredBufferSize);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to calculate the required buffer size, %!STATUS!", status);
        goto Done;
    }

    if (OutputBufferLength < requiredBufferSize) {
        status = STATUS_BUFFER_OVERFLOW;
        goto Done;
    }

    CopyMemory(((PUCHAR)OutputBuffer) + sizeof(ULONG), Data, DataLength);
    *BufferUsed = requiredBufferSize;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

BOOLEAN
NfpCxNfpInterfaceCompleteSubscriptionRequestLocked(
    _In_ WDFQUEUE SubQueue,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ USHORT DataLength
    )
/*++

Routine Description:

    This routine attempts to complete a request from the provided queue
    with the provided data.

Arguments:

    SubQueue - The Subscription queue to attempt to complete a request from
    Data - Data to complete the request with
    DataLength - The Data Length

Return Value:

    TRUE - A request was successfully completed
    FALSE - The event was not distributed

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFREQUEST wdfRequest = NULL;
    WDFMEMORY reqOutMemory = NULL;
    PVOID pOutBuffer = NULL;
    size_t sizeOutBuffer = 0;
    ULONG actualSize;
    BOOLEAN eventProcessed = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Get the next pended IOCTL_NFP_GET_NEXT_SUBSCRIBED_MESSAGE request to see
    // if any of them matches the event
    //
    status = WdfIoQueueRetrieveNextRequest(SubQueue, &wdfRequest);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_WARNING,
            "No requests pended, %!STATUS!", status);
        goto Done;
    }

    //
    //  Get the request's output buffer
    //
    status = WdfRequestRetrieveOutputMemory(wdfRequest, &reqOutMemory); 
    if (NT_SUCCESS(status)) { 
        pOutBuffer = WdfMemoryGetBuffer(reqOutMemory, &sizeOutBuffer);   
    } else {
        TRACE_LINE(LEVEL_ERROR,
            "Failed to get output buffer, %!STATUS!", status);
        goto Done;
    }
        
    //
    // Copy the event onto the output memory
    //
    status = NfcCxNfpInterfaceCopySubcriptionData(pOutBuffer,
                                                    (ULONG)sizeOutBuffer,
                                                    Data,
                                                    DataLength,
                                                    &actualSize);

    TRACE_LINE(LEVEL_INFO,
        "Completing request %p, with %!STATUS!, 0x%I64x", wdfRequest, status, actualSize);

#ifdef EVENT_WRITE
    EventWriteNfpGetNextSubscribedMsgStop(WdfRequestGetFileObject(wdfRequest),
                                          status,
                                          actualSize);
#endif

    WdfRequestCompleteWithInformation(wdfRequest, status, actualSize);
    wdfRequest = NULL;

    if (NT_SUCCESS(status)) {
        //
        // It is possible that the event was completed with STATUS_BUFFER_OVERFLOW
        // to indicate to the client that a bigger buffer is required, in that
        // context, we still enqueue the event for the next request from the client.
        // 
        eventProcessed = TRUE;
    }

Done:

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);

    return eventProcessed;
}

typedef enum _NFP_ARRIVAL_DEPARTURE_EVENT {
    NfpTagArrival,
    NfpTagWritableArrival,
    NfpTagDeparture,
    NfpP2PArrival,
    NfpP2PDeparture
} NFP_ARRIVAL_DEPARTURE_EVENT;

NTSTATUS
NfcCxNfpInterfaceDistributeArrivalDepartureEvent(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ NFP_ARRIVAL_DEPARTURE_EVENT Event,
    _In_ DWORD EventData
    )
/*++

Routine Description:

    This routine distribute the event to subscribed client

Arguments:

    NfpInterface - A pointer to the NfpInterface
    Event - Event to distribute
    EventData - The Data associated with the event

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PLIST_ENTRY head = NULL;
    PLIST_ENTRY ple = NULL;
    TRANSLATION_TYPE_PROTOCOL expectedTranslationType;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

#ifdef EVENT_WRITE
    EventWriteRfArrivalDeparture(Event);
#endif

    WdfWaitLockAcquire(NfpInterface->SubsLock, NULL);

    if (NfpTagArrival == Event ||
        NfpP2PArrival == Event) {
        head = &NfpInterface->ArrivalSubsList;
        expectedTranslationType = TRANSLATION_TYPE_ARRIVAL;
        NfpInterface->DeviceConnected = TRUE;
        NfpInterface->TagConnected = (NfpTagArrival == Event);
    } else if (NfpTagDeparture == Event ||
               NfpP2PDeparture == Event) {
        head = &NfpInterface->RemovalSubsList;
        expectedTranslationType = TRANSLATION_TYPE_REMOVAL;
        NfpInterface->DeviceConnected = FALSE;
        NfpInterface->TagConnected = FALSE;
    } else if (NfpTagWritableArrival == Event) {
        head = &NfpInterface->SubsList;
        expectedTranslationType = TRANSLATION_TYPE_WRITABLETAG_SIZE;
        NfpInterface->TagMaxWriteSize = EventData;
    } else {
        NT_ASSERTMSG("Invalid arrival/departure event", FALSE);
        goto Done;
    }

    if (IsListEmpty(head)) {
        TRACE_LINE(LEVEL_WARNING, "No subscription for event");
        goto Done;
    }

    for (ple = head->Flink; 
         ple != head; 
         ple = ple->Flink) {
        PNFCCX_FILE_CONTEXT client;

        client = CONTAINING_RECORD(ple, NFCCX_FILE_CONTEXT, ListEntry);

        WdfWaitLockAcquire(client->StateLock, NULL);

        NT_ASSERT(ROLE_SUBSCRIPTION == client->Role);

        if (expectedTranslationType != client->TranslationType) {
            WdfWaitLockRelease(client->StateLock);
            continue;
        }

        if (!client->Enabled) {
            TRACE_LINE(LEVEL_WARNING, "Client %p is not enabled", client);
            WdfWaitLockRelease(client->StateLock);
            continue;
        }

        //
        // If there is a pending request, complete it,
        // else enqueue the message
        //
        if (!NfpCxNfpInterfaceCompleteSubscriptionRequestLocked(
              client->RoleParameters.Sub.SubsMessageRequestQueue,
              &EventData,
              sizeof(EventData))) {
            CNFCPayload* pEntry = NULL;

            TRACE_LINE(LEVEL_WARNING, "No request to complete, enqueuing event");

            if (NFP_MAX_PUB_SUB_QUEUE_LENGTH <= client->RoleParameters.Sub.SubscribedMessageQueueLength) {
                TRACE_LINE(LEVEL_ERROR, "Too many queued messages, dropping event");
                status = STATUS_ABANDONED;
                WdfWaitLockRelease(client->StateLock);
                continue;
            } else if (client->IsUnresponsiveClientDetected) {
                TRACE_LINE(LEVEL_ERROR, "Ignoring event, client is unresponsive");
                status = STATUS_ABANDONED;
                WdfWaitLockRelease(client->StateLock);
                continue;
            }

            pEntry = new CNFCPayload();
            if (NULL == pEntry) {
                TRACE_LINE(LEVEL_ERROR, "Failed to allocate the sub queue entry");
                status = STATUS_INSUFFICIENT_RESOURCES;
                WdfWaitLockRelease(client->StateLock);
                continue;
            }

            // This function can't fail for <= sizeof(DWORD)
            (VOID)pEntry->Initialize((PBYTE)&EventData, sizeof(DWORD));

            InsertTailList(&client->RoleParameters.Sub.SubscribedMessageQueue,
                            pEntry->GetListEntry());
            client->RoleParameters.Sub.SubscribedMessageQueueLength++;

            NfcCxFileObjectStartUnresponsiveClientDetectionTimer(client);
        }

        WdfWaitLockRelease(client->StateLock);
    }

Done:

    WdfWaitLockRelease(NfpInterface->SubsLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
NfcCxNfpInterfaceHandleTagConnectionEstablished(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ DWORD ArrivalBitMask
    )
/*++

Routine Description:

    This routine distribute The TagConnectionEstablished event

Arguments:

    NfpInterface - A pointer to the NfpInterface
    ArrivalBitMask - bit mask for the arrival subscription
    
Return Value:

    VOID

--*/
{
    TRACE_LINE(LEVEL_INFO, "TagConnectionEstablished!!!!!!!");

    NfcCxNfpInterfaceDistributeArrivalDepartureEvent(NfpInterface,
                                                     NfpTagArrival,
                                                     ArrivalBitMask);
}

VOID
NfcCxNfpInterfaceHandleTagConnectionLost(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    This routine distribute The TagConnectionLost event

Arguments:

    NfpInterface - A pointer to the NfpInterface

Return Value:

    VOID

--*/
{
    TRACE_LINE(LEVEL_INFO, "TagConnectionLost!!!!!!!");

    NfcCxNfpInterfaceDistributeArrivalDepartureEvent(NfpInterface,
                                                     NfpTagDeparture,
                                                     0);
}

VOID
NfcCxNfpInterfaceHandleWriteableTagEvent(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ ULONG cbMaxWriteable
    )
/*++

Routine Description:

    This routine distribute The TagWritableArrival event

Arguments:

    NfpInterface - A pointer to the NfpInterface
    cbMaxWriteable - Maximum writeable size of the tag

Return Value:

    VOID

--*/
{
    TRACE_LINE(LEVEL_INFO, "Tag Writable Arrival, MaxWritable %d !!!!!!!", cbMaxWriteable);

    NfcCxNfpInterfaceDistributeArrivalDepartureEvent(NfpInterface,
                                                     NfpTagWritableArrival,
                                                     cbMaxWriteable);
}

VOID
NfcCxNfpInterfaceVerifyAndSendPublication(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    This routine sends the pending publications.

Arguments:

    NfpInterface - A pointer to the NfpInterface

Return Value:

    VOID

--*/
{
    PLIST_ENTRY ple;

    //
    // See if we need to perform writes
    //
    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);
    for (ple = NfpInterface->PubsList.Flink; 
         ple != &NfpInterface->PubsList; 
         ple = ple->Flink) {
        PNFCCX_FILE_CONTEXT pubClient = CONTAINING_RECORD(ple, NFCCX_FILE_CONTEXT, ListEntry);
        NfcCxNfpInterfaceVerifyStateAndSendPublicationLocked(NfpInterface, pubClient);
    }
    WdfWaitLockRelease(NfpInterface->PubsLock);
}

BOOLEAN
NfcCxNfpInterfaceCheckIfWriteTagPublicationsExist(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    This routine loops through the available clients and
    determines if a tag publication exists.

Arguments:

    NfpInterface - A pointer to the NfpInterface

Return Value:

    TRUE - A Tag write publication exists
    FALSE - A Tag write publication does NOT exist.

--*/
{
    BOOLEAN retValue = FALSE;
    PLIST_ENTRY ple;

    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);

    for (ple = NfpInterface->PubsList.Flink; 
         ple != &NfpInterface->PubsList && FALSE == retValue; 
         ple = ple->Flink) {
        PNFCCX_FILE_CONTEXT fileContext = CONTAINING_RECORD(ple, NFCCX_FILE_CONTEXT, ListEntry);

        if (fileContext->Enabled) {
            retValue = NfcCxFileObjectIsTagWriteClient(fileContext);
        }
    }

    WdfWaitLockRelease(NfpInterface->PubsLock);
    return retValue;
}

BOOLEAN
NfcCxNfpInterfaceCheckIfReadOnlyTagPublicationsExist(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    This routine loops through the available clients and
    determines if a readonly publication exists.

Arguments:

    NfpInterface - A pointer to the NfpInterface

Return Value:

    TRUE - A Tag readonly publication exists
    FALSE - A Tag readonly publication does NOT exist.

--*/
{
    BOOLEAN retValue = FALSE;
    PLIST_ENTRY ple;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);

    for (ple = NfpInterface->PubsList.Flink; 
         ple != &NfpInterface->PubsList && FALSE == retValue; 
         ple = ple->Flink) {
        PNFCCX_FILE_CONTEXT fileContext = CONTAINING_RECORD(ple, NFCCX_FILE_CONTEXT, ListEntry);

        retValue = NfcCxFileObjectIsTagReadOnlyClient(fileContext);
    }

    WdfWaitLockRelease(NfpInterface->PubsLock);

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, retValue);
    return retValue;
}


VOID 
NfcCxNfpInterfaceHandleP2pConnectionEstablished(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    This routine distribute The P2PConnectionEstablished event

Arguments:

    NfpInterface - A pointer to the NfpInterface

Return Value:

    VOID

--*/
{
    PLIST_ENTRY ple;
    TRACE_LINE(LEVEL_INFO, "P2PConnectionEstablish!!!!!!!");

    NfcCxNfpInterfaceDistributeArrivalDepartureEvent(NfpInterface,
                                                     NfpP2PArrival,
                                                     DEVICE_ARRIVAL_BIT_BIDIRECTION);

    //
    // See if we need to perform writes
    //
    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);
    for (ple = NfpInterface->PubsList.Flink; 
         ple != &NfpInterface->PubsList; 
         ple = ple->Flink) {
        PNFCCX_FILE_CONTEXT pubClient = CONTAINING_RECORD(ple, NFCCX_FILE_CONTEXT, ListEntry);
        NfcCxNfpInterfaceVerifyStateAndSendPublicationLocked(NfpInterface, pubClient);
    }
    WdfWaitLockRelease(NfpInterface->PubsLock);
}

VOID
NfcCxNfpInterfaceHandleP2pConnectionLost(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    This routine distribute The P2PConnectionLost event

Arguments:

    NfpInterface - A pointer to the NfpInterface

Return Value:

    VOID

--*/
{
    TRACE_LINE(LEVEL_INFO, "P2PConnectionLost!!!!!!!");

    //
    // Since the connection has been lost, remove any writes that may be pended.
    //
    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);
    while (!IsListEmpty(&NfpInterface->SendList)) {
        PNFCCX_FILE_CONTEXT fileContext = CONTAINING_RECORD(RemoveHeadList(&NfpInterface->SendList), 
                                                                           NFCCX_FILE_CONTEXT, 
                                                                           SendListEntry);
        InitializeListHead(&fileContext->SendListEntry);
        TRACE_LINE(LEVEL_VERBOSE, "Removing fileContext (0x%p) from SendList", fileContext);
        
        WdfObjectDereferenceWithTag(fileContext->FileObject, SEND_LIST_ENTRY_TAG);
    }

    WdfWaitLockRelease(NfpInterface->PubsLock);

    NfcCxNfpInterfaceDistributeArrivalDepartureEvent(NfpInterface,
                                                     NfpP2PDeparture,
                                                     0);
}

VOID
NfcCxNfpInterfaceHandleReceivedNdefMessage(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ CNFCProximityBuffer* MessageBuffer
    )
/*++

Routine Description:

    This routine distribute the received NDEF message from
    the device to the subscribed client

Arguments:

    NfpInterface - A pointer to the NfpInterface
    MessageBuffer - A pointer to the MessageBuffer to distribute

Return Value:

    VOID

--*/
{
    PLIST_ENTRY ple;

    TRACE_LINE(LEVEL_INFO, "NDEF message received. Size = %u, Tnf = %u",
                            MessageBuffer->GetPayloadSize(),
                            MessageBuffer->GetTnf());

    WdfWaitLockAcquire(NfpInterface->SubsLock, NULL);

    for (ple = NfpInterface->SubsList.Flink;
         ple != &NfpInterface->SubsList;
         ple = ple->Flink) {

        NTSTATUS status = STATUS_SUCCESS;
        PNFCCX_FILE_CONTEXT fileContext = CONTAINING_RECORD(ple, NFCCX_FILE_CONTEXT, ListEntry);
        USHORT eventDataLength = 0;
        PBYTE eventData = NULL;

        WdfWaitLockAcquire(fileContext->StateLock, NULL);
        if (!fileContext->Enabled) {
            TRACE_LINE(LEVEL_WARNING, "Client %p is not enabled", fileContext);
            WdfWaitLockRelease(fileContext->StateLock);
            continue;
        }

        //
        // Does the received NDEF message match the subscription client
        //
        if (!MessageBuffer->MatchesSubscription(fileContext->TranslationType,
                                                fileContext->Tnf,
                                                fileContext->cchTypes,
                                                fileContext->pszTypes)) {
            WdfWaitLockRelease(fileContext->StateLock);
            continue;
        }

        TRACE_LINE(LEVEL_INFO, "Found matching subscription. Translation type = %d",
                                fileContext->TranslationType);

        status = MessageBuffer->GetMessagePayload(fileContext->TranslationType,
                                                  &eventDataLength,
                                                  &eventData);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to get the message payload, %!STATUS!", status);
            WdfWaitLockRelease(fileContext->StateLock);
            continue;
        }

        if (!NfpCxNfpInterfaceCompleteSubscriptionRequestLocked(
                            fileContext->RoleParameters.Sub.SubsMessageRequestQueue,
                            eventData,
                            eventDataLength)) {

            CNFCPayload* queuedMessage = NULL;

            TRACE_LINE(LEVEL_WARNING, "No request to complete, enqueuing event");

            if (NFP_MAX_PUB_SUB_QUEUE_LENGTH <= fileContext->RoleParameters.Sub.SubscribedMessageQueueLength) {
                TRACE_LINE(LEVEL_ERROR, "Too many queued messages, dropping event");
                WdfWaitLockRelease(fileContext->StateLock);
                continue;
            } else if (fileContext->IsUnresponsiveClientDetected) {
                TRACE_LINE(LEVEL_ERROR, "Ignoring event, client is unresponsive");
                WdfWaitLockRelease(fileContext->StateLock);
                continue;
            }

            queuedMessage = new CNFCPayload();

            if (NULL == queuedMessage) {
                TRACE_LINE(LEVEL_ERROR, "Could not allocate the message queue entry");
                WdfWaitLockRelease(fileContext->StateLock);
                continue;
            }

            status = queuedMessage->Initialize(eventData, eventDataLength);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to set the data payload, %!STATUS!", status);
                WdfWaitLockRelease(fileContext->StateLock);
                delete queuedMessage;
                queuedMessage = NULL;
                continue;
            }

            InsertTailList(&fileContext->RoleParameters.Sub.SubscribedMessageQueue,
                queuedMessage->GetListEntry());
            fileContext->RoleParameters.Sub.SubscribedMessageQueueLength++;

            NfcCxFileObjectStartUnresponsiveClientDetectionTimer(fileContext);
        }

        WdfWaitLockRelease(fileContext->StateLock);
    }

    WdfWaitLockRelease(NfpInterface->SubsLock);
}


VOID
NfcCxNfpInterfaceAddPublicationClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine adds the Publication client (represented by the FileContext)
    to the Publication list

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - Client to add

Return Value:

    NONE

--*/
{
    NT_ASSERTMSG("Client not for Publication",
                ROLE_PUBLICATION == FileContext->Role);

    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);

    NfcCxNfpInterfaceAddPublicationClientLocked(NfpInterface, 
                    FileContext);

    WdfWaitLockRelease(NfpInterface->PubsLock);
}

_Requires_lock_held_(NfpInterface->PubsLock)
VOID
NfcCxNfpInterfaceAddPublicationClientLocked(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine adds the Publication client (represented by the FileContext)
    to the send list while the publication lock is already held

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - Client to add

Return Value:

    NONE

--*/
{
    NT_ASSERTMSG("Client not for Publication",
                ROLE_PUBLICATION == FileContext->Role);

    InsertHeadList(&NfpInterface->PubsList, 
                    &FileContext->ListEntry);
}

VOID
NfcCxNfpInterfaceRemovePublicationClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine removes the Publication client (represented by the FileContext)
    from the Publication list

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - Client to remove

Return Value:

    NONE

--*/
{
    NT_ASSERTMSG("Client not for Publications",
                ROLE_PUBLICATION == FileContext->Role);

    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);

    RemoveEntryList(&FileContext->ListEntry);

    WdfWaitLockRelease(NfpInterface->PubsLock);
}

BOOLEAN
NfcCxNfpInterfaceAddSendClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine adds the Send client (represented by the FileContext)
    to the send list

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - Client to add

Return Value:

    NONE

--*/
{
    BOOLEAN addedToSendList = FALSE;
    NT_ASSERTMSG("Client not for Publication",
                ROLE_PUBLICATION == FileContext->Role);

    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);

    addedToSendList = NfcCxNfpInterfaceAddSendClientLocked(NfpInterface, 
                                                           FileContext);

    WdfWaitLockRelease(NfpInterface->PubsLock);

    return addedToSendList;
}

_Requires_lock_held_(NfpInterface->PubsLock)
BOOLEAN
NfcCxNfpInterfaceAddSendClientLocked(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine adds the Send client (represented by the FileContext)
    to the send list while the publication lock is already held

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - Client to add

Return Value:

    NONE

--*/
{
    BOOLEAN addedToSendList = FALSE;
    NT_ASSERTMSG("Client not for Publication",
                ROLE_PUBLICATION == FileContext->Role);

    if (!NfcCxFileObjectIsTagReadOnlyClient(FileContext)) {

        if (!IsListEmpty(&FileContext->SendListEntry)) {
            TRACE_LINE(LEVEL_WARNING, "SendList entry of client %p is not empty", FileContext);
            goto Done;
        }
        addedToSendList = TRUE;
        
        //
        // Take a reference onto the file object to make sure it doesn't
        // get deleted while it is still in the list
        //
        WdfObjectReferenceWithTag(FileContext->FileObject, SEND_LIST_ENTRY_TAG);

        InsertHeadList(&NfpInterface->SendList, 
                        &FileContext->SendListEntry);
        TRACE_LINE(LEVEL_VERBOSE, "Adding fileContext (0x%p) to SendList", FileContext);
    }
    
Done:
    return addedToSendList;
}

_Requires_lock_held_(NfpInterface->PubsLock)
VOID
NfcCxNfpInterfaceRemoveSendClientLocked(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine removes the Send client (represented by the FileContext)
    from the send list

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - Client to remove

Return Value:

    NONE

--*/
{
    NT_ASSERTMSG("Client not for Publications",
                ROLE_PUBLICATION == FileContext->Role);

    UNREFERENCED_PARAMETER(NfpInterface);

    if (!IsListEmpty(&FileContext->SendListEntry)) {
        TRACE_LINE(LEVEL_VERBOSE, "Removing fileContext (0x%p) from SendList", FileContext);
        RemoveEntryList(&FileContext->SendListEntry);
        InitializeListHead(&FileContext->SendListEntry);
        WdfObjectDereferenceWithTag(FileContext->FileObject, SEND_LIST_ENTRY_TAG);
    }
}

VOID
NfcCxNfpInterfaceRemoveSendClient(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine removes the Send client (represented by the FileContext)
    from the send list

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - Client to remove

Return Value:

    NONE

--*/
{
    NT_ASSERTMSG("Client not for Publications",
                ROLE_PUBLICATION == FileContext->Role);

    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);

    NfcCxNfpInterfaceRemoveSendClientLocked(NfpInterface,
                                            FileContext);

    WdfWaitLockRelease(NfpInterface->PubsLock);
}

_Requires_lock_held_(FileContext->StateLock)
BOOLEAN
NfcCxNfpInterfaceMustSendPublicationLocked(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

   Determines if a publication should be sent based on the client's
   attributes and the current state of the NFP interface (ie: connection state)

Arguments:

    NfpInterface - Pointer to the NFP interface
    FileContext - Pointer to the file object context

Return Value:

    TRUE if a publication should be sent, FALSE otherwise.

--*/
{
    if (NfpInterface->DeviceConnected && 
        FileContext->Enabled) {

        //
        // Verify tag attributes
        //
        if (NfpInterface->TagConnected && 
            NfcCxFileObjectIsTagWriteClient(FileContext) &&
            (NfcCxFileObjectIsTagReadOnlyClient(FileContext) ||
            0 != FileContext->RoleParameters.Pub.PublicationBuffer->GetSize()) ) {
            return TRUE;
        } 
        
        //
        // Verify device attributes
        //
        else if (!NfpInterface->TagConnected &&
                 !NfcCxFileObjectIsTagWriteClient(FileContext) &&
                 0 != FileContext->RoleParameters.Pub.PublicationBuffer->GetSize()) {
            return TRUE;
        }
    }

    return FALSE;
}

_Requires_lock_held_(NfpInterface->PubsLock)
NTSTATUS
NfcCxNfpInterfaceVerifyStateAndSendPublicationLocked(
    _In_ PNFP_INTERFACE NfpInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    Verifies the current state of the Nfp Interface and
    appropriately sends the publication.

Arguments:

    NfpInterface - A pointer to the NfpInterface
    FileContext - The file context associated with the publication

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN fShouldCleanupSendClient = FALSE;
    BOOLEAN acquiredStateLock = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NfpInterface->SendWorkerCurrentClient == FileContext) {
        TRACE_LINE(LEVEL_VERBOSE, "FileContext (0x%p) already being processed by send worker.", FileContext);
        goto Done;
    }

    WdfWaitLockAcquire(FileContext->StateLock, NULL);
    acquiredStateLock = TRUE;

    if (NfcCxNfpInterfaceMustSendPublicationLocked(NfpInterface,
                                                   FileContext)) {
        fShouldCleanupSendClient = NfcCxNfpInterfaceAddSendClientLocked(NfpInterface,
                                                                        FileContext);

        status = NfcCxNfpInterfaceScheduleSendWorker(NfpInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to schedule the send worker, %!STATUS!", status);
            goto Done;
        }

        fShouldCleanupSendClient = FALSE;
    }

Done:

    if (fShouldCleanupSendClient) {
        NfcCxNfpInterfaceRemoveSendClientLocked(NfpInterface,
                                                FileContext);
    }

    if (acquiredStateLock) {
        WdfWaitLockRelease(FileContext->StateLock);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
NfcCxNfpInterfaceCompleteReadOnlyRequests(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    This routine loops through the available clients and
    completes the readonly requests

Arguments:

    NfpInterface - A pointer to the NfpInterface

Return Value:

    VOID

--*/
{
    PLIST_ENTRY ple;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(NfpInterface->PubsLock, NULL);

    for (ple = NfpInterface->PubsList.Flink; 
         ple != &NfpInterface->PubsList; 
         ple = ple->Flink) {
        PNFCCX_FILE_CONTEXT fileContext = CONTAINING_RECORD(ple, NFCCX_FILE_CONTEXT, ListEntry);

        WdfWaitLockAcquire(fileContext->StateLock, NULL);
        if (NfcCxFileObjectIsTagReadOnlyClient(fileContext)) {
            (VOID)NfcCxFileObjectCompleteSentMessageRequestLocked(fileContext);
        }
        WdfWaitLockRelease(fileContext->StateLock);
    }

    WdfWaitLockRelease(NfpInterface->PubsLock);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxNfpInterfaceScheduleSendWorker(
    _In_ PNFP_INTERFACE NfpInterface
    )
/*++

Routine Description:

    This schedules a workitem to process the publication
    asynchronously

Arguments:

    NfpInterface - A pointer to the NfpInterface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    WdfWorkItemEnqueue(NfpInterface->SendWorker);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
NfcCxNfpInterfaceSendWorker(
    _In_ WDFWORKITEM WorkItem
    )
/*++

Routine Description:

    This routine is the workitem that is used to
    send publications to the device.  It is scheduled whenever a client
    sends a publication as well as after a connection is established
    with a device or tag

Arguments:

    WorkItem - The workitem

Return Value:

    NONE

--*/
{
    PNFP_INTERFACE nfpInterface;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN stop = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    nfpInterface = (NfcCxFdoGetContext(WdfWorkItemGetParentObject(WorkItem)))->NfpInterface;

    if (FALSE == nfpInterface->DeviceConnected) {
        TRACE_LINE(LEVEL_ERROR, "Device not connected");
        goto Done;
    }

    WdfWaitLockAcquire(nfpInterface->SendWorkerLock, NULL);

    while (FALSE == stop) {
        //
        // Look in the SendList for file objects for publishing and acquire the StateLock
        // within the PubsLock since it prevents file context cleanup in EvtFileClose.
        //
        WdfWaitLockAcquire(nfpInterface->PubsLock, NULL);

        if (IsListEmpty(&nfpInterface->SendList)) {
            WdfWaitLockRelease(nfpInterface->PubsLock);
            break;
        }

        PNFCCX_FILE_CONTEXT fileContext = CONTAINING_RECORD(RemoveHeadList(&nfpInterface->SendList), 
                                                            NFCCX_FILE_CONTEXT, 
                                                            SendListEntry);
        InitializeListHead(&fileContext->SendListEntry);
        TRACE_LINE(LEVEL_VERBOSE, "Removing fileContext (0x%p) from SendList.", fileContext);
        NT_ASSERT(nfpInterface->SendWorkerCurrentClient == NULL);
        nfpInterface->SendWorkerCurrentClient = fileContext;

        WdfWaitLockAcquire(fileContext->StateLock, NULL);

        WdfWaitLockRelease(nfpInterface->PubsLock);

        NT_ASSERT(fileContext->Role == ROLE_PUBLICATION);

        if (!fileContext->Enabled) {
            TRACE_LINE(LEVEL_WARNING, "Skipping write since the client %p is not enabled", fileContext);
            WdfWaitLockRelease(fileContext->StateLock);
            WdfObjectDereferenceWithTag(fileContext->FileObject, SEND_LIST_ENTRY_TAG);
            continue;
        }

        if (nfpInterface->TagConnected) {

           status = NfcCxRFInterfaceWriteTag(nfpInterface->FdoContext->RFInterface,
                                    fileContext->RoleParameters.Pub.PublicationBuffer);
           if (!NT_SUCCESS(status)) {
               TRACE_LINE(LEVEL_ERROR, "Tag write operation failed, %!STATUS!", status);
           } else {
               TRACE_LINE(LEVEL_INFO, "Tag Write operation successfull");
               (VOID)NfcCxFileObjectCompleteSentMessageRequestLocked(fileContext);
           }

        } else {

           status = NfcCxRFInterfaceWriteP2P(nfpInterface->FdoContext->RFInterface,
                                    fileContext->RoleParameters.Pub.PublicationBuffer);
           if (!NT_SUCCESS(status)) {
               TRACE_LINE(LEVEL_ERROR, "P2P write operation failed, %!STATUS!", status);
               stop = TRUE;
           } else {
               TRACE_LINE(LEVEL_INFO, "P2P Write operation successfull");
               (VOID)NfcCxFileObjectCompleteSentMessageRequestLocked(fileContext);
           }
        }

        WdfWaitLockRelease(fileContext->StateLock);

        WdfWaitLockAcquire(nfpInterface->PubsLock, NULL);

        TRACE_LINE(LEVEL_VERBOSE, "Finished processing SendWorkerCurrentClient (0x%p)", fileContext);
        NT_ASSERT(nfpInterface->SendWorkerCurrentClient == fileContext);
        nfpInterface->SendWorkerCurrentClient = NULL;

        WdfWaitLockRelease(nfpInterface->PubsLock);

        WdfObjectDereferenceWithTag(fileContext->FileObject, SEND_LIST_ENTRY_TAG);
    }

    if (nfpInterface->TagConnected && 
        NfcCxNfpInterfaceCheckIfReadOnlyTagPublicationsExist(nfpInterface)) {

        status = NfcCxRFInterfaceConvertToReadOnlyTag(nfpInterface->FdoContext->RFInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Tag convert to readonly operation failed, %!STATUS!", status);
        } else {
            TRACE_LINE(LEVEL_INFO, "Tag convert to readonly operation successfull");
            (VOID)NfcCxNfpInterfaceCompleteReadOnlyRequests(nfpInterface);
        }
    }

    WdfWaitLockRelease(nfpInterface->SendWorkerLock);

Done:

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);

    return;
}

NTSTATUS
NfcCxNfpInterfaceIoDispatch (
    _In_opt_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++

Routine Description:

    This is the first entry into the NfpInterface.  It validates and dispatches Nfp request
    as appropriate.

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    OutputBufferLength - Length of the output buffer associated with the request.
    InputBufferLength - Length of the input buffer associated with the request.
    IoControlCode - IOCTL code.

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR bytesCopied = 0;
    WDFMEMORY outMem = NULL;
    WDFMEMORY inMem = NULL;
    PVOID     inBuffer = NULL;
    PVOID     outBuffer = NULL;
    size_t    sizeInBuffer = 0;
    size_t    sizeOutBuffer = 0;

    UNREFERENCED_PARAMETER(bytesCopied);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // All NFP handlers requires a valid file context
    //
    if (NULL == FileContext) {
        TRACE_LINE(LEVEL_ERROR, "NFP request received without a file context");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    //
    // Is the NFP radio enabled
    //
    if (FALSE == NfcCxPowerIsAllowedNfp(NfcCxFileObjectGetFdoContext(FileContext)->Power)) {
        TRACE_LINE(LEVEL_ERROR, "NFP radio is off");
        status = STATUS_DEVICE_POWERED_OFF;
        goto Done;
    }

    //
    // Get the request memory and perform the operation here
    //
    if (0 != OutputBufferLength) {
        status = WdfRequestRetrieveOutputMemory(Request, &outMem);
        if(!NT_SUCCESS(status) ) {
            TRACE_LINE(LEVEL_ERROR, "Failed to retrieve the output buffer, %!STATUS!", status);
            goto Done;
        }

        outBuffer = WdfMemoryGetBuffer(outMem, &sizeOutBuffer);
        NT_ASSERT(sizeOutBuffer == OutputBufferLength);
        NT_ASSERT(NULL != outBuffer);
    }
    if (0 != InputBufferLength) {
        status = WdfRequestRetrieveInputMemory(Request, &inMem);
        if(!NT_SUCCESS(status) ) {
            TRACE_LINE(LEVEL_ERROR, "Failed to retrieve the input buffer, %!STATUS!", status);
            goto Done;
        }

        inBuffer = WdfMemoryGetBuffer(inMem, &sizeInBuffer);
        NT_ASSERT(sizeInBuffer == InputBufferLength);
        NT_ASSERT(NULL != inBuffer);
    }

    //
    // Validate the request
    //
    status = NfcCxNfpInterfaceValidateRequest(IoControlCode,
                                              InputBufferLength,
                                              OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Request validation failed, %!STATUS!", status);
        goto Done;
    }

    //
    // Dispatch the request
    //
    status = NfcCxNfpInterfaceDispatchRequest(FileContext,
                                              Request,
                                              IoControlCode,
                                              inBuffer,
                                              InputBufferLength,
                                              outBuffer,
                                              OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Request dispatch failed, %!STATUS!", status);
        goto Done;
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxNfpInterfaceValidateRequest (
    _In_ ULONG        IoControlCode,
    _In_ size_t       InputBufferLength,
    _In_ size_t       OutputBufferLength
    )
/*++

Routine Description:

    This routine validates the Nfp request.

Arguments:

    IoControlCode - IOCTL code.
    InputBufferLength - Length of the input buffer associated with the request.
    OutputBufferLength - Length of the output buffer associated with the request.

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_INVALID_DEVICE_STATE;
    USHORT i = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (i = 0; i < ARRAYSIZE(g_NfpDispatch); i++) {
        if (g_NfpDispatch[i].IoControlCode == IoControlCode) {
            
            if (g_NfpDispatch[i].MinimumInputBufferLength > InputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Input buffer.  Expected %I64x, got %I64x",
                    g_NfpDispatch[i].MinimumInputBufferLength,
                    InputBufferLength);
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            if (g_NfpDispatch[i].MinimumOutputBufferLength > OutputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Output buffer.  Expected %I64x, got %I64x",
                    g_NfpDispatch[i].MinimumOutputBufferLength,
                    OutputBufferLength);
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            status = STATUS_SUCCESS;
            break;
        }
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxNfpInterfaceDispatchRequest(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the Nfp request to the appropriate handler.

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    IoControlCode - The Io Control Code of the Nfp Request.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_INVALID_DEVICE_STATE;
    USHORT i = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (i = 0; i < ARRAYSIZE(g_NfpDispatch); i++) {
        if (g_NfpDispatch[i].IoControlCode == IoControlCode) {

            status = g_NfpDispatch[i].DispatchHandler(FileContext,
                                                      Request,
                                                      InputBuffer,
                                                      InputBufferLength,
                                                      OutputBuffer,
                                                      OutputBufferLength
                                                      );
            break;
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxNfpInterfaceDispatchEnable(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the Nfp Enable

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    status = NfcCxFileObjectNfpEnable(FileContext);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to enable the pub/sub, %!STATUS!", status);
        goto Done;
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxNfpInterfaceDispatchDisable(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the Nfp Disable

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    status = NfcCxFileObjectNfpDisable(FileContext);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to disable the pub/sub, %!STATUS!", status);
        goto Done;
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxNfpInterfaceDispatchGetNextSubscribedMessage(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the Nfp Get Next Subscribed Message

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);

#ifdef EVENT_WRITE
    EventWriteNfpGetNextSubscribedMsgStart(FileContext->FileObject);
#endif

    if (ROLE_SUBSCRIPTION != FileContext->Role) {
        TRACE_LINE(LEVEL_ERROR, "Invalid role for request");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL for GetNextSubscribedMessage");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(FileContext->StateLock, NULL);

    if (!FileContext->Enabled) {
        TRACE_LINE(LEVEL_ERROR, "FileContext disabled");
        status = STATUS_INVALID_DEVICE_STATE;
        WdfWaitLockRelease(FileContext->StateLock);
        goto Done;
    }

    //
    // Make sure the unresponsive client detection timer is stopped
    //
    NfcCxFileObjectStopUnresponsiveClientDetectionTimer(FileContext, FALSE);
    NfcCxFileObjectResetUnresponsiveClientDetection(FileContext);

    //
    // If there are any queued messages, dispatch and complete it now
    //
    if (!IsListEmpty(&FileContext->RoleParameters.Sub.SubscribedMessageQueue)) {

        //
        // Get the head of the list and complete the request
        //
        PLIST_ENTRY ple = FileContext->RoleParameters.Sub.SubscribedMessageQueue.Flink;
        ULONG usedBufferSize = 0;
        CNFCPayload * pBuffer = CNFCPayload::FromListEntry(ple);

        //
        // Complete the request
        //
        status = NfcCxNfpInterfaceCopySubcriptionData(OutputBuffer,
                                                      (ULONG)OutputBufferLength,
                                                      pBuffer->GetPayload(),
                                                      pBuffer->GetSize(),
                                                      &usedBufferSize);
        if (STATUS_BUFFER_OVERFLOW != status &&
            !NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to copy the subscription data, %!STATUS!", status);
            WdfWaitLockRelease(FileContext->StateLock);
            goto Done;
        } else if (NT_SUCCESS(status)) {
            RemoveEntryList(ple);
            FileContext->RoleParameters.Sub.SubscribedMessageQueueLength--;
            delete pBuffer;
        }

#ifdef EVENT_WRITE
        EventWriteNfpGetNextSubscribedMsgStop(FileContext->FileObject, status, usedBufferSize);
#endif

        WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    } else {

        WDFREQUEST subRequest = NULL;

        TRACE_LINE(LEVEL_INFO, "No subscribed message available. Placing request into subcribed message queue");

        //
        // Ensure that there are no other requests in the queue.
        //
        status = WdfIoQueueFindRequest(FileContext->RoleParameters.Sub.SubsMessageRequestQueue,
                                       NULL,
                                       FileContext->FileObject,
                                       NULL,
                                       &subRequest);

        if (subRequest) {
            TRACE_LINE(LEVEL_ERROR, "A GetNextSubscribedMessage request is already pending in the queue");
            WdfObjectDereference(subRequest);
            status = STATUS_INVALID_DEVICE_STATE;
            WdfWaitLockRelease(FileContext->StateLock);
            goto Done;
        }
        else {
            NT_ASSERT(STATUS_NO_MORE_ENTRIES == status);
        }

        //
        // Else, forward the request to the holding queue
        //
        status = WdfRequestForwardToIoQueue(Request,
                                FileContext->RoleParameters.Sub.SubsMessageRequestQueue);

        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to forward the request to the SubsMessageRequestQueue, %!STATUS!", status);
            WdfWaitLockRelease(FileContext->StateLock);
            goto Done;
        }
    }

    WdfWaitLockRelease(FileContext->StateLock);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

Done:

#ifdef EVENT_WRITE
    if (!NT_SUCCESS(status)) {
        EventWriteNfpGetNextSubscribedMsgStop(FileContext->FileObject, status, 0);
    }
#endif

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxNfpInterfaceDispatchSetPayload(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the Nfp Set Payload

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL for SetPayload");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO,
        "Set Payload for client role %!FILE_OBJECT_ROLE! and %!TRANSLATION_TYPE_PROTOCOL!", 
        FileContext->Role, FileContext->TranslationType);

    //
    // Verify and set the payload
    //
    status = NfcCxFileObjectValidateAndSetPayload(FileContext, 
                                                  InputBuffer, 
                                                  InputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to set the payload, %!STATUS!", status);
        goto Done;
    }

#ifdef EVENT_WRITE
    EventWriteNfpSetPayload(FileContext->FileObject, InputBufferLength);
#endif

    //
    // If the file object is enabled, and we are already connected,
    // we must send it right away
    //
    WdfWaitLockAcquire(NfcCxFileObjectGetNfpInterface(FileContext)->PubsLock, NULL);

    status = NfcCxNfpInterfaceVerifyStateAndSendPublicationLocked(NfcCxFileObjectGetNfpInterface(FileContext),
                                                                  FileContext);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to verify the state and send the publication, %!STATUS!", status);
        WdfWaitLockRelease(NfcCxFileObjectGetNfpInterface(FileContext)->PubsLock);
        goto Done;
    }
    WdfWaitLockRelease(NfcCxFileObjectGetNfpInterface(FileContext)->PubsLock);


Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxNfpInterfaceDispatchGetNextTransmittedMessage(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the Nfp Get Next Transmitted Message

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);

    TRACE_LINE(LEVEL_INFO,
        "Get Next Transmitted Message for client role %!FILE_OBJECT_ROLE! and %!TRANSLATION_TYPE_PROTOCOL!", 
        FileContext->Role, FileContext->TranslationType);

#ifdef EVENT_WRITE
    EventWriteNfpGetNextTransmittedMsgStart(FileContext->FileObject);
#endif

    if (ROLE_PUBLICATION != FileContext->Role) {
        TRACE_LINE(LEVEL_ERROR, "Invalid role for request");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL for GetNextTransmittedMessage");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL for GetNextTransmittedMessage");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(FileContext->StateLock, NULL);

    //
    // Is there already a payload set?
    //
    if (NULL == FileContext->RoleParameters.Pub.PublicationBuffer->Get() &&
        FileContext->TranslationType != TRANSLATION_TYPE_SETTAG_READONLY) {
        TRACE_LINE(LEVEL_ERROR, "Publication not set");
        status = STATUS_INVALID_DEVICE_STATE;
        WdfWaitLockRelease(FileContext->StateLock);
        goto Done;
    }

    if (!FileContext->Enabled) {
        TRACE_LINE(LEVEL_ERROR, "FileContext disabled");
        status = STATUS_INVALID_DEVICE_STATE;
        WdfWaitLockRelease(FileContext->StateLock);
        goto Done;
    }

    //
    // Make sure the unresponsive client detection timer is stopped
    //
    NfcCxFileObjectStopUnresponsiveClientDetectionTimer(FileContext, FALSE);
    NfcCxFileObjectResetUnresponsiveClientDetection(FileContext);

    if (0 != FileContext->RoleParameters.Pub.SentMsgCounter) {

        TRACE_LINE(LEVEL_INFO, "Completed queued notification");
        FileContext->RoleParameters.Pub.SentMsgCounter--;

#ifdef EVENT_WRITE
        EventWriteNfpGetNextTransmittedMsgStop(FileContext->FileObject, STATUS_SUCCESS);
#endif

        WdfRequestComplete(Request, STATUS_SUCCESS);

    } else {

        WDFREQUEST pubRequest = NULL;
        //
        // Ensure that there are no other requests in the queue.
        //
        status = WdfIoQueueFindRequest(FileContext->RoleParameters.Pub.SendMsgRequestQueue,
                                       NULL,
                                       FileContext->FileObject,
                                       NULL,
                                       &pubRequest);

        if (pubRequest) {
            TRACE_LINE(LEVEL_ERROR, "A GetNextTransMessage request is already pending in the queue");
            WdfObjectDereference(pubRequest);
            status = STATUS_INVALID_DEVICE_STATE;
            WdfWaitLockRelease(FileContext->StateLock);
            goto Done;
        }
        else {
            NT_ASSERT(STATUS_NO_MORE_ENTRIES == status);
        }

        status = WdfRequestForwardToIoQueue(Request, 
                                FileContext->RoleParameters.Pub.SendMsgRequestQueue);

        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to forward the request to the SendMsgRequestQueue, %!STATUS!", status);
            WdfWaitLockRelease(FileContext->StateLock);
            goto Done;
        }
    }

    WdfWaitLockRelease(FileContext->StateLock);

    //
    // Now that the request is is the holding queue or that we completed it
    // return STATUS_PENDING so the request isn't completed
    //
    status = STATUS_PENDING;

Done:

#ifdef EVENT_WRITE
    if (!NT_SUCCESS(status)) {
        EventWriteNfpGetNextTransmittedMsgStop(FileContext->FileObject, status);
    }
#endif

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxNfpInterfaceDispatchGetMaxMessageBytes(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the Nfp Get Max Message Bytes

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PULONG maximumMessagePayloadSize;

    UNREFERENCED_PARAMETER(FileContext);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(NULL != OutputBuffer);
    _Analysis_assume_(sizeof(*maximumMessagePayloadSize) <= OutputBufferLength);

    maximumMessagePayloadSize = (PULONG)OutputBuffer;
    *maximumMessagePayloadSize = NFP_MAXIMUM_MESSAGE_PAYLOAD_SIZE;

    WdfRequestCompleteWithInformation(Request, status, sizeof(*maximumMessagePayloadSize));

    //
    // Since we have completed the request here, 
    // return STATUS_PENDING to avoid double completion
    // of the request
    //
    status = STATUS_PENDING;

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

NTSTATUS
NfcCxNfpInterfaceDispatchGetKbps(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the Nfp Get Kbps

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PULONG averageTxSpeed;

    UNREFERENCED_PARAMETER(FileContext);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(NULL != OutputBuffer);
    _Analysis_assume_(sizeof(*averageTxSpeed) <= OutputBufferLength);

    averageTxSpeed = (PULONG)OutputBuffer;
    *averageTxSpeed = NFP_AVERAGE_TRANSMIT_SPEED_KBPS;

    WdfRequestCompleteWithInformation(Request, status, sizeof(*averageTxSpeed));

    //
    // Since we have completed the request here, 
    // return STATUS_PENDING to avoid double completion
    // of the request
    //
    status = STATUS_PENDING;

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

