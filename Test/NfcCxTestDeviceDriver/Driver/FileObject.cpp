//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include "Precomp.h"
#include "Device.h"
#include "FileObject.h"

#include "FileObject.tmh"

void
FileObjectContext::DeviceInit(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WDF_FILEOBJECT_CONFIG config;
    WDF_FILEOBJECT_CONFIG_INIT(&config, CreateCallback, CloseCallback, nullptr);

    WDF_OBJECT_ATTRIBUTES objectAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttributes, FileObjectContext);
    objectAttributes.EvtDestroyCallback = DestroyCallback;

    WdfDeviceInitSetFileObjectConfig(DeviceInit, &config, &objectAttributes);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
}

void
FileObjectContext::CreateCallback(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ WDFFILEOBJECT FileObject
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    DeviceContext* deviceContext = DeviceGetContext(Device);
    FileObjectContext* fileContext = FileObjectGetContext(FileObject);

    // Initialize class (using placement new operator).
    new (fileContext) FileObjectContext();
    fileContext->_FileObject = FileObject;
    fileContext->_DeviceContext = deviceContext;

    NTSTATUS status = fileContext->Create();
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Create failed. %!STATUS!", status);
        WdfRequestComplete(Request, status);
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);
    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
}

NTSTATUS
FileObjectContext::Create()
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    UNICODE_STRING* fileObjectName = WdfFileObjectGetFileName(_FileObject);
    if (CSTR_EQUAL == CompareStringOrdinal(fileObjectName->Buffer, fileObjectName->Length / sizeof(WCHAR), L"\\" FILE_NAMESPACE_NCI_SIMULATOR, int(wcslen(L"\\" FILE_NAMESPACE_NCI_SIMULATOR)), /*IgnoreCase*/ TRUE))
    {
        _Type = Type::NciSim;
    }
    else
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        TRACE_LINE(LEVEL_ERROR, "Unknown file namespace: %S. %!STATUS!", fileObjectName->Buffer, status);
        return status;
    }

    status = _DeviceContext->ClientConnected(this);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "DeviceContext::ClientConnected failed. %!STATUS!", status);
        return status;
    }

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

// Called after the HANDLE has been closed and all outstanding I/O requests have completed.
void
FileObjectContext::CloseCallback(
    _In_ WDFFILEOBJECT FileObject
    )
{
    FileObjectContext* fileContext = FileObjectGetContext(FileObject);
    fileContext->Close();
}

void
FileObjectContext::Close()
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    _DeviceContext->ClientDisconnected(this);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
}

// Called just before the WDFFILEOBJECT memory is freed.
void
FileObjectContext::DestroyCallback(
    _In_ WDFOBJECT Object
    )
{
    FileObjectContext* fileContext = FileObjectGetContext(Object);
    fileContext->~FileObjectContext();
}

FileObjectContext::Type
FileObjectContext::GetType()
{
    return _Type;
}
