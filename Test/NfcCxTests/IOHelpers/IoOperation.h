//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "UniqueHandle.h"

// A helper class for making a Win32 I/O request.
class IoOperation
{
    // Make the constructor private but still allow std::make_shared to be used.
    struct PrivateToken {};

public:
    static std::shared_ptr<IoOperation> DeviceIoControl(_In_ HANDLE driverHandle, _In_ DWORD ioctl, _In_reads_bytes_opt_(inputSize) const void* input, _In_ DWORD inputSize, DWORD _In_ outputBufferSize);

    IoOperation(PrivateToken, _In_ HANDLE driverHandle, _In_reads_bytes_opt_(inputSize) const void* input, _In_ DWORD inputSize, DWORD _In_ outputBufferSize);
    ~IoOperation();

    IoOperation(const IoOperation&) = delete;
    IoOperation(IoOperation&&) = delete;
    IoOperation& operator=(const IoOperation&) = delete;
    IoOperation& operator=(IoOperation&&) = delete;

    // Waits for the I/O request to complete.
    bool Wait(_In_ DWORD timeoutMilliseconds);
    // Requests that the I/O request is canceled. Note that the I/O request will still be completed.
    void Cancel();
    // Gets the Win32 result code.
    DWORD Result();
    // Waits for the I/O request to complete and returns the result. If I/O operation fails to complete in
    // the specified time, ERROR_TIMEOUT is returned.
    DWORD WaitForResult(_In_ DWORD timeoutMilliseconds);
    // Gets the I/O request's output.
    const std::vector<BYTE>& OutputBuffer();

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
    std::atomic<DWORD> _OperationResult = ERROR_IO_PENDING;
};
