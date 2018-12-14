//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "ApduSamples.h"

const uint8_t ApduSamples::GetDataCommand[5] = { 0x80, 0xCA, 0xFF, 0xFF, 0x00 };
const uint8_t ApduSamples::GetDataResponse[11] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x90, 0x00 };

std::vector<uint8_t>
ApduSamples::GenerateExtendedApduCommand(uint16_t payloadLength)
{
    std::vector<uint8_t> result;
    result.reserve(payloadLength + 10);

    // Add APDU header for a proprietary command.
    result.insert(result.end(), { 0x80, 0xFF, 0x00, 0x00 });

    // Add payload length using the extended format.
    result.insert(result.end(), { 0x00, uint8_t(payloadLength >> 8), uint8_t(payloadLength) });

    // Add payload of arbitrary bytes.
    for (uint16_t i = 0; i != payloadLength; ++i)
    {
        result.push_back(uint8_t(i));
    }

    // Set max response length (Le) to to the maxiumum possible value (65,536) using the extended format.
    result.insert(result.end(), { 0x00, 0x00, 0x00 });

    return std::move(result);
}

std::vector<uint8_t>
ApduSamples::GenerateExtendedApduResponse(uint32_t payloadLength)
{
    std::vector<uint8_t> result;
    result.reserve(payloadLength + 2);

    // Add payload of arbitrary bytes.
    for (uint32_t i = 0; i != payloadLength; ++i)
    {
        result.push_back(uint8_t(i));
    }

    // Set the status code to success.
    result.insert(result.end(), { 0x90, 0x00 });

    return std::move(result);
}
