/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxESE.cpp

Abstract:

    Embedded SE Interface definition

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxESE.tmh"

// TODO: Temporarily define this here until we get ScDeviceEnum.h published
// {D6B5B883-18BD-4B4D-B2EC-9E38AFFEDA82}, 2, DEVPROP_TYPE_BYTE
DEFINE_DEVPROPKEY(DEVPKEY_Device_ReaderKind,
    0xD6B5B883, 0x18BD, 0x4B4D, 0xB2, 0xEC, 0x9E, 0x38, 0xAF, 0xFE, 0xDA, 0x82, 0x02);

#define SCARD_READER_TYPE_EMBEDDEDSE 0x800

typedef
NTSTATUS
NFCCX_EmbeddedSE_DISPATCH_HANDLER(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

typedef NFCCX_EmbeddedSE_DISPATCH_HANDLER *PFN_NFCCX_EmbeddedSE_DISPATCH_HANDLER;

typedef struct _NFCCX_eSE_DISPATCH_ENTRY {
    ULONG IoControlCode;
    BOOLEAN fPowerManaged;
    BOOLEAN fSequentialDispatch;
    size_t MinimumInputBufferLength;
    size_t MinimumOutputBufferLength;
    PFN_NFCCX_EmbeddedSE_DISPATCH_HANDLER DispatchHandler;
} NFCCX_eSE_DISPATCH_ENTRY, *PNFCCX_eSE_DISPATCH_ENTRY;

NFCCX_SC_DISPATCH_HANDLER NfcCxEmbeddedSEInterfaceDispatchTransmit;

typedef struct _EmbeddedSE_ATTRIBUTE_DISPATCH_ENTRY {
    DWORD dwAttributeId;
    PBYTE pbResultBuffer;
    size_t cbResultBuffer;
    BOOL fRequireLock;
    PFN_NFCCX_SC_ATTRIBUTE_DISPATCH_HANDLER pfnDispatchHandler;
} EmbeddedSE_ATTRIBUTE_DISPATCH_ENTRY, *PEmbeddedSEATTRIBUTE_DISPATCH_ENTRY;

static const CHAR eSEReaderVendorName[] = "NXP";
static const CHAR eSEReaderVendorIfd[] = "eSE";
static const DWORD eSEReaderVendorIfdVersion = ((IFD_MAJOR_VER & 0x3) << 6) | ((IFD_MINOR_VER & 0x3) << 4) | (IFD_BUILD_NUM & 0xF);
static const DWORD eSEReaderChannelId = SCARD_READER_TYPE_EMBEDDEDSE << 16;
static const DWORD eSEReaderProtocolTypes = SCARD_PROTOCOL_T1;
static const DWORD eSEReaderDeviceUnit = 0;
static const DWORD eSEReaderDefaultClk = 13560;
static const DWORD eSEReaderMaxClk = 13560;
static const DWORD eSEReaderDefaultDataRate = 1;
static const DWORD eSEReaderMaxDataRate = 1;
static const DWORD eSEReaderMaxIfsd = 254;
static const DWORD eSEReaderCharacteristics = SCARD_READER_CONTACTLESS;
static const DWORD eSEReaderCurrentProtocolType = SCARD_PROTOCOL_T1;
static const DWORD eSEReaderCurrentClk = 13560;
static const DWORD eSEReaderCurrentD = 1;
static const DWORD eSEReaderCurrentIfsc = 32;
static const DWORD eSEReaderCurrentIfsd = 254;
static const DWORD eSEReaderCurrentBwt = 4;


_NFCCX_eSE_DISPATCH_ENTRY
g_EmbeddedSEDispatch[] = {
    { IOCTL_SMARTCARD_IS_ABSENT,            FALSE,  FALSE,  0,                                 0,                                NfcCxEmbeddedSEInterfaceDispatchIsAbsent },
    { IOCTL_SMARTCARD_IS_PRESENT,           FALSE,  FALSE,  0,                                 0,                                NfcCxEmbeddedSEInterfaceDispatchIsPresent },
    { IOCTL_SMARTCARD_GET_STATE,            FALSE,  FALSE,  0,                                 sizeof(DWORD),                    NfcCxEmbeddedSEInterfaceDispatchGetState },
    // IOCTL_SMARTCARD_TRANSMIT is expecting input to have at least one byte and IO_REQUEST and output to be SW1 + SW2 + IO_REQUEST
    { IOCTL_SMARTCARD_TRANSMIT,             TRUE,   TRUE,   sizeof(SCARD_IO_REQUEST) + 1,      sizeof(SCARD_IO_REQUEST) + 2,     NfcCxEmbeddedSEInterfaceDispatchTransmit },
    { IOCTL_SMARTCARD_GET_ATTRIBUTE,        TRUE,   FALSE,  sizeof(DWORD),                     sizeof(BYTE),                     NfcCxEmbeddedSEInterfaceDispatchGetAttribute },
    { IOCTL_SMARTCARD_SET_ATTRIBUTE,        FALSE,  FALSE,  sizeof(DWORD),                     0,                                NfcCxEmbeddedSEInterfaceDispatchSetAttribute },
    { IOCTL_SMARTCARD_POWER,                FALSE,  FALSE,  sizeof(DWORD),                     0,                                NfcCxEmbeddedSEInterfaceDispatchSetPower },
    { IOCTL_SMARTCARD_SET_PROTOCOL,         FALSE,  FALSE,  sizeof(DWORD),                     sizeof(DWORD),                    NfcCxEmbeddedSEInterfaceDispatchSetProtocol },
    { IOCTL_SMARTCARD_EJECT,                FALSE,  FALSE,  0,                                 0,                                NfcCxSCInterfaceDispatchNotSupported },
    { IOCTL_SMARTCARD_GET_LAST_ERROR,       FALSE,  FALSE,  0,                                 sizeof(DWORD),                    NfcCxSCInterfaceDispatchGetLastError },
    { IOCTL_SMARTCARD_SWALLOW,              FALSE,  FALSE,  0,                                 0,                                NfcCxSCInterfaceDispatchNotSupported },
    { IOCTL_SMARTCARD_CONFISCATE,           FALSE,  FALSE,  0,                                 0,                                NfcCxSCInterfaceDispatchNotSupported },
    { IOCTL_SMARTCARD_GET_PERF_CNTR,        FALSE,  FALSE,  0,                                 0,                                NfcCxSCInterfaceDispatchNotSupported },
};

EmbeddedSE_ATTRIBUTE_DISPATCH_ENTRY
g_EmbeddedSEAttributeDispatch[] = {
    { SCARD_ATTR_VENDOR_NAME,               (PBYTE)eSEReaderVendorName,           sizeof(eSEReaderVendorName),             TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_VENDOR_IFD_TYPE,           (PBYTE)eSEReaderVendorIfd,            sizeof(eSEReaderVendorName),              TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_VENDOR_IFD_VERSION,        (PBYTE)&eSEReaderVendorIfdVersion,    sizeof(eSEReaderVendorName),       TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_CHANNEL_ID,                (PBYTE)&eSEReaderChannelId,           sizeof(eSEReaderVendorName),              TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_PROTOCOL_TYPES,            (PBYTE)&eSEReaderProtocolTypes,       sizeof(eSEReaderVendorName),          TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_DEVICE_UNIT,               (PBYTE)&eSEReaderDeviceUnit,          sizeof(eSEReaderVendorName),             TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_DEFAULT_CLK,               (PBYTE)&eSEReaderDefaultClk,          sizeof(eSEReaderVendorName),             TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_MAX_CLK,                   (PBYTE)&eSEReaderMaxClk,              sizeof(eSEReaderVendorName),                 TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_DEFAULT_DATA_RATE,         (PBYTE)&eSEReaderDefaultDataRate,     sizeof(eSEReaderVendorName),        TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_MAX_DATA_RATE,             (PBYTE)&eSEReaderMaxDataRate,         sizeof(eSEReaderVendorName),            TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_MAX_IFSD,                  (PBYTE)&eSEReaderMaxIfsd,             sizeof(eSEReaderVendorName),                TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_CHARACTERISTICS,           (PBYTE)&eSEReaderCharacteristics,     sizeof(eSEReaderVendorName),        TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_CURRENT_CLK,               (PBYTE)&eSEReaderCurrentClk,          sizeof(eSEReaderVendorName),             TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_CURRENT_D,                 (PBYTE)&eSEReaderCurrentD,            sizeof(eSEReaderVendorName),               TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_CURRENT_IFSC,              (PBYTE)&eSEReaderCurrentIfsc,         sizeof(eSEReaderVendorName),            TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_CURRENT_IFSD,              (PBYTE)&eSEReaderCurrentIfsd,         sizeof(eSEReaderVendorName),            TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_CURRENT_BWT,               (PBYTE)&eSEReaderCurrentBwt,          sizeof(eSEReaderVendorName),             TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeLocked },
    { SCARD_ATTR_CURRENT_PROTOCOL_TYPE,     (PBYTE)&eSEReaderCurrentProtocolType, sizeof(eSEReaderVendorName),    TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeCurrentProtocolTypeLocked },
    { SCARD_ATTR_ICC_PRESENCE,              NULL,                                0,                                      TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributePresentLocked },
    { SCARD_ATTR_ATR_STRING,                NULL,                                0,                                      FALSE, &NfcCxEmbeddedSEInterfaceDispatchAttributeAtr },
    { SCARD_ATTR_ICC_TYPE_PER_ATR,          NULL,                                0,                                      TRUE,  &NfcCxEmbeddedSEInterfaceDispatchAttributeIccTypeLocked },
};

//
// Dispatched attribute methods below
//

FORCEINLINE NTSTATUS
NfcCxEmbeddedSEInterfaceCopyToBuffer(
    _In_reads_bytes_(cbAttributeValue) const VOID *pbAttributeValue,
    _In_ size_t cbAttributeValue,
    _Out_writes_bytes_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (*pcbOutputBuffer < cbAttributeValue) {
        status = STATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    RtlCopyMemory(pbOutputBuffer, pbAttributeValue, cbAttributeValue);
    *pcbOutputBuffer = cbAttributeValue;

Done:

    return status;
}

_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchAttributeLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
    /*++

    Routine Description:

    This routine dispatches to copy the attribute dword

    Arguments:

    ScInterface - Pointer to the SmartCard Reader interface.
    pbResultBuffer - The buffer points to the result value.
    cbResultBuffer - Length of the result buffer.
    pbOutputBuffer - The output buffer.
    pcbOutputBuffer - Pointer to the length of the output buffer.

    Return Value:

    NTSTATUS.

    --*/
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ScInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = NfcCxEmbeddedSEInterfaceCopyToBuffer(pbResultBuffer, cbResultBuffer, pbOutputBuffer, pcbOutputBuffer);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchAttributeCurrentProtocolTypeLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
    /*++

    Routine Description:

    This routine dispatches to get the current protocol type

    Arguments:

    ScInterface - Pointer to the SmartCard Reader interface.
    pbResultBuffer - The buffer points to the result value.
    cbResultBuffer - Length of the result buffer.
    pbOutputBuffer - The output buffer.
    pcbOutputBuffer - Pointer to the length of the output buffer.

    Return Value:

    NTSTATUS.

    --*/
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ScInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = NfcCxEmbeddedSEInterfaceCopyToBuffer(pbResultBuffer, cbResultBuffer, pbOutputBuffer, pcbOutputBuffer);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}


_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchAttributePresentLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
    /*++

    Routine Description:

    This routine dispatches to get the present state

    Arguments:

    ScInterface - Pointer to the SmartCard Reader interface.
    pbResultBuffer - The buffer points to the result value.
    cbResultBuffer - Length of the result buffer.
    pbOutputBuffer - The output buffer.
    pcbOutputBuffer - Pointer to the length of the output buffer.

    Return Value:

    NTSTATUS.

    --*/
{
    NTSTATUS status = STATUS_SUCCESS;
    NFCCX_SE_INTERFACE* seInterface = NULL;
    BYTE iccPresent = 0x0;

    UNREFERENCED_PARAMETER(pbResultBuffer);
    UNREFERENCED_PARAMETER(cbResultBuffer);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    seInterface = ScInterface->FdoContext->SEInterface;
    iccPresent = seInterface->IsInWiredMode ? 0x1 : 0x0;

    TRACE_LINE(LEVEL_INFO, "ICC present? %u", iccPresent);

    status = NfcCxEmbeddedSEInterfaceCopyToBuffer(&iccPresent, sizeof(iccPresent), pbOutputBuffer, pcbOutputBuffer);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxEmbeddedSEInterfaceValidateRequest(
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

    for (i = 0; i < ARRAYSIZE(g_EmbeddedSEDispatch); i++) {
        if (g_EmbeddedSEDispatch[i].IoControlCode == IoControlCode) {

            if (g_EmbeddedSEDispatch[i].MinimumInputBufferLength > InputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Input buffer.  Expected %I64x, got %I64x",
                    g_EmbeddedSEDispatch[i].MinimumInputBufferLength,
                    InputBufferLength);
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            if (g_EmbeddedSEDispatch[i].MinimumOutputBufferLength > OutputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Output buffer.  Expected %I64x, got %I64x",
                    g_EmbeddedSEDispatch[i].MinimumOutputBufferLength,
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

//
//Set Dispatch Method
//

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchSetAttribute(
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
    DWORD *pdwAttributeId = (DWORD*)InputBuffer;

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

    if (*pdwAttributeId == SCARD_ATTR_DEVICE_IN_USE) {
        status = STATUS_SUCCESS;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}


//
// Dispatched methods below
//

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchGetAttribute(
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
    DWORD *pdwAttributeId = (DWORD*)InputBuffer;
    size_t cbOutputBuffer = OutputBufferLength;
    PNFCCX_SC_INTERFACE scInterface;


    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(sizeof(BYTE) <= OutputBufferLength);

    scInterface = NfcCxFdoGetContext(Device)->SCInterface;

    for (USHORT TableEntry = 0; TableEntry < ARRAYSIZE(g_EmbeddedSEAttributeDispatch); TableEntry++) {
        if (g_EmbeddedSEAttributeDispatch[TableEntry].dwAttributeId == *pdwAttributeId) {

            if (g_EmbeddedSEAttributeDispatch[TableEntry].fRequireLock) {
                WdfWaitLockAcquire(scInterface->SmartCardLock, NULL);
            }

            status = (*g_EmbeddedSEAttributeDispatch[TableEntry].pfnDispatchHandler)(
                scInterface,
                g_EmbeddedSEAttributeDispatch[TableEntry].pbResultBuffer,
                g_EmbeddedSEAttributeDispatch[TableEntry].cbResultBuffer,
                (PBYTE)OutputBuffer,
                (size_t*)&cbOutputBuffer);

            if (g_EmbeddedSEAttributeDispatch[TableEntry].fRequireLock) {
                WdfWaitLockRelease(scInterface->SmartCardLock);
            }

            break;
        }
    }


    if (NT_SUCCESS(status)) {
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
NfcCxEmbeddedSEInterfaceDispatchGetState(
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
    PNFCCX_SC_INTERFACE scInterface;
    PNFCCX_SE_INTERFACE eSEInterface;
    DWORD *pdwState = (DWORD*)OutputBuffer;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Buffer validated by the validation method
    //
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    scInterface = NfcCxFdoGetContext(Device)->SCInterface;
    eSEInterface = NfcCxFdoGetContext(Device)->SEInterface;

    WdfWaitLockAcquire(scInterface->SmartCardLock, NULL);

    if (eSEInterface->IsInWiredMode) {
        *pdwState = SCARD_SPECIFIC;
    }
    else {
        *pdwState = SCARD_ABSENT;
    }

    WdfWaitLockRelease(scInterface->SmartCardLock);

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
NfcCxEmbeddedSEInterfaceDispatchTransmit(
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
    phLibNfc_SE_List_t SEList[MAX_NUMBER_OF_SE] = {0};
    PNFCCX_SE_INTERFACE eSEInterface = NULL;
    size_t OutputBufferUsed = 0;
    DWORD cbOutputBuffer = 0;
    DWORD cbResponseBuffer = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);

    cbOutputBuffer = (DWORD)OutputBufferLength;

    rfInterface = NfcCxFdoGetContext(Device)->RFInterface;
    eSEInterface = NfcCxFdoGetContext(Device)->SEInterface;

    TRACE_LINE(LEVEL_INFO, "Buffer length %lu", cbOutputBuffer);

    if (!eSEInterface->IsInWiredMode) {
        TRACE_LINE(LEVEL_ERROR, "Failed to transmit: Embedded SE is NOT in wired mode");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Embedded SE Transceive to start....");

    status = NfcCxRFInterfaceEmbeddedSETransmit(eSEInterface->FdoContext->RFInterface,
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

    status = NfcCxEmbeddedSEInterfaceCopyResponseData(OutputBuffer, cbOutputBuffer, NULL, 0, &cbResponseBuffer);
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

    return status;
}

BOOLEAN
NfcCxEmbeddedSEInterfaceIsIoctlSupported(
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

    for (i = 0; i < ARRAYSIZE(g_EmbeddedSEDispatch); i++) {
        if (g_EmbeddedSEDispatch[i].IoControlCode == IoControlCode) {
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchRequest(
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

    for (i = 0; i < ARRAYSIZE(g_EmbeddedSEDispatch); i++) {
        if (g_EmbeddedSEDispatch[i].IoControlCode == IoControlCode) {

            status = g_EmbeddedSEDispatch[i].DispatchHandler(device,
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

BOOLEAN
NfcCxEmbeddedSEIsPowerManagedRequest(
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

    for (i = 0; i < ARRAYSIZE(g_EmbeddedSEDispatch); i++) {
        if (g_EmbeddedSEDispatch[i].IoControlCode == IoControlCode) {
            return g_EmbeddedSEDispatch[i].fPowerManaged;
        }
    }

    return FALSE;
}


NTSTATUS
NfcCxEmbeddedSEInterfaceIoDispatch(
    _In_opt_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
    /*++

    Routine Description:

    This is the first entry into the SCInterface.  It validates and dispatches SmartCard request
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
    WDFMEMORY outMem = { 0 };
    WDFMEMORY inMem = { 0 };
    PVOID     inBuffer = NULL;
    PVOID     outBuffer = NULL;
    size_t    sizeInBuffer = 0;
    size_t    sizeOutBuffer = 0;
    BOOLEAN   releasePowerReference = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    fdoContext = NfcCxFileObjectGetFdoContext(FileContext);

    UNREFERENCED_PARAMETER(FileContext);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    //
    // Is the NFP radio enabled. SmartCard sharing the same radio control with NFP
    //
    if (FALSE == NfcCxPowerIsAllowedNfp(fdoContext)) {
        TRACE_LINE(LEVEL_ERROR, "NFP radio is off");
        status = STATUS_DEVICE_POWERED_OFF;
        goto Done;
    }

    //
    // Take a power reference if the request is power managed
    //
    if (NfcCxEmbeddedSEIsPowerManagedRequest(IoControlCode)) {
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
    status = NfcCxEmbeddedSEInterfaceValidateRequest(IoControlCode,
        InputBufferLength,
        OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Request validation failed, %!STATUS!", status);
        goto Done;
    }


    //
    // Dispatch the request
    //
    status = NfcCxEmbeddedSEInterfaceDispatchRequest(FileContext,
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
NfcCxEmbeddedSEInterfaceDispatchAttributeIccTypeLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
    /*++

    Routine Description:

    This routine dispatches to get the present state

    Arguments:

    ScInterface - Pointer to the SmartCard Reader interface.
    pbResultBuffer - The buffer points to the result value.
    cbResultBuffer - Length of the result buffer.
    pbOutputBuffer - The output buffer.
    pcbOutputBuffer - Pointer to the length of the output buffer.

    Return Value:

    NTSTATUS.

    --*/
{
    NTSTATUS status = STATUS_SUCCESS;
    BYTE iccType = 0x0;
    NFCCX_SE_INTERFACE* seInterface = NULL;

    UNREFERENCED_PARAMETER(pbResultBuffer);
    UNREFERENCED_PARAMETER(cbResultBuffer);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    seInterface = ScInterface->FdoContext->SEInterface;
    if (!seInterface->IsInWiredMode) {
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    // Since SE always support 3A, the type is by default 3A
    iccType = ICC_TYPE_14443_TYPE_A; // 14443 Type A
    status = NfcCxEmbeddedSEInterfaceCopyToBuffer(&iccType, sizeof(iccType), pbOutputBuffer, pcbOutputBuffer);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxEmbeddedSEInterfaceVerifyAndAddIsPresent(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ WDFREQUEST Request
    )
    /*++

    Routine Description:

    This routine verifies the IsPresent request has not been previously called,
    and it forwards the request into a manual IO queue.

    Arguments:

    ScInterface - Pointer to the SmartCard Reader interface.
    Request - Handle to a framework request object.

    Return Value:

    NTSTATUS.

    --*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFREQUEST outRequest = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NT_SUCCESS(WdfIoQueueFindRequest(SEInterface->PresentQueue,
        NULL,
        WdfRequestGetFileObject(Request),
        NULL,
        &outRequest))) {
        WdfObjectDereference(outRequest);
        status = STATUS_DEVICE_BUSY;
        goto Done;
    }

    status = WdfRequestForwardToIoQueue(Request, SEInterface->PresentQueue);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to forward request to IO Queue, %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchIsPresent(
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
    PNFCCX_SC_INTERFACE scInterface = NULL;
    PNFCCX_SE_INTERFACE eSEInterface = NULL;

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

    eSEInterface = NfcCxFdoGetContext(Device)->SEInterface;
    scInterface = NfcCxFdoGetContext(Device)->SCInterface;

    WdfWaitLockAcquire(scInterface->SmartCardLock, NULL);

    if (eSEInterface->IsInWiredMode) {
        WdfWaitLockRelease(scInterface->SmartCardLock);
        WdfRequestComplete(Request, STATUS_SUCCESS);
        TRACE_LINE(LEVEL_INFO, "eSE is present, completing request immediately");
        goto Done;
    }

    WdfWaitLockRelease(scInterface->SmartCardLock);

    TRACE_LINE(LEVEL_INFO, "eSE is not present, queueing request");

    status = NfcCxEmbeddedSEInterfaceVerifyAndAddIsPresent(eSEInterface, Request);

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
NfcCxEmbeddedSEInterfaceVerifyAndAddIsAbsent(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ WDFREQUEST Request
    )
    /*++

    Routine Description:

    This routine verifies the IsAbsent request has not been previously called,
    and it forwards the request into a manual IO queue.

    Arguments:

    ScInterface - Pointer to the SmartCard Reader interface.
    Request - Handle to a framework request object.

    Return Value:

    NTSTATUS.

    --*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFREQUEST outRequest = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NT_SUCCESS(WdfIoQueueFindRequest(SEInterface->AbsentQueue,
        NULL,
        WdfRequestGetFileObject(Request),
        NULL,
        &outRequest))) {
        WdfObjectDereference(outRequest);
        status = STATUS_DEVICE_BUSY;
        goto Done;
    }

    status = WdfRequestForwardToIoQueue(Request, SEInterface->AbsentQueue);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to forward request to IO Queue, %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchIsAbsent(
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
    PNFCCX_SC_INTERFACE scInterface = NULL;
    PNFCCX_SE_INTERFACE eSEInterface = NULL;

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

    scInterface = NfcCxFdoGetContext(Device)->SCInterface;
    eSEInterface = NfcCxFdoGetContext(Device)->SEInterface;

    WdfWaitLockAcquire(scInterface->SmartCardLock, NULL);

    if (!eSEInterface->IsInWiredMode) {
        WdfWaitLockRelease(scInterface->SmartCardLock);
        WdfRequestComplete(Request, STATUS_SUCCESS);
        TRACE_LINE(LEVEL_INFO, "eSE is absent, completing request immediately");
        goto Done;
    }

    WdfWaitLockRelease(scInterface->SmartCardLock);

    TRACE_LINE(LEVEL_INFO, "eSE is not absent, queueing request");

    status = NfcCxEmbeddedSEInterfaceVerifyAndAddIsAbsent(eSEInterface, Request);

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
NfcCxEmbeddedSEInterfaceDispatchSetProtocol(
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
    PNFCCX_SE_INTERFACE seInterface;
    PNFCCX_SC_INTERFACE scInterface;
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

    seInterface = NfcCxFdoGetContext(Device)->SEInterface;
    scInterface = NfcCxFdoGetContext(Device)->SCInterface;

    WdfWaitLockAcquire(scInterface->SmartCardLock, NULL);

    if (!seInterface->IsInWiredMode) {
        TRACE_LINE(LEVEL_ERROR, "eSE not in Wired Mode");
        status = STATUS_NO_MEDIA;
        WdfWaitLockRelease(scInterface->SmartCardLock);
        goto Done;
    }

    WdfWaitLockRelease(scInterface->SmartCardLock);

    if (*pdwProtocol == SCARD_PROTOCOL_OPTIMAL) {
        *pdwSelectedProtocol = eSEReaderCurrentProtocolType;
    }
    else if ((((*pdwProtocol) & SCARD_PROTOCOL_DEFAULT) != 0) ||
        (((*pdwProtocol) & SCARD_PROTOCOL_T1) != 0)) {
        *pdwSelectedProtocol = eSEReaderCurrentProtocolType;
    }
    else if ((((*pdwProtocol) & SCARD_PROTOCOL_RAW) != 0) ||
        (((*pdwProtocol) & SCARD_PROTOCOL_T0) != 0) ||
        (((*pdwProtocol) & SCARD_PROTOCOL_Tx) != 0)) {
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
NfcCxEmbeddedSEInterfaceSetEmulationModeForESE(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ CARD_EMULATION_MODE_INTERNAL eMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NFCCX_SE_INTERFACE* seInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    seInterface = NfcCxFileObjectGetFdoContext(FileContext)->SEInterface;
    status = NfcCxSEInterfaceSetCardEmulationMode(FileContext, seInterface->EmbeddedSeId, eMode);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to set card emulation mode for eSE, %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

#if 0
static VOID CALLBACK
NfcCxEmbeddedSEAPDUThread(
    _In_ PTP_CALLBACK_INSTANCE pInstance,
    _In_ PVOID pContext,
    _In_ PTP_WORK pWork
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FILE_CONTEXT FileContext = (PNFCCX_FILE_CONTEXT)pContext;
    PNFCCX_SE_INTERFACE seInterface = NfcCxFileObjectGetFdoContext(FileContext)->SEInterface;
    //PNFCCX_SC_INTERFACE scInterface = NfcCxFileObjectGetFdoContext(FileContext)->SCInterface;
    CARD_EMULATION_MODE_INTERNAL eMode = CardEmulationOff;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(pInstance);
    UNREFERENCED_PARAMETER(pWork);

    if (seInterface->isAPDUThreadFlag == TRUE) {
        eMode = EmbeddedSeApduMode;
        TRACE_LINE(LEVEL_INFO, "Turning on APDU Mode");
    }
    else {
        eMode = CardEmulationOff;
        TRACE_LINE(LEVEL_INFO, "Turning off APDU Mode");
    }

    status = NfcCxSEInterfaceSetCardEmulationMode(FileContext, seInterface->EmbeddedSeId, eMode);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to set card emulation mode, %!STATUS!", status);
        //TRACE_LOG_NTSTATUS_ON_FAILURE(status);
        /*
        if (eMode == CardEmulationOff) {
            //
            // Stop the device interface to have a clean exit
            //
            NfcCxEmbeddedSEInterfaceStop(scInterface);
        }
        */

        goto Done;
    }

    /*
    if (eMode == CardEmulationOff) {
        TRACE_LINE(LEVEL_INFO, "Stop the eSE pcsc Interface and APDU Mode");
        NfcCxEmbeddedSEInterfaceStop(scInterface);
    }
    */

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}
#endif

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchAttributeAtr(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_SE_INTERFACE seInterface;
    PNFCCX_RF_INTERFACE rfInterface;
    seInterface = NfcCxFdoGetContext(ScInterface->FdoContext->Device)->SEInterface;
    rfInterface = NfcCxFdoGetContext(ScInterface->FdoContext->Device)->RFInterface;
    UNREFERENCED_PARAMETER(pbResultBuffer);
    UNREFERENCED_PARAMETER(cbResultBuffer);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Buffer size is validated at dispatch time
    _Analysis_assume_(DWORD_MAX >= *pcbOutputBuffer);

    WdfWaitLockAcquire(ScInterface->SmartCardLock, NULL);

    if (!seInterface->IsInWiredMode) {
        TRACE_LINE(LEVEL_ERROR, "SmartCard not connected");
        status = STATUS_INVALID_DEVICE_STATE;
        WdfWaitLockRelease(ScInterface->SmartCardLock);
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Get Attribute to start Output Buffer length is %Iu....", *pcbOutputBuffer);

    status = NfcCxRFInterfaceEmbeddedSEGetATRString(seInterface->FdoContext->RFInterface,
        (PBYTE)pbOutputBuffer,
        *pcbOutputBuffer);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Get Attribute failed, %!STATUS!", status);
        goto Done;
    }

    //RtlCopyMemory(pbOutputBuffer, seInterface->FdoContext->RFInterface->sReceiveBuffer.buffer, seInterface->FdoContext->RFInterface->sReceiveBuffer.length);
    *pcbOutputBuffer = rfInterface->pSeATRInfo.dwLength;

Done:
    WdfWaitLockRelease(ScInterface->SmartCardLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;

}

NTSTATUS
NfcCxEmbeddedSEInterfaceStart(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    )
    /*++

    Routine Description:

    Start the EmbeddedSE PCSC Interface

    Arguments:

    SEInterface - The EmbeddedSE PCSC Interface

    Return Value:

    NTSTATUS

    --*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext = ScInterface->FdoContext;
    WDF_DEVICE_INTERFACE_PROPERTY_DATA nfcEmbeddedSEReaderData;
    DECLARE_CONST_UNICODE_STRING(embeddedSeReference, EMBEDDED_SE_NAMESPACE);
    BYTE readerKind = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(ScInterface->SmartCardLock, NULL);

        //Check wihether the Smart card interface is published
        if (ScInterface->InterfaceCreated) {
            status = WdfDeviceCreateDeviceInterface(fdoContext->Device,
                (LPGUID)&GUID_DEVINTERFACE_SMARTCARD_READER,
                &embeddedSeReference);

            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to create the SmartCard Reader device interface, %!STATUS!", status);
                goto Done;
            }


            WDF_DEVICE_INTERFACE_PROPERTY_DATA_INIT(&nfcEmbeddedSEReaderData,
                &GUID_DEVINTERFACE_SMARTCARD_READER,
                &DEVPKEY_Device_ReaderKind);
            nfcEmbeddedSEReaderData.ReferenceString = &embeddedSeReference;
            readerKind = 5;
            status = WdfDeviceAssignInterfaceProperty(fdoContext->Device,
                &nfcEmbeddedSEReaderData,
                DEVPROP_TYPE_BYTE,
                sizeof(readerKind),
                &readerKind);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to assign property for the NFC smart card reader device interface, %!STATUS!", status);
                goto Done;
            }

            WdfDeviceSetDeviceInterfaceState(fdoContext->Device,
                (LPGUID)&GUID_DEVINTERFACE_SMARTCARD_READER,
                &embeddedSeReference,
                TRUE);
        }

    else {

        WdfDeviceSetDeviceInterfaceState(fdoContext->Device,
            (LPGUID)&GUID_DEVINTERFACE_SMARTCARD_READER,
            &embeddedSeReference,
            TRUE);
    }

        

    Done:
    WdfWaitLockRelease(ScInterface->SmartCardLock);
        TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

        return status;
    }


    VOID
        NfcCxEmbeddedSEInterfaceDestroy(
            _In_ PNFCCX_SC_INTERFACE ScInterface
            )
        /*++

        Routine Description:

        This routine cleans up the Embedded SE SmartCard Interface

        Arguments:

        Interface - A pointer to the SEInterface to cleanup.

        Return Value:

        None

        --*/
    {
        TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
        DECLARE_CONST_UNICODE_STRING(embeddedSeReference, EMBEDDED_SE_NAMESPACE);

        //
        // Since the lock and queue objects are parented to the device,
        // there are no needs to manually delete them here
        //
        if (ScInterface->InterfaceCreated) {

            //
            // Disable the eSE SmartCard Reader interface
            //
            WdfDeviceSetDeviceInterfaceState(ScInterface->FdoContext->Device,
                (LPGUID)&GUID_DEVINTERFACE_SMARTCARD_READER,
                &embeddedSeReference,
                FALSE);

            TRACE_LINE(LEVEL_VERBOSE, "SmartCard Reader interface disabled");
        }

        TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    }


    NTSTATUS
        NfcCxEmbeddedSEInterfaceStop(
            _In_ PNFCCX_SC_INTERFACE ScInterface
            )
        /*++

        Routine Description:

        Stop the EmbeddedSE SmartCard  Interface

        Arguments:

        SEInterface - The Secure Element  Interface

        Return Value:

        NTSTATUS

        --*/
    {
        NTSTATUS status = STATUS_SUCCESS;
        DECLARE_CONST_UNICODE_STRING(embeddedSeReference, EMBEDDED_SE_NAMESPACE);

        TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

        if (ScInterface->InterfaceCreated) {

            WdfDeviceSetDeviceInterfaceState(
                ScInterface->FdoContext->Device,
                (LPGUID)&GUID_DEVINTERFACE_SMARTCARD_READER,
                &embeddedSeReference,
                FALSE);
        }

        TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

        return status;
    }

NTSTATUS NfcCxEmbeddedSEInterfaceCopyResponseData(
        _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
        _In_ ULONG OutputBufferLength,
        _In_bytecount_(DataLength) PVOID Data,
        _In_ ULONG DataLength,
        _Out_ PULONG BufferUsed
        )
/*++

Routine Description:

Copies the response data into the transmit output buffer.

Arguments:

OutputBuffer - The Output buffer
OutputBufferLength - The output buffer length
Data - The response data buffer
DataLength - The response data buffer length
BufferUsed - A pointer to a ULONG to receive how many bytes of the output buffer was used.

Return Value:

NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG requiredBufferSize = 0;
    SCARD_IO_REQUEST outputRequest = { 0 };

    *BufferUsed = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // The output buffer has already been validated
    //
    _Analysis_assume_(sizeof(SCARD_IO_REQUEST) + 2 <= OutputBufferLength);

    //
    // The returning buffer should contains the SCARD_IO_REQUEST structure
    // followed by the response payload
    //
    outputRequest.dwProtocol = SCARD_PROTOCOL_T1;
    outputRequest.cbPciLength = sizeof(SCARD_IO_REQUEST);

    if (OutputBufferLength < sizeof(SCARD_IO_REQUEST)) {
        status = STATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    CopyMemory(OutputBuffer, &outputRequest, sizeof(SCARD_IO_REQUEST));
    *BufferUsed = sizeof(SCARD_IO_REQUEST);

    status = RtlULongAdd(DataLength, sizeof(SCARD_IO_REQUEST), &requiredBufferSize);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to calculate the required buffer size, %!STATUS!", status);
        goto Done;
    }

    if (OutputBufferLength < requiredBufferSize) {
        status = STATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    CopyMemory(((PUCHAR)OutputBuffer) + sizeof(SCARD_IO_REQUEST), Data, DataLength);
    *BufferUsed = requiredBufferSize;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}
NTSTATUS
NfcCxEmbeddedSEInterfaceAddClient(
    _In_ PNFCCX_SE_INTERFACE SeInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
    /*++

    Routine Description:

    This routine holds the reference of the client context

    Arguments:

    ScInterface - A pointer to the SeInterface
    FileContext - Client to add

    Return Value:

    NTSTATUS

    --*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (SeInterface->eSEPCSCCurrentClient != NULL) {
        TRACE_LINE(LEVEL_ERROR, "There is existing file handle on the eSE PCSC interface");
        status = STATUS_ACCESS_DENIED;
    }
    else {
        SeInterface->eSEPCSCCurrentClient = FileContext;
        TRACE_LINE(LEVEL_INFO, "eSE PCSC client = %p added", FileContext);
    }



    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
NfcCxEmbeddedSEInterfaceRemoveClient(
    _In_ PNFCCX_SE_INTERFACE SeInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
    /*++

    Routine Description:

    This routine release the reference  of the client

    Arguments:

    ScInterface - A pointer to the SeInterface

    Return Value:

    VOID

    --*/
{

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);


    if (SeInterface->eSEPCSCCurrentClient != FileContext) {
        TRACE_LINE(LEVEL_INFO, "eSE PCSC client Not removed, File context mismatch ");
        goto Done;
    }

    SeInterface->eSEPCSCCurrentClient = NULL;
    TRACE_LINE(LEVEL_INFO, "eSE PCSC client = %p removed", FileContext);

Done:

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchSetPower(
	_In_ WDFDEVICE Device,
	_In_ WDFREQUEST Request,
	_In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
	_In_ size_t InputBufferLength,
	_Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
	_In_ size_t OutputBufferLength
)
/*++

Routine Description:

This routine dispatches the eSE Set Power

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
	PNFCCX_FILE_CONTEXT fileContext;
	DWORD *pdwPower = (DWORD*)InputBuffer;
	NFCCX_SE_INTERFACE* seInterface = NULL;
	CARD_EMULATION_MODE_INTERNAL eMode;

	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

	//
	// Buffer validated by the validation method
	//
	_Analysis_assume_(sizeof(DWORD) <= InputBufferLength);

	seInterface = NfcCxFdoGetContext(Device)->SEInterface;
	fileContext = NfcCxFileGetContext(WdfRequestGetFileObject(Request));
	seInterface = NfcCxFileObjectGetFdoContext(fileContext)->SEInterface;



	if (!seInterface->IsInWiredMode) {
		TRACE_LINE(LEVEL_ERROR, "eSE not in Wired Mode");
		status = STATUS_NO_MEDIA;
		goto Done;
	}

	switch (*pdwPower)
	{
	case SCARD_COLD_RESET:
	case SCARD_WARM_RESET:
		TRACE_LINE(LEVEL_ERROR, "eSE SCARD cold or warm reset");

		eMode = CardEmulationOff;
		status = NfcCxSEInterfaceSetCardEmulationMode(fileContext, seInterface->EmbeddedSeId, eMode);

		if (!NT_SUCCESS(status)) {
			TRACE_LINE(LEVEL_ERROR, "Failed to set card emulation mode to off, %!STATUS!", status);
			TRACE_LOG_NTSTATUS_ON_FAILURE(status);
			goto Done;
		}

		eMode = EmbeddedSeApduMode;
		status = NfcCxSEInterfaceSetCardEmulationMode(fileContext, seInterface->EmbeddedSeId, eMode);
		if (!NT_SUCCESS(status)) {
			TRACE_LINE(LEVEL_ERROR, "Failed to set card emulation mode to APDU mode, %!STATUS!", status);
			TRACE_LOG_NTSTATUS_ON_FAILURE(status);
			goto Done;
		}
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
