//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include "NciControlPacket.h"
#include "NciSimConnector.h"
#include "SimSequenceView.h"
#include "SimSequenceStep.h"

class SimSequenceRunner
{
public:
    static void Run(
        _In_ NciSimConnector& simConnector,
        _In_ const SimSequenceStep& step);

    static void Run(
        _In_ NciSimConnector& simConnector,
        _In_reads_(stepListSize) const SimSequenceStep* stepList,
        _In_ size_t stepListSize);

    template <size_t ArraySize>
    static void Run(
        _In_ NciSimConnector& simConnector,
        const SimSequenceStep (&stepList)[ArraySize])
    {
        Run(simConnector, stepList, ArraySize);
    }

    static void Run(
        NciSimConnector& simConnector,
        SimSequenceView sequence);
};
