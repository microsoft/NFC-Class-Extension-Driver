//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <vector>

#include <NfcCxTestDeviceDriver.h>

#include <IOHelpers\UniqueHandle.h>

class NciControlPacket;

struct NciSimCallbackView
{
    DWORD Length = 0;
    const NciSimCallbackHeader* Header = nullptr;
};

// Connects the NFC CX Test Driver's test driver interface and provides a wrapper API.
class NciSimConnector
{
public:
    NciSimConnector();
    ~NciSimConnector() = default;

    const std::wstring& DeviceId();

    void AddD0PowerReference();
    void RemoveD0PowerReference();
    void SendNciWriteCompleted();
    void SendNciRead(const NciPacket& packet);
    void SendSequenceCompleted(NTSTATUS status, ULONG flags);
    NciSimCallbackView ReceiveCallback();

private:
    static void ThrowIfWin32BoolFailed(BOOL succeeded);
    static void ThrowWin32Failed(DWORD error);

    UniqueHandle _DriverHandle;
    std::vector<BYTE> _CallbackDataBuffer;
    std::wstring _DeviceId;
};
