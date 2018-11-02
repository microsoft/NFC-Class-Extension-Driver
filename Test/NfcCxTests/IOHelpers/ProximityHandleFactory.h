//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include "UniqueHandle.h"

class ProximityHandleFactory
{
public:
    static std::wstring FindProximityInterfaceForDevice(_In_ PCWSTR deviceName);
    static UniqueHandle OpenSubscriptionHandle(_In_ PCWSTR deviceName, _In_ PCWSTR messageType);
};
