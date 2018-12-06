//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "TestLogging.h"
#include "Module.h"

BEGIN_MODULE()
    MODULE_PROPERTY(L"RunFixtureAs:Module", L"System")
END_MODULE()

MODULE_SETUP(ModuleSetup);
MODULE_CLEANUP(ModuleCleanup);

static Module _Module;

bool
ModuleSetup()
{
    return _Module.Setup();
}

bool
ModuleCleanup()
{
    return _Module.Cleanup();
}

bool
Module::Setup()
{
    // Stop system services.
    if (!StopAndDisableServices())
    {
        ResumeServices();
        return false;
    }

    // Install the test device.
    _TestDeviceInstall.emplace();

    return true;
}

bool
Module::Cleanup()
{
    // Uninstall test device.
    _TestDeviceInstall = std::nullopt;

    // Resume system services.
    ResumeServices();
    return true;
}

bool
Module::StopAndDisableServices()
{
    _SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!_SCManager)
    {
        return false;
    }

    for (size_t i = 0; i != ProximityServicesCount; ++i)
    {
        PCWSTR serviceName = ProximityServicesNames[i];
        LOG_COMMENT(L"Stopping and disabling service '%s'.", serviceName);

        SC_HANDLE serviceHandle = OpenService(_SCManager, serviceName, SC_MANAGER_ALL_ACCESS);
        if (!serviceHandle)
        {
            if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
            {
                // Service doesn't exist on this platform.
                // Just skip it.
                LOG_COMMENT(L"Service doesn't exist. Skipping.");
                continue;
            }

            return false;
        }

        std::unique_ptr<QUERY_SERVICE_CONFIG> serviceConfig;
        if (!GetServiceConfig(serviceHandle, &serviceConfig))
        {
            CloseServiceHandle(serviceHandle);
            return false;
        }

        if (!ChangeServiceStartType(serviceHandle, SERVICE_DISABLED))
        {
            LOG_COMMENT(L"Failed to disable service.");
            CloseServiceHandle(serviceHandle);
            return false;
        }

        LOG_COMMENT(L"Disabled service.");

        if (!StopService(serviceHandle))
        {
            CloseServiceHandle(serviceHandle);
            return false;
        }

        _ServiceStateCache[i].ServiceHandle = serviceHandle;
        _ServiceStateCache[i].StartType = serviceConfig->dwStartType;
    }

    return true;
}

void
Module::ResumeServices()
{
    if (_SCManager)
    {
        for (size_t i = 0; i != ProximityServicesCount; ++i)
        {
            ServiceState& serviceState = _ServiceStateCache[i];
            PCWSTR serviceName = ProximityServicesNames[i];

            if (!serviceState.ServiceHandle)
            {
                // Nothing to do.
                continue;
            }

            // Restore service start type.
            LOG_COMMENT(L"Restore '%s' service start type.", serviceName);
            if (!ChangeServiceStartType(serviceState.ServiceHandle, serviceState.StartType))
            {
                LOG_COMMENT(L"Failed to re-enable.");
            }

            // Cleanup.
            CloseServiceHandle(serviceState.ServiceHandle);
            serviceState.ServiceHandle = nullptr;
        }

        // Cleanup.
        CloseServiceHandle(_SCManager);
        _SCManager = nullptr;
    }
}

bool
Module::GetServiceConfig(
    _In_ SC_HANDLE serviceHandle,
    _Inout_ std::unique_ptr<QUERY_SERVICE_CONFIG>* config)
{
    // Grab the size of the config data.
    BOOL queryResult;
    DWORD serviceConfigSize;
    queryResult = QueryServiceConfig(serviceHandle, NULL, 0, &serviceConfigSize);
    if (queryResult ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
        serviceConfigSize < sizeof(QUERY_SERVICE_CONFIG))
    {
        LOG_COMMENT(L"Failed to get service config size.");
        return false;
    }

    // Allocate space for the config data.
    auto serviceConfigBuffer = reinterpret_cast<QUERY_SERVICE_CONFIG*>(operator new (serviceConfigSize));
    auto serviceConfig = std::unique_ptr<QUERY_SERVICE_CONFIG>(serviceConfigBuffer);
    if (!serviceConfig)
    {
        LOG_COMMENT(L"Failed to allocate space for service config.");
        return false;
    }

    // Grab config data.
    queryResult = QueryServiceConfig(serviceHandle, serviceConfig.get(), serviceConfigSize, &serviceConfigSize);
    if (!queryResult)
    {
        LOG_COMMENT(L"Failed to get service config.");
        return false;
    }

    // Return result.
    *config = std::move(serviceConfig);
    LOG_COMMENT(L"Got service config.");
    return true;
}

bool
Module::ChangeServiceStartType(
    _In_ SC_HANDLE serviceHandle,
    _In_ DWORD startType)
{
    return !!ChangeServiceConfig(
        serviceHandle,
        SERVICE_NO_CHANGE,
        startType,
        SERVICE_NO_CHANGE,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
}

bool
Module::StopService(_In_ SC_HANDLE serviceHandle)
{
    // Signal service to stop.
    SERVICE_STATUS serviceStatus;
    BOOL controlServiceResult = ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus);
    if (!controlServiceResult)
    {
        DWORD lastError = GetLastError();
        if (lastError != ERROR_SERVICE_NOT_ACTIVE &&
            lastError != ERROR_SERVICE_CANNOT_ACCEPT_CTRL &&
            serviceStatus.dwCurrentState != SERVICE_START_PENDING)
        {
            LOG_COMMENT(L"Failed to send service stop command.");
            return false;
        }
    }

    auto serviceStoppedCallback = [] (void* /*context*/)
    {
    };

    // Register for service stopped event.
    SERVICE_NOTIFY serviceNotifyData = {};
    serviceNotifyData.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    serviceNotifyData.pfnNotifyCallback = serviceStoppedCallback;

    DWORD registerServiceEventResult = NotifyServiceStatusChange(serviceHandle, SERVICE_NOTIFY_STOPPED, &serviceNotifyData);
    if (registerServiceEventResult != ERROR_SUCCESS)
    {
        LOG_COMMENT(L"Failed to subscribe to service stopped event.");
        return false;
    }

    // Wait for event.
    DWORD sleepResult = SleepEx(/*time(ms)*/ 30000, /*APC*/ TRUE);
    if (sleepResult == 0)
    {
        LOG_COMMENT(L"Service took too long to stop.");
        return false;
    }

    // Cleanup.
    LOG_COMMENT(L"Service stopped.");
    return true;
}
