//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <phNciNfc.h>
#include <phNciNfc_Core.h>
#include <phNciNfc_CoreStatus.h>

#include "TagSequences.h"

// NFC Controller Interface (NCI), Version 1.1, Section 4.4.4, CORE_CONN_CREDITS_NTF
const SimSequenceStep TagSequences::Connection0CreditStep = SimSequenceStep::NciControlRead(
    L"CORE_CONN_CREDITS_NTF",
    {
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreConnCreditNtfOid,
        {
            1, // Number of entries that follow
                0, // Connection ID
                1, // Number of credits
        }
    }
);

// NFC Controller Interface (NCI), Version 1.1, Section 7.3.1, RF_INTF_ACTIVATED_NTF
const SimSequenceStep TagSequences::Ntag216::ActivatedNotification = SimSequenceStep::NciControlRead(
    L"RF_INTF_ACTIVATED_NTF",
    {
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfIntfActivatedNtfOid,
        {
            1, // Discovery ID
            phNciNfc_e_RfInterfacesFrame_RF, // RF interface
            phNfc_RfProtocolsT2tProtocol, // RF protocol
            phNciNfc_NFCA_Poll, // RF technology and mode for activation
            255, // Max payload data size
            1, // Initial number of credits

            12, // Length of RF specific parameters that follow
                // NFC Controller Interface (NCI), Version 1.1, Section 7.1, Table 59
                0x44, 0x00, // SENS_RES
                7, // NFCID1 length (that follows)
                    0x04, 0xD1, 0x49, 0xD2, 0x9C, 0x39, 0x80, // NFCID1
                1, // SEL_RES response length
                    0x00, // SEL_RES response

            phNciNfc_NFCA_Poll, // RF technology and mode for data exchange
            phNciNfc_e_BitRate106, // Transmit bit rate
            phNciNfc_e_BitRate106, // Receive bit rate
            0, // Length of activation parameters that follow
        }
    }
);

// NTAG213/215/216, Rev. 3.2, Section 10.1, GET_VERSION command, Table 26
const SimSequenceStep TagSequences::Ntag216::GetVersionCommand = SimSequenceStep::NciDataWrite(
    L"[Mifare] GET_VERSION command",
    {
        0, // Connection ID
        {
            0x60, // Command code (GET_VERSION)
        }
    }
);

// NTAG213/215/216, Rev. 3.2, Section 10.1, GET_VERSION response, Table 28
// NFC Controller Interface (NCI), Version 1.1, Section 8.2.1.2, Figure 14
const SimSequenceStep TagSequences::Ntag216::GetVersionResponse = SimSequenceStep::NciDataRead(
    L"[Mifare] GET_VERSION response",
    {
        0, // Connection ID
        {
            0x00, 0x04, 0x04, 0x02, 0x01, 0x00, 0x13, 0x03, // Version information
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// NFC Controller Interface (NCI), Version 1.1, Section 7.3.2, RF_DEACTIVATE_CMD
const SimSequenceStep TagSequences::Ntag216::DeactivateCommand = SimSequenceStep::NciControlWrite(
    L"RF_DEACTIVATE_CMD",
    {
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfDeactivateCmdOid,
        {
            phNciNfc_e_SleepMode, // Deactivation type
        },
    }
);

// NFC Controller Interface (NCI), Version 1.1, Section 7.3.2, RF_DEACTIVATE_RSP
const SimSequenceStep TagSequences::Ntag216::DeactivateResponse = SimSequenceStep::NciControlRead(
    L"RF_DEACTIVATE_RSP",
    {
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfDeactivateCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        },
    }
);

// NFC Controller Interface (NCI), Version 1.1, Section 7.3.2, RF_DEACTIVATE_NTF
const SimSequenceStep TagSequences::Ntag216::DeactivateNotification = SimSequenceStep::NciControlRead(
    L"RF_DEACTIVATE_NTF",
    {
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfDeactivateCmdOid,
        {
            phNciNfc_e_SleepMode, // Deactivation type (sleep mode)
            phNciNfc_e_DhRequest, // Deactivation reason (device host request)
        }
    }
);

// NFC Controller Interface (NCI), Version 1.1, Section 7.3.2, RF_DISCOVER_SELECT_CMD
const SimSequenceStep TagSequences::Ntag216::DiscoverSelectCommand = SimSequenceStep::NciControlWrite(
    L"RF_DISCOVER_SELECT_CMD",
    {
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfDiscSelectCmdOid,
        {
            1, // Discovery Id
            phNfc_RfProtocolsT2tProtocol, // RF protocol
            phNciNfc_e_RfInterfacesFrame_RF, // RF interface
        },
    }
);

// NFC Controller Interface (NCI), Version 1.1, Section 7.3.2, RF_DISCOVER_SELECT_RSP
const SimSequenceStep TagSequences::Ntag216::DiscoverSelectResponse = SimSequenceStep::NciControlRead(
    L"RF_DISCOVER_SELECT_RSP",
    {
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfDiscSelectCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        },
    }
);

// NTAG213/215/216, Rev. 3.2, Section 10.2, Table 29. READ command
const SimSequenceStep TagSequences::Ntag216::ReadPage2Command = SimSequenceStep::NciDataWrite(
    L"[Mifare] READ page 2 command",
    {
        0, // Connection ID
        {
            phNfc_eMifareRead16, // Command code (READ)
            2, // Start page address
        }
    }
);

// NTAG213/215/216, Rev. 3.2, Section 8.5, Fig 7. Memory organization NTAG216
// NFC Controller Interface (NCI), Version 1.1, Section 8.2.1.2, Figure 14
const SimSequenceStep TagSequences::Ntag216::ReadPage2Response = SimSequenceStep::NciDataRead(
    L"[Mifare] READ page 2 response",
    {
        0, // Connection ID
        {
            0x00, // Serial number (last byte)
            0x00, // Internal
            0x00, 0x00, // Lock bytes
            0xE1, 0x10, 0x6D, 0x00, // Capability container (CC)
            0x03, 0x0D, 0xD1, 0x01, 0x09, 0x55, 0x01, 0x62, // User data (first 8 bytes)
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// NTAG213/215/216, Rev. 3.2, Section 10.2, Table 29. READ command
const SimSequenceStep TagSequences::Ntag216::ReadPage4Command = SimSequenceStep::NciDataWrite(
    L"[Mifare] READ page 4 command",
    {
        0, // Connection ID
        {
            phNfc_eMifareRead16, // Command code (READ)
            4, // Start page address
        }
    }
);

// NTAG213/215/216, Rev. 3.2, Section 8.5, Fig 7. Memory organization NTAG216
// NFC Controller Interface (NCI), Version 1.1, Section 8.2.1.2, Figure 14
const SimSequenceStep TagSequences::Ntag216::ReadPage4Response = SimSequenceStep::NciDataRead(
    L"[Mifare] READ page 4 response",
    {
        0, // Connection ID
        {
            0x03, 0x0D, 0xD1, 0x01, 0x09, 0x55, 0x01, 0x62, 0x69, 0x6E, 0x67, 0x2E, 0x63, 0x6F, 0x6D, 0x00, // User data bytes [16,31]
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// NTAG213/215/216, Rev. 3.2, Section 10.2, Table 29. READ command
const SimSequenceStep TagSequences::Ntag216::ReadPage8Command = SimSequenceStep::NciDataWrite(
    L"[Mifare] READ page 8 command",
    {
        0, // Connection ID
        {
            phNfc_eMifareRead16, // Command code (READ)
            8, // Start page address
        }
    }
);

// NTAG213/215/216, Rev. 3.2, Section 8.5, Fig 7. Memory organization NTAG216
// NFC Controller Interface (NCI), Version 1.1, Section 8.2.1.2, Figure 14
const SimSequenceStep TagSequences::Ntag216::ReadPage8Response = SimSequenceStep::NciDataRead(
    L"[Mifare] READ page 8 response",
    {
        0, // Connection ID
        {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // User data bytes [32,47]
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// NFC Controller Interface (NCI), Version 1.1, Section 8.2.1.2, Figure 14
const SimSequenceStep TagSequences::Ntag216::ReadErrorResponse = SimSequenceStep::NciDataRead(
    L"[Mifare] READ error response",
    {
        0, // Connection ID
        {
            PH_NCINFC_STATUS_RF_FRAME_CORRUPTED, // NCI status
        }
    }
);

const SimSequenceStep TagSequences::Ntag216::ActivatedSequence[10] =
{
    // Signal that a tag has arrived.
    ActivatedNotification,

    // Driver checks if the GET_VERSION command works, as a way to distinguish between different types of Type 2 tags.
    GetVersionCommand,
    Connection0CreditStep,
    GetVersionResponse,

    // Driver deactivates tag and reactivates it with the desired protocol.
    DeactivateCommand,
    DeactivateResponse,
    DeactivateNotification,
    DiscoverSelectCommand,
    DiscoverSelectResponse,
    ActivatedNotification,
};

// Sequence when the tag is reset through IOCTL_SMARTCARD_POWER.
const SimSequenceStep TagSequences::Ntag216::ResetSequence[6]
{
    DeactivateCommand,
    DeactivateResponse,
    DeactivateNotification,
    DiscoverSelectCommand,
    DiscoverSelectResponse,
    ActivatedNotification,
};

const SimSequenceStep TagSequences::Ntag216::ReadSequence[18] =
{
    // Driver reads bytes from the tag to verify that the connection is working.
    ReadPage2Command,
    Connection0CreditStep,
    ReadPage2Response,

    // Driver reads capability container (CC) information to check if tag is NDEF compliant.
    ReadPage2Command,
    Connection0CreditStep,
    ReadPage2Response,

    // Driver reads first block of user data to check if it has an NDEF TLV tag.
    ReadPage4Command,
    Connection0CreditStep,
    ReadPage4Response,

    // Driver reads tag's user data.
    ReadPage4Command,
    Connection0CreditStep,
    ReadPage4Response,

    ReadPage4Command,
    Connection0CreditStep,
    ReadPage4Response,

    ReadPage8Command,
    Connection0CreditStep,
    ReadPage8Response,
};

const SimSequenceStep TagSequences::Ntag216::PresenceCheckConnected[2] =
{
    // Driver checks if tag is still connected by issuing a read command.
    // Succeed to indicate that the tag is still present.
    ReadPage2Command,
    ReadPage2Response,
};

const SimSequenceStep TagSequences::Ntag216::PresenceCheckDisconnected[2]
{
    // Driver checks if tag is still connected by issuing a read command.
    // Return an error to indicate that the tag has disappeared.
    ReadPage2Command,
    ReadErrorResponse,
};

const uint8_t TagSequences::Ntag216::Atr[20] =
{
    0x3B, 0x8F, 0x80, 0x01, 0x80, 0x4F, 0x0C, 0xA0, 0x00, 0x00, 0x03, 0x06, 0x03, 0x00, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x56
};
