//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "IoOperation.h"

std::shared_ptr<IoOperation>
IoOperation::DeviceIoControl(_In_ HANDLE driverHandle, _In_ DWORD ioctl, _In_reads_bytes_opt_(inputSize) const void* input, _In_ DWORD inputSize, DWORD _In_ outputBufferSize, _In_ std::function<Callback> callback)
{
    auto ioOperation = std::make_shared<IoOperation>(PrivateToken{}, driverHandle, input, inputSize, outputBufferSize, std::move(callback));

    void* inputBuffer = inputSize == 0 ?
        nullptr :
        ioOperation->_InputBuffer.data();

    void* outputBuffer = outputBufferSize == 0 ?
        nullptr :
        ioOperation->_OutputBuffer.data();

    // Ensure the object's memory stays around until the I/O operation has completed.
    ioOperation->_SelfRef = ioOperation;

    BOOL ioResult = ::DeviceIoControl(driverHandle, ioctl, inputBuffer, inputSize, outputBuffer, outputBufferSize, nullptr, &ioOperation->_Overlapped);
    if (!ioResult && GetLastError() != ERROR_IO_PENDING)
    {
        ioOperation->_SelfRef = nullptr;
        throw std::system_error(GetLastError(), std::system_category());
    }

    return ioOperation;
}

IoOperation::IoOperation(PrivateToken, _In_ HANDLE driverHandle, _In_reads_bytes_opt_(inputSize) const void* input, _In_ DWORD inputSize, DWORD _In_ outputBufferSize, std::function<Callback>&& callback) :
    _DriverHandle(driverHandle),
    _InputBuffer((const BYTE*)input, (const BYTE*)input + inputSize),
    _OutputBuffer(outputBufferSize),
    _Callback(std::move(callback))
{
    ZeroMemory(&_Overlapped, sizeof(_Overlapped));

    _EventHandle.Reset(CreateEvent(nullptr, /*ManualReset*/ TRUE, /*InitialState*/ FALSE, nullptr));
    if (nullptr == _EventHandle.Get())
    {
        throw std::system_error(GetLastError(), std::system_category());
    }

    _Overlapped.hEvent = _EventHandle.Get();

    _EventWait = CreateThreadpoolWait(IoCompletedCallback, this, nullptr);
    if (nullptr == _EventWait)
    {
        throw std::system_error(GetLastError(), std::system_category());
    }

    SetThreadpoolWait(_EventWait, _EventHandle.Get(), nullptr);
}

IoOperation::~IoOperation()
{
    if (_EventWait != nullptr)
    {
        CloseThreadpoolWait(_EventWait);
    }
}

// Called when the I/O request has completed.
void CALLBACK
IoOperation::IoCompletedCallback(
    _Inout_ PTP_CALLBACK_INSTANCE /*instance*/,
    _Inout_opt_ void* context,
    _Inout_ PTP_WAIT /*wait*/,
    _In_ TP_WAIT_RESULT /*waitResult*/
    )
{
    auto me = reinterpret_cast<IoOperation*>(context);
    me->IoCompleted();
}

void
IoOperation::IoCompleted()
{
    // Drop the self-reference once this function has completed.
    std::shared_ptr<IoOperation> selfRef = std::move(_SelfRef);

    // Get the result of the operation.
    BOOL operationSucceeded = GetOverlappedResult(_DriverHandle, &_Overlapped, &_BytesReturned, /*wait*/ false);

    // Handle results.
    if (operationSucceeded)
    {
        _OperationResult = ERROR_SUCCESS;
    }
    else
    {
        _OperationResult = GetLastError();
    }

    // Let other threads know that the operation has completed.
    WakeByAddressAll(&_OperationResult);

    // Call callback (if provided).
    if (_Callback)
    {
        _Callback(selfRef);
    }
}

bool
IoOperation::Wait(_In_ DWORD timeoutMilliseconds)
{
    for (DWORD operationResult = _OperationResult; _OperationResult == ERROR_IO_PENDING; operationResult = _OperationResult)
    {
        static_assert(sizeof(_OperationResult) == sizeof(DWORD), "std::atomic has an unsupported implementation");
        if (!WaitOnAddress(&_OperationResult, &operationResult, sizeof(DWORD), timeoutMilliseconds))
        {
            // Wait timed out
            return false;
        }
    }

    return true;
}

void
IoOperation::Cancel()
{
    CancelIoEx(_DriverHandle, &_Overlapped);
}

IoOperation::Result
IoOperation::GetResult()
{
    if (_OperationResult == ERROR_IO_PENDING)
    {
        throw std::runtime_error("I/O operation has not completed yet.");
    }

    return { _OperationResult, _BytesReturned, std::move(_OutputBuffer) };
}

// Waits for the I/O request to complete and returns the result.
IoOperation::Result
IoOperation::WaitForResult(_In_ DWORD timeoutMilliseconds)
{
    if (!Wait(timeoutMilliseconds))
    {
        return { ERROR_TIMEOUT, 0, {} };
    }

    return { _OperationResult, _BytesReturned, std::move(_OutputBuffer) };
}
