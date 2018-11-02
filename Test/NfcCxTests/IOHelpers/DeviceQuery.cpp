//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <cfgmgr32.h>

#include "DeviceQuery.h"

// {D6B5B883-18BD-4B4D-B2EC-9E38AFFEDA82}, 2, DEVPROP_TYPE_BYTE
DEFINE_DEVPROPKEY(DEVPKEY_Device_ReaderKind, 0xD6B5B883, 0x18BD, 0x4B4D, 0xB2, 0xEC, 0x9E, 0x38, 0xAF, 0xFE, 0xDA, 0x82, 0x02);

std::vector<std::wstring>
DeviceQuery::FindDriverInterfaces(
    _In_opt_ PCWSTR deviceId,
    const GUID& interfaceTypeId)
{
    // Get the interface list's size.
    ULONG rawInterfaceListSize;
    VerifyCmSucceeded(CM_Get_Device_Interface_List_Size(
        &rawInterfaceListSize,
        const_cast<GUID*>(&interfaceTypeId),
        const_cast<WCHAR*>(deviceId),
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT));

    // Get the interface list.
    std::unique_ptr<WCHAR[]> rawInterfaceList(new WCHAR[rawInterfaceListSize]);
    VerifyCmSucceeded(CM_Get_Device_Interface_List(
        const_cast<GUID*>(&interfaceTypeId),
        const_cast<WCHAR*>(deviceId),
        rawInterfaceList.get(),
        rawInterfaceListSize,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT));

    // Convert the interface list into a more useful format.
    return ConvertFlatStringList(rawInterfaceList.get());
}

std::vector<std::wstring>
DeviceQuery::FindDriverInterfaces(
    const GUID& interfaceTypeId)
{
    return FindDriverInterfaces(nullptr, interfaceTypeId);
}

std::vector<BYTE>
DeviceQuery::GetDeviceInterfaceProperty(
    _In_ PCWSTR interfaceId,
    const DEVPROPKEY& propertyId,
    _Out_ DEVPROPTYPE* resultType)
{
    // The the size of the property's value.
    DEVPROPTYPE propertyType;
    ULONG propertySizeBytes = 0;
    VERIFY_ARE_EQUAL(CONFIGRET(CR_BUFFER_SMALL), CM_Get_Device_Interface_Property(
        interfaceId,
        &propertyId,
        &propertyType,
        nullptr,
        &propertySizeBytes,
        0));

    // Get the proerty's value.
    std::vector<BYTE> result(propertySizeBytes);
    VerifyCmSucceeded(CM_Get_Device_Interface_Property(
        interfaceId,
        &propertyId,
        &propertyType,
        result.data(),
        &propertySizeBytes,
        0));

    result.resize(propertySizeBytes);

    *resultType = propertyType;
    return std::move(result);
}

BYTE
DeviceQuery::GetDeviceInterfaceByteProperty(
    _In_ PCWSTR interfaceId,
    const DEVPROPKEY& propertyId)
{
    // Get the property's value.
    BYTE property;
    ULONG propertySizeBytes = sizeof(property);
    DEVPROPTYPE propertyType;
    VerifyCmSucceeded(CM_Get_Device_Interface_Property(
        interfaceId,
        &propertyId,
        &propertyType,
        &property,
        &propertySizeBytes,
        0));

    if (DEVPROP_TYPE_BYTE != propertyType)
    {
        throw std::runtime_error("Device property does not have type of BYTE.");
    }

    return property;
}

std::wstring
DeviceQuery::GetDeviceInterfaceStringProperty(
    _In_ PCWSTR interfaceId,
    const DEVPROPKEY& propertyId)
{
    // Get the property's value as raw bytes.
    DEVPROPTYPE propertyType;
    std::vector<BYTE> propertyData = GetDeviceInterfaceProperty(interfaceId, propertyId, &propertyType);

    if (DEVPROP_TYPE_STRING != propertyType)
    {
        throw std::runtime_error("Device property does not have type of STRING.");
    }

    return reinterpret_cast<WCHAR*>(propertyData.data());
}

std::wstring
DeviceQuery::GetDeviceIdOfInterface(
    _In_ PCWSTR interfaceId)
{
    return GetDeviceInterfaceStringProperty(interfaceId, DEVPKEY_Device_InstanceId);
}

std::vector<std::wstring>
DeviceQuery::GetSmartcardInterfacesOfType(
    _In_opt_ PCWSTR deviceId,
    _In_ BYTE smartcardType)
{
    std::vector<std::wstring> interfaceList = FindDriverInterfaces(deviceId, GUID_DEVINTERFACE_SMARTCARD_READER);

    std::vector<std::wstring> filteredInterfaceList;
    for (std::wstring& interfaceId : interfaceList)
    {
        if (smartcardType == GetDeviceInterfaceByteProperty(interfaceId.c_str(), DEVPKEY_Device_ReaderKind))
        {
            filteredInterfaceList.push_back(std::move(interfaceId));
        }
    }

    return std::move(filteredInterfaceList);
}

void
DeviceQuery::VerifyCmSucceeded(CONFIGRET result)
{
    if (result != CR_SUCCESS)
    {
        throw std::system_error(result, std:: generic_category(), "CM API error");
    }
}

std::vector<std::wstring>
DeviceQuery::ConvertFlatStringList(
    _In_ _NullNull_terminated_ WCHAR* list)
{
    // The end of list is marked by a double null terminator.
    std::vector<std::wstring> result;
    for (WCHAR* iterator = list; iterator[0] != L'\0'; )
    {
        size_t stringLength = wcslen(iterator);
        result.push_back(std::wstring(iterator, stringLength));
        iterator += stringLength + 1;
    }

    return std::move(result);
}
