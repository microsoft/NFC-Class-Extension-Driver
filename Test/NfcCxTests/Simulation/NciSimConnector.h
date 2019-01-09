//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>

#include <NfcCxTestDeviceDriver.h>

#include <IOHelpers\AsyncTask.h>
#include <IOHelpers\IoOperation.h>
#include <IOHelpers\UniqueHandle.h>

#include "NciPacket.h"

struct NciSimCallbackMessage
{
public:
    NciSimCallbackMessage(std::vector<BYTE>&& data) :
        _Data(std::move(data))
    {
    }

    DWORD Length() const
    {
        return DWORD(_Data.size());
    }

    const NciSimCallbackHeader* Header() const
    {
        return reinterpret_cast<const NciSimCallbackHeader*>(_Data.data());
    }

private:
    std::vector<BYTE> _Data;
};

// Connects the NFC CX Test Driver's test driver interface and provides a wrapper API.
class NciSimConnector
{
public:
    NciSimConnector();
    ~NciSimConnector();

    const std::wstring& DeviceId();

    std::shared_ptr<IoOperation> StartHostAsync();
    std::shared_ptr<IoOperation> StopHostAsync();
    void SendNciWriteCompleted();
    void SendNciRead(const NciPacket& packet);
    void SendSequenceCompleted(NTSTATUS status, ULONG flags);
    NciSimCallbackMessage ReceiveLibNfcThreadCallback();
    NciSimCallbackMessage ReceivePowerCallback();
    std::shared_ptr<Async::AsyncTaskBase<void>> WhenLibNfcThreadCallbackAvailableAsync();
    std::shared_ptr<Async::AsyncTaskBase<void>> WhenPowerCallbackAvailableAsync();

private:
    static void ThrowIfWin32BoolFailed(BOOL succeeded);
    static void ThrowIfWin32Failed(DWORD error);

    void SendCommandSync(_In_ DWORD ioctl, _In_reads_bytes_opt_(inputSize) const void* input, _In_ DWORD inputSize);

    void StartGetNextCallback();
    void CallbackRetrieved(Async::AsyncTaskBase<IoOperationResult>& ioOperation);

    UniqueHandle _DriverHandle;
    std::vector<BYTE> _CallbackDataBuffer;
    std::wstring _DeviceId;

    enum class CallbacksState
    {
        // The get-callbacks loop is running.
        Started,
        // The class is shutting down and has requested that the get-callbacks loop stop.
        Stopping,
        // The get-callbacks is not running.
        Stopped,
    };

    std::mutex _CallbacksLock;
    std::condition_variable _CallbacksUpdatedEvent;
    CallbacksState _CallbacksState = CallbacksState::Stopped;
    DWORD _CallbackAllocSize = NciPacketRaw::MaxLength + sizeof(NciSimCallbackHeader);
    std::weak_ptr<IoOperation> _CurrentCallbackIo;
    std::queue<NciSimCallbackMessage> _LibNfcThreadCallbacks;
    std::queue<NciSimCallbackMessage> _PowerCallbacks;
    std::vector<std::shared_ptr<Async::AsyncTaskCompletionSource<void>>> _LibNfcThreadCallbackWaiters;
    std::vector<std::shared_ptr<Async::AsyncTaskCompletionSource<void>>> _PowerCallbackWaiters;
};
