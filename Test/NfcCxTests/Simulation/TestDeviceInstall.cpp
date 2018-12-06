#include "Precomp.h"

#include <combaseapi.h>
#include <system_error>

#include "TestDeviceInstall.h"

TestDeviceInstall::TestDeviceInstall()
{
    GUID instanceId;
    VerifyHresultSucceeded(CoCreateGuid(&instanceId));

    WCHAR instanceIdString[40];
    (void)StringFromGUID2(instanceId, instanceIdString, ARRAYSIZE(instanceIdString));

    SW_DEVICE_CREATE_INFO deviceCreateInfo = {};
    deviceCreateInfo.cbSize = sizeof(SW_DEVICE_CREATE_INFO);
    deviceCreateInfo.pszInstanceId = instanceIdString;
    deviceCreateInfo.pszzHardwareIds = L"root\\NfcCxTestDevice\0"; // Multi-string (i.e. null-null terminated list of strings)
    deviceCreateInfo.CapabilityFlags = SWDeviceCapabilitiesSilentInstall | SWDeviceCapabilitiesDriverRequired;

    // Install the device.
    VerifyHresultSucceeded(SwDeviceCreate(
        L"NfcCxTestDevice", // enumerator name
        L"HTREE\\ROOT\\0", // parent device (system root)
        &deviceCreateInfo, // info
        0, // properties count
        nullptr, // properties
        DeviceCreatedCallback,
        this,
        &_Device));

    // Wait for the DeviceCreateCallback to execute.
    HRESULT createResult = _CreateResult;
    while (createResult == E_PENDING)
    {
        static_assert(sizeof(createResult) == sizeof(_CreateResult), "std::atomic has an unsupported implementation");
        WaitOnAddress(&_CreateResult, &createResult, sizeof(createResult), INFINITE);

        createResult = _CreateResult;
    }

    VerifyHresultSucceeded(createResult);
}

TestDeviceInstall::~TestDeviceInstall()
{
    if (_Device)
    {
        // Uninstall the device.
        SwDeviceClose(_Device);
        _Device = nullptr;
    }
}

void
TestDeviceInstall::DeviceCreatedCallback(
    _In_ HSWDEVICE hSwDevice,
    _In_ HRESULT CreateResult,
    _In_opt_ void* pContext,
    _In_opt_ PCWSTR pszDeviceInstanceId)
{
    auto me = reinterpret_cast<TestDeviceInstall*>(pContext);
    me->DeviceCreated(hSwDevice, CreateResult, pszDeviceInstanceId);
}

void
TestDeviceInstall::DeviceCreated(
    _In_ HSWDEVICE /*hSwDevice*/,
    _In_ HRESULT CreateResult,
    _In_opt_ PCWSTR pszDeviceInstanceId)
{
    _DeviceInstanceId = pszDeviceInstanceId;
    _CreateResult = CreateResult;
    WakeByAddressAll(&_CreateResult);
}

void
TestDeviceInstall::VerifyHresultSucceeded(HRESULT hr)
{
    if (!SUCCEEDED(hr))
    {
        throw std::system_error(hr, std::system_category(), "HRESULT failed.");
    }
}
