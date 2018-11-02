//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

class FileObjectContext
{
public:
    enum class Type
    {
        Unknown,
        NciSim,
    };

    static void DeviceInit(
        _Inout_ PWDFDEVICE_INIT DeviceInit
        );

    Type GetType();

private:
    static void CreateCallback(
        _In_ WDFDEVICE Device,
        _In_ WDFREQUEST Request,
        _In_ WDFFILEOBJECT FileObject
        );
    NTSTATUS Create(
        );
    static void CloseCallback(
        _In_ WDFFILEOBJECT FileObject
        );
    void Close(
        );
    static void DestroyCallback(
        _In_ WDFOBJECT Object
        );

    FileObjectContext() = default;
    ~FileObjectContext() = default;

    WDFFILEOBJECT _FileObject = nullptr;
    DeviceContext* _DeviceContext = nullptr;
    Type _Type = Type::Unknown;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FileObjectContext, FileObjectGetContext);
