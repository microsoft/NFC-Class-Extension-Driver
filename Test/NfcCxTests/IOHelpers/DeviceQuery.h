//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <devpropdef.h>
#include <vector>
#include <memory>
#include <string>

typedef DWORD CONFIGRET;

// Helper class for querying for drivers, devices and device interfaces on the system.
class DeviceQuery
{
public:
    // Find all the driver interfaces with the specified GUID type on a device.
    static std::vector<std::wstring> FindDriverInterfaces(
        _In_opt_ PCWSTR deviceId,
        const GUID& interfaceTypeId);

    // Find all the driver interfaces on the system with the specified GUID type.
    static std::vector<std::wstring> FindDriverInterfaces(
        const GUID& interfaceTypeId);

    // Retrieves a property of a device interface.
    static std::vector<BYTE> GetDeviceInterfaceProperty(
        _In_ PCWSTR interfaceId,
        const DEVPROPKEY& propertyId,
        _Out_opt_ DEVPROPTYPE* resultType);

    // Retrieves a property of a device interface as a BYTE.
    static BYTE GetDeviceInterfaceByteProperty(
        _In_ PCWSTR interfaceId,
        const DEVPROPKEY& propertyId);

    // Retrieves a property of a device interface as a string.
    static std::wstring GetDeviceInterfaceStringProperty(
        _In_ PCWSTR interfaceId,
        const DEVPROPKEY& propertyId);

    // Gets the ID of the device that exposes the specified device interface.
    static std::wstring GetDeviceIdOfInterface(
        _In_ PCWSTR interfaceId);

    // Gets all the smartcard reader interfaces of a particular type.
    static std::vector<std::wstring> GetSmartcardInterfacesOfType(
        _In_opt_ PCWSTR deviceId,
        _In_ BYTE smartcardType);

private:
    static void VerifyCmSucceeded(CONFIGRET result);

    static std::vector<std::wstring> ConvertFlatStringList(
        _In_ _NullNull_terminated_ WCHAR* list);
};
