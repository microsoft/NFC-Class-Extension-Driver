//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <stdint.h>

// NFC Controller Interface (NCI), Version 1.1, Section 3.4, Packet Formats
#pragma pack(push,1)
struct NciPacketRaw
{
    static constexpr uint32_t HeaderBytes = 2;
    static constexpr uint32_t TotalHeaderLength = HeaderBytes + 1; // include 'PayloadLength' byte
    static constexpr uint32_t MaxPayloadLength = 255;
    static constexpr uint32_t MaxLength = TotalHeaderLength + MaxPayloadLength;

    uint8_t Header[HeaderBytes];
    uint8_t PayloadLength;
    uint8_t Payload[MaxPayloadLength];
};
#pragma pack(pop)

class NciPacket
{
public:
    const void* PacketBytes() const;
    uint32_t PacketBytesLength() const;

protected:
    NciPacketRaw _NciPacket = {};
    uint32_t _NciPacketLength = 0;
};
