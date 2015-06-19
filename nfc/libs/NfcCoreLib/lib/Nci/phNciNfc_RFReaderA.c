/*
* =============================================================================
*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*
* =============================================================================
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_RFReaderA.h"
#include "phNciNfc_RFReader4A.h"
#include "phNciNfc_RFReaderMifare.h"
#include "phNciNfc_Jewel.h"

#include "phNciNfc_RFReaderA.tmh"

#define NFCID1_LEN4            0x04U   /**< 4 byte length NFCID1 */
#define NFCID1_LEN7            0x07U   /**< 7 byte length NFCID1 */
#define NFCID1_LEN10           0x0AU   /**< 10 byte length NFCID1 */

NFCSTATUS
phNciNfc_RdrAInit(
                    pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                    uint8_t *pBuff,
                    uint16_t wLen
                )
{
    NFCSTATUS                  status = NFCSTATUS_SUCCESS;
    uint8_t                     *pRfNtfBuff = NULL;
    uint8_t                     RfTechSpecParamsLen = 0;
    uint8_t                     ActvnParamsLen = 0;
    uint8_t                     bSelResRespLen = 0;
    uint8_t                     bUidLength = 0;
    uint8_t                     bSelRespVal = 0;
    phNciNfc_RFDevType_t        eDevType = phNciNfc_eInvalid_DevType;

    PH_LOG_NCI_FUNC_ENTRY();
    if((0 != (wLen)) && (NULL != pBuff) && (NULL != pRemDevInf))
    {
        PH_LOG_NCI_INFO_STR(" NFC-A Passive Poll Mode Info being captured..");

        /* Length of technology specific parameters */
        RfTechSpecParamsLen = pBuff[6];

        /* Shift the buffer pointer points to first parameter of technology specific parameters */
        pRfNtfBuff = &pBuff[7];

        /* Get technology specific parameters incase of Nfc-A poll mode */
        if(phNciNfc_NFCA_Listen != pRemDevInf->eRFTechMode)
        {
            /* Length of NFCID1 */
            bUidLength = *(pRfNtfBuff+2);

            if((0 != bUidLength) &&
               (NFCID1_LEN4 != bUidLength) &&
               (NFCID1_LEN7 != bUidLength) &&
               (NFCID1_LEN10 != bUidLength))
            {
                PH_LOG_NCI_CRIT_STR("Invalid Nfc-A UID length");
                status = NFCSTATUS_FAILED;
            }

            if(NFCSTATUS_SUCCESS == status)
            {
                /* Length of SEL_RES response */
                bSelResRespLen = *(pRfNtfBuff + 3 + bUidLength);
                if(1 == bSelResRespLen)
                {
                    /* Length of SEL_RES shall be '0' incase of Nfc Forum Type 1 Tag */
                    bSelRespVal = *(pRfNtfBuff + 3 + bUidLength + 1);
                }
                else if(0 == bSelResRespLen)
                {
                    bSelRespVal = 0;
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Invalid Nfc-A SEL_RES length");
                    status = NFCSTATUS_FAILED;
                }
            }
        }
        if(NFCSTATUS_SUCCESS == status)
        {
            phNciNfc_GetRfDevType(bSelRespVal,bSelResRespLen,pRemDevInf,&eDevType);
            (pRemDevInf->RemDevType) = eDevType;
        }
        else
        {
            pRemDevInf->RemDevType = phNciNfc_eInvalid_DevType;
        }

        /*Update technology specific parameters (these parameters are specific to the remote device type detected)*/
        switch((pRemDevInf->RemDevType))
        {
            case phNciNfc_eISO14443_4A_PICC:
            case phNciNfc_eMifareUL_PICC:
            case phNciNfc_eMifare1k_PICC:
            case phNciNfc_eMifare4k_PICC:
            case phNciNfc_eMifareMini_PICC:
            {
                if(0 != bUidLength)
                {
                    (pRemDevInf->tRemoteDevInfo.Iso14443A_Info.UidLength) = bUidLength;
                    (pRemDevInf->tRemoteDevInfo.Iso14443A_Info.bSelResRespLen) = bSelResRespLen;
                    (pRemDevInf->tRemoteDevInfo.Iso14443A_Info.Sak) = bSelRespVal;

                    phOsalNfc_MemCopy(&(pRemDevInf->tRemoteDevInfo.Iso14443A_Info.bSensResResp),pRfNtfBuff,2);
                    phOsalNfc_SetMemory((pRemDevInf->tRemoteDevInfo.Iso14443A_Info.Uid),0,PH_NCINFCTYPES_MAX_UID_LENGTH);
                    phOsalNfc_MemCopy(&(pRemDevInf->tRemoteDevInfo.Iso14443A_Info.Uid),
                        (pRfNtfBuff+3),(pRemDevInf->tRemoteDevInfo.Iso14443A_Info.UidLength));
                }
                else
                {
                    status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
                    PH_LOG_NCI_INFO_STR(" Invalid UID Length received");
                }
            }
            break;
            case phNciNfc_eJewel_PICC:
            {
                if((0 == bUidLength) || (NFCID1_LEN4 == bUidLength))
                {
                    phOsalNfc_MemCopy(&(pRemDevInf->tRemoteDevInfo.Jewel_Info.bSensResResp),pRfNtfBuff,2);
                }
                else
                {
                    status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
                    PH_LOG_NCI_INFO_STR(" Invalid UID Length received");
                }
            }
            break;
            default:
            {
                break;
            }
        }

        /* Update gpphNciNfc_RdrDataXchgSequence with the appropriate functions to be called
               by sequence handler on invocation during data exchange */

        switch(pRemDevInf->RemDevType)
        {
            case phNciNfc_eISO14443_A_PICC:
            {
                break;
            }
            case phNciNfc_eISO14443_4A_PICC:
            {
                if(NFCSTATUS_SUCCESS == status)
                {
                    if(phNciNfc_e_RfInterfacesISODEP_RF == (pRemDevInf->eRfIf))
                    {
                        /* Obtain the length of Activation parameters from pBuff */
                        ActvnParamsLen = pBuff[7+RfTechSpecParamsLen+PH_NCINFCTYPES_DATA_XCHG_PARAMS_LEN];

                        if(0 != ActvnParamsLen)
                        {
                            pRfNtfBuff = &(pBuff[7+RfTechSpecParamsLen+PH_NCINFCTYPES_DATA_XCHG_PARAMS_LEN+1]);
                            pRemDevInf->tRemoteDevInfo.Iso14443A_Info.bRatsRespLen = *pRfNtfBuff;

                            if(0 != pRemDevInf->tRemoteDevInfo.Iso14443A_Info.bRatsRespLen)
                            {
                                if(((ActvnParamsLen - 1) >= pRemDevInf->tRemoteDevInfo.Iso14443A_Info.bRatsRespLen) &&
                                    (sizeof(phNciNfc_RATSResp_t) >= pRemDevInf->tRemoteDevInfo.Iso14443A_Info.bRatsRespLen))
                                {
                                    phOsalNfc_MemCopy(&(pRemDevInf->tRemoteDevInfo.Iso14443A_Info.
                                        tRatsResp),(pRfNtfBuff+1),(pRemDevInf->tRemoteDevInfo.
                                        Iso14443A_Info.bRatsRespLen));
                                }
                                else
                                {
                                    PH_LOG_NCI_CRIT_STR("Invalid RATS Resp Recvd!!, ActvnParamsLen %d, RatsRespLen %d",
                                        ActvnParamsLen, pRemDevInf->tRemoteDevInfo.Iso14443A_Info.bRatsRespLen);
                                    status = NFCSTATUS_FAILED;
                                }
                            }
                        }
                    }
                    else
                    {
                        /* TODO:- RF Frame interface case,no activation parameters available for 4A Tag */
                    }
                    gpphNciNfc_RdrDataXchgSequence[0].SequnceInitiate = &phNciNfc_Send4AData;
                    gpphNciNfc_RdrDataXchgSequence[0].SequenceProcess = &phNciNfc_Recv4AResp;
                }
                break;
            }
            case phNciNfc_eMifareUL_PICC:
            {
                if(NFCSTATUS_SUCCESS == status)
                {
                    gpphNciNfc_RdrDataXchgSequence[0].SequnceInitiate = &phNciNfc_SendMfReq;
                    gpphNciNfc_RdrDataXchgSequence[0].SequenceProcess = &phNciNfc_RecvMfResp;
                }
                break;
            }
            case phNciNfc_eMifare1k_PICC:
            case phNciNfc_eMifare4k_PICC:
            case phNciNfc_eMifareMini_PICC:
            {
                if(NFCSTATUS_SUCCESS == status)
                {
                    gpphNciNfc_RdrDataXchgSequence[0].SequnceInitiate = &phNciNfc_SendMfReq;
                    gpphNciNfc_RdrDataXchgSequence[0].SequenceProcess = &phNciNfc_RecvMfResp;
                }
            }
            break;
            case phNciNfc_eJewel_PICC:
            {
                if(NFCSTATUS_SUCCESS == status)
                {
                    /* Validate Technology specific parameters */
                    status = phNciNfc_JewelInit(pRemDevInf->tRemoteDevInfo.Jewel_Info.bSensResResp);

                    gpphNciNfc_RdrDataXchgSequence[0].SequnceInitiate = &phNciNfc_SendJewelReq;
                    gpphNciNfc_RdrDataXchgSequence[0].SequenceProcess = &phNciNfc_RecvJewelResp;
                }
                break;
            }
            default:
            {
              break;
            }
        }
        gpphNciNfc_RdrDataXchgSequence[1].SequnceInitiate = NULL;
        gpphNciNfc_RdrDataXchgSequence[1].SequenceProcess = &phNciNfc_CompleteDataXchgSequence;
    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Params..");
     }
    PH_LOG_NCI_FUNC_EXIT();
#pragma prefast(suppress: __WARNING_POTENTIAL_RANGE_POSTCONDITION_VIOLATION, "ESP:1220 PreFast Bug")
    return status;
}
