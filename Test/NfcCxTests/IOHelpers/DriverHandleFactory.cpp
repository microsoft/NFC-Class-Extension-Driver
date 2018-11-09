//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "DeviceQuery.h"
#include "DriverHandleFactory.h"

std::wstring
DriverHandleFactory::FindProximityInterfaceForDevice(_In_ PCWSTR deviceName)
{
    // Find the proximity interface for the device.
    std::vector<std::wstring> nfpInterfaces = DeviceQuery::FindDriverInterfaces(deviceName, GUID_DEVINTERFACE_NFP);

    // Only a single interface is expected to be found.
    VERIFY_ARE_EQUAL(1u, nfpInterfaces.size());
    return std::move(nfpInterfaces[0]);
}

std::wstring
DriverHandleFactory::FindSmartcardInterfaceForDevice(
    _In_ PCWSTR deviceName,
    _In_ ::ABI::Windows::Devices::SmartCards::SmartCardReaderKind readerKind)
{
    std::vector<std::wstring> nfcScInterfaces = DeviceQuery::GetSmartcardInterfacesOfType(deviceName, BYTE(readerKind));
    VERIFY_ARE_EQUAL(1u, nfcScInterfaces.size());

    return std::move(nfcScInterfaces[0]);
}

UniqueHandle
DriverHandleFactory::OpenSubscriptionHandle(_In_ PCWSTR deviceName, _In_ PCWSTR messageType)
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

UniqueHandle
DriverHandleFactory::OpenSmartcardHandle(
    _In_ PCWSTR deviceName,
    _In_ ::ABI::Windows::Devices::SmartCards::SmartCardReaderKind readerKind)
{
    std::wstring smartcardInterfaceName = FindSmartcardInterfaceForDevice(deviceName, readerKind);

    UniqueHandle scInterface(CreateFile(
        smartcardInterfaceName.c_str(),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr));
    VERIFY_WIN32_BOOL_SUCCEEDED(INVALID_HANDLE_VALUE != scInterface.Get());

    return std::move(scInterface);
}
