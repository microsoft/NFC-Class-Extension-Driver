//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include "Precomp.h"
#include "Device.h"
#include "FileObject.h"

#include "Device.tmh"

// Sets up a new device.
NTSTATUS
DeviceContext::DeviceAdd(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    NFC_CX_CLIENT_CONFIG nfcCxConfig;
    NFC_CX_CLIENT_CONFIG_INIT(&nfcCxConfig, NFC_CX_TRANSPORT_CUSTOM);
    nfcCxConfig.EvtNfcCxWriteNciPacket = WriteNciPacketCallback;
    nfcCxConfig.EvtNfcCxDeviceIoControl = DeviceIoCallback;
    nfcCxConfig.DriverFlags = NFC_CX_DRIVER_ENABLE_EEPROM_WRITE_PROTECTION;

    status = NfcCxDeviceInitConfig(DeviceInit, &nfcCxConfig);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "NfcCxDeviceInitConfig failed. %!STATUS!", status);
        return status;
    }

    WDF_OBJECT_ATTRIBUTES deviceObjectAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceObjectAttributes, DeviceContext);
    deviceObjectAttributes.EvtCleanupCallback = ShutdownCallback;
    deviceObjectAttributes.EvtDestroyCallback = DestroyCallback;

    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDeviceD0Entry = D0EntryCallback;
    pnpCallbacks.EvtDeviceD0Exit = D0ExitCallback;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

    FileObjectContext::DeviceInit(DeviceInit);

    WDFDEVICE device;
    status = WdfDeviceCreate(&DeviceInit, &deviceObjectAttributes, &device);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfDeviceCreate failed. %!STATUS!", status);
        return status;
    }

    // Initialize device context.
    DeviceContext* deviceContext = DeviceGetContext(device);
    new (deviceContext) DeviceContext();
    deviceContext->_Device = device;

    status = NfcCxDeviceInitialize(device);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "NfcCxDeviceInitialize failed. %!STATUS!", status);
        return status;
    }

    // Create lock for clients (and sequence handling).
    WDF_OBJECT_ATTRIBUTES lockAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&lockAttributes);
    lockAttributes.ParentObject = device;

    status = WdfWaitLockCreate(&lockAttributes, &deviceContext->_ClientLock);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfWaitLockCreate failed. %!STATUS!", status);
        return status;
    }

    // Initialize RF discovery config (enable everything).
    NFC_CX_RF_DISCOVERY_CONFIG discoveryConfig;
    NFC_CX_RF_DISCOVERY_CONFIG_INIT(&discoveryConfig);
    discoveryConfig.PollConfig = NFC_CX_POLL_NFC_A | NFC_CX_POLL_NFC_B | NFC_CX_POLL_NFC_F_212 | NFC_CX_POLL_NFC_F_424 | NFC_CX_POLL_NFC_15693 | NFC_CX_POLL_NFC_ACTIVE | NFC_CX_POLL_NFC_A_KOVIO;
    discoveryConfig.NfcIPMode = NFC_CX_NFCIP_NFC_A | NFC_CX_NFCIP_NFC_F_212 | NFC_CX_NFCIP_NFC_F_424 | NFC_CX_NFCIP_NFC_ACTIVE | NFC_CX_NFCIP_NFC_ACTIVE_A | NFC_CX_NFCIP_NFC_ACTIVE_F_212 | NFC_CX_NFCIP_NFC_ACTIVE_F_424;
    discoveryConfig.NfcIPTgtMode = NFC_CX_NFCIP_TGT_NFC_A | NFC_CX_NFCIP_TGT_NFC_F | NFC_CX_NFCIP_TGT_NFC_ACTIVE_A | NFC_CX_NFCIP_TGT_NFC_ACTIVE_F;
    discoveryConfig.NfcCEMode = NFC_CX_CE_NFC_A | NFC_CX_CE_NFC_B | NFC_CX_CE_NFC_F;

    status = NfcCxSetRfDiscoveryConfig(device, &discoveryConfig);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "NfcCxSetRfDiscoveryConfig failed. %!STATUS!", status);
        return status;
    }

    // Set the sequence handlers.
    for (int sequenceType = 0; sequenceType != SequenceMaximum; ++sequenceType)
    {
        status = NfcCxRegisterSequenceHandler(device, NFC_CX_SEQUENCE(sequenceType), SequenceHandlerCallback);
        if (!NT_SUCCESS(status))
        {
            TRACE_LINE(LEVEL_ERROR, "NfcCxRegisterSequenceHandler failed. %!STATUS!", status);
            return status;
        }
    }

    // Initialize callbacks manager.
    status = deviceContext->_ApiCallbacksManager.Initialize(device);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "CallbacksManager::Initialize failed. %!STATUS!", status);
        return status;
    }

    // Create device interface for NciSim
    DECLARE_CONST_UNICODE_STRING(interfaceName, FILE_NAMESPACE_NCI_SIMULATOR);
    status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_NCI_SIMULATOR, &interfaceName);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfDeviceCreateDeviceInterface failed. %!STATUS!", status);
        return status;
    }

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

void
DeviceContext::DestroyCallback(
    _In_ WDFOBJECT Object
    )
{
    DeviceContext* deviceContext = DeviceGetContext(Object);
    deviceContext->~DeviceContext();
}

void
DeviceContext::ShutdownCallback(
    _In_ WDFOBJECT Object
    )
{
    DeviceContext* deviceContext = DeviceGetContext(Object);
    deviceContext->Shutdown();
}

// Called when device is being torn-down but before any of memory backing the WDF objects have been freed.
void
DeviceContext::Shutdown()
{
}

// Called when a NciSim file handle is opened.
NTSTATUS
DeviceContext::ClientConnected(
    _In_ FileObjectContext* FileContext
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(_ClientLock, nullptr);

    // Ensure there is only a single client connected at a time.
#ifdef WDF_IS_NOT_CALLING_FILE_CLOSE
    if (_CurrentSimClient)
    {
        WdfWaitLockRelease(_ClientLock);

        NTSTATUS status = STATUS_ACCESS_DENIED;
        TRACE_LINE(LEVEL_ERROR, "A client is already connected. %!STATUS!", status);
        return status;
    }
#endif

    _CurrentSimClient = FileContext;

    WdfWaitLockRelease(_ClientLock);

    // The device's WDF lock provides a convenient way to serialize with the power event callbacks.
    WdfObjectAcquireLock(_Device);

    // We only want to start NfcCx after the first client connection, otherwise the NCI initialization
    // will fail.
    if (_DeviceState != DeviceState::FirstTcpClientNotConnected)
    {
        // Nothing to do.
        WdfObjectReleaseLock(_Device);
        TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
        return STATUS_SUCCESS;
    }

    _DeviceState = DeviceState::HostNotStarted;

    WdfObjectReleaseLock(_Device);
    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

// Called when a NciSim file handle is closed.
void
DeviceContext::ClientDisconnected(
    _In_ FileObjectContext* FileContext
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(_ClientLock, nullptr);

    if (FileContext != _CurrentSimClient)
    {
        WdfWaitLockRelease(_ClientLock);
        TRACE_LINE(LEVEL_WARNING, "Incorrect client.");
        return;
    }

    _CurrentSimClient = nullptr;

    // Acquire the pending NCI write request (if any).
    WDFREQUEST nciWriteRequest = _NciWriteRequest;
    _NciWriteRequest = nullptr;

    // Acquire the sequence completed function (if any).
    PFN_NFC_CX_SEQUENCE_COMPLETION_ROUTINE sequenceCompleted = _SequenceCompleted;
    WDFCONTEXT sequenceCompletedContext = _SequenceCompletedContext;

    _SequenceCompleted = nullptr;
    _SequenceCompletedContext = nullptr;

    WdfWaitLockRelease(_ClientLock);

    if (nciWriteRequest)
    {
        TRACE_LINE(LEVEL_WARNING, "NCI write request was not completed by test client. Completing with error.");
        WdfRequestComplete(nciWriteRequest, STATUS_CONNECTION_DISCONNECTED);
    }

    if (sequenceCompleted)
    {
        // Complete the pending sequence handler, to help prevent the driver from getting into a blocked state.
        TRACE_LINE(LEVEL_INFO, "Completing leftover sequence handler.");
        sequenceCompleted(_Device, STATUS_SUCCESS, 0, sequenceCompletedContext);
    }

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
}

NTSTATUS
DeviceContext::D0EntryCallback(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    DeviceContext* deviceContext = DeviceGetContext(Device);
    return deviceContext->D0Entry(PreviousState);
}

// Called when the device enters the D0 power state.
NTSTATUS
DeviceContext::D0Entry(
    _In_ WDF_POWER_DEVICE_STATE /*PreviousState*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    if (_DeviceState != DeviceState::HostNotStarted)
    {
        // Don't do anything
        TRACE_LINE(LEVEL_VERBOSE, "Waiting for client connection.");
        return STATUS_SUCCESS;
    }

    NFC_CX_HARDWARE_EVENT eventArgs = {};
    eventArgs.HostAction = HostActionStart;
    eventArgs.HardwareStatus = STATUS_SUCCESS;

    status = NfcCxHardwareEvent(_Device, &eventArgs);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "NfcCxHardwareEvent (HostActionStart) failed. %!STATUS!", status);
        return status;
    }

    _DeviceState = DeviceState::HostStarted;

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

NTSTATUS
DeviceContext::D0ExitCallback(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    DeviceContext* deviceContext = DeviceGetContext(Device);
    return deviceContext->D0Exit(TargetState);
}

// Called when the device exits the D0 power state.
NTSTATUS
DeviceContext::D0Exit(
    _In_ WDF_POWER_DEVICE_STATE /*TargetState*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    if (_DeviceState != DeviceState::HostStarted)
    {
        // Don't do anything
        TRACE_LINE(LEVEL_VERBOSE, "Waiting for client connection.");
        return STATUS_SUCCESS;
    }

    NFC_CX_HARDWARE_EVENT eventArgs = {};
    eventArgs.HostAction = HostActionStop;
    eventArgs.HardwareStatus = STATUS_SUCCESS;

    status = NfcCxHardwareEvent(_Device, &eventArgs);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "NfcCxHardwareEvent (HostActionStop) failed. %!STATUS!", status);
        return status;
    }

    _DeviceState = DeviceState::HostNotStarted;

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

void
DeviceContext::WriteNciPacketCallback(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    )
{
    DeviceContext* deviceContext = DeviceGetContext(Device);
    deviceContext->WriteNciPacket(Request);
}

// Called when NfcCx has an NCI packet it wishes to sent to the device.
void
DeviceContext::WriteNciPacket(
    _In_ WDFREQUEST Request
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    // Get NCI packet.
    void* nciBuffer;
    size_t nciLength;
    status = WdfRequestRetrieveInputBuffer(Request, 0, &nciBuffer, &nciLength);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to read input buffer. %!STATUS!", status);
        WdfRequestComplete(Request, status);
        return;
    }

    WdfWaitLockAcquire(_ClientLock, nullptr);

    if (_NciWriteRequest != nullptr)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        TRACE_LINE(LEVEL_ERROR, "A pending NCI write already exists. %!STATUS!", status);
        WdfWaitLockRelease(_ClientLock);
        WdfRequestComplete(Request, status);
        return;
    }

    // Post NCI write callback.
    status = _ApiCallbacksManager.PostNciWriteCallback(nciBuffer, nciLength);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "CallbacksManager::PostNciWriteCallback failed. %!STATUS!", status);
        WdfWaitLockRelease(_ClientLock);
        WdfRequestComplete(Request, status);
        return;
    }

    _NciWriteRequest = Request;

    WdfWaitLockRelease(_ClientLock);
    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
}

void
DeviceContext::DeviceIoCallback(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    DeviceContext* deviceContext = DeviceGetContext(Device);
    deviceContext->DeviceIo(Request, OutputBufferLength, InputBufferLength, IoControlCode);
}

void
DeviceContext::DeviceIo(
    _In_ WDFREQUEST Request,
    _In_ size_t /*OutputBufferLength*/,
    _In_ size_t /*InputBufferLength*/,
    _In_ ULONG IoControlCode
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WDFFILEOBJECT fileObject = WdfRequestGetFileObject(Request);
    FileObjectContext* fileContext = FileObjectGetContext(fileObject);

    using CommandFunctionType = NTSTATUS (DeviceContext::*)(_In_ WDFREQUEST Request);
    CommandFunctionType commandFunction = nullptr;
    switch (fileContext->GetType())
    {
    case FileObjectContext::Type::NciSim:
        switch (IoControlCode)
        {
        case IOCTL_NCISIM_GET_NEXT_CALLBACK:
            commandFunction = &DeviceContext::CommandGetNextCallback;
            break;

        case IOCTL_NCISIM_NCI_READ:
            commandFunction = &DeviceContext::CommandNciRead;
            break;

        case IOCTL_NCISIM_ADD_D0_POWER_REFERENCE:
            commandFunction = &DeviceContext::CommandAddD0PowerReference;
            break;

        case IOCTL_NCISIM_REMOVE_D0_POWER_REFERENCE:
            commandFunction = &DeviceContext::CommandRemoveD0PowerReference;
            break;

        case IOCTL_NCISIM_SEQUENCE_HANDLER_COMPLETE:
            commandFunction = &DeviceContext::CommandSequenceHandlerComplete;
            break;

        case IOCTL_NCISIM_NCI_WRITE_COMPLETE:
            commandFunction = &DeviceContext::CommandNciWriteComplete;
            break;
        }
        break;
    }

    NTSTATUS commandStatus = STATUS_SUCCESS;
    if (commandFunction)
    {
        commandStatus = (this->*commandFunction)(Request);
    }
    else
    {
        commandStatus = STATUS_INVALID_DEVICE_STATE;
        TRACE_LINE(LEVEL_ERROR, "Unsupported IOCTL (%d): 0x%08X. %!STATUS!", int(fileContext->GetType()), IoControlCode, commandStatus);
    }

    if (!NT_SUCCESS(commandStatus))
    {
        TRACE_LINE(LEVEL_ERROR, "Request failed. %!STATUS!", commandStatus);
        WdfRequestComplete(Request, commandStatus);
        return;
    }

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
}

void
DeviceContext::SequenceHandlerCallback(
    _In_ WDFDEVICE Device,
    _In_ NFC_CX_SEQUENCE Sequence,
    _In_ PFN_NFC_CX_SEQUENCE_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ WDFCONTEXT CompletionContext
    )
{
    DeviceContext* deviceContext = DeviceGetContext(Device);
    deviceContext->SequenceHandler(Sequence, CompletionRoutine, CompletionContext);
}

void
DeviceContext::SequenceHandler(
    _In_ NFC_CX_SEQUENCE Sequence,
    _In_ PFN_NFC_CX_SEQUENCE_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ WDFCONTEXT CompletionContext
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    WdfWaitLockAcquire(_ClientLock, nullptr);

    if (!_CurrentSimClient)
    {
        // There currently isn't a connected client. So just complete the sequence handler immediately.
        // This helps improve the robustness of the driver if a test fails.
        TRACE_LINE(LEVEL_INFO, "Leftover sequence handler.");
        WdfWaitLockRelease(_ClientLock);
        CompletionRoutine(_Device, STATUS_SUCCESS, 0, CompletionContext);
        return;
    }

    // Notify the test process that a sequence handler was invoked.
    status = _ApiCallbacksManager.PostSequenceHandlerCallback(Sequence);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "CallbacksManager::PostSequenceHandlerCallback failed. %!STATUS!", status);
        WdfWaitLockRelease(_ClientLock);
        CompletionRoutine(_Device, status, 0, CompletionContext);
        return;
    }

    _SequenceCompleted = CompletionRoutine;
    _SequenceCompletedContext = CompletionContext;

    WdfWaitLockRelease(_ClientLock);
    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
}

NTSTATUS
DeviceContext::CommandGetNextCallback(
    _In_ WDFREQUEST Request
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    _ApiCallbacksManager.HandleCallbackRequest(Request);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

NTSTATUS
DeviceContext::CommandNciRead(
    _In_ WDFREQUEST Request
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    WDFMEMORY nciPacket;
    status = WdfRequestRetrieveInputMemory(Request, &nciPacket);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfRequestRetrieveInputMemory failed. %!STATUS!", status);
        return status;
    }

    status = NfcCxNciReadNotification(_Device, nciPacket);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "NfcCxNciReadNotification failed. %!STATUS!", status);
        return status;
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

NTSTATUS
DeviceContext::CommandAddD0PowerReference(
    _In_ WDFREQUEST Request
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    status = WdfDeviceStopIdle(_Device, /*WaitForD0*/ FALSE);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfDeviceStopIdle failed. %!STATUS!", status);
        return status;
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

NTSTATUS
DeviceContext::CommandRemoveD0PowerReference(
    _In_ WDFREQUEST Request
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfDeviceResumeIdle(_Device);

    WdfRequestComplete(Request, STATUS_SUCCESS);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

NTSTATUS
DeviceContext::CommandSequenceHandlerComplete(
    _In_ WDFREQUEST Request
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    // Get request's parameters.
    NciSimSequenceHandlerComplete* params;
    status = WdfRequestRetrieveInputBuffer(Request, sizeof(NciSimSequenceHandlerComplete), reinterpret_cast<void**>(&params), nullptr);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfRequestRetrieveInputBuffer failed. %!STATUS!", status);
        return status;
    }

    WdfWaitLockAcquire(_ClientLock, nullptr);

    // Acquire the sequence completed function.
    PFN_NFC_CX_SEQUENCE_COMPLETION_ROUTINE sequenceCompleted = _SequenceCompleted;
    WDFCONTEXT sequenceCompletedContext = _SequenceCompletedContext;

    _SequenceCompleted = nullptr;
    _SequenceCompletedContext = nullptr;

    WdfWaitLockRelease(_ClientLock);

    // Ensure there is a sequence handler pending.
    if (!sequenceCompleted)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        TRACE_LINE(LEVEL_ERROR, "No sequence completion routine is pending. %!STATUS!", status);
        return status;
    }

    // Call the sequence completed function.
    sequenceCompleted(_Device, params->Status, params->Flags, sequenceCompletedContext);

    // Complete I/O request.
    WdfRequestComplete(Request, STATUS_SUCCESS);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

NTSTATUS
DeviceContext::CommandNciWriteComplete(
    _In_ WDFREQUEST Request
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    WdfWaitLockAcquire(_ClientLock, nullptr);

    if (_NciWriteRequest == nullptr)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        TRACE_LINE(LEVEL_ERROR, "No NCI write request is pending. %!STATUS!", status);
        WdfWaitLockRelease(_ClientLock);
        return status;
    }

    WDFREQUEST nciWriteRequest = _NciWriteRequest;
    _NciWriteRequest = nullptr;

    WdfWaitLockRelease(_ClientLock);

    // Complete NCI write I/O request.
    WdfRequestComplete(nciWriteRequest, STATUS_SUCCESS);

    // Complete I/O request.
    WdfRequestComplete(Request, STATUS_SUCCESS);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}
