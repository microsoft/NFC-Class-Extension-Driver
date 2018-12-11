//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <initializer_list>

#include <phHciNfc_Core.h>
#include <phHciNfc_Interface.h>

#include "NciDataPacket.h"

// A HCI packet wrapped within an NCI data packet.
// ETSI Host Controller Interface (HCI), Version 12.1.0, Section 5, HCP
class NciHciDataPacket :
    public NciDataPacket
{
public:
    static constexpr uint8_t MaxHeaderSize = 2;
    static constexpr uint8_t MaxPayloadSize = NciPacketRaw::MaxPayloadLength - MaxHeaderSize;

    // Simple packet (no message fragment chaining).
    NciHciDataPacket(
        _In_ uint8_t connectionId,
        _In_ uint8_t pipeId,
        _In_ phHciNfc_MsgType_t messageType,
        _In_ uint8_t instruction,
        _In_ std::initializer_list<uint8_t> payload);

    // First packet in message fragment chain.
    NciHciDataPacket(
        _In_ uint8_t connectionId,
        _In_ uint8_t pipeId,
        _In_ phHciNfc_MsgType_t messageType,
        _In_ uint8_t instruction,
        _In_ std::initializer_list<uint8_t> payload,
        _In_ bool finalPacket);

    // First packet in message fragment chain.
    NciHciDataPacket(
        _In_ uint8_t connectionId,
        _In_ uint8_t pipeId,
        _In_ phHciNfc_MsgType_t messageType,
        _In_ uint8_t instruction,
        _In_reads_(payloadLength) const uint8_t* payload,
        _In_ uint8_t payloadLength,
        _In_ bool finalPacket);

    // Subsequent and final packet in message fragment chain.
    NciHciDataPacket(
        _In_ uint8_t connectionId,
        _In_ uint8_t pipeId,
        _In_ std::initializer_list<uint8_t> payload);

    // Subsequent packet in message fragment chain.
    NciHciDataPacket(
        _In_ uint8_t connectionId,
        _In_ uint8_t pipeId,
        _In_ std::initializer_list<uint8_t> payload,
        _In_ bool finalPacket);

    // Subsequent packet in message fragment chain.
    NciHciDataPacket(
        _In_ uint8_t connectionId,
        _In_ uint8_t pipeId,
        _In_reads_(payloadLength) const uint8_t* payload,
        _In_ uint8_t payloadLength,
        _In_ bool finalPacket);

private:
    static constexpr uint8_t
    GenerateHcpHeader(bool isFinalPacket, uint8_t pipeId);

    static std::array<uint8_t, NciPacketRaw::MaxPayloadLength>
    GenerateFirstPacket(
        _In_ uint8_t pipeId,
        _In_ phHciNfc_MsgType_t messageType,
        _In_ uint8_t instruction,
        _In_reads_(payloadLength) const uint8_t* payload,
        _In_ uint8_t payloadLength,
        _In_ bool finalPacket);

    static constexpr uint8_t
    CalculateFirstPacketLength(uint8_t payloadLength);

    static std::array<uint8_t, NciPacketRaw::MaxPayloadLength>
    GenerateSubsequentPacket(
        _In_ uint8_t pipeId,
        _In_reads_(payloadLength) const uint8_t* payload,
        _In_ uint8_t payloadLength,
        _In_ bool finalPacket);

    static constexpr uint8_t
    CalculateSubsequentPacketLength(uint8_t payloadLength);
};
