//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <initializer_list>

#include "NciPacket.h"

// NFC Controller Interface (NCI), Version 1.1, Section 3.4.2, Format of Control Packets
class NciControlPacket :
    public NciPacket
{
public:
    NciControlPacket(
        _In_ uint8_t type,
        _In_ uint8_t groupId,
        _In_ uint8_t opcodeId);

    NciControlPacket(
        _In_ uint8_t type,
        _In_ uint8_t groupId,
        _In_ uint8_t opcodeId,
        _In_ std::initializer_list<uint8_t> payload);

    NciControlPacket(
        _In_ uint8_t type,
        _In_ uint8_t groupId,
        _In_ uint8_t opcodeId,
        _In_ std::initializer_list<uint8_t> payload,
        _In_ bool packetBoundary);

    NciControlPacket(
        _In_ uint8_t type,
        _In_ uint8_t groupId,
        _In_ uint8_t opcodeId,
        _In_reads_(payloadLength) const uint8_t* payload,
        _In_ uint8_t payloadLength,
        _In_ bool packetBoundary);
};
