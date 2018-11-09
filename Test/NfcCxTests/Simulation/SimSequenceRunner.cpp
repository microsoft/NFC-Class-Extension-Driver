//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <Tests\TestLogging.h>
#include "SimSequenceRunner.h"
#include "VerifyHelpers.h"

void
SimSequenceRunner::Run(
    _In_ NciSimConnector& simConnector,
    _In_ const SimSequenceStep& step)
{
    LOG_COMMENT(L"# Step: %s.", step.StepName.c_str());

    switch (step.Type)
    {
    case SimSequenceStepType::NciWrite:
    {
        NciSimCallbackView message = simConnector.ReceiveCallback();
        VerifyNciPacket(step.NciPacketData, message);

        simConnector.SendNciWriteCompleted();

        break;
    }
    case SimSequenceStepType::NciRead:
    {
        LogByteBuffer(L"Packet", step.NciPacketData.PacketBytes(), step.NciPacketData.PacketBytesLength());
        simConnector.SendNciRead(step.NciPacketData);
        break;
    }
    case SimSequenceStepType::SequenceHandler:
    {
        NciSimCallbackView message = simConnector.ReceiveCallback();
        VerifySequenceHandler(step.SequenceHandlerType, message);

        simConnector.SendSequenceCompleted(step.SequenceHandlerStatus, step.SequenceHandlerFlags);
        break;
    }
    }
}

void
SimSequenceRunner::Run(
    _In_ NciSimConnector& simConnector,
    _In_reads_(stepListSize) const SimSequenceStep* stepList,
    _In_ size_t stepListSize)
{
    for (const SimSequenceStep* itr = stepList; itr != stepList + stepListSize; ++itr)
    {
        Run(simConnector, *itr);
    }
}

void
SimSequenceRunner::Run(
    NciSimConnector& simConnector,
    SimSequenceView sequence)
{
    Run(simConnector, sequence.GetList(), sequence.GetListSize());
}
