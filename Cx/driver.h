/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    Driver.c

Abstract:

    Base driver declarations for the NFC Class Extension

Environment:

    User-mode Driver Framework

--*/
#pragma once

#define PWUDF_MODULE PVOID

#define GLOBALS_SIG     'CCFN'

typedef struct _NFCCX_CLIENT_GLOBALS {
    //
    // Equal to GLOBALS_SIG
    //
    ULONG Signature;

    //
    // Client driver configuration
    //
    NFC_CX_CLIENT_CONFIG Config;

    //
    // Public part of the globals
    //
    NFCCX_DRIVER_GLOBALS Public;

} NFCCX_CLIENT_GLOBALS, *PNFCCX_CLIENT_GLOBALS;

PNFCCX_CLIENT_GLOBALS
FORCEINLINE
GetPrivateGlobals(
    PNFCCX_DRIVER_GLOBALS PublicGlobals
    )
{
    return CONTAINING_RECORD(PublicGlobals,
                             NFCCX_CLIENT_GLOBALS,
                             Public);
}

BOOLEAN
FORCEINLINE
VerifyPrivateGlobals(
    PNFCCX_DRIVER_GLOBALS PublicGlobals
    )
{
    return (GetPrivateGlobals(PublicGlobals))->Signature == GLOBALS_SIG;
}

WDF_EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD NfcCxEvtDriverUnload;

VOID NfcCxDeviceSetFailed(
    _In_ WDFDEVICE Device
    );

//
// Class extension binding
//
NTSTATUS
NfcCxInitialize(
    VOID
    );

VOID
NfcCxDeinitialize(
    VOID
    );

NTSTATUS
NfcCxBindClient(
    _In_ PVOID ClassBindInfo,
    _In_ ULONG FunctionTableCount,
    _Out_writes_(FunctionTableCount) PFN_WDF_CLASS_EXPORT* FunctionTable
    );

VOID
NfcCxUnbindClient(
    _In_ PVOID ClassBindInfo
    );

//
// Class extension exported methods
//
NTSTATUS
NfcCxEvtDeviceInitConfig(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ PWDFDEVICE_INIT DeviceInit,
    _In_ PNFC_CX_CLIENT_CONFIG Config
    );

NTSTATUS
NfcCxEvtDeviceInitialize(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device
    );

NTSTATUS
NfcCxEvtDeviceDeinitialize(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device
    );

NTSTATUS
NfcCxEvtHardwareEvent(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ PNFC_CX_HARDWARE_EVENT NciCxHardwareEventParams
    );

NTSTATUS
NfcCxEvtNciReadNotification(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE    Device,
    _In_ WDFMEMORY    Memory
    );

NTSTATUS
NfcCxEvtSetRfDiscoveryConfig(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ PCNFC_CX_RF_DISCOVERY_CONFIG Config
    );

NTSTATUS
NfcCxEvtSetLlcpConfig(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ PCNFC_CX_LLCP_CONFIG Config
    );

NTSTATUS
NfcCxEvtRegisterSequenceHandler(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ NFC_CX_SEQUENCE Sequence,
    _In_ PFN_NFC_CX_SEQUENCE_HANDLER EvtNfcCxSequenceHandler
    );

NTSTATUS
NfcCxEvtUnregisterSequenceHandler(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ NFC_CX_SEQUENCE Sequence
    );

NTSTATUS
NfcCxEvtReleaseHardwareControl(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device
);

NTSTATUS
NfcCxEvtReacquireHardwareControl(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device
);

WDF_EXTERN_C_END
