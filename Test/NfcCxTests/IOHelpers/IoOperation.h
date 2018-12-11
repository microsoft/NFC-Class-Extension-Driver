//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "AsyncTask.h"
#include "UniqueHandle.h"

struct IoOperationResult
{
    DWORD ErrorCode = ERROR_SUCCESS;
    DWORD BytesTransferred = 0;
    std::vector<BYTE> Output;
};

// A helper class for making a Win32 I/O request.
class IoOperation final :
    public Async::AsyncTaskBase<IoOperationResult>
{
    // Make the constructor private but still allow std::make_shared to be used.
    struct PrivateToken {};

public:
    static std::shared_ptr<IoOperation> DeviceIoControl(
        _In_ HANDLE driverHandle,
        _In_ DWORD ioctl,
        _In_reads_bytes_opt_(inputSize) const void* input,
        _In_ DWORD inputSize,
        _In_ DWORD outputBufferSize);

    IoOperation(PrivateToken, _In_ HANDLE driverHandle, _In_reads_bytes_opt_(inputSize) const void* input, _In_ DWORD inputSize, _In_ DWORD outputBufferSize);
    ~IoOperation();

protected:
    void OnCanceled() override;

private:
    static void CALLBACK IoCompletedCallback(
        _Inout_ PTP_CALLBACK_INSTANCE instance,
        _Inout_opt_ void* context,
        _Inout_ PTP_WAIT wait,
        _In_ TP_WAIT_RESULT waitResult
        );
    void IoCompleted();

    HANDLE _DriverHandle = nullptr;
    OVERLAPPED _Overlapped;
    UniqueHandle _EventHandle;
    PTP_WAIT _EventWait;
    std::shared_ptr<IoOperation> _SelfRef;
    std::vector<BYTE> _InputBuffer;
    std::vector<BYTE> _OutputBuffer;
};
