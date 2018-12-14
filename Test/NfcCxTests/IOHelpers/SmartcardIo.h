//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <memory>
#include <vector>

#include "AsyncTask.h"

struct IoApduResult
{
    DWORD ErrorCode = ERROR_SUCCESS;
    std::vector<BYTE> ApduResponse;
};

class SmartcardIo
{
public:
    static std::shared_ptr<::Async::AsyncTaskBase<IoApduResult>> SendApdu(
        _In_ HANDLE smartcardInterface,
        _In_reads_bytes_(apduCommandLength) const void* apduCommand,
        _In_ DWORD apduCommandLength,
        _In_ DWORD apduResponseLength);
};
