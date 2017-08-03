/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    WdfCxProxy.h

Abstract:

    WDF class extension proxy interface declaration

Environment:

    User-mode Driver Framework

--*/

#pragma once

typedef
VOID
(*PFN_WDF_CLASS_EXPORT)(
    VOID
    );

typedef
_Function_class_(EVT_WDFCX_DEVICE_FILE_CREATE)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
EVT_WDFCX_DEVICE_FILE_CREATE(
    _In_
    WDFDEVICE Device,
    _In_
    WDFREQUEST Request,
    _In_
    WDFFILEOBJECT FileObject
    );

typedef EVT_WDFCX_DEVICE_FILE_CREATE *PFN_WDFCX_DEVICE_FILE_CREATE;

typedef NTSTATUS (*ClassLibraryInitializeCallback)(VOID);

typedef VOID (*ClassLibraryDeinitializeCallback)(VOID);

typedef NTSTATUS (*ClassLibraryBindClientCallback)(
    _In_ PVOID ClassBindInfo,
    _In_ ULONG FunctionTableCount,
    _In_ PFN_WDF_CLASS_EXPORT* FunctionTable);

typedef VOID (*ClassLibraryUnbindClientCallback)(
    _In_ PVOID ClassBindInfo);


WDF_EXTERN_C_START

NTSTATUS
CxProxyWdfRegisterClassLibrary(
    _In_ PUNICODE_STRING RegistryPath,
    _In_opt_ PCUNICODE_STRING ClassLibraryDeviceName,
    _In_ ULONG VersionMajor,
    _In_ ULONG VersionMinor,
    _In_ ULONG VersionBuild,
    _In_ ClassLibraryInitializeCallback InitializeCallback,
    _In_ ClassLibraryDeinitializeCallback DeinitializeCallback,
    _In_ ClassLibraryBindClientCallback BindClientCallback,
    _In_ ClassLibraryUnbindClientCallback UnbindClientCallback
    );

PWDFCXDEVICE_INIT
CxProxyWdfCxDeviceInitAllocate(
    _In_ PWDFDEVICE_INIT DeviceInit
    );

VOID
CxProxyWdfDeviceInitAllowSelfIoTarget(
    _In_ PWDFDEVICE_INIT DeviceInit
    );

VOID
CxProxyWdfCxDeviceInitSetFileObjectConfig(
    _In_ PWDFCXDEVICE_INIT CxDeviceInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES FileObjectAttributes,
    _In_opt_ PFN_WDFCX_DEVICE_FILE_CREATE EvtCxDeviceFileCreate,
    _In_opt_ PFN_WDF_FILE_CLOSE EvtFileClose,
    _In_opt_ PFN_WDF_FILE_CLEANUP EvtFileCleanup
    );

NTSTATUS
CxProxyWdfIoTargetSelfAssignDefaultIoQueue(
    _In_ WDFIOTARGET IoTarget,
    _In_ WDFQUEUE Queue
    );

WDFIOTARGET
CxProxyWdfDeviceGetSelfIoTarget(
    _In_ WDFDEVICE Device
    );

VOID
CxProxyWdfCxVerifierKeBugCheck(
    _In_opt_ WDFOBJECT Object,
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR BugCheckParameter1,
    _In_ ULONG_PTR BugCheckParameter2,
    _In_ ULONG_PTR BugCheckParameter3,
    _In_ ULONG_PTR BugCheckParameter4
    );

WDF_EXTERN_C_END
