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

    if (IsPowerRFManagementEnabled(FdoContext))
    {
        // Check the currently required power state
        powerManager->NfpRadioState = (!FdoContext->NfpPowerOffPolicyOverride && !FdoContext->NfpPowerOffSystemOverride);
        powerManager->SERadioState = (!FdoContext->SEPowerOffPolicyOverride && !FdoContext->SEPowerOffSystemOverride);
    }

    WDF_OBJECT_ATTRIBUTES objectAttrib;
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = FdoContext->Device;

    status = WdfWaitLockCreate(&objectAttrib, &powerManager->WaitLock);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the PowerPolicy WaitLock, %!STATUS!", status);
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
        BOOLEAN shouldDeviceStopIdle = NfcCxPowerShouldDeviceStopIdle(PowerManager);
        if (shouldDeviceStopIdle)
        {
            // Ensure RF radio is turned on.
            status = NfcCxPowerUpdateRFPollingState(PowerManager);
            if (!NT_SUCCESS(status))
            {
                TRACE_LINE(LEVEL_ERROR, "Failed to update RF discovery state, %!STATUS!", status);
                goto Done;
            }
        }
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxPowerStop(
    _In_ PNFCCX_POWER_MANAGER /*PowerManager*/
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
    // Nothing to do
}

NTSTATUS
NfcCxPowerSetPolicy(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PNCI_POWER_POLICY PowerPolicy
    )
/*++

Routine Description:

   NfcCxPowerSetPolicy will acquire or release a reference 
   into the file extention then forward the acquire/release 
   reference to the fdo.

Arguments:

    PowerManager - Pointer to the Power Manager
    FileContext - Pointer to the file object context
    PowerPolicy - The desired power policy

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG powerPolicyReferences = 0;
    BOOLEAN releaseFdoContextRef = FALSE;
    BOOLEAN acquireFdoContextRef = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(PowerManager->WaitLock, NULL);

    powerPolicyReferences = FileContext->PowerPolicyReferences;
    if (PowerPolicy->CanPowerDown) {

        //
        // Releasing a reference that doesn't exist
        //
        if (0 == powerPolicyReferences) {
            TRACE_LINE(LEVEL_ERROR, "Releasing a reference that was not acquired!");
            status = STATUS_INVALID_DEVICE_REQUEST;
            WdfWaitLockRelease(PowerManager->WaitLock);
            goto Done;
        }

        powerPolicyReferences--;

        if (0 == powerPolicyReferences) {
            releaseFdoContextRef = TRUE;
        }

    } else {

        if (0 == powerPolicyReferences) {
            acquireFdoContextRef = TRUE;
        }

        if (MAX_ULONG == powerPolicyReferences) {

            //
            // About to overflow the Policy references
            //
            TRACE_LINE(LEVEL_ERROR, "Power policy references overflow");

            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxPowerSetPolicyOverflow",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

            status = STATUS_UNSUCCESSFUL;
            NfcCxDeviceSetFailed(PowerManager->FdoContext->Device);

            WdfWaitLockRelease(PowerManager->WaitLock);
            goto Done;
        }

        powerPolicyReferences++;
    }

    if (acquireFdoContextRef) {
        BOOLEAN powerReferenceTaken = FALSE;
        status = NfcCxPowerAcquireFdoContextReferenceLocked(PowerManager, FileContext, &powerReferenceTaken);
        if (NT_SUCCESS(status) && !powerReferenceTaken)
        {
            powerPolicyReferences--;
        }
    }

    if (releaseFdoContextRef) {
        status = NfcCxPowerReleaseFdoContextReferenceLocked(PowerManager, FileContext);
    }

    if (NT_SUCCESS(status))
    {
        FileContext->PowerPolicyReferences = powerPolicyReferences;
    }

    TRACE_LINE(LEVEL_INFO, "Current Power Policy references [FileObject = %p], [FileReferences = %d], [FdoReferences = %d,%d]",
            FileContext->FileObject,
            FileContext->PowerPolicyReferences,
            PowerManager->NfpPowerPolicyReferences,
            PowerManager->SEPowerPolicyReferences);

    WdfWaitLockRelease(PowerManager->WaitLock);

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

BOOLEAN
NfcCxPowerShouldDeviceStopIdle(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
/*++

Routine Description:

   Determines whether or not driver idle timer should be stopped. That is, whether or not the
   device should be powered on.

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:
    BOOLEAN

--*/
{
    // Note: The values of all these variables do not provide any gaurantees about the state of any other memory within the driver.
    // Hence no memory fence is required.
    return
        (PowerManager->SERadioState && (PowerManager->SEPowerPolicyReferences != 0)) ||
        (PowerManager->NfpRadioState && (PowerManager->NfpPowerPolicyReferences != 0));
}

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerAcquireFdoContextReferenceLocked(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _Out_ BOOLEAN* pReferenceTaken
    )
/*++

Routine Description:

   NfcCxPowerAcquireFdoContextReferenceLocked acquires
   an FDO power policy reference.

   Callers of this function must hold the Power.WaitLock.

Arguments:

    PowerManager - Pointer to the Power Manager
    FileContext - Pointer to the file object context
    pReferenceTaken - Output boolean indicating whether we acquired a power policy reference

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN IsInterfacePoweringUp = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(pReferenceTaken != NULL);

    *pReferenceTaken = FALSE;
    if (NFC_CX_DEVICE_MODE_RAW == PowerManager->FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        TRACE_LINE(LEVEL_INFO, "Raw device mode. Powering up");

        status = WdfDeviceStopIdle(PowerManager->FdoContext->Device, TRUE);
        goto Done;
    }

    if (!NfcCxFileObjectIsSEManager(FileContext)) {
        if (MAX_ULONG == PowerManager->NfpPowerPolicyReferences) {

            //
            // About to overflow the Policy references
            //
            TRACE_LINE(LEVEL_ERROR, "NFP power policy references overflow");

            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxPowerAcquireFdoContextNfpReferencesOverflow",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

            status = STATUS_UNSUCCESSFUL;
            goto Done;
        }

        IsInterfacePoweringUp = (0 == PowerManager->NfpPowerPolicyReferences) && PowerManager->NfpRadioState;
        PowerManager->NfpPowerPolicyReferences++;
        *pReferenceTaken = TRUE;

    } else {
        if (MAX_ULONG == PowerManager->SEPowerPolicyReferences) {

            //
            // About to overflow the Policy references
            //
            TRACE_LINE(LEVEL_ERROR, "SE power policy references overflow");

            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxPowerAcquireFdoContextSeReferencesOverflow",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

            status = STATUS_UNSUCCESSFUL;
            goto Done;
        }

        IsInterfacePoweringUp = (0 == PowerManager->SEPowerPolicyReferences) && PowerManager->SERadioState;
        PowerManager->SEPowerPolicyReferences++;
        *pReferenceTaken = TRUE;
    }

    BOOLEAN shouldDeviceStopIdle = NfcCxPowerShouldDeviceStopIdle(PowerManager);

    if (shouldDeviceStopIdle)
    {
        // Ensure device is powered up.
        // Note: None of our I/O queues are power managed. Hence 'WaitForD0' is allowed to be TRUE.
        //   (Though some IOCTL's do call 'WdfDeviceStopIdle' directly.)
        status = NfcCxPowerDeviceStopIdle(PowerManager, /*WaitForD0*/ TRUE);
        if (!NT_SUCCESS(status)) {
            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxPowerAcquireFdoContextPoweringUpFailed",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
            goto Done;
        }
    }

    // NFP(/SC) and SE can be enabled and disabled separately. So if either of them just got their first power reference,
    // we need to update the RF config. (See, 'NfcCxRFInterfaceGetDiscoveryConfig'.)
    if (IsInterfacePoweringUp)
    {
        status = NfcCxPowerUpdateRFPollingState(PowerManager);
        if (!NT_SUCCESS(status)) {
            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxPowerAcquireFdoContextPoweringUpFailed",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
            goto Done;
        }
    }

Done:
    if (!NT_SUCCESS(status))
    {
        NfcCxDeviceSetFailed(PowerManager->FdoContext->Device);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerReleaseFdoContextReferenceLocked(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

   NfcCxPowerReleaseFdoContextReferenceLocked releases
   an FDO power policy reference.

   Callers of this function must hold the Power.WaitLock.

Arguments:

    PowerManager - Pointer to the Power Manager
    FileContext - Pointer to the file object context

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN IsInterfacePoweringDown = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFC_CX_DEVICE_MODE_RAW == PowerManager->FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        TRACE_LINE(LEVEL_INFO, "Raw device mode. Powering down");

        WdfDeviceResumeIdle(PowerManager->FdoContext->Device);
        goto Done;
    }

    if (!NfcCxFileObjectIsSEManager(FileContext)) {

        if (0 == PowerManager->NfpPowerPolicyReferences) {

            //
            // About to underflow the Policy references
            //
            TRACE_LINE(LEVEL_ERROR, "NFP power policy references underflow. Releasing a reference that was not acquired!");

            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxPowerReleaseFdoContextNfpReferencesUnderflow",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

            status = STATUS_UNSUCCESSFUL;
            goto Done;
        }

        PowerManager->NfpPowerPolicyReferences--;
        IsInterfacePoweringDown = (0 == PowerManager->NfpPowerPolicyReferences) && PowerManager->NfpRadioState;
    } else {

        if (0 == PowerManager->SEPowerPolicyReferences) {

            //
            // About to underflow the Policy references
            //
            TRACE_LINE(LEVEL_ERROR, "SE power policy references underflow. Releasing a reference that was not acquired!");

            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxPowerReleaseFdoContextSeReferencesUnderflow",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

            status = STATUS_UNSUCCESSFUL;
            goto Done;
        }

        PowerManager->SEPowerPolicyReferences--;
        IsInterfacePoweringDown = (0 == PowerManager->SEPowerPolicyReferences) && PowerManager->SERadioState;
    }

    // NFP(/SC) and SE can be enabled and disabled separately. So if either of them just got lost their last power reference,
    // we need to update the RF config. (See, 'NfcCxRFInterfaceGetDiscoveryConfig'.)
    if (IsInterfacePoweringDown)
    {
        status = NfcCxPowerUpdateRFPollingState(PowerManager);
        if (!NT_SUCCESS(status))
        {
            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxPowerReleaseFdoContextPoweringDownFailed",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

            goto Done;
        }
    }

    BOOLEAN shouldDeviceStopIdle = NfcCxPowerShouldDeviceStopIdle(PowerManager);

    if (!shouldDeviceStopIdle)
    {
        // Allow device to power down.
        NfcCxPowerDeviceResumeIdle(PowerManager);
    }

Done:
    if (!NT_SUCCESS(status))
    {
        NfcCxDeviceSetFailed(PowerManager->FdoContext->Device);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

_Requires_lock_held_(PowerManager->WaitLock)
NTSTATUS
NfcCxPowerDeviceStopIdle(
    _In_ PNFCCX_POWER_MANAGER PowerManager,
    _In_ BOOLEAN WaitForD0
    )
/*++

Routine Description:

   Stops the idle timer for the device and brings the device back into the D0 power state (if neccessary).

Arguments:

    PowerManager - Pointer to the Power Manager
    WaitForD0 - Whether or not to wait for the driver to enter the D0 power state before returning.

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (PowerManager->DeviceStopIdle)
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_VERBOSE, "Device idle timer already stopped.");
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Powering up");

    status = WdfDeviceStopIdle(PowerManager->FdoContext->Device, WaitForD0);

    if (!NT_SUCCESS(status))
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

    if (!PowerManager->DeviceStopIdle)
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_VERBOSE, "Device idle timer already resumed.");
        TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
        return;
    }

    TRACE_LINE(LEVEL_INFO, "Powering down");

    WdfDeviceResumeIdle(PowerManager->FdoContext->Device);
    PowerManager->DeviceStopIdle = FALSE;

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxPowerUpdateRFPollingState(
    _In_ PNFCCX_POWER_MANAGER PowerManager
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
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!IsPowerRFManagementEnabled(PowerManager->FdoContext))
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_INFO, "'NfcCxPowerUpdateRFPollingState' called but 'FdoContext->RFInterface' doesn't exist.");
        goto Done;
    }

    status = NfcCxRFInterfaceUpdateDiscoveryState(PowerManager->FdoContext->RFInterface);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "'NfcCxRFInterfaceUpdateDiscoveryState' call failed, status=%!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
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

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(PowerManager->WaitLock, NULL);

    if (0 != FileContext->PowerPolicyReferences) {
        (VOID)NfcCxPowerReleaseFdoContextReferenceLocked(PowerManager, FileContext);
    }
    FileContext->PowerPolicyReferences = 0;

    TRACE_LINE(LEVEL_INFO, "Current Power Policy references [FileObject = %p], [FileReferences = %d], [FdoReferences = %d,%d]",
            FileContext->FileObject,
            FileContext->PowerPolicyReferences,
            PowerManager->NfpPowerPolicyReferences,
            PowerManager->SEPowerPolicyReferences);

    WdfWaitLockRelease(PowerManager->WaitLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
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
    BOOLEAN desiredRadioState;
    BOOLEAN currentRadioState;

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

    if (RadioState->SystemStateUpdate) {
        fdoContext->NfpPowerOffSystemOverride = RadioState->MediaRadioOn ? FALSE : TRUE;
    }
    else {
        //
        // Since the request is for modifying the radio state
        // we override the system state.
        //
        fdoContext->NfpPowerOffSystemOverride = FALSE;
        fdoContext->NfpPowerOffPolicyOverride = RadioState->MediaRadioOn ? FALSE : TRUE;
    }

    desiredRadioState = !fdoContext->NfpPowerOffPolicyOverride && !fdoContext->NfpPowerOffSystemOverride;

    if (PowerManager->NfpRadioState == desiredRadioState) {
        TRACE_LINE(LEVEL_ERROR, "We are already in the requested power state");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    //
    // Update the radio state here so that when we update the polling loop
    // configuration of the controller so that the correct states are used
    //
    currentRadioState = PowerManager->NfpRadioState;
    PowerManager->NfpRadioState = desiredRadioState;

    BOOLEAN shouldDeviceStopIdle = NfcCxPowerShouldDeviceStopIdle(PowerManager);

    TRACE_LINE(LEVEL_INFO, "Current state %d, Desired State %d", 
                                                    currentRadioState,
                                                    RadioState->MediaRadioOn);
    if (desiredRadioState) {
        if (shouldDeviceStopIdle)
        {
            // Ensure device is powered up.
            status = NfcCxPowerDeviceStopIdle(PowerManager, /*WaitForD0*/ TRUE);
            if (NT_SUCCESS(status)) {
                // Ensure RF config is updated.
                NfcCxPowerUpdateRFPollingState(PowerManager);
            }
        }

        //
        // Start the appropriate modules
        //
        NfcCxSCInterfaceStart(fdoContext->SCInterface);
        NfcCxNfpInterfaceStart(fdoContext->NfpInterface);

    } else {
        //
        // Stop the appropriate modules
        //
        NfcCxSCInterfaceStop(fdoContext->SCInterface);
        NfcCxNfpInterfaceStop(fdoContext->NfpInterface);

        if (!shouldDeviceStopIdle)
        {
            // Ensure RF config is updated.
            NfcCxPowerUpdateRFPollingState(PowerManager);

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
    NfcCxFdoWritePersistedDeviceRegistrySettings(fdoContext);
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
NfcCxPowerIsAllowedSE(
    _In_ PNFCCX_POWER_MANAGER PowerManager
    )
/*++

Routine Description:

   NfcCxPowerIsAllowedSE returns the current SE radio state.

Arguments:

    PowerManager - Pointer to the Power Manager

Return Value:
    TRUE - If the radio policy hasn't been overwriten to OFF
    FALSE - Otherwise

--*/
{
    BOOLEAN isAllowed = FALSE;

    WdfWaitLockAcquire(PowerManager->WaitLock, NULL);
    isAllowed = PowerManager->SERadioState;
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
