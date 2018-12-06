//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <combaseapi.h>

#include <condition_variable>
#include <mutex>

#include "DeviceQuery.h"

// {D6B5B883-18BD-4B4D-B2EC-9E38AFFEDA82}, 2, DEVPROP_TYPE_BYTE
DEFINE_DEVPROPKEY(DEVPKEY_Device_ReaderKind, 0xD6B5B883, 0x18BD, 0x4B4D, 0xB2, 0xEC, 0x9E, 0x38, 0xAF, 0xFE, 0xDA, 0x82, 0x02);

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::Enumeration;

std::vector<::winrt::Windows::Devices::Enumeration::DeviceInformation>
DeviceQuery::DevicesQuery(
    std::wstring_view queryString,
    const ::winrt::param::iterable<::winrt::hstring>& additionalProperties,
    ::winrt::Windows::Devices::Enumeration::DeviceInformationKind deviceKind,
    DWORD timeoutMilliseconds)
{
    // Setup the DeviceWatcher
    std::mutex queryLock;
    std::condition_variable queryUpdated;
    std::vector<::winrt::Windows::Devices::Enumeration::DeviceInformation> result;

    enum class QueryState { Started, EnumerationCompleted, Stopped };
    QueryState queryState = QueryState::Started;

    DeviceWatcher deviceWatcher = DeviceInformation::CreateWatcher(queryString, additionalProperties, deviceKind);
    deviceWatcher.Added([&](DeviceWatcher deviceWatcher, DeviceInformation deviceInformation)
    {
        std::unique_lock<std::mutex> gate(queryLock);
        result.push_back(deviceInformation);
        if (queryState != QueryState::Started)
        {
            queryUpdated.notify_all();
        }
    });
    deviceWatcher.Removed([](DeviceWatcher deviceWatcher, DeviceInformationUpdate deviceInformation)
    {
    });
    deviceWatcher.Updated([](DeviceWatcher deviceWatcher, DeviceInformationUpdate deviceInformation)
    {
    });
    deviceWatcher.EnumerationCompleted([&](DeviceWatcher deviceWatcher, IInspectable args)
    {
        std::unique_lock<std::mutex> gate(queryLock);
        queryState = QueryState::EnumerationCompleted;
        queryUpdated.notify_all();
    });
    deviceWatcher.Stopped([&](DeviceWatcher deviceWatcher, IInspectable args)
    {
        std::unique_lock<std::mutex> gate(queryLock);
        queryState = QueryState::Stopped;
        queryUpdated.notify_all();
    });

    // Start the DeviceWatcher
    deviceWatcher.Start();

    // Wait for the initial enumeration to complete.
    std::unique_lock<std::mutex> gate(queryLock);
    while (queryState == QueryState::Started)
    {
        queryUpdated.wait(gate);
    }

    // Wait for at least one interface to appear.
    while (result.empty())
    {
        if (std::cv_status::timeout == queryUpdated.wait_for(gate, std::chrono::milliseconds(timeoutMilliseconds)))
        {
            break;
        }
    }

    // Stop the DeviceWatcher.
    deviceWatcher.Stop();

    // Wait for the DeviceWatcher to stop.
    while (queryState != QueryState::Stopped)
    {
        queryUpdated.wait(gate);
    }

    // Return results.
    return std::move(result);
}

// Find all the driver interfaces with the specified GUID type on a device.
std::vector<std::wstring>
DeviceQuery::FindDriverInterfaces(
    _In_opt_ PCWSTR deviceId,
    const GUID& interfaceTypeId,
    DWORD timeoutMilliseconds)
{
    std::wstring queryString = CreateInterfaceQueryString(deviceId, interfaceTypeId);

    auto devices = DevicesQuery(queryString, nullptr, DeviceInformationKind::DeviceInterface, timeoutMilliseconds);
    return ToIdList(devices);
}

std::vector<std::wstring>
DeviceQuery::FindDriverInterfaces(
    const GUID& interfaceTypeId,
    DWORD timeoutMilliseconds)
{
    return FindDriverInterfaces(nullptr, interfaceTypeId, timeoutMilliseconds);
}

std::wstring
DeviceQuery::GetDeviceIdOfInterface(
    _In_ PCWSTR interfaceId)
{
    winrt::hstring deviceIdPropertyName(L"System.Devices.DeviceInstanceId");
    DeviceInformation deviceInformation = DeviceInformation::CreateFromIdAsync(interfaceId, { deviceIdPropertyName }).get();
    return winrt::unbox_value<winrt::hstring>(deviceInformation.Properties().Lookup(deviceIdPropertyName)).c_str();
}

std::vector<std::wstring>
DeviceQuery::GetSmartcardInterfacesOfType(
    _In_opt_ PCWSTR deviceId,
    _In_ BYTE smartcardType,
    DWORD timeoutMilliseconds)
{
    // Create the device query string.
    std::wstring queryString = CreateInterfaceQueryString(deviceId, GUID_DEVINTERFACE_SMARTCARD_READER);

    winrt::hstring readerKindPropertyName(DevPropToString(DEVPKEY_Device_ReaderKind));
    auto devicesList = DevicesQuery(queryString, { readerKindPropertyName }, DeviceInformationKind::DeviceInterface, timeoutMilliseconds);

    std::vector<std::wstring> result;
    for (const DeviceInformation& deviceInformation : devicesList)
    {
        if (smartcardType == winrt::unbox_value<BYTE>(deviceInformation.Properties().Lookup(readerKindPropertyName)))
        {
            result.push_back(deviceInformation.Id().c_str());
        }
    }

    return std::move(result);
}

std::wstring
DeviceQuery::GuidToString(const GUID& guid)
{
    WCHAR result[40];
    StringFromGUID2(guid, result, ARRAYSIZE(result));
    return result;
}

// Converts a DEVPROPKEY into the string format recongized by the Windows.Devices.Enumeration API.
std::wstring
DeviceQuery::DevPropToString(const DEVPROPKEY& propertyId)
{
    std::wstring result = GuidToString(propertyId.fmtid);
    result += L" ";
    result += std::to_wstring(propertyId.pid);
    return std::move(result);
}

std::vector<std::wstring>
DeviceQuery::ToIdList(const std::vector<::winrt::Windows::Devices::Enumeration::DeviceInformation>& devicesList)
{
    std::vector<std::wstring> result;
    result.reserve(devicesList.size());

    for (const DeviceInformation& deviceInformation : devicesList)
    {
        result.push_back(deviceInformation.Id().c_str());
    }

    return std::move(result);
}

// Creates a Windows.Devices.Enumeration query string that searches for device interfaces of the specified GUID type.
// The search can optionally be restricted to interfaces created by a particular device.
std::wstring
DeviceQuery::CreateInterfaceQueryString(
    _In_opt_ PCWSTR deviceId,
    const GUID& interfaceTypeId)
{
    // Create the device query string.
    std::wstring queryString;

    if (deviceId)
    {
        queryString += L"System.Devices.DeviceInstanceId:=\"";
        queryString += deviceId;
        queryString += L"\" AND ";
    }

    queryString += L"System.Devices.InterfaceClassGuid:=\"";
    queryString += GuidToString(interfaceTypeId);
    queryString += L"\" AND System.Devices.InterfaceEnabled:=System.StructuredQueryType.Boolean#True";

    return std::move(queryString);
}
