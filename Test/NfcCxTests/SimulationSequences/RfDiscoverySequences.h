//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <Simulation/SimSequenceView.h>
#include <Simulation/SimSequenceStep.h>

// Standard NCI command/response sequences relating to NCI discovery.
struct RfDiscoverySequences
{
    struct DiscoveryStart
    {
        static const SimSequenceStep PreDiscoveryStart;
        static const SimSequenceStep GetConfigCommand;
        static const SimSequenceStep GetConfigResponse;
        static const SimSequenceStep DiscoverCommand_Nci1;
        static const SimSequenceStep DiscoverCommand_Nci2;
        static const SimSequenceStep DiscoverResponse;
        static const SimSequenceStep DiscoverStartComplete;

        static const SimSequenceStep Sequence_Nci1[6];
        static const SimSequenceStep Sequence_Nci2[6];
        static const SimSequenceView Sequence(bool isNci2);
    };

    struct DiscoveryStop
    {
        static const SimSequenceStep PreDiscoverStop;
        static const SimSequenceStep DeactivateCommand;
        static const SimSequenceStep DeactivateResponse;
        static const SimSequenceStep DiscoverStopComplete;

        static const SimSequenceStep Sequence[4];
    };
};
