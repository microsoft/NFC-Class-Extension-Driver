/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h"

#include "phNciNfc_DbgDescription.tmh"

#define PHNCINFC_VALIDATE_PACKET_LENGTH(Expected, Actual) \
    if ((Expected) > (Actual)) { return; } \

static uint8_t NciVer = 0;

void phNciNfc_PrintCoreResetCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Reset Type: %!NCI_RESET_TYPE!", pBuff[0]);
}

void phNciNfc_PrintCoreInitCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    UNUSED(pBuff);
    PHNCINFC_VALIDATE_PACKET_LENGTH(0, wLen);
}

void phNciNfc_PrintCoreSetConfigCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint8_t bSize, bNumParams;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bNumParams = pBuff[bIndex++];
    PH_LOG_NCI_INFO_X32MSG("Number of Parameters:", (uint32_t)bNumParams);

    for (bCount = 0; bCount < bNumParams; bCount++) {
        bSize = (PHNCINFC_TLVUTIL_NXP_PROP_ID1 == pBuff[bIndex]) ?
                            (pBuff[bIndex+2] + 3) : (pBuff[bIndex+1] + 2);
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+bSize, wLen);

        if (pBuff[bIndex] < PHNCINFC_TLVUTIL_NXP_PROP_ID1) {
            PH_LOG_NCI_INFO_STR("ID: %!NCI_CONFIG_PARAM_ID!", pBuff[bIndex]);
            PH_LOG_NCI_INFO_X32MSG("Length:", (uint32_t)pBuff[bIndex+1]);

            if (pBuff[bIndex+1] == 1) {
                PH_LOG_NCI_INFO_X32MSG("Value:", (uint32_t)pBuff[bIndex+2]);
            }
            else if (pBuff[bIndex+1] > 1) {
                PH_LOG_NCI_INFO_HEXDUMP("Value: %!HEXDUMP!",
                                        WppLogHex((void*)&pBuff[bIndex+2], (uint16_t)pBuff[bIndex+1]));
            }
        }
        else if (pBuff[bIndex] == PHNCINFC_TLVUTIL_NXP_PROP_ID1) {
            PH_LOG_NCI_INFO_STR("ID: %!NXP_CONFIG_PARAM_ID!", pBuff[bIndex+1]);
            PH_LOG_NCI_INFO_X32MSG("Length:", (uint32_t)pBuff[bIndex+2]);

            if (pBuff[bIndex+2] == 1) {
                PH_LOG_NCI_INFO_X32MSG("Value:", (uint32_t)pBuff[bIndex+3]);
            }
            else if (pBuff[bIndex+2] > 1) {
                PH_LOG_NCI_INFO_HEXDUMP("Value: %!HEXDUMP!",
                                        WppLogHex((void*)&pBuff[bIndex+3], (uint16_t)pBuff[bIndex+2]));
            }
        }

        bIndex += bSize;
    }
}

void phNciNfc_PrintCoreGetConfigCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint8_t bSize, bNumParams;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);

    bNumParams = pBuff[bIndex++];
    PH_LOG_NCI_INFO_X32MSG("Number of Parameters", (uint32_t)bNumParams);

    for (bCount = 0; bCount < bNumParams; bCount++) {
        bSize = (PHNCINFC_TLVUTIL_NXP_PROP_ID1 == pBuff[bIndex]) ? 2 : 1;
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+bSize, wLen);

        if (pBuff[bIndex] < PHNCINFC_TLVUTIL_NXP_PROP_ID1) {
            PH_LOG_NCI_INFO_STR("ID: %!NCI_CONFIG_PARAM_ID!", pBuff[bIndex]);
        }
        else if (pBuff[bIndex] == PHNCINFC_TLVUTIL_NXP_PROP_ID1) {
            PH_LOG_NCI_INFO_STR("ID: %!NXP_CONFIG_PARAM_ID!", pBuff[bIndex+1]);
        }

        bIndex += bSize;
    }
}

void phNciNfc_PrintCoreConnCreateCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint8_t bSize, bNumParams;
    phNciNfc_DestType_t eDestinationType;

    PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
    eDestinationType = (phNciNfc_DestType_t)pBuff[bIndex++];
    bNumParams = pBuff[bIndex++];

    PH_LOG_NCI_INFO_STR("Destination Type: %!NCI_CONN_DEST_TYPE!", eDestinationType);
    PH_LOG_NCI_INFO_X32MSG("Number of Destination-Specific Parameters:", (uint32_t)bNumParams);

    for (bCount = 0; bCount < bNumParams; bCount++) {
        bSize = (1 + pBuff[bIndex+1]);
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+bSize, wLen);

        PH_LOG_NCI_INFO_X32MSG("Type:", (uint32_t)pBuff[bIndex]);
        PH_LOG_NCI_INFO_X32MSG("Length:", (uint32_t)pBuff[bIndex+1]);

        if (pBuff[bIndex] == 0x00) {
            PH_LOG_NCI_INFO_X32MSG("RF Discovery ID:", (uint32_t)pBuff[bIndex+2]);
            PH_LOG_NCI_INFO_STR("RF Protocol: %!NCI_RF_PROTOCOL!", pBuff[bIndex+3]);
        }
        else if (pBuff[bIndex] == 0x01) {
            PH_LOG_NCI_INFO_X32MSG("NFCEE ID:", (uint32_t)pBuff[bIndex+2]);
            PH_LOG_NCI_INFO_STR("NFCEE Protocol: %!NCI_NFCEE_INTERFACE!", pBuff[bIndex+3]);
        }

        bIndex += bSize;
    }
}

void phNciNfc_PrintCoreConnCloseCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_X32MSG("Conn ID:", (uint32_t)pBuff[0]);
}

void phNciNfc_PrintRfDiscoverMapCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0, bNumConfigs;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bNumConfigs = pBuff[bIndex++];
    PH_LOG_NCI_INFO_X32MSG("Number of Mapping Configurations:", (uint32_t)bNumConfigs);

    for (bCount = 0; bCount < bNumConfigs; bCount++, bIndex += 3) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+3, wLen);
        PH_LOG_NCI_INFO_STR("RF Protocol: %!NCI_RF_PROTOCOL!", pBuff[bIndex]);

        if (pBuff[bIndex+1] & 0x1) {
            PH_LOG_NCI_INFO_STR("RF Interface is mapped to the RF Protocol in Poll Mode");
        }

        if (pBuff[bIndex+1] & 0x2) {
            PH_LOG_NCI_INFO_STR("RF Interface is mapped to the RF Protocol in Listen Mode");
        }

        PH_LOG_NCI_INFO_STR("RF Interface: %!NCI_RF_INTERFACE!", pBuff[bIndex+2]);
    }
}

void phNciNfc_PrintRfSetListenModeRoutingCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint8_t bSize, bMore, bNumEntries;

    PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);

    bMore = pBuff[bIndex++];
    bNumEntries = pBuff[bIndex++];

    PH_LOG_NCI_INFO_X32MSG("More:", (uint32_t)bMore);
    PH_LOG_NCI_INFO_X32MSG("Number of Routing Entries:", (uint32_t)bNumEntries);

    for (bCount = 0; bCount < bNumEntries; bCount++) {
        bSize = (2 + pBuff[bIndex+1]);
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+bSize, wLen);

        PH_LOG_NCI_INFO_STR("Type: %!NCI_LISTEN_MODE_RTNG_TYPE!", pBuff[bIndex]);
        PH_LOG_NCI_INFO_X32MSG("Length:", (uint32_t)pBuff[bIndex+1]);

        switch (pBuff[bIndex])
        {
        case 0x00:
            PH_LOG_NCI_INFO_X32MSG("Route:", (uint32_t)pBuff[bIndex+2]);
            PH_LOG_NCI_INFO_X32MSG("Power State:", (uint32_t)pBuff[bIndex + 3]);
            PH_LOG_NCI_INFO_STR("Technology: %!NCI_RF_TECHNOLOGY!", pBuff[bIndex+4]);
            break;
        case 0x01:
            PH_LOG_NCI_INFO_X32MSG("Route:", (uint32_t)pBuff[bIndex+2]);
            PH_LOG_NCI_INFO_X32MSG("Power State:", (uint32_t)pBuff[bIndex+3]);
            PH_LOG_NCI_INFO_STR("Protocol: %!NCI_RF_PROTOCOL!", pBuff[bIndex+4]);
            break;
        case 0x02:
            PH_LOG_NCI_INFO_X32MSG("Route:", (uint32_t)pBuff[bIndex+2]);
            PH_LOG_NCI_INFO_X32MSG("Power State:", (uint32_t)pBuff[bIndex + 3]);
            PH_LOG_NCI_INFO_HEXDUMP("AID: %!HEXDUMP!", WppLogHex((void*)&pBuff[bIndex+4], (uint16_t)pBuff[bIndex+1]-2));
            break;
        }

        bIndex += bSize;
    }
}

void phNciNfc_PrintRfGetListenModeRoutingCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    UNUSED(pBuff);
    PHNCINFC_VALIDATE_PACKET_LENGTH(0, wLen);
}

void phNciNfc_PrintRfDiscoverCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint8_t bNumConfigs;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bNumConfigs = pBuff[bIndex++];
    PH_LOG_NCI_INFO_X32MSG("Number of Mapping Configurations:", (uint32_t)bNumConfigs);

    for (bCount = 0; bCount < bNumConfigs; bCount++, bIndex += 2) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+2, wLen);
        PH_LOG_NCI_INFO_STR("RF Technology and Mode: %!NCI_RF_TECH_MODE!", pBuff[bIndex]);
        PH_LOG_NCI_INFO_X32MSG("Discovery Frequency:", (uint32_t)pBuff[bIndex+1]);
    }
}

void phNciNfc_PrintRfDiscoverSelectCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(3, wLen);
    PH_LOG_NCI_INFO_X32MSG("RF Discovery ID:", (uint32_t)pBuff[0]);
    PH_LOG_NCI_INFO_STR("RF Protocol: %!NCI_RF_PROTOCOL!", pBuff[1]);
    PH_LOG_NCI_INFO_STR("RF Interface: %!NCI_RF_INTERFACE!", pBuff[2]);
}

void phNciNfc_PrintRfDeactivateCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Deactivation Type: %!NCI_RF_DEACT_TYPE!", pBuff[0]);
}

void phNciNfc_PrintRfT3tPollingCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(4, wLen);
    PH_LOG_NCI_INFO_HEXDUMP("SENSF_REQ_PARAMS: %!HEXDUMP!", WppLogHex((void*)&pBuff[0], 4));
}

void phNciNfc_PrintRfParamUpdateCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint8_t bSize, bNumParams;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bNumParams = pBuff[bIndex++];
    PH_LOG_NCI_INFO_X32MSG("Number of Parameters:", (uint32_t)bNumParams);

    for (bCount = 0; bCount < bNumParams; bCount++) {
        bSize = (2 + pBuff[bIndex+1]);
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+bSize, wLen);

        PH_LOG_NCI_INFO_STR("ID: %!NCI_RF_PARAMETER_ID!", pBuff[bIndex]);
        PH_LOG_NCI_INFO_X32MSG("Length:", (uint32_t)pBuff[bIndex+1]);

        if (pBuff[bIndex+1] == 1) {
            PH_LOG_NCI_INFO_X32MSG("Value:", (uint32_t)pBuff[bIndex+2]);
        }
        else {
            PH_LOG_NCI_INFO_HEXDUMP("Value: %!HEXDUMP!",
                                    WppLogHex((void*)&pBuff[bIndex+2], (uint16_t)pBuff[bIndex+1]));
        }

        bIndex += bSize;
    }
}

void phNciNfc_PrintRfIsoDepPresChkCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    UNUSED(pBuff);
    PHNCINFC_VALIDATE_PACKET_LENGTH(0, wLen);
}

void phNciNfc_PrintNfceeDiscoverCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);

    if (pBuff[0] == 0x00) {
        PH_LOG_NCI_INFO_STR("Discovery Action: Disable discovery of NFCEE");
    }
    else if (pBuff[0] == 0x01) {
        PH_LOG_NCI_INFO_STR("Discovery Action: Enable discovery of NFCEE");
    }
    else {
        PH_LOG_NCI_INFO_STR("Discovery Action: RFU");
    }
}

void phNciNfc_PrintNfceeModeSetCmdDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
    PH_LOG_NCI_INFO_X32MSG("NFCEE ID:", (uint32_t)pBuff[0]);

    if (pBuff[1] == 0x00) {
        PH_LOG_NCI_INFO_STR("NFCEE Mode: Disable the connected NFCEE");
    }
    else if (pBuff[1] == 0x01) {
        PH_LOG_NCI_INFO_STR("NFCEE Mode: Enable the connected NFCEE");
    }
    else {
        PH_LOG_NCI_INFO_STR("NFCEE Mode: RFU");
    }
}

void phNciNfc_PrintCoreResetRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(PHNCINFC_CORE_RESET_RSP_LEN_NCI2x, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
    PHNCINFC_VALIDATE_PACKET_LENGTH(PHNCINFC_CORE_RESET_RSP_LEN_NCI1x, wLen);
    PH_LOG_NCI_INFO_X32MSG("NCI Version:", (uint32_t)pBuff[1]);
    NciVer = pBuff[1];
    PH_LOG_NCI_INFO_STR("Configuration Status: %!NCI_RESET_TYPE!", pBuff[2]);
}

void phNciNfc_PrintCoreInitNci1xRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0, bStatus, bNumInterfaces;
    uint16_t wMaxRoutingTableSize, wMaxSizeLargeParam;
    phNciNfc_sCoreNfccFeatures_t tNfccFeatures;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bStatus = pBuff[bIndex++];
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", bStatus);

    if (bStatus == PH_NCINFC_STATUS_OK)
    {
        PH_LOG_NCI_INFO_STR("Discovery Configuration Mode:");

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+4, wLen);
        tNfccFeatures.DiscoveryConfiguration = pBuff[bIndex++];
        tNfccFeatures.RoutingType = pBuff[bIndex++];
        tNfccFeatures.PwrOffState = pBuff[bIndex++];
        tNfccFeatures.Byte3 = pBuff[bIndex++];

        if (tNfccFeatures.DiscoveryConfiguration & PHNCINFC_DISCOVERY_FREQUENCY_CONFIG_SUPPORTED_MASK) {
            PH_LOG_NCI_INFO_STR("Discovery Frequency supported");
        }
        else {
            PH_LOG_NCI_INFO_STR("Discovery Frequency value is ignored");
        }

        if ((tNfccFeatures.DiscoveryConfiguration & PHNCINFC_DISCOVERY_CONFIG_MODE_MASK) == 0x00) {
            PH_LOG_NCI_INFO_STR("DH is the only entity that configures the NFCC");
        }
        else {
            PH_LOG_NCI_INFO_STR("NFCC can receive configurations from the DH and other NFCEEs");
        }

        PH_LOG_NCI_INFO_STR("Technology based routing %s", (tNfccFeatures.RoutingType & PHNCINFC_TECH_BASED_ROUTING_MASK) ? "supported" : "not supported");
        PH_LOG_NCI_INFO_STR("Protocol based routing %s", (tNfccFeatures.RoutingType & PHNCINFC_PROTO_BASED_ROUTING_MASK) ? "supported" : "not supported");
        PH_LOG_NCI_INFO_STR("AID based routing %s", (tNfccFeatures.RoutingType & PHNCINFC_AID_BASED_ROUTING_MASK) ? "supported" : "not supported");
        PH_LOG_NCI_INFO_STR("Battery Off state %s", (tNfccFeatures.PwrOffState & PHNCINFC_BATTERY_OFF_STATE_MASK) ? "supported" : "not supported");
        PH_LOG_NCI_INFO_STR("Switched Off state %s", (tNfccFeatures.PwrOffState & PHNCINFC_SWITCH_OFF_STATE_MASK) ? "supported" : "not supported");

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+1, wLen);
        bNumInterfaces = pBuff[bIndex++];
        PH_LOG_NCI_INFO_X32MSG("Number of Supported RF Interfaces:", (uint32_t)bNumInterfaces);

        for (bCount = 0; bCount < bNumInterfaces; bCount++) {
            PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+1, wLen);
            PH_LOG_NCI_INFO_STR("RF Interface: %!NCI_RF_INTERFACE!", pBuff[bIndex++]);
        }

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+1, wLen);
        PH_LOG_NCI_INFO_X32MSG("Max Logical Connections:",(uint32_t)pBuff[bIndex++]);

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+2, wLen);
        wMaxRoutingTableSize = ((uint16_t)pBuff[bIndex] + ((uint16_t)pBuff[bIndex+1] << 8));
        PH_LOG_NCI_INFO_X32MSG("Max Routing Table Size:", (uint32_t)wMaxRoutingTableSize);
        bIndex += 2;

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+1, wLen);
        PH_LOG_NCI_INFO_X32MSG("Max Control Packet Payload Size:", (uint32_t)pBuff[bIndex++]);

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+2, wLen);
        wMaxSizeLargeParam = ((uint16_t)pBuff[bIndex] + ((uint16_t)pBuff[bIndex+1] << 8));
        PH_LOG_NCI_INFO_X32MSG("Max Size for Large Parameters:", (uint32_t)wMaxSizeLargeParam);
        bIndex += 2;

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+1, wLen);
        PH_LOG_NCI_INFO_X32MSG("Manufacturer ID:", (uint32_t)pBuff[bIndex++]);

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+4, wLen);
        for (bCount = 0; bCount < 4; bCount++) {
            PH_LOG_NCI_INFO_STR("Manufacturer Specific Info Byte%d: 0x%x", (uint32_t)bCount+1, (uint32_t)pBuff[bIndex++]);
        }
    }
}

void phNciNfc_PrintCoreInitNci2xRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0, bStatus, bNumInterfaces, bExtLen;
    uint16_t wMaxRoutingTableSize, wMaxNFCVFrameSize;
    phNciNfc_sCoreNfccFeatures_t tNfccFeatures;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bStatus = pBuff[bIndex++];
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", bStatus);

    if (bStatus == PH_NCINFC_STATUS_OK)
    {
        PH_LOG_NCI_INFO_STR("Discovery Configuration Mode:");

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 4, wLen);
        tNfccFeatures.DiscoveryConfiguration = pBuff[bIndex++];
        tNfccFeatures.RoutingType = pBuff[bIndex++];
        tNfccFeatures.PwrOffState = pBuff[bIndex++];
        tNfccFeatures.Byte3 = pBuff[bIndex++];

        if (tNfccFeatures.DiscoveryConfiguration & PHNCINFC_DISCOVERY_FREQUENCY_CONFIG_SUPPORTED_MASK) {
            PH_LOG_NCI_INFO_STR("Discovery Frequency supported");
        }
        else {
            PH_LOG_NCI_INFO_STR("Discovery Frequency value is ignored");
        }

        if ((tNfccFeatures.DiscoveryConfiguration & PHNCINFC_DISCOVERY_CONFIG_MODE_MASK) == 0x00) {
            PH_LOG_NCI_INFO_STR("DH is the only entity that configures the NFCC");
        }
        else {
            PH_LOG_NCI_INFO_STR("NFCC can receive configurations from the DH and other NFCEEs");
        }

        if ((tNfccFeatures.DiscoveryConfiguration & PHNCINFC_HCI_NETWORK_SUPPORTED_MASK) == 0x00) {
            PH_LOG_NCI_INFO_STR("NFCC does not implement the HCI network");
        }
        else {
            PH_LOG_NCI_INFO_STR("NFCC implements the HCI network as defined in ETSI_102622");
        }

        PH_LOG_NCI_INFO_STR("Technology based routing %s", (tNfccFeatures.RoutingType & PHNCINFC_TECH_BASED_ROUTING_MASK) ? "supported" : "not supported");
        PH_LOG_NCI_INFO_STR("Protocol based routing %s", (tNfccFeatures.RoutingType & PHNCINFC_PROTO_BASED_ROUTING_MASK) ? "supported" : "not supported");
        PH_LOG_NCI_INFO_STR("AID based routing %s", (tNfccFeatures.RoutingType & PHNCINFC_AID_BASED_ROUTING_MASK) ? "supported" : "not supported");
        PH_LOG_NCI_INFO_STR("Battery Off state %s", (tNfccFeatures.PwrOffState & PHNCINFC_BATTERY_OFF_STATE_MASK) ? "supported" : "not supported");
        PH_LOG_NCI_INFO_STR("Switched Off state %s", (tNfccFeatures.PwrOffState & PHNCINFC_SWITCH_OFF_STATE_MASK) ? "supported" : "not supported");

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 1, wLen);
        PH_LOG_NCI_INFO_X32MSG("Max Logical Connections:", (uint32_t)pBuff[bIndex++]);

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 2, wLen);
        wMaxRoutingTableSize = ((uint16_t)pBuff[bIndex] + ((uint16_t)pBuff[bIndex + 1] << 8));
        PH_LOG_NCI_INFO_X32MSG("Max Routing Table Size:", (uint32_t)wMaxRoutingTableSize);
        bIndex += 2;

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 1, wLen);
        PH_LOG_NCI_INFO_X32MSG("Max Control Packet Payload Size:", (uint32_t)pBuff[bIndex++]);

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 1, wLen);
        PH_LOG_NCI_INFO_X32MSG("Max Data Packet Payload Size of the Static HCI Connection:", (uint32_t)pBuff[bIndex++]);

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 1, wLen);
        PH_LOG_NCI_INFO_X32MSG("Number of Credits of the Static HCI Connection:", (uint32_t)pBuff[bIndex++]);

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 2, wLen);
        wMaxNFCVFrameSize = ((uint16_t)pBuff[bIndex] + ((uint16_t)pBuff[bIndex + 1] << 8));
        PH_LOG_NCI_INFO_X32MSG("Max NFC-V RF Frame Size:", (uint32_t)wMaxNFCVFrameSize);
        bIndex += 2;

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 1, wLen);
        bNumInterfaces = pBuff[bIndex++];
        PH_LOG_NCI_INFO_X32MSG("Number of Supported RF Interfaces:", (uint32_t)bNumInterfaces);

        for (bCount = 0; bCount < bNumInterfaces; bCount++) {
            PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 2, wLen);
            PH_LOG_NCI_INFO_STR("RF Interface: %!NCI_RF_INTERFACE!", pBuff[bIndex++]);
            bExtLen = pBuff[bIndex++];
            PH_LOG_NCI_INFO_STR("Number of Extensions: %d", bExtLen);
            PH_LOG_NCI_INFO_HEXDUMP("Extension List: %!HEXDUMP!",
                WppLogHex((void*)&pBuff[bIndex], (uint16_t)bExtLen));
            bIndex += bExtLen;
        }
    }
}

void phNciNfc_PrintCoreSetConfigRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount, bIndex = 0;
    uint8_t bSize, bStatus, bNumParams;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bStatus = pBuff[bIndex++];
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", bStatus);

    if (bStatus != PH_NCINFC_STATUS_OK) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
        bNumParams = pBuff[bIndex++];
        PH_LOG_NCI_INFO_X32MSG("Num of Parameters:",(uint32_t)bNumParams);

        for (bCount = 0; bCount < bNumParams; bCount++) {
            bSize = (pBuff[bIndex] == PHNCINFC_TLVUTIL_NXP_PROP_ID1) ? 2 : 1;
            PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+bSize, wLen);

            if (pBuff[bIndex] < PHNCINFC_TLVUTIL_NXP_PROP_ID1) {
                PH_LOG_NCI_INFO_STR("ID: %!NCI_CONFIG_PARAM_ID!", pBuff[bIndex]);
            }
            else if (pBuff[bIndex] == PHNCINFC_TLVUTIL_NXP_PROP_ID1) {
                PH_LOG_NCI_INFO_STR("ID: %!NXP_CONFIG_PARAM_ID!", pBuff[bIndex+1]);
            }

            bIndex += bSize;
        }
    }
}

void phNciNfc_PrintCoreGetConfigRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint8_t bSize, bStatus, bNumParams;

    PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
    bStatus = pBuff[bIndex++];
    bNumParams = pBuff[bIndex++];

    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", bStatus);
    PH_LOG_NCI_INFO_X32MSG("Number of Parameters:", (uint32_t)bNumParams);

    for (bCount = 0; bCount < bNumParams; bCount++)
    {
        bSize = (PHNCINFC_TLVUTIL_NXP_PROP_ID1 == pBuff[bIndex]) ?
                            (pBuff[bIndex + 2] + 3) : (pBuff[bIndex + 1] + 2);
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+bSize, wLen);

        if (pBuff[bIndex] < PHNCINFC_TLVUTIL_NXP_PROP_ID1) {
            PH_LOG_NCI_INFO_STR("ID: %!NCI_CONFIG_PARAM_ID!", pBuff[bIndex]);
            PH_LOG_NCI_INFO_X32MSG("Length:", (uint32_t)pBuff[bIndex + 1]);

            if (pBuff[bIndex + 1] > 0) {

                if (pBuff[bIndex + 1] == 1) {
                    PH_LOG_NCI_INFO_X32MSG("Value:", (uint32_t)pBuff[bIndex + 2]);
                }
                else {
                    PH_LOG_NCI_INFO_HEXDUMP("Value: %!HEXDUMP!",
                                            WppLogHex((void*)&pBuff[bIndex + 2], (uint16_t)pBuff[bIndex + 1]));
                }
            }
        }
        else if (pBuff[bIndex] == PHNCINFC_TLVUTIL_NXP_PROP_ID1) {
            PH_LOG_NCI_INFO_STR("ID: %!NXP_CONFIG_PARAM_ID!", pBuff[bIndex + 1]);
            PH_LOG_NCI_INFO_X32MSG("Length:", (uint32_t)pBuff[bIndex + 2]);

            if (pBuff[bIndex + 2] == 1) {
                PH_LOG_NCI_INFO_X32MSG("Value:", (uint32_t)pBuff[bIndex + 3]);
            }
            else if (pBuff[bIndex + 2] > 0) {
                PH_LOG_NCI_INFO_HEXDUMP("Value: %!HEXDUMP!",
                                        WppLogHex((void*)&pBuff[bIndex + 3], (uint16_t)pBuff[bIndex + 2]));
            }
        }

        bIndex += bSize;
    }
}

void phNciNfc_PrintCoreConnCreateRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);

    if (pBuff[0] == PH_NCINFC_STATUS_OK) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(4, wLen);
        PH_LOG_NCI_INFO_X32MSG("Max Data Packet Payload Size:", (uint32_t)pBuff[1]);

        if (pBuff[2] != 0xFF) {
            PH_LOG_NCI_INFO_X32MSG("Initial Numer of Credits:", (uint32_t)pBuff[2]);
        }
        else {
            PH_LOG_NCI_INFO_STR("Data flow control is not used");
        }

        PH_LOG_NCI_INFO_X32MSG("Conn ID:", (uint32_t)pBuff[3]);
    }
}

void phNciNfc_PrintCoreConnCloseRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintRfDiscoverMapRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintRfSetListenModeRoutingRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintRfGetListenModeRoutingRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintRfDiscoverRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintRfDiscoverSelectRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintRfDeactivateRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintRfT3tPollingRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintRfParamUpdateRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount, bIndex = 0;
    uint8_t bStatus, bNumParams;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bStatus = pBuff[bIndex++];
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", bStatus);

    if (bStatus != PH_NCINFC_STATUS_OK) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
        bNumParams = pBuff[bIndex++];
        PH_LOG_NCI_INFO_X32MSG("Number of Parameters:", (uint32_t)bNumParams);

        for (bCount = 0; bCount < bNumParams; bCount++) {
            PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+1, wLen);
            PH_LOG_NCI_INFO_STR("ID: %!NCI_RF_PARAMETER_ID!", pBuff[bIndex++]);
        }
    }
}

void phNciNfc_PrintRfIsoDepPresChkRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintNfceeDiscoverRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);

    if (pBuff[0] == PH_NCINFC_STATUS_OK) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
        PH_LOG_NCI_INFO_X32MSG("Number of NFCEEs:", (uint32_t)pBuff[1]);
    }
}

void phNciNfc_PrintNfceeModeSetRspDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintCoreResetNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint16_t bLen;

    PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
    PH_LOG_NCI_INFO_X32MSG("Reason Code:", (uint32_t)pBuff[bIndex++]);
    PH_LOG_NCI_INFO_STR("Configuration Status: %!NCI_RESET_TYPE!", pBuff[bIndex++]);
    if (wLen >= 5)
    {
        NciVer = pBuff[bIndex];
        PH_LOG_NCI_INFO_X32MSG("NCI Version:", (uint32_t)pBuff[bIndex++]);
        PH_LOG_NCI_INFO_X32MSG("Manufacturer ID:", (uint32_t)pBuff[bIndex++]);
        PH_LOG_NCI_INFO_X32MSG("Manufacturer Specific Length:", (uint32_t)pBuff[bIndex++]);

        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex + 5, wLen);
        bLen = wLen - bIndex;
        for (bCount = 0; bCount < bLen; bCount++) {
            PH_LOG_NCI_INFO_STR("Manufacturer Specific Info Byte%d: 0x%x", (uint32_t)bCount + 1, (uint32_t)pBuff[bIndex++]);
        }
    }
}

void phNciNfc_PrintCoreConnCreditsNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint8_t bNumEntries;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bNumEntries = pBuff[bIndex++];
    PH_LOG_NCI_INFO_X32MSG("Number of Entries:", (uint32_t)bNumEntries);

    for (bCount = 0; bCount < bNumEntries; bCount++, bIndex += 2) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+2, wLen);
        PH_LOG_NCI_INFO_X32MSG("Conn ID:", (uint32_t)pBuff[bIndex]);
        PH_LOG_NCI_INFO_X32MSG("Credits:", (uint32_t)pBuff[bIndex+1]);
    }
}

void phNciNfc_PrintCoreGenericErrorNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintCoreInterfaceErrorNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
    PH_LOG_NCI_INFO_X32MSG("Conn ID:", (uint32_t)pBuff[1]);
}

void phNciNfc_PrintRfGetListenModeRoutingNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    phNciNfc_PrintRfSetListenModeRoutingCmdDescription(pBuff, wLen);
}

__forceinline void phNciNfc_PrintRfTechSpecificParameters(phNciNfc_RfTechMode_t eTechMode, uint8_t *pBuff, uint8_t bLen)
{
    uint16_t wSensRsp;
    uint8_t bIndex = 0;

    if (eTechMode == phNciNfc_NFCA_Poll) {
        wSensRsp = (uint16_t)pBuff[bIndex] + ((uint16_t)pBuff[bIndex+1] << 8);

        PH_LOG_NCI_INFO_X32MSG("SENS_RES Response:", (uint32_t)wSensRsp);
        bIndex += sizeof(uint16_t);

        PH_LOG_NCI_INFO_X32MSG("NFCID1 Length:", (uint32_t)pBuff[bIndex]);

        if (pBuff[bIndex] > 0) {
            PH_LOG_NCI_INFO_HEXDUMP("NFCID1: %!HEXDUMP!", WppLogHex(&pBuff[bIndex+1], pBuff[bIndex]));
        }
        bIndex += (pBuff[bIndex] + 1);

        PH_LOG_NCI_INFO_X32MSG("SEL_RES Response Length:", (uint32_t)pBuff[bIndex]);

        if (pBuff[bIndex] > 0) {
            PH_LOG_NCI_INFO_HEXDUMP("SEL_RES Response: %!HEXDUMP!", WppLogHex(&pBuff[bIndex+1], pBuff[bIndex]));
        }
    }
    else if (eTechMode == phNciNfc_NFCB_Poll) {
        PH_LOG_NCI_INFO_X32MSG("SENSB_RES Response Length:", (uint32_t)pBuff[bIndex]);

        if (pBuff[bIndex] > 0) {
            PH_LOG_NCI_INFO_HEXDUMP("SENSB_RES Response: %!HEXDUMP!", WppLogHex(&pBuff[bIndex+1], pBuff[bIndex]));
        }
    }
    else if (eTechMode == phNciNfc_NFCF_Poll) {
        if (pBuff[bIndex] == 1) {
            PH_LOG_NCI_INFO_STR("Bit Rate: 212 kbps");
        }
        else if (pBuff[bIndex] == 2) {
            PH_LOG_NCI_INFO_STR("Bit Rate: 424 kbps");
        }
        else {
            PH_LOG_NCI_INFO_STR("Bit Rate: RFU");
        }

        bIndex++;
        PH_LOG_NCI_INFO_X32MSG("SENSF_RES Response Length:", (uint32_t)pBuff[bIndex]);

        if (pBuff[bIndex] > 0) {
            PH_LOG_NCI_INFO_HEXDUMP("SENSF_RES Response: %!HEXDUMP!", WppLogHex(&pBuff[bIndex+1], pBuff[bIndex]));
        }
    }
    else if (eTechMode == phNciNfc_NFCF_Listen) {
        PH_LOG_NCI_INFO_X32MSG("Local NFCID2 Length:", (uint32_t)pBuff[bIndex]);

        if (pBuff[bIndex] > 0) {
            PH_LOG_NCI_INFO_HEXDUMP("Local NFCID2: %!HEXDUMP!", WppLogHex(&pBuff[bIndex+1], pBuff[bIndex]));
        }
    }
    else {
        PH_LOG_NCI_INFO_HEXDUMP("RF Technology Specific Parameters: %!HEXDUMP!", WppLogHex(pBuff, bLen));
    }
}

void phNciNfc_PrintRfDiscoverNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bIndex = 4;

    PHNCINFC_VALIDATE_PACKET_LENGTH(4, wLen);

    PH_LOG_NCI_INFO_X32MSG("RF Discovery ID:", (uint32_t)pBuff[0]);
    PH_LOG_NCI_INFO_STR("RF Protocol: %!NCI_RF_PROTOCOL!", pBuff[1]);
    PH_LOG_NCI_INFO_STR("RF Technology and Mode: %!NCI_RF_TECH_MODE!", pBuff[2]);
    PH_LOG_NCI_INFO_X32MSG("Length of RF Technology Specific Parameters:", (uint32_t)pBuff[3]);

    if (pBuff[3] > 0) {
        phNciNfc_PrintRfTechSpecificParameters((phNciNfc_RfTechMode_t)pBuff[2], &pBuff[4], pBuff[3]);
        bIndex += pBuff[3];
    }

    PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex, wLen);

    switch (pBuff[bIndex])
    {
    case 0:
    case 1:
        PH_LOG_NCI_INFO_STR("Notification Type: Last Notification");
        break;
    case 2:
        PH_LOG_NCI_INFO_STR("Notification Type: More Notification to follow");
        break;
    default:
        PH_LOG_NCI_INFO_STR("Notification Type: RFU");
        break;
    }
}

void phNciNfc_PrintRfIntfActivatedNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(7, wLen);

    PH_LOG_NCI_INFO_X32MSG("RF Discovery ID:", (uint32_t)pBuff[0]);
    PH_LOG_NCI_INFO_STR("RF Interface: %!NCI_RF_INTERFACE!", pBuff[1]);
    PH_LOG_NCI_INFO_STR("RF Protocol: %!NCI_RF_PROTOCOL!", pBuff[2]);
    PH_LOG_NCI_INFO_STR("RF Technology and Mode: %!NCI_RF_TECH_MODE!", pBuff[3]);
    PH_LOG_NCI_INFO_X32MSG("Max Data Packet Payload Size:", (uint32_t)pBuff[4]);
    PH_LOG_NCI_INFO_X32MSG("Initial Number of Credits:", (uint32_t)pBuff[5]);
    PH_LOG_NCI_INFO_X32MSG("Length of RF Technology Specific Parameters:", (uint32_t)pBuff[6]);

    if (pBuff[6] > 0) {
        phNciNfc_PrintRfTechSpecificParameters((phNciNfc_RfTechMode_t)pBuff[3], &pBuff[7], pBuff[6]);
    }
}

void phNciNfc_PrintRfDeactivateNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
    PH_LOG_NCI_INFO_STR("Deactivation Type: %!NCI_RF_DEACT_TYPE!", pBuff[0]);
    PH_LOG_NCI_INFO_STR("Deactivation Reason: %!NCI_RF_DEACT_REASON!", pBuff[1]);
}

void phNciNfc_PrintRfFieldInfoNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("RF Field Status: %!NCI_RF_FIELD_STATUS!", pBuff[0]);
}

void phNciNfc_PrintRfT3tPollingNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount = 0, bIndex = 0;
    uint8_t bStatus, bSize, bNumResponses;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bStatus = pBuff[bIndex++];
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", bStatus);

    if (bStatus == PH_NCINFC_STATUS_OK) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
        bNumResponses = pBuff[bIndex++];
        PH_LOG_NCI_INFO_X32MSG("Number of Responses:", (uint32_t)bNumResponses);

        for (bCount = 0; bCount < bNumResponses; bCount++) {
            bSize = (1 + pBuff[bIndex]);
            PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+bSize, wLen);

            if (pBuff[bIndex] > 0) {
                PH_LOG_NCI_INFO_HEXDUMP("SENSF_RES: %!HEXDUMP!", WppLogHex(&pBuff[bIndex+1], pBuff[bIndex]));
            }

            bIndex += bSize;
        }
    }
}

void phNciNfc_PrintRfNfceeActionNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(3, wLen);
    PH_LOG_NCI_INFO_X32MSG("NFCEE ID:", (uint32_t)pBuff[0]);
    PH_LOG_NCI_INFO_STR("Trigger: %!NCI_NFCEE_TRIGGER_TYPE!", pBuff[1]);
    PH_LOG_NCI_INFO_X32MSG("Supporting Data Length:", (uint32_t)pBuff[2]);

    if (pBuff[2] > 0) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(3 + pBuff[2], wLen);
        PH_LOG_NCI_INFO_HEXDUMP("Supporting Data: %!HEXDUMP!", WppLogHex(&pBuff[3], pBuff[2]));
    }
}

void phNciNfc_PrintRfNfceeDiscoveryReqNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bCount, bIndex = 0;
    uint8_t bSize, bNumEntries;

    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    bNumEntries = pBuff[bIndex++];
    PH_LOG_NCI_INFO_X32MSG("Number of Information Entries:", (uint32_t)bNumEntries);

    PH_LOG_NCI_INFO_STR("Information Entries:");

    for (bCount = 0; bCount < bNumEntries; bCount++)
    {
        bSize = (2 + pBuff[bIndex+1]);
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+bSize, wLen);

        switch (pBuff[bIndex])
        {
        case 0x00:
            PH_LOG_NCI_INFO_STR("Request is to add following discovery");

            if (3 == pBuff[bIndex+1]) {
                PH_LOG_NCI_INFO_X32MSG("NFCEE ID", pBuff[bIndex+2]);
                PH_LOG_NCI_INFO_STR("RF Technology and Mode %!NCI_RF_TECH_MODE!", pBuff[bIndex+3]);
                PH_LOG_NCI_INFO_STR("RF Protocol %!NCI_RF_PROTOCOL!", pBuff[bIndex+4]);
            }
            else {
                PH_LOG_NCI_INFO_STR("Invalid length!");
            }
            break;
        case 0x01:
            PH_LOG_NCI_INFO_STR("Request is to remove following discovery");

            if (3 == pBuff[bIndex+1]) {
                PH_LOG_NCI_INFO_X32MSG("NFCEE ID", pBuff[bIndex+2]);
                PH_LOG_NCI_INFO_STR("RF Technology and Mode %!NCI_RF_TECH_MODE!", pBuff[bIndex+3]);
                PH_LOG_NCI_INFO_STR("RF Protocol %!NCI_RF_PROTOCOL!", pBuff[bIndex+4]);
            }
            else {
                PH_LOG_NCI_INFO_STR("Invalid length!");
            }
            break;
        default:
            PH_LOG_NCI_INFO_X32MSG("Invalid or Proprietary TLV Type!", pBuff[bIndex]);
            break;
        }

        bIndex += bSize;
    }
}

void phNciNfc_PrintRfIsoDepPresChkNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintNfceeDiscoverNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    uint8_t bIndex = 0, bCount = 0;

    PHNCINFC_VALIDATE_PACKET_LENGTH(3, wLen);
    PH_LOG_NCI_INFO_X32MSG("NFCEE ID:", (uint32_t)pBuff[bIndex++]);

    switch (pBuff[bIndex++])
    {
    case 0x00:
        PH_LOG_NCI_INFO_STR("NFCEE Status: NFCEE connected and enabled");
        break;
    case 0x01:
        PH_LOG_NCI_INFO_STR("NFCEE Status: NFCEE connected and disabled");
        break;
    case 0x02:
        PH_LOG_NCI_INFO_STR("NFCEE Status: NFCEE removed");
        break;
    default:
        PH_LOG_NCI_INFO_STR("NFCEE Status: RFU");
        break;
    }

    bCount = pBuff[bIndex++];

    PH_LOG_NCI_INFO_X32MSG("Number of Protocols Information Entries", (uint32_t)bCount);
    PH_LOG_NCI_INFO_STR("Supported NFCEE Protocols");

    for (; bCount > 0; bCount--) {
        PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+1, wLen);
        PH_LOG_NCI_INFO_STR("NFCEE Interface/Protocol: %!NCI_NFCEE_INTERFACE!", pBuff[bIndex++]);
    }

    PHNCINFC_VALIDATE_PACKET_LENGTH(bIndex+1, wLen);
    PH_LOG_NCI_INFO_X32MSG("Number of TLVs are", (uint32_t)pBuff[bIndex++]);
}

void phNciNfc_PrintNfceeModeSetNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(1, wLen);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_STATUS!", pBuff[0]);
}

void phNciNfc_PrintNfceeStatusNtfDescription(uint8_t *pBuff, uint16_t wLen)
{
    PHNCINFC_VALIDATE_PACKET_LENGTH(2, wLen);
    PH_LOG_NCI_INFO_X32MSG("NFCEE ID", pBuff[0]);
    PH_LOG_NCI_INFO_STR("Status: %!NCI_NFCEE_STATUS!", pBuff[1]);
}

void phNciNfc_PrintPacketDescription(
    pphNciNfc_sCoreHeaderInfo_t pHeaderInfo,
    uint8_t *pBuff,
    uint16_t wLen,
    uint8_t bLogDataMessages)
{
    if (!WPP_FLAG_LEVEL_ENABLED(TF_NCI, LEVEL_INFO))
    {
        return;
    }

    PH_LOG_NCI_FUNC_ENTRY();

    PH_LOG_NCI_INFO_STR("NCI Packet Details:");
    PH_LOG_NCI_INFO_STR("====================================================");

    switch (pHeaderInfo->eMsgType)
    {
        case phNciNfc_e_NciCoreMsgTypeData:
        {
            PH_LOG_NCI_INFO_STR("Message type: Data");
            PH_LOG_NCI_INFO_X32MSG("Conn ID:", (uint32_t)pHeaderInfo->bConn_ID);
        }
        break;
        case phNciNfc_e_NciCoreMsgTypeCntrlCmd:
        {
            PH_LOG_NCI_INFO_STR("Message type: Command");
            PH_LOG_NCI_INFO_STR("GID: %!phNciNfc_CoreGid!", pHeaderInfo->Group_ID);

            switch (pHeaderInfo->Group_ID)
            {
            case phNciNfc_e_CoreNciCoreGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CoreNciCoreCmdOid_t!", pHeaderInfo->Opcode_ID.Val);

                switch (pHeaderInfo->Opcode_ID.Val)
                {
                case phNciNfc_e_NciCoreResetCmdOid:
                    phNciNfc_PrintCoreResetCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreInitCmdOid:
                    phNciNfc_PrintCoreInitCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreSetConfigCmdOid:
                    phNciNfc_PrintCoreSetConfigCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreGetConfigCmdOid:
                    phNciNfc_PrintCoreGetConfigCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreConnCreateCmdOid:
                    phNciNfc_PrintCoreConnCreateCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreConnCloseCmdOid:
                    phNciNfc_PrintCoreConnCloseCmdDescription(pBuff, wLen);
                    break;
                }
                break;
            case phNciNfc_e_CoreRfMgtGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CoreRfMgtCmdOid_t!", pHeaderInfo->Opcode_ID.Val);
                switch (pHeaderInfo->Opcode_ID.Val)
                {
                case phNciNfc_e_RfMgtRfDiscoverMapCmdOid:
                    phNciNfc_PrintRfDiscoverMapCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfSetRoutingCmdOid:
                    phNciNfc_PrintRfSetListenModeRoutingCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfGetRoutingCmdOid:
                    phNciNfc_PrintRfGetListenModeRoutingCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfDiscoverCmdOid:
                    phNciNfc_PrintRfDiscoverCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfDiscSelectCmdOid:
                    phNciNfc_PrintRfDiscoverSelectCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfDeactivateCmdOid:
                    phNciNfc_PrintRfDeactivateCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfT3tPollingCmdOid:
                    phNciNfc_PrintRfT3tPollingCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfParamUpdateCmdOid:
                    phNciNfc_PrintRfParamUpdateCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfIsoDepPresChkCmdOid:
                    phNciNfc_PrintRfIsoDepPresChkCmdDescription(pBuff, wLen);
                    break;
                }
                break;
            case phNciNfc_e_CoreNfceeMgtGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CoreNfceeMgtCmdOid_t!", pHeaderInfo->Opcode_ID.Val);
                switch (pHeaderInfo->Opcode_ID.Val)
                {
                case phNciNfc_e_NfceeMgtNfceeDiscCmdOid:
                    phNciNfc_PrintNfceeDiscoverCmdDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NfceeMgtModeSetCmdOid:
                    phNciNfc_PrintNfceeModeSetCmdDescription(pBuff, wLen);
                    break;
                }
                break;
            case phNciNfc_e_CorePropGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CorePropCmdOid_t!", pHeaderInfo->Opcode_ID.Val);
                break;
            case phNciNfc_e_CoreInvalidGid:
            default:
                PH_LOG_NCI_INFO_X32MSG("OID:",(uint32_t)pHeaderInfo->Opcode_ID.Val);
                break;
            }
        }
        break;
        case phNciNfc_e_NciCoreMsgTypeCntrlRsp:
        {
            PH_LOG_NCI_INFO_STR("Message type: Response");
            PH_LOG_NCI_INFO_STR("GID: %!phNciNfc_CoreGid!", pHeaderInfo->Group_ID);

            switch (pHeaderInfo->Group_ID)
            {
            case phNciNfc_e_CoreNciCoreGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CoreNciCoreRspOid_t!", pHeaderInfo->Opcode_ID.Val);
                switch (pHeaderInfo->Opcode_ID.Val)
                {
                case phNciNfc_e_NciCoreResetRspOid:
                    phNciNfc_PrintCoreResetRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreInitRspOid:
                    switch (NciVer & PH_NCINFC_VERSION_MAJOR_MASK)
                    {
                    case PH_NCINFC_VERSION_1x:
                        phNciNfc_PrintCoreInitNci1xRspDescription(pBuff, wLen);
                        break;
                    case PH_NCINFC_VERSION_2x:
                        phNciNfc_PrintCoreInitNci2xRspDescription(pBuff, wLen);
                        break;
                    }
                    break;
                case phNciNfc_e_NciCoreSetConfigRspOid:
                    phNciNfc_PrintCoreSetConfigRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreGetConfigRspOid:
                    phNciNfc_PrintCoreGetConfigRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreDhConnRspOid:
                    phNciNfc_PrintCoreConnCreateRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreConnCloseRspOid:
                    phNciNfc_PrintCoreConnCloseRspDescription(pBuff, wLen);
                    break;
                }
                break;
            case phNciNfc_e_CoreRfMgtGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CoreRfMgtRspOid_t!", pHeaderInfo->Opcode_ID.Val);
                switch (pHeaderInfo->Opcode_ID.Val)
                {
                case phNciNfc_e_RfMgtRfDiscoverMapRspOid:
                    phNciNfc_PrintRfDiscoverMapRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfSetRoutingRspOid:
                    phNciNfc_PrintRfSetListenModeRoutingRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfGetRoutingRspOid:
                    phNciNfc_PrintRfGetListenModeRoutingRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfDiscoverRspOid:
                    phNciNfc_PrintRfDiscoverRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfDiscSelectRspOid:
                    phNciNfc_PrintRfDiscoverSelectRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfDeactivateRspOid:
                    phNciNfc_PrintRfDeactivateRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfT3tPollingRspOid:
                    phNciNfc_PrintRfT3tPollingRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfParamUpdateRspOid:
                    phNciNfc_PrintRfParamUpdateRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfIsoDepPresChkRspOid:
                    phNciNfc_PrintRfIsoDepPresChkRspDescription(pBuff, wLen);
                    break;
                }
                break;
            case phNciNfc_e_CoreNfceeMgtGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CoreNfceeMgtRspOid_t!", pHeaderInfo->Opcode_ID.Val);
                switch (pHeaderInfo->Opcode_ID.Val)
                {
                case phNciNfc_e_NfceeMgtNfceeDiscRspOid:
                    phNciNfc_PrintNfceeDiscoverRspDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NfceeMgtModeSetRspOid:
                    phNciNfc_PrintNfceeModeSetRspDescription(pBuff, wLen);
                    break;
                }
                break;
            case phNciNfc_e_CorePropGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CorePropRspOid_t!", pHeaderInfo->Opcode_ID.Val);
                break;
            case phNciNfc_e_CoreInvalidGid:
            default:
                PH_LOG_NCI_INFO_X32MSG("OID:",(uint32_t)pHeaderInfo->Opcode_ID.Val);
                break;
            }
        }
        break;
        case phNciNfc_e_NciCoreMsgTypeCntrlNtf:
        {
            PH_LOG_NCI_INFO_STR("Message type: Notification Message");
            PH_LOG_NCI_INFO_STR("GID: %!phNciNfc_CoreGid!", pHeaderInfo->Group_ID);

            switch (pHeaderInfo->Group_ID)
            {
            case phNciNfc_e_CoreNciCoreGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CoreNciCoreNtfOid_t!", pHeaderInfo->Opcode_ID.Val);
                switch (pHeaderInfo->Opcode_ID.Val)
                {
                case phNciNfc_e_NciCoreResetNtfOid:
                    phNciNfc_PrintCoreResetNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreConnCreditNtfOid:
                    phNciNfc_PrintCoreConnCreditsNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreGenericErrNtfOid:
                    phNciNfc_PrintCoreGenericErrorNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NciCoreInterfaceErrNtfOid:
                    phNciNfc_PrintCoreInterfaceErrorNtfDescription(pBuff, wLen);
                    break;
                }
                break;
            case phNciNfc_e_CoreRfMgtGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CoreRfMgtNtfOid_t!", pHeaderInfo->Opcode_ID.Val);
                switch (pHeaderInfo->Opcode_ID.Val)
                {
                case phNciNfc_e_RfMgtRfGetListenModeRoutingNtfOid:
                    phNciNfc_PrintRfGetListenModeRoutingNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfDiscoverNtfOid:
                    phNciNfc_PrintRfDiscoverNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfIntfActivatedNtfOid:
                    phNciNfc_PrintRfIntfActivatedNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfDeactivateNtfOid:
                    phNciNfc_PrintRfDeactivateNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfFieldInfoNtfOid:
                    phNciNfc_PrintRfFieldInfoNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfT3tPollingNtfOid:
                    phNciNfc_PrintRfT3tPollingNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfNfceeActionNtfOid:
                    phNciNfc_PrintRfNfceeActionNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfNfceeDiscoveryReqNtfOid:
                    phNciNfc_PrintRfNfceeDiscoveryReqNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_RfMgtRfIsoDepPresChkNtfOid:
                    phNciNfc_PrintRfIsoDepPresChkNtfDescription(pBuff, wLen);
                    break;
                }
                break;
            case phNciNfc_e_CoreNfceeMgtGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CoreNfceeMgtNtfOid_t!", pHeaderInfo->Opcode_ID.Val);
                switch (pHeaderInfo->Opcode_ID.Val)
                {
                case phNciNfc_e_NfceeMgtNfceeDiscNtfOid:
                    phNciNfc_PrintNfceeDiscoverNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NfceeMgtModeSetNtfOid:
                    phNciNfc_PrintNfceeModeSetNtfDescription(pBuff, wLen);
                    break;
                case phNciNfc_e_NfceeMgtStatusNtfOid:
                    phNciNfc_PrintNfceeStatusNtfDescription(pBuff, wLen);
                    break;
                }
                break;
            case phNciNfc_e_CorePropGid:
                PH_LOG_NCI_INFO_STR("OID: %!phNciNfc_CorePropNtfOid_t!", pHeaderInfo->Opcode_ID.Val);
                break;
            case phNciNfc_e_CoreInvalidGid:
            default:
                PH_LOG_NCI_INFO_X32MSG("OID:",(uint32_t)pHeaderInfo->Opcode_ID.Val);
                break;
            }
        }
        break;
        default:
        {
            PH_LOG_NCI_INFO_STR("Message type: Invalid Message");
        }
        break;
    }

    PH_LOG_NCI_INFO_U32MSG("Payload Length:", (uint32_t)wLen);
    if (pHeaderInfo->eMsgType == phNciNfc_e_NciCoreMsgTypeData)
    {
        if (bLogDataMessages)
        {
            PH_LOG_NCI_INFO_HEXDUMP("Payload: %!HEXDUMP!", WppLogHex(pBuff, wLen));
        }
    }
    else
    {
        PH_LOG_NCI_INFO_HEXDUMP("Payload: %!HEXDUMP!", WppLogHex(pBuff, wLen));
    }

    PH_LOG_NCI_INFO_STR("====================================================");
    PH_LOG_NCI_FUNC_EXIT();
}
