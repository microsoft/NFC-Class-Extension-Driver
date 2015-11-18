/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    Driver.cpp

Abstract:

    Base driver implementation for the NFC Class Extension
    
Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"
#include "driver.tmh"

// Allocate storage for the provider and define a corresponding TraceLoggingHProvider variable
TRACELOGGING_DEFINE_PROVIDER(g_hNfcCxProvider,
    "Microsoft.Windows.Nfc.NfcCx",
    (0x6E6BACF6, 0x5635, 0x4670, 0xBE, 0xdf, 0x93, 0xf5, 0x5a, 0x82, 0x2f, 0x4b),
    TraceLoggingOptionMicrosoftTelemetry());

WDF_CLASS_LIBRARY_INFO WdfClassLibraryInfo =
{
    sizeof(WDF_CLASS_LIBRARY_INFO),    // Size
    {
        1,  // Major
        0,  // Minor
        0,  // Buid
    },                                  // Version
    NfcCxInitialize,                    // ClassLibraryInitialize
    NfcCxDeinitialize,                  // ClassLibraryDeinitialize
    NfcCxBindClient,                    // ClassLibraryBindClient
    NfcCxUnbindClient,                  // ClassLibraryUnbindClient
};

BOOL WINAPI
DllMain(
    HINSTANCE ModuleHandle,
    DWORD Reason,
    PVOID Reserved
    )
{
    UNREFERENCED_PARAMETER(ModuleHandle);
    UNREFERENCED_PARAMETER(Reserved);

    if (DLL_PROCESS_ATTACH == Reason) {
#ifndef WPP_MACRO_USE_KM_VERSION_FOR_UM
        WPP_INIT_TRACING(NFCCX_TRACING_ID);
#endif
        EventRegisterMicrosoft_Windows_NFC_ClassExtension();
        TraceLoggingRegister(g_hNfcCxProvider);
    }
    else if (DLL_PROCESS_DETACH == Reason) {
#ifndef WPP_MACRO_USE_KM_VERSION_FOR_UM
        WPP_CLEANUP();
#endif
        EventUnregisterMicrosoft_Windows_NFC_ClassExtension();
        TraceLoggingUnregister(g_hNfcCxProvider);
    }

    return TRUE;
}

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object
    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful, error code otherwise.

--*/
{
    NTSTATUS status;
    WDF_DRIVER_CONFIG config;
    WDFDRIVER hDriver;

#ifdef WPP_MACRO_USE_KM_VERSION_FOR_UM
    WPP_INIT_TRACING(DriverObject, RegistryPath);
#endif

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WDF_DRIVER_CONFIG_INIT(&config, NULL);
    config.DriverInitFlags = WdfDriverInitNonPnpDriver;
    config.DriverPoolTag = NFCCX_POOL_TAG;
    config.EvtDriverUnload = NfcCxEvtDriverUnload;

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             &hDriver);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "WdfDriverCreate failed status = %!STATUS!", status);
#ifdef WPP_MACRO_USE_KM_VERSION_FOR_UM
        WPP_CLEANUP(DriverObject);
#endif
        goto Done;
    }

    status = WdfRegisterClassLibrary(&WdfClassLibraryInfo,
                                     RegistryPath,
                                     NULL);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "WdfRegisterClassLibrary failed status = %!STATUS!", status);
#ifdef WPP_MACRO_USE_KM_VERSION_FOR_UM
        WPP_CLEANUP(DriverObject);
#endif
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxEvtDriverUnload(
    _In_ WDFDRIVER Driver
    )
/*++

Routine Description:

    Driver Unload routine.  While the implementation of this
    function doesn't actually do anything, it is required to tell
    WDF that our driver is unloadable.

Arguments:

    DriverObject - pointer to the driver object

Return Value:

    None

--*/
{
    UNREFERENCED_PARAMETER(Driver);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);

#ifdef WPP_MACRO_USE_KM_VERSION_FOR_UM
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));
#endif
}

NTSTATUS
NfcCxEvtDeviceInitConfig(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ PWDFDEVICE_INIT DeviceInit,
    _In_ PNFC_CX_CLIENT_CONFIG Config
    )
/*++
Routine Description:

    NfcCxEvtDeviceInitConfig is called by the client driver from within
    its AddDevice callback to finish initializing the device before creation.

Arguments:

    NfcCxGlobals - Pointer to the Class extension globals
    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.
    Config - Pointer to a client provided structure with their configuration.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                          status = STATUS_SUCCESS; 
    WDFCX_FILEOBJECT_CONFIG           fileConfig;
    WDF_OBJECT_ATTRIBUTES             fileObjectAttributes;
    PWDFCXDEVICE_INIT                 exDeviceInit;
    PNFCCX_CLIENT_GLOBALS             nfcCxClientGlobal;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!VerifyPrivateGlobals(NfcCxGlobals)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid CX global pointer");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (NULL == Config ||
        sizeof(NFC_CX_CLIENT_CONFIG) != Config->Size ||
        NULL == Config->EvtNfcCxWriteNciPacket) {
        TRACE_LINE(LEVEL_ERROR, "Invalid Client Driver Configuration");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (NFC_CX_DEVICE_MODE_NCI != Config->DeviceMode &&
        NFC_CX_DEVICE_MODE_DTA != Config->DeviceMode &&
        NFC_CX_DEVICE_MODE_RAW != Config->DeviceMode) {
        TRACE_LINE(LEVEL_ERROR, "Invalid Client Driver Device Mode %d", Config->DeviceMode);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    nfcCxClientGlobal = GetPrivateGlobals(NfcCxGlobals);

    //
    // The CX is the power policy owner of this stack.
    //
    WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, TRUE);

    //
    // Allocate the CX DeviceInit
    //
    exDeviceInit = WdfCxDeviceInitAllocate(DeviceInit);
    if (NULL == exDeviceInit) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_LINE(LEVEL_ERROR,
            "Failed WdfCxDeviceInitAllocate, status=%!STATUS!", status);
        goto Done;
    }

    //
    // Enable SelfIoTarget
    //
    WdfDeviceInitAllowSelfIoTarget(DeviceInit);

    //
    // Initialize WDF_FILEOBJECT_CONFIG_INIT struct to tell the
    // per handle (fileobject) context.
    //
    WDFCX_FILEOBJECT_CONFIG_INIT(
                            &fileConfig,
                            NfcCxEvtDeviceFileCreate,
                            NfcCxEvtFileClose,
                            WDF_NO_EVENT_CALLBACK // not interested in Cleanup
                            );

    WDF_OBJECT_ATTRIBUTES_INIT(&fileObjectAttributes);
    fileObjectAttributes.SynchronizationScope = WdfSynchronizationScopeNone;

    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&fileObjectAttributes,
                                            NFCCX_FILE_CONTEXT);

    WdfCxDeviceInitSetFileObjectConfig(exDeviceInit,
                                       &fileConfig,
                                       &fileObjectAttributes);

    TRACE_LINE(LEVEL_INFO, "DriverFlags=0x%x PowerIdleType=%d PowerIdleTimeout=%d",
                            Config->DriverFlags, Config->PowerIdleType, Config->PowerIdleTimeout);

    TraceLoggingWrite(g_hNfcCxProvider,
        "NfcCxClientConfig",
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
        TraceLoggingHexInt32(Config->DriverFlags, "DriverFlags"),
        TraceLoggingValue((INT32)(Config->PowerIdleType), "PowerIdleType"),
        TraceLoggingValue(Config->PowerIdleTimeout, "PowerIdleTimeout"));

    //
    // Save the client driver configs
    //
    RtlCopyMemory(&nfcCxClientGlobal->Config, Config, Config->Size);

Done:
    
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);
    
    return status;
}

NTSTATUS
NfcCxEvtDeviceInitialize(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This routine is called by the CX client to indicate
    that a device initialization is required.

Arguments:

    NfcCxGlobal - CX global pointer
    Device - WDF device to initialize

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                              status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES                 fdoAttributes;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_IO_QUEUE_CONFIG                   queueConfig;
    PNFCCX_FDO_CONTEXT                    fdoContext;
    WDFQUEUE                              queue;
    PNFCCX_CLIENT_GLOBALS                 nfcCxClientGlobal;
    WDF_OBJECT_ATTRIBUTES                 objectAttrib;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!VerifyPrivateGlobals(NfcCxGlobals)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid CX global pointer");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    nfcCxClientGlobal = GetPrivateGlobals(NfcCxGlobals);

    //
    // Create class extension device context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, NFCCX_FDO_CONTEXT);
    fdoAttributes.EvtCleanupCallback = NfcCxFdoContextCleanup;
    
    status = WdfObjectAllocateContext(Device, &fdoAttributes, (PVOID*)&fdoContext);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the client context");
        goto Done;
    }
    fdoContext->Device = Device;
    fdoContext->NfpRadioInterfaceCreated = FALSE;
    fdoContext->NfpPowerOffSystemOverride = FALSE;
    fdoContext->NfpPowerOffPolicyOverride = FALSE;
    fdoContext->NfpPowerPolicyReferences = 0;
    fdoContext->SERadioInterfaceCreated = FALSE;
    fdoContext->SEPowerOffSystemOverride = FALSE;
    fdoContext->SEPowerOffPolicyOverride = FALSE;
    fdoContext->SEPowerPolicyReferences = 0;
    fdoContext->NfcCxClientGlobal = nfcCxClientGlobal;

    status = NfcCxFdoReadCxDriverRegistrySettings(&fdoContext->LogNciDataMessages);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "NfcCxFdoReadCxDriverRegistrySettings failed, %!STATUS!", status);
        goto Done;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = Device;
    
    status = WdfWaitLockCreate(&objectAttrib,
                                &fdoContext->PowerPolicyWaitLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the PowerPolicy WaitLock, %!STATUS!", status);
        goto Done;
    }
    
    //
    // Register I/O callbacks to tell the framework that you are interested
    // in handling IRP_MJ_DEVICE_CONTROL requests.
    //
    // In case a specific handler is not specified for one of these,
    // the request will be dispatched to the EvtIoDefault handler, if any.
    // If there is no EvtIoDefault handler, the request will be failed with
    // STATUS_INVALID_DEVICE_REQUEST.
    //
    // WdfIoQueueDispatchParallel means that we are capable of handling
    // all the I/O request simultaneously and we are responsible for protecting
    // data that could be accessed by these callbacks simultaneously.
    // A default queue gets all the requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
                             WdfIoQueueDispatchParallel);

    //
    // Our default queue is non power managed. Based on the request and the Radio on/off
    // state, we forward the request to the power managed queue to wake the system as
    // appropriate
    //
    queueConfig.PowerManaged = WdfFalse;
    queueConfig.EvtIoDeviceControl = NfcCxEvtDefaultIoControl;

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = Device;

    status = WdfIoQueueCreate(Device,
                                &queueConfig,
                                &objectAttrib,
                                &queue
                                );
    if (!NT_SUCCESS (status)) {
        TRACE_LINE(LEVEL_ERROR, "WdfIoQueueCreate failed %!STATUS!", status);
        goto Done;
    }
    fdoContext->DefaultQueue = queue;

    //
    // Our internal queue is non power managed because we need to send IO during.
    // D0Entry/D0Exit routines. It is the default queue for SelfIoTarget requests.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                             WdfIoQueueDispatchParallel);

    queueConfig.PowerManaged = WdfFalse;
    queueConfig.EvtIoDeviceControl = NfcCxEvtSelfIoControl;

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = Device;

    status = WdfIoQueueCreate(Device,
                                &queueConfig,
                                &objectAttrib,
                                &queue
                                );
    if (!NT_SUCCESS (status)) {
        TRACE_LINE(LEVEL_ERROR, "WdfIoQueueCreate failed %!STATUS!", status);
        goto Done;
    }
    fdoContext->SelfQueue = queue;

    //
    // Assign our internal queue as the default queue for the SelfIoTarget. Any IO
    // sent to the SelfIoTarget would be dispatched from this queue.
    //
    status = WdfIoTargetSelfAssignDefaultIoQueue(WdfDeviceGetSelfIoTarget(Device),
                                                 fdoContext->SelfQueue);
    if (!NT_SUCCESS (status)) {
        TRACE_LINE(LEVEL_ERROR, "WdfIoTargetSelfAssignDefaultIoQueue failed %!STATUS!", status);
        goto Done;
    }

    status = NfcCxFdoCreate(fdoContext);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the Fdo context failed %!STATUS!", status);
        goto Done;
    }

    if (NFC_CX_DEVICE_MODE_NCI == fdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        //
        // Read the FDO's persisted settings from the registry
        //
        status = NfcCxFdoReadPersistedDeviceRegistrySettings(fdoContext);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "NfcCxFdoReadPersistedDeviceRegistrySettings, %!STATUS!", status);
            goto Done;
        }

        //
        // Log all settings that could have been overridden from the registry.
        //
        EventWriteDevicePersistedRegistrySettings(fdoContext->NfpPowerOffPolicyOverride,
                                                  fdoContext->NfpPowerOffSystemOverride,
                                                  fdoContext->SEPowerOffPolicyOverride,
                                                  fdoContext->SEPowerOffSystemOverride);

        //
        // Check the currently required power state
        //
        fdoContext->NfpRadioState = (!fdoContext->NfpPowerOffPolicyOverride && !fdoContext->NfpPowerOffSystemOverride);
        fdoContext->SERadioState = (!fdoContext->SEPowerOffPolicyOverride && !fdoContext->SEPowerOffSystemOverride);
    }

    //
    // The class extension is the default power policy owner. This option allows the client
    // to now be the power policy owner
    //
    if (fdoContext->NfcCxClientGlobal->Config.IsPowerPolicyOwner == WdfUseDefault) {
        fdoContext->NfcCxClientGlobal->Config.IsPowerPolicyOwner = WdfFalse;
    }

    if (fdoContext->NfcCxClientGlobal->Config.IsPowerPolicyOwner == WdfFalse) {
        //
        // Set the idle power policy to put the device to Dx if the device is not used
        // for the specified IdleTimeout time. Since this is a non wakeable device we
        // tell the framework that we cannot wake ourself if we sleep in S0. Only
        // way the device can be brought to D0 is if the device recieves an I/O from
        // the system.
        //
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
        idleSettings.IdleTimeoutType = fdoContext->NfcCxClientGlobal->Config.PowerIdleType;
        idleSettings.IdleTimeout = fdoContext->NfcCxClientGlobal->Config.PowerIdleTimeout;

        status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "WdfDeviceAssignS0IdleSettings failed %!STATUS!", status);
            goto Done;
        }
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);
    
    return status;
}

NTSTATUS
NfcCxEvtDeviceDeinitialize(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This routine is called by the CX client to indicate
    that a device deinitialization is required.

Arguments:

    NfcCxGlobal - CX global pointer
    Device - WDF device to initialize

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!VerifyPrivateGlobals(NfcCxGlobals)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid CX global pointer");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    fdoContext = NfcCxFdoGetContext(Device);

    status = NfcCxFdoCleanup(fdoContext);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to cleanup the Fdo context failed %!STATUS!", status);
        goto Done;
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);
    
    return status;
}

NTSTATUS
NfcCxEvtHardwareEvent(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ PNFC_CX_HARDWARE_EVENT NciCxHardwareEventParams
    )
/*++

Routine Description:

    This routine is called by the CX client to indicate
    that a Hardware Event has occured.

Arguments:

    NfcCxGlobal - CX global pointer
    Device - WDF device to initialize
    NciCxHardwareEventParams - A pointer to a hardware event description structure

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!VerifyPrivateGlobals(NfcCxGlobals)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid CX global pointer");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    fdoContext = NfcCxFdoGetContext(Device);

    TRACE_LINE(LEVEL_INFO, "Client signalled an event, %!STATUS!, %!NFC_CX_HOST_ACTION!", 
                NciCxHardwareEventParams->HardwareStatus,
                NciCxHardwareEventParams->HostAction);

    switch (NciCxHardwareEventParams->HostAction) {
    case HostActionStart:
        status = NfcCxFdoInitialize(fdoContext);
        if (!NT_SUCCESS(status)) {
            NfcCxFdoDeInitialize(fdoContext);
            TRACE_LINE(LEVEL_ERROR, "Failed to initialize the Fdo, %!STATUS!", status);
            goto Done;
        }
        break;
    case HostActionStop:
        status = NfcCxFdoDeInitialize(fdoContext);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to NfcCxFdoDeInitialize the Fdo, %!STATUS!", status);
            goto Done;
        }
        break;
    case HostActionRestart:
        WdfDeviceSetFailed(Device, WdfDeviceFailedAttemptRestart);
        break;
    case HostActionUnload:
        WdfDeviceSetFailed(Device, WdfDeviceFailedNoRestart);
        break;
    default:
        TRACE_LINE(LEVEL_ERROR, "Invalid Host Action %d", NciCxHardwareEventParams->HostAction);
        break;
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    TRACE_LOG_NTSTATUS_ON_FAILURE(status);

    return status;
}

NTSTATUS
NfcCxEvtNciReadNotification(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE    Device,
    _In_ WDFMEMORY    Memory
    )
/*++

Routine Description:

    This routine is called by the CX client to signal a
    read notification.  The implementation of this function forwards
    the content of the notification to the Tml layer which will complete
    a pended read request up to the NfcLib.

Arguments:

    NfcCxGlobal - CX global pointer
    Device - WDF device to initialize
    Memory - A pointer to a WDFMEMORY object that contains the 
             content of the read notification

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    
    PUCHAR buffer = NULL;
    size_t bufferSize = 0;

    UNREFERENCED_PARAMETER(NfcCxGlobals);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    buffer = (PUCHAR)WdfMemoryGetBuffer(Memory, &bufferSize);

    if (MAX_USHORT < bufferSize) {
        TRACE_LINE(LEVEL_ERROR, "Invalid read notification sent, ignoring!!!");
        NT_ASSERTMSG("ReadNotification too large", FALSE);
        goto Done;
    }

    //
    // Forward the read to the Tml interface
    //
    status = NfcCxTmlDispatchReadNotification((NfcCxFdoGetContext(Device))->TmlInterface,
                                              buffer,
                                              (USHORT)bufferSize);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to dispatch read notification, %!STATUS!", status);
        goto Done;
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxEvtSetRfDiscoveryConfig(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ PCNFC_CX_RF_DISCOVERY_CONFIG Config
    )
/*++

Routine Description:

    This routine is called by the CX client to configure
    the RF discovery settings.

Arguments:

    NfcCxGlobal - CX global pointer
    Device - WDF device to initialize
    Config - Pointer to a structure containing the Discovery configuration

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!VerifyPrivateGlobals(NfcCxGlobals)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid CX global pointer");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (NULL == Config ||
        sizeof(*Config) != Config->Size) {
        TRACE_LINE(LEVEL_ERROR, "Invalid Client Driver Configuration");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    fdoContext = NfcCxFdoGetContext(Device);

    if (fdoContext->RFInterface == NULL) {
        TRACE_LINE(LEVEL_ERROR, "CX not initialized");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    status = NfcCxRFInterfaceSetDiscoveryConfig(fdoContext->RFInterface, Config);

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxEvtSetLlcpConfig(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ PCNFC_CX_LLCP_CONFIG Config
    )
/*++

Routine Description:

    This routine is called by the CX client to configure
    the RF discovery settings.

Arguments:

    NfcCxGlobal - CX global pointer
    Device - WDF device to initialize
    Config - Pointer to a structure containing the LLCP configuration

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!VerifyPrivateGlobals(NfcCxGlobals)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid CX global pointer");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (NULL == Config ||
        sizeof(*Config) != Config->Size) {
        TRACE_LINE(LEVEL_ERROR, "Invalid Client Driver Configuration");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    fdoContext = NfcCxFdoGetContext(Device);

    if (fdoContext->RFInterface == NULL) {
        TRACE_LINE(LEVEL_ERROR, "CX not initialized");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    status = NfcCxRFInterfaceSetLLCPConfig(fdoContext->RFInterface, Config);

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxEvtRegisterSequenceHandler(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ NFC_CX_SEQUENCE Sequence,
    _In_ PFN_NFC_CX_SEQUENCE_HANDLER EvtNfcCxSequenceHandler
    )
/*++

Routine Description:

    This routine is called by the CX client to register
    for a sequence handler

Arguments:

    NfcCxGlobal - CX global pointer
    Device - WDF device to initialize
    Sequence - The sequence to register
    EvtNfcCxSequenceHandler - The sequence handler callback

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!VerifyPrivateGlobals(NfcCxGlobals)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid CX global pointer");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (NULL == EvtNfcCxSequenceHandler ||
        SequenceMaximum <= Sequence) {
        TRACE_LINE(LEVEL_ERROR, "Invalid Client Driver Parameters");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    fdoContext = NfcCxFdoGetContext(Device);

    if (fdoContext->RFInterface == NULL) {
        TRACE_LINE(LEVEL_ERROR, "CX not initialized");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    TraceLoggingWrite(
        g_hNfcCxProvider,
        "NfcCxEvtRegisterSequence",
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
        TraceLoggingValue((DWORD)Sequence, "Sequence"));

    status = NfcCxRFInterfaceRegisterSequenceHandler(fdoContext->RFInterface, Sequence, EvtNfcCxSequenceHandler);

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxEvtUnregisterSequenceHandler(
    _In_ PNFCCX_DRIVER_GLOBALS NfcCxGlobals,
    _In_ WDFDEVICE Device,
    _In_ NFC_CX_SEQUENCE Sequence
    )
/*++

Routine Description:

    This routine is called by the CX client to unregister
    a previously registered sequence handler

Arguments:

    NfcCxGlobal - CX global pointer
    Device - WDF device to initialize
    Sequence - The sequence to unregister

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!VerifyPrivateGlobals(NfcCxGlobals)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid CX global pointer");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (SequenceMaximum <= Sequence) {
        TRACE_LINE(LEVEL_ERROR, "Invalid Client Driver Parameters");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    fdoContext = NfcCxFdoGetContext(Device);

    if (fdoContext->RFInterface == NULL) {
        TRACE_LINE(LEVEL_ERROR, "CX not initialized");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    TraceLoggingWrite(
        g_hNfcCxProvider,
        "NfcCxEvtUnregisterSequence",
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
        TraceLoggingValue((DWORD)Sequence, "sequence"));

    status = NfcCxRFInterfaceUnregisterSequenceHandler(fdoContext->RFInterface, Sequence);

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxInitialize(
    VOID
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);

    return STATUS_SUCCESS;
}

VOID
NfcCxDeinitialize(
    VOID
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxBindClient(
    PWDF_CLASS_BIND_INFO ClassInfo,
    PWDF_COMPONENT_GLOBALS ClientGlobals
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_CLIENT_GLOBALS pGlobals;

    static const PFN_NFC_CX exports[] =
    {
        (PFN_NFC_CX)NfcCxEvtDeviceInitConfig,
        (PFN_NFC_CX)NfcCxEvtDeviceInitialize,
        (PFN_NFC_CX)NfcCxEvtDeviceDeinitialize,
        (PFN_NFC_CX)NfcCxEvtHardwareEvent,
        (PFN_NFC_CX)NfcCxEvtNciReadNotification,
        (PFN_NFC_CX)NfcCxEvtSetRfDiscoveryConfig,
        (PFN_NFC_CX)NfcCxEvtSetLlcpConfig,
        (PFN_NFC_CX)NfcCxEvtRegisterSequenceHandler,
        (PFN_NFC_CX)NfcCxEvtUnregisterSequenceHandler,
    };

    UNREFERENCED_PARAMETER(ClientGlobals);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    if (ClassInfo->FunctionTableCount != ARRAYSIZE(exports)) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    CopyMemory(ClassInfo->FunctionTable, &exports[0], sizeof(exports));

    pGlobals = (PNFCCX_CLIENT_GLOBALS) malloc(sizeof(NFCCX_CLIENT_GLOBALS));
    if (pGlobals == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    RtlZeroMemory(pGlobals, sizeof(NFCCX_CLIENT_GLOBALS));
    pGlobals->Signature = GLOBALS_SIG;

    *((PNFCCX_DRIVER_GLOBALS*)ClassInfo->ClassBindInfo) = &pGlobals->Public;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
NfcCxUnbindClient(
    PWDF_CLASS_BIND_INFO ClassInfo,
    PWDF_COMPONENT_GLOBALS ClientGlobals
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_CLIENT_GLOBALS pGlobals;
    UNREFERENCED_PARAMETER(ClientGlobals);
    
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (ClassInfo->ClassBindInfo == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    if (!VerifyPrivateGlobals(*((PNFCCX_DRIVER_GLOBALS*)ClassInfo->ClassBindInfo))) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    pGlobals = GetPrivateGlobals(
        *((PNFCCX_DRIVER_GLOBALS*)ClassInfo->ClassBindInfo)
        );

    if (pGlobals == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    free (pGlobals);
    *((PVOID*)ClassInfo->ClassBindInfo) = NULL;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}
