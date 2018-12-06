//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <devpropdef.h>
#include <vector>
#include <memory>
#include <string>

#include <winrt/windows.devices.enumeration.h>

// Helper class for querying for drivers, devices and device interfaces on the system.
class DeviceQuery
{
public:
    // Find all the driver interfaces with the specified GUID type on a device.
    static std::vector<::winrt::Windows::Devices::Enumeration::DeviceInformation> DevicesQuery(
        std::wstring_view queryString,
        const ::winrt::param::iterable<::winrt::hstring>& additionalProperties,
        ::winrt::Windows::Devices::Enumeration::DeviceInformationKind deviceKind,
        DWORD timeoutMilliseconds);

    // Find all the driver interfaces with the specified GUID type on a device.
    static std::vector<std::wstring> FindDriverInterfaces(
        _In_opt_ PCWSTR deviceId,
        const GUID& interfaceTypeId,
        DWORD timeoutMilliseconds = 0);

    // Find all the driver interfaces on the system with the specified GUID type.
    static std::vector<std::wstring> FindDriverInterfaces(
        const GUID& interfaceTypeId,
        DWORD timeoutMilliseconds = 0);

    // Gets the ID of the device that exposes the specified device interface.
    static std::wstring GetDeviceIdOfInterface(
        _In_ PCWSTR interfaceId);

    // Gets all the smartcard reader interfaces of a particular type.
    static std::vector<std::wstring> GetSmartcardInterfacesOfType(
        _In_opt_ PCWSTR deviceId,
        _In_ BYTE smartcardType,
        DWORD timeoutMilliseconds = 0);

private:
    static std::wstring GuidToString(const GUID& guid);
    static std::wstring DevPropToString(const DEVPROPKEY& propertyId);
    static std::vector<std::wstring> ToIdList(const std::vector<::winrt::Windows::Devices::Enumeration::DeviceInformation>& devicesList);
    static std::wstring CreateInterfaceQueryString(
        _In_opt_ PCWSTR deviceId,
        const GUID& interfaceTypeId);
};
