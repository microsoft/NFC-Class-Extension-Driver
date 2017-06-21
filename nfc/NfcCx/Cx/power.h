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

typedef struct _NFCCX_POWER_MANAGER
{
    PNFCCX_FDO_CONTEXT FdoContext;

    WDFWAITLOCK WaitLock;

    LONG NfpPowerPolicyReferences;
    BOOLEAN NfpRadioState;

    LONG SEPowerPolicyReferences;
    BOOLEAN SERadioState;

    BOOLEAN DeviceStopIdle;             // TRUE == 'WdfDeviceStopIdle' has been called.

} NFCCX_POWER_MANAGER, *PNFCCX_POWER_MANAGER;

typedef struct _NCI_POWER_POLICY {
    BOOLEAN CanPowerDown;
} NCI_POWER_POLICY, *PNCI_POWER_POLICY;

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

NTSTATUS
NfcCxPowerSetPolicy(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PNCI_POWER_POLICY PowerPolicy
    );

NTSTATUS
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
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _Out_ BOOLEAN* pReferenceTaken
    );

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerReleaseFdoContextReferenceLocked(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerDeviceStopIdle(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ BOOLEAN WaitForD0
    );

_Requires_lock_held_(PowerManager->WaitLock)
void
NfcCxPowerDeviceResumeIdle(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

BOOLEAN
NfcCxPowerShouldDeviceStopIdle(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    );

NTSTATUS
NfcCxPowerUpdateRFPollingState(
    _In_ PNFCCX_POWER_MANAGER PowerManager
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
