//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <string>
#include <memory>

#include <winrt/windows.devices.smartcards.h>

#include "AsyncTask.h"
#include "UniqueHandle.h"

class DriverHandleFactory
{
public:
    static std::wstring FindProximityInterfaceForDevice(_In_ PCWSTR deviceName);
    static std::wstring FindSmartcardInterfaceForDevice(
        _In_ PCWSTR deviceName,
        _In_ ::winrt::Windows::Devices::SmartCards::SmartCardReaderKind readerKind);
    static UniqueHandle OpenSubscriptionHandle(_In_ PCWSTR deviceName, _In_ PCWSTR messageType);
    static UniqueHandle OpenSmartcardHandle(
        _In_ PCWSTR deviceName,
        _In_ ::winrt::Windows::Devices::SmartCards::SmartCardReaderKind readerKind);
    static std::shared_ptr<::Async::AsyncTaskBase<UniqueHandle>> OpenSmartcardHandleAsync(
        _In_ PCWSTR deviceName,
        _In_ ::winrt::Windows::Devices::SmartCards::SmartCardReaderKind readerKind);
    static std::shared_ptr<::Async::AsyncTaskBase<void>> CloseHandleAsync(UniqueHandle&& obj);
};
