/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxSE.cpp

Abstract:

    SE Interface declaration

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxSE.tmh"

typedef struct _NFCCX_SE_DISPATCH_ENTRY {
    ULONG IoControlCode;
    BOOLEAN fPowerManaged;
    BOOLEAN fSequentialDispatch;
    size_t MinimumInputBufferLength;
    size_t MinimumOutputBufferLength;
    PFN_NFCCX_SE_DISPATCH_HANDLER DispatchHandler;
} NFCCX_SE_DISPATCH_ENTRY, *PNFCCX_SE_DISPATCH_ENTRY;

NFCCX_SE_DISPATCH_ENTRY
g_SEDispatch [] = {
    { IOCTL_NFCSE_ENUM_ENDPOINTS,           TRUE,   TRUE,   0,                                                      sizeof(DWORD),      NfcCxSEInterfaceDispatchEnumEndpoints },
    { IOCTL_NFCSE_GET_NEXT_EVENT,           FALSE,  FALSE,  0,                                                      sizeof(DWORD),      NfcCxSEInterfaceDispatchGetNextEvent },
    { IOCTL_NFCSE_SUBSCRIBE_FOR_EVENT,      TRUE,   FALSE,  sizeof(SECURE_ELEMENT_EVENT_SUBSCRIPTION_INFO),         0,                  NfcCxSEInterfaceDispatchSubscribeForEvent },
    { IOCTL_NFCSE_SET_CARD_EMULATION_MODE,  TRUE,   TRUE,   sizeof(SECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO),    0,                  NfcCxSEInterfaceDispatchSetCardEmulationMode },
    { IOCTL_NFCSE_GET_NFCC_CAPABILITIES,    TRUE,   FALSE,  0,                                                      sizeof(SECURE_ELEMENT_NFCC_CAPABILITIES),
                                                                                                                                        NfcCxSEInterfaceDispatchGetNfccCapabilities },
    { IOCTL_NFCSE_GET_ROUTING_TABLE,        TRUE,   TRUE,   0,                                                      sizeof(DWORD),      NfcCxSEInterfaceDispatchGetRoutingTable },
    { IOCTL_NFCSE_SET_ROUTING_TABLE,        TRUE,   TRUE,   sizeof(SECURE_ELEMENT_ROUTING_TABLE),                   0,                  NfcCxSEInterfaceDispatchSetRoutingTable },
    { IOCTL_NFCSE_HCE_REMOTE_RECV,          FALSE,  FALSE,  0,                                                      sizeof(DWORD),      NfcCxSEInterfaceDispatchHCERemoteRecv },
    { IOCTL_NFCSE_HCE_REMOTE_SEND,          TRUE,   TRUE,   2 * sizeof(USHORT) + DEFAULT_APDU_STATUS_SIZE,          0,                  NfcCxSEInterfaceDispatchHCERemoteSend },
};

NTSTATUS
NfcCxSEInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT DeviceContext,
    _Out_ PNFCCX_SE_INTERFACE * PPSEInterface
    )
/*++

Routine Description:

    This routine creates and initalizes the SE Interface.

Arguments:

    FdoContext - A pointer to the FdoContext
    PPSEInterface - A pointer to a memory location to receive the allocated SE interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_SE_INTERFACE seInterface = NULL;
    WDF_OBJECT_ATTRIBUTES objectAttrib;
    WDF_IO_QUEUE_CONFIG queueConfig;
    
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    seInterface = (PNFCCX_SE_INTERFACE)malloc(sizeof(*seInterface));
    if (NULL == seInterface) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the SE interface");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    RtlZeroMemory(seInterface, sizeof((*seInterface)));
    seInterface->FdoContext = DeviceContext;
    InitializeListHead(&seInterface->SEEventsList);

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = seInterface->FdoContext->Device;

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                             WdfIoQueueDispatchParallel);

    queueConfig.PowerManaged = WdfFalse;
    queueConfig.EvtIoDeviceControl = NfcCxSEInterfaceSequentialIoDispatch;
    queueConfig.Settings.Parallel.NumberOfPresentedRequests = 1;

    status = WdfIoQueueCreate(seInterface->FdoContext->Device,
                              &queueConfig,
                              &objectAttrib,
                              &seInterface->SerialIoQueue);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed WdfIoQueueCreate, %!STATUS!", status);
        goto Done;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = seInterface->FdoContext->Device;

    status = WdfWaitLockCreate(&objectAttrib,
                               &seInterface->SEManagerLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create SEManagerLock, %!STATUS!", status);
        goto Done;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = seInterface->FdoContext->Device;

    status = WdfWaitLockCreate(&objectAttrib,
                               &seInterface->SEEventsLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create SEEventsLock, %!STATUS!", status);
        goto Done;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = seInterface->FdoContext->Device;

    status = WdfWaitLockCreate(&objectAttrib,
                               &seInterface->SEPowerSettingsLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create EmulationModesLock, %!STATUS!", status);
        goto Done;
    }

Done:

    if (!NT_SUCCESS(status)) {
        if (NULL != seInterface) {
            NfcCxSEInterfaceDestroy(seInterface);
            seInterface = NULL;
        }
    }

    *PPSEInterface = seInterface;

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxSEInterfaceDestroy(
    _In_ PNFCCX_SE_INTERFACE SEInterface
    )
{
    //
    // Since the lock objects are parented to the device,
    // there are no needs to manually delete them here
    //
    if (SEInterface->InterfaceCreated) {
        //
        // Disable the SE interface
        //
        WdfDeviceSetDeviceInterfaceState(SEInterface->FdoContext->Device,
                                         (LPGUID) &GUID_DEVINTERFACE_NFCSE,
                                         NULL,
                                         FALSE);

        TRACE_LINE(LEVEL_VERBOSE, "SE interface disabled");
    }

    free(SEInterface);
}

NTSTATUS
NfcCxSEInterfaceStart(
    _In_ PNFCCX_SE_INTERFACE SEInterface
    )
/*++

Routine Description:

    Start the SE Interface

Arguments:

    SEInterface - The SE Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext = SEInterface->FdoContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (FALSE == fdoContext->SERadioInterfaceCreated) {
        //
        // Create and publish the SE RM Interface
        //
        status = WdfDeviceCreateDeviceInterface(
                        fdoContext->Device,
                        (LPGUID)&GUID_NFCSE_RADIO_MEDIA_DEVICE_INTERFACE,
                        NULL);
        if (!NT_SUCCESS (status)) {
            TRACE_LINE(LEVEL_ERROR, "WdfDeviceCreateDeviceInterface failed %!STATUS!", status);
            goto Done;
        }

        WdfDeviceSetDeviceInterfaceState(fdoContext->Device,
                                         (LPGUID)&GUID_NFCSE_RADIO_MEDIA_DEVICE_INTERFACE,
                                         NULL,
                                         TRUE);

        fdoContext->SERadioInterfaceCreated = TRUE;
    }

    // Note: If both NFCEE emulation and HCE have been disabled by the client driver, then there will be
    // nothing for the SE interface to do. So don't bother enabling it.
    ULONG driverFlags = SEInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags;
    BOOLEAN seFullyDisabled = (driverFlags & NFC_CX_DRIVER_DISABLE_HOST_CARD_EMULATION) &&
        (driverFlags & NFC_CX_DRIVER_DISABLE_NFCEE_DISCOVERY);

    BOOLEAN enableInterface = fdoContext->Power->SERadioState && !seFullyDisabled;

    if (enableInterface)
    {
        //
        // Publish the SE interface
        //
        if (!SEInterface->InterfaceCreated) {
            status = WdfDeviceCreateDeviceInterface(fdoContext->Device,
                                                    (LPGUID) &GUID_DEVINTERFACE_NFCSE,
                                                    NULL);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to create the SE device interface, %!STATUS!", status);
                goto Done;
            }

            SEInterface->InterfaceCreated = TRUE;
        }
    }

    if (SEInterface->InterfaceCreated)
    {
        WdfDeviceSetDeviceInterfaceState(fdoContext->Device,
                                         (LPGUID) &GUID_DEVINTERFACE_NFCSE,
                                         NULL,
                                         enableInterface);
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxSEInterfaceStop(
    _In_ PNFCCX_SE_INTERFACE SEInterface
    )
/*++

Routine Description:

    Stop the SE Interface

Arguments:

    SEInterface - The SE Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (SEInterface->InterfaceCreated) {

        WdfDeviceSetDeviceInterfaceState(SEInterface->FdoContext->Device,
                                         (LPGUID) &GUID_DEVINTERFACE_NFCSE,
                                         NULL,
                                         FALSE);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

BOOLEAN 
NfcCxSEInterfaceIsIoctlSupported(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This routine returns true if the provided IOCTL is supported by the
    module.
    
Arguments:

    FdoContext - The FDO Context
    IoControlCode - The IOCTL code to check

Return Value:

    TRUE - The IOCTL is supported by this module
    FALSE - The IOCTL is not supported by this module

--*/
{
    ULONG i;

    UNREFERENCED_PARAMETER(FdoContext);

    for (i=0; i < ARRAYSIZE(g_SEDispatch); i++) {
        if (g_SEDispatch[i].IoControlCode == IoControlCode) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
NfcCxSEIsPowerManagedRequest(
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This routine returns true if the provided IOCTL is power managed.
    
Arguments:

    IoControlCode - The IOCTL code to check

Return Value:

    TRUE - The IOCTL is power managed
    FALSE - The IOCTL is not power managed

--*/
{
    ULONG i;

    for (i=0; i < ARRAYSIZE(g_SEDispatch); i++) {
        if (g_SEDispatch[i].IoControlCode == IoControlCode) {
            return g_SEDispatch[i].fPowerManaged;
        }
    }

    return FALSE;
}

BOOLEAN
NfcCxSEIsSequentialDispatchRequest(
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This routine returns true if the provided IOCTL requires
    sequential dispatch.
    
Arguments:

    IoControlCode - The IOCTL code to check

Return Value:

    TRUE - The IOCTL requires sequential dispatch
    FALSE - The IOCTL doesn't requires sequential dispatch

--*/
{
    ULONG i;

    for (i=0; i < ARRAYSIZE(g_SEDispatch); i++) {
        if (g_SEDispatch[i].IoControlCode == IoControlCode) {
            return g_SEDispatch[i].fSequentialDispatch;
        }
    }

    return FALSE;
}

VOID
NfcCxSEInterfaceSequentialIoDispatch(
    _In_ WDFQUEUE      Queue,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++

Routine Description:

    This is the sequential dispatch IoControl handler for the SE module.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the I/O request.
    Request - Handle to a framework request object.
    OutputBufferLength - Length of the output buffer associated with the request.
    InputBufferLength - Length of the input buffer associated with the request.
    IoControlCode - IOCTL code.

Return Value:

  None.

--*/
{
    NTSTATUS  status = STATUS_SUCCESS;
    PNFCCX_FILE_CONTEXT fileContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Queue);

    fileContext = NfcCxFileGetContext(WdfRequestGetFileObject(Request));

    status = NfcCxSEInterfaceIoDispatch(fileContext,
                                        Request,
                                        OutputBufferLength,
                                        InputBufferLength,
                                        IoControlCode);

    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
    } else if (STATUS_SUCCESS == status) {
        WdfRequestComplete(Request, status);
    } else {
        //
        // At this point we know the NT_SUCCESS macro was satisfied, 
        // this means that one of the dispatch should have completed
        // the request already.
        //
        NT_ASSERT(STATUS_PENDING == status);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS 
NfcCxSEInterfaceIoDispatch(
    _In_opt_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++

Routine Description:

    This is the first entry into the SEInterface.  It validates and dispatches SE request
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
    PNFCCX_FDO_CONTEXT fdoContext;
    WDFMEMORY outMem = {0};
    WDFMEMORY inMem = {0};
    PVOID     inBuffer = NULL;
    PVOID     outBuffer = NULL;
    size_t    sizeInBuffer = 0;
    size_t    sizeOutBuffer = 0;
    BOOLEAN   releasePowerReference = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    fdoContext = NfcCxFileObjectGetFdoContext(FileContext);

    //
    // Is the SE radio enabled?
    //
    if (FALSE == NfcCxPowerIsAllowedSE(fdoContext->Power)) {
        TRACE_LINE(LEVEL_ERROR, "SE radio is off");
        status = STATUS_DEVICE_POWERED_OFF;
        goto Done;
    }

    //
    // Forward the request to sequential dispatch IO queue
    //
    if (NfcCxSEIsSequentialDispatchRequest(IoControlCode) &&
        fdoContext->SEInterface->SerialIoQueue != WdfRequestGetIoQueue(Request)) {
        status = WdfRequestForwardToIoQueue(Request,
                                            fdoContext->SEInterface->SerialIoQueue);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed WdfRequestForwardToIoQueue, %!STATUS!", status);
            goto Done;
        }
        status = STATUS_PENDING;
        goto Done;
    }

    //
    // Take a power reference if the request is power managed
    //
    if (NfcCxSEIsPowerManagedRequest(IoControlCode)) {
        status = WdfDeviceStopIdle(fdoContext->Device,
                               TRUE);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed WdfDeviceStopIdle, %!STATUS!", status);
            goto Done;
        }
        releasePowerReference = TRUE;
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
    status = NfcCxSEInterfaceValidateRequest(IoControlCode,
                                             InputBufferLength,
                                             OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Request validation failed, %!STATUS!", status);
        goto Done;
    }

    //
    // Dispatch the request
    //
    status = NfcCxSEInterfaceDispatchRequest(FileContext,
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
    if (releasePowerReference) {
        WdfDeviceResumeIdle(fdoContext->Device);
        releasePowerReference = FALSE;
    }
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceValidateRequest(
    _In_ ULONG IoControlCode,
    _In_ size_t InputBufferLength,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine validates the SE request.

Arguments:

    IoControlCode - IOCTL code.
    InputBufferLength - Length of the input buffer associated with the request.
    OutputBufferLength - Length of the output buffer associated with the request.

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    USHORT i = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (i = 0; i < ARRAYSIZE(g_SEDispatch); i++) {
        if (g_SEDispatch[i].IoControlCode == IoControlCode) {
            
            if (g_SEDispatch[i].MinimumInputBufferLength > InputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Input buffer.  Expected %I64x, got %I64x",
                    g_SEDispatch[i].MinimumInputBufferLength,
                    InputBufferLength);
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            if (g_SEDispatch[i].MinimumOutputBufferLength > OutputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Output buffer.  Expected %I64x, got %I64x",
                    g_SEDispatch[i].MinimumOutputBufferLength,
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
NfcCxSEInterfaceDispatchRequest(
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

    This routine dispatches the SE request to the appropriate handler.

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    IoControlCode - The Io Control Code of the SE Request.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    USHORT i = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (i = 0; i < ARRAYSIZE(g_SEDispatch); i++) {
        if (g_SEDispatch[i].IoControlCode == IoControlCode) {

            status = g_SEDispatch[i].DispatchHandler(FileContext,
                                                     Request,
                                                     InputBuffer,
                                                     InputBufferLength,
                                                     OutputBuffer,
                                                     OutputBufferLength);
            break;
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceAddClient(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine holds the reference of the client context

Arguments:

    SEInterface - A pointer to the SEInterface
    FileContext - Client to add
    
Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NfcCxFileObjectIsSEManager(FileContext)) {

        WdfWaitLockAcquire(SEInterface->SEManagerLock, NULL);

        if (SEInterface->SEManager != NULL) {
            TRACE_LINE(LEVEL_ERROR, "There is existing file handle on the SEManager");
            status = STATUS_ACCESS_DENIED;

        } else {
            SEInterface->SEManager = FileContext;
        }

        WdfWaitLockRelease(SEInterface->SEManagerLock);
        
    } else if (NfcCxFileObjectIsSEEvent(FileContext)) {

        WdfWaitLockAcquire(SEInterface->SEEventsLock, NULL);
        InsertHeadList(&SEInterface->SEEventsList, 
                       &FileContext->ListEntry);
        WdfWaitLockRelease(SEInterface->SEEventsLock);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxSEInterfaceRemoveClient(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine release the reference  of the client

Arguments:

    Interface - A pointer to the SEInterface
    FileContext - Client to remove
    
Return Value:

    VOID

--*/
{
    PNFCCX_RF_INTERFACE rfInterface = SEInterface->FdoContext->RFInterface;
    phLibNfc_SE_List_t SEList[MAX_NUMBER_OF_SE] = {0};
    uint8_t SECount = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NfcCxFileObjectIsSEManager(FileContext))
    {
        WdfWaitLockAcquire(SEInterface->SEManagerLock, NULL);

        if (SEInterface->SEManager != FileContext) {
            WdfWaitLockRelease(SEInterface->SEManagerLock);
            goto Done;
        }

        SEInterface->SEManager = NULL;
        TRACE_LINE(LEVEL_INFO, "SEManager %p removed", FileContext);

        WdfWaitLockRelease(SEInterface->SEManagerLock);
        
        if (!NT_SUCCESS(NfcCxRFInterfaceGetSecureElementList(rfInterface, SEList, &SECount))) {
            TRACE_LINE(LEVEL_WARNING, "Failed NfcCxRFInterfaceGetSecureElementList");
            goto Done;
        }

        //
        // Force turn off card emulation mode in case if it is still left on when the
        // SE manager handle is being released
        //
        for (uint8_t i = 0; i < SECount; i++)
        {
            if (SEList[i].eSE_ActivationMode == phLibNfc_SE_ActModeOff) {
                continue;
            }

            GUID secureElementId = NfcCxSEInterfaceGetSecureElementId(rfInterface, SEList[i].hSecureElement);
            bool isEmulationOn = false;

            {
                // Acquire lock for 'SEInterface->SEPowerSettings'.
                // Note: We can't call 'NfcCxSEInterfaceSetCardEmulationMode' while holding the lock as WDF locks
                // aren't recursive.
                WdfWaitLockAcquire(SEInterface->SEPowerSettingsLock, NULL);

                NFCCX_POWER_SETTING* sePower = NfcCxSEInterfaceGetSettingsListItem(SEInterface, secureElementId);
                isEmulationOn = sePower != nullptr && sePower->EmulationMode != EmulationOff;

                WdfWaitLockRelease(SEInterface->SEPowerSettingsLock);
            }

            if (!isEmulationOn)
            {
                continue;
            }

            SECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO EmulationMode;
            EmulationMode.eMode = EmulationOff;
            EmulationMode.guidSecureElementId = secureElementId;
            (void)NfcCxSEInterfaceSetCardEmulationMode(FileContext, &EmulationMode);
        }
    }
    else if (NfcCxFileObjectIsSEEvent(FileContext))
    {
        WdfWaitLockAcquire(SEInterface->SEEventsLock, NULL);
        RemoveEntryList(&FileContext->ListEntry);
        WdfWaitLockRelease(SEInterface->SEEventsLock);
    }

Done:

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxSEInterfaceHandleEvent(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ SECURE_ELEMENT_EVENT_TYPE EventType,
    _In_ phLibNfc_SE_List_t *pSEInfo,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo
    )
{
    PLIST_ENTRY ple;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(SEInterface->SEEventsLock, NULL);

    for (ple = SEInterface->SEEventsList.Flink;
         ple != &SEInterface->SEEventsList;
         ple = ple->Flink) {

        PNFCCX_FILE_CONTEXT fileContext = CONTAINING_RECORD(ple, NFCCX_FILE_CONTEXT, ListEntry);
        CNFCPayload* queuedEvent = NULL;

        WdfWaitLockAcquire(fileContext->StateLock, NULL);

        if (!NT_SUCCESS(NfcCxSEInterfaceGetEventPayload(SEInterface->FdoContext->RFInterface,
                                                        pSEInfo,
                                                        EventType,
                                                        pSeEvtInfo,
                                                        &queuedEvent))) {
            WdfWaitLockRelease(fileContext->StateLock);
            continue;
        }

        if (!NfcCxSEInterfaceMatchesEvent(queuedEvent, fileContext)) {
            WdfWaitLockRelease(fileContext->StateLock);
            delete queuedEvent;
            continue;
        }

        if (!NfcCxSEInterfaceCompleteRequestLocked(fileContext->RoleParameters.SEEvent.EventRequestQueue,
                                                   queuedEvent->GetPayload(),
                                                   queuedEvent->GetSize())) {

            TRACE_LINE(LEVEL_WARNING, "No request to complete, enqueuing event");

            if (NFP_MAX_PUB_SUB_QUEUE_LENGTH <= fileContext->RoleParameters.SEEvent.EventQueueLength) {
                TRACE_LINE(LEVEL_ERROR, "Too many queued, dropping event");
                WdfWaitLockRelease(fileContext->StateLock);
                delete queuedEvent;
                continue;
            } else if (fileContext->IsUnresponsiveClientDetected) {
                TRACE_LINE(LEVEL_ERROR, "Ignoring even, client is unresponsive");
                WdfWaitLockRelease(fileContext->StateLock);
                delete queuedEvent;
                continue;
            }

            InsertTailList(&fileContext->RoleParameters.SEEvent.EventQueue, 
                queuedEvent->GetListEntry());
            fileContext->RoleParameters.SEEvent.EventQueueLength++;

            NfcCxFileObjectStartUnresponsiveClientDetectionTimer(fileContext);
        
        } else {
            delete queuedEvent;
        }

        WdfWaitLockRelease(fileContext->StateLock);
    }

    WdfWaitLockRelease(SEInterface->SEEventsLock);

    TraceLoggingWrite(
        g_hNfcCxProvider,
        "NfcCxSEInterfaceHandleEvent",
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
        TraceLoggingValue((DWORD)EventType, "EventType"));

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxSEInterfacePurgeHCERecvQueue(
    _In_ PNFCCX_SE_INTERFACE SEInterface
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(SEInterface->SEManagerLock, NULL);

    if (SEInterface->SEManager != NULL) {

        WdfWaitLockAcquire(SEInterface->SEManager->StateLock, NULL);

        //
        // Drain the received packet queue
        //
        while (!IsListEmpty(&SEInterface->SEManager->RoleParameters.SEManager.PacketQueue)) {

            PLIST_ENTRY ple = RemoveHeadList(&SEInterface->SEManager->RoleParameters.SEManager.PacketQueue);
            SEInterface->SEManager->RoleParameters.SEManager.PacketQueueLength--;
            delete CNFCPayload::FromListEntry(ple);
        }

        WdfWaitLockRelease(SEInterface->SEManager->StateLock);
    }

    WdfWaitLockRelease(SEInterface->SEManagerLock);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxSEInterfaceHandleHCEPacket(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ USHORT ConnectionId,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ USHORT DataLength
    )
{
    PNFCCX_FILE_CONTEXT fileContext = NULL;
    CNFCPayload* queuedPacket = NULL;
    PSECURE_ELEMENT_HCE_DATA_PACKET dataPacket = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(SEInterface->SEManagerLock, NULL);

    if (NULL == SEInterface->SEManager) {
        TRACE_LINE(LEVEL_ERROR, "No SEManager client present");
        goto Done;
    }

    fileContext = SEInterface->SEManager;

    WdfWaitLockAcquire(fileContext->StateLock, NULL);

    queuedPacket = new CNFCPayload();
    if (NULL == queuedPacket ||
        !NT_SUCCESS(queuedPacket->Initialize(NULL, sizeof(DWORD) + DataLength))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources, dropping Packet");
        WdfWaitLockRelease(fileContext->StateLock);
        goto Done;
    }

    dataPacket = (PSECURE_ELEMENT_HCE_DATA_PACKET)queuedPacket->GetPayload();
    dataPacket->bConnectionId = ConnectionId;
    dataPacket->cbPayload = DataLength;
    RtlCopyMemory(dataPacket->pbPayload, Data, DataLength);

    if (!NfcCxSEInterfaceCompleteRequestLocked(fileContext->RoleParameters.SEManager.PacketRequestQueue,
                                               queuedPacket->GetPayload(),
                                               queuedPacket->GetSize())) {
        TRACE_LINE(LEVEL_WARNING, "No request to complete, enqueueing packet");

        if (NFCCX_MAX_HCE_PACKET_QUEUE_LENGTH <= fileContext->RoleParameters.SEManager.PacketQueueLength) {
            TRACE_LINE(LEVEL_ERROR, "Too many queued, dropping packet");
            WdfWaitLockRelease(fileContext->StateLock);
            delete queuedPacket;
            goto Done;
        }

        InsertTailList(&fileContext->RoleParameters.SEManager.PacketQueue, 
                       queuedPacket->GetListEntry());
        fileContext->RoleParameters.SEManager.PacketQueueLength++;
       
    }
    else {
        delete queuedPacket;
    }

    WdfWaitLockRelease(fileContext->StateLock);

Done:
    WdfWaitLockRelease(SEInterface->SEManagerLock);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxSEInterfaceDispatchEnumEndpoints(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SE Enum Endpoint IOCTL
    from a power managed queue

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
    PNFCCX_RF_INTERFACE rfInterface = NULL;
    SECURE_ELEMENT_ENDPOINT_INFO endpointInfo[MAX_NUMBER_OF_SE];
    PSECURE_ELEMENT_ENDPOINT_LIST endpointList = NULL;
    phLibNfc_SE_List_t SEList[MAX_NUMBER_OF_SE] = {0};
    uint8_t SECount = 0;
    DWORD cbOutputBuffer = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);

    //
    // Since the output buffer has already been validated
    // for output buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    rfInterface = NfcCxFileObjectGetFdoContext(FileContext)->RFInterface;
    endpointList = (PSECURE_ELEMENT_ENDPOINT_LIST)OutputBuffer;

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer must be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxRFInterfaceGetSecureElementList(rfInterface, SEList, &SECount, TRUE);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_WARNING, "Failed NfcCxRFInterfaceGetSecureElementList, %!STATUS!", status);
        goto Done;
    }

    endpointList->NumberOfEndpoints = 0;
    cbOutputBuffer = sizeof(DWORD);

    for (uint8_t i=0; (i < SECount); i++) {
        RtlZeroMemory(&endpointInfo[i], sizeof(SECURE_ELEMENT_ENDPOINT_INFO));
        endpointInfo[i].guidSecureElementId = NfcCxSEInterfaceGetSecureElementId(rfInterface, SEList[i].hSecureElement);
        endpointInfo[i].eSecureElementType = NfcCxSEInterfaceGetSecureElementType(SEList[i].eSE_Type);
        endpointList->NumberOfEndpoints++;
    }

    status = STATUS_BUFFER_OVERFLOW;

    if ((cbOutputBuffer + (endpointList->NumberOfEndpoints * sizeof(SECURE_ELEMENT_ENDPOINT_INFO))) <= OutputBufferLength) {
        RtlCopyMemory(endpointList->EndpointList, endpointInfo, endpointList->NumberOfEndpoints * sizeof(SECURE_ELEMENT_ENDPOINT_INFO));
        cbOutputBuffer += (endpointList->NumberOfEndpoints * sizeof(SECURE_ELEMENT_ENDPOINT_INFO));
        status = STATUS_SUCCESS;
    }

    WdfRequestCompleteWithInformation(Request, status, cbOutputBuffer);

    //
    // Now that the request is completed it return STATUS_PENDING 
    // so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);
    
    return status;
}

NTSTATUS
NfcCxSEInterfaceDispatchGetNextEvent(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SE Get Next Event IOCTL

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

    if (!NfcCxFileObjectIsSEEvent(FileContext)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid role for request");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer must be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(FileContext->StateLock, NULL);

    //
    // Make sure the unresponsive client detection timer is stopped
    //
    NfcCxFileObjectStopUnresponsiveClientDetectionTimer(FileContext, FALSE);
    NfcCxFileObjectResetUnresponsiveClientDetection(FileContext);

    //
    // If there are any queued events, dispatch and complete it now
    //
    if (!IsListEmpty(&FileContext->RoleParameters.SEEvent.EventQueue)) {

        //
        // Get the head of the list and complete the request
        //
        PLIST_ENTRY ple = FileContext->RoleParameters.SEEvent.EventQueue.Flink;
        ULONG usedBufferSize = 0;
        CNFCPayload * pBuffer = CNFCPayload::FromListEntry(ple);

        //
        // Complete the request
        //
        status = NfcCxSEInterfaceCopyEventData(OutputBuffer,
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
            FileContext->RoleParameters.SEEvent.EventQueueLength--;
            delete pBuffer;
        }

        WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    } else {

        WDFREQUEST eventRequest = NULL;
        //
        // Ensure that there are no other requests in the queue.
        //
        status = WdfIoQueueFindRequest(FileContext->RoleParameters.SEEvent.EventRequestQueue,
                                       NULL,
                                       FileContext->FileObject,
                                       NULL,
                                       &eventRequest);
        if (eventRequest != NULL) {
            TRACE_LINE(LEVEL_ERROR, "A GetNextSubscribedMessage request is already pending in the queue");
            WdfObjectDereference(eventRequest);
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
                                            FileContext->RoleParameters.SEEvent.EventRequestQueue);
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

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceDispatchSubscribeForEvent(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SE Subscribe For Event IOCTL

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
    PSECURE_ELEMENT_EVENT_SUBSCRIPTION_INFO eventInfo = NULL;
    phLibNfc_Handle hSecureElement = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(sizeof(SECURE_ELEMENT_EVENT_SUBSCRIPTION_INFO) <= InputBufferLength);

    eventInfo = (PSECURE_ELEMENT_EVENT_SUBSCRIPTION_INFO)InputBuffer;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer must be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (!NfcCxSEInterfaceValidateEventType(eventInfo->eEventType)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SE event type %d", eventInfo->eEventType);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (!IsEqualGUID(eventInfo->guidSecureElementId, GUID_NULL) &&
        !NfcCxSEInterfaceGetSecureElementHandle(NfcCxFileObjectGetFdoContext(FileContext)->RFInterface,
                                                eventInfo->guidSecureElementId,
                                                &hSecureElement)) {
        TRACE_LINE(LEVEL_ERROR,
                   "Invalid SE identifier %!GUID!", &eventInfo->guidSecureElementId);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO,
        "SubscribeForEvent for client role %!FILE_OBJECT_ROLE! Id=%!GUID!(%p) EventType=%!SECURE_ELEMENT_EVENT_TYPE!",  
        FileContext->Role,
        &eventInfo->guidSecureElementId,
        hSecureElement,
        eventInfo->eEventType);

    //
    // Verify and subscribe for event
    //
    status = NfcCxFileObjectValidateAndSubscribeForEvent(FileContext, 
                                                         eventInfo->guidSecureElementId, 
                                                         eventInfo->eEventType);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to set the payload, %!STATUS!", status);
        goto Done;
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceDispatchSetCardEmulationMode(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SE Set CardEmulation Mode IOCTL from
    a power managed queue

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
    PNFCCX_RF_INTERFACE rfInterface = NULL;
    PSECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO pMode = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    //
    // Since the input buffer has already been validated
    // for input buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(SECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO) <= InputBufferLength);

    rfInterface = NfcCxFileObjectGetFdoContext(FileContext)->RFInterface;
    pMode = (PSECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO)InputBuffer;

    if (!NfcCxFileObjectIsSEManager(FileContext)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid role for request");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer must be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (!NfcCxSEInterfaceValidateEmulationMode(pMode->eMode)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SE emulation mode %d", pMode->eMode);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "SecureElement Id=%!GUID! Mode=%!SECURE_ELEMENT_CARD_EMULATION_MODE!", &pMode->guidSecureElementId, pMode->eMode);

    status = NfcCxSEInterfaceSetCardEmulationMode(FileContext, pMode);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to set card emulation mode, %!STATUS!", status);
        TRACE_LOG_NTSTATUS_ON_FAILURE(status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceDispatchGetNfccCapabilities(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine returns information about the device capabilities
    retreived from the CORE_INIT_RESP message

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
    PNFCCX_RF_INTERFACE rfInterface = NULL;
    PSECURE_ELEMENT_NFCC_CAPABILITIES pCapabilities = (PSECURE_ELEMENT_NFCC_CAPABILITIES)OutputBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // Since the output buffer has already been validated
    // for output buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(SECURE_ELEMENT_NFCC_CAPABILITIES) <= OutputBufferLength);

    rfInterface = NfcCxFileObjectGetFdoContext(FileContext)->RFInterface;

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer must be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxRFInterfaceGetNfccCapabilities(rfInterface,
                                                 pCapabilities);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to get NFCC capabilities, %!STATUS!", status);
        goto Done;
    }

    WdfRequestCompleteWithInformation(Request, status, sizeof(SECURE_ELEMENT_NFCC_CAPABILITIES));

    //
    // Now that the request is completed it returns STATUS_PENDING 
    // so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    TraceLoggingWrite(
        g_hNfcCxProvider,
        "NfcCxSEInterfaceDispatchGetNfccCapabilities",
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
        TraceLoggingValue(pCapabilities->cbMaxRoutingTableSize, "cbMaxRoutingTableSize"),
        TraceLoggingValue(pCapabilities->IsAidRoutingSupported, "IsAidRoutingSupported"),
        TraceLoggingValue(pCapabilities->IsProtocolRoutingSupported, "IsProtocolRoutingSupported"),
        TraceLoggingValue(pCapabilities->IsTechRoutingSupported, "IsTechRoutingSupported"));

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);
    
    return status;
}

NTSTATUS
NfcCxSEInterfaceDispatchGetRoutingTable(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine returns the current  listen mode routing table

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
    PNFCCX_RF_INTERFACE rfInterface = NULL;
    PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable = (PSECURE_ELEMENT_ROUTING_TABLE)OutputBuffer;
    uint32_t uiRoutingTableSize = (uint32_t)OutputBufferLength;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer must be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    rfInterface = NfcCxFileObjectGetFdoContext(FileContext)->RFInterface;

    status = NfcCxSEInterfaceGetRoutingTable(rfInterface, pRoutingTable, uiRoutingTableSize);
    WdfRequestCompleteWithInformation(Request, status, uiRoutingTableSize);

    //
    // Now that the request is completed it returns STATUS_PENDING 
    // so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceDispatchSetRoutingTable(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the Set routing table ioctl

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
    PNFCCX_RF_INTERFACE rfInterface = NULL;
    PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable = (PSECURE_ELEMENT_ROUTING_TABLE)InputBuffer;
    phLibNfc_RtngConfig_t pRtngTable[MAX_ROUTING_TABLE_SIZE];
    DWORD expectedBufferLength = pRoutingTable->NumberOfEntries * sizeof(SECURE_ELEMENT_ROUTING_TABLE_ENTRY) +
                                 sizeof(pRoutingTable->NumberOfEntries);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    rfInterface = NfcCxFileObjectGetFdoContext(FileContext)->RFInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Ouput buffer must be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (expectedBufferLength > InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Expected input buffer length (%lu) is greater than input buffer length (%Iu)",
                   expectedBufferLength,
                   InputBufferLength);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxSEInterfaceValidateRoutingTable(rfInterface, pRoutingTable);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "NfcCxSEInterfaceValidateRoutingTable Failed with status, %!STATUS!", status);
        goto Done;
    }

    status = NfcCxSEInterfaceConvertRoutingTable(rfInterface, pRoutingTable, pRtngTable);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "NfcCxSEInterfaceConvertRoutingTable Failed with status, %!STATUS!", status);
        goto Done;
    }

    status = NfcCxRFInterfaceSetRoutingTable(rfInterface, pRtngTable, (uint8_t)pRoutingTable->NumberOfEntries);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "NfcCxRFInterfaceSetRoutingTable Failed with status, %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);
    
    return status;
}

NTSTATUS
NfcCxSEInterfaceDispatchHCERemoteRecv(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the HCE remote receive IOCTL

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
    PNFCCX_RF_INTERFACE rfInterface = NfcCxFileObjectGetFdoContext(FileContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);

    WdfWaitLockAcquire(FileContext->StateLock, NULL);

    if (!NfcCxRFInterfaceIsHCESupported(rfInterface)) {
        TRACE_LINE(LEVEL_WARNING, "HCE is not enabled or not supported");
        status = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    if (!NfcCxFileObjectIsSEManager(FileContext)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid role for request");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer must be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    //
    // If there are any queued events, dispatch and complete it now
    //
    if (!IsListEmpty(&FileContext->RoleParameters.SEManager.PacketQueue)) {
        //
        // Get the head of the list and complete the request
        //
        PLIST_ENTRY ple = FileContext->RoleParameters.SEManager.PacketQueue.Flink;
        ULONG usedBufferSize = 0;
        CNFCPayload* pBuffer = CNFCPayload::FromListEntry(ple);

        //
        // Complete the request
        //
        status = NfcCxSEInterfaceCopyEventData(OutputBuffer,
                                               (ULONG)OutputBufferLength,
                                               pBuffer->GetPayload(),
                                               pBuffer->GetSize(),
                                               &usedBufferSize);
        if (STATUS_BUFFER_OVERFLOW != status &&
            !NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to copy the subscription data, %!STATUS!", status);
            goto Done;
        } else if (NT_SUCCESS(status)) {
            RemoveEntryList(ple);
            FileContext->RoleParameters.SEManager.PacketQueueLength--;
            delete pBuffer;
        }

        //
        // Complete the request
        //
        WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    } else {

        WDFREQUEST packetRequest = NULL;
        //
        // Ensure that there are no other requests in the queue.
        //
        status = WdfIoQueueFindRequest(FileContext->RoleParameters.SEManager.PacketRequestQueue,
                                       NULL,
                                       FileContext->FileObject,
                                       NULL,
                                       &packetRequest);
        if (packetRequest != NULL) {
            TRACE_LINE(LEVEL_ERROR, "An HCERecv request is already pending in the queue");
            WdfObjectDereference(packetRequest);
            status = STATUS_INVALID_DEVICE_STATE;
            goto Done;
        }
        else {
            NT_ASSERT(STATUS_NO_MORE_ENTRIES == status);
        }
        
        //
        // Else, forward the request to the holding queue
        //
        status = WdfRequestForwardToIoQueue(Request, 
                                            FileContext->RoleParameters.SEManager.PacketRequestQueue);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to forward the request to the PacketRequestQueue, %!STATUS!", status);
            goto Done;
        }
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

Done:
    WdfWaitLockRelease(FileContext->StateLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

NTSTATUS
NfcCxSEInterfaceDispatchHCERemoteSend(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the HCE remote send ioctl

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
    PNFCCX_RF_INTERFACE rfInterface = NfcCxFileObjectGetFdoContext(FileContext)->RFInterface;
    PSECURE_ELEMENT_HCE_DATA_PACKET pDataPacket = (PSECURE_ELEMENT_HCE_DATA_PACKET)InputBuffer;
    DWORD cbExpectedInputSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    if (!NfcCxRFInterfaceIsHCESupported(rfInterface)) {
        TRACE_LINE(LEVEL_WARNING, "HCE is not enabled or not supported");
        status = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    if (!NfcCxFileObjectIsSEManager(FileContext)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid role for request");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer must be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (0 == pDataPacket->cbPayload) {
        TRACE_LINE(LEVEL_ERROR, "Payload size cannot be 0");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    cbExpectedInputSize = SECURE_ELEMENT_HCE_DATA_PACKET_HEADER + pDataPacket->cbPayload;
    if (cbExpectedInputSize != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Invalid input buffer size. Expected: %lu, Actual: %Iu", cbExpectedInputSize, InputBufferLength);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxRFInterfaceTargetSend(rfInterface, (PBYTE)pDataPacket->pbPayload, pDataPacket->cbPayload);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

NTSTATUS
NfcCxSEInterfaceCopyEventData(
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ ULONG DataLength,
    _Out_ PULONG BufferUsed
    )
/*++

Routine Description:

   Copies the event data into the event output buffer.

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
        TRACE_LINE(LEVEL_ERROR, "Insufficient buffer size, provided %lu, required %lu", OutputBufferLength, requiredBufferSize);
        goto Done;
    }

    CopyMemory(((PUCHAR)OutputBuffer) + sizeof(ULONG), Data, DataLength);
    *BufferUsed = requiredBufferSize;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

BOOLEAN
NfcCxSEInterfaceMatchesEvent(
    _In_ CNFCPayload *EventPayload,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    PSECURE_ELEMENT_EVENT_INFO eventInfo = (PSECURE_ELEMENT_EVENT_INFO)EventPayload->GetPayload();

    NT_ASSERT(NfcCxFileObjectIsSEEvent(FileContext));
    NT_ASSERT(EventPayload->GetSize() >= SECURE_ELEMENT_EVENT_INFO_HEADER);

    if (EventPayload->GetSize() >= SECURE_ELEMENT_EVENT_INFO_HEADER) {
        return ((IsEqualGUID(FileContext->RoleParameters.SEEvent.SEIdentifier, eventInfo->guidSecureElementId) ||
                 IsEqualGUID(FileContext->RoleParameters.SEEvent.SEIdentifier, GUID_NULL)) &&
                (eventInfo->eEventType == FileContext->RoleParameters.SEEvent.eSEEventType));
    }

    return FALSE;
}

#define EVT_TRANSACTION_AID     (0x81U)
#define EVT_TRANSACTION_PARAM   (0x82U)

NTSTATUS
NfcCxSEInterfaceGetEventPayload(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_SE_List_t *pSEInfo,
    _In_ SECURE_ELEMENT_EVENT_TYPE EventType,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo,
    _Outptr_ CNFCPayload **EventPayload
    )
/*++

Routine Description:

    This routine retrieves the secure element event payload

Arguments:

    RFInterface - A pointer to the RFInterface
    hSecureElement - The handle to the secure element object
    EventType - The secure element event type
    pSeEvtInfo - The optional data for the event
    EventPayload - The event payload as expected by the client

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    CNFCPayload *payload = NULL;
    PSECURE_ELEMENT_EVENT_INFO eventInfo = NULL;
    PSECURE_ELEMENT_HCE_ACTIVATION_PAYLOAD hceActivationPayload = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *EventPayload = NULL;

    if (ExternalReaderArrival == EventType ||
        ExternalReaderDeparture == EventType) {
        //
        // External Reader Arrival and Departure events maps to these events
        // Both the events have empty event data
        //
        payload = new CNFCPayload();

        if (NULL == payload ||
            !NT_SUCCESS(payload->Initialize(NULL, SECURE_ELEMENT_EVENT_INFO_HEADER))) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        eventInfo = (PSECURE_ELEMENT_EVENT_INFO)payload->GetPayload();
        eventInfo->guidSecureElementId = NfcCxSEInterfaceGetSecureElementId(pSEInfo);
        eventInfo->eEventType = EventType;
        eventInfo->cbEventData = 0;

    } else if (Transaction == EventType) {
        //
        // The Transaction event from the UICC maps to this event
        // The Event Data for Transaction event contains the AID and PARAM fields
        // The AID and PARAM fields are in BER-TLV format as per ETSI
        //
        if (pSeEvtInfo->UiccEvtInfo.aid.length > UCHAR_MAX ||
            pSeEvtInfo->UiccEvtInfo.param.length > UCHAR_MAX) {
            NT_ASSERTMSG("The EVT_TRANSACTION should never be larger than UCHAR_MAX", FALSE);
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }

        payload = new CNFCPayload();

        if (NULL == payload ||
            !NT_SUCCESS(payload->Initialize(NULL,
                                            SECURE_ELEMENT_EVENT_INFO_HEADER +
                                            (pSeEvtInfo->UiccEvtInfo.aid.length != 0 ? pSeEvtInfo->UiccEvtInfo.aid.length+2 : 0) +
                                            (pSeEvtInfo->UiccEvtInfo.param.length != 0 ? pSeEvtInfo->UiccEvtInfo.param.length+2 : 0)))) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        eventInfo = (PSECURE_ELEMENT_EVENT_INFO)payload->GetPayload();
        eventInfo->guidSecureElementId = NfcCxSEInterfaceGetSecureElementId(pSEInfo);
        eventInfo->eEventType = Transaction;
        eventInfo->cbEventData = 0;

        if (pSeEvtInfo->UiccEvtInfo.aid.length != 0) {
            eventInfo->pbEventData[eventInfo->cbEventData++] = EVT_TRANSACTION_AID;
            eventInfo->pbEventData[eventInfo->cbEventData++] = (uint8_t) pSeEvtInfo->UiccEvtInfo.aid.length;
            RtlCopyMemory(&eventInfo->pbEventData[eventInfo->cbEventData], pSeEvtInfo->UiccEvtInfo.aid.buffer, pSeEvtInfo->UiccEvtInfo.aid.length);
            eventInfo->cbEventData += pSeEvtInfo->UiccEvtInfo.aid.length;
        }

        if (pSeEvtInfo->UiccEvtInfo.param.length != 0) {
            eventInfo->pbEventData[eventInfo->cbEventData++] = EVT_TRANSACTION_PARAM;
            eventInfo->pbEventData[eventInfo->cbEventData++] = (uint8_t) pSeEvtInfo->UiccEvtInfo.param.length;
            RtlCopyMemory(&eventInfo->pbEventData[eventInfo->cbEventData], pSeEvtInfo->UiccEvtInfo.param.buffer, pSeEvtInfo->UiccEvtInfo.param.length);
            eventInfo->cbEventData += pSeEvtInfo->UiccEvtInfo.param.length;
        }
    } else if ((HceActivated == EventType) ||
               (HceDeactivated == EventType)) {

        payload = new CNFCPayload();

        if (NULL == payload ||
            !NT_SUCCESS(payload->Initialize(NULL,
                                            SECURE_ELEMENT_EVENT_INFO_HEADER +
                                            sizeof(SECURE_ELEMENT_HCE_ACTIVATION_PAYLOAD)))) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }
    
        eventInfo = (PSECURE_ELEMENT_EVENT_INFO)payload->GetPayload();
        eventInfo->guidSecureElementId = NfcCxSEInterfaceGetSecureElementId(pSEInfo);
        eventInfo->eEventType = EventType;
        eventInfo->cbEventData = sizeof(SECURE_ELEMENT_HCE_ACTIVATION_PAYLOAD);

        hceActivationPayload = (PSECURE_ELEMENT_HCE_ACTIVATION_PAYLOAD)&eventInfo->pbEventData;
        hceActivationPayload->bConnectionId = RFInterface->pLibNfcContext->uHCEConnectionId;
        hceActivationPayload->eRfTechType = NFC_RF_TECHNOLOGY_A;
        hceActivationPayload->eRfProtocolType = PROTOCOL_ISO_DEP;

    } else {

        TRACE_LINE(LEVEL_WARNING, "No mapping for SE Event Type=%d", EventType);
        status = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    *EventPayload = payload;
    payload = NULL;

Done:

    if (payload != NULL) {
        delete payload;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

BOOLEAN
NfcCxSEInterfaceCompleteRequestLocked(
    _In_ WDFQUEUE EventQueue,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ ULONG DataLength
    )
/*++

Routine Description:

    This routine attempts to complete a request from the provided queue
    with the provided data.

Arguments:

    EventQueue - The event queue to attempt to complete a request from
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
    // Get the next pended IOCTL_SE_GET_NEXT_EVENT request to see
    // if any of them matches the event
    //
    status = WdfIoQueueRetrieveNextRequest(EventQueue, &wdfRequest);
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
    status = NfcCxSEInterfaceCopyEventData(pOutBuffer,
                                           (ULONG)sizeOutBuffer,
                                           Data,
                                           DataLength,
                                           &actualSize);

    TRACE_LINE(LEVEL_INFO,
        "Completing request %p, with %!STATUS!, bytes returned %lu, required size %lu", wdfRequest, status, actualSize, *(DWORD*)pOutBuffer);

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

GUID
NfcCxSEInterfaceGetSecureElementId(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement
    )
{
    phLibNfc_SE_List_t SEList[MAX_NUMBER_OF_SE] = {0};
    uint8_t SECount = 0;
    GUID secureElementId;

    RtlZeroMemory(&secureElementId, sizeof(GUID));

    if (!NT_SUCCESS(NfcCxRFInterfaceGetSecureElementList(RFInterface, SEList, &SECount))) {
        TRACE_LINE(LEVEL_WARNING, "Failed NfcCxRFInterfaceGetSecureElementList");
        goto Done;
    }

    for (uint8_t i=0; i < SECount; i++) {
        if (SEList[i].hSecureElement != hSecureElement) {
            continue;
        }

        switch (SEList[i].eSE_Type) {
        case phLibNfc_SE_Type_UICC:
            RtlCopyMemory(&secureElementId, &UICC_SECUREELEMENT_ID, sizeof(GUID));
            break;
        case phLibNfc_SE_Type_eSE:
            RtlCopyMemory(&secureElementId, &ESE_SECUREELEMENT_ID, sizeof(GUID));
            break;
        case phLibNfc_SE_Type_DeviceHost:
            RtlCopyMemory(&secureElementId, &DH_SECUREELEMENT_ID, sizeof(GUID));
            break;
        default:
            TRACE_LINE(LEVEL_ERROR, "Unknown or Unsupported SE Type = %d", SEList[i].eSE_Type);
            break;
        }
        break;
    }

Done:
    return secureElementId;
}

BOOLEAN
NfcCxSEInterfaceGetSecureElementHandle(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ const GUID& guidSecureElementId,
    _Outptr_result_maybenull_ phLibNfc_Handle *phSecureElement
    )
{
    phLibNfc_SE_List_t SEInfo = {0};

    if (!NT_SUCCESS(NfcCxSEInterfaceGetSEInfo(RFInterface,
                                              guidSecureElementId,
                                              &SEInfo))) {
        *phSecureElement = NULL;
        return FALSE;
    }

    *phSecureElement = SEInfo.hSecureElement;
    return TRUE;
}


NTSTATUS
NfcCxSEInterfaceValidateCardEmulationState(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PSECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO pMode,
    _Inout_ uint8_t& RtngTableCount,
    _Out_ phLibNfc_RtngConfig_t* pRtngTable,
    _Out_ DWORD *pdwFlags
    )
/*++

Routine Description:

    This internal helper routine is validate the card emulation state
    of the secure element

Arguments:

    RFInterface - The pointer to the RFInterface context
    hSecureElement - The handle to the secure element
    eActivationMode - The card emulation mode to set for the secure element
    pRtngTable - The pointer to the routing table
    RtngTableCount - The size of the routing table
    pdwFlags - The flags to determine if emulation mode and routing table needs to be updated

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    phNfc_PowerState_t ePowerState = {0};
    BOOLEAN bIsRoutingTableChanged = FALSE;
    phLibNfc_SE_List_t SEInfo = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *pdwFlags = 0;

    status = NfcCxSEInterfaceGetSEInfo(RFInterface, pMode->guidSecureElementId, &SEInfo);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_WARNING, "Failed NfcCxSEInterfaceGetSEInfo, %!STATUS!", status);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (!NfcCxSEInterfaceGetPowerState(pMode->eMode,
                                       RFInterface->pLibNfcContext->sStackCapabilities.psDevCapabilities,
                                       &ePowerState)) {
        TRACE_LINE(LEVEL_ERROR, "NfcCxSEInterfaceGetPowerState failed");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxSEInterfaceVerifyRoutingTablePower(RFInterface,
                                                     SEInfo.hSecureElement,
                                                     ePowerState,
                                                     RtngTableCount,
                                                     pRtngTable,
                                                     &bIsRoutingTableChanged);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_WARNING, "Failed NfcCxRFInterfaceGetRoutingTable, %!STATUS!", status);
        goto Done;
    }

    *pdwFlags |= (bIsRoutingTableChanged) ? NFCCX_SE_FLAG_SET_ROUTING_TABLE : 0;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

static bool
NfcCxSEIsCardEmulationEnabled(SECURE_ELEMENT_CARD_EMULATION_MODE EmulationMode)
{
    return EmulationMode != EmulationOff;
}

static phLibNfc_eSE_ActivationMode
NfcCxSECalculateActivationMode(
    SECURE_ELEMENT_CARD_EMULATION_MODE EmulationMode,
    BOOLEAN WiredMode
    )
{
    return WiredMode || NfcCxSEIsCardEmulationEnabled(EmulationMode) ?
        phLibNfc_SE_ActModeOn :
        phLibNfc_SE_ActModeOff;
}

NTSTATUS
NfcCxSEInterfaceSetCardEmulationMode(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PSECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO pMode
    )
/*++

Routine Description:

    This internal helper routine is used to set the card emulation mode.

Arguments:

    FileContext - The file context for the SEManager
    hSecureElement - The handle to the secure element
    pPowerState - The power state to set for the secure element

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext = NULL;
    PNFCCX_RF_INTERFACE rfInterface = NULL;
    PNFCCX_SE_INTERFACE seInterface = NULL;
    phLibNfc_RtngConfig_t RtngConfig[MAX_ROUTING_TABLE_SIZE] = {0};
    uint8_t RtngConfigCount = MAX_ROUTING_TABLE_SIZE;
    phLibNfc_Handle hSecureElement;
    DWORD dwFlags = 0;
    bool sePowerSettingsLockHeld = false;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    fdoContext = NfcCxFileObjectGetFdoContext(FileContext);
    rfInterface = fdoContext->RFInterface;
    seInterface = fdoContext->SEInterface;

    //
    // Get the SE handle
    //
    if (!NfcCxSEInterfaceGetSecureElementHandle(rfInterface,
                                                pMode->guidSecureElementId,
                                                &hSecureElement)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid secure element identifier %!GUID!", &pMode->guidSecureElementId);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxSEInterfaceValidateCardEmulationState(rfInterface,
                                                        pMode,
                                                        RtngConfigCount,
                                                        RtngConfig,
                                                        &dwFlags);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid card emulation state, %!STATUS!", status);
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "SE set emulation mode hSecureElement=%p, eEmulationMode=%!SECURE_ELEMENT_CARD_EMULATION_MODE!, dwFlags=0x%x",
                                              hSecureElement,
                                              pMode->eMode,
                                              dwFlags);

    //
    // Update the Routing Table if neccessary.
    // Note: The Routing Table is how the SE's card emulation power settings are set.
    // (e.g. EmulationOnPowerIndependent vs. EmulationOnPowerDependent.)
    //
    if (dwFlags & NFCCX_SE_FLAG_SET_ROUTING_TABLE) {
        status = NfcCxRFInterfaceSetRoutingTable(rfInterface,
                                                    RtngConfig,
                                                    RtngConfigCount);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to set routing table, %!STATUS!", status);
            goto Done;
        }
    }

    bool enableCardEmulation = NfcCxSEIsCardEmulationEnabled(pMode->eMode);

    //
    // Acquire SE Power Settings Lock
    //
    WdfWaitLockAcquire(seInterface->SEPowerSettingsLock, NULL);
    sePowerSettingsLockHeld = true;

    //
    // Get the SE's current power settings
    //
    NFCCX_POWER_SETTING* sePower;
    status = NfcCxSEInterfaceGetOrAddSettingsListItem(seInterface, pMode->guidSecureElementId, &sePower);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
        goto Done;
    }

    phLibNfc_eSE_ActivationMode activationMode = NfcCxSECalculateActivationMode(pMode->eMode, sePower->WiredMode);

    //
    // Update SE's activation mode
    //
    status = NfcCxRFInterfaceSetCardActivationMode(rfInterface,
                                                    hSecureElement,
                                                    activationMode);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to set card emulation mode, %!STATUS!", status);
        goto Done;
    }

    //
    // Check if the card emulation mode has switch from an on to off state or vice versa.
    //
    bool previousEnableCardEmulation = NfcCxSEIsCardEmulationEnabled(sePower->EmulationMode);

    //
    // Update SE's power settings.
    //
    sePower->EmulationMode = pMode->eMode;

    //
    // Release SE Power Settings Lock
    //
    WdfWaitLockRelease(seInterface->SEPowerSettingsLock);
    sePowerSettingsLockHeld = false;

    //
    // Ensure RF (NFCC radio) has card emulation enabled when required
    //
    if (previousEnableCardEmulation != enableCardEmulation)
    {
        if (enableCardEmulation)
        {
            status = NfcCxPowerFileAddReference(fdoContext->Power, FileContext, NfcCxPowerReferenceType_CardEmulation);
            if (!NT_SUCCESS(status))
            {
                TRACE_LINE(LEVEL_ERROR, "NfcCxPowerFileAddReference failed, %!STATUS!", status);
                goto Done;
            }
        }
        else
        {
            status = NfcCxPowerFileRemoveReference(fdoContext->Power, FileContext, NfcCxPowerReferenceType_CardEmulation);
            if (!NT_SUCCESS(status))
            {
                TRACE_LINE(LEVEL_ERROR, "NfcCxPowerFileRemoveReference failed, %!STATUS!", status);
                goto Done;
            }
        }
    }

Done:
    if (sePowerSettingsLockHeld)
    {
        WdfWaitLockRelease(seInterface->SEPowerSettingsLock);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxSEInterfaceSetCardWiredMode(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& SecureElementId,
    _In_ BOOLEAN WiredMode
    )
/*++

Routine Description:

    Turns on/off WIRED mode of SE.

Arguments:

    SEInterface - A pointer to the SEInterface.
    SecureElementId - The GUID of the SE.
    WiredMode - TRUE to turn on wired mode. FALSE to turn off WIRED mode.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    bool sePowerSettingsLockHeld = false;
    PNFCCX_RF_INTERFACE rfInterface = SEInterface->FdoContext->RFInterface;

    //
    // Get the SE handle
    //
    phLibNfc_Handle hSecureElement;
    if (!NfcCxSEInterfaceGetSecureElementHandle(
        rfInterface,
        SecureElementId,
        &hSecureElement))
    {
        TRACE_LINE(LEVEL_ERROR, "Invalid secure element identifier %!GUID!", &SecureElementId);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    //
    // Acquire SE Power Settings Lock
    //
    WdfWaitLockAcquire(SEInterface->SEPowerSettingsLock, NULL);
    sePowerSettingsLockHeld = true;

    //
    // Get the SE's current power settings
    //
    NFCCX_POWER_SETTING* sePower;
    status = NfcCxSEInterfaceGetOrAddSettingsListItem(SEInterface, SecureElementId, &sePower);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
        goto Done;
    }

    phLibNfc_eSE_ActivationMode activationMode = NfcCxSECalculateActivationMode(sePower->EmulationMode, WiredMode);

    //
    // Update SE's activation mode
    //
    status = NfcCxRFInterfaceSetCardActivationMode(
        SEInterface->FdoContext->RFInterface,
        hSecureElement,
        activationMode);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to set card emulation mode, %!STATUS!", status);
        goto Done;
    }

    //
    // Update SE's power settings.
    //
    sePower->WiredMode = WiredMode;

Done:
    if (sePowerSettingsLockHeld)
    {
        WdfWaitLockRelease(SEInterface->SEPowerSettingsLock);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceResetCard(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& SecureElementId
    )
/*++

Routine Description:

    Resets an SE by power cycling it (or equivalent)

Arguments:

    SEInterface - A pointer to the SEInterface
    SecureElementId - The GUID of the SE

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Get the SE handle
    //
    phLibNfc_Handle hSecureElement;
    if (!NfcCxSEInterfaceGetSecureElementHandle(
        SEInterface->FdoContext->RFInterface,
        SecureElementId,
        &hSecureElement))
    {
        TRACE_LINE(LEVEL_ERROR, "Invalid secure element identifier %!GUID!", &SecureElementId);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxRFInterfaceResetCard(
        SEInterface->FdoContext->RFInterface,
        hSecureElement);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceGetActivationMode(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement,
    _Out_ phLibNfc_eSE_ActivationMode& eActivationMode
    )
/*++

Routine Description:

    This internal helper routine is used to return the SE CE mode

Arguments:

    hSecureElement - The handle to the secure element
    eActivationMode - The card emulation mode to set for the secure element

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    phLibNfc_SE_List_t SEList[MAX_NUMBER_OF_SE] = {0};
    uint8_t SECount = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = NfcCxRFInterfaceGetSecureElementList(RFInterface,
                                                  SEList,
                                                  &SECount);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_WARNING, "Failed NfcCxRFInterfaceGetSecureElementList, %!STATUS!", status);
        goto Done;
    }

    for (uint8_t i = 0; i < SECount; i++) {
        if (SEList[i].hSecureElement == hSecureElement) {
            eActivationMode = SEList[i].eSE_ActivationMode;
            goto Done;
        }
    }

    TRACE_LINE(LEVEL_WARNING, "Failed to locate SE with handle %p", hSecureElement);
    status = STATUS_INVALID_PARAMETER;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

static BOOLEAN
NfcCxSEValidRoutingTechnology(
    _In_ BYTE Technology
    )
/*++

Routine Description:

    Checks the routing technology to see whether it is valid or not based on the
    NCI 1.0 spec, section 6.3.2.

Arguments:

    Technology - The routing technology to check.

Return Value:

    BOOLEAN - TRUE if the routing technology is valid, FALSE otherwise.

--*/
{
    if ((Technology >= 0x04 && Technology <= 0x7F) || Technology == 0xFF) {
        // RFU
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
NfcCxSEValidRoutingProtocol(
    _In_ BYTE Protocol
    )
/*++

Routine Description:

    Checks the routing protocol to see whether it is valid or not based on the
    NCI 1.0 spec, section 6.3.2.

Arguments:

    Protocol - The routing protocol to check.

Return Value:

    BOOLEAN - TRUE if the routing protocol is valid, FALSE otherwise.

--*/
{
    if ((Protocol >= 0x06 && Protocol <= 0x7F) || Protocol == 0xFF) {
        // RFU
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
NfcCxSEInterfaceValidateRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable
    )
/*++

Routine Description:

    This internal helper routine is used to perform light validation of the
    routing table before it is sent to libnfc. Libnfc does validate other
    NCI requirement as uniqueness of the route protocol, technology, and aid
    route entries. It also validate the ranges for aid lengths, technology,
    and protocol.

    This routine makes sure that there are no nfc-dep routing rule.
    Also, make sure that AID length is OK

Arguments:

    RFInterface - The pointer to RF interface
    pRoutingTable - Route table to be validate

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD cbRoutingTable = NCI_PROTO_ROUTING_ENTRY_SIZE; // implicit NFC-DEP route
    SECURE_ELEMENT_NFCC_CAPABILITIES capabilities = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (pRoutingTable->NumberOfEntries > MAX_ROUTING_TABLE_SIZE) {
        status = STATUS_INVALID_BUFFER_SIZE;
        TRACE_LINE(LEVEL_ERROR, "Exceeded maximum driver routing table size");
        goto Done;
    }

    status = NfcCxRFInterfaceGetNfccCapabilities(RFInterface, &capabilities);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to get NFCC capabilities, %!STATUS!", status);
        goto Done;
    }

    for (DWORD i = 0; i < pRoutingTable->NumberOfEntries; i++) {
        switch (pRoutingTable->TableEntries[i].eRoutingType) {
        case RoutingTypeTech:
            if (!NfcCxSEValidRoutingTechnology(pRoutingTable->TableEntries[i].TechRoutingInfo.eRfTechType)) {
                status = STATUS_INVALID_PARAMETER;
                TRACE_LINE(LEVEL_ERROR, "Invalid technology = %u", pRoutingTable->TableEntries[i].TechRoutingInfo.eRfTechType);
                goto Done;
            }
            cbRoutingTable += NCI_TECH_ROUTING_ENTRY_SIZE;
            break;
        case RoutingTypeProtocol:
            if (pRoutingTable->TableEntries[i].ProtoRoutingInfo.eRfProtocolType == phNfc_RfProtocolsNfcDepProtocol ||
                !NfcCxSEValidRoutingProtocol(pRoutingTable->TableEntries[i].ProtoRoutingInfo.eRfProtocolType)) {
                status = STATUS_INVALID_PARAMETER;
                TRACE_LINE(LEVEL_ERROR, "Invalid protocol = %u", pRoutingTable->TableEntries[i].ProtoRoutingInfo.eRfProtocolType);
                goto Done;
            }
            cbRoutingTable += NCI_PROTO_ROUTING_ENTRY_SIZE;
            break;
        case RoutingTypeAid:
            if ((pRoutingTable->TableEntries[i].AidRoutingInfo.cbAid < ISO_7816_MINIMUM_AID_LENGTH) ||
                (pRoutingTable->TableEntries[i].AidRoutingInfo.cbAid > ISO_7816_MAXIMUM_AID_LENGTH)) {
                status = STATUS_INVALID_PARAMETER;
                TRACE_LINE(LEVEL_ERROR, "Invalid AID route size = %lu", pRoutingTable->TableEntries[i].AidRoutingInfo.cbAid);
                goto Done;
            }
            cbRoutingTable += NCI_AID_ROUTING_ENTRY_SIZE(pRoutingTable->TableEntries[i].AidRoutingInfo.cbAid);
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }
    }

    if (cbRoutingTable > capabilities.cbMaxRoutingTableSize) {
        status = STATUS_INVALID_BUFFER_SIZE;
        TRACE_LINE(LEVEL_ERROR, "Exceeded maximum NFCC routing table size");
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceConvertRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable,
    _Out_ phLibNfc_RtngConfig_t* pRtngTable
    )
/*++

Routine Description:

    This internal helper routine is used to convert route table format from
    MSFT defined to libnfc defined format

Arguments:

    RFInterface - The pointer to RF interface
    pRoutingTable -  The pointer to SE DDI routing table structure 
    pRtngTable - The LIBNFC route table

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURE_ELEMENT_CARD_EMULATION_MODE eEmulationMode;
    phLibNfc_SE_List_t SEInfo = {0};
    PNFCCX_SE_INTERFACE seInterface = RFInterface->FdoContext->SEInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    _Analysis_assume_(pRoutingTable->NumberOfEntries > 0);

    for (uint8_t i = 0; (i < pRoutingTable->NumberOfEntries) && NT_SUCCESS(status); i++)
    {
        //
        // Get SE's GUID
        //
        GUID secureElementId;
        switch (pRoutingTable->TableEntries[i].eRoutingType)
        {
        case RoutingTypeTech:
            secureElementId = pRoutingTable->TableEntries[i].TechRoutingInfo.guidSecureElementId;
            break;

        case RoutingTypeProtocol:
            secureElementId = pRoutingTable->TableEntries[i].ProtoRoutingInfo.guidSecureElementId;
            break;

        case RoutingTypeAid:
            secureElementId = pRoutingTable->TableEntries[i].AidRoutingInfo.guidSecureElementId;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }

        //
        // Get SE's libNfc info
        //
        status = NfcCxSEInterfaceGetSEInfo(RFInterface, secureElementId, &SEInfo);
        if (!NT_SUCCESS(status))
        {
            goto Done;
        }

        //
        // Get SE's current emulation mode
        //
        WdfWaitLockAcquire(seInterface->SEPowerSettingsLock, NULL);

        NFCCX_POWER_SETTING* sePower = NfcCxSEInterfaceGetSettingsListItem(seInterface, secureElementId);
        eEmulationMode = (sePower != nullptr) ? sePower->EmulationMode : EmulationOff;

        WdfWaitLockRelease(seInterface->SEPowerSettingsLock);

        //
        // Calculate SE's power state
        //
        phNfc_PowerState_t powerState;
        NfcCxSEInterfaceGetPowerState(eEmulationMode, RFInterface->pLibNfcContext->sStackCapabilities.psDevCapabilities, &powerState);

        //
        // Fill out routing table entry
        //
        pRtngTable[i].hSecureElement = SEInfo.hSecureElement;

        switch (pRoutingTable->TableEntries[i].eRoutingType) {
        case RoutingTypeTech:
            pRtngTable[i].Type = phNfc_LstnModeRtngTechBased;
            pRtngTable[i].LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology = (phNfc_eRfTechnologies_t) pRoutingTable->TableEntries[i].TechRoutingInfo.eRfTechType;
            pRtngTable[i].LstnModeRtngValue.tTechBasedRtngValue.tPowerState = powerState;
            break;
        case RoutingTypeProtocol:
            pRtngTable[i].Type = phNfc_LstnModeRtngProtocolBased;
            pRtngTable[i].LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol = (phNfc_eRfProtocols_t) pRoutingTable->TableEntries[i].ProtoRoutingInfo.eRfProtocolType;
            pRtngTable[i].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState = powerState;
            break;
        case RoutingTypeAid:
            pRtngTable[i].Type = phNfc_LstnModeRtngAidBased;
            pRtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.bAidSize = (uint8_t) pRoutingTable->TableEntries[i].AidRoutingInfo.cbAid;
            // No need to validate the length here as it was validated before
            RtlCopyMemory(pRtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.aAid, pRoutingTable->TableEntries[i].AidRoutingInfo.pbAid, pRtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.bAidSize);
            pRtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.tPowerState = powerState;
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceGetSEInfo(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ const GUID& SecureElementId,
    _Out_ phLibNfc_SE_List_t* pSEInfo
    )
/*++

Routine Description:

    This internal helper routine is used retrieve information about the SE
    endpoint

Arguments:

    RFInterface - The pointer to the RF interface
    SecureElementId - The Secure Element Identifier
    pSEInfo - The Secure Element information

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    phLibNfc_SE_List_t SEList[MAX_NUMBER_OF_SE] = {0};
    uint8_t SECount = 0;
    phLibNfc_SE_Type_t eSE_Type;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (IsEqualGUID(SecureElementId, UICC_SECUREELEMENT_ID)) {
        eSE_Type = phLibNfc_SE_Type_UICC;
    }
    else if (IsEqualGUID(SecureElementId, DH_SECUREELEMENT_ID)) {
        eSE_Type = phLibNfc_SE_Type_DeviceHost;
    }
    else if (IsEqualGUID(SecureElementId, ESE_SECUREELEMENT_ID)) {
        eSE_Type = phLibNfc_SE_Type_eSE;
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxRFInterfaceGetSecureElementList(RFInterface,
                                                  SEList,
                                                  &SECount);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_WARNING, "Failed NfcCxRFInterfaceGetSecureElementList");
        goto Done;
    }

    for (uint8_t i = 0; i < SECount; i++) {
        if (eSE_Type == SEList[i].eSE_Type) {
            RtlCopyMemory(pSEInfo, &SEList[i], sizeof(phLibNfc_SE_List_t));
            goto Done;
        }
    }

    status = STATUS_INVALID_PARAMETER;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceGetRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_writes_to_(uiRoutingTableSize, uiRoutingTableSize) PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable,
    _Inout_ uint32_t& uiRoutingTableSize
    )
/*++

Routine Description:

    This routine is called from the SE module to retrieve the current routing table

Arguments:

    RFInterface - The RF Interface
    pRoutingTable - The pointer to receive the routing table
    uiRoutingTableSize - The input and output size of the copied routing table

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    phLibNfc_RtngConfig_t RtngConfig[MAX_ROUTING_TABLE_SIZE] = {0};
    uint8_t RtngConfigCount = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = NfcCxRFInterfaceGetRoutingTable(RFInterface, RtngConfig, ARRAYSIZE(RtngConfig), &RtngConfigCount);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_WARNING, "NfcCxRFInterfaceGetRoutingTable failed with status, %!STATUS!", status);
        goto Done;
    }
    pRoutingTable->NumberOfEntries = RtngConfigCount;

    if (sizeof(pRoutingTable->NumberOfEntries) + pRoutingTable->NumberOfEntries * sizeof(pRoutingTable->TableEntries[0]) > uiRoutingTableSize) {
        status = STATUS_BUFFER_OVERFLOW;
        uiRoutingTableSize = sizeof(pRoutingTable->NumberOfEntries);
        goto Done;
    }

    for (uint8_t i = 0; (i < RtngConfigCount) && NT_SUCCESS(status); i++) {

        phLibNfc_RtngConfig_t& RtngTableEntry = RtngConfig[i];

        switch (RtngTableEntry.Type) {
        case phNfc_LstnModeRtngTechBased:
            pRoutingTable->TableEntries[i].eRoutingType = RoutingTypeTech;
            pRoutingTable->TableEntries[i].TechRoutingInfo.guidSecureElementId = NfcCxSEInterfaceGetSecureElementId(RFInterface, RtngTableEntry.hSecureElement);
            pRoutingTable->TableEntries[i].TechRoutingInfo.eRfTechType = (BYTE)RtngTableEntry.LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology;
            status = IsEqualGUID(pRoutingTable->TableEntries[i].TechRoutingInfo.guidSecureElementId, GUID_NULL) ? STATUS_INTERNAL_ERROR : STATUS_SUCCESS;
            break;
        case phNfc_LstnModeRtngProtocolBased:
            pRoutingTable->TableEntries[i].eRoutingType = RoutingTypeProtocol;
            pRoutingTable->TableEntries[i].ProtoRoutingInfo.guidSecureElementId = NfcCxSEInterfaceGetSecureElementId(RFInterface, RtngTableEntry.hSecureElement);
            pRoutingTable->TableEntries[i].ProtoRoutingInfo.eRfProtocolType = (BYTE)RtngTableEntry.LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol;
            status = IsEqualGUID(pRoutingTable->TableEntries[i].ProtoRoutingInfo.guidSecureElementId, GUID_NULL) ? STATUS_INTERNAL_ERROR : STATUS_SUCCESS;
            break;
        case phNfc_LstnModeRtngAidBased:
            pRoutingTable->TableEntries[i].eRoutingType = RoutingTypeAid;
            pRoutingTable->TableEntries[i].AidRoutingInfo.guidSecureElementId = NfcCxSEInterfaceGetSecureElementId(RFInterface, RtngTableEntry.hSecureElement);
            pRoutingTable->TableEntries[i].AidRoutingInfo.cbAid = RtngTableEntry.LstnModeRtngValue.tAidBasedRtngValue.bAidSize;

            _Analysis_assume_(ISO_7816_MAXIMUM_AID_LENGTH >= RtngTableEntry.LstnModeRtngValue.tAidBasedRtngValue.bAidSize);
            RtlCopyMemory(pRoutingTable->TableEntries[i].AidRoutingInfo.pbAid,
                          RtngTableEntry.LstnModeRtngValue.tAidBasedRtngValue.aAid,
                          RtngTableEntry.LstnModeRtngValue.tAidBasedRtngValue.bAidSize);
            status = IsEqualGUID(pRoutingTable->TableEntries[i].AidRoutingInfo.guidSecureElementId, GUID_NULL) ? STATUS_INTERNAL_ERROR : STATUS_SUCCESS;
            break;
        default:
            status = STATUS_INTERNAL_ERROR;
            goto Done;
        }
    }

    uiRoutingTableSize = sizeof(pRoutingTable->NumberOfEntries) + 
                         pRoutingTable->NumberOfEntries * sizeof(pRoutingTable->TableEntries[0]);
Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSEInterfaceVerifyRoutingTablePower(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement,
    _In_ phNfc_PowerState_t PowerState,
    _Inout_ uint8_t& RtngTableCount,
    _Out_ phLibNfc_RtngConfig_t* pRtngTable,
    _Out_ BOOLEAN* pbIsRoutingTableChanged
    )
/*++

Routine Description:

    This routine is called to validate current routing table power.
    It also returns the updated routing table based on the power if the SE endpoint

Arguments:

    RFInterface - The RF Interface
    hSecureElement - The secure element LIBNFC handle
    PowerState - The requested power state of the NFCEE
    RtngTableCount - The max size of the routing table buffer provided, 
                      also holds the size of the returned table
    pRtngTable - The pointer to the routing table
    pbIsRoutingTableChanged - true if routing table needs to be updated

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *pbIsRoutingTableChanged = FALSE;

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    if (RtngTableCount < RFInterface->pLibNfcContext->RtngTableCount) {
        RtngTableCount = 0;
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }
    RtngTableCount = 0;

    for (USHORT i = 0; i < RFInterface->pLibNfcContext->RtngTableCount; i++) {
        pRtngTable[i] = RFInterface->pLibNfcContext->RtngTable[i];

        if (RFInterface->pLibNfcContext->RtngTable[i].hSecureElement == hSecureElement) {
            switch (RFInterface->pLibNfcContext->RtngTable[i].Type) {
            case phNfc_LstnModeRtngTechBased:
                *pbIsRoutingTableChanged |= !NfcCxSEInterfaceIsPowerStateEqual(RFInterface->pLibNfcContext->RtngTable[i].LstnModeRtngValue.tTechBasedRtngValue.tPowerState, PowerState);
                pRtngTable[i].LstnModeRtngValue.tTechBasedRtngValue.tPowerState = PowerState;
                break;
            case phNfc_LstnModeRtngProtocolBased:
                *pbIsRoutingTableChanged |= !NfcCxSEInterfaceIsPowerStateEqual(RFInterface->pLibNfcContext->RtngTable[i].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState, PowerState);
                pRtngTable[i].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState = PowerState;
                break;
            case phNfc_LstnModeRtngAidBased:
                *pbIsRoutingTableChanged |= !NfcCxSEInterfaceIsPowerStateEqual(RFInterface->pLibNfcContext->RtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.tPowerState, PowerState);
                pRtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.tPowerState = PowerState;
                break;
            default:
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }
        }
    }

    RtngTableCount = RFInterface->pLibNfcContext->RtngTableCount;

Done:
    WdfWaitLockRelease(RFInterface->DeviceLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

_Requires_lock_held_(SEInterface->SEPowerSettingsLock)
NFCCX_POWER_SETTING*
NfcCxSEInterfaceGetSettingsListItem(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& secureElementId
    )
{
    for (DWORD i = 0; i != SEInterface->SEPowerSettingsCount; ++i)
    {
        NFCCX_POWER_SETTING* item = &SEInterface->SEPowerSettings[i];

        if (IsEqualGUID(secureElementId, item->SecureElementId))
        {
            return item;
        }
    }

    return nullptr;
}

_Requires_lock_held_(SEInterface->SEPowerSettingsLock)
NTSTATUS
NfcCxSEInterfaceGetOrAddSettingsListItem(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& SecureElementId,
    _Outptr_ NFCCX_POWER_SETTING** listItem
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NFCCX_POWER_SETTING* sePower = NfcCxSEInterfaceGetSettingsListItem(SEInterface, SecureElementId);

    if (sePower == nullptr)
    {
        if (SEInterface->SEPowerSettingsCount >= ARRAYSIZE(SEInterface->SEPowerSettings))
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TRACE_LINE(LEVEL_ERROR, "SEPowerSettings too small, %!STATUS!", status);
            goto Done;
        }

        sePower = &SEInterface->SEPowerSettings[SEInterface->SEPowerSettingsCount];
        *sePower = { SecureElementId, EmulationOff, FALSE };
        SEInterface->SEPowerSettingsCount++;
    }

    *listItem = sePower;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}
