/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxSCCommon.cpp

Abstract:

    SmartCard Common functions/types.

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxSCCommon.tmh"

static const CHAR SCReaderVendorName[] = "Microsoft";
static const DWORD SCReaderVendorIfdVersion = ((IFD_MAJOR_VER & 0x3) << 6) | ((IFD_MINOR_VER & 0x3) << 4) | (IFD_BUILD_NUM & 0xF);
static const DWORD SCReaderProtocolTypes = SCARD_PROTOCOL_T1;
static const DWORD SCReaderDeviceUnit = 0;
static const DWORD SCReaderDefaultClk = 13560;
static const DWORD SCReaderMaxClk = 13560;
static const DWORD SCReaderDefaultDataRate = 1;
static const DWORD SCReaderMaxDataRate = 1;
static const DWORD SCReaderMaxIfsd = 254;
static const DWORD SCReaderCharacteristics = SCARD_READER_CONTACTLESS;
static const DWORD SCReaderCurrentClk = 13560;
static const DWORD SCReaderCurrentD = 1;
static const DWORD SCReaderCurrentIfsc = 32;
static const DWORD SCReaderCurrentIfsd = 254;
static const DWORD SCReaderCurrentBwt = 4;

typedef struct _ATTRIBUTE_DISPATCH_ENTRY {
    DWORD dwAttributeId;
    PBYTE pbResultBuffer;
    size_t cbResultBuffer;
} ATTRIBUTE_DISPATCH_ENTRY, *PATTRIBUTE_DISPATCH_ENTRY;

static const ATTRIBUTE_DISPATCH_ENTRY
g_ScCommonAttributes [] =
{
    { SCARD_ATTR_VENDOR_NAME,               (PBYTE)SCReaderVendorName,           sizeof(SCReaderVendorName),        },
    { SCARD_ATTR_VENDOR_IFD_VERSION,        (PBYTE)&SCReaderVendorIfdVersion,    sizeof(SCReaderVendorIfdVersion),  },
    { SCARD_ATTR_PROTOCOL_TYPES,            (PBYTE)&SCReaderProtocolTypes,       sizeof(SCReaderProtocolTypes),     },
    { SCARD_ATTR_DEVICE_UNIT,               (PBYTE)&SCReaderDeviceUnit,          sizeof(SCReaderDeviceUnit),        },
    { SCARD_ATTR_DEFAULT_CLK,               (PBYTE)&SCReaderDefaultClk,          sizeof(SCReaderDefaultClk),        },
    { SCARD_ATTR_MAX_CLK,                   (PBYTE)&SCReaderMaxClk,              sizeof(SCReaderMaxClk),            },
    { SCARD_ATTR_DEFAULT_DATA_RATE,         (PBYTE)&SCReaderDefaultDataRate,     sizeof(SCReaderDefaultDataRate),   },
    { SCARD_ATTR_MAX_DATA_RATE,             (PBYTE)&SCReaderMaxDataRate,         sizeof(SCReaderMaxDataRate),       },
    { SCARD_ATTR_MAX_IFSD,                  (PBYTE)&SCReaderMaxIfsd,             sizeof(SCReaderMaxIfsd),           },
    { SCARD_ATTR_CHARACTERISTICS,           (PBYTE)&SCReaderCharacteristics,     sizeof(SCReaderCharacteristics),   },
    { SCARD_ATTR_CURRENT_CLK,               (PBYTE)&SCReaderCurrentClk,          sizeof(SCReaderCurrentClk),        },
    { SCARD_ATTR_CURRENT_D,                 (PBYTE)&SCReaderCurrentD,            sizeof(SCReaderCurrentD),          },
    { SCARD_ATTR_CURRENT_IFSC,              (PBYTE)&SCReaderCurrentIfsc,         sizeof(SCReaderCurrentIfsc),       },
    { SCARD_ATTR_CURRENT_IFSD,              (PBYTE)&SCReaderCurrentIfsd,         sizeof(SCReaderCurrentIfsd),       },
    { SCARD_ATTR_CURRENT_BWT,               (PBYTE)&SCReaderCurrentBwt,          sizeof(SCReaderCurrentBwt),        },
};

NTSTATUS
NfcCxSCCommonGetAttribute(
    _In_ DWORD AttributeId,
    _Out_writes_bytes_to_(*OutputBufferLength, *OutputBufferLength) PVOID OutputBuffer,
    _Inout_ size_t* OutputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    bool found = false;
    for (const ATTRIBUTE_DISPATCH_ENTRY& tableEntry : g_ScCommonAttributes)
    {
        if (tableEntry.dwAttributeId == AttributeId)
        {
            status = NfcCxCopyToBuffer(tableEntry.pbResultBuffer, tableEntry.cbResultBuffer, (BYTE*)OutputBuffer, OutputBufferLength);
            if (!NT_SUCCESS(status))
            {
                goto Done;
            }

            found = true;
            break;
        }
    }

    if (!found)
    {
        status = STATUS_NOT_SUPPORTED;
        TRACE_LINE(LEVEL_INFO, "Unknown smartcard attribute: 0x%x", AttributeId);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSCCommonDispatchNotSupported(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine returns not supported status code

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
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NfcCxSCCommonDispatchGetLastError(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine returns success status code

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
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(NULL != OutputBuffer);
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    *((DWORD*)OutputBuffer) = STATUS_SUCCESS;
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(DWORD));

    return STATUS_PENDING;
}

NTSTATUS
NfcCxSCCommonCopyTrasmitResponseData(
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
    SCARD_IO_REQUEST outputRequest = {0};

    *BufferUsed = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // The output buffer has already been validated
    //
    _Analysis_assume_(sizeof(SCARD_IO_REQUEST)+2 <= OutputBufferLength);

    //
    // The returning buffer should contains the SCARD_IO_REQUEST structure
    // followed by the response payload
    //
    outputRequest.dwProtocol = SCReaderCurrentProtocolType;
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
