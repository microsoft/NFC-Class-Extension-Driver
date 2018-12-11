//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <phLibNfc_Internal.h>
#include <phNciNfc.h>
#include <phNciNfc_Core.h>
#include <phNciNfc_CoreStatus.h>
#include <phHciNfc_Core.h>
#include <phHciNfc_Interface.h>

#include "SEInitializationSequences.h"

static constexpr uint8_t EseNfceeId = phHciNfc_e_ProprietaryHostID_Min;
static constexpr uint8_t EseApduPipeId = 0x19;
static constexpr uint8_t HciNetworkConnectionId = 0x03;

// NFC Controller Interface (NCI), Version 1.1, Section 4.4.4, CORE_CONN_CREDITS_NTF
const SimSequenceStep SEInitializationSequences::HciNetworkCredit = SimSequenceStep::NciControlRead(
    L"CORE_CONN_CREDITS_NTF",
    {
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreConnCreditNtfOid,
        {
            1, // Number of entries that follow
                HciNetworkConnectionId, // Connection ID
                1, // Number of credits
        }
    }
);

const SimSequenceStep SEInitializationSequences::Common::PreNfceeDiscovery = SimSequenceStep::SequenceHandler(
    L"SequencePreNfceeDisc",
    SequencePreNfceeDisc,
    STATUS_SUCCESS,
    0
);

// Driver asks NFC Controller to enumerate the NFCEEs.
const SimSequenceStep SEInitializationSequences::Common::NfceeDiscoverCommand_Nci1 = SimSequenceStep::NciControlWrite(
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

// Driver asks NFC Controller to enumerate the NFCEEs.
const SimSequenceStep SEInitializationSequences::Common::NfceeDiscoverCommand_Nci2 = SimSequenceStep::NciControlWrite(
    L"NFCEE_DISCOVER_CMD",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.2, NFCEE_DISCOVER_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtNfceeDiscCmdOid,
    }
);

const SimSequenceStep SEInitializationSequences::Common::NfceeDiscoveryComplete = SimSequenceStep::SequenceHandler(
    L"SequenceNfceeDiscComplete",
    SequenceNfceeDiscComplete,
    STATUS_SUCCESS,
    0
);

// NFC Controller responds that there are 0 NFCEEs.
const SimSequenceStep SEInitializationSequences::NoSEs::NfceeDiscoverResponse = SimSequenceStep::NciControlRead(
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

// NFC Controller responds that there is 1 NFCEE.
const SimSequenceStep SEInitializationSequences::WithEse::NfceeDiscoverResponse = SimSequenceStep::NciControlRead(
    L"NFCEE_DISCOVER_RSP",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.2, NFCEE_DISCOVER_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtNfceeDiscCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
            0x1, // Number of NFCEEs.
        },
    }
);

// NFC Controller enumerates the first NFCEE, which is the HCI Network.
const SimSequenceStep SEInitializationSequences::WithEse::HciNetworkEnumeration = SimSequenceStep::NciControlRead(
    L"NFCEE_DISCOVER_NTF (HCI Network)",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.2, NFCEE_DISCOVER_NTF
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtNfceeDiscNtfOid,
        {
            phHciNfc_e_TerminalHostID, // NFCEE ID
            phNciNfc_NfceeStatus_Enabled, // NFCEE status
            1, // Number of protocol entries (that follow)
                phNciNfc_e_NfceeHciAccessIf, // HCI access protocol
            0, // Number of information TLVs
        },
    }
);

// Driver opens a connection to the HCI Network.
const SimSequenceStep SEInitializationSequences::WithEse::HciNetworkCreateConnectionCommand = SimSequenceStep::NciControlWrite(
    L"CORE_CONN_CREATE_CMD",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.4.2, CORE_CONN_CREATE_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreConnCreateCmdOid,
        {
            phNciNfc_e_NFCEE, // Destination type
            1, // Number of TLV parameters that follow
                0x01, // Type (NFEE Type)
                2, // Length
                // Value:
                    phHciNfc_e_TerminalHostID, // NFCEE ID
                    phNciNfc_e_NfceeHciAccessIf, // NFCEE protocol
        },
    }
);

// NFC Controller responds that the connection to the HCI Network was opened successfully.
const SimSequenceStep SEInitializationSequences::WithEse::HciNetworkCreateConnectionResponse = SimSequenceStep::NciControlRead(
    L"CORE_CONN_CREATE_RSP",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 4.4.2, CORE_CONN_CREATE_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNciCoreGid,
        phNciNfc_e_NciCoreConnCreateCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
            0xFF, // Max data packet payload size
            1, // Initial number of credits
            HciNetworkConnectionId, // Connection ID
        },
    }
);

// NFC Controller notifies that an eSE NFCEE also exists.
const SimSequenceStep SEInitializationSequences::WithEse::EseEnumeration = SimSequenceStep::NciControlRead(
    L"NFCEE_DISCOVER_NTF (eSE)",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.2, NFCEE_DISCOVER_NTF
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtNfceeDiscNtfOid,
        {
            EseNfceeId,
            phNciNfc_NfceeStatus_Enabled,
            1, // Number of protocol entries (that follow)
                0x80, // Proprietary protocol
            0, // Number of information TLVs (that follow)
        },
    }
);

// NFC Controller notifies that eSE supports NFC-A card emulation.
const SimSequenceStep SEInitializationSequences::WithEse::EseSupportsNfcA = SimSequenceStep::NciControlRead(
    L"RF_NFCEE_DISCOVERY_REQ_NTF (eSE, NFC-A)",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 7.4, RF_NFCEE_DISCOVERY_REQ_NTF
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfNfceeDiscoveryReqNtfOid,
        {
            1, // Number of TLV information entries (that follow)
                phNciNfc_e_RfNfceeDiscReqAdd, // Type (add information)
                3, // Value length
                    // Value:
                    EseNfceeId, // NFCEE Id
                    phNciNfc_NFCA_Listen, // RF technology/mode
                    phNciNfc_e_RfProtocolsIsoDepProtocol, // RF protocol
        },
    }
);

// NFC Controller notifies that eSE supports NFC-B card emulation.
const SimSequenceStep SEInitializationSequences::WithEse::EseSupportsNfcB = SimSequenceStep::NciControlRead(
    L"RF_NFCEE_DISCOVERY_REQ_NTF (eSE, NFC-B)",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 7.4, RF_NFCEE_DISCOVERY_REQ_NTF
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfNfceeDiscoveryReqNtfOid,
        {
            1, // Number of TLV information entries (that follow)
                phNciNfc_e_RfNfceeDiscReqAdd, // Type (add information)
                3, // Value length
                    // Value:
                    EseNfceeId, // NFCEE Id
                    phNciNfc_NFCB_Listen, // RF technology/mode
                    phNciNfc_e_RfProtocolsIsoDepProtocol, // RF protocol
        },
    }
);

// NFC Controller notifies that the eSE supports NFC-F (Type 3 Tag) card emulation.
const SimSequenceStep SEInitializationSequences::WithEse::EseSupportsNfcF = SimSequenceStep::NciControlRead(
    L"RF_NFCEE_DISCOVERY_REQ_NTF (eSE, NFC-F)",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 7.4, RF_NFCEE_DISCOVERY_REQ_NTF
        phNciNfc_e_NciCoreMsgTypeCntrlNtf,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfNfceeDiscoveryReqNtfOid,
        {
            1, // Number of TLV information entries (that follow)
                phNciNfc_e_RfNfceeDiscReqAdd, // Type (add information)
                3, // Value length
                    // Value:
                    EseNfceeId, // NFCEE Id
                    phNciNfc_NFCF_Listen, // RF technology/mode
                    phNciNfc_e_RfProtocolsT3tProtocol, // RF protocol
        },
    }
);

// Driver opens the HCI admin pipe so that it can communicate with the HCI host controller (which is implemented by the NFC Controller).
const SimSequenceStep SEInitializationSequences::WithEse::OpenAdminPipeCommand = SimSequenceStep::HciWrite(
    L"[HCI] ANY_OPEN_PIPE",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.3, ANY_OPEN_PIPE
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeCommand, // Message type
        phHciNfc_e_AnyOpenPipe, // Command ID
        {
        }
    }
);

// HCI host controller (aka, NFC Controller) responds with ANY_OK.
const SimSequenceStep SEInitializationSequences::WithEse::OpenAdminPipeResponse = SimSequenceStep::HciRead(
    L"[HCI] ANY_OPEN_PIPE (response)",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.3, ANY_OPEN_PIPE
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeResponse, // Message type
        phHciNfc_e_RspAnyOk, // Response code
        {
        }
    }
);

// Driver tells the HCI host controller (aka, NFC controller) to permit communication between the driver and the eSE.
const SimSequenceStep SEInitializationSequences::WithEse::SetWhitelistCommand = SimSequenceStep::HciWrite(
    L"[HCI] ANY_SET_PARAMETER: WHITELIST",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.1, ANY_SET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeCommand, // Message type
        phHciNfc_e_AnySetParameter, // Command code
        {
            // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 7.1.1.1, Table 20, WHITELIST
            phHciNfc_e_WhitelistRegistryId, // Parameter ID
            EseNfceeId,
        }
    }
);

// HCI host controller (aka, NFC controller) responds with ANY_OK.
const SimSequenceStep SEInitializationSequences::WithEse::SetWhitelistResponse = SimSequenceStep::HciRead(
    L"[HCI] ANY_SET_PARAMETER: WHITELIST (response)",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.1, ANY_SET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeResponse, // Message type
        phHciNfc_e_RspAnyOk, // Response code
        {
        }
    }
);

// Driver tells the HCI host controller (aka, NFC controller) that the driver is a HCI terminal host.
const SimSequenceStep SEInitializationSequences::WithEse::SetHostTypeCommand = SimSequenceStep::HciWrite(
    L"[HCI] ANY_SET_PARAMETER: HOST_TYPE",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.1, ANY_SET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeCommand, // Message type
        phHciNfc_e_AnySetParameter, // Command code
        {
            // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 7.1.1.1, Table 20, HOST_TYPE
            phHciNfc_e_HostTypeRegistryId, // Parameter ID
            phHciNfc_e_HostType_Terminal >> 8, // Host type family
            phHciNfc_e_HostType_Terminal & 0xFF, // Host type
        }
    }
);

// HCI host controller (aka, NFC controller) responds with ANY_OK.
const SimSequenceStep SEInitializationSequences::WithEse::SetHostTypeResponse = SimSequenceStep::HciRead(
    L"[HCI] ANY_SET_PARAMETER: HOST_TYPE (response)",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.1, ANY_SET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeResponse, // Message type
        phHciNfc_e_RspAnyOk, // Response code
        {
        }
    }
);

// Driver requests the session ID from the HCI host controller (aka, NFC controller).
// This is used to check if any hardware configuration has changed (e.g. an eSE firmware update occured, a SIM card was
// swapped, etc.), as this may require the HCI pipes to be re-initialized.
//
// The driver is free to use the session ID for whatever it wants. But NfcCx uses it to store information relating to
// the open pipes.
//
// See, ETSI Host Controller Interface (HCI), Version 12.1.0, Section 8.4, Session initialization
const SimSequenceStep SEInitializationSequences::WithEse::GetSessionIdCommand = SimSequenceStep::HciWrite(
    L"[HCI] ANY_SET_PARAMETER: SESSION_IDENTITY",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.2, ANY_GET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeCommand, // Message type
        phHciNfc_e_AnyGetParameter, // Command code
        {
            // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 7.1.1.1, Table 20, SESSION_IDENTITY
            phHciNfc_e_SessionIdentityRegistryId, // Parameter ID
        }
    }
);

// HCI host controller (aka, NFC controller) returns the session ID.
const SimSequenceStep SEInitializationSequences::WithEse::GetSessionIdResponse = SimSequenceStep::HciRead(
    L"[HCI] ANY_SET_PARAMETER: SESSION_IDENTITY (response)",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.2, ANY_GET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeResponse, // Message type
        phHciNfc_e_RspAnyOk, // Response code
        {
            // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 7.1.1.1, Table 20, SESSION_IDENTITY
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, EseApduPipeId, 0x7F
        }
    }
);

// Driver requests the host list from the HCI host controller (aka, NFC controller).
const SimSequenceStep SEInitializationSequences::WithEse::GetHostListCommand = SimSequenceStep::HciWrite(
    L"[HCI] ANY_GET_PARAMETER: HOST_LIST",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.2, ANY_GET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeCommand, // Message type
        phHciNfc_e_AnyGetParameter, // Command code
        {
            // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 7.1.1.1, Table 20, HOST_LIST
            phHciNfc_e_HostListRegistryId, // Parameter ID
        }
    }
);

// HCI host controller (aka, NFC controller) returns host list.
const SimSequenceStep SEInitializationSequences::WithEse::GetHostListResponse = SimSequenceStep::HciRead(
    L"[HCI] ANY_GET_PARAMETER: HOST_LIST (response)",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.2, ANY_GET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeResponse, // Message type
        phHciNfc_e_RspAnyOk, // Response code
        {
            // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 7.1.1.1, Table 20, HOST_LIST
            phHciNfc_e_HostControllerID,
            EseNfceeId,
        }
    }
);

// Driver requests the host type list from the HCI host controller (aka, NFC controller).
const SimSequenceStep SEInitializationSequences::WithEse::GetHostTypeListCommand = SimSequenceStep::HciWrite(
    L"[HCI] ANY_GET_PARAMETER: HOST_TYPE_LIST",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.2, ANY_GET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeCommand, // Message type
        phHciNfc_e_AnyGetParameter, // Command code
        {
            // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 7.1.1.1, Table 20, HOST_TYPE_LIST
            phHciNfc_e_HostTypeListRegistryId, // Parameter ID
        }
    }
);

// HCI host controller (aka, NFC controller) returns the host type list.
const SimSequenceStep SEInitializationSequences::WithEse::GetHostTypeListResponse = SimSequenceStep::HciRead(
    L"[HCI] ANY_GET_PARAMETER: HOST_TYPE_LIST (response)",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 6.1.2.2, ANY_GET_PARAMETER
        HciNetworkConnectionId, // Connection ID
        phHciNfc_e_HciAdminPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeResponse, // Message type
        phHciNfc_e_RspAnyOk, // Response code
        {
            // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 7.1.1.1, Table 20, HOST_TYPE_LIST

            // Host controller
            phHciNfc_e_HostType_HostController >> 8, // Host type family
            phHciNfc_e_HostType_HostController & 0xFF, // Host type

            // Terminal (aka, Host Device. aka, Driver)
            phHciNfc_e_HostType_Terminal >> 8, // Host type family
            phHciNfc_e_HostType_Terminal & 0xFF, // Host type

            // eSE
            phHciNfc_e_HostType_eSE >> 8, // Host type family
            phHciNfc_e_HostType_eSE & 0xFF, // Host type
        }
    }
);

// Driver tells NFC Controller to enable the eSE.
const SimSequenceStep SEInitializationSequences::WithEse::EseEnableCommand = SimSequenceStep::NciControlWrite(
    L"NFCEE_MODE_SET_CMD: ENABLE",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.3, NFCEE_MODE_SET_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtModeSetCmdOid,
        {
            EseNfceeId, // NFCEE ID
            PH_NCINFC_EXT_NFCEEMODE_ENABLE, // NFCEE mode (enable)
        },
    }
);

// NFC Controller responds that the eSE was enabled successfully.
const SimSequenceStep SEInitializationSequences::WithEse::EseEnableResponse = SimSequenceStep::NciControlRead(
    L"NFCEE_MODE_SET_RSP: ENABLE",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.3, NFCEE_MODE_SET_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtModeSetCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        },
    }
);

// Driver tells NFC Controller to disable the eSE.
const SimSequenceStep SEInitializationSequences::WithEse::EseDisableCommand = SimSequenceStep::NciControlWrite(
    L"NFCEE_MODE_SET_CMD: DISABLE",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.3, NFCEE_MODE_SET_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtModeSetCmdOid,
        {
            EseNfceeId, // NFCEE ID
            PH_NCINFC_EXT_NFCEEMODE_DISABLE, // NFCEE mode (enable)
        },
    }
);

// NFC Controller responds that the eSE was disabled successfully.
const SimSequenceStep SEInitializationSequences::WithEse::EseDisableResponse = SimSequenceStep::NciControlRead(
    L"NFCEE_MODE_SET_RSP: DISABLE",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.3, NFCEE_MODE_SET_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtModeSetCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        },
    }
);

// Driver tells NFC Controller to power on the eSE.
const SimSequenceStep SEInitializationSequences::WithEse::EsePowerOnCommand = SimSequenceStep::NciControlWrite(
    L"NFCEE_POWER_AND_LINK_CNTRL_CMD (eSE): Power always on",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.3, NFCEE_POWER_AND_LINK_CNTRL_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtPowerAndLinkCtrlCmdOid,
        {
            EseNfceeId, // NFCEE ID
            phNciNfc_PLM_PowerSupplyAlwaysOn, // NFCEE power mode (power always on)
        },
    }
);

// Success.
const SimSequenceStep SEInitializationSequences::WithEse::EsePowerOnResponse = SimSequenceStep::NciControlRead(
    L"NFCEE_POWER_AND_LINK_CNTRL_RSP (eSE): Power always on",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.3, NFCEE_POWER_AND_LINK_CNTRL_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtPowerAndLinkCtrlCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        },
    }
);

// Driver tells NFC Controller to power on the eSE and to keep the communication link open.
const SimSequenceStep SEInitializationSequences::WithEse::EsePowerAndLinkOnCommand = SimSequenceStep::NciControlWrite(
    L"NFCEE_POWER_AND_LINK_CNTRL_CMD (eSE): Power always on",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.3, NFCEE_POWER_AND_LINK_CNTRL_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtPowerAndLinkCtrlCmdOid,
        {
            EseNfceeId, // NFCEE ID
            phNciNfc_PLM_PowerNfccLinkAlwaysOn, // NFCEE power mode (power and link always on)
        },
    }
);

// Success.
const SimSequenceStep SEInitializationSequences::WithEse::EsePowerAndLinkOnResponse = SimSequenceStep::NciControlRead(
    L"NFCEE_POWER_AND_LINK_CNTRL_RSP (eSE): Power always on",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 9.3, NFCEE_POWER_AND_LINK_CNTRL_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtPowerAndLinkCtrlCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        },
    }
);

// Driver tells NFC Controller that it can power off the eSE.
const SimSequenceStep SEInitializationSequences::WithEse::EsePowerOffCommand = SimSequenceStep::NciControlWrite(
    L"NFCEE_POWER_AND_LINK_CNTRL_CMD (eSE): NFCC decides",
    {
        // NFC Controller Interface (NCI), Version 2.0, Section 10.6, NFCEE_POWER_AND_LINK_CNTRL_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtPowerAndLinkCtrlCmdOid,
        {
            EseNfceeId, // NFCEE ID
            phNciNfc_PLM_NfccDecides, // NFCEE power mode (NFCC decides)
        },
    }
);

// Success.
const SimSequenceStep SEInitializationSequences::WithEse::EsePowerOffResponse = SimSequenceStep::NciControlRead(
    L"NFCEE_POWER_AND_LINK_CNTRL_RSP (eSE): NFCC decides",
    {
        // NFC Controller Interface (NCI), Version 2.0, Section 10.6, NFCEE_POWER_AND_LINK_CNTRL_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreNfceeMgtGid,
        phNciNfc_e_NfceeMgtPowerAndLinkCtrlCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        },
    }
);

// Driver sets the default routing table.
const SimSequenceStep SEInitializationSequences::WithEse::SetDefaultRoutingTableCommand = SimSequenceStep::NciControlWrite(
    L"RF_SET_LISTEN_MODE_ROUTING_CMD (default routing table)",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 6.3.2, NFCEE_POWER_AND_LINK_CNTRL_CMD
        phNciNfc_e_NciCoreMsgTypeCntrlCmd,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfSetRoutingCmdOid,
        {
            0, // More to come? (last message)
            2, // Number of routing entries (that follow)
                phNciNfc_e_LstnModeRtngProtocolBased, // Type
                3, // Length of value
                    phHciNfc_e_HostControllerID, // NFEE ID
                    0x00, // Power state
                    phNfc_RfProtocolsIsoDepProtocol, // Protocol

                phNciNfc_e_LstnModeRtngProtocolBased, // Type
                3, // Length of value
                    phHciNfc_e_HostControllerID, // NFCEE ID
                    0x00, // Power state
                    phNfc_RfProtocolsNfcDepProtocol, // Protocol
        },
    }
);

// Success.
const SimSequenceStep SEInitializationSequences::WithEse::SetDefaultRoutingTableResponse = SimSequenceStep::NciControlRead(
    L"RF_SET_LISTEN_MODE_ROUTING_RSP (default routing table)",
    {
        // NFC Controller Interface (NCI), Version 1.1, Section 6.3.2, RF_SET_LISTEN_MODE_ROUTING_RSP
        phNciNfc_e_NciCoreMsgTypeCntrlRsp,
        phNciNfc_e_CoreRfMgtGid,
        phNciNfc_e_RfMgtRfSetRoutingCmdOid,
        {
            PH_NCINFC_STATUS_OK, // Status
        },
    }
);

// Driver resets the eSE.
const SimSequenceStep SEInitializationSequences::WithEse::EseResetCommand = SimSequenceStep::HciWrite(
    L"[HCI] EVT_ABORT (eSE)",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 12.2.2.2, EVT_ABORT
        HciNetworkConnectionId, // Connection ID
        EseApduPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeEvent, // Message type
        PHHCINFC_EVT_ABORT, // Event ID
        {
        }
    }
);

// eSE provides the ATR (e.g. because the eSE was reset with EVT_ABORT).
const SimSequenceStep SEInitializationSequences::WithEse::EseAtrEvent = SimSequenceStep::HciRead(
    L"[HCI] EVT_ATR (eSE)",
    {
        // ETSI Host Controller Interface (HCI), Version 12.1.0, Section 12.3.2.3, EVT_ATR
        HciNetworkConnectionId, // Connection ID
        EseApduPipeId, // Pipe ID
        phHciNfc_e_HcpMsgTypeEvent, // Message type
        PHHCINFC_EVT_ATR, // Event ID
        {
            // ATR
            0x3B, 0x8F, 0x80, 0x01, 0x4D, 0x53, 0x46, 0x54, 0x20, 0x56, 0x30, 0x2E, 0x30, 0x2E, 0x30, 0x20, 0x20, 0x20, 0x20, 0x44
        }
    }
);

const SimSequenceStep
SEInitializationSequences::Common::InitializeStartSequence_Nci1[2] =
{
    PreNfceeDiscovery,
    NfceeDiscoverCommand_Nci1,
};

const SimSequenceStep
SEInitializationSequences::Common::InitializeStartSequence_Nci2[2] =
{
    PreNfceeDiscovery,
    NfceeDiscoverCommand_Nci2,
};

const SimSequenceView
SEInitializationSequences::Common::InitializeStartSequence(bool isNci2)
{
    if (isNci2)
    {
        return InitializeStartSequence_Nci2;
    }

    return InitializeStartSequence_Nci1;
}

const SimSequenceStep
SEInitializationSequences::Common::InitializeEndSequence[1] =
{
    NfceeDiscoveryComplete,
};

const SimSequenceView
SEInitializationSequences::NoSEs::InitializeSequence_Nci1[3] =
{
    Common::InitializeStartSequence_Nci1,
    NfceeDiscoverResponse,
    Common::InitializeEndSequence,
};

const SimSequenceView
SEInitializationSequences::NoSEs::InitializeSequence_Nci2[3] =
{
    Common::InitializeStartSequence_Nci2,
    NfceeDiscoverResponse,
    Common::InitializeEndSequence,
};

const SimSequenceView
SEInitializationSequences::NoSEs::InitializeSequence(bool isNci2)
{
    if (isNci2)
    {
        return InitializeSequence_Nci2;
    }

    return InitializeSequence_Nci1;
}

const SimSequenceView
SEInitializationSequences::WithEse::InitializeSequence_Nci1[32] =
{
    Common::InitializeStartSequence_Nci1,

    NfceeDiscoverResponse,

    HciNetworkEnumeration,
    HciNetworkCreateConnectionCommand,
    HciNetworkCreateConnectionResponse,

    EseEnumeration,
    EseSupportsNfcA,
    EseSupportsNfcB,
    EseSupportsNfcF,

    OpenAdminPipeCommand,
    HciNetworkCredit,
    OpenAdminPipeResponse,

    SetWhitelistCommand,
    HciNetworkCredit,
    SetWhitelistResponse,

    SetHostTypeCommand,
    HciNetworkCredit,
    SetHostTypeResponse,

    GetSessionIdCommand,
    HciNetworkCredit,
    GetSessionIdResponse,

    EseEnableCommand,
    EseEnableResponse,

    GetHostListCommand,
    HciNetworkCredit,
    GetHostListResponse,

    GetHostTypeListCommand,
    HciNetworkCredit,
    GetHostTypeListResponse,

    Common::InitializeEndSequence,

    // Driver disables all the SEs to save power, until there is a client handle that requires them.
    EseDisableCommand,
    EseDisableResponse,
};

const SimSequenceView
SEInitializationSequences::WithEse::ClientConnectedSequence_Nci1[4] =
{
    EsePowerAndLinkOnCommand,
    EsePowerAndLinkOnResponse,

    EseEnableCommand,
    EseEnableResponse,
};

const SimSequenceView
SEInitializationSequences::WithEse::GetAtrSequence_Nci1[2] =
{
    EseResetCommand,
    EseAtrEvent,
};

const SimSequenceView
SEInitializationSequences::WithEse::ClientDisconnectedSequence_Nci1[4] =
{
    EsePowerOffCommand,
    EsePowerOffResponse,

    EseDisableCommand,
    EseDisableResponse,
};
