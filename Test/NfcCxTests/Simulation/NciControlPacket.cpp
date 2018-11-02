//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "NciControlPacket.h"

#include <NciDef.h>

NciControlPacket::NciControlPacket(
    _In_ uint8_t type,
    _In_ uint8_t groupId,
    _In_ uint8_t opcodeId)
    :
    NciControlPacket(type, groupId, opcodeId, nullptr, 0, true)
{
}

NciControlPacket::NciControlPacket(
    _In_ uint8_t type,
    _In_ uint8_t groupId,
    _In_ uint8_t opcodeId,
    _In_ std::initializer_list<uint8_t> payload)
    :
    NciControlPacket(type, groupId, opcodeId, payload, true)
{
}

NciControlPacket::NciControlPacket(
    _In_ uint8_t type,
    _In_ uint8_t groupId,
    _In_ uint8_t opcodeId,
    _In_ std::initializer_list<uint8_t> payload,
    _In_ bool packetBoundary)
    :
    NciControlPacket(type, groupId, opcodeId, payload.begin(), uint8_t(payload.size()), packetBoundary)
{
    if (payload.size() > size_t(std::numeric_limits<uint8_t>::max()))
    {
        throw std::exception("NciControlPacket payload is too big.");
    }
}

NciControlPacket::NciControlPacket(
    _In_ uint8_t type,
    _In_ uint8_t groupId,
    _In_ uint8_t opcodeId,
    _In_reads_(payloadLength) const uint8_t* payload,
    _In_ uint8_t payloadLength,
    _In_ bool packetBoundary)
{
    // Refer to: NFC Controller Interface (NCI), Version 1.1, Section 3.4.2
    uint8_t packetBoundaryFlag = packetBoundary ? NCI_PACKET_PBF_COMPLETE : NCI_PACKET_PBF_NOT_COMPLETE;

    _NciPacket.Header[0] =
        ((type << NCI_PACKET_MT_SHIFT) & NCI_PACKET_MT_MASK) |
        ((packetBoundaryFlag << NCI_PACKET_PBF_SHIFT) & NCI_PACKET_PBF_MASK) |
        (groupId & NCI_PACKET_CONTROL_GID_MASK);

    _NciPacket.Header[1] = opcodeId & NCI_PACKET_CONTROL_OID_MASK;

    _NciPacket.PayloadLength = payloadLength;
    _NciPacketLength = NciPacketRaw::TotalHeaderLength + payloadLength;

    // Copy the payload data into the packet.
    if (payloadLength != 0)
    {
        memcpy(_NciPacket.Payload, payload, payloadLength);
    }
}
