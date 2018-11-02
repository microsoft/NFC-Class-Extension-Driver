//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <NfcCxTestDeviceDriver.h>
#include <string>

#include "NciControlPacket.h"
#include "NciDataPacket.h"
#include "NciPacket.h"

enum class SimSequenceStepType
{
    NciWrite,
    NciRead,
    SequenceHandler,
};

struct SimSequenceStep
{
    SimSequenceStepType Type;
    std::wstring StepName;
    NciPacket NciPacketData;
    NFC_CX_SEQUENCE SequenceHandlerType;
    NTSTATUS SequenceHandlerStatus;
    ULONG SequenceHandlerFlags;

    static SimSequenceStep NciWrite(
        std::wstring stepName,
        const NciPacket& packet);
    static SimSequenceStep NciRead(
        std::wstring stepName,
        const NciPacket& packet);
    static SimSequenceStep NciControlWrite(
        std::wstring stepName,
        const NciControlPacket& packet);
    static SimSequenceStep NciControlRead(
        std::wstring stepName,
        const NciControlPacket& packet);
    static SimSequenceStep NciDataWrite(
        std::wstring stepName,
        const NciDataPacket& packet);
    static SimSequenceStep NciDataRead(
        std::wstring stepName,
        const NciDataPacket& packet);

    static SimSequenceStep SequenceHandler(
        std::wstring stepName,
        NFC_CX_SEQUENCE type,
        NTSTATUS status,
        ULONG flags);
};
