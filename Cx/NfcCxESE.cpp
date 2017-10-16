/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxESE.cpp

Abstract:

    eSE Interface declaration

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxESE.tmh"

typedef struct _NFCCX_ESE_DISPATCH_ENTRY {
    ULONG IoControlCode;
    BOOLEAN fPowerManaged;
    size_t MinimumInputBufferLength;
    size_t MinimumOutputBufferLength;
    PFN_NFCCX_SC_DISPATCH_HANDLER DispatchHandler;
} NFCCX_ESE_DISPATCH_ENTRY, *PNFCCX_ESE_DISPATCH_ENTRY;

static const DWORD ESeIccType = ICC_TYPE_14443_TYPE_A;
static const CHAR ESeVendorIfd[] = "eSE";
static const DWORD ESeReaderChannelId = SCARD_READER_TYPE_EMBEDDEDSE << 16;

const NFCCX_ESE_DISPATCH_ENTRY
g_ESeDispatch [] = {
    { IOCTL_SMARTCARD_GET_ATTRIBUTE,        TRUE,   sizeof(DWORD),                     sizeof(BYTE),                     NfcCxESEInterfaceDispatchGetAttribute },
    { IOCTL_SMARTCARD_SET_ATTRIBUTE,        FALSE,  sizeof(DWORD),                     0,                                NfcCxESEInterfaceDispatchSetAttribute },
    { IOCTL_SMARTCARD_GET_STATE,            FALSE,  0,                                 sizeof(DWORD),                    NfcCxESEInterfaceDispatchGetState },
    { IOCTL_SMARTCARD_POWER,                FALSE,  sizeof(DWORD),                     0,                                NfcCxESEInterfaceDispatchSetPower },
    { IOCTL_SMARTCARD_SET_PROTOCOL,         FALSE,  sizeof(DWORD),                     sizeof(DWORD),                    NfcCxESEInterfaceDispatchSetProtocol },
    { IOCTL_SMARTCARD_IS_ABSENT,            FALSE,  0,                                 0,                                NfcCxESEInterfaceDispatchIsAbsent },
    { IOCTL_SMARTCARD_IS_PRESENT,           FALSE,  0,                                 0,                                NfcCxESEInterfaceDispatchIsPresent },
    // IOCTL_SMARTCARD_TRANSMIT is expecting input to have at least one byte and IO_REQUEST and output to be SW1 + SW2 + IO_REQUEST
    { IOCTL_SMARTCARD_TRANSMIT,             TRUE,   sizeof(SCARD_IO_REQUEST)+1,        sizeof(SCARD_IO_REQUEST)+2,       NfcCxESEInterfaceDispatchTransmit },
    { IOCTL_SMARTCARD_EJECT,                FALSE,  0,                                 0,                                NfcCxSCCommonDispatchNotSupported },
    { IOCTL_SMARTCARD_GET_LAST_ERROR,       FALSE,  0,                                 sizeof(DWORD),                    NfcCxSCCommonDispatchGetLastError },
    { IOCTL_SMARTCARD_SWALLOW,              FALSE,  0,                                 0,                                NfcCxSCCommonDispatchNotSupported },
    { IOCTL_SMARTCARD_CONFISCATE,           FALSE,  0,                                 0,                                NfcCxSCCommonDispatchNotSupported },
    { IOCTL_SMARTCARD_GET_PERF_CNTR,        FALSE,  0,                                 0,                                NfcCxSCCommonDispatchNotSupported },
};

NTSTATUS
NfcCxESEInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT DeviceContext,
    _Outptr_ PNFCCX_ESE_INTERFACE * ESEInterface
    )
/*++

Routine Description:

    This routine creates and initalizes the eSE SmartCard Reader Interface.

Arguments:

    DeviceContext - A pointer to the FdoContext
    ESEInterface - A pointer to a memory location to receive the allocated SmartCard Reader interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_ESE_INTERFACE eseInterface = NULL;
    WDF_OBJECT_ATTRIBUTES objectAttrib;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    eseInterface = (PNFCCX_ESE_INTERFACE)malloc(sizeof((*eseInterface)));
    if (NULL == eseInterface) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the SC interface");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    RtlZeroMemory(eseInterface, sizeof(*eseInterface));
    eseInterface->FdoContext = DeviceContext;

    //
    // Setup the locks and manual IO queue
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = eseInterface->FdoContext->Device;

    status = WdfWaitLockCreate(&objectAttrib,
                                &eseInterface->SmartCardLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the SmartCard WaitLock, %!STATUS!", status);
        goto Done;
    }

    NfcCxSCPresentAbsentDispatcherInitialize(&eseInterface->PresentDispatcher, /*PowerManaged*/ FALSE);
    NfcCxSCPresentAbsentDispatcherInitialize(&eseInterface->AbsentDispatcher, /*PowerManaged*/ FALSE);

Done:

    if (!NT_SUCCESS(status)) {
        if (NULL != eseInterface) {
            NfcCxESEInterfaceDestroy(eseInterface);
            eseInterface = NULL;
        }
    }

    *ESEInterface = eseInterface;

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxESEInterfaceDestroy(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface
    )
/*++

Routine Description:

    This routine cleans up the eSE SmartCard Reader Interface

Arguments:

    Interface - A pointer to the ESEInterface to cleanup.

Return Value:

    None

--*/
{
    DECLARE_CONST_UNICODE_STRING(nfcESeReaderReference, EMBEDDED_SE_NAMESPACE);

    //
    // Since the lock and queue objects are parented to the device,
    // there are no needs to manually delete them here
    //
    if (ESEInterface->InterfaceCreated) {
        //
        // Disable the SmartCard Reader interface
        //
        WdfDeviceSetDeviceInterfaceState(ESEInterface->FdoContext->Device,
                                         &GUID_DEVINTERFACE_SMARTCARD_READER,
                                         &nfcESeReaderReference,
                                         FALSE);

        TRACE_LINE(LEVEL_VERBOSE, "SmartCard Reader interface disabled");
    }

    free(ESEInterface);
}

NTSTATUS
NfcCxESEInterfaceStart(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface
    )
/*++

Routine Description:

    Start the eSE SmartCard Reader Interface

Arguments:

    ESEInterface - The eSE SmartCard Reader Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext = ESEInterface->FdoContext;
    WDF_DEVICE_INTERFACE_PROPERTY_DATA nfcScReaderData;
    BYTE readerKind = 0;
    DECLARE_CONST_UNICODE_STRING(nfcESeReaderReference, EMBEDDED_SE_NAMESPACE);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Enumerate the avaliable SEs.
    phLibNfc_SE_List_t seList[MAX_NUMBER_OF_SE] = {};
    uint8_t seCount = 0;
    status = NfcCxRFInterfaceGetSecureElementList(ESEInterface->FdoContext->RFInterface, seList, &seCount, TRUE);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to enumerate/get SE list. %!STATUS!", status);
        goto Done;
    }

    // Check if NFC Controller has an eSE
    bool hasESE = false;

    for (uint8_t i = 0; i != seCount; ++i)
    {
        const phLibNfc_SE_List_t& se = seList[i];

        if (se.eSE_Type == phLibNfc_SE_Type_eSE)
        {
            ESEInterface->SecureElementId = NfcCxSEInterfaceGetSecureElementId(&se);
            hasESE = true;
            break;
        }
    }

    TRACE_LINE(LEVEL_VERBOSE, "Found eSE = %!BOOLEAN!", hasESE);

    if (hasESE)
    {
        //
        // Publish the NFC eSE smart card reader interface
        //
        if (!ESEInterface->InterfaceCreated) {
            status = WdfDeviceCreateDeviceInterface(fdoContext->Device,
                                                    &GUID_DEVINTERFACE_SMARTCARD_READER,
                                                    &nfcESeReaderReference);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to create the NFC smart card reader device interface, %!STATUS!", status);
                goto Done;
            }

            WDF_DEVICE_INTERFACE_PROPERTY_DATA_INIT(&nfcScReaderData,
                                                    &GUID_DEVINTERFACE_SMARTCARD_READER,
                                                    &DEVPKEY_Device_ReaderKind);
            nfcScReaderData.ReferenceString = &nfcESeReaderReference;
            readerKind = ABI::Windows::Devices::SmartCards::SmartCardReaderKind_EmbeddedSE;
            status = WdfDeviceAssignInterfaceProperty(fdoContext->Device,
                                                        &nfcScReaderData,
                                                        DEVPROP_TYPE_BYTE,
                                                        sizeof(readerKind),
                                                        &readerKind);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to assign ReaderKind property for the NFC eSE smart card reader device interface, %!STATUS!", status);
                goto Done;
            }

            WDF_DEVICE_INTERFACE_PROPERTY_DATA_INIT(&nfcScReaderData,
                                                    &GUID_DEVINTERFACE_SMARTCARD_READER,
                                                    &DEVPKEY_Device_ReaderSecureElementId);
            nfcScReaderData.ReferenceString = &nfcESeReaderReference;
            status = WdfDeviceAssignInterfaceProperty(fdoContext->Device,
                                                        &nfcScReaderData,
                                                        DEVPROP_TYPE_GUID,
                                                        sizeof(ESEInterface->SecureElementId),
                                                        &ESEInterface->SecureElementId);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to assign ReaderSecureElementId property for the NFC eSE smart card reader device interface, %!STATUS!", status);
                goto Done;
            }

            ESEInterface->InterfaceCreated = TRUE;
        }
    }

    if (ESEInterface->InterfaceCreated)
    {
        // Enable eSE smartcard driver interface
        WdfDeviceSetDeviceInterfaceState(
            fdoContext->Device,
            &GUID_DEVINTERFACE_SMARTCARD_READER,
            &nfcESeReaderReference,
            hasESE);
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
NfcCxESEInterfaceStop(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface
    )
/*++

Routine Description:

    Stop the eSE SmartCard Reader Interface

Arguments:

    ESEInterface - The eSE SmartCard Reader Interface

Return Value:

    NTSTATUS

--*/
{
    DECLARE_CONST_UNICODE_STRING(nfcESeReaderReference, EMBEDDED_SE_NAMESPACE);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (ESEInterface->InterfaceCreated) {
        WdfDeviceSetDeviceInterfaceState(ESEInterface->FdoContext->Device,
                                         &GUID_DEVINTERFACE_SMARTCARD_READER,
                                         &nfcESeReaderReference,
                                         FALSE);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

BOOLEAN
NfcCxESEInterfaceIsIoctlSupported(
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

    for (i=0; i < ARRAYSIZE(g_ESeDispatch); i++) {
        if (g_ESeDispatch[i].IoControlCode == IoControlCode) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
NfcCxESEIsPowerManagedRequest(
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This routine returns true if the provided IOCTL is power managed.

Arguments:

    FdoContext - The FDO Context
    IoControlCode - The IOCTL code to check

Return Value:

    TRUE - The IOCTL is power managed
    FALSE - The IOCTL is not power managed

--*/
{
    ULONG i;

    for (i=0; i < ARRAYSIZE(g_ESeDispatch); i++) {
        if (g_ESeDispatch[i].IoControlCode == IoControlCode) {
            return g_ESeDispatch[i].fPowerManaged;
        }
    }

    return FALSE;
}

NTSTATUS
NfcCxESEInterfaceIoDispatch(
    _In_opt_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++

Routine Description:

    This is the first entry into the ESEInterface.  It validates and dispatches SmartCard request
    as appropriate.

Arguments:

    FileContext - The File context.
    Request - Handle to a framework request object.
    OutputBufferLength - Length of the output buffer associated with the request.
    InputBufferLength - Length of the input buffer associated with the request.
    IoControlCode - IOCTL code.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext = NULL;
    WDFMEMORY outMem = NULL;
    WDFMEMORY inMem = NULL;
    PVOID     inBuffer = NULL;
    PVOID     outBuffer = NULL;
    size_t    sizeInBuffer = 0;
    size_t    sizeOutBuffer = 0;
    BOOLEAN   releasePowerReference = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    fdoContext = NfcCxFileObjectGetFdoContext(FileContext);

    //
    // Take a power reference if the request is power managed
    //
    if (NfcCxESEIsPowerManagedRequest(IoControlCode)) {
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
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to retrieve the output buffer, %!STATUS!", status);
            goto Done;
        }

        outBuffer = WdfMemoryGetBuffer(outMem, &sizeOutBuffer);
        NT_ASSERT(sizeOutBuffer == OutputBufferLength);
        NT_ASSERT(NULL != outBuffer);
    }

    if (0 != InputBufferLength) {
        status = WdfRequestRetrieveInputMemory(Request, &inMem);
        if (!NT_SUCCESS(status)) {
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
    status = NfcCxESEInterfaceValidateRequest(IoControlCode,
                                             InputBufferLength,
                                             OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Request validation failed, %!STATUS!", status);
        goto Done;
    }

    //
    // Dispatch the request
    //
    status = NfcCxESEInterfaceDispatchRequest(FileContext,
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
NfcCxESEInterfaceValidateRequest(
    _In_ ULONG        IoControlCode,
    _In_ size_t       InputBufferLength,
    _In_ size_t       OutputBufferLength
    )
/*++

Routine Description:

    This routine validates the SmartCard request.

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

    for (i = 0; i < ARRAYSIZE(g_ESeDispatch); i++) {
        if (g_ESeDispatch[i].IoControlCode == IoControlCode) {

            if (g_ESeDispatch[i].MinimumInputBufferLength > InputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Input buffer.  Expected %I64x, got %I64x",
                    g_ESeDispatch[i].MinimumInputBufferLength,
                    InputBufferLength);
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            if (g_ESeDispatch[i].MinimumOutputBufferLength > OutputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Output buffer.  Expected %I64x, got %I64x",
                    g_ESeDispatch[i].MinimumOutputBufferLength,
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
NfcCxESEInterfaceDispatchRequest(
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

    This routine dispatches the SmartCard request to the appropriate handler.

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
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    USHORT i = 0;
    WDFDEVICE device = WdfFileObjectGetDevice(FileContext->FileObject);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (i = 0; i < ARRAYSIZE(g_ESeDispatch); i++) {
        if (g_ESeDispatch[i].IoControlCode == IoControlCode) {

            status = g_ESeDispatch[i].DispatchHandler(device,
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
NfcCxESEInterfaceAddClient(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine holds the reference of the client context

Arguments:

    ESEInterface - A pointer to the ESEInterface
    FileContext - Client to add

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    bool stopIdleCalled = false;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(ESEInterface->SmartCardLock, NULL);

    if (ESEInterface->CurrentClient != NULL) {
        WdfWaitLockRelease(ESEInterface->SmartCardLock);
        TRACE_LINE(LEVEL_ERROR, "There is existing file handle on the eSE SmartCard");
        status = STATUS_ACCESS_DENIED;
        goto Done;
    }

    ESEInterface->CurrentClient = FileContext;

    WdfWaitLockRelease(ESEInterface->SmartCardLock);

    // Ensure RF has been initialized, by waiting for the device to enter D0, so that we can successfully enable the SE.
    status = WdfDeviceStopIdle(ESEInterface->FdoContext->Device, /*WaitForD0*/ TRUE);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
        goto Done;
    }
    stopIdleCalled = true;

    // Enable the SE.
    status = NfcCxSEInterfaceSetCardWiredMode(
        ESEInterface->FdoContext->SEInterface,
        ESEInterface->SecureElementId,
        TRUE); // WiredMode
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
        goto Done;
    }

    // Signal smartcard connected event.
    NfcCxESEInterfaceHandleSmartCardConnectionEstablished(ESEInterface, ESEInterface->FdoContext->RFInterface);

Done:
    if (stopIdleCalled)
    {
        WdfDeviceResumeIdle(ESEInterface->FdoContext->Device);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxESEInterfaceRemoveClient(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

    This routine release the reference of the client

Arguments:

    ESEInterface - A pointer to the ESEInterface

Return Value:

    VOID

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(ESEInterface->SmartCardLock, NULL);

    if (ESEInterface->CurrentClient != FileContext) {
        WdfWaitLockRelease(ESEInterface->SmartCardLock);
        TRACE_LINE(LEVEL_ERROR, "eSE SmartCard client not removed. File context mismatch.");
        goto Done;
    }

    ESEInterface->CurrentClient = NULL;

    WdfWaitLockRelease(ESEInterface->SmartCardLock);

    TRACE_LINE(LEVEL_INFO, "eSE SmartCard client = %p removed", FileContext);

    // Disable WIRED mode
    status = NfcCxSEInterfaceSetCardWiredMode(
        ESEInterface->FdoContext->SEInterface,
        ESEInterface->SecureElementId,
        FALSE); // WiredMode
    if (!NT_SUCCESS(status))
    {
        // Best effort
        TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
    }

    // Signal smartcard disconnected event.
    NfcCxESEInterfaceHandleSmartCardConnectionLost(ESEInterface);

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

//
// Handling methods below
//

VOID
NfcCxESEInterfaceHandleSmartCardConnectionEstablished(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
/*++

Routine Description:

    This routine distributes the SmartCardConnectionEstablished event

Arguments:

    ESEInterface - ESEInterface object corresponding to smartcard connection
    RFInterface - RFInterface of ESEInterface provided containing data for ESEInterface

Return Value:

    VOID

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(ESEInterface->SmartCardLock, NULL);

    ESEInterface->SmartCardConnected = TRUE;
    TRACE_LINE(LEVEL_INFO, "eSE SmartCardConnectionEstablished!!!");

    WdfWaitLockRelease(ESEInterface->SmartCardLock);

    NfcCxSCPresentAbsentDispatcherCompleteRequest(ESEInterface->FdoContext, &ESEInterface->PresentDispatcher);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

VOID
NfcCxESEInterfaceHandleSmartCardConnectionLost(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface
    )
/*++

Routine Description:

    This routine distribute The SmartCardConnectionLost event

Arguments:

    ESEInterface - A pointer to the ESEInterface

Return Value:

    VOID

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(ESEInterface->SmartCardLock, NULL);

    if (!ESEInterface->SmartCardConnected) {
        WdfWaitLockRelease(ESEInterface->SmartCardLock);
        TRACE_LINE(LEVEL_INFO, "eSE SmartCard is not connected, no action taken in connection lost!");
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "eSE SmartCardConnectionLost!!!");

    ESEInterface->SmartCardConnected = FALSE;

    WdfWaitLockRelease(ESEInterface->SmartCardLock);

    // Complete IsAbsent Request.
    NfcCxSCPresentAbsentDispatcherCompleteRequest(ESEInterface->FdoContext, &ESEInterface->AbsentDispatcher);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

//
// Dispatched methods below
//

NTSTATUS
NfcCxESEInterfaceDispatchGetAttribute(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SmartCard Get Attribute

Arguments:

    Device - Handle to a framework device object.
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    size_t cbOutputBuffer = OutputBufferLength;
    DWORD attributeId;
    PNFCCX_ESE_INTERFACE eseInterface;
    PNFCCX_FDO_CONTEXT fdoContext;

    UNREFERENCED_PARAMETER(InputBufferLength);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(sizeof(BYTE) <= OutputBufferLength);
    _Analysis_assume_(sizeof(BYTE) <= InputBufferLength);

    attributeId = *(DWORD*)InputBuffer;
    fdoContext  = NfcCxFdoGetContext(Device);
    eseInterface = fdoContext->ESEInterface;

    switch (attributeId)
    {
    case SCARD_ATTR_VENDOR_IFD_TYPE:
        status = NfcCxCopyToBuffer(ESeVendorIfd, sizeof(ESeVendorIfd), (BYTE*)OutputBuffer, &cbOutputBuffer);
        break;

    case SCARD_ATTR_CHANNEL_ID:
        status = NfcCxCopyToBuffer(&ESeReaderChannelId, sizeof(ESeReaderChannelId), (BYTE*)OutputBuffer, &cbOutputBuffer);
        break;

    case SCARD_ATTR_CURRENT_PROTOCOL_TYPE:
        status = NfcCxESEInterfaceDispatchAttributeCurrentProtocolType(eseInterface, (PBYTE)OutputBuffer, &cbOutputBuffer);
        break;

    case SCARD_ATTR_ICC_PRESENCE:
        status = NfcCxESEInterfaceDispatchAttributePresent(eseInterface, (PBYTE)OutputBuffer, &cbOutputBuffer);
        break;

    case SCARD_ATTR_ATR_STRING:
        status = NfcCxESEInterfaceDispatchAttributeAtr(eseInterface, (PBYTE)OutputBuffer, &cbOutputBuffer);
        break;

    case SCARD_ATTR_ICC_TYPE_PER_ATR:
        status = NfcCxESEInterfaceDispatchAttributeIccType(eseInterface, (PBYTE)OutputBuffer, &cbOutputBuffer);
        break;

    default:
        status = NfcCxSCCommonGetAttribute(attributeId, (PBYTE)OutputBuffer, &cbOutputBuffer);
        break;
    }

    if (NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_INFO,
            "Completing request %p, with %!STATUS!, 0x%I64x", Request, status, cbOutputBuffer);
        WdfRequestCompleteWithInformation(Request, status, cbOutputBuffer);
        //
        // Since we have completed the request here,
        // return STATUS_PENDING to avoid double completion of the request
        //
        status = STATUS_PENDING;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

NTSTATUS
NfcCxESEInterfaceDispatchSetAttribute(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SmartCard Set Attribute

Arguments:

    Device - Handle to a framework device object.
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    DWORD attributeId;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(sizeof(DWORD) <= InputBufferLength);

    attributeId = *(DWORD*)InputBuffer;

    if (attributeId == SCARD_ATTR_DEVICE_IN_USE)
    {
        status = STATUS_SUCCESS;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

NTSTATUS
NfcCxESEInterfaceDispatchGetState(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SmartCard Get State

Arguments:

    Device - Handle to a framework device object.
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
    PNFCCX_ESE_INTERFACE eseInterface;
    DWORD *pdwState = (DWORD*)OutputBuffer;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    eseInterface = NfcCxFdoGetContext(Device)->ESEInterface;

    WdfWaitLockAcquire(eseInterface->SmartCardLock, NULL);

    if (eseInterface->SmartCardConnected) {
        *pdwState = SCARD_SPECIFIC;
    }
    else {
        *pdwState = SCARD_ABSENT;
    }

    WdfWaitLockRelease(eseInterface->SmartCardLock);

    if (NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_INFO,
            "Completing request %p, with %!STATUS!, 0x%I64x", Request, status, sizeof(DWORD));
        WdfRequestCompleteWithInformation(Request, status, sizeof(DWORD));
        //
        // Since we have completed the request here,
        // return STATUS_PENDING to avoid double completion of the request
        //
        status = STATUS_PENDING;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

NTSTATUS
NfcCxESEInterfaceDispatchSetPower(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SmartCard Set Power

Arguments:

    Device - Handle to a framework device object.
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
    PNFCCX_ESE_INTERFACE eseInterface;
    DWORD *pdwPower = (DWORD*)InputBuffer;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(sizeof(DWORD) <= InputBufferLength);

    eseInterface = NfcCxFdoGetContext(Device)->ESEInterface;

    WdfWaitLockAcquire(eseInterface->SmartCardLock, NULL);

    if (!eseInterface->SmartCardConnected) {
        status = STATUS_NO_MEDIA;
        WdfWaitLockRelease(eseInterface->SmartCardLock);
        goto Done;
    }

    WdfWaitLockRelease(eseInterface->SmartCardLock);

    switch (*pdwPower)
    {
    case SCARD_COLD_RESET:
    case SCARD_WARM_RESET:
        NfcCxESEInterfaceResetCard(eseInterface);
        break;

    case SCARD_POWER_DOWN:
        status = STATUS_NOT_SUPPORTED;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

NTSTATUS
NfcCxESEInterfaceDispatchSetProtocol(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SmartCard Set Protocol

Arguments:

    Device - Handle to a framework device object.
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
    PNFCCX_ESE_INTERFACE eseInterface;
    DWORD *pdwProtocol = (DWORD*)InputBuffer;
    DWORD *pdwSelectedProtocol = (DWORD*)OutputBuffer;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(sizeof(DWORD) <= InputBufferLength);
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    eseInterface = NfcCxFdoGetContext(Device)->ESEInterface;

    WdfWaitLockAcquire(eseInterface->SmartCardLock, NULL);

    if (!eseInterface->SmartCardConnected) {
        TRACE_LINE(LEVEL_ERROR, "SmartCard not connected");
        status = STATUS_NO_MEDIA;
        WdfWaitLockRelease(eseInterface->SmartCardLock);
        goto Done;
    }

    WdfWaitLockRelease(eseInterface->SmartCardLock);

    if (*pdwProtocol == SCARD_PROTOCOL_OPTIMAL) {
        *pdwSelectedProtocol = SCReaderCurrentProtocolType;
    }
    else if ((((*pdwProtocol) & SCARD_PROTOCOL_DEFAULT) != 0) ||
            (((*pdwProtocol) & SCARD_PROTOCOL_T1) != 0) ) {
        *pdwSelectedProtocol = SCReaderCurrentProtocolType;
    }
    else if ((((*pdwProtocol) & SCARD_PROTOCOL_RAW) != 0) ||
            (((*pdwProtocol) & SCARD_PROTOCOL_T0) != 0) ||
            (((*pdwProtocol) & SCARD_PROTOCOL_Tx) != 0) ) {
        status = STATUS_NOT_SUPPORTED;
        TRACE_LINE(LEVEL_ERROR, "Protocol not supported %d", *pdwProtocol);
    }
    else {
        status = STATUS_INVALID_DEVICE_REQUEST;
        TRACE_LINE(LEVEL_ERROR, "Invalid protocol %d", *pdwProtocol);
    }

Done:
    if (NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_INFO,
            "Completing request %p, with %!STATUS!, 0x%I64x", Request, status, sizeof(DWORD));
        WdfRequestCompleteWithInformation(Request, status, sizeof(DWORD));
        //
        // Since we have completed the request here,
        // return STATUS_PENDING to avoid double completion of the request
        //
        status = STATUS_PENDING;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

NTSTATUS
NfcCxESEInterfaceDispatchIsAbsent(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SmartCard Is Absent

Arguments:

    Device - Handle to a framework device object.
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
    PNFCCX_ESE_INTERFACE eseInterface;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    eseInterface = NfcCxFdoGetContext(Device)->ESEInterface;

    WdfWaitLockAcquire(eseInterface->SmartCardLock, NULL);

    if (!eseInterface->SmartCardConnected) {
        WdfWaitLockRelease(eseInterface->SmartCardLock);
        WdfRequestComplete(Request, STATUS_SUCCESS);
        goto Done;
    }

    WdfWaitLockRelease(eseInterface->SmartCardLock);

    // Pass request to Present/Absent dispatcher.
    status = NfcCxSCPresentAbsentDispatcherSetRequest(eseInterface->FdoContext, &eseInterface->AbsentDispatcher, Request);

Done:
    if (NT_SUCCESS(status)) {
        //
        // Since we have completed the request here,
        // return STATUS_PENDING to avoid double completion of the request
        //
        status = STATUS_PENDING;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxESEInterfaceDispatchIsPresent(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SmartCard Is Present

Arguments:

    Device - Handle to a framework device object.
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
    PNFCCX_ESE_INTERFACE eseInterface;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    eseInterface = NfcCxFdoGetContext(Device)->ESEInterface;

    WdfWaitLockAcquire(eseInterface->SmartCardLock, NULL);

    if (eseInterface->SmartCardConnected) {
        WdfWaitLockRelease(eseInterface->SmartCardLock);
        WdfRequestComplete(Request, STATUS_SUCCESS);
        goto Done;
    }

    WdfWaitLockRelease(eseInterface->SmartCardLock);

    status = NfcCxSCPresentAbsentDispatcherSetRequest(eseInterface->FdoContext, &eseInterface->PresentDispatcher, Request);

Done:
    if (NT_SUCCESS(status)) {
        //
        // Since we have completed the request here,
        // return STATUS_PENDING to avoid double completion of the request
        //
        status = STATUS_PENDING;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxESEInterfaceDispatchTransmit(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the SmartCard Transmit

Arguments:

    Device - Handle to a framework device object.
    Request - Handle to a framework request object.
    InputBuffer - The input buffer.
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_ESE_INTERFACE eseInterface = NULL;
    DWORD cbOutputBuffer = 0;
    size_t OutputBufferUsed = 0;
    DWORD cbResponseBuffer = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    cbOutputBuffer = (DWORD)OutputBufferLength;

    eseInterface = NfcCxFdoGetContext(Device)->ESEInterface;

    TRACE_LINE(LEVEL_INFO, "Buffer length %lu", cbOutputBuffer);

    if (!eseInterface->SmartCardConnected) {
        TRACE_LINE(LEVEL_ERROR, "Failed to transmit: Embedded SE is NOT in wired mode");
        status = STATUS_NO_MEDIA;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Embedded SE Transceive to start....");

    status = NfcCxRFInterfaceESETransmit(
        eseInterface->FdoContext->RFInterface,
        (PBYTE)InputBuffer + sizeof(SCARD_IO_REQUEST),
        InputBufferLength - sizeof(SCARD_IO_REQUEST),
        (PBYTE)OutputBuffer + sizeof(SCARD_IO_REQUEST),
        cbOutputBuffer - sizeof(SCARD_IO_REQUEST),
        &OutputBufferUsed,
        DEFAULT_HCI_TX_RX_TIME_OUT);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_INFO, "Embedded SE Transceive failed, %!STATUS!", status);
        goto Done;
    }

    status = NfcCxSCCommonCopyTrasmitResponseData(OutputBuffer, cbOutputBuffer, NULL, 0, &cbResponseBuffer);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to construct response buffer, %!STATUS!", status);
        goto Done;
    }

    OutputBufferUsed += cbResponseBuffer;
    TRACE_LINE(LEVEL_INFO, "Output Buffer length after adding header is %Iu", OutputBufferUsed);

Done:
    if (NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_INFO,
            "Completing request %p, with %!STATUS!, Output buffer length = %Iu", Request, status, OutputBufferUsed);
        WdfRequestCompleteWithInformation(Request, status, OutputBufferUsed);
        //
        // Since we have completed the request here,
        // return STATUS_PENDING to avoid double completion of the request
        //
        status = STATUS_PENDING;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

//
// Dispatched attribute methods below
//

NTSTATUS
NfcCxESEInterfaceDispatchAttributePresent(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
/*++

Routine Description:

    This routine dispatches to get the present state

Arguments:

    ESEInterface - Pointer to the SmartCard Reader interface.
    pbResultBuffer - The buffer points to the result value.
    cbResultBuffer - Length of the result buffer.
    pbOutputBuffer - The output buffer.
    pcbOutputBuffer - Pointer to the length of the output buffer.

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    WdfWaitLockAcquire(ESEInterface->SmartCardLock, NULL);
    BYTE IccPresence = (ESEInterface->SmartCardConnected ? 0x1 : 0x0);
    WdfWaitLockRelease(ESEInterface->SmartCardLock);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = NfcCxCopyToBuffer(&IccPresence, sizeof(IccPresence), pbOutputBuffer, pcbOutputBuffer);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxESEInterfaceDispatchAttributeCurrentProtocolType(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
/*++

Routine Description:

    This routine dispatches to get the current protocol type

Arguments:

    ESEInterface - Pointer to the SmartCard Reader interface.
    pbOutputBuffer - The output buffer.
    pcbOutputBuffer - Pointer to the length of the output buffer.

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(ESEInterface->SmartCardLock, NULL);
    BOOLEAN isSmartCardConnected = ESEInterface->SmartCardConnected;
    WdfWaitLockRelease(ESEInterface->SmartCardLock);

    if (!isSmartCardConnected) {
        TRACE_LINE(LEVEL_ERROR, "SmartCard not connected");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    status = NfcCxCopyToBuffer(&SCReaderCurrentProtocolType, sizeof(SCReaderCurrentProtocolType), pbOutputBuffer, pcbOutputBuffer);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

_Requires_lock_not_held_(ESEInterface->SmartCardLock)
NTSTATUS
NfcCxESEInterfaceDispatchAttributeAtr(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
/*++

Routine Description:

    This routine dispatches to get the present state

Arguments:

    ESEInterface - Pointer to the SmartCard Reader interface.
    pbOutputBuffer - The output buffer.
    pcbOutputBuffer - Pointer to the length of the output buffer.

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PNFCCX_FDO_CONTEXT fdoContext = ESEInterface->FdoContext;
    PNFCCX_RF_INTERFACE rfInterface = fdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(ESEInterface->SmartCardLock, NULL);

    if (!ESEInterface->SmartCardConnected) {
        TRACE_LINE(LEVEL_ERROR, "SmartCard not connected");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Get Attribute to start Output Buffer length is %Iu....", *pcbOutputBuffer);

    status = NfcCxRFInterfaceESEGetATRString(
        rfInterface,
        (PBYTE)pbOutputBuffer,
        *pcbOutputBuffer,
        pcbOutputBuffer);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Get Attribute failed, %!STATUS!", status);
        goto Done;
    }

Done:
    WdfWaitLockRelease(ESEInterface->SmartCardLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxESEInterfaceDispatchAttributeIccType(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
/*++

Routine Description:

    This routine dispatches to get the present state

Arguments:

    ESEInterface - Pointer to the SmartCard Reader interface.
    pbOutputBuffer - The output buffer.
    pcbOutputBuffer - Pointer to the length of the output buffer.

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    WdfWaitLockAcquire(ESEInterface->SmartCardLock, NULL);

    if (!ESEInterface->SmartCardConnected) {
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    status = NfcCxCopyToBuffer(&ESeIccType, sizeof(ESeIccType), pbOutputBuffer, pcbOutputBuffer);

Done:
    WdfWaitLockRelease(ESEInterface->SmartCardLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

//
// Other internal methods below
//

_Requires_lock_not_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxESEInterfaceResetCard(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface
    )
/*++

Routine Description:

    This routine warm resets the smart card

Arguments:

    ESEInterface - The eSE Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = NfcCxSEInterfaceResetCard(ESEInterface->FdoContext->SEInterface, ESEInterface->SecureElementId);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}
