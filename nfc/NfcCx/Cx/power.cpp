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

NTSTATUS
NfcCxPowerSetPolicy(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PNCI_POWER_POLICY PowerPolicy
    )
/*++

Routine Description:

   NfcCxPowerSetPolicy will acquire or release a reference 
   into the file extention then forward the acquire/release 
   reference to the fdo.

Arguments:

    FdoContext - Pointer to the FDO Context
    FileContext - Pointer to the file object context
    PowerPolicy - The desired power policy

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN releaseFdoContextRef = FALSE;
    BOOLEAN acquireFdoContextRef = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(FdoContext->PowerPolicyWaitLock, NULL);

    if (PowerPolicy->CanPowerDown) {

        //
        // Releasing a reference that doesn't exist
        //
        if (0 == FileContext->PowerPolicyReferences) {
            TRACE_LINE(LEVEL_ERROR, "Releasing a reference that was not acquired!");
            status = STATUS_INVALID_DEVICE_REQUEST;
            WdfWaitLockRelease(FdoContext->PowerPolicyWaitLock);
            goto Done;
        }

        FileContext->PowerPolicyReferences--;

        if (0 == FileContext->PowerPolicyReferences) {
            releaseFdoContextRef = TRUE;
        }

    } else {

        if (0 == FileContext->PowerPolicyReferences) {
            acquireFdoContextRef = TRUE;
        }

        if (MAX_ULONG == FileContext->PowerPolicyReferences) {

            //
            // About to overflow the Policy references -> BC
            //
            WdfCxVerifierKeBugCheck(FileContext->FileObject, 
                                    NFC_CX_VERIFIER_BC, 
                                    NFC_CX_BC_POWER_REF_OVERFLOW, 
                                    NULL, 
                                    NULL, 
                                    NULL);
            WdfWaitLockRelease(FdoContext->PowerPolicyWaitLock);
            goto Done;
        }

        FileContext->PowerPolicyReferences++;
    }

    if (acquireFdoContextRef) {
        status = NfcCxPowerAcquireFdoContextReferenceLocked(FdoContext, FileContext);
    }

    if (releaseFdoContextRef) {
        status = NfcCxPowerReleaseFdoContextReferenceLocked(FdoContext, FileContext);
    }

    TRACE_LINE(LEVEL_INFO, "Current Power Policy references [FileObject = %p], [FileReferences = %d], [FdoReferences = %d,%d]",
            FileContext->FileObject,
            FileContext->PowerPolicyReferences,
            FdoContext->NfpPowerPolicyReferences,
            FdoContext->SEPowerPolicyReferences);

    WdfWaitLockRelease(FdoContext->PowerPolicyWaitLock);

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

_Requires_lock_held_(FdoContext->PowerPolicyWaitLock)
NTSTATUS
NfcCxPowerAcquireFdoContextReferenceLocked(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

   NfcCxPowerAcquireFdoContextReferenceLocked acquires
   an FDO power policy reference.

   Callers of this function must hold the PowerPolicyWaitLock.

Arguments:

    FdoContext - Pointer to the FDO Context
    FileContext - Pointer to the file object context

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN IsPoweringUp = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFC_CX_DEVICE_MODE_RAW == FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        IsPoweringUp = TRUE;
        status = WdfDeviceStopIdle(FdoContext->Device, TRUE);
        goto Done;
    }

    if (!NfcCxFileObjectIsSEManager(FileContext)) {
        if (MAX_ULONG == FdoContext->NfpPowerPolicyReferences) {

            //
            // About to overflow the Policy references -> BC
            //
            WdfCxVerifierKeBugCheck(FdoContext->Device, 
                                    NFC_CX_VERIFIER_BC, 
                                    NFC_CX_BC_POWER_REF_OVERFLOW, 
                                    NULL, 
                                    NULL, 
                                    NULL);
            goto Done;
        }

        if (!FdoContext->NfpRadioState) {
            //
            // It is possible to get here due to a race condition between
            // the thread responsible for disabling the interface and a CreateFile
            //
            NT_ASSERT(FileContext->PowerPolicyReferences > 0);
            FileContext->PowerPolicyReferences--;
            TRACE_LINE(LEVEL_WARNING, 
                       "Request received to increment NfpPowerPolicyReference with the Nfp interface disabled");

        } else {
            IsPoweringUp = (0 == FdoContext->NfpPowerPolicyReferences);
            FdoContext->NfpPowerPolicyReferences++;
        }

    } else {
        if (MAX_ULONG == FdoContext->SEPowerPolicyReferences) {

            //
            // About to overflow the Policy references -> BC
            //
            WdfCxVerifierKeBugCheck(FdoContext->Device, 
                                    NFC_CX_VERIFIER_BC, 
                                    NFC_CX_BC_POWER_REF_OVERFLOW, 
                                    NULL, 
                                    NULL, 
                                    NULL);
            goto Done;
        }

        if (!FdoContext->SERadioState) {
            //
            // It is possible to get here due to a race condition between
            // the thread responsible for disabling the interface and a CreateFile
            //
            NT_ASSERT(FileContext->PowerPolicyReferences > 0);
            FileContext->PowerPolicyReferences--;
            TRACE_LINE(LEVEL_WARNING, 
                       "Request received to increment SEPowerPolicyReference with the SE interface disabled");

        } else {
            IsPoweringUp = (0 == FdoContext->SEPowerPolicyReferences);
            FdoContext->SEPowerPolicyReferences++;
        }
    }

    if (IsPoweringUp) {
        // SEManager power requests are dispatched from power managed queue
        status = WdfDeviceStopIdle(FdoContext->Device,
                                   !NfcCxFileObjectIsSEManager(FileContext));
        if (NT_SUCCESS(status)) {
            NfcCxRFInterfaceUpdateDiscoveryState(FdoContext->RFInterface);
            status = STATUS_SUCCESS;
        }
    }

Done:

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, IsPoweringUp);

    return status;
}

_Requires_lock_held_(FdoContext->PowerPolicyWaitLock)
NTSTATUS
NfcCxPowerReleaseFdoContextReferenceLocked(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

   NfcCxPowerReleaseFdoContextReferenceLocked releases
   an FDO power policy reference.

   Callers of this function must hold the PowerPolicyWaitLock.

Arguments:

    FdoContext - Pointer to the FDO Context
    FileContext - Pointer to the file object context

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN IsPoweringDown = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFC_CX_DEVICE_MODE_RAW == FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        IsPoweringDown = TRUE;
        WdfDeviceResumeIdle(FdoContext->Device);
        goto Done;
    }

    if (!NfcCxFileObjectIsSEManager(FileContext)) {

        if (0 == FdoContext->NfpPowerPolicyReferences) {

            //
            // About to underflow the Policy references -> BC
            //
            WdfCxVerifierKeBugCheck(FdoContext->Device, 
                                    NFC_CX_VERIFIER_BC, 
                                    NFC_CX_BC_POWER_REF_UNDERFLOW, 
                                    NULL, 
                                    NULL, 
                                    NULL);
            
            TRACE_LINE(LEVEL_ERROR, "Releasing a reference that was not acquired!");
            goto Done;
        }

        FdoContext->NfpPowerPolicyReferences--;
        IsPoweringDown = (0 == FdoContext->NfpPowerPolicyReferences);
            
    } else {

        if (0 == FdoContext->SEPowerPolicyReferences) {

            //
            // About to underflow the Policy references -> BC
            //
            WdfCxVerifierKeBugCheck(FdoContext->Device, 
                                    NFC_CX_VERIFIER_BC, 
                                    NFC_CX_BC_POWER_REF_UNDERFLOW, 
                                    NULL, 
                                    NULL, 
                                    NULL);
            
            TRACE_LINE(LEVEL_ERROR, "Releasing a reference that was not acquired!");
            goto Done;
        }

        FdoContext->SEPowerPolicyReferences--;
        IsPoweringDown = (0 == FdoContext->SEPowerPolicyReferences);
    }

    if (IsPoweringDown) {
        //
        // Either proximity or secure element feature has been disabled, so increment idle count
        //
        if (FdoContext->RFInterface != NULL) {
            NfcCxRFInterfaceUpdateDiscoveryState(FdoContext->RFInterface);
        }
        WdfDeviceResumeIdle(FdoContext->Device);
    }

Done:

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, IsPoweringDown);

    return status;
}

NTSTATUS
NfcCxPowerCleanupFilePolicyReferences(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

   NfcCxPowerCleanupFilePolicyReferences cleans up any left
   over power policy references associated with the file object.

Arguments:

    FdoContext - Pointer to the FDO Context
    FileContext - Pointer to the file object context

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(FdoContext->PowerPolicyWaitLock, NULL);

    if (0 != FileContext->PowerPolicyReferences) {
        (VOID)NfcCxPowerReleaseFdoContextReferenceLocked(FdoContext, FileContext);
    }
    FileContext->PowerPolicyReferences = 0;

    TRACE_LINE(LEVEL_INFO, "Current Power Policy references [FileObject = %p], [FileReferences = %d], [FdoReferences = %d,%d]",
            FileContext->FileObject,
            FileContext->PowerPolicyReferences,
            FdoContext->NfpPowerPolicyReferences,
            FdoContext->SEPowerPolicyReferences);

    WdfWaitLockRelease(FdoContext->PowerPolicyWaitLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxPowerSetRadioState(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PNFCRM_SET_RADIO_STATE RadioState
    )
/*++

Routine Description:

   NfcCxPowerSet Policy handles the call from IOCTL_NFCRM_SET_RADIO_STATE.
   
   Based on the desired Power setting will disable IO forwarding between
   the non power managed and power managed queue and stop the later.

Arguments:

    FdoContext - Pointer to the FDO Context
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

    WdfWaitLockAcquire(FdoContext->PowerPolicyWaitLock, NULL);

    //
    // Validate that the caller is not within the app container
    //
    if (FileContext->IsAppContainerProcess) {
        TRACE_LINE(LEVEL_ERROR, "SetPower cannot be called from the app container process");
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    if (RadioState->SystemStateUpdate) {
        FdoContext->NfpPowerOffSystemOverride = RadioState->MediaRadioOn ? FALSE : TRUE;
    }
    else {
        //
        // Since the request is for modifying the radio state
        // we override the system state.
        //
        FdoContext->NfpPowerOffSystemOverride = FALSE;
        FdoContext->NfpPowerOffPolicyOverride = RadioState->MediaRadioOn ? FALSE : TRUE;
    }

    desiredRadioState = !FdoContext->NfpPowerOffPolicyOverride && !FdoContext->NfpPowerOffSystemOverride;

    if (FdoContext->NfpRadioState == desiredRadioState) {
        TRACE_LINE(LEVEL_ERROR, "We are already in the requested power state");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    //
    // Update the radio state here so that when we update the polling loop
    // configuration of the controller so that the correct states are used
    //
    currentRadioState = FdoContext->NfpRadioState;
    FdoContext->NfpRadioState = desiredRadioState;

    TRACE_LINE(LEVEL_INFO, "Current state %d, Desired State %d", 
                                                    currentRadioState,
                                                    RadioState->MediaRadioOn);
    if (desiredRadioState) {
        //
        // If there are power policy references, it means that we have called WdfDeviceResumeIdle,
        // as a result, we must stop it again here.
        //
        if (FdoContext->NfpPowerPolicyReferences > 0) {
            FdoContext->NfpPowerPolicyReferences--;
        }
        
        if (0 != FdoContext->NfpPowerPolicyReferences) {
            status = WdfDeviceStopIdle(FdoContext->Device, TRUE);
            
            if (NT_SUCCESS(status)) {
                NfcCxRFInterfaceUpdateDiscoveryState(FdoContext->RFInterface);
            }
        }

        //
        // Start the appropriate modules
        //
        NfcCxSCInterfaceStart(FdoContext->SCInterface);
        NfcCxNfpInterfaceStart(FdoContext->NfpInterface);

    } else {
        //
        // Stop the appropriate modules
        //
        NfcCxSCInterfaceStop(FdoContext->SCInterface);
        NfcCxNfpInterfaceStop(FdoContext->NfpInterface);

        //
        // We are turning off.
        // If there are power policy references, it means that we have called WdfDeviceIdleStop,
        // as a result, we must resume idle detection
        //
        if (0 != FdoContext->NfpPowerPolicyReferences) {
            //
            // Add a power policy reference so that when all the handles are released, 
            // we do not call WdfDeviceResumeIdle again.
            //
            FdoContext->NfpPowerPolicyReferences++;
            
            NfcCxRFInterfaceUpdateDiscoveryState(FdoContext->RFInterface);
            WdfDeviceResumeIdle(FdoContext->Device);
        }
    }
    
    TRACE_LINE(LEVEL_INFO, "Current Power State = %!BOOLEAN!", FdoContext->NfpRadioState);
    EventWritePowerSetRadioState(FdoContext->NfpRadioState);

Done:
    //
    // Persist the data into the registry
    //
    NfcCxFdoWritePersistedDeviceRegistrySettings(FdoContext);
    WdfWaitLockRelease(FdoContext->PowerPolicyWaitLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxPowerQueryRadioState(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _Out_ PNFCRM_RADIO_STATE RadioState
    )
/*++

Routine Description:

   NfcCxPowerQuery Policy handles the call from IOCTL_NFCRM_QUERY_RADIO_STATE.
   
   The function will return the current power state of the device.

Arguments:

    FdoContext - Pointer to the FDO Context
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

    WdfWaitLockAcquire(FdoContext->PowerPolicyWaitLock, NULL);
    RadioState->MediaRadioOn = (FdoContext->NfpRadioState);
    WdfWaitLockRelease(FdoContext->PowerPolicyWaitLock);

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

BOOLEAN
NfcCxPowerIsAllowedNfp(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    )
/*++

Routine Description:

   NfcCxPowerIsAllowedNfp returns the current NFP radio state.

Arguments:

    FdoContext - Pointer to the FDO Context

Return Value:
    TRUE - If the radio policy hasn't been overwriten to OFF
    FALSE - Otherwise

--*/
{
    BOOLEAN isAllowed = FALSE;

    WdfWaitLockAcquire(FdoContext->PowerPolicyWaitLock, NULL);
    isAllowed = FdoContext->NfpRadioState;
    WdfWaitLockRelease(FdoContext->PowerPolicyWaitLock);

    return isAllowed;
}

BOOLEAN
NfcCxPowerIsAllowedSE(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    )
/*++

Routine Description:

   NfcCxPowerIsAllowedSE returns the current SE radio state.

Arguments:

    FdoContext - Pointer to the FDO Context

Return Value:
    TRUE - If the radio policy hasn't been overwriten to OFF
    FALSE - Otherwise

--*/
{
    BOOLEAN isAllowed = FALSE;

    WdfWaitLockAcquire(FdoContext->PowerPolicyWaitLock, NULL);
    isAllowed = FdoContext->SERadioState;
    WdfWaitLockRelease(FdoContext->PowerPolicyWaitLock);

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
        break;
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

            status = NfcCxPowerSetRadioState(fdoContext,
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

            status = NfcCxPowerQueryRadioState(fdoContext,
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
