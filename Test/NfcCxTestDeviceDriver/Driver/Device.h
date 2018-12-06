//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <NfcCxTestDeviceDriver.h>

#include "ApiCallbacksManager.h"

class FileObjectContext;

// Holds all the context information for a device.
class DeviceContext
{
public:
    static NTSTATUS DeviceAdd(
        _Inout_ PWDFDEVICE_INIT DeviceInit
        );
    NTSTATUS ClientConnected(
        _In_ FileObjectContext* FileContext
        );
    void ClientDisconnected(
        _In_ FileObjectContext* FileContext
        );

private:
    DeviceContext() = default;
    ~DeviceContext() = default;
    static void ShutdownCallback(
        _In_ WDFOBJECT Object
        );
    void Shutdown();
    static void DestroyCallback(
        _In_ WDFOBJECT Object
        );
    static NTSTATUS D0EntryCallback(
        _In_ WDFDEVICE Device,
        _In_ WDF_POWER_DEVICE_STATE PreviousState
        );
    NTSTATUS D0Entry(
        _In_ WDF_POWER_DEVICE_STATE PreviousState
        );
    static NTSTATUS D0ExitCallback(
        _In_ WDFDEVICE Device,
        _In_ WDF_POWER_DEVICE_STATE TargetState
        );
    NTSTATUS D0Exit(
        _In_ WDF_POWER_DEVICE_STATE TargetState
        );
    static void WriteNciPacketCallback(
        _In_ WDFDEVICE Device,
        _In_ WDFREQUEST Request
        );
    void WriteNciPacket(
        _In_ WDFREQUEST Request
        );
    static void DeviceIoCallback(
        _In_ WDFDEVICE Device,
        _In_ WDFREQUEST Request,
        _In_ size_t OutputBufferLength,
        _In_ size_t InputBufferLength,
        _In_ ULONG  IoControlCode
        );
    void DeviceIo(
        _In_ WDFREQUEST Request,
        _In_ size_t OutputBufferLength,
        _In_ size_t InputBufferLength,
        _In_ ULONG IoControlCode
        );
    static void SequenceHandlerCallback(
        _In_ WDFDEVICE Device,
        _In_ NFC_CX_SEQUENCE Sequence,
        _In_ PFN_NFC_CX_SEQUENCE_COMPLETION_ROUTINE CompletionRoutine,
        _In_opt_ WDFCONTEXT CompletionContext
        );
    void SequenceHandler(
        _In_ NFC_CX_SEQUENCE Sequence,
        _In_ PFN_NFC_CX_SEQUENCE_COMPLETION_ROUTINE CompletionRoutine,
        _In_opt_ WDFCONTEXT CompletionContext
        );
    NTSTATUS CommandGetNextCallback(
        _In_ WDFREQUEST Request
        );
    NTSTATUS CommandNciRead(
        _In_ WDFREQUEST Request
        );
    NTSTATUS CommandHardwareEvent(
        _In_ WDFREQUEST Request
        );
    NTSTATUS CommandSequenceHandlerComplete(
        _In_ WDFREQUEST Request
        );
    NTSTATUS CommandNciWriteComplete(
        _In_ WDFREQUEST Request
        );

    WDFDEVICE _Device = nullptr;

    ApiCallbacksManager _ApiCallbacksManager;

    WDFWAITLOCK _ClientLock = nullptr;
    FileObjectContext* _CurrentSimClient = nullptr;
    PFN_NFC_CX_SEQUENCE_COMPLETION_ROUTINE _SequenceCompleted = nullptr;
    WDFCONTEXT _SequenceCompletedContext = nullptr;
    WDFREQUEST _NciWriteRequest = nullptr;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DeviceContext, DeviceGetContext);
