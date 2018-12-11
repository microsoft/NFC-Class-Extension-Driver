//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <Tests\TestLogging.h>
#include "SimSequenceRunner.h"
#include "VerifyHelpers.h"

void
SimSequenceRunner::VerifyStep(
    const SimSequenceStep& expectedStep,
    const NciSimCallbackMessage& message)
{
    // Log the actual message and ensure it matches what is expected.
    switch (message.Header()->Type)
    {
    case NciSimCallbackType::NciWrite:
    {
        DWORD nciPacketSize = message.Length() - NciSimCallbackNciWriteMinSize;
        auto nciWrite = static_cast<const NciSimCallbackNciWrite*>(message.Header());

        if (expectedStep.Type != SimSequenceStepType::NciWrite ||
            !AreArraysEqual(expectedStep.NciPacketData.PacketBytes(), expectedStep.NciPacketData.PacketBytesLength(), nciWrite->NciMessage, nciPacketSize))
        {
            LogExpectedStep(expectedStep);
            LogByteBuffer(L"Actual NCI packet  ", nciWrite->NciMessage, nciPacketSize);
            VERIFY_FAIL(L"Step and driver message don't match.");
        }

        break;
    }
    case NciSimCallbackType::SequenceHandler:
    {
        auto params = static_cast<const NciSimCallbackSequenceHandler*>(message.Header());

        if (expectedStep.Type != SimSequenceStepType::SequenceHandler ||
            expectedStep.SequenceHandlerType != params->Sequence)
        {
            LogExpectedStep(expectedStep);
            LOG_COMMENT(L"Actual sequence handler:   %d", int(params->Sequence));
            VERIFY_FAIL(L"Step and driver message don't match.");
        }

        break;
    }
    case NciSimCallbackType::D0Entry:
    {
        if (expectedStep.Type != SimSequenceStepType::D0Entry)
        {
            LogExpectedStep(expectedStep);
            LOG_COMMENT(L"Actual:   D0 Entry");
            VERIFY_FAIL(L"Step and driver message don't match.");
        }
        break;
    }
    case NciSimCallbackType::D0Exit:
    {
        if (expectedStep.Type != SimSequenceStepType::D0Exit)
        {
            LogExpectedStep(expectedStep);
            LOG_COMMENT(L"Actual:   D0 Exit");
            VERIFY_FAIL(L"Step and driver message don't match.");
        }
        break;
    }
    default:
        LogExpectedStep(expectedStep);
        VERIFY_FAIL_MSG(L"Unknown driver message type: %d", message.Header()->Type);
        break;
    }

    LOG_COMMENT(L"Step and driver message match.");
}

void
SimSequenceRunner::LogExpectedStep(const SimSequenceStep& expectedStep)
{
    switch (expectedStep.Type)
    {
    case SimSequenceStepType::NciWrite:
    {
        LogByteBuffer(L"Expected NCI packet", expectedStep.NciPacketData.PacketBytes(), expectedStep.NciPacketData.PacketBytesLength());
        break;
    }
    case SimSequenceStepType::SequenceHandler:
    {
        LOG_COMMENT(L"Expected sequence handler: %d", int(expectedStep.SequenceHandlerType));
        break;
    }
    case SimSequenceStepType::D0Entry:
    {
        LOG_COMMENT(L"Expected: D0 Entry");
        break;
    }
    case SimSequenceStepType::D0Exit:
    {
        LOG_COMMENT(L"Expected: D0 Exit");
        break;
    }
    default:
        LOG_ERROR(L"Step (%d) doesn't have an equivalent NciSimCallbackType.", int(expectedStep.Type));
        break;
    }
}

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
        NciSimCallbackMessage message = simConnector.ReceiveLibNfcThreadCallback();
        VerifyStep(step, message);

        simConnector.SendNciWriteCompleted();

        break;
    }
    case SimSequenceStepType::NciRead:
    {
        simConnector.SendNciRead(step.NciPacketData);
        break;
    }
    case SimSequenceStepType::SequenceHandler:
    {
        NciSimCallbackMessage message = simConnector.ReceiveLibNfcThreadCallback();
        VerifyStep(step, message);

        simConnector.SendSequenceCompleted(step.SequenceHandlerStatus, step.SequenceHandlerFlags);
        break;
    }
    case SimSequenceStepType::D0Entry:
    {
        NciSimCallbackMessage message = simConnector.ReceivePowerCallback();
        VerifyStep(step, message);

        break;
    }
    case SimSequenceStepType::D0Exit:
    {
        NciSimCallbackMessage message = simConnector.ReceivePowerCallback();
        VerifyStep(step, message);

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
    switch (sequence.GetType())
    {
    case SimSequenceView::Type::SequenceList:
        Run(simConnector, sequence.GetSequenceList(), sequence.GetSequenceListSize());
        break;

    case SimSequenceView::Type::StepList:
        Run(simConnector, sequence.GetStepList(), sequence.GetStepListSize());
        break;
    }
}

void
SimSequenceRunner::Run(
    _In_ NciSimConnector& simConnector,
    _In_reads_(sequenceListSize) const SimSequenceView* sequenceList,
    _In_ size_t sequenceListSize)
{
    for (const SimSequenceView* itr = sequenceList; itr != sequenceList + sequenceListSize; ++itr)
    {
        Run(simConnector, *itr);
    }
}
