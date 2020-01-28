//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <Simulation/SimSequenceStep.h>

// Sequences relating to reading/writing Type 5 NFC tags (T5T).
struct Type5TagSequences
{
    struct LRI2K
    {
        static const SimSequenceStep ActivatedNotification;
        static const SimSequenceStep GetSystemInfoCommand;
        static const SimSequenceStep GetSystemInfoResponse;

        static const SimSequenceStep ReadBlock0Command;
        static const SimSequenceStep ReadBlock0Response;
        static const SimSequenceStep ReadBlock1Command;
        static const SimSequenceStep ReadBlock1Response;
        static const SimSequenceStep ReadBlock2Command;
        static const SimSequenceStep ReadBlock2Response;
        static const SimSequenceStep ReadBlock3Command;
        static const SimSequenceStep ReadBlock3Response;
        static const SimSequenceStep ReadBlock4Command;
        static const SimSequenceStep ReadBlock4Response;
        static const SimSequenceStep PresenceCheckCommand;
        static const SimSequenceStep PresenceCheckConnectedResponse;

        static const SimSequenceStep ActivatedSequence[4];
        static const SimSequenceStep ReadSequence[21];
        static const SimSequenceStep PresenceCheckConnectedSequence[3];
        static const SimSequenceStep PresenceCheckDisconnectedSequence[2];
    };
};
