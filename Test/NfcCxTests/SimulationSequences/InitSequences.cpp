//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <phNciNfc.h>
#include <phNciNfc_Core.h>
#include <phNciNfc_CoreStatus.h>
#include <phNciNfc_RfConfig.h>

#include "InitSequences.h"

const SimSequenceStep InitSequences::NciResetCommand = SimSequenceStep::NciControlWrite(
    L"CORE_RESET_CMD",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.1, CORE_RESET_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreResetCmdOid,
        {
            phNciNfc_ResetType_KeepConfig, // Reset type
        },
    }
);

const SimSequenceStep InitSequences::NciResetResponse = SimSequenceStep::NciControlRead(
    L"CORE_RESET_RSP",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.1, CORE_RESET_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreResetCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
            0x11, // NCI version (1.1)
            phNciNfc_ResetType_KeepConfig, // Reset type
        },
    }
);

const SimSequenceStep InitSequences::InitializeNoSEs::PreInitialize = SimSequenceStep::SequenceHandler(
    L"SequencePreInit",
    SequencePreInit,
    STATUS_SUCCESS,
    NFC_CX_SEQUENCE_PRE_INIT_FLAG_SKIP_CONFIG
);

const SimSequenceStep InitSequences::InitializeNoSEs::InitializeCommand = SimSequenceStep::NciControlWrite(
    L"CORE_INIT_CMD",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.2, CORE_INIT_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreInitCmdOid,
    }
);

const SimSequenceStep InitSequences::InitializeNoSEs::InitializeResponse = SimSequenceStep::NciControlRead(
    L"CORE_INIT_RSP",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.2, CORE_INIT_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreInitCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
            0b00000011, // Supported discovery config
            0b00011110, // Supported routing types
            0b00000011, // Supported routing power modes
            0b00000000, // Proprietary capabilities
            5, // Number of supported interfaces, that follow.
                phNciNfc_e_RfInterfacesNfceeDirect_RF,
                phNciNfc_e_RfInterfacesFrame_RF,
                phNciNfc_e_RfInterfacesISODEP_RF,
                phNciNfc_e_RfInterfacesNFCDEP_RF,
                phNciNfc_e_RfInterfacesTagCmd_RF,
            1, // Max logical channels
            0xFF, 0xFF, // Max routing table size
            0xFF, // Max control packet payload size
            0xFF, 0xFF, // Max size for large parameters
            0x0, // Manufacturer ID
            0x0, 0x0, 0x0, 0x0, // Manufacturer specific info
        },
    }
);

const SimSequenceStep InitSequences::InitializeNoSEs::InitializeComplete = SimSequenceStep::SequenceHandler(
    L"SequenceInitComplete",
    SequenceInitComplete,
    STATUS_SUCCESS,
    0
);

const SimSequenceStep InitSequences::InitializeNoSEs::GetConfigCommand = SimSequenceStep::NciControlWrite(
    L"CORE_GET_CONFIG_CMD",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.3.2, CORE_GET_CONFIG_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreGetConfigCmdOid,
        {
            0x4, // Number of parameters, that follow.
                PHNCINFC_RFCONFIG_ATR_REQ_GEN_BYTES,
                PHNCINFC_RFCONFIG_ATR_REQ_CONFIG,
                PHNCINFC_RFCONFIG_ATR_RES_GEN_BYTES,
                PHNCINFC_RFCONFIG_ATR_RES_CONFIG,
        },
    }
);

const SimSequenceStep InitSequences::InitializeNoSEs::GetConfigResponse = SimSequenceStep::NciControlRead(
    L"CORE_GET_CONFIG_RSP",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.3.2, CORE_GET_CONFIG_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreGetConfigCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
            0x4, // Number of parameters, that follow.
                // Simple TLV format: { 1 byte tag, 1 byte length, <256 bytes payload }
                PHNCINFC_RFCONFIG_ATR_REQ_GEN_BYTES, // Id
                0x11, // Length
                0x46, 0x66, 0x6D, 0x01, 0x01, 0x11, 0x02, 0x02, 0x03, 0x80, 0x03, 0x02, 0x00, 0x01, 0x04, 0x01, 0x64, // Value

                PHNCINFC_RFCONFIG_ATR_REQ_CONFIG, // Id
                0x01, // Length
                0x30, // Value

                PHNCINFC_RFCONFIG_ATR_RES_GEN_BYTES, // Id
                0x11, // Length
                0x46, 0x66, 0x6D, 0x01, 0x01, 0x11, 0x02, 0x02, 0x03, 0x80, 0x03, 0x02, 0x00, 0x01, 0x04, 0x01, 0x64, // Value

                PHNCINFC_RFCONFIG_ATR_RES_CONFIG, // Id
                0x01, // Length
                0x30, // Value
        },
    }
);

const SimSequenceStep InitSequences::InitializeNoSEs::PreNfceeDiscovery = SimSequenceStep::SequenceHandler(
    L"SequencePreNfceeDisc",
    SequencePreNfceeDisc,
    STATUS_SUCCESS,
    0
);

const SimSequenceStep InitSequences::InitializeNoSEs::NfceeDiscoverCommand = SimSequenceStep::NciControlWrite(
    L"NFCEE_DISCOVER_CMD",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.2, NFCEE_DISCOVER_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtNfceeDiscCmdOid,
        {
            0x01, // Enable NFCEE discovery
        },
    }
);

const SimSequenceStep InitSequences::InitializeNoSEs::NfceeDiscoverResponse = SimSequenceStep::NciControlRead(
    L"NFCEE_DISCOVER_RSP",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.2, NFCEE_DISCOVER_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtNfceeDiscCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
            0x0, // Number of NFCEEs.
        },
    }
);

const SimSequenceStep InitSequences::InitializeNoSEs::NfceeDiscoveryComplete = SimSequenceStep::SequenceHandler(
    L"SequenceNfceeDiscComplete",
    SequenceNfceeDiscComplete,
    STATUS_SUCCESS,
    0
);

const SimSequenceStep InitSequences::Uninitialize::PreShutdown = SimSequenceStep::SequenceHandler(
    L"SequencePreShutdown",
    SequencePreShutdown,
    STATUS_SUCCESS,
    0
);

const SimSequenceStep InitSequences::Uninitialize::ShutdownComplete = SimSequenceStep::SequenceHandler(
    L"SequenceShutdownComplete",
    SequenceShutdownComplete,
    STATUS_SUCCESS,
    0
);

// Error-free NCI initialize.
// Reports 0 attached SEs.
const SimSequenceStep InitSequences::InitializeNoSEs::Sequence[12] =
{
    PreInitialize,
    NciResetCommand,
    NciResetResponse,
    InitializeCommand,
    InitializeResponse,
    InitializeComplete,
    GetConfigCommand,
    GetConfigResponse,
    PreNfceeDiscovery,
    NfceeDiscoverCommand,
    NfceeDiscoverResponse,
    NfceeDiscoveryComplete,
};

// Error-free NCI uninitialize.
const SimSequenceStep InitSequences::Uninitialize::Sequence[4] =
{
    PreShutdown,
    NciResetCommand,
    NciResetResponse,
    ShutdownComplete,
};
