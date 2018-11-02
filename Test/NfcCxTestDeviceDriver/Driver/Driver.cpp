//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "Device.h"

#include "Driver.tmh"

extern "C" DRIVER_INITIALIZE DriverEntry;
static EVT_WDF_DRIVER_DEVICE_ADD DriverEvtDeviceAdd;
static EVT_WDF_OBJECT_CONTEXT_CLEANUP DriverEvtCleanup;

// DriverEntry initializes the driver and is the first routine called by the
// system after the driver is loaded.
extern "C" NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    WPP_INIT_TRACING(DriverObject, RegistryPath);
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WDF_DRIVER_CONFIG driverConfig;
    WDF_DRIVER_CONFIG_INIT(&driverConfig, DriverEvtDeviceAdd);

    WDF_OBJECT_ATTRIBUTES driverObjectAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&driverObjectAttributes);
    driverObjectAttributes.EvtCleanupCallback = DriverEvtCleanup;

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &driverObjectAttributes,
        &driverConfig,
        WDF_NO_HANDLE
        );
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfDriverCreate failed. %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

NTSTATUS
DriverEvtDeviceAdd(
    _In_ WDFDRIVER /*Driver*/,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    return DeviceContext::DeviceAdd(DeviceInit);
}

// Free all the resources allocated in DriverEntry.
VOID
DriverEvtCleanup(
    _In_ WDFOBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}
