//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <array>

#include "NciHciDataPacket.h"

NciHciDataPacket::NciHciDataPacket(
    _In_ uint8_t connectionId,
    _In_ uint8_t pipeId,
    _In_ phHciNfc_MsgType_t messageType,
    _In_ uint8_t instruction,
    _In_ std::initializer_list<uint8_t> payload)
    :
    NciHciDataPacket(connectionId, pipeId, messageType, instruction, payload.begin(), uint8_t(payload.size()), true)
{
}

NciHciDataPacket::NciHciDataPacket(
    _In_ uint8_t connectionId,
    _In_ uint8_t pipeId,
    _In_ phHciNfc_MsgType_t messageType,
    _In_ uint8_t instruction,
    _In_ std::initializer_list<uint8_t> payload,
    _In_ bool finalPacket)
    :
    NciHciDataPacket(connectionId, pipeId, messageType, instruction, payload.begin(), uint8_t(payload.size()), finalPacket)
{
}

NciHciDataPacket::NciHciDataPacket(
    _In_ uint8_t connectionId,
    _In_ uint8_t pipeId,
    _In_ phHciNfc_MsgType_t messageType,
    _In_ uint8_t instruction,
    _In_reads_(payloadLength) const uint8_t* payload,
    _In_ uint8_t payloadLength,
    _In_ bool finalPacket)
    :
    NciDataPacket(connectionId, GenerateFirstPacket(pipeId, messageType, instruction, payload, payloadLength, finalPacket).data(), CalculateFirstPacketLength(payloadLength), true)
{
}

NciHciDataPacket::NciHciDataPacket(
    _In_ uint8_t connectionId,
    _In_ uint8_t pipeId,
    _In_ std::initializer_list<uint8_t> payload)
    :
    NciHciDataPacket(connectionId, pipeId, payload.begin(), uint8_t(payload.size()), true)
{
}

NciHciDataPacket::NciHciDataPacket(
    _In_ uint8_t connectionId,
    _In_ uint8_t pipeId,
    _In_ std::initializer_list<uint8_t> payload,
    _In_ bool finalPacket)
    :
    NciHciDataPacket(connectionId, pipeId, payload.begin(), uint8_t(payload.size()), finalPacket)
{
}

NciHciDataPacket::NciHciDataPacket(
    _In_ uint8_t connectionId,
    _In_ uint8_t pipeId,
    _In_reads_(payloadLength) const uint8_t* payload,
    _In_ uint8_t payloadLength,
    _In_ bool finalPacket)
    :
    NciDataPacket(connectionId, GenerateSubsequentPacket(pipeId, payload, payloadLength, finalPacket).data(), CalculateSubsequentPacketLength(payloadLength), true)
{
}

constexpr uint8_t
NciHciDataPacket::GenerateHcpHeader(bool isFinalPacket, uint8_t pipeId)
{
    // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 5.1, HCP packets
    constexpr uint8_t chainBit = 0x80; // bit 8
    constexpr uint8_t instructionMask = 0x7F; // bits [0,7]

    return (isFinalPacket ? chainBit : 0x00) |
        (pipeId & instructionMask);
}

std::array<uint8_t, NciPacketRaw::MaxPayloadLength>
NciHciDataPacket::GenerateFirstPacket(
    _In_ uint8_t pipeId,
    _In_ phHciNfc_MsgType_t messageType,
    _In_ uint8_t instruction,
    _In_reads_(payloadLength) const uint8_t* payload,
    _In_ uint8_t payloadLength,
    _In_ bool finalPacket)
{
    std::array<uint8_t, NciPacketRaw::MaxPayloadLength> packet = {};

    // Add HCP packet header
    packet[0] = GenerateHcpHeader(finalPacket, pipeId);

    // Add HCP message header
    // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 5.2, HCP message structure
    constexpr uint8_t messageTypeBitShift = 6;
    constexpr uint8_t messageTypeBitMask = 0xC0; // bits [7,8]
    constexpr uint8_t instructionBitMask = 0x3F; // bits [0,6]
    packet[1] = ((messageType << messageTypeBitShift) & messageTypeBitMask) |
        (instruction & instructionBitMask);

    // Ensure HCI payload isn't too big.
    if (payloadLength > NciHciDataPacket::MaxFirstPayloadSize)
    {
        throw std::exception("NciHciDataPacket payload is too big.");
    }

    // Copy payload.
    std::copy(payload, payload + payloadLength, packet.data() + 2);
    return packet;
}

constexpr uint8_t
NciHciDataPacket::CalculateFirstPacketLength(uint8_t payloadLength)
{
    return payloadLength + 2; // Add 2 bytes for HCP header + HCP message header
}

std::array<uint8_t, NciPacketRaw::MaxPayloadLength>
NciHciDataPacket::GenerateSubsequentPacket(
    _In_ uint8_t pipeId,
    _In_reads_(payloadLength) const uint8_t* payload,
    _In_ uint8_t payloadLength,
    _In_ bool finalPacket)
{
    std::array<uint8_t, NciPacketRaw::MaxPayloadLength> packet = {};

    // Add HCP packet header
    packet[0] = GenerateHcpHeader(finalPacket, pipeId);

    if (payloadLength > NciHciDataPacket::MaxSubsequentPayloadSize)
    {
        throw std::exception("NciHciDataPacket payload is too big.");
    }

    std::copy(payload, payload + payloadLength, packet.data() + 1);
    return packet;
}

constexpr uint8_t
NciHciDataPacket::CalculateSubsequentPacketLength(uint8_t payloadLength)
{
    return payloadLength + 1; // Add 1 byte for HCP header
}
