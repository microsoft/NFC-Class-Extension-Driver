/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_RFReaderB.h"
#include "phNciNfc_RFReader4B.h"

#include "phNciNfc_RFReaderB.tmh"


NFCSTATUS
phNciNfc_RdrBInit(
                    pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                    uint8_t *pBuff,
                    uint16_t wLen
                )
{
    NFCSTATUS                  status = NFCSTATUS_SUCCESS;
    uint8_t                     *pRfNtfBuff;
    uint8_t                     RfTechSpecParamsLen;
    uint8_t                     ActvnParamsLen = 0;
    uint8_t                     bSensBRespLen = 0;
    uint8_t                     bSensBRespVal;
    phNciNfc_RFDevType_t        DevType = phNciNfc_eInvalid_DevType;

    PH_LOG_NCI_FUNC_ENTRY();

    if((0 != (wLen)) && (NULL != pBuff) && (NULL != pRemDevInf))
    {
        /* Capture Poll mode specific params info */
        if(phNciNfc_NFCB_Poll == pBuff[3])
        {
            PH_LOG_NCI_INFO_STR(" NFC-B Passive Poll Mode Info being captured..");

            bSensBRespLen = pBuff[7];
            (pRemDevInf->tRemoteDevInfo.Iso14443B_Info.bSensBRespLen) = bSensBRespLen;

            if((0 != bSensBRespLen) && ((11 == bSensBRespLen) || (12 == bSensBRespLen)))
            {
                phOsalNfc_SetMemory((pRemDevInf->tRemoteDevInfo.Iso14443B_Info.aSensBResp),0,bSensBRespLen);
                phOsalNfc_MemCopy(&(pRemDevInf->tRemoteDevInfo.Iso14443B_Info.aSensBResp)
                ,&pBuff[8],bSensBRespLen);

                /* Update the actual type of target device */
                bSensBRespVal = (pRemDevInf->tRemoteDevInfo.Iso14443B_Info.aSensBResp[9]);

                phNciNfc_GetRfDevType(bSensBRespVal,bSensBRespLen,pRemDevInf,&DevType);

                (pRemDevInf->RemDevType) = DevType;

                switch((pRemDevInf->RemDevType))
                {
                    case phNciNfc_eISO14443_4B_PICC:
                    {
                        if(phNciNfc_e_RfInterfacesISODEP_RF == (pRemDevInf->eRfIf))
                        {
                            /* Obtain the length of Activation parameters from pBuff */
                            RfTechSpecParamsLen = pBuff[6];
                            ActvnParamsLen = pBuff[7+RfTechSpecParamsLen+DATA_XCHG_PARAMS_LEN];

                            if(0 != ActvnParamsLen)
                            {
                                pRfNtfBuff = &(pBuff[ActvnParamsLen+1]);
                                (pRemDevInf->tRemoteDevInfo.Iso14443B_Info.bAttribRespLen) = *pRfNtfBuff;

                                if(0 != (pRemDevInf->tRemoteDevInfo.Iso14443B_Info.bAttribRespLen))
                                {
                                    phOsalNfc_SetMemory(&(pRemDevInf->tRemoteDevInfo.Iso14443B_Info.tAttribResp),0,
                                        sizeof(phNciNfc_ATTRIBResp_t));

                                    phOsalNfc_MemCopy(&(pRemDevInf->tRemoteDevInfo.Iso14443B_Info.tAttribResp),
                                        (pRfNtfBuff+1),(pRemDevInf->tRemoteDevInfo.Iso14443B_Info.bAttribRespLen));
                                }
                            }
                        }
                        else
                        {
                            /* TODO:- RF Frame interface case,no activation parameters available for 4B Tag */
                        }
                        gpphNciNfc_RdrDataXchgSequence[0].SequnceInitiate = &phNciNfc_Send4BData;
                        gpphNciNfc_RdrDataXchgSequence[0].SequenceProcess = &phNciNfc_Recv4BResp;
                        break;
                    }
                    case phNciNfc_eISO14443_BPrime_PICC:
                    {
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }

                /* Update gpphNciNfc_RdrDataXchgSequence with the appropriate functions to be called
                   by sequence handler on invocation during data exchange */

                gpphNciNfc_RdrDataXchgSequence[1].SequnceInitiate = NULL;
                gpphNciNfc_RdrDataXchgSequence[1].SequenceProcess = &phNciNfc_CompleteDataXchgSequence;
            }
            else
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                PH_LOG_NCI_INFO_STR(" Invalid SENSB_RES Length received..");
            }
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
