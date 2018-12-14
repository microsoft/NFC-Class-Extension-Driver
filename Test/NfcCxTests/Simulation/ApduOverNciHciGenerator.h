//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "NciHciDataPacket.h"
#include "SimSequenceStep.h"

class ApduOverNciHciGenerator
{
public:
    static std::vector<SimSequenceStep> CreateCommandSequence(
        _In_ PCWSTR name,
        _In_ uint8_t connectionId,
        _In_ uint8_t pipeId,
        _In_reads_bytes_(apduCommandLength) const void* apduCommand,
        _In_ DWORD apduCommandLength);

    static std::vector<SimSequenceStep> CreateResponseSequence(
        _In_ PCWSTR name,
        _In_ uint8_t connectionId,
        _In_ uint8_t pipeId,
        _In_reads_bytes_(apduResponseLength) const void* apduResponse,
        _In_ DWORD apduResponseLength);

private:
    static std::vector<SimSequenceStep> CreateSequence(
        _In_ PCWSTR name,
        _In_ uint8_t connectionId,
        _In_ uint8_t pipeId,
        _In_ bool isCommand,
        _In_reads_bytes_(apduLength) const void* apdu,
        _In_ DWORD apduLength);
};
