//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <swdevice.h>

#include <atomic>
#include <string>

class TestDeviceInstall
{
public:
    TestDeviceInstall();
    ~TestDeviceInstall();

private:
    static void WINAPI DeviceCreatedCallback(
        _In_ HSWDEVICE hSwDevice,
        _In_ HRESULT  CreateResult,
        _In_opt_ void* pContext,
        _In_opt_ PCWSTR pszDeviceInstanceId);
    void DeviceCreated(
        _In_ HSWDEVICE hSwDevice,
        _In_ HRESULT CreateResult,
        _In_opt_ PCWSTR pszDeviceInstanceId);
    static void VerifyHresultSucceeded(HRESULT hr);

    HSWDEVICE _Device = nullptr;
    std::wstring _DeviceInstanceId;
    std::atomic<HRESULT> _CreateResult = E_PENDING;
};
