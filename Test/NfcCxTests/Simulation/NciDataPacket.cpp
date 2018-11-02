//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <NciDef.h>

#include "NciDataPacket.h"

NciDataPacket::NciDataPacket(
    _In_ uint8_t connectionId,
    _In_ std::initializer_list<uint8_t> payload)
    :
    NciDataPacket(connectionId, payload, true)
{
}

NciDataPacket::NciDataPacket(
    _In_ uint8_t connectionId,
    _In_ std::initializer_list<uint8_t> payload,
    _In_ bool packetBoundary)
    :
    NciDataPacket(connectionId, payload.begin(), uint8_t(payload.size()), packetBoundary)
{
    if (payload.size() > size_t(std::numeric_limits<uint8_t>::max()))
    {
        throw std::exception("NciDataPacket payload is too big.");
    }
}

NciDataPacket::NciDataPacket(
    _In_ uint8_t connectionId,
    _In_reads_(payloadLength) const uint8_t* payload,
    _In_ uint8_t payloadLength,
    _In_ bool packetBoundary)
{
    // Refer to: NFC Controller Interface (NCI), Version 1.1, Section 3.4.2
    uint8_t packetBoundaryFlag = packetBoundary ? NCI_PACKET_PBF_COMPLETE : NCI_PACKET_PBF_NOT_COMPLETE;

    _NciPacket.Header[0] =
        ((NCI_PACKET_MT_DATA << NCI_PACKET_MT_SHIFT) & NCI_PACKET_MT_MASK) |
        ((packetBoundaryFlag << NCI_PACKET_PBF_SHIFT) & NCI_PACKET_PBF_MASK) |
        (connectionId & NCI_PACKET_DATA_CONNID_MASK);

    _NciPacket.Header[1] = 0;
    _NciPacket.PayloadLength = payloadLength;
    _NciPacketLength = NciPacketRaw::TotalHeaderLength + payloadLength;

    // Copy the payload data into the packet.
    if (payloadLength != 0)
    {
        memcpy(_NciPacket.Payload, payload, payloadLength);
    }
}
