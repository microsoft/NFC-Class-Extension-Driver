/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    power.cpp

Abstract:

    Cx Device power utilities implementation.
    
Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "power.tmh"

static const LONGLONG c_DefaultRFUpdateWaitTime = WDF_REL_TIMEOUT_IN_SEC(1);

static bool IsPowerRFManagementEnabled(_In_ PNFCCX_FDO_CONTEXT FdoContext)
{
    // We only manage RF radio when device is in NCI mode.
    return NFC_CX_DEVICE_MODE_NCI == FdoContext->NfcCxClientGlobal->Config.DeviceMode;
}

NTSTATUS
NfcCxPowerCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Outptr_ PNFCCX_POWER_MANAGER* PPPowerManager
    )
/*++

Routine Description:

   Creates a power manager object. (Called when device is initializing for the first time.)

Arguments:

    FdoContext - Pointer to the FDO Context
    PPPowerManager - Returns a pointer to the new power manager object

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    PNFCCX_POWER_MANAGER powerManager = (PNFCCX_POWER_MANAGER)malloc(sizeof(NFCCX_POWER_MANAGER));
    if (powerManager == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the power manager, %!STATUS!", status);
        goto Done;
    }

    *powerManager = {};
    powerManager->FdoContext = FdoContext;
    powerManager->NfpRadioState = TRUE;

    if (IsPowerRFManagementEnabled(FdoContext))
    {
        // Check the currently required power state
        NfcCxPowerReadRadioStateFromRegistry(powerManager);
    }

    WDF_OBJECT_ATTRIBUTES objectAttrib;
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = FdoContext->Device;

    status = WdfWaitLockCreate(&objectAttrib, &powerManager->WaitLock);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the WaitLock, %!STATUS!", status);
        goto Done;
    }

    WDF_TIMER_CONFIG timerConfig;
    WDF_TIMER_CONFIG_INIT(&timerConfig, NfcCxPowerUpdateRFPollingStateWorker);
    timerConfig.AutomaticSerialization = false;

    WDF_OBJECT_ATTRIBUTES timerAttib;
    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttib);
    timerAttib.ParentObject = FdoContext->Device;

    status = WdfTimerCreate(&timerConfig, &timerAttib, &powerManager->UpdateRFStateTimer);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to create threadppol work for RF update. %!STATUS!", status);
        goto Done;
    }


    *PPPowerManager = powerManager;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxPowerDestroy(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
/*++

Routine Description:

   Called when device is shutting down.

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Ensure all the Update RF State callbacks are finished.
    if (PowerManager->UpdateRFStateTimer != nullptr)
    {
        WdfTimerStop(PowerManager->UpdateRFStateTimer, /*wait*/ TRUE);
    }

    // Free memory
    free(PowerManager);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxPowerStart(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
/*++

Routine Description:

   Called when device is entering the D0 power state (either first initialiation or resuming from D3).

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (IsPowerRFManagementEnabled(PowerManager->FdoContext))
    {
        // Check to see if we have any active interface handles.
        // If we do, it probably means the system has just awoken from sleep.
        NFC_CX_POWER_RF_STATE rfState = NfcCxPowerGetRfState(PowerManager);
        if (NfcCxPowerShouldEnableDiscovery(rfState))
        {
            // Ensure RF radio is turned on.
            NfcCxPowerUpdateRFPollingState(PowerManager, /*PoweringUpRF*/ TRUE);
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxPowerStop(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
/*++

Routine Description:

   Called when device is stopping, either because it is entering D3 or shutting down.

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:

    VOID

--*/
{
    // Ensure all the Update RF State callbacks are finished.
    if (PowerManager->UpdateRFStateTimer != nullptr)
    {
        WdfTimerStop(PowerManager->UpdateRFStateTimer, /*wait*/ TRUE);
    }
}

NTSTATUS
NfcCxPowerFileAddRemoveReference(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ NFC_CX_POWER_REFERENCE_TYPE Type,
    _In_ BOOLEAN AddReference
    )
{
    if (AddReference)
    {
        return NfcCxPowerFileAddReference(PowerManager, FileContext, Type);
    }
    else
    {
        return NfcCxPowerFileRemoveReference(PowerManager, FileContext, Type);
    }
}

NTSTATUS
NfcCxPowerFileAddReference(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ NFC_CX_POWER_REFERENCE_TYPE Type
    )
/*++

Routine Description:

   Acquires a power reference for a file handle.

Arguments:

    PowerManager - Pointer to the Power Manager
    FileContext - Pointer to the file object context
    Type - The type of power reference being acquired

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    WdfWaitLockAcquire(PowerManager->WaitLock, NULL);

    LONG& filePowerReference = FileContext->PowerReferences[Type];

    if (MAXLONG == filePowerReference)
    {
        // About to overflow the Policy references
        status = STATUS_INTEGER_OVERFLOW;
        TRACE_LINE(LEVEL_ERROR, "File power policy references overflow (%d), %!STATUS!", Type, status);
        goto Done;
    }

    filePowerReference += 1;

    if (filePowerReference == 1)
    {
        status = NfcCxPowerAcquireFdoContextReferenceLocked(PowerManager, Type);
        if (!NT_SUCCESS(status))
        {
            filePowerReference -= 1;
            TRACE_LINE(LEVEL_VERBOSE, "NfcCxPowerAcquireFdoContextReferenceLocked failed, %!STATUS!", status);
            goto Done;
        }
    }

    TRACE_LINE(LEVEL_INFO, "File power reference acquired: Type=%d, RefCount=%d", Type, filePowerReference);

Done:
    WdfWaitLockRelease(PowerManager->WaitLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxPowerFileRemoveReference(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ NFC_CX_POWER_REFERENCE_TYPE Type
    )
/*++

Routine Description:

   Releases a power reference for a file handle.

Arguments:

    PowerManager - Pointer to the Power Manager
    FileContext - Pointer to the file object context
    Type - The type of power reference being released

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    WdfWaitLockAcquire(PowerManager->WaitLock, NULL);

    LONG& filePowerReference = FileContext->PowerReferences[Type];

    if (filePowerReference <= 0)
    {
        // About to underflow the file handle's power policy references
        NT_ASSERT(false);

        status = STATUS_INTEGER_OVERFLOW;
        TRACE_LINE(LEVEL_ERROR, "File power policy references underflow (%d), %!STATUS!", Type, status);

        TraceLoggingWrite(
            g_hNfcCxProvider,
            "FileHandlePowerRefCountUnderflow",
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingInt32(Type, "powerReferenceType"),
            TraceLoggingInt32(FileContext->Role, "powerHandleRole"));

        NfcCxDeviceSetFailed(PowerManager->FdoContext->Device);
        goto Done;
    }

    filePowerReference -= 1;

    if (filePowerReference == 0)
    {
        status = NfcCxPowerReleaseFdoContextReferenceLocked(PowerManager, Type);
        if (!NT_SUCCESS(status))
        {
            TRACE_LINE(LEVEL_VERBOSE, "NfcCxPowerReleaseFdoContextReferenceLocked failed, %!STATUS!", status);
            goto Done;
        }
    }

    TRACE_LINE(LEVEL_INFO, "File power reference released: Type=%d, RefCount=%d", Type, filePowerReference);

Done:
    WdfWaitLockRelease(PowerManager->WaitLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NFC_CX_POWER_RF_STATE
NfcCxPowerGetRfState(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
/*++

Routine Description:

   Returns whether or not NFP (Proximity) and/or SE (card emulation) should be enabled by the radio (RF).

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:

    NFC_CX_POWER_RF_STATE

--*/
{
    NFC_CX_POWER_RF_STATE result = NfcCxPowerRfState_None;

    // Note: The values of all these variables do not provide any guarantees about the state of any other memory within the driver.
    // Hence no memory fence is required.

    if (PowerManager->NfpRadioState && (PowerManager->PowerReferences[NfcCxPowerReferenceType_Proximity] != 0))
    {
        result = (NFC_CX_POWER_RF_STATE)(result | NfcCxPowerRfState_ProximityEnabled);
    }

    if (PowerManager->NfpRadioState && (PowerManager->PowerReferences[NfcCxPowerReferenceType_NoListen] != 0))
    {
        result = (NFC_CX_POWER_RF_STATE)(result | NfcCxPowerRfState_NoListenEnabled);
    }

    if (PowerManager->PowerReferences[NfcCxPowerReferenceType_CardEmulation] != 0)
    {
        result = (NFC_CX_POWER_RF_STATE)(result | NfcCxPowerRfState_CardEmulationEnabled);
    }

    if (PowerManager->PowerReferences[NfcCxPowerReferenceType_StealthListen] != 0)
    {
        result = (NFC_CX_POWER_RF_STATE)(result | NfcCxPowerRfState_StealthListenEnabled);
    }

    if (PowerManager->PowerReferences[NfcCxPowerReferenceType_ESe] != 0)
    {
        result = (NFC_CX_POWER_RF_STATE)(result | NfcCxPowerRfState_ESeEnabled);
    }

    return result;
}

BOOLEAN
NfcCxPowerShouldStopIdle(
    _In_ NFC_CX_POWER_RF_STATE RfState
    )
{
    // NoListen disables the RF instead of enabling it.
    // So it shouldn't be considered when determining if the NFC Controller should be powered on or off.
    return (RfState & ~NfcCxPowerRfState_NoListenEnabled) != NfcCxPowerRfState_None;
}

BOOLEAN
NfcCxPowerShouldEnableDiscovery(
    _In_ NFC_CX_POWER_RF_STATE RfState
    )
{
    // NoListen fully disables CardEmulation and StealthListen.
    bool listenEnabled = !(NfcCxPowerRfState_NoListenEnabled & RfState) &&
        ((NfcCxPowerRfState_CardEmulationEnabled & RfState) ||
         (NfcCxPowerRfState_StealthListenEnabled & RfState));

    bool proximityEnabled = (NfcCxPowerRfState_ProximityEnabled & RfState);

    // Return 'true' if we should enable RF discovery.
    return listenEnabled || proximityEnabled;
}

static BOOLEAN
NfcCxPowerHasDiscoveryConfigChanged(
    _In_ NFC_CX_POWER_RF_STATE OldRfState,
    _In_ NFC_CX_POWER_RF_STATE NewRfState)
{
    // ESe doesn't affect the discovery config.
    NFC_CX_POWER_RF_STATE oldRefStateSansESe = NFC_CX_POWER_RF_STATE(OldRfState & ~NfcCxPowerRfState_ESeEnabled);
    NFC_CX_POWER_RF_STATE newRefStateSansESe = NFC_CX_POWER_RF_STATE(NewRfState & ~NfcCxPowerRfState_ESeEnabled);

    return oldRefStateSansESe != newRefStateSansESe;
}

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerAcquireFdoContextReferenceLocked(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ NFC_CX_POWER_REFERENCE_TYPE Type
    )
/*++

Routine Description:

   NfcCxPowerAcquireFdoContextReferenceLocked acquires
   an FDO power policy reference.

   Callers of this function must hold the Power.WaitLock.

Arguments:

    PowerManager - Pointer to the Power Manager
    Type - The type of power reference being acquired

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    LONG& powerReference = PowerManager->PowerReferences[Type];

    if (MAXLONG == powerReference)
    {
        status = STATUS_INTEGER_OVERFLOW;
        TRACE_LINE(LEVEL_ERROR, "Power policy references overflow (%d), %!STATUS!", Type, status);
        goto Done;
    }

    NFC_CX_POWER_RF_STATE oldRfState = NfcCxPowerGetRfState(PowerManager);

    powerReference += 1;

    NFC_CX_POWER_RF_STATE newRfState = NfcCxPowerGetRfState(PowerManager);
    if (NfcCxPowerShouldStopIdle(newRfState))
    {
        // Ensure device is powered up.
        status = NfcCxPowerDeviceStopIdle(PowerManager);
        if (!NT_SUCCESS(status))
        {
            TRACE_LINE(LEVEL_ERROR, "NfcCxPowerDeviceStopIdle failed, %!STATUS!", status);
            powerReference -= 1;
            goto Done;
        }
    }

    // Check if we need to update the RF config.
    if (NfcCxPowerHasDiscoveryConfigChanged(oldRfState, newRfState))
    {
        NfcCxPowerUpdateRFPollingState(PowerManager, /*PoweringUpRF*/ TRUE);
    }

    TRACE_LINE(LEVEL_INFO, "Current power references: NFP=%d, CE=%d, SL=%d, NL=%d, eSE=%d",
        PowerManager->PowerReferences[NfcCxPowerReferenceType_Proximity],
        PowerManager->PowerReferences[NfcCxPowerReferenceType_CardEmulation],
        PowerManager->PowerReferences[NfcCxPowerReferenceType_StealthListen],
        PowerManager->PowerReferences[NfcCxPowerReferenceType_NoListen],
        PowerManager->PowerReferences[NfcCxPowerReferenceType_ESe]);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerReleaseFdoContextReferenceLocked(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ NFC_CX_POWER_REFERENCE_TYPE Type
    )
/*++

Routine Description:

   NfcCxPowerReleaseFdoContextReferenceLocked releases
   an FDO power policy reference.

   Callers of this function must hold the Power.WaitLock.

Arguments:

    PowerManager - Pointer to the Power Manager
    Type - The type of power reference being released

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    LONG& powerReference = PowerManager->PowerReferences[Type];

    if (powerReference <= 0)
    {
        // About to underflow the Policy references
        NT_ASSERT(false);

        status = STATUS_INTEGER_OVERFLOW;
        TRACE_LINE(LEVEL_ERROR, "Power policy references underflow (%d), %!STATUS!", Type, status);

        TraceLoggingWrite(
            g_hNfcCxProvider,
            "PowerRefCountUnderflow",
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingInt32(Type, "powerReferenceType"));

        NfcCxDeviceSetFailed(PowerManager->FdoContext->Device);
        goto Done;
    }

    NFC_CX_POWER_RF_STATE oldRfState = NfcCxPowerGetRfState(PowerManager);

    powerReference -= 1;

    NFC_CX_POWER_RF_STATE newRfState = NfcCxPowerGetRfState(PowerManager);

    // Check if we need to update the RF config.
    if (NfcCxPowerHasDiscoveryConfigChanged(oldRfState, newRfState))
    {
        NfcCxPowerUpdateRFPollingState(PowerManager, /*PoweringUpRF*/ FALSE);
    }

    // Check if we need to allow the NFC Controller to power down.
    if (!NfcCxPowerShouldStopIdle(newRfState))
    {
        NfcCxPowerDeviceResumeIdle(PowerManager);
    }

    TRACE_LINE(LEVEL_INFO, "Current power references: NFP=%d, CE=%d, SL=%d, NL=%d, eSE=%d",
        PowerManager->PowerReferences[NfcCxPowerReferenceType_Proximity],
        PowerManager->PowerReferences[NfcCxPowerReferenceType_CardEmulation],
        PowerManager->PowerReferences[NfcCxPowerReferenceType_StealthListen],
        PowerManager->PowerReferences[NfcCxPowerReferenceType_NoListen],
        PowerManager->PowerReferences[NfcCxPowerReferenceType_ESe]);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerDeviceStopIdle(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
/*++

Routine Description:

   Stops the idle timer for the device and signals the device to asynchronously enter the D0 power state.

   NOTE: Some client drivers enter a low power mode (instead of uninitializaing the NFC Controller) in their
   Dx idle state. For these devices, we don't want to force them into the D0 state just because there is an
   active client handle. Hence those drivers are given the option to opt-out of the Power Manager's
   WdfDeviceStopIdle call.

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Allow the client driver to opt-out of the Power Manager's WdfDeviceStopIdle/WdfDeviceResumeIdle calls.
    if (PowerManager->FdoContext->DisablePowerManagerStopIdle)
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_VERBOSE, "'DisablePowerManagerStopIdle' is set. Skipping stop idle.");
        goto Done;
    }

    if (PowerManager->DeviceStopIdle)
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_VERBOSE, "Device idle timer already stopped.");
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Powering up");

    status = WdfDeviceStopIdle(PowerManager->FdoContext->Device, /*WaitForD0*/ FALSE);

    if (status == STATUS_PENDING)
    {
        // STATUS_PENDING means the device is booting up asynchronously (which is what we told it to do).
        status = STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "'WdfDeviceStopIdle' call failed, %!STATUS!", status);
        goto Done;
    }

    PowerManager->DeviceStopIdle = TRUE;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

_Requires_lock_held_(PowerManager->WaitLock)
void
NfcCxPowerDeviceResumeIdle(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
/*++

Routine Description:

   Resumes the idle timer for the device, if it hasn't been resumed already.

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:
    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Allow the client driver to opt-out of the Power Manager's WdfDeviceStopIdle/WdfDeviceResumeIdle calls.
    if (PowerManager->FdoContext->DisablePowerManagerStopIdle)
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_VERBOSE, "'DisablePowerManagerStopIdle' is set. Skipping resume idle.");
        goto Done;
    }

    if (!PowerManager->DeviceStopIdle)
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_VERBOSE, "Device idle timer already resumed.");
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Powering down");

    WdfDeviceResumeIdle(PowerManager->FdoContext->Device);
    PowerManager->DeviceStopIdle = FALSE;

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxPowerUpdateRFPollingState(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ BOOLEAN PoweringUpRF
    )
/*++

Routine Description:

   Pings the NfcCx state machine to update the RF polling state.
   Note: This function synchronously blocks until the state machine thread has finished processing this request.

   Pertinent functions:
     - NfcCxRFInterfaceGetDiscoveryConfig

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:

    VOID

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!IsPowerRFManagementEnabled(PowerManager->FdoContext))
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_WARNING, "'NfcCxPowerUpdateRFPollingState' called but not in NCI mode.");
        goto Done;
    }

    // Notes:
    // 1: We may be on the RF/NCI thread here. (For example, the smartcard interface adds/releases power references on the
    //   RF/NCI thread.) So we can't call directly into NfcCxRF without risking a deadlock. So we must invoke the RF Config Update
    //   on a threadpool thread instead.
    // 2: We are using a timer here to prevent us from constantly turning on and off the radio.
    LONGLONG dueTime = PoweringUpRF ? 0 : c_DefaultRFUpdateWaitTime;
    WdfTimerStart(PowerManager->UpdateRFStateTimer, dueTime);

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxPowerUpdateRFPollingStateWorker(
    _In_ WDFTIMER Timer
    )
/*++

Routine Description:

   Worker callback for NfcCxPowerUpdateRFPollingState

Arguments:

    Timer - The timer object that triggered this callback

Return Value

    VOID
--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WDFOBJECT device = WdfTimerGetParentObject(Timer);
    PNFCCX_FDO_CONTEXT fdoContext = NfcCxFdoGetContext(device);
    PNFCCX_POWER_MANAGER powerManager = fdoContext->Power;

    WdfWaitLockAcquire(powerManager->WaitLock, NULL);

    NFC_CX_POWER_RF_STATE currentRfState = NfcCxPowerGetRfState(powerManager);

    WdfWaitLockRelease(powerManager->WaitLock);

    status = NfcCxRFInterfaceUpdateDiscoveryState(fdoContext->RFInterface, currentRfState);

    if (status == STATUS_INVALID_DEVICE_STATE)
    {
        // If the device has entered D3, then the RF is off and can't be configured. So we will recieve
        // STATUS_INVALID_DEVICE_STATE. When the device boots back up, RF config will be run then.
        // So it is safe to ignore this error code.
        TRACE_LINE(LEVEL_VERBOSE, "'NfcCxRFInterfaceUpdateDiscoveryState' skipped.");
        status = STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "'NfcCxRFInterfaceUpdateDiscoveryState' call failed, status=%!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    if (!NT_SUCCESS(status))
    {
        TraceLoggingWrite(
            g_hNfcCxProvider,
            "UpdateRfPollingStateFailed",
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingNTStatus(status, "status"));

        NfcCxDeviceSetFailed(fdoContext->Device);
    }
}

void
NfcCxPowerCleanupFilePolicyReferences(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

   NfcCxPowerCleanupFilePolicyReferences cleans up any left
   over power policy references associated with the file object.

Arguments:

    PowerManager - Pointer to the Power Manager
    FileContext - Pointer to the file object context

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    WdfWaitLockAcquire(PowerManager->WaitLock, NULL);

    for (int i = 0; i != NfcCxPowerReferenceType_MaxCount; ++i)
    {
        LONG& filePowerReference = FileContext->PowerReferences[i];

        if (filePowerReference > 0)
        {
            NfcCxPowerReleaseFdoContextReferenceLocked(PowerManager, (NFC_CX_POWER_REFERENCE_TYPE)i);
        }

        filePowerReference = 0;
    }

    WdfWaitLockRelease(PowerManager->WaitLock);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxPowerSetRadioState(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PNFCRM_SET_RADIO_STATE RadioState
    )
/*++

Routine Description:

   NfcCxPowerSet Policy handles the call from IOCTL_NFCRM_SET_RADIO_STATE.
   
   Based on the desired Power setting will disable IO forwarding between
   the non power managed and power managed queue and stop the latter.

Arguments:

    PowerManager - Pointer to the Power Manager
    FileContext - Pointer to the file object context
    RadioState - The desired radio state

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    PNFCCX_FDO_CONTEXT fdoContext = PowerManager->FdoContext;

    WdfWaitLockAcquire(PowerManager->WaitLock, NULL);

    //
    // Validate that the caller is not within the app container
    //
    if (FileContext->IsAppContainerProcess) {
        TRACE_LINE(LEVEL_ERROR, "SetPower cannot be called from the app container process");
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "SystemStateUpdate: %!BOOLEAN!, MediaRadioOn: %!BOOLEAN!", RadioState->SystemStateUpdate, RadioState->MediaRadioOn);

    BOOLEAN desiredRadioState;
    if (RadioState->SystemStateUpdate)
    {
        PowerManager->NfpFlightModeEnabled = !RadioState->MediaRadioOn;

        if (PowerManager->NfpFlightModeEnabled)
        {
            // Airplane mode turned on.
            // Cache the current radio state and turn the radio off.
            PowerManager->NfpRadioStateBeforeFlightMode = PowerManager->NfpRadioState;
            desiredRadioState = FALSE;
        }
        else
        {
            // Airplane mode turned off.
            // Turn the radio back on if it was on before airplane mode was turned off (and keep the radio on
            // if it is already on).
            desiredRadioState = PowerManager->NfpRadioState || PowerManager->NfpRadioStateBeforeFlightMode;
        }
    }
    else
    {
        desiredRadioState = !!RadioState->MediaRadioOn;
    }

    if (PowerManager->NfpRadioState == desiredRadioState) {
        TRACE_LINE(LEVEL_ERROR, "We are already in the requested power state");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Current state %d, Desired State %d", PowerManager->NfpRadioState, desiredRadioState);

    //
    // Update the radio state here so that when we update the polling loop
    // configuration of the controller so that the correct states are used
    //
    NFC_CX_POWER_RF_STATE previousRfState = NfcCxPowerGetRfState(PowerManager);

    PowerManager->NfpRadioState = desiredRadioState;

    NFC_CX_POWER_RF_STATE rfState = NfcCxPowerGetRfState(PowerManager);


    if (desiredRadioState) {
        if (NfcCxPowerShouldStopIdle(rfState))
        {
            // Ensure device is powered up.
            status = NfcCxPowerDeviceStopIdle(PowerManager);
            if (!NT_SUCCESS(status))
            {
                TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
                goto Done;
            }
        }

        // Check if NFP has just been enabled.
        if (NfcCxPowerHasDiscoveryConfigChanged(previousRfState, rfState))
        {
            // Ensure RF config is updated.
            NfcCxPowerUpdateRFPollingState(PowerManager, /*PoweringUpRF*/ TRUE);
        }

        //
        // Start the appropriate modules
        //
        status = NfcCxSCInterfaceStart(fdoContext->SCInterface);
        if (!NT_SUCCESS(status))
        {
            TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
            goto Done;
        }

        status = NfcCxNfpInterfaceStart(fdoContext->NfpInterface);
        if (!NT_SUCCESS(status))
        {
            TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
            goto Done;
        }

    } else {
        //
        // Stop the appropriate modules
        //
        NfcCxSCInterfaceStop(fdoContext->SCInterface);
        NfcCxNfpInterfaceStop(fdoContext->NfpInterface);

        // Check if NFP has just been disabled.
        if (NfcCxPowerHasDiscoveryConfigChanged(previousRfState, rfState))
        {
            // Ensure RF config is updated.
            NfcCxPowerUpdateRFPollingState(PowerManager, /*PoweringUpRF*/ FALSE);
        }

        if (!NfcCxPowerShouldStopIdle(rfState))
        {
            // Allow the device to power down.
            NfcCxPowerDeviceResumeIdle(PowerManager);
        }
    }
    
    TRACE_LINE(LEVEL_INFO, "Current Power State = %!BOOLEAN!", PowerManager->NfpRadioState);

#ifdef EVENT_WRITE
    EventWritePowerSetRadioState(PowerManager->NfpRadioState);
#endif

Done:
    //
    // Persist the data into the registry
    //
    NfcCxPowerWriteRadioStateToRegistry(PowerManager);
    WdfWaitLockRelease(PowerManager->WaitLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxPowerQueryRadioState(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _Out_ PNFCRM_RADIO_STATE RadioState
    )
/*++

Routine Description:

   NfcCxPowerQuery Policy handles the call from IOCTL_NFCRM_QUERY_RADIO_STATE.
   
   The function will return the current power state of the device.

Arguments:

    PowerManager - Pointer to the Power Manager
    FileContext - Pointer to the file object context
    RadioState - A pointer to an NFCRM_RADIO_STATE structure to receive the current radio state

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Validate that the caller is not within the app container
    //
    if (FileContext->IsAppContainerProcess) {
        TRACE_LINE(LEVEL_ERROR, "SetPower cannot be called from the app container process");
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    WdfWaitLockAcquire(PowerManager->WaitLock, NULL);
    RadioState->MediaRadioOn = (PowerManager->NfpRadioState);
    WdfWaitLockRelease(PowerManager->WaitLock);

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

BOOLEAN
NfcCxPowerIsAllowedNfp(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
/*++

Routine Description:

   NfcCxPowerIsAllowedNfp returns the current NFP radio state.

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:
    TRUE - If the radio policy hasn't been overwriten to OFF
    FALSE - Otherwise

--*/
{
    BOOLEAN isAllowed = FALSE;

    WdfWaitLockAcquire(PowerManager->WaitLock, NULL);
    isAllowed = PowerManager->NfpRadioState;
    WdfWaitLockRelease(PowerManager->WaitLock);

    return isAllowed;
}

BOOLEAN
NfcCxPowerIsIoctlSupported(
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
    UNREFERENCED_PARAMETER(FdoContext);

    switch(IoControlCode) {
    case IOCTL_NFCRM_SET_RADIO_STATE:
    case IOCTL_NFCRM_QUERY_RADIO_STATE:
    case IOCTL_NFCSERM_SET_RADIO_STATE:
    case IOCTL_NFCSERM_QUERY_RADIO_STATE:
        return TRUE;
    default:
        return FALSE;
    }
}

NTSTATUS
NfcCxPowerIoDispatch(
    _In_opt_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext = NULL;
    ULONG_PTR bytesCopied =0;
    WDFMEMORY outMem = {0};
    WDFMEMORY inMem = {0};
    PVOID     inBuffer = NULL;
    PVOID     outBuffer = NULL;
    size_t    sizeInBuffer = 0;
    size_t    sizeOutBuffer = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // All Power handlers requires a valid file context
    //
    if (NULL == FileContext) {
        TRACE_LINE(LEVEL_ERROR, "Power request received without a file context");
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    //
    // All power requests are not allowed from the app container process
    //
    if (FileContext->IsAppContainerProcess) {
        TRACE_LINE(LEVEL_ERROR, "Power request received from the app container");
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }
    fdoContext = NfcCxFileObjectGetFdoContext(FileContext);

    //
    // Get the request memory and perform the operation here
    //
    if (0 != OutputBufferLength) {
        status = WdfRequestRetrieveOutputMemory(Request, &outMem);
        if(!NT_SUCCESS(status) ) {
            TRACE_LINE(LEVEL_ERROR, "Failed to retrieve the output buffer, %!STATUS!", status);
            goto Done;
        }
    }
    if (0 != InputBufferLength) {
        status = WdfRequestRetrieveInputMemory(Request, &inMem);
        if(!NT_SUCCESS(status) ) {
            TRACE_LINE(LEVEL_ERROR, "Failed to retrieve the input buffer, %!STATUS!", status);
            goto Done;
        }
    }

    switch (IoControlCode) {
    case IOCTL_NFCRM_SET_RADIO_STATE:
        {
            PNFCRM_SET_RADIO_STATE pSetRadioState = NULL;

            if (sizeof(*pSetRadioState) != InputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid buffer");
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            inBuffer = WdfMemoryGetBuffer(inMem, &sizeInBuffer);
            NT_ASSERT(sizeInBuffer == InputBufferLength);

            pSetRadioState = (PNFCRM_SET_RADIO_STATE)inBuffer;

            status = NfcCxPowerSetRadioState(fdoContext->Power,
                                    FileContext,
                                    pSetRadioState);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to set the Power, %!STATUS!", status);
                goto Done;
            }

        }
        break;
    case IOCTL_NFCRM_QUERY_RADIO_STATE:
        {
            PNFCRM_RADIO_STATE pRadioState = NULL;

            if (sizeof(*pRadioState) != OutputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid buffer");
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            outBuffer = WdfMemoryGetBuffer(outMem, &sizeOutBuffer);
            NT_ASSERT(sizeOutBuffer == OutputBufferLength);

            pRadioState = (PNFCRM_RADIO_STATE)outBuffer;

            status = NfcCxPowerQueryRadioState(fdoContext->Power,
                                        FileContext,
                                        pRadioState);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Failed to set the Power, %!STATUS!", status);
                goto Done;
            }

            //
            // Return the current power state
            //
            bytesCopied = sizeof(*pRadioState);
            status = STATUS_SUCCESS;
        }
        break;
    case IOCTL_NFCSERM_SET_RADIO_STATE:
    case IOCTL_NFCSERM_QUERY_RADIO_STATE:
        {
            status = STATUS_NOT_IMPLEMENTED;
        }
        break;
    default:
        
        break;
    }

Done:

    if (NT_SUCCESS(status)) {
        WdfRequestCompleteWithInformation(Request, status, bytesCopied);
        status = STATUS_PENDING;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

void
NfcCxPowerReadRadioStateFromRegistry(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    WDFKEY regkey = nullptr;

    // Open registry key
    status = WdfDeviceOpenRegistryKey(
        PowerManager->FdoContext->Device,
        PLUGPLAY_REGKEY_DEVICE | WDF_REGKEY_DEVICE_SUBKEY,
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &regkey);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Can't open registry key. %!STATUS!", status);
        goto Done;
    }

    // RadioEnabled
    status = NfcCxRegistryQueryBoolean(regkey, NFCCX_REG_NFC_RADIO_STATE, &PowerManager->NfpRadioState);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to read 'RadioEnabled' value. %!STATUS!", status);
    }

    // FlightModeEnabled
    status = NfcCxRegistryQueryBoolean(regkey, NFCCX_REG_NFC_FLIGHT_MODE, &PowerManager->NfpFlightModeEnabled);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to read 'FlightModeEnabled' value. %!STATUS!", status);
    }

    // RadioEnabledBeforeFlightMode
    status = NfcCxRegistryQueryBoolean(regkey, NFCCX_REG_NFC_RADIO_STATE_BEFORE_FLIGHT_MODE, &PowerManager->NfpRadioStateBeforeFlightMode);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to read 'NfpRadioStateBeforeFlightMode' value. %!STATUS!", status);
    }

Done:
    if (regkey)
    {
        WdfRegistryClose(regkey);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

_Requires_lock_held_(PowerManager->WaitLock)
void
NfcCxPowerWriteRadioStateToRegistry(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    WDFKEY regkey = nullptr;

     // Open registry key
    status = WdfDeviceOpenRegistryKey(
        PowerManager->FdoContext->Device,
        PLUGPLAY_REGKEY_DEVICE | WDF_REGKEY_DEVICE_SUBKEY,
        KEY_READ | KEY_SET_VALUE,
        WDF_NO_OBJECT_ATTRIBUTES,
        &regkey);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Can't open registry key. %!STATUS!", status);
        goto Done;
    }

    // RadioEnabled
    status = NfcCxRegistryAssignBoolean(regkey, NFCCX_REG_NFC_RADIO_STATE, PowerManager->NfpRadioState);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to write 'RadioEnabled' value. %!STATUS!", status);
    }

    // FlightModeEnabled
    status = NfcCxRegistryAssignBoolean(regkey, NFCCX_REG_NFC_FLIGHT_MODE, PowerManager->NfpFlightModeEnabled);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to write 'FlightModeEnabled' value. %!STATUS!", status);
    }

    // RadioEnabledBeforeFlightMode
    status = NfcCxRegistryAssignBoolean(regkey, NFCCX_REG_NFC_RADIO_STATE_BEFORE_FLIGHT_MODE, PowerManager->NfpRadioStateBeforeFlightMode);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to write 'NfpRadioStateBeforeFlightMode' value. %!STATUS!", status);
    }

Done:
    if (regkey)
    {
        WdfRegistryClose(regkey);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}
