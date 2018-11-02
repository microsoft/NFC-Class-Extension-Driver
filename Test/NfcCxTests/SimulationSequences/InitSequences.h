//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <Simulation/SimSequenceStep.h>

// Common NCI command/response sequences relating to NCI initialize and uninitialize.
struct InitSequences
{
    static const SimSequenceStep NciResetCommand;
    static const SimSequenceStep NciResetResponse;

    struct InitializeNoSEs
    {
        static const SimSequenceStep PreInitialize;
        static const SimSequenceStep InitializeCommand;
        static const SimSequenceStep InitializeResponse;
        static const SimSequenceStep InitializeComplete;
        static const SimSequenceStep GetConfigCommand;
        static const SimSequenceStep GetConfigResponse;
        static const SimSequenceStep PreNfceeDiscovery;
        static const SimSequenceStep NfceeDiscoverCommand;
        static const SimSequenceStep NfceeDiscoverResponse;
        static const SimSequenceStep NfceeDiscoveryComplete;

        static const SimSequenceStep Sequence[12];
    };

    struct Uninitialize
    {
        static const SimSequenceStep PreShutdown;
        static const SimSequenceStep ShutdownComplete;

        static const SimSequenceStep Sequence[4];
    };
};
