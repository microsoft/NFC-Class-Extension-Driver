/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    power.h

Abstract:

    Cx Device power utilities declaration.
    
Environment:

    User-mode Driver Framework

--*/

#pragma once

enum NFC_CX_POWER_RF_STATE : LONG
{
    // Values are bit flags
    NfcCxPowerRfState_None = 0x0,
    NfcCxPowerRfState_ProximityEnabled = 0x1,
    NfcCxPowerRfState_CardEmulationEnabled = 0x2,
    NfcCxPowerRfState_StealthListenEnabled = 0x4,
    NfcCxPowerRfState_NoListenEnabled = 0x8,
    NfcCxPowerRfState_ESeEnabled = 0x10,
};

enum NFC_CX_POWER_REFERENCE_TYPE
{
    // Note: These values are indexes into the 'PowerReferences' array.
    NfcCxPowerReferenceType_Proximity = 0,
    NfcCxPowerReferenceType_CardEmulation,
    NfcCxPowerReferenceType_StealthListen,
    NfcCxPowerReferenceType_NoListen,
    NfcCxPowerReferenceType_ESe,
    // Max count
    NfcCxPowerReferenceType_MaxCount,
};

typedef struct _NFCCX_POWER_MANAGER
{
    PNFCCX_FDO_CONTEXT FdoContext;

    WDFWAITLOCK WaitLock;

    LONG PowerReferences[NfcCxPowerReferenceType_MaxCount];

    BOOLEAN NfpRadioState;
    BOOLEAN SERadioState;

    BOOLEAN DeviceStopIdle;             // TRUE == 'WdfDeviceStopIdle' has been called.

    WDFTIMER UpdateRFStateTimer;

} NFCCX_POWER_MANAGER, *PNFCCX_POWER_MANAGER;

NTSTATUS
NfcCxPowerCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Outptr_ PNFCCX_POWER_MANAGER* PPPowerManager
    );

VOID
NfcCxPowerDestroy(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

NTSTATUS
NfcCxPowerStart(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

VOID
NfcCxPowerStop(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

BOOLEAN
NfcCxPowerShouldStopIdle(
    _In_ NFC_CX_POWER_RF_STATE RfState
    );

BOOLEAN
NfcCxPowerShouldEnableDiscovery(
    _In_ NFC_CX_POWER_RF_STATE RfState
    );

NTSTATUS
NfcCxPowerFileAddReference(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ NFC_CX_POWER_REFERENCE_TYPE Type
    );

NTSTATUS
NfcCxPowerFileRemoveReference(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ NFC_CX_POWER_REFERENCE_TYPE Type
    );

NFC_CX_POWER_RF_STATE
NfcCxPowerGetRfState(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

void
NfcCxPowerCleanupFilePolicyReferences(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

NTSTATUS
NfcCxPowerSetRadioState(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PNFCRM_SET_RADIO_STATE RadioState
    );

NTSTATUS
NfcCxPowerQueryRadioState(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _Out_ PNFCRM_RADIO_STATE RadioState
    );

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerAcquireFdoContextReferenceLocked(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ NFC_CX_POWER_REFERENCE_TYPE Type
    );

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerReleaseFdoContextReferenceLocked(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ NFC_CX_POWER_REFERENCE_TYPE Type
    );

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerDeviceStopIdle(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

_Requires_lock_held_(PowerManager->WaitLock)
void
NfcCxPowerDeviceResumeIdle(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

VOID
NfcCxPowerUpdateRFPollingState(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ BOOLEAN PoweringUpRF
    );

VOID
NfcCxPowerUpdateRFPollingStateWorker(
    _In_ WDFTIMER Timer
    );

BOOLEAN
NfcCxPowerIsAllowedNfp(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

BOOLEAN
NfcCxPowerIsAllowedSE(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

FN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED  NfcCxPowerIsIoctlSupported;
FN_NFCCX_DDI_MODULE_IODISPATCH  NfcCxPowerIoDispatch;
