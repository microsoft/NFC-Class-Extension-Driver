/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    device.cpp

Abstract:

    Cx Device implementation.

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"
#include "device.tmh"

NFCCX_DDI_MODULE g_NfcCxDdiModules [] =
{
    {L"RadioMedia",
        FALSE, //IsNullFileObjectOk
        FALSE, //IsAppContainerAllowed
        NfcCxPowerIsIoctlSupported,
        NfcCxPowerIoDispatch
    },
    {L"NFP",
        FALSE, //IsNullFileObjectOk
        TRUE,  //IsAppContainerAllowed
        NfcCxNfpInterfaceIsIoctlSupported,
        NfcCxNfpInterfaceIoDispatch
    },
    {L"SecureElement",
        FALSE, //IsNullFileObjectOk
        FALSE,  //IsAppContainerAllowed
        NfcCxSEInterfaceIsIoctlSupported,
        NfcCxSEInterfaceIoDispatch
    },
    {L"SmartCard",
        FALSE, //IsNullFileObjectOk
        TRUE,  //IsAppContainerAllowed
        NfcCxSCInterfaceIsIoctlSupported,
        NfcCxSCInterfaceIoDispatch
    },
    {L"DTA",
        FALSE, //IsNullFileObjectOk
        TRUE, //IsAppContainerAllowed
        NfcCxDTAInterfaceIsIoctlSupported,
        NfcCxDTAInterfaceIoDispatch
    },
};

static const NFCCX_DDI_MODULE EmbeddedSEModule =
{
    L"EmbeddedSE",
    FALSE, //IsNullFileObjectOk
    TRUE, //IsAppContainerAllowed
    NfcCxESEInterfaceIsIoctlSupported,
    NfcCxESEInterfaceIoDispatch,
};

NTSTATUS
NfcCxFdoCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    )
/*++

Routine Description:

    Create the FDO context's sub modules.

Arguments:

    FdoContext - The Fdo's context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Initialize the Power Manager
    //
    status = NfcCxPowerCreate(FdoContext, &FdoContext->Power);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initialize the power manager, %!STATUS!", status);
        goto Done;
    }

    if (NFC_CX_DEVICE_MODE_NCI == FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        //
        // Create the Tml Interface
        //
        status = NfcCxTmlInterfaceCreate(FdoContext,
                                         &FdoContext->TmlInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to create the TML Interface, %!STATUS!", status);
            goto Done;
        }

        //
        // Create the Nfp Interface
        //
        status = NfcCxNfpInterfaceCreate(FdoContext,
                                         &FdoContext->NfpInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to create the NFP Interface, %!STATUS!", status);
            goto Done;
        }

        //
        // Create the SE Interface
        //
        status = NfcCxSEInterfaceCreate(FdoContext,
                                        &FdoContext->SEInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to create the SE Interface, %!STATUS!", status);
            goto Done;
        }

        //
        // Create the SC Interface
        //
        status = NfcCxSCInterfaceCreate(FdoContext,
                                        &FdoContext->SCInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to create the SC Interface, %!STATUS!", status);
            goto Done;
        }

        //
        // Create the eSE Interface
        //
        status = NfcCxESEInterfaceCreate(FdoContext,
                                         &FdoContext->ESEInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to create the eSE Interface, %!STATUS!", status);
            goto Done;
        }

        //
        // Create the RF Interface
        //
        status = NfcCxRFInterfaceCreate(FdoContext,
                                        &FdoContext->RFInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to create the RF Interface, %!STATUS!", status);
            goto Done;
        }
    } else if (NFC_CX_DEVICE_MODE_DTA == FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        //
        // Create the Tml Interface
        //
        status = NfcCxTmlInterfaceCreate(FdoContext,
                                         &FdoContext->TmlInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to create the TML Interface, %!STATUS!", status);
            goto Done;
        }

        //
        // Create the DTA Interface
        //
        status = NfcCxDTAInterfaceCreate(FdoContext,
                                         &FdoContext->DTAInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to create the DTA Interface, %!STATUS!", status);
            goto Done;
        }
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
NfcCxFdoContextCleanup(
    _In_ WDFOBJECT Object
    )
/*++

Routine Description:

    WDF context cleanup callback.

Arguments:

    FdoContext - The Fdo's context

Return Value:

    None

--*/
{
    PNFCCX_FDO_CONTEXT fdoContext = NfcCxFdoGetContext(Object);

    if (NULL != fdoContext->RFInterface) {
        NfcCxRFInterfaceDestroy(fdoContext->RFInterface);
        fdoContext->RFInterface = NULL;
    }

    if (NULL != fdoContext->ESEInterface) {
        NfcCxESEInterfaceDestroy(fdoContext->ESEInterface);
        fdoContext->ESEInterface = NULL;
    }

    if (NULL != fdoContext->SCInterface) {
        NfcCxSCInterfaceDestroy(fdoContext->SCInterface);
        fdoContext->SCInterface = NULL;
    }

    if (NULL != fdoContext->SEInterface) {
        NfcCxSEInterfaceDestroy(fdoContext->SEInterface);
        fdoContext->SEInterface = NULL;
    }

    if (NULL != fdoContext->NfpInterface) {
        NfcCxNfpInterfaceDestroy(fdoContext->NfpInterface);
        fdoContext->NfpInterface = NULL;
    }

    if (NULL != fdoContext->DTAInterface) {
        NfcCxDTAInterfaceDestroy(fdoContext->DTAInterface);
        fdoContext->DTAInterface = NULL;
    }

    if (NULL != fdoContext->TmlInterface) {
        NfcCxTmlInterfaceDestroy(fdoContext->TmlInterface);
        fdoContext->TmlInterface = NULL;
    }

    if (NULL != fdoContext->Power)
    {
        NfcCxPowerDestroy(fdoContext->Power);
        fdoContext->Power = NULL;
    }
}

NTSTATUS
NfcCxFdoInitialize(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    )
/*++

Routine Description:

    Initializes the Fdo.  This is called when the hardware is
    indicating that it is ready for initialization.  As a result, we
    start all sub interfaces of the FDO

Arguments:

    FdoContext - The Fdo's context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    char* interfaceFailure = NULL;
    NFC_CX_DEVICE_MODE nfcCxDeviceMode = NFC_CX_DEVICE_MODE_NCI;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFC_CX_DEVICE_MODE_NCI == FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        //
        // Start Tml
        //
        status = NfcCxTmlInterfaceStart(FdoContext->TmlInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to start the Tml Interface, %!STATUS!", status);
            interfaceFailure = "TML";
            goto Done;
        }

        //
        // Start RF
        //
        status = NfcCxRFInterfaceStart(FdoContext->RFInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to start the RF Interface, %!STATUS!", status);
            interfaceFailure = "RF";
            goto Done;
        }

        //
        // Start Nfp
        //
        status = NfcCxNfpInterfaceStart(FdoContext->NfpInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to start the Nfp Interface, %!STATUS!", status);
            interfaceFailure = "NFP";
            goto Done;
        }

        //
        // Start SC
        //
        status = NfcCxSCInterfaceStart(FdoContext->SCInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to start the SC Interface, %!STATUS!", status);
            interfaceFailure = "SC";
            goto Done;
        }

        //
        // Start SE
        //
        status = NfcCxSEInterfaceStart(FdoContext->SEInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to start the SE Interface, %!STATUS!", status);
            interfaceFailure = "SE";
            goto Done;
        }

        //
        // Start eSE
        //
        status = NfcCxESEInterfaceStart(FdoContext->ESEInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to start the eSE Interface, %!STATUS!", status);
            interfaceFailure = "ESE";
            goto Done;
        }

    } else if (NFC_CX_DEVICE_MODE_DTA == FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        //
        // Start Tml
        //
        status = NfcCxTmlInterfaceStart(FdoContext->TmlInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to start the Tml Interface, %!STATUS!", status);
            interfaceFailure = "TML";
            nfcCxDeviceMode = NFC_CX_DEVICE_MODE_DTA;
            goto Done;
        }

        //
        // Start DTA
        //
        status = NfcCxDTAInterfaceStart(FdoContext->DTAInterface);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to start the DTA Interface, %!STATUS!", status);
            interfaceFailure = "DTA";
            nfcCxDeviceMode = NFC_CX_DEVICE_MODE_DTA;
            goto Done;
        }
    }

    //
    // Start Power Manager
    //
    status = NfcCxPowerStart(FdoContext->Power);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to start power manager, %!STATUS!", status);
        interfaceFailure = "POWER";
        goto Done;
    }

Done:

    if(!NT_SUCCESS(status)) {
        TraceLoggingWrite(
            g_hNfcCxProvider,
            "NfcCxFdoInitialize",
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingString(interfaceFailure, "Interface"),
            TraceLoggingUInt32(nfcCxDeviceMode, "NFCCxDeviceMode"),
            TraceLoggingHexInt32(status, "NTStatus")
        );
    }
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}



NTSTATUS
NfcCxFdoDeInitialize(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    )
/*++

Routine Description:

    This is called when the hardware is
    indicating that it is powering down.  As a result, we
    stop all sub interfaces of the FDO

Arguments:

    FdoContext - The Fdo's context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Stop Power Manager
    //
    if (NULL != FdoContext->Power)
    {
        NfcCxPowerStop(FdoContext->Power);
    }

    //
    // Stop RF
    //
    if (NULL != FdoContext->RFInterface) {
        (VOID)NfcCxRFInterfaceStop(FdoContext->RFInterface);
    }

    //
    // Stop Tml
    //
    if (NULL != FdoContext->TmlInterface) {
        (VOID)NfcCxTmlInterfaceStop(FdoContext->TmlInterface);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}


NTSTATUS
NfcCxFdoCleanup(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    )
/*++

Routine Description:

    Cleans up the Fdo.  This is called when the hardware is
    indicating that it is shutting down.  As a result, we
    stop all sub interfaces of the FDO

Arguments:

    FdoContext - The Fdo's context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NULL != FdoContext->DefaultQueue) {
        WdfIoQueueStopAndPurgeSynchronously(FdoContext->DefaultQueue);
    }

    if (NULL != FdoContext->ESEInterface) {
        NfcCxESEInterfaceDestroy(FdoContext->ESEInterface);
        FdoContext->ESEInterface = NULL;
    }

    if (NULL != FdoContext->SCInterface) {
        NfcCxSCInterfaceDestroy(FdoContext->SCInterface);
        FdoContext->SCInterface = NULL;
    }

    if (NULL != FdoContext->SEInterface) {
        NfcCxSEInterfaceDestroy(FdoContext->SEInterface);
        FdoContext->SEInterface = NULL;
    }

    if (NULL != FdoContext->NfpInterface) {
        NfcCxNfpInterfaceDestroy(FdoContext->NfpInterface);
        FdoContext->NfpInterface = NULL;
    }

    if (NULL != FdoContext->RFInterface) {
        NfcCxRFInterfaceDestroy(FdoContext->RFInterface);
        FdoContext->RFInterface = NULL;
    }

    if (NULL != FdoContext->DTAInterface) {
        NfcCxDTAInterfaceDestroy(FdoContext->DTAInterface);
        FdoContext->DTAInterface = NULL;
    }

    if (NULL != FdoContext->TmlInterface) {
        NfcCxTmlInterfaceDestroy(FdoContext->TmlInterface);
        FdoContext->TmlInterface = NULL;
    }

    if (NULL != FdoContext->Power)
    {
        NfcCxPowerDestroy(FdoContext->Power);
        FdoContext->Power = NULL;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxFdoReadCxDriverRegistrySettings(
    _Out_ BOOLEAN* pLogNciDataMessages
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFKEY hKey = NULL;
    UNICODE_STRING valueName;
    ULONG tempValue = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(pLogNciDataMessages != NULL);

    status = WdfDriverOpenParametersRegistryKey(
        WdfGetDriver(),
        PLUGPLAY_REGKEY_DEVICE,
        WDF_NO_OBJECT_ATTRIBUTES,
        &hKey);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_INFO, "No settings");
        status = STATUS_SUCCESS;
        goto Done;
    }

    status = RtlUnicodeStringInit(&valueName, NFCCX_REG_LOG_DATA_MESSAGES);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initialize string %S: %!STATUS!", NFCCX_REG_LOG_DATA_MESSAGES, status);
        goto Done;
    }

    status = WdfRegistryQueryULong(
        hKey,
        &valueName,
        &tempValue);
    if (!NT_SUCCESS(status)) {
        // Value not present, allow continuation
        status = STATUS_SUCCESS;
    } else {
        TRACE_LINE(LEVEL_INFO, "%S = %d", NFCCX_REG_LOG_DATA_MESSAGES, tempValue);
        *pLogNciDataMessages = tempValue != 0;
    }

Done:

    if (NULL != hKey) {
        WdfRegistryClose(hKey);
        hKey = NULL;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxFdoReadPersistedDeviceRegistrySettings(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    )
/*++

Routine Description:

    Reads the NfcCx persisted device registry parameters.

Arguments:

    FdoContext - The Fdo's context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFKEY hKey = NULL;
    UNICODE_STRING valueName;
    ULONG tempValue = 0;
    DECLARE_UNICODE_STRING_SIZE(guidValueString, STR_GUID_LENGTH);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ZeroMemory(&valueName, sizeof(valueName));

    status = WdfDeviceOpenRegistryKey (
                            FdoContext->Device,
                            PLUGPLAY_REGKEY_DEVICE | WDF_REGKEY_DEVICE_SUBKEY,
                            KEY_READ,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &hKey);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_INFO, "No settings");
        status = STATUS_SUCCESS;
        goto Done;
    }

    status = RtlUnicodeStringInit(&valueName, NFCCX_REG_DISABLE_POWER_MANAGER_STOP_IDLE);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initialize string %S: %!STATUS!", NFCCX_REG_DISABLE_POWER_MANAGER_STOP_IDLE, status);
        goto Done;
    }

    status = WdfRegistryQueryULong(
                            hKey,
                            &valueName,
                            &tempValue);
    if (!NT_SUCCESS(status)) {
        // Value not present, allow continuation
        status = STATUS_SUCCESS;
    } else {
        TRACE_LINE(LEVEL_INFO, "%S = %d", NFCCX_REG_DISABLE_POWER_MANAGER_STOP_IDLE, tempValue);
        FdoContext->DisablePowerManagerStopIdle = (FdoContext->DisablePowerManagerStopIdle) || (tempValue != 0);
    }

    status = RtlUnicodeStringInit(&valueName, NFCCX_REG_SESSION_IDENTIFIER);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initialize string %S: %!STATUS!", NFCCX_REG_SESSION_IDENTIFIER, status);
        goto Done;
    }

    status = WdfRegistryQueryUnicodeString(
                            hKey,
                            &valueName,
                            &guidValueString.MaximumLength,
                            &guidValueString);
    if (!NT_SUCCESS(status)) {
        // Value not present, allow continuation
        status = STATUS_SUCCESS;
    } else {
        TRACE_LINE(LEVEL_INFO, "%S = %S", NFCCX_REG_SESSION_IDENTIFIER, guidValueString.Buffer);

        //
        // Convert the string back into a guid
        //
        status = NfcCxGuidFromUnicodeString(&guidValueString,
                                            &FdoContext->RFInterface->SessionId);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_WARNING, "Failed to read the GUID value, %!STATUS!", status);
            status = STATUS_SUCCESS;
        }
    }

Done:

    if (NULL != hKey) {
        WdfRegistryClose(hKey);
        hKey = NULL;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxFdoWritePersistedDeviceRegistrySettings(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    )
/*++

Routine Description:

    Writes the NfcCx device registry parameters.

Arguments:

    FdoContext - The Fdo's context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFKEY hKey = NULL;
    UNICODE_STRING valueName;
    DECLARE_UNICODE_STRING_SIZE(guidValueString, STR_GUID_LENGTH);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ZeroMemory(&valueName, sizeof(valueName));

    status = WdfDeviceOpenRegistryKey(
                            FdoContext->Device,
                            PLUGPLAY_REGKEY_DEVICE | WDF_REGKEY_DEVICE_SUBKEY,
                            GENERIC_ALL & ~(GENERIC_WRITE | KEY_CREATE_SUB_KEY | WRITE_DAC),
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &hKey);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_INFO, "No settings");
        status = STATUS_SUCCESS;
        goto Done;
    }

    status = RtlUnicodeStringInit(&valueName, NFCCX_REG_SESSION_IDENTIFIER);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initialize string %S: %!STATUS!", NFCCX_REG_SESSION_IDENTIFIER, status);
        goto Done;
    }

    status = NfcCxUnicodeStringFromGuid(FdoContext->RFInterface->SessionId, &guidValueString);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to convert the guid to a string, %!STATUS!", status);
        goto Done;
    }

    status = WdfRegistryAssignUnicodeString(
                            hKey,
                            &valueName,
                            &guidValueString);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to persist %S = %S, %!STATUS!", NFCCX_REG_SESSION_IDENTIFIER, guidValueString.Buffer, status);
        goto Done;
    }

Done:

    if (NULL != hKey) {
        WdfRegistryClose(hKey);
        hKey = NULL;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

static NTSTATUS
NfcCxDeviceDispatchIoctl(
    _In_ NFCCX_FILE_CONTEXT* FileContext,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode,
    const NFCCX_DDI_MODULE& Module
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (NULL == FileContext &&
        !Module.IsNullFileObjectOk) {
        TRACE_LINE(LEVEL_ERROR, "%S module request received without a file context", Module.Name);
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (NULL != FileContext &&
        FileContext->IsAppContainerProcess &&
        !Module.IsAppContainerAllowed) {
        TRACE_LINE(LEVEL_ERROR, "%S module request received received from the AppContainer process", Module.Name);
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    status = Module.IoDispatch(FileContext,
                               Request,
                               OutputBufferLength,
                               InputBufferLength,
                               IoControlCode);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to forward the request to the %S module, %!STATUS!", Module.Name, status);
        goto Done;
    }

Done:
    return status;
}

VOID
NfcCxEvtDefaultIoControl(
    _In_ WDFQUEUE      Queue,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++

Routine Description:

    This is the default IOControl handler for the class extension.  It is an non power
    managed queue to ensure that we can impose our power management policies when
    the radio should remain in idle.

Arguments:

    Queue - Handle to the framework queue object that is associated with the I/O request.
    Request - Handle to a framework request object.
    OutputBufferLength - Length of the output buffer associated with the request.
    InputBufferLength - Length of the input buffer associated with the request.
    IoControlCode - IOCTL code.

Return Value:

    None.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    NFCCX_FILE_CONTEXT* fileContext = NULL;
    NFCCX_FDO_CONTEXT* fdoContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO,
               "Queue 0x%p, Request 0x%p, OutputBufferLength %Iu, InputBufferLength %Iu, IoControlCode %!NFC_IOCTL!",
               Queue,
               Request,
               OutputBufferLength,
               InputBufferLength,
               IoControlCode);

    fdoContext = NfcCxFdoGetContext(WdfIoQueueGetDevice(Queue));
    fileContext = NfcCxFileGetContext(WdfRequestGetFileObject(Request));

    if (fileContext->Role == ROLE_EMBEDDED_SE)
    {
        if (EmbeddedSEModule.IsIoctlSupported(fdoContext, IoControlCode))
        {
            status = NfcCxDeviceDispatchIoctl(fileContext, Request, OutputBufferLength, InputBufferLength, IoControlCode, EmbeddedSEModule);
            goto Done;
        }
    }
    else
    {
        for (const NFCCX_DDI_MODULE& module : g_NfcCxDdiModules)
        {
            if (module.IsIoctlSupported(fdoContext, IoControlCode))
            {
                status = NfcCxDeviceDispatchIoctl(fileContext, Request, OutputBufferLength, InputBufferLength, IoControlCode, module);
                goto Done;
            }
        }
    }

    if (NULL != fdoContext->NfcCxClientGlobal->Config.EvtNfcCxDeviceIoControl) {
        fdoContext->NfcCxClientGlobal->Config.EvtNfcCxDeviceIoControl(fdoContext->Device,
                                                                        Request,
                                                                        OutputBufferLength,
                                                                        InputBufferLength,
                                                                        IoControlCode);
        status = STATUS_PENDING;
    }
    else {
        //
        // This class extension is the bottom of the stack so complete the unknown request
        //
        TRACE_LINE(LEVEL_WARNING, "Unknown IOCTL: 0x%08lX", IoControlCode);
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

Done:

    if (STATUS_PENDING != status) {
        WdfRequestComplete(Request, status);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

VOID
NfcCxEvtSelfIoControl(
    _In_ WDFQUEUE      Queue,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++

Routine Description:

    This is the self IOControl handler for the class extension.  It is an non power
    managed queue.

Arguments:

    Queue - Handle to the framework queue object that is associated with the I/O request.
    Request - Handle to a framework request object.
    OutputBufferLength - Length of the output buffer associated with the request.
    InputBufferLength - Length of the input buffer associated with the request.
    IoControlCode - IOCTL code.

Return Value:

    None.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    NFCCX_FDO_CONTEXT* fdoContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO,
               "Queue 0x%p, Request 0x%p, OutputBufferLength %Iu, InputBufferLength %Iu, IoControlCode %!NFCCX_IOCTL!",
               Queue,
               Request,
               OutputBufferLength,
               InputBufferLength,
               IoControlCode);

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    fdoContext = NfcCxFdoGetContext(WdfIoQueueGetDevice(Queue));

    if (IoControlCode == IOCTL_NFCCX_WRITE_PACKET) {
        fdoContext->NfcCxClientGlobal->Config.EvtNfcCxWriteNciPacket(fdoContext->Device,
                                                                     Request);
        status = STATUS_PENDING;
    }
    else {
        TRACE_LINE(LEVEL_WARNING, "Unknown IOCTL: 0x%08lX", IoControlCode);
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

Done:

    if (STATUS_PENDING != status) {
        WdfRequestComplete(Request, status);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}
