//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <IOHelpers\IoOperation.h>
#include <IOHelpers\DeviceQuery.h>
#include "NciControlPacket.h"
#include "NciSimConnector.h"

#include "Tests\TestLogging.h"

static constexpr DWORD IO_TIMEOUT_MILLISECONDS = 5'000;

NciSimConnector::NciSimConnector()
{
    // Find the NciSim device interface.
    std::vector<std::wstring> interfaceList = DeviceQuery::FindDriverInterfaces(GUID_DEVINTERFACE_NCI_SIMULATOR, /*timeout(ms)*/ 1'000);
    VERIFY_ARE_EQUAL(1u, interfaceList.size());

    const std::wstring& interfaceId = interfaceList[0];
    _DeviceId = DeviceQuery::GetDeviceIdOfInterface(interfaceId.c_str());

    // Open handle to driver.
    LOG_COMMENT(L"Test Device: %s", interfaceId.c_str());
    _DriverHandle.Reset(CreateFile(
        interfaceId.c_str(),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr));
    ThrowIfWin32BoolFailed(INVALID_HANDLE_VALUE != _DriverHandle.Get());

    // Start callbacks loop.
    std::unique_lock<std::mutex> gate(_CallbacksLock);
    _CallbacksState = CallbacksState::Started;

    try
    {
        StartGetNextCallback();
    }
    catch (...)
    {
        _CallbacksState = CallbacksState::Stopped;
        throw;
    }
}

NciSimConnector::~NciSimConnector()
{
    std::unique_lock<std::mutex> gate(_CallbacksLock);
    if (_CallbacksState == CallbacksState::Started)
    {
        _CallbacksState = CallbacksState::Stopping;
    }

    while (_CallbacksState != CallbacksState::Stopped)
    {
        // Cancel any pending I/O.
        if (_CurrentCallbackIo)
        {
            _CurrentCallbackIo->Cancel();
        }

        // Wait for the callbacks loop to stop.
        _CallbacksUpdatedEvent.wait(gate);
    }
}

const std::wstring&
NciSimConnector::DeviceId()
{
    return _DeviceId;
}

std::shared_ptr<IoOperation>
NciSimConnector::StartHostAsync()
{
    NFC_CX_HOST_ACTION param = HostActionStart;
    return IoOperation::DeviceIoControl(_DriverHandle.Get(), IOCTL_NCISIM_HARDWARE_EVENT, &param, sizeof(param), 0);
}

std::shared_ptr<IoOperation>
NciSimConnector::StopHostAsync()
{
    NFC_CX_HOST_ACTION param = HostActionStop;
    return IoOperation::DeviceIoControl(_DriverHandle.Get(), IOCTL_NCISIM_HARDWARE_EVENT, &param, sizeof(param), 0);
}

void
NciSimConnector::SendNciWriteCompleted()
{
    SendCommandSync(IOCTL_NCISIM_NCI_WRITE_COMPLETE, nullptr, 0);
}

void
NciSimConnector::SendNciRead(const NciPacket& packet)
{
    SendCommandSync(IOCTL_NCISIM_NCI_READ, const_cast<void*>(packet.PacketBytes()), packet.PacketBytesLength());
}

void
NciSimConnector::SendSequenceCompleted(NTSTATUS status, ULONG flags)
{
    NciSimSequenceHandlerComplete params = { status, flags };
    SendCommandSync(IOCTL_NCISIM_SEQUENCE_HANDLER_COMPLETE, &params, sizeof(params));
}

void
NciSimConnector::SendCommandSync(_In_ DWORD ioctl, _In_reads_bytes_opt_(inputSize) const void* input, _In_ DWORD inputSize)
{
    std::shared_ptr<IoOperation> ioOperation = IoOperation::DeviceIoControl(_DriverHandle.Get(), ioctl, input, inputSize, 0);
    if (!ioOperation->Wait(IO_TIMEOUT_MILLISECONDS))
    {
        VERIFY_FAIL(L"NciSimConnector command timed out.");
    }
}

NciSimCallbackMessage
NciSimConnector::ReceiveLibNfcThreadCallback()
{
    std::unique_lock<std::mutex> gate(_CallbacksLock);
    while (_LibNfcThreadCallbacks.empty())
    {
        if (std::cv_status::no_timeout != _CallbacksUpdatedEvent.wait_for(gate, std::chrono::milliseconds(IO_TIMEOUT_MILLISECONDS)))
        {
            VERIFY_FAIL(L"NciSimConnector LibNfcThread callback timed out.");
        }
    }

    NciSimCallbackMessage result = std::move(_LibNfcThreadCallbacks.front());
    _LibNfcThreadCallbacks.pop();
    return std::move(result);
}

NciSimCallbackMessage
NciSimConnector::ReceivePowerCallback()
{
    std::unique_lock<std::mutex> gate(_CallbacksLock);
    while (_PowerCallbacks.empty())
    {
        if (std::cv_status::no_timeout != _CallbacksUpdatedEvent.wait_for(gate, std::chrono::milliseconds(IO_TIMEOUT_MILLISECONDS)))
        {
            VERIFY_FAIL(L"NciSimConnector power callback timed out.");
        }
    }

    NciSimCallbackMessage result = std::move(_PowerCallbacks.front());
    _PowerCallbacks.pop();
    return std::move(result);
}

void
NciSimConnector::StartGetNextCallback()
{
    if (_CallbacksState == CallbacksState::Stopping)
    {
        // Class is being destroyed.
        _CallbacksState = CallbacksState::Stopped;
        _CallbacksUpdatedEvent.notify_all();
        return;
    }

    _CurrentCallbackIo = IoOperation::DeviceIoControl(_DriverHandle.Get(), IOCTL_NCISIM_GET_NEXT_CALLBACK, nullptr, 0, _CallbackAllocSize,
        [this](const std::shared_ptr<IoOperation>& ioOperation)
    {
        this->CallbackRetrieved(ioOperation);
    });
}

void
NciSimConnector::CallbackRetrieved(const std::shared_ptr<IoOperation>& ioOperation)
{
    std::unique_lock<std::mutex> gate(_CallbacksLock);

    _CurrentCallbackIo = nullptr;
    if (_CallbacksState == CallbacksState::Stopping)
    {
        // Class is being destroyed.
        _CallbacksState = CallbacksState::Stopped;
        _CallbacksUpdatedEvent.notify_all();
        return;
    }

    IoOperation::Result ioResult = ioOperation->GetResult();
    if (ioResult.ErrorCode == ERROR_INSUFFICIENT_BUFFER)
    {
        // Increase the size of the output buffer.
        while (_CallbackAllocSize < ioResult.BytesTransferred)
        {
            _CallbackAllocSize *= 2;
        }

        // Try get the message again but with a bigger buffer this time.
        StartGetNextCallback();
    }

    ThrowIfWin32Failed(ioResult.ErrorCode);

    // Apply the size to the output buffer.
    ioResult.Output.resize(ioResult.BytesTransferred);

    NciSimCallbackMessage callbackMessage(std::move(ioResult.Output));
    switch (callbackMessage.Header()->Type)
    {
    case NciSimCallbackType::NciWrite:
    case NciSimCallbackType::SequenceHandler:
        _LibNfcThreadCallbacks.push(std::move(callbackMessage));
        break;

    case NciSimCallbackType::D0Entry:
    case NciSimCallbackType::D0Exit:
        _PowerCallbacks.push(std::move(callbackMessage));
        break;
    }

    _CallbacksUpdatedEvent.notify_all();

    // Get next callback message.
    StartGetNextCallback();
}

void
NciSimConnector::ThrowIfWin32BoolFailed(BOOL succeeded)
{
    if (!succeeded)
    {
        throw std::system_error(GetLastError(), std::system_category());
    }
}

void
NciSimConnector::ThrowIfWin32Failed(DWORD error)
{
    if (error != ERROR_SUCCESS)
    {
        throw std::system_error(error, std::system_category());
    }
}
