//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <initializer_list>

#include "NciPacket.h"

// NFC Controller Interface (NCI), Version 1.1, Section 3.4.3, Format of Data Packets
class NciDataPacket :
    public NciPacket
{
public:
    NciDataPacket(
        _In_ uint8_t connectionId,
        _In_ std::initializer_list<uint8_t> payload);

    NciDataPacket(
        _In_ uint8_t connectionId,
        _In_ std::initializer_list<uint8_t> payload,
        _In_ bool packetBoundary);

    NciDataPacket(
        _In_ uint8_t connectionId,
        _In_reads_(payloadLength) const uint8_t* payload,
        _In_ uint8_t payloadLength,
        _In_ bool packetBoundary);
};
