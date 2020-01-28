//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <phNciNfc.h>
#include <phNciNfc_Core.h>
#include <phNciNfc_CoreStatus.h>

#include "TagSequences.h"
#include "Type5TagSequences.h"

// NFC Controller Interface (NCI), Version 1.1, Section 7.3.1, RF_INTF_ACTIVATED_NTF
const SimSequenceStep Type5TagSequences::LRI2K::ActivatedNotification = SimSequenceStep::NciControlRead(
    L"RF_INTF_ACTIVATED_NTF",
    {
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfIntfActivatedNtfOid,
        {
            1, // Discovery ID
            phNciNfc_e_RfInterfacesFrame_RF, // RF interface
            phNciNfc_e_RfProtocols15693Protocol, // RF protocol
            phNciNfc_NFCISO15693_Poll, // RF technology and mode for activation
            0xFF, // Max payload data size
            1, // Initial number of credits

            10, // Length of RF specific parameters that follow
                // NFC Controller Interface (NCI), Version 2.0, Section 7.1, Table 74, Specific Parameters for NFC-V Poll Mode
                0x00, // RES_FLAG
                0x00, // DSFID
                0x66, 0x69, 0x43, 0x1B, 0xC3, 0x20, 0x02, 0xE0, // UID

            phNciNfc_NFCISO15693_Poll, // RF technology and mode for data exchange
            0x80, // Transmit bit rate (proprietary)
            0x80, // Receive bit rate (proprietary)
            0, // Length of activation parameters that follow
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.13, Get System Info
const SimSequenceStep Type5TagSequences::LRI2K::GetSystemInfoCommand = SimSequenceStep::NciDataWrite(
    L"[T5T] Get System Info command",
    {
        0, // Connection ID
        {
            0x02, // Request flags (High data rate)
            0x2B, // Command (Get System Info)
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.13, Get System Info
// NFC Controller Interface (NCI), Version 2.0, Section 8.2.1.1, Figure 18, Format for Frame RF Interface (NFC-V) for Reception
const SimSequenceStep Type5TagSequences::LRI2K::GetSystemInfoResponse = SimSequenceStep::NciDataRead(
    L"[T5T] Get System Info response",
    {
        0, // Connection ID
        {
            0x00, // Response flags
            0x0F, // Information flags
            0x66, 0x69, 0x43, 0x1B, 0xC3, 0x20, 0x02, 0xE0, // UID
            0x00, // DSFID
            0x00, // AFI
            0x3F, 0x03, // Memory size (64 blocks of 4 bytes)
            0x20, // IC Reference
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock0Command = SimSequenceStep::NciDataWrite(
    L"[T5T] Read Single Block 0 command",
    {
        0, // Connection ID
        {
            0x22, // Request flags (High data rate, Address)
            0x20, // Command (Read Single Block)
            0x66, 0x69, 0x43, 0x1B, 0xC3, 0x20, 0x02, 0xE0, // UID
            0x00, // Block number
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
// NFC Controller Interface (NCI), Version 2.0, Section 8.2.1.1, Figure 18, Format for Frame RF Interface (NFC-V) for Reception
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock0Response = SimSequenceStep::NciDataRead(
    L"[T5T] Read Single Block 0 response",
    {
        0, // Connection ID
        {
            0x00, // Response flags
            0xE1, 0x40, 0x20, 0x00, // Data
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock1Command = SimSequenceStep::NciDataWrite(
    L"[T5T] Read Single Block 1 command",
    {
        0, // Connection ID
        {
            0x22, // Request flags (High data rate, Address)
            0x20, // Command (Read Single Block)
            0x66, 0x69, 0x43, 0x1B, 0xC3, 0x20, 0x02, 0xE0, // UID
            0x01, // Block number
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
// NFC Controller Interface (NCI), Version 2.0, Section 8.2.1.1, Figure 18, Format for Frame RF Interface (NFC-V) for Reception
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock1Response = SimSequenceStep::NciDataRead(
    L"[T5T] Read Single Block 1 response",
    {
        0, // Connection ID
        {
            0x00, // Response flags
            0x03, 0x0D, 0xD1, 0x01, // Data
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock2Command = SimSequenceStep::NciDataWrite(
    L"[T5T] Read Single Block 2 command",
    {
        0, // Connection ID
        {
            0x22, // Request flags (High data rate, Address)
            0x20, // Command (Read Single Block)
            0x66, 0x69, 0x43, 0x1B, 0xC3, 0x20, 0x02, 0xE0, // UID
            0x02, // Block number
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
// NFC Controller Interface (NCI), Version 2.0, Section 8.2.1.1, Figure 18, Format for Frame RF Interface (NFC-V) for Reception
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock2Response = SimSequenceStep::NciDataRead(
    L"[T5T] Read Single Block 2 response",
    {
        0, // Connection ID
        {
            0x00, // Response flags
            0x09, 0x55, 0x01, 0x62, // Data
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock3Command = SimSequenceStep::NciDataWrite(
    L"[T5T] Read Single Block 3 command",
    {
        0, // Connection ID
        {
            0x22, // Request flags (High data rate, Address)
            0x20, // Command (Read Single Block)
            0x66, 0x69, 0x43, 0x1B, 0xC3, 0x20, 0x02, 0xE0, // UID
            0x03, // Block number
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
// NFC Controller Interface (NCI), Version 2.0, Section 8.2.1.1, Figure 18, Format for Frame RF Interface (NFC-V) for Reception
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock3Response = SimSequenceStep::NciDataRead(
    L"[T5T] Read Single Block 3 response",
    {
        0, // Connection ID
        {
            0x00, // Response flags
            0x69, 0x6E, 0x67, 0x2E, // Data
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock4Command = SimSequenceStep::NciDataWrite(
    L"[T5T] Read Single Block 4 command",
    {
        0, // Connection ID
        {
            0x22, // Request flags (High data rate, Address)
            0x20, // Command (Read Single Block)
            0x66, 0x69, 0x43, 0x1B, 0xC3, 0x20, 0x02, 0xE0, // UID
            0x04, // Block number
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
// NFC Controller Interface (NCI), Version 2.0, Section 8.2.1.1, Figure 18, Format for Frame RF Interface (NFC-V) for Reception
const SimSequenceStep Type5TagSequences::LRI2K::ReadBlock4Response = SimSequenceStep::NciDataRead(
    L"[T5T] Read Single Block 4 response",
    {
        0, // Connection ID
        {
            0x00, // Response flags
            0x63, 0x6F, 0x6D, 0x00, // Data
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
const SimSequenceStep Type5TagSequences::LRI2K::PresenceCheckCommand = SimSequenceStep::NciDataWrite(
    L"[T5T] Read Single Block 0 command: Presence Check",
    {
        0, // Connection ID
        {
            0x02, // Request flags (High data rate)
            0x20, // Command (Read Single Block)
            0x00, // Block number
        }
    }
);

// STMicroelectronics LRI2K Datasheet, Section 20.3, Read Single Block
// NFC Controller Interface (NCI), Version 2.0, Section 8.2.1.1, Figure 18, Format for Frame RF Interface (NFC-V) for Reception
const SimSequenceStep Type5TagSequences::LRI2K::PresenceCheckConnectedResponse = SimSequenceStep::NciDataRead(
    L"[T5T] Read Single Block 0 response: Presence Check",
    {
        0, // Connection ID
        {
            0x00, // Response flags
            0xE1, 0x40, 0x20, 0x00, // Data
            PH_NCINFC_STATUS_OK, // NCI status
        }
    }
);

const SimSequenceStep Type5TagSequences::LRI2K::ActivatedSequence[4] =
{
    ActivatedNotification,

    GetSystemInfoCommand,
    TagSequences::Connection0CreditStep,
    GetSystemInfoResponse,
};

const SimSequenceStep Type5TagSequences::LRI2K::ReadSequence[21] =
{
    // Driver reads bytes from the tag to verify that the connection is working.
    ReadBlock0Command,
    TagSequences::Connection0CreditStep,
    ReadBlock0Response,

    // Driver reads first 8 bytes to check if the tag is NDEF compliant.
    ReadBlock0Command,
    TagSequences::Connection0CreditStep,
    ReadBlock0Response,

    ReadBlock1Command,
    TagSequences::Connection0CreditStep,
    ReadBlock1Response,

    // Driver reads the tag's data.
    ReadBlock1Command,
    TagSequences::Connection0CreditStep,
    ReadBlock1Response,

    ReadBlock2Command,
    TagSequences::Connection0CreditStep,
    ReadBlock2Response,

    ReadBlock3Command,
    TagSequences::Connection0CreditStep,
    ReadBlock3Response,

    ReadBlock4Command,
    TagSequences::Connection0CreditStep,
    ReadBlock4Response,
};

const SimSequenceStep Type5TagSequences::LRI2K::PresenceCheckConnectedSequence[3] =
{
    // Presence check: report tag still exists.
    PresenceCheckCommand,
    TagSequences::Connection0CreditStep,
    PresenceCheckConnectedResponse,
};

const SimSequenceStep Type5TagSequences::LRI2K::PresenceCheckDisconnectedSequence[2] =
{
    // Presence check: report tag has disappeared by not providing a response resulting in a timeout.
    PresenceCheckCommand,
    TagSequences::Connection0CreditStep,
};
