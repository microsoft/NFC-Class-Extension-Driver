//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <Simulation/SimSequenceView.h>
#include <Simulation/SimSequenceStep.h>

// Common NCI command/response sequences relating to NCI initialize and uninitialize.
struct InitSequences
{
    struct Reset
    {
        static const SimSequenceStep NciResetCommand;
        static const SimSequenceStep NciResetResponse_Nci1;
        static const SimSequenceStep NciResetResponse_Nci2;
        static const SimSequenceStep NciResetNotification_Nci2;

        static const SimSequenceStep Sequence_Nci1[2];
        static const SimSequenceStep Sequence_Nci2[3];
    };

    struct InitializeNoSEs
    {
        static const SimSequenceStep PreInitialize;
        static const SimSequenceStep InitializeCommand_Nci1;
        static const SimSequenceStep InitializeCommand_Nci2;
        static const SimSequenceStep InitializeResponse_Nci1;
        static const SimSequenceStep InitializeResponse_Nci2;
        static const SimSequenceStep InitializeComplete;
        static const SimSequenceStep GetConfigCommand;
        static const SimSequenceStep GetConfigResponse;
        static const SimSequenceStep PreNfceeDiscovery;
        static const SimSequenceStep NfceeDiscoverCommand_Nci1;
        static const SimSequenceStep NfceeDiscoverCommand_Nci2;
        static const SimSequenceStep NfceeDiscoverResponse;
        static const SimSequenceStep NfceeDiscoveryComplete;

        static const SimSequenceView Sequence_Nci1[11];
        static const SimSequenceView Sequence_Nci2[11];
        static const SimSequenceView Sequence(bool isNci2);
    };

    struct Uninitialize
    {
        static const SimSequenceStep PreShutdown;
        static const SimSequenceStep ShutdownComplete;

        static const SimSequenceView Sequence_Nci1[3];
        static const SimSequenceView Sequence_Nci2[3];
        static const SimSequenceView Sequence(bool isNci2);
    };
};
