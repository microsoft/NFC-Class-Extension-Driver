//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <phLibNfc_Internal.h>

#include <SimulationSequences\SEInitializationSequences.h>

#include "ApduOverNciHciGenerator.h"

std::vector<SimSequenceStep>
ApduOverNciHciGenerator::CreateCommandSequence(
    _In_ PCWSTR name,
    _In_ uint8_t connectionId,
    _In_ uint8_t pipeId,
    _In_reads_bytes_(apduCommandLength) const void* apduCommand,
    _In_ DWORD apduCommandLength)
{
    return CreateSequence(name, connectionId, pipeId, true, apduCommand, apduCommandLength);
}

std::vector<SimSequenceStep>
ApduOverNciHciGenerator::CreateResponseSequence(
    _In_ PCWSTR name,
    _In_ uint8_t connectionId,
    _In_ uint8_t pipeId,
    _In_reads_bytes_(apduResponseLength) const void* apduResponse,
    _In_ DWORD apduResponseLength)
{
    return CreateSequence(name, connectionId, pipeId, false, apduResponse, apduResponseLength);
}

std::vector<SimSequenceStep>
ApduOverNciHciGenerator::CreateSequence(
    _In_ PCWSTR name,
    _In_ uint8_t connectionId,
    _In_ uint8_t pipeId,
    _In_ bool isCommand,
    _In_reads_bytes_(apduLength) const void* apdu,
    _In_ DWORD apduLength)
{
    DWORD totalParts = 1;
    if (apduLength > NciHciDataPacket::MaxFirstPayloadSize)
    {
        DWORD remainingApduLength = apduLength - NciHciDataPacket::MaxFirstPayloadSize;

        totalParts += remainingApduLength / NciHciDataPacket::MaxSubsequentPayloadSize;
        if (remainingApduLength % NciHciDataPacket::MaxSubsequentPayloadSize != 0)
        {
            totalParts += 1; // round up
        }
    }

    std::vector<SimSequenceStep> result;
    result.reserve(isCommand ? totalParts * 2 : totalParts);

    DWORD packetNumber = 0;
    const uint8_t* apduItr = reinterpret_cast<const uint8_t*>(apdu);
    const uint8_t* apduItrEnd = apduItr + apduLength;

    for (;;) // while(true)
    {
        WCHAR stepName[128];
        swprintf_s(stepName, L"%s (%u/%u)", name, packetNumber + 1, totalParts);

        bool isFirstPacket = packetNumber == 0;
        uint8_t maxPayloadSize = isFirstPacket ?
            NciHciDataPacket::MaxFirstPayloadSize :
            NciHciDataPacket::MaxSubsequentPayloadSize;

        // Calculate the size of this packet.
        bool finalPacket = true;
        size_t payloadSize = size_t(apduItrEnd - apduItr);
        if (payloadSize > maxPayloadSize)
        {
            payloadSize = maxPayloadSize;
            finalPacket = false;
        }

        // Check if this is the first packet.
        NciPacket hciPacket;
        if (isFirstPacket)
        {
            uint8_t instruction = isCommand ? PHHCINFC_EVT_C_APDU : PHHCINFC_EVT_R_APDU;
            hciPacket = NciHciDataPacket(connectionId, pipeId, phHciNfc_e_HcpMsgTypeEvent, instruction, apduItr, uint8_t(payloadSize), finalPacket);
        }
        else
        {
            hciPacket = NciHciDataPacket(connectionId, pipeId, apduItr, uint8_t(payloadSize), finalPacket);
        }

        // Check if this is an APDU command (or APDU response).
        if (isCommand)
        {
            // Add the APDU command packet.
            result.push_back(SimSequenceStep::NciWrite(stepName, hciPacket));

            // Add the flow control credit notification.
            result.push_back(SEInitializationSequences::HciNetworkCredit);
        }
        else
        {
            // Add the APDU response packet.
            result.push_back(SimSequenceStep::NciRead(stepName, hciPacket));
        }

        // Check if this was the last packet.
        if (finalPacket)
        {
            break;
        }

        // Process next packet.
        apduItr += payloadSize;
        packetNumber += 1;
    }

    return std::move(result);
}
