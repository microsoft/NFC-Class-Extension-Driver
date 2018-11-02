//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "NciPacket.h"

const void* NciPacket::PacketBytes() const
{
    return static_cast<const void*>(&_NciPacket);
}

uint32_t NciPacket::PacketBytesLength() const
{
    return _NciPacketLength;
}
