//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "IoOperation.h"
#include "SmartcardIo.h"

std::shared_ptr<::Async::AsyncTaskBase<IoApduResult>>
SmartcardIo::SendApdu(
    _In_ HANDLE smartcardInterface,
    _In_reads_bytes_(apduCommandLength) const void* apduCommand,
    _In_ DWORD apduCommandLength,
    _In_ DWORD apduResponseLength)
{
    DWORD inputLength = sizeof(SCARD_IO_REQUEST) + apduCommandLength;
    DWORD outputLength = sizeof(SCARD_IO_REQUEST) + apduResponseLength;

    // Add the SCARD_IO_REQUEST header to the command APDU.
    auto input = std::make_unique<BYTE[]>(inputLength);

    auto commandApduHeader = reinterpret_cast<SCARD_IO_REQUEST*>(input.get());
    commandApduHeader->dwProtocol = SCARD_PROTOCOL_T1;
    commandApduHeader->cbPciLength = sizeof(SCARD_IO_REQUEST);

    memcpy(input.get() + sizeof(SCARD_IO_REQUEST), apduCommand, apduCommandLength);

    // Start the I/O request.
    std::shared_ptr<IoOperation> ioTransmit = IoOperation::DeviceIoControl(smartcardInterface, IOCTL_SMARTCARD_TRANSMIT, input.get(), inputLength, outputLength);

    // Create a wrapper async task, to allow for a little bit of post processing.
    auto completionSource = Async::MakeCompletionSource<IoApduResult>();
    ioTransmit->SetCompletedHandler(
        [completionSource](Async::AsyncTaskBase<IoOperationResult>& asyncTask)
    {
        IoOperationResult ioResult = asyncTask.Get();

        IoApduResult result = {};
        result.ErrorCode = ioResult.ErrorCode;
        if (result.ErrorCode == ERROR_SUCCESS)
        {
            result.ApduResponse = std::move(ioResult.Output);

            // Strip out the SCARD_IO_REQUEST header from the response APDU.
            DWORD responseSize = ioResult.BytesTransferred - sizeof(SCARD_IO_REQUEST);
            memmove(result.ApduResponse.data(), result.ApduResponse.data() + sizeof(SCARD_IO_REQUEST), responseSize);
            result.ApduResponse.resize(responseSize);
        }

        // Complete async task.
        completionSource->EmplaceResult(std::move(result));
    });

    // Return async task.
    return completionSource;
}
