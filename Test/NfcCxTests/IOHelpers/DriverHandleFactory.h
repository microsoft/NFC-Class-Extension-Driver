//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include "UniqueHandle.h"

class DriverHandleFactory
{
public:
    static std::wstring FindProximityInterfaceForDevice(_In_ PCWSTR deviceName);
    static std::wstring FindSmartcardInterfaceForDevice(
        _In_ PCWSTR deviceName,
        _In_ ::ABI::Windows::Devices::SmartCards::SmartCardReaderKind readerKind);
    static UniqueHandle OpenSubscriptionHandle(_In_ PCWSTR deviceName, _In_ PCWSTR messageType);
    static UniqueHandle OpenSmartcardHandle(
        _In_ PCWSTR deviceName,
        _In_ ::ABI::Windows::Devices::SmartCards::SmartCardReaderKind readerKind);
};
