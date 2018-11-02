//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "DeviceQuery.h"
#include "ProximityHandleFactory.h"

std::wstring
ProximityHandleFactory::FindProximityInterfaceForDevice(_In_ PCWSTR deviceName)
{
    // Find the proximity interface for the device.
    std::vector<std::wstring> nfpInterfaces = DeviceQuery::FindDriverInterfaces(deviceName, GUID_DEVINTERFACE_NFP);

    // Only a single interface is expected to be found.
    VERIFY_ARE_EQUAL(1u, nfpInterfaces.size());
    return std::move(nfpInterfaces[0]);
}

UniqueHandle
ProximityHandleFactory::OpenSubscriptionHandle(_In_ PCWSTR deviceName, _In_ PCWSTR messageType)
{
    // Find the proximity interface for the device.
    std::wstring subId = FindProximityInterfaceForDevice(deviceName);

    // Create the name for the subscription.
    subId += L"\\Subs\\";
    subId += messageType;

    // Open the device handle.
    UniqueHandle nfpSubInterface(CreateFile(
        subId.c_str(),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr));
    VERIFY_WIN32_BOOL_SUCCEEDED(INVALID_HANDLE_VALUE != nfpSubInterface.Get());

    return std::move(nfpSubInterface);
}
