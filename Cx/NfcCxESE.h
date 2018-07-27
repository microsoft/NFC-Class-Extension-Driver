/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxESE.h

Abstract:

    eSE Interface declaration

Environment:

    User-mode Driver Framework

--*/

#pragma once

#include "NfcCxSCCommon.h"

#define DEFAULT_HCI_TX_RX_TIME_OUT      5000U /* 5 Secs */

typedef struct _NFCCX_ESE_INTERFACE
{
    //
    // Back link to the FDO context
    //
    PNFCCX_FDO_CONTEXT FdoContext;

    //
    // Interface Created
    //
    BOOLEAN InterfaceCreated;

    //
    // SE ID
    //
    GUID SecureElementId;

    //
    // Present / Absent
    //
    NFCCX_SC_PRESENT_ABSENT_DISPATCHER PresentDispatcher;
    NFCCX_SC_PRESENT_ABSENT_DISPATCHER AbsentDispatcher;

    //
    // SmartCard Connection state
    //
    WDFWAITLOCK SmartCardLock;

    _Guarded_by_(SmartCardLock)
    BOOLEAN SmartCardConnected;

    //
    // Reference count for exclusive file handle
    //
    _Guarded_by_(SmartCardLock)
    PNFCCX_FILE_CONTEXT CurrentClient;

    BOOLEAN ClientPowerReferenceHeld;

} NFCCX_ESE_INTERFACE, *PNFCCX_ESE_INTERFACE;

NTSTATUS
NfcCxESEInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT DeviceContext,
    _Outptr_ PNFCCX_ESE_INTERFACE * ESEInterface
    );

VOID
NfcCxESEInterfaceDestroy(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface
    );

NTSTATUS
NfcCxESEInterfaceStart(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface
    );

VOID
NfcCxESEInterfaceStop(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface
    );

FN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED
NfcCxESEInterfaceIsIoctlSupported;

FN_NFCCX_DDI_MODULE_IODISPATCH
NfcCxESEInterfaceIoDispatch;

//
// Reference count in the fileObject providing exclusive file handling
//

NTSTATUS
NfcCxESEInterfaceAddClient(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID
NfcCxESEInterfaceRemoveClient(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

//
// Handling methods below
//

VOID
NfcCxESEInterfaceHandleSmartCardConnectionEstablished(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface,
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

VOID
NfcCxESEInterfaceHandleSmartCardConnectionLost(
    _In_ PNFCCX_ESE_INTERFACE ESEInterface
    );

NTSTATUS
NfcCxESEInterfaceValidateRequest(
    _In_ ULONG        IoControlCode,
    _In_ size_t       InputBufferLength,
    _In_ size_t       OutputBufferLength
    );

NTSTATUS
NfcCxESEInterfaceDispatchRequest(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

//
// Dispatch methods below
//
NFCCX_SC_DISPATCH_HANDLER NfcCxESEInterfaceDispatchGetAttribute;
NFCCX_SC_DISPATCH_HANDLER NfcCxESEInterfaceDispatchSetAttribute;
NFCCX_SC_DISPATCH_HANDLER NfcCxESEInterfaceDispatchGetState;
NFCCX_SC_DISPATCH_HANDLER NfcCxESEInterfaceDispatchSetPower;
NFCCX_SC_DISPATCH_HANDLER NfcCxESEInterfaceDispatchSetProtocol;
NFCCX_SC_DISPATCH_HANDLER NfcCxESEInterfaceDispatchIsAbsent;
NFCCX_SC_DISPATCH_HANDLER NfcCxESEInterfaceDispatchIsPresent;
NFCCX_SC_DISPATCH_HANDLER NfcCxESEInterfaceDispatchTransmit;

//
// Dispatched attribute methods below
//
NTSTATUS
NfcCxESEInterfaceDispatchAttributePresent(
    _In_ PNFCCX_ESE_INTERFACE ScInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxESEInterfaceDispatchAttributeCurrentProtocolType(
    _In_ PNFCCX_ESE_INTERFACE ScInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

_Requires_lock_not_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxESEInterfaceDispatchAttributeAtr(
    _In_ PNFCCX_ESE_INTERFACE ScInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxESEInterfaceDispatchAttributeIccType(
    _In_ PNFCCX_ESE_INTERFACE ScInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );
