//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "SimSequenceStep.h"

SimSequenceStep
SimSequenceStep::NciWrite(
    std::wstring stepName,
    const NciPacket& packet)
{
    return SimSequenceStep{ SimSequenceStepType::NciWrite, std::move(stepName), packet, {}, {}, {} };
}

SimSequenceStep
SimSequenceStep::NciRead(
    std::wstring stepName,
    const NciPacket& packet)
{
    return SimSequenceStep{ SimSequenceStepType::NciRead, std::move(stepName), packet, {}, {}, {} };
}

SimSequenceStep
SimSequenceStep::NciControlWrite(
    std::wstring stepName,
    const NciControlPacket& packet)
{
    return NciWrite(std::move(stepName), packet);
}

SimSequenceStep
SimSequenceStep::NciControlRead(
    std::wstring stepName,
    const NciControlPacket& packet)
{
    return NciRead(std::move(stepName), packet);
}

SimSequenceStep
SimSequenceStep::NciDataWrite(
    std::wstring stepName,
    const NciDataPacket& packet)
{
    return NciWrite(std::move(stepName), packet);
}

SimSequenceStep
SimSequenceStep::NciDataRead(
    std::wstring stepName,
    const NciDataPacket& packet)
{
    return NciRead(std::move(stepName), packet);
}

SimSequenceStep
SimSequenceStep::HciWrite(
    std::wstring stepName,
    const NciHciDataPacket& packet)
{
    return NciWrite(std::move(stepName), packet);
}

SimSequenceStep
SimSequenceStep::HciRead(
    std::wstring stepName,
    const NciHciDataPacket& packet)
{
    return NciRead(std::move(stepName), packet);
}

SimSequenceStep
SimSequenceStep::SequenceHandler(
    std::wstring stepName,
    NFC_CX_SEQUENCE type,
    NTSTATUS status,
    ULONG flags)
{
    return SimSequenceStep{ SimSequenceStepType::SequenceHandler, std::move(stepName), {}, type, status, flags };
}

SimSequenceStep
SimSequenceStep::D0Entry(
    std::wstring stepName)
{
    return SimSequenceStep{ SimSequenceStepType::D0Entry, std::move(stepName), {}, {}, {}, {} };
}

SimSequenceStep
SimSequenceStep::D0Exit(
    std::wstring stepName)
{
    return SimSequenceStep{ SimSequenceStepType::D0Exit, std::move(stepName), {}, {}, {}, {} };
}
