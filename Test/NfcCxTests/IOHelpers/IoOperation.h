//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "UniqueHandle.h"

// A helper class for making a Win32 I/O request.
class IoOperation
{
    // Make the constructor private but still allow std::make_shared to be used.
    struct PrivateToken {};

public:
    using Callback = void(const std::shared_ptr<IoOperation>& ioOperation);

    struct Result
    {
        DWORD ErrorCode = ERROR_SUCCESS;
        DWORD BytesTransferred = 0;
        std::vector<BYTE> Output;
    };

    static std::shared_ptr<IoOperation> DeviceIoControl(
        _In_ HANDLE driverHandle,
        _In_ DWORD ioctl,
        _In_reads_bytes_opt_(inputSize) const void* input,
        _In_ DWORD inputSize,
        _In_ DWORD outputBufferSize,
        _In_ std::function<Callback> callback = nullptr);

    IoOperation(PrivateToken, _In_ HANDLE driverHandle, _In_reads_bytes_opt_(inputSize) const void* input, _In_ DWORD inputSize, DWORD _In_ outputBufferSize, std::function<Callback>&& callback);
    ~IoOperation();

    IoOperation(const IoOperation&) = delete;
    IoOperation(IoOperation&&) = delete;
    IoOperation& operator=(const IoOperation&) = delete;
    IoOperation& operator=(IoOperation&&) = delete;

    // Waits for the I/O request to complete.
    bool Wait(_In_ DWORD timeoutMilliseconds);
    // Requests that the I/O request is canceled. Note that the I/O request will still be completed.
    void Cancel();
    // Gets the result.
    Result GetResult();
    // Waits for the I/O request to complete and returns the result. If I/O operation fails to complete in
    // the specified time, ERROR_TIMEOUT is returned.
    Result WaitForResult(_In_ DWORD timeoutMilliseconds);

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
    DWORD _BytesReturned = 0;
    std::atomic<DWORD> _OperationResult = ERROR_IO_PENDING;
    std::function<Callback> _Callback;
};
