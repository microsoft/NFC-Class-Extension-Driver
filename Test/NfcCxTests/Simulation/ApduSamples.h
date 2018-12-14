//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <stdint.h>
#include <vector>

struct ApduSamples
{
    static const uint8_t GetDataCommand[5];
    static const uint8_t GetDataResponse[11];

    // A pseduo command/response that tests extended APDUs
    static std::vector<uint8_t> GenerateExtendedApduCommand(uint16_t payloadLength);
    static std::vector<uint8_t> GenerateExtendedApduResponse(uint32_t payloadLength);
};
