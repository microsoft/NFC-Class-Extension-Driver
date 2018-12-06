//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <IOHelpers\DeviceQuery.h>
#include "NciControlPacket.h"
#include "NciSimConnector.h"

static constexpr size_t INITIAL_CALLBACK_DATA_BUFFER_SIZE = NciPacketRaw::MaxLength + sizeof(NciSimCallbackHeader);

NciSimConnector::NciSimConnector()
{
    _CallbackDataBuffer.resize(INITIAL_CALLBACK_DATA_BUFFER_SIZE);

    // Find the NciSim device interface.
    std::vector<std::wstring> interfaceList = DeviceQuery::FindDriverInterfaces(GUID_DEVINTERFACE_NCI_SIMULATOR);
    VERIFY_ARE_NOT_EQUAL(static_cast<size_t>(0), interfaceList.size());

    const std::wstring& interfaceId = interfaceList[0];
    _DeviceId = DeviceQuery::GetDeviceIdOfInterface(interfaceId.c_str());

    // Open handle to driver.
    _DriverHandle.Reset(CreateFile(
        interfaceId.c_str(),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr));
    ThrowIfWin32BoolFailed(INVALID_HANDLE_VALUE != _DriverHandle.Get());
}

const std::wstring&
NciSimConnector::DeviceId()
{
    return _DeviceId;
}

void
NciSimConnector::AddD0PowerReference()
{
    DWORD bytesReturned;
    ThrowIfWin32BoolFailed(DeviceIoControl(_DriverHandle.Get(), IOCTL_NCISIM_ADD_D0_POWER_REFERENCE, nullptr, 0, nullptr, 0, &bytesReturned, nullptr));
}

void
NciSimConnector::RemoveD0PowerReference()
{
    DWORD bytesReturned;
    ThrowIfWin32BoolFailed(DeviceIoControl(_DriverHandle.Get(), IOCTL_NCISIM_REMOVE_D0_POWER_REFERENCE, nullptr, 0, nullptr, 0, &bytesReturned, nullptr));
}

void
NciSimConnector::SendNciWriteCompleted()
{
    DWORD bytesReturned;
    ThrowIfWin32BoolFailed(DeviceIoControl(_DriverHandle.Get(), IOCTL_NCISIM_NCI_WRITE_COMPLETE, nullptr, 0, nullptr, 0, &bytesReturned, nullptr));
}

void
NciSimConnector::SendNciRead(const NciPacket& packet)
{
    DWORD bytesReturned;
    ThrowIfWin32BoolFailed(DeviceIoControl(_DriverHandle.Get(), IOCTL_NCISIM_NCI_READ, const_cast<void*>(packet.PacketBytes()), packet.PacketBytesLength(), nullptr, 0, &bytesReturned, nullptr));
}

void
NciSimConnector::SendSequenceCompleted(NTSTATUS status, ULONG flags)
{
    NciSimSequenceHandlerComplete params = { status, flags };

    DWORD bytesReturned;
    ThrowIfWin32BoolFailed(DeviceIoControl(_DriverHandle.Get(), IOCTL_NCISIM_SEQUENCE_HANDLER_COMPLETE, &params, sizeof(params), nullptr, 0, &bytesReturned, nullptr));
}

NciSimCallbackView
NciSimConnector::ReceiveCallback()
{
    for (;;) // while(true)
    {
        DWORD bytesReturned;
        BOOL ioSucceeded = DeviceIoControl(_DriverHandle.Get(), IOCTL_NCISIM_GET_NEXT_CALLBACK, nullptr, 0, _CallbackDataBuffer.data(), DWORD(_CallbackDataBuffer.size()), &bytesReturned, nullptr);
        if (!ioSucceeded && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            // The buffer is too small. So resize it so that it can hold the message.
            size_t newBufferSize = _CallbackDataBuffer.size();
            while (newBufferSize < bytesReturned)
            {
                newBufferSize *= 2;
            }

            _CallbackDataBuffer.resize(newBufferSize);

            // Try the I/O request again.
            continue;
        }

        ThrowIfWin32BoolFailed(ioSucceeded);
        return { bytesReturned, reinterpret_cast<NciSimCallbackHeader*>(_CallbackDataBuffer.data()) };
    }
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
NciSimConnector::ThrowWin32Failed(DWORD error)
{
    if (error != ERROR_SUCCESS)
    {
        throw std::system_error(error, std::system_category());
    }
}
