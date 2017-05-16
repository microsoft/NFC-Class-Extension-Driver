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

typedef struct _NCI_POWER_POLICY {
    BOOLEAN CanPowerDown;
}NCI_POWER_POLICY, *PNCI_POWER_POLICY;

NTSTATUS
NfcCxPowerFdoInitialize(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    );

NTSTATUS
NfcCxPowerSetPolicy(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PNCI_POWER_POLICY PowerPolicy
    );

NTSTATUS
NfcCxPowerCleanupFilePolicyReferences(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

NTSTATUS
NfcCxPowerSetRadioState(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PNFCRM_SET_RADIO_STATE RadioState
    );

NTSTATUS
NfcCxPowerQueryRadioState(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _Out_ PNFCRM_RADIO_STATE RadioState
    );

_Requires_lock_held_(FdoContext->PowerPolicyWaitLock)
NTSTATUS
NfcCxPowerAcquireFdoContextReferenceLocked(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _Out_ BOOLEAN* pReferenceTaken
    );

_Requires_lock_held_(FdoContext->PowerPolicyWaitLock)
NTSTATUS
NfcCxPowerReleaseFdoContextReferenceLocked(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

_Requires_lock_held_(FdoContext->PowerPolicyWaitLock)
NTSTATUS
NfcCxPowerDeviceStopIdle(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ BOOLEAN WaitForD0
    );

_Requires_lock_held_(FdoContext->PowerPolicyWaitLock)
void
NfcCxPowerDeviceResumeIdle(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    );

BOOLEAN
NfcCxPowerShouldDeviceStopIdle(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    );

NTSTATUS
NfcCxPowerUpdateRFPollingState(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    );

BOOLEAN
NfcCxPowerIsAllowedNfp(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    );

BOOLEAN
NfcCxPowerIsAllowedSE(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    );

FN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED  NfcCxPowerIsIoctlSupported;
FN_NFCCX_DDI_MODULE_IODISPATCH  NfcCxPowerIoDispatch;
