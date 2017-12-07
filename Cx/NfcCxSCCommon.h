/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxSCCommon.h

Abstract:

    SmartCard Common functions/types.

Environment:

    User-mode Driver Framework

--*/

#pragma once

typedef
NTSTATUS
NFCCX_SC_DISPATCH_HANDLER(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

typedef NFCCX_SC_DISPATCH_HANDLER *PFN_NFCCX_SC_DISPATCH_HANDLER;

// {D6B5B883-18BD-4B4D-B2EC-9E38AFFEDA82}, 2, DEVPROP_TYPE_BYTE
DEFINE_DEVPROPKEY(DEVPKEY_Device_ReaderKind, 0xD6B5B883, 0x18BD, 0x4B4D, 0xB2, 0xEC, 0x9E, 0x38, 0xAF, 0xFE, 0xDA, 0x82, 0x02);

// {D6B5B883-18BD-4B4D-B2EC-9E38AFFEDA82}, D, DEVPROP_TYPE_GUID
DEFINE_DEVPROPKEY(DEVPKEY_Device_ReaderSecureElementId,
0xD6B5B883, 0x18BD, 0x4B4D, 0xB2, 0xEC, 0x9E, 0x38, 0xAF, 0xFE, 0xDA, 0x82, 0x0D);

static const DWORD SCReaderCurrentProtocolType = SCARD_PROTOCOL_T1;

NTSTATUS
NfcCxSCCommonGetAttribute(
    _In_ DWORD AttributeId,
    _Out_writes_bytes_to_(*OutputBufferLength, *OutputBufferLength) PVOID OutputBuffer,
    _Inout_ size_t* OutputBufferLength);

NFCCX_SC_DISPATCH_HANDLER NfcCxSCCommonDispatchNotSupported;
NFCCX_SC_DISPATCH_HANDLER NfcCxSCCommonDispatchGetLastError;

NTSTATUS
NfcCxSCCommonCopyTrasmitResponseData(
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ ULONG DataLength,
    _Out_ PULONG BufferUsed
    );
