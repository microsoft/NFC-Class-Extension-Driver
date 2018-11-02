//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <phNciNfc.h>
#include <phNciNfc_Core.h>
#include <phNciNfc_CoreStatus.h>
#include <phNciNfc_RfConfig.h>

#include "RfDiscoverySequences.h"

const SimSequenceStep RfDiscoverySequences::DiscoveryStart::PreDiscoveryStart = SimSequenceStep::SequenceHandler(
    L"SequencePreRfDiscStart",
    SequencePreRfDiscStart,
    STATUS_SUCCESS,
    0
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStart::GetConfigCommand = SimSequenceStep::NciControlWrite(
    L"CORE_GET_CONFIG_CMD",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.3.2, CORE_GET_CONFIG_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreGetConfigCmdOid,
        {
            4, // Number of parameters, that follow.
                PHNCINFC_RFCONFIG_PF_BIT_RATE, // PF_BIT_RATE: NFC-F poll initial bit-rate
                PHNCINFC_RFCONFIG_LA_SEL_INFO, // LA_SEL_INFO: NFC-A listen supported protocols
                PHNCINFC_RFCONFIG_LF_PROTOCOL_TYPE, // LF_PROTOCOL_TYPE: NFC-F listen supported protocols
                PHNCINFC_RFCONFIG_TOTAL_DURATION, // TOTAL_DURATION: Duration (ms) of a discovery period
        }
    }
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStart::GetConfigResponse = SimSequenceStep::NciControlRead(
    L"CORE_GET_CONFIG_RSP",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.3.2, CORE_GET_CONFIG_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreGetConfigCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
            4, // Number of TLV parameters, that follow.
                PHNCINFC_RFCONFIG_PF_BIT_RATE, // Id
                1, // Length
                phNciNfc_e_BitRate212, // 212 Kb/s

                PHNCINFC_RFCONFIG_LA_SEL_INFO, // Id
                1, // Length
                (1 << PHNCINFC_RFCONFIG_LNFCA_NFCDEP_OFFSET), // NFC-DEP supported

                PHNCINFC_RFCONFIG_LF_PROTOCOL_TYPE, // Id
                1, // Length
                (1 << PHNCINFC_RFCONFIG_LNFCF_NFCDEP_OFFSET), // NFC-DEP supported

                PHNCINFC_RFCONFIG_TOTAL_DURATION,
                2, // Length
                0x2C, 0x01, // 300 (0x012C) milliseconds
        }
    }
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStart::DiscoverCommand = SimSequenceStep::NciControlWrite(
    L"RF_DISCOVER_CMD",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 7.1, RF_DISCOVER_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfDiscoverCmdOid,
        {
            11, // Number of parameters, that follow.
                phNciNfc_NFCA_Active_Poll,
                1, // Poll every discovery loop

                phNciNfc_NFCA_Poll,
                1, // Poll every discovery loop

                phNciNfc_NFCB_Poll,
                1, // Poll every discovery loop

                phNciNfc_NFCF_Active_Poll,
                1, // Poll every discovery loop

                phNciNfc_NFCF_Poll,
                1, // Poll every discovery loop

                phNciNfc_NFCISO15693_Poll,
                1, // Poll every discovery loop

                phNciNfc_NFCA_Kovio_Poll,
                1, // Poll every discovery loop

                phNciNfc_NFCA_Listen,
                1, // Poll every discovery loop

                phNciNfc_NFCA_Active_Listen,
                1, // Poll every discovery loop

                phNciNfc_NFCF_Listen,
                1, // Poll every discovery loop

                phNciNfc_NFCF_Active_Listen,
                1, // Poll every discovery loop
        }
    }
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStart::DiscoverResponse = SimSequenceStep::NciControlRead(
    L"RF_DISCOVER_RSP",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 7.1, RF_DISCOVER_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfDiscoverCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        }
    }
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStart::DiscoverStartComplete = SimSequenceStep::SequenceHandler(
    L"SequenceRfDiscStartComplete",
    SequenceRfDiscStartComplete,
    STATUS_SUCCESS,
    0
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStop::PreDiscoverStop = SimSequenceStep::SequenceHandler(
    L"SequencePreRfDiscStop",
    SequencePreRfDiscStop,
    STATUS_SUCCESS,
    0
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStop::DeactivateCommand = SimSequenceStep::NciControlWrite(
    L"RF_DEACTIVATE_CMD",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.3.2, RF_DEACTIVATE_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfDeactivateCmdOid,
        {
            phNciNfc_e_IdleMode, // Exit discovery
        }
    }
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStop::DeactivateResponse = SimSequenceStep::NciControlRead(
    L"RF_DEACTIVATE_RSP",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.3.2, RF_DEACTIVATE_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfDeactivateCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        }
    }
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStop::DiscoverStopComplete = SimSequenceStep::SequenceHandler(
    L"SequenceRfDiscStopComplete",
    SequenceRfDiscStopComplete,
    STATUS_SUCCESS,
    0
);

const SimSequenceStep RfDiscoverySequences::DiscoveryStart::Sequence[6] =
{
    PreDiscoveryStart,
    GetConfigCommand,
    GetConfigResponse,
    DiscoverCommand,
    DiscoverResponse,
    DiscoverStartComplete,
};

const SimSequenceStep RfDiscoverySequences::DiscoveryStop::Sequence[4] =
{
    PreDiscoverStop,
    DeactivateCommand,
    DeactivateResponse,
    DiscoverStopComplete,
};
