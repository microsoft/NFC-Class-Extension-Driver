//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <NfcRadioDev.h>

#include "DeviceQuery.h"
#include "IoOperation.h"
#include "RadioManager.h"

RadioManager::RadioManager(_In_ PCWSTR deviceName)
{
    // Find the proximity interface for the device.
    std::vector<std::wstring> radioManagerInterfaces = DeviceQuery::FindDriverInterfaces(deviceName, GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE);

    // Only a single interface is expected to be found.
    VERIFY_ARE_EQUAL(1u, radioManagerInterfaces.size());
    const std::wstring& radioManagerInterface = radioManagerInterfaces[0];

    // Open handle to interface.
    _DeviceInterface.Reset(CreateFile(
        radioManagerInterface.c_str(),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr));
    VERIFY_WIN32_BOOL_SUCCEEDED(INVALID_HANDLE_VALUE != _DeviceInterface.Get(), L"Open radio manager interface");
}

bool
RadioManager::GetNfcRadioState()
{
    std::shared_ptr<IoOperation> ioOperation = IoOperation::DeviceIoControl(_DeviceInterface.Get(), IOCTL_NFCRM_QUERY_RADIO_STATE, nullptr, 0, sizeof(NFCRM_RADIO_STATE));
    IoOperation::Result ioResult = ioOperation->WaitForResult(/*wait (ms)*/ 1'000);

    VERIFY_WIN32_SUCCEEDED(ioResult.ErrorCode, L"Get radio state");

    auto radioState = reinterpret_cast<const NFCRM_RADIO_STATE*>(ioResult.Output.data());
    return !!radioState->MediaRadioOn;
}

void
RadioManager::SetNfcRadioState(bool enableNfcRadio)
{
    SetRadioState(false, enableNfcRadio);
}

void
RadioManager::SetAirplaneMode(bool enableAirplaneMode)
{
    SetRadioState(true, !enableAirplaneMode);
}

// Params:
//   isSystemUpdate: Is airplane mode switch (or nfc radio switch).
//   enableRadio: Enable the radio. (For airplane mode, 'true' means disable airplane mode.)
void
RadioManager::SetRadioState(bool isSystemUpdate, bool enableRadio)
{
    NFCRM_SET_RADIO_STATE radioState = {};
    radioState.SystemStateUpdate = isSystemUpdate;
    radioState.MediaRadioOn = enableRadio;

    std::shared_ptr<IoOperation> ioOperation = IoOperation::DeviceIoControl(_DeviceInterface.Get(), IOCTL_NFCRM_SET_RADIO_STATE, &radioState, sizeof(radioState), 0);
    IoOperation::Result ioResult = ioOperation->WaitForResult(/*wait (ms)*/ 1'000);

    // ERROR_BAD_COMMAND is returned when the state is already correct.
    if (ioResult.ErrorCode != ERROR_BAD_COMMAND)
    {
        VERIFY_WIN32_SUCCEEDED(ioResult.ErrorCode);
    }
}
