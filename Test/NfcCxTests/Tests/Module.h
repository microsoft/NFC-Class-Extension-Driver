//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

static PCWSTR ProximityServicesNames[] = 
{
    L"ProximitySvc",
    L"SCardSvr",
    L"ScDeviceEnum",
    L"SEMgrSvc",
    L"ShellHWDetection",
};

static constexpr size_t ProximityServicesCount = ARRAYSIZE(ProximityServicesNames);

struct ServiceState
{
    SC_HANDLE ServiceHandle;
    DWORD StartType;
};

class Module
{
public:
    bool Setup();
    bool Cleanup();

private:
    bool StopAndDisableServices();
    void ResumeServices();
    bool GetServiceConfig(_In_ SC_HANDLE serviceHandle, _Inout_ std::unique_ptr<QUERY_SERVICE_CONFIG>* config);
    bool ChangeServiceStartType(
        _In_ SC_HANDLE serviceHandle,
        _In_ DWORD startType);
    bool StopService(_In_ SC_HANDLE serviceHandle);

    SC_HANDLE _SCManager;
    ServiceState _ServiceStateCache[ProximityServicesCount];
};
