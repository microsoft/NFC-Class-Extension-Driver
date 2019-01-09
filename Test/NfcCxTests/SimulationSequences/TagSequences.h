//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <stdint.h>

#include <Simulation/SimSequenceStep.h>

// Common NCI command/response sequences relating to reading/writing tags.
struct TagSequences
{
    static const SimSequenceStep Connection0CreditStep;

    // NTAG 216 tag with a payload containing http://www.bing.com
    struct Ntag216
    {
        static const SimSequenceStep ActivatedNotification;
        static const SimSequenceStep GetVersionCommand;
        static const SimSequenceStep GetVersionResponse;
        static const SimSequenceStep DeactivateCommand;
        static const SimSequenceStep DeactivateResponse;
        static const SimSequenceStep DeactivateNotification;
        static const SimSequenceStep DiscoverSelectCommand;
        static const SimSequenceStep DiscoverSelectResponse;

        static const SimSequenceStep ReadPage2Command;
        static const SimSequenceStep ReadPage2Response;
        static const SimSequenceStep ReadPage4Command;
        static const SimSequenceStep ReadPage4Response;
        static const SimSequenceStep ReadPage8Command;
        static const SimSequenceStep ReadPage8Response;
        static const SimSequenceStep ReadErrorResponse;

        static const SimSequenceStep ActivatedSequence[10];
        static const SimSequenceStep ResetSequence[6];

        static const SimSequenceStep ReadSequence[18];
        static const SimSequenceStep PresenceCheckConnected[2];
        static const SimSequenceStep PresenceCheckDisconnected[2];

        static const uint8_t Atr[20];
    };
};
