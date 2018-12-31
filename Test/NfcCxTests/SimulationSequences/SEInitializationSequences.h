//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <Simulation/SimSequenceView.h>
#include <Simulation/SimSequenceStep.h>

// NCI command/response sequences relating to NFCEE enumeration.
struct SEInitializationSequences
{
    static constexpr uint8_t EseNfceeId = phHciNfc_e_ProprietaryHostID_Min;
    static constexpr uint8_t EseApduPipeId = 0x19;
    static constexpr uint8_t HciNetworkConnectionId = 0x01;

    static const SimSequenceStep HciNetworkCredit;

    struct Common
    {
        static const SimSequenceStep PreNfceeDiscovery;
        static const SimSequenceStep NfceeDiscoverCommand_Nci1;
        static const SimSequenceStep NfceeDiscoverCommand_Nci2;
        static const SimSequenceStep NfceeDiscoveryComplete;

        static const SimSequenceStep InitializeStartSequence_Nci1[2];
        static const SimSequenceStep InitializeStartSequence_Nci2[2];
        static const SimSequenceView InitializeStartSequence(bool isNci2);

        static const SimSequenceStep InitializeEndSequence[1];
    };

    struct NoSEs
    {
        static const SimSequenceStep NfceeDiscoverResponse;

        static const SimSequenceView InitializeSequence_Nci1[3];
        static const SimSequenceView InitializeSequence_Nci2[3];
        static const SimSequenceView InitializeSequence(bool isNci2);
    };

    struct WithEse
    {
        // Initialize steps.
        static const SimSequenceStep NfceeDiscoverResponse;
        static const SimSequenceStep HciNetworkEnumeration;
        static const SimSequenceStep HciNetworkCreateConnectionCommand;
        static const SimSequenceStep HciNetworkCreateConnectionResponse;
        static const SimSequenceStep EseEnumeration_Nci1;
        static const SimSequenceStep EseEnumeration_Nci2;
        static const SimSequenceStep EseSupportsNfcA;
        static const SimSequenceStep EseSupportsNfcB;
        static const SimSequenceStep EseSupportsNfcF;
        static const SimSequenceStep OpenAdminPipeCommand;
        static const SimSequenceStep OpenAdminPipeResponse;
        static const SimSequenceStep SetWhitelistCommand;
        static const SimSequenceStep SetWhitelistResponse;
        static const SimSequenceStep SetHostTypeCommand;
        static const SimSequenceStep SetHostTypeResponse;
        static const SimSequenceStep GetSessionIdCommand;
        static const SimSequenceStep GetSessionIdResponse;
        static const SimSequenceStep GetHostListCommand;
        static const SimSequenceStep GetHostListResponse;
        static const SimSequenceStep GetHostTypeListCommand;
        static const SimSequenceStep GetHostTypeListResponse;

        // Enable and disable command/responses.
        static const SimSequenceStep EseEnableCommand;
        static const SimSequenceStep EseEnableResponse;
        static const SimSequenceStep EseEnableNotification;
        static const SimSequenceStep EseDisableCommand;
        static const SimSequenceStep EseDisableResponse;
        static const SimSequenceStep EseDisableNotification;

        // Power and link control command/responses.
        static const SimSequenceStep EsePowerOnCommand;
        static const SimSequenceStep EsePowerOnResponse;
        static const SimSequenceStep EsePowerAndLinkOnCommand;
        static const SimSequenceStep EsePowerAndLinkOnResponse;
        static const SimSequenceStep EsePowerOffCommand;
        static const SimSequenceStep EsePowerOffResponse;

        // Client connect and disconnect steps.
        static const SimSequenceStep SetDefaultRoutingTableCommand;
        static const SimSequenceStep SetDefaultRoutingTableResponse;
        static const SimSequenceStep EseResetCommand;
        static const SimSequenceStep EseAtrEvent;

        // Sequences
        static const SimSequenceView InitializeSequence_Nci1[32];
        static const SimSequenceView InitializeSequence_Nci2[31];
        static const SimSequenceView InitializeSequence(bool isNci2);

        static const SimSequenceView ClientConnectedSequence_Nci1[4];
        static const SimSequenceView ClientConnectedSequence_Nci2[5];
        static const SimSequenceView ClientConnectedSequence(bool isNci2);

        static const SimSequenceView GetAtrSequence[3];

        static const SimSequenceView ClientDisconnectedSequence_Nci1[4];
        static const SimSequenceView ClientDisconnectedSequence_Nci2[5];
        static const SimSequenceView ClientDisconnectedSequence(bool isNci2);
    };
};
