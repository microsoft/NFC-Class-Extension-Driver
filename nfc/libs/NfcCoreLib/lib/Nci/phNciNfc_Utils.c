/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_Utils.tmh"

#define SEL_RESP_CONFIG_MASK        (0xFFU)     /**< Mask to obtain the target type from SEL_RES byte */

#define MIFARE_UL_SAK               (0x00U)     /**< SAK value for Mifare UL */
#define MIFARE_1K_SAK               (0x08U)     /**< SAK value for Mifare 1k */
#define MIFARE_1K_CLASSIC_SAK       (0x01U)     /**< SAK value for Mifare 1k Classic (Old Card)*/
#define MIFARE_4K_SAK               (0x18U)     /**< SAK value for Mifare 4k */
#define MIFARE_MINI_SAK             (0x09U)     /**< SAK value for Mifare 2k */
#define MIFARE_1K_SAK88             (0x88U)     /**< SAK value for Mifare 1k - Infineon card */
#define MIFARE_1K_SAK28             (0x28U)     /**< SAK value for Mifare 1k - Emulation */
#define MIFARE_4K_SAK38             (0x38U)     /**< SAK value for Mifare 4k - Emulation */
#define MIFARE_4K_SAK98             (0x98U)     /**< SAK value for Mifare 4k - Pro card */
#define MIFARE_4K_SAKB8             (0xB8U)     /**< SAK value for Mifare 4k - Pro card */

static NFCSTATUS phNciNfc_ValidateIfActParams(uint8_t *pNtfBuff, uint16_t wSize);

uint8_t
phNciNfc_ValidateRfInterface(phNciNfc_RfInterfaces_t eRfInterface)
{
    uint8_t bStatus = 1;

    PH_LOG_NCI_FUNC_ENTRY();

    /* Verify whether input RF interface is out of the supported range */
    switch(eRfInterface)
    {
        case phNciNfc_e_RfInterfacesNfceeDirect_RF:
        case phNciNfc_e_RfInterfacesFrame_RF:
        case phNciNfc_e_RfInterfacesISODEP_RF:
        case phNciNfc_e_RfInterfacesNFCDEP_RF:
            bStatus = 0;
            break;
        default:
            bStatus = (eRfInterface >= PH_NCINFCTYPES_RF_INTFS_PROP_MIN &&
                       eRfInterface <= PH_NCINFCTYPES_RF_INTFS_PROP_MAX) ? 0 : 1;
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

uint8_t
phNciNfc_ValidateRfProtocol(phNciNfc_RfProtocols_t eRfProtocol)
{
    uint8_t bStatus = 1;

    PH_LOG_NCI_FUNC_ENTRY();

    /* Verify whether input Rf Protocol is out of the supported range */
    switch(eRfProtocol)
    {
        case phNciNfc_e_RfProtocolsUnknownProtocol:
        case phNciNfc_e_RfProtocolsT1tProtocol:
        case phNciNfc_e_RfProtocolsT2tProtocol:
        case phNciNfc_e_RfProtocolsT3tProtocol:
        case phNciNfc_e_RfProtocolsIsoDepProtocol:
        case phNciNfc_e_RfProtocolsNfcDepProtocol:
        case phNciNfc_e_RfProtocols15693Protocol:
        case phNciNfc_e_RfProtocolsKovioProtocol:
            bStatus = 0;
            break;
        default:
            bStatus = (eRfProtocol >= PH_NCINFCTYPES_RF_PROTOS_PROP_MIN &&
                       eRfProtocol <= PH_NCINFCTYPES_RF_PROTOS_PROP_MAX) ? 0 : 1;
            break;
    }

    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

uint8_t phNciNfc_ValidateRfTechMode(phNciNfc_RfTechMode_t eRfTechmode)
{
    uint8_t bStatus = 1;

    PH_LOG_NCI_FUNC_ENTRY();

    switch(eRfTechmode)
    {
        case phNciNfc_NFCA_Poll:
        case phNciNfc_NFCA_Active_Poll:
        case phNciNfc_NFCB_Poll:
        case phNciNfc_NFCF_Poll:
        case phNciNfc_NFCF_Active_Poll:
        case phNciNfc_NFCISO15693_Poll:
        case phNciNfc_NFCA_Listen:
        case phNciNfc_NFCA_Active_Listen:
        case phNciNfc_NFCB_Listen:
        case phNciNfc_NFCF_Listen:
        case phNciNfc_NFCF_Active_Listen:
        case phNciNfc_NFCISO15693_Active_Listen:
            bStatus = 0;
            break;
        default:
            bStatus = (eRfTechmode >= PH_NCINFCTYPES_RF_TECH_POLL_PROP_MIN && eRfTechmode <= PH_NCINFCTYPES_RF_TECH_POLL_PROP_MAX) ||
                      (eRfTechmode >= PH_NCINFCTYPES_RF_TECH_LISTEN_PROP_MIN && eRfTechmode <= PH_NCINFCTYPES_RF_TECH_LISTEN_PROP_MAX) ? 0 : 1;
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

uint8_t phNciNfc_ValidateBitRate(phNciNfc_BitRates_t eBitRate)
{
    uint8_t bStatus = 1;

    PH_LOG_NCI_FUNC_ENTRY();

    switch(eBitRate)
    {
        case phNciNfc_e_BitRate106:
        case phNciNfc_e_BitRate212:
        case phNciNfc_e_BitRate424:
        case phNciNfc_e_BitRate848:
        case phNciNfc_e_BitRate1696:
        case phNciNfc_e_BitRate3392:
        case phNciNfc_e_BitRate6784:
        case phNciNfc_e_BitRate26:
            bStatus = 0;
            break;
        default:
            bStatus = (eBitRate >= PH_NCINFCTYPES_BIT_RATE_PROP_MIN &&
                       eBitRate <= PH_NCINFCTYPES_BIT_RATE_PROP_MAX) ? 0 : 1;
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

void
phNciNfc_GetRfDevType(uint8_t bRespVal, uint8_t bRespLen,
                      pphNciNfc_RemoteDevInformation_t pRemDevInf,
                      phNciNfc_RFDevType_t *pDevType)
{
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pRemDevInf) && (NULL != pDevType))
    {
        /* Technology and mode of local device (NFCC) */
        if(phNciNfc_NFCA_Kovio_Poll == pRemDevInf->eRFTechMode)
        {
            if (phNciNfc_e_RfProtocolsKovioProtocol == pRemDevInf->eRFProtocol)
            {
                *pDevType = phNciNfc_eKovio_PICC;
            }
        }
        else if(phNciNfc_NFCA_Poll == pRemDevInf->eRFTechMode)
        {
            /* If length of select response is '0', remote device is of 'Jewel' type */
            if(0 != bRespLen)
            {
                if((phNciNfc_e_RfInterfacesNFCDEP_RF == pRemDevInf->eRfIf) &&
                   (phNciNfc_e_RfProtocolsNfcDepProtocol == pRemDevInf->eRFProtocol))
                {
                    /* Remote device is a P2P Target */
                    *pDevType = phNciNfc_eNfcIP1_Target;
                }
                else
                {
                    PH_LOG_NCI_INFO_X32MSG("SAK Received",(SEL_RESP_CONFIG_MASK & bRespVal));
                    switch(bRespVal & SEL_RESP_CONFIG_MASK)
                    {
                        /* Ref: section 4.8.2 of [DIGITAL] */
                        case 0:     /* Configured for Type 2 Tag Platform */
                        {
                            *pDevType = phNciNfc_eMifareUL_PICC;
                            PH_LOG_NCI_INFO_STR(" MifareUL tag detected..");
                            break;
                        }
                        case MIFARE_1K_SAK:
                        case MIFARE_1K_SAK88:
                        case MIFARE_1K_SAK28:
                        case MIFARE_1K_CLASSIC_SAK:
                        {
                            if((phNciNfc_e_RfInterfacesISODEP_RF == pRemDevInf->eRfIf) &&
                               (phNciNfc_e_RfProtocolsIsoDepProtocol == pRemDevInf->eRFProtocol))
                            {
                                /* Remote device is a phNciNfc_eISO14443_4A_PICC*/
                                *pDevType = phNciNfc_eISO14443_4A_PICC;
                                PH_LOG_NCI_INFO_STR("4A tag detected..");
                            }
                            else
                            {
                                *pDevType = phNciNfc_eMifare1k_PICC;
                                PH_LOG_NCI_INFO_STR(" Mifare1k tag detected..");
                            }
                        }
                        break;
                        case MIFARE_4K_SAK:
                        case MIFARE_4K_SAK38:
                        case MIFARE_4K_SAK98:
                        case MIFARE_4K_SAKB8:
                        {
                            *pDevType = phNciNfc_eMifare4k_PICC;
                            PH_LOG_NCI_INFO_STR(" Mifare4k tag detected..");
                        }
                        break;
                        case MIFARE_MINI_SAK:
                        {
                            *pDevType = phNciNfc_eMifareMini_PICC;
                            PH_LOG_NCI_INFO_STR(" Mifare Mini tag detected..");
                        }
                        break;
                        case 0x24:
                        case 0x20:     /* Configured for Type 4A Tag Platform */
                        case 0x60:     /* FIXME: Workaround to enable CE when ISODEP & NFCDEP is enabled */
                        case 0x68:
                        {
                            *pDevType = phNciNfc_eISO14443_4A_PICC;
                            PH_LOG_NCI_INFO_STR(" 4A tag detected..");
                            break;
                        }
                        default:
                        {
                            *pDevType = phNciNfc_eUnknown_DevType;
                            PH_LOG_NCI_INFO_STR(" Unknown tag detected..");
                            break;
                        }
                    }
                }
            }
            else
            {
                /* Type 1 Tag type [TODO?] Temporary type assigned below */
                *pDevType = phNciNfc_eJewel_PICC;
                PH_LOG_NCI_INFO_STR(" Type 1 Tag detected..");
            }
        }
        else if((phNciNfc_NFCA_Active_Listen == pRemDevInf->eRFTechMode) ||
                (phNciNfc_NFCF_Active_Listen == pRemDevInf->eRFTechMode) ||
                (phNciNfc_NFCA_Listen == pRemDevInf->eRFTechMode))
        {
            if((phNciNfc_e_RfInterfacesNFCDEP_RF == pRemDevInf->eRfIf) &&
               (phNciNfc_e_RfProtocolsNfcDepProtocol == pRemDevInf->eRFProtocol))
            {
                /* Remote device is a P2P Initiator */
                *pDevType =  phNciNfc_eNfcIP1_Initiator;
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Nfc-A Listen mode notification - unknown");
            }
        }
        else if(phNciNfc_NFCB_Poll == pRemDevInf->eRFTechMode)
        {
            if(bRespVal & 0x01)
            {
                PH_LOG_NCI_INFO_STR(" 4B tag detected..");
                *pDevType = phNciNfc_eISO14443_4B_PICC;
            }
            else
            {
                /* Todo:- Non ISO14443 B card detected,exact name to be updated */
                *pDevType = phNciNfc_eUnknown_DevType;
            }
        }
        else if((phNciNfc_NFCF_Poll == pRemDevInf->eRFTechMode) ||
                (phNciNfc_NFCF_Active_Poll == pRemDevInf->eRFTechMode))
        {
            if(phNciNfc_e_RfProtocolsNfcDepProtocol == pRemDevInf->eRFProtocol)
            {
                /* Remote device is a P2P Target */
                *pDevType = phNciNfc_eNfcIP1_Target;
            }
        }
        else if((phNciNfc_NFCA_Active_Poll == pRemDevInf->eRFTechMode)  )
        {
            if(phNciNfc_e_RfProtocolsNfcDepProtocol == pRemDevInf->eRFProtocol)
            {
                /* Remote device is a P2P Target */
                *pDevType = phNciNfc_eNfcIP1_Target;
            }
        }
        else
        {
            /* Other types */
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid Input Parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
}

NFCSTATUS
phNciNfc_ValidateIntfActvdNtf(uint8_t *pNtf, uint16_t wSize)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint16_t wOffset = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNtf) && (0 != wSize))
    {
        /* Validate the size of received ntf (Min it should be 11 bytes) */
        if(wSize >= PHNCINFC_RFDISC_ACTVNTFMINLEN)
        {
            if(pNtf[1] == phNciNfc_e_RfInterfacesNfceeDirect_RF)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                wOffset = (PHNCINFC_RF_DISCOVERY_ID_LEN +
                           PHNCINFC_RF_INTFERFACE_LEN +
                           PHNCINFC_RF_PROTOCOL_LEN +
                           PHNCINFC_ACTVD_RF_TECH_MODE_LEN +
                           PHNCINFC_MAX_DATA_PKT_PAYLOAD_LEN +
                           PHNCINFC_INITIAL_NO_CREDITS_LEN +
                           PHNCINFC_RF_TECH_SPECIFIC_PARAMS_LEN);
                wOffset += pNtf[wOffset-1];
                wOffset += (PHNCINFC_DATA_XCNG_RF_TECH_MODE_LEN +
                            PHNCINFC_DATA_XCNG_TX_BIT_RATE_LEN +
                            PHNCINFC_DATA_XCNG_RX_BIT_RATE_LEN +
                            PHNCINFC_ACTIVATION_PARAMS_LEN);
                wOffset += pNtf[wOffset-1];
                if(wOffset == wSize)
                {
                    /* Validate common parameters of Interface Actvf Ntf */
                    wStatus = phNciNfc_ValidateIfActParams(pNtf, wSize);
                }
            }
        }
    }

    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidateIfActParams(uint8_t *pNtfBuff, uint16_t wSize)
{
    NFCSTATUS               wStatus = NFCSTATUS_SUCCESS;
    uint8_t                 bRfDiscId;
    phNciNfc_RfInterfaces_t eRfIf;
    phNciNfc_RfProtocols_t  eRFProtocol;
    phNciNfc_RfTechMode_t   eActvdRFTechMode;
    uint8_t                 bMaxDataPayLoadSize;
    uint8_t                 bInitialCredit;
    uint8_t                 bTechSpecificParamLen;
    phNciNfc_RfTechMode_t   eDataXchgRFTechMode;
    uint8_t                 bTransBitRate;
    uint8_t                 bRecvBitRate;
    uint16_t                wOffset = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNtfBuff) && (0 != wSize))
    {
        bRfDiscId = pNtfBuff[wOffset++];
        eRfIf = (phNciNfc_RfInterfaces_t) pNtfBuff[wOffset++];
        eRFProtocol =  (phNciNfc_RfProtocols_t) pNtfBuff[wOffset++];
        eActvdRFTechMode = (phNciNfc_RfTechMode_t) pNtfBuff[wOffset++];
        bMaxDataPayLoadSize = pNtfBuff[wOffset++];
        bInitialCredit = pNtfBuff[wOffset++];
        UNREFERENCED_PARAMETER(bInitialCredit);
        bTechSpecificParamLen = pNtfBuff[wOffset++];
        wOffset += bTechSpecificParamLen;
        eDataXchgRFTechMode = (phNciNfc_RfTechMode_t) pNtfBuff[wOffset++];
        bTransBitRate = pNtfBuff[wOffset++];
        bRecvBitRate = pNtfBuff[wOffset];

        if((bRfDiscId < 1) || (bRfDiscId > PHNCINFC_RF_DISCOVERY_ID_MAX))
        {
            PH_LOG_NCI_CRIT_STR("Invalid RfDiscId!");
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        else if(phNciNfc_ValidateRfInterface(eRfIf))
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
            PH_LOG_NCI_CRIT_STR("Invalid RfInterface!");
        }
        else if(phNciNfc_ValidateRfProtocol(eRFProtocol))
        {
            PH_LOG_NCI_CRIT_STR("Invalid RfProtocol!");
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        else if(phNciNfc_ValidateRfTechMode(eActvdRFTechMode))
        {
            PH_LOG_NCI_CRIT_STR("Invalid Actv Rf Tech and mode received!");
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        else if(phNciNfc_ValidateRfTechMode(eDataXchgRFTechMode))
        {
            PH_LOG_NCI_CRIT_STR("Invalid DataXchg Actv Rf Tech and mode received!");
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }

        if((NFCSTATUS_SUCCESS == wStatus) && (bMaxDataPayLoadSize < PHNCINFC_MIN_DATA_PKT_PAYLOAD_SIZE))
        {
            PH_LOG_NCI_CRIT_STR("Invalid Max Data packet Payload!");
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if(phNciNfc_ValidateBitRate(bTransBitRate) || phNciNfc_ValidateBitRate(bRecvBitRate))
            {
                PH_LOG_NCI_CRIT_STR("Invalid BitRate!");
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
            else
            {
                /* Interface to Protocol mapping check */
                if((phNciNfc_e_RfInterfacesISODEP_RF == eRfIf) &&
                    (phNciNfc_e_RfProtocolsIsoDepProtocol != eRFProtocol))
                {
                    PH_LOG_NCI_CRIT_STR("ISO DEP interface mapped to wrong protocol!");
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                }
                else if( (phNciNfc_e_RfInterfacesTagCmd_RF == eRfIf) &&\
                         (phNciNfc_e_RfProtocolsMifCProtocol != eRFProtocol) )
                {
                    PH_LOG_NCI_CRIT_STR("TagCmd interface mapped to wrong protocol!");
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                }
                else if((phNciNfc_e_RfInterfacesFrame_RF == eRfIf) &&\
                        (eActvdRFTechMode != phNciNfc_NFCISO15693_Poll))
                {
                    if((phNciNfc_e_RfProtocolsT1tProtocol == eRFProtocol) ||
                        (phNciNfc_e_RfProtocolsT2tProtocol == eRFProtocol) ||
                        (phNciNfc_e_RfProtocolsT3tProtocol == eRFProtocol) ||
                        (phNciNfc_e_RfProtocolsKovioProtocol == eRFProtocol) ||
                        (phNciNfc_e_RfProtocolsIsoDepProtocol == eRFProtocol))
                    {
                        PH_LOG_NCI_INFO_STR("Valid Frame-RF interface to protocol mapping!");
                    }
                    else
                    {
                        PH_LOG_NCI_CRIT_STR("Frame-RF interface mapped to wrong protocol!");
                        wStatus = NFCSTATUS_INVALID_PARAMETER;
                    }
                }
                else if((phNciNfc_e_RfInterfacesNFCDEP_RF == eRfIf) &&
                        (phNciNfc_e_RfProtocolsNfcDepProtocol != eRFProtocol))
                {
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                    PH_LOG_NCI_CRIT_STR("NFC DEP interface mapped to wrong protocol!");
                }
                else
                {
                    PH_LOG_NCI_INFO_STR("Interface activated ntf parameter validation success");
                }
            }
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
