/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_ListenNfcDep.tmh"

NFCSTATUS
phNciNfc_NfcDepLstnRdrAInit(
                                pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                                uint8_t *pBuff,
                                uint16_t wLen
                         )
{
    NFCSTATUS                  status = NFCSTATUS_SUCCESS;
    uint8_t                     *pRfNtfBuff = NULL;
    uint8_t                     RfTechSpecParamsLen = 0;
    uint8_t                     ActvnParamsLen = 0;
    uint8_t                     AtrLen = 0;
    uint8_t                     bCount = 7;

    PH_LOG_NCI_FUNC_ENTRY();
    if((wLen > bCount + RfTechSpecParamsLen + PH_NCINFCTYPES_DATA_XCHG_PARAMS_LEN) && (NULL != pBuff) && (NULL != pRemDevInf))
    {
        /* No Technology specific parameters incase of Listen A as per NCI Spec */
        RfTechSpecParamsLen = pBuff[6];

        /* Shift the buffer pointer points to first parameter of technology specific parameters */
        pRfNtfBuff = &pBuff[bCount];

        /* The activated remote device is a P2P Initiator */
        (pRemDevInf->RemDevType) = phNciNfc_eNfcIP1_Initiator;

        /* NOTE: NO TECHNOLOGY SPECIFIC PARAMETERS ARE DEFINED FOR LISTEN NFC-A TECHN (as per Nci Spec Rev.21) */
        /* Initiator speed (Initiator to target speed) */
        pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Datarate = (phNciNfc_BitRates_t)pRemDevInf->bTransBitRate;
        switch(pRemDevInf->eRFTechMode)
        {
            case phNciNfc_NFCA_Active_Listen:
            case phNciNfc_NFCF_Active_Listen:
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active = 1; /* Active communciation */
            break;
            default:
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active = 0; /* Passive communciation */
            break;
        }

        bCount += RfTechSpecParamsLen + PH_NCINFCTYPES_DATA_XCHG_PARAMS_LEN;
        /* Obtain the length of Activation parameters from pBuff */
        ActvnParamsLen = pBuff[bCount];
        if(0 != ActvnParamsLen)
        {
            bCount++;
            AtrLen = pBuff[bCount];

            status = (wLen > bCount + AtrLen) ? NFCSTATUS_SUCCESS : NFCSTATUS_INVALID_PARAMETER;

            /* Holds ATR_RES if remote device is a P2P target and ATR_REQ if remote device is a
               P2P initiator */
            /* The relevant ATR Length is present in the first activation parameter.
               In the case of NCI 2.0, we ignore the Data Exchange Length Reduction parameter for now */
            if (0 != AtrLen && status == NFCSTATUS_SUCCESS)
            {
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length = AtrLen;
                bCount++;
                pRfNtfBuff = &(pBuff[bCount]);
                phOsalNfc_SetMemory(pRemDevInf->tRemoteDevInfo.NfcIP_Info.aAtrInfo, 0,
                                    PH_NCINFCTYPES_ATR_MAX_LEN);
                if (pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length <= PH_NCINFCTYPES_ATR_MAX_LEN)
                {
                    phOsalNfc_MemCopy(pRemDevInf->tRemoteDevInfo.NfcIP_Info.aAtrInfo,
                                pRfNtfBuff, pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length);
                }
                else
                {
                    pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length = 0;
                    PH_LOG_NCI_CRIT_STR("Invalid ATR_INFO length");
                    status = NFCSTATUS_FAILED;
                }
            }
        }

        if(0 != RfTechSpecParamsLen && status == NFCSTATUS_SUCCESS)
        {
            pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length = pRfNtfBuff[0];
            phOsalNfc_SetMemory(pRemDevInf->tRemoteDevInfo.NfcIP_Info.aAtrInfo, 0,
                                PH_NCINFCTYPES_ATR_MAX_LEN);
            if(pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length <= PH_NCINFCTYPES_ATR_MAX_LEN)
            {
                /* Specific Parameters for NFC-ACM Listen Mode have the following format:
                    - ATR_RES Response Length: 1 byte
                    - ATR_RES Response: n bytes*/
                phOsalNfc_MemCopy(pRemDevInf->tRemoteDevInfo.NfcIP_Info.aAtrInfo,
                            pRfNtfBuff + 1, pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length);
            }
            else
            {
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length = 0;
                PH_LOG_NCI_CRIT_STR("Invalid ATR_INFO length");
                status = NFCSTATUS_FAILED;
            }
        }

        /* In NCI1.0 NFC-DEP params are stored in Activation Parameters
           In NCI2.0 NFC-DEP params are stored in:
           - RF Technology Specific Parameters in case of Active P2P
           - Activation Parameters in case of Passive P2P
           If using Active Communication Mode the RF_INTF_ACTIVATED_INTF SHALL NOT include any
           Activation Parameters. */
        if(status == NFCSTATUS_SUCCESS &&
           ActvnParamsLen != 0 && 
           pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active == 1 &&
           phNciNfc_IsVersion2x(phNciNfc_GetContext()))
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
            PH_LOG_NCI_INFO_STR(" Invalid Params..");
        }

        /* The Activated device is a P2P Initiator. The P2P 'Send' and 'Receive' functions shall be active.
           The P2P Target (i.e., us) is expected to receive data from remote P2P initiator and expected
           to send data to Remote P2P target in response */
    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Params..");
     }
    PH_LOG_NCI_FUNC_EXIT();
    return status;
}

NFCSTATUS
phNciNfc_NfcDepLstnRdrFInit(
                           pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                           uint8_t *pBuff,
                           uint16_t wLen)
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    uint8_t                     *pRfNtfBuff;
    uint8_t                     RfTechSpecParamsLen;
    uint8_t                     bNfcId2Len;
    uint8_t                     ActvnParamsLen = 0;
    uint8_t                     AtrLen = 0;
    uint8_t                     bCount = 7;

    PH_LOG_NCI_FUNC_ENTRY();
    if( (0 != (wLen)) && (NULL != pBuff) && (NULL != pRemDevInf))
    {
        /* Remote devuce is a P2P Initiator */
        pRemDevInf->RemDevType = phNciNfc_eNfcIP1_Initiator;

        /* Obtain the len of RF tech specific parameters from Resp buff */
        RfTechSpecParamsLen = pBuff[6]; /*TODO: Check should added for this*/
        pRfNtfBuff = &pBuff[bCount];

        bNfcId2Len = *(pRfNtfBuff);

        pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID_Length = 0;
        if(0 != RfTechSpecParamsLen)
        {
            pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID_Length = bNfcId2Len;
            phOsalNfc_SetMemory(pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID,0,PH_NCINFCTYPES_MAX_NFCID3_SIZE);
            if(0 == bNfcId2Len)
            {
                PH_LOG_NCI_CRIT_STR("No NFCID2 received");
            }
            else if(PH_NCINFCTYPES_NFCID2_LEN == bNfcId2Len)
            {
                phOsalNfc_SetMemory(pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID,0,PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                phOsalNfc_MemCopy(pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID,(pRfNtfBuff+1),bNfcId2Len);
            }
            else
            {
                status = NFCSTATUS_FAILED;
                PH_LOG_NCI_CRIT_STR("Invalid NFCID2 received");
            }
        }

        if(NFCSTATUS_SUCCESS == status)
        {
            /* Initiator speed (Initiator to target speed) */
            pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Datarate = (phNciNfc_BitRates_t)pRemDevInf->bTransBitRate;

            switch(pRemDevInf->eRFTechMode)
            {
                case phNciNfc_NFCA_Active_Listen:
                case phNciNfc_NFCF_Active_Listen:
                    pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active = 1; /* Active communciation */
                break;
                default:
                    pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active = 0; /* Passive communciation */
                break;
            }

            bCount += RfTechSpecParamsLen + PH_NCINFCTYPES_DATA_XCHG_PARAMS_LEN;
            /* Obtain the length of Activation parameters from pBuff */
            ActvnParamsLen = pBuff[bCount];

            if (0 != ActvnParamsLen)
            {
                bCount++;
                AtrLen = pBuff[bCount];
            }
            if(0 != AtrLen)
            {
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length = AtrLen;
                bCount++;
                pRfNtfBuff = &(pBuff[bCount]);
                phOsalNfc_SetMemory(pRemDevInf->tRemoteDevInfo.NfcIP_Info.aAtrInfo,0,
                                    PH_NCINFCTYPES_ATR_MAX_LEN);
                if(pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length <= PH_NCINFCTYPES_ATR_MAX_LEN)
                {
                    phOsalNfc_MemCopy(pRemDevInf->tRemoteDevInfo.NfcIP_Info.aAtrInfo,
                                pRfNtfBuff,pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length);
                }
                else
                {
                    pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length = 0;
                    PH_LOG_NCI_CRIT_STR("Invalid ATR_INFO length");
                    status = NFCSTATUS_FAILED;
                }
            }
            /* The Activated device is a P2P Initiator. The P2P 'Send' and 'Receive' functions shall be active.
               The P2P Target (i.e., us) is expected to receive data from remote P2P initiator and expected
               to send data to Remote P2P target in response */
        }
    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Params..");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return status;
}
