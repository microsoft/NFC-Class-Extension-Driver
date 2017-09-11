/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_PollNfcDep.tmh"

NFCSTATUS
phNciNfc_NfcDepPollRdrAInit(
                         pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                         uint8_t *pBuff,
                         uint16_t wLen)
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    uint8_t                     *pRfNtfBuff = NULL;
    uint8_t                     RfTech;
    uint8_t                     RfTechSpecParamsLen = 0;
    uint8_t                     ActvnParamsLen = 0;
    uint8_t                     AtrLen = 0;
    uint8_t                     bSelResRespLen = 0;
    uint8_t                     bUidLength = 0;
    uint8_t                     bSelRespVal = 0;
    uint8_t                     bCount = 7;

    PH_LOG_NCI_FUNC_ENTRY();

    if((0 != (wLen)) && (NULL != pBuff) && (NULL != pRemDevInf))
    {
        RfTech = pBuff[3];

        /* Length of technology specific parameters */
        RfTechSpecParamsLen = pBuff[6];

        /* Clear information */
        pRemDevInf->tRemoteDevInfo.NfcIP_Info.SelRes = 0;
        pRemDevInf->tRemoteDevInfo.NfcIP_Info.SelResLen = 0;
        pRemDevInf->tRemoteDevInfo.NfcIP_Info.SensResLength = 0;
        pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID_Length = 0;
        if(0 != RfTechSpecParamsLen)
        {
            /* Point the buffer to the first parameter of technology specific parameters */
            pRfNtfBuff = &pBuff[bCount];

            if(RfTech == phNciNfc_NFCA_Poll)
            {
                /* Get technology specific parameters incase of Nfc-A poll mode */
                /* Length of NFCID1 */
                bUidLength = *(pRfNtfBuff + 2);
                /* Length of SEL_RES response */
                bSelResRespLen = *(pRfNtfBuff + 3 + bUidLength);
                if(0 != bSelResRespLen)
                {
                    /* Length of SEL_RES should always be '1' as per Nci spec ver 25 */
                    if(1 == bSelResRespLen)
                    {
                        /* Length of SEL_RES shall be '0' incase of Nfc Forum Type 1 Tag */
                        bSelRespVal = *(pRfNtfBuff + 3 + bUidLength + 1);
                    }
                    else
                    {
                        PH_LOG_NCI_CRIT_STR("Invalid SEL_RES length");
                        status = NFCSTATUS_FAILED;
                    }
                }
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.SelRes = bSelRespVal;
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.SelResLen = bSelResRespLen;

                phOsalNfc_SetMemory(pRemDevInf->tRemoteDevInfo.NfcIP_Info.SensRes, 0,
                                    PH_NCINFCTYPES_SENS_RES_LEN);
                /* Sense Response incase of Poll Nfc-A is of 2 bytes (incase of Poll Nfc-F, its 16 or 18 bytes) */
                phOsalNfc_MemCopy(pRemDevInf->tRemoteDevInfo.NfcIP_Info.SensRes, pRfNtfBuff, 2);
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.SensResLength = 2;

                /* NFCID3 shall is always 10 bytes */
                if(0 == bUidLength)
                {
                    PH_LOG_NCI_CRIT_STR("NFCID3 does not exist");
                }
                else if((PH_NCINFCTYPES_NFCID1_4BYTES == bUidLength) ||
                        (PH_NCINFCTYPES_NFCID1_7BYTES == bUidLength) ||
                        (PH_NCINFCTYPES_MAX_NFCID1_SIZE == bUidLength))
                {
                    pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID_Length = bUidLength;
                    phOsalNfc_SetMemory((pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID), 0,
                                PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                    phOsalNfc_MemCopy(&(pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID),
                                (pRfNtfBuff + 3), bUidLength);
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Invalid NFCID3 length");
                    status = NFCSTATUS_FAILED;
                }
            }
            else
            {
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length = pRfNtfBuff[0];
                phOsalNfc_SetMemory(pRemDevInf->tRemoteDevInfo.NfcIP_Info.aAtrInfo, 0,
                            PH_NCINFCTYPES_ATR_MAX_LEN);

                if(pRemDevInf->tRemoteDevInfo.NfcIP_Info.bATRInfo_Length <= PH_NCINFCTYPES_ATR_MAX_LEN)
                {
                    /* Specific Parameters for NFC-ACM Poll Mode have the following format:
                       - ATR_REQ Response Length: 1 byte
                       - ATR_REQ Response: n bytes*/
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
        }

        if(NFCSTATUS_SUCCESS == status)
        {
            /* Update the activated remote device type as P2P Target */
            (pRemDevInf->RemDevType) = phNciNfc_eNfcIP1_Target;

            /* Target speed (Target to initiator speed) */
            pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Datarate = (phNciNfc_BitRates_t)pRemDevInf->bTransBitRate;
            switch(pRemDevInf->eRFTechMode)
            {
                case phNciNfc_NFCA_Active_Poll:
                    pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active = 1; /* Active communciation */
                break;
                default:
                    pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active = 0; /* Passive communciation */
                break;
            }

            /* Update gpphNciNfc_RdrDataXchgSequence with the appropriate functions to be called
                   by sequence handler on invocation during data exchange */
            /* Obtain the length of Activation parameters from pBuff */
            bCount += RfTechSpecParamsLen + PH_NCINFCTYPES_DATA_XCHG_PARAMS_LEN;
            ActvnParamsLen = pBuff[bCount];
            if (0 != ActvnParamsLen)
            {
                bCount++;
                AtrLen = pBuff[bCount];
            }

            /* Holds ATR_RES if remote device is a P2P target and ATR_REQ if remote device is a
               P2P initiator */
            /* The relevant ATR Length is present in the first activation parameter.
               In the case of NCI 2.0, we ignore the Data Exchange Length Reduction parameter for now */
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

            /* In NCI1.0 NFC-DEP params are stored in Activation Parameters
            In NCI2.0 NFC-DEP params are stored in:
            - RF Technology Specific Parameters in case of Active P2P
            - Activation Parameters in case of Passive P2P
            If using Active Communication Mode the RF_INTF_ACTIVATED_INTF SHALL NOT include any
            Activation Parameters.
            */
            if(0 != ActvnParamsLen && pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active == 1 &&
               phNciNfc_IsVersion2x(phNciNfc_GetContext()))
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
                PH_LOG_NCI_INFO_STR(" Invalid Params..");
            }

            /* If Activated device is P2P Target. Only 'Transreceive' function shall be active
               The P2P Initiator (i.e., us) is expected to send data to remote P2P target and expected
               to receive data from Remote P2P target */
            /* Register SendData and ReceiveData P2P functions with DataXchg SequenceHandler */
            gpphNciNfc_RdrDataXchgSequence[0].SequnceInitiate = &phNciNfc_P2P_TransceiveSend;
            gpphNciNfc_RdrDataXchgSequence[0].SequenceProcess = &phNciNfc_P2P_TransceiveReceive;
            gpphNciNfc_RdrDataXchgSequence[1].SequnceInitiate = NULL;
            gpphNciNfc_RdrDataXchgSequence[1].SequenceProcess = &phNciNfc_CompleteDataXchgSequence;
        }
    }
    else
    {
        status = NFCSTATUS_FAILED;
        PH_LOG_NCI_INFO_STR(" Invalid Params..");
     }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_NfcDepPollRdrFInit(
                           pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                           uint8_t *pBuff,
                           uint16_t wLen)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    uint8_t   *pRfNtfBuff;
    uint8_t   RfTechSpecParamsLen;
    uint8_t   SensFResRespLen;
    uint8_t   ActvnParamsLen = 0;
    uint8_t   AtrLen = 0;
    uint8_t   bCount = 7;

    PH_LOG_NCI_FUNC_ENTRY();
    if( (0 != (wLen)) && (NULL != pBuff) && (NULL != pRemDevInf))
    {
        /* Remote device is a P2P target */
        pRemDevInf->RemDevType = phNciNfc_eNfcIP1_Target;

        /* Obtain the len of RF tech specific parameters from Resp buff */
        RfTechSpecParamsLen = pBuff[6];
        pRfNtfBuff = &pBuff[bCount];

        /* Clear the information */
        pRemDevInf->tRemoteDevInfo.NfcIP_Info.SensResLength = 0;
        pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Datarate = phNciNfc_e_BitRate212;
        pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID_Length = 0;
        if(0 != RfTechSpecParamsLen)
        {
            if((0 == *(pRfNtfBuff+0)) || (phNciNfc_e_BitRate848 <= (*(pRfNtfBuff+0))))
            {
                status = NFCSTATUS_FAILED;
                PH_LOG_NCI_WARN_STR(" Invalid BitRate!");
            }
            else
            {
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Datarate = (phNciNfc_BitRates_t)*(pRfNtfBuff+0);
                SensFResRespLen = *(pRfNtfBuff+1);
                pRemDevInf->tRemoteDevInfo.NfcIP_Info.SensResLength = SensFResRespLen;

                if(((0 != SensFResRespLen) && ((PH_NCINFCTYPES_MIN_SENF_LEN == SensFResRespLen) ||
                    (PH_NCINFCTYPES_MAX_SENSF_LEN == SensFResRespLen))) ||
                   ((0 == SensFResRespLen) && (pRemDevInf->eRFTechMode == phNciNfc_NFCF_Active_Poll)))
                {
                    phOsalNfc_SetMemory(&(pRemDevInf->tRemoteDevInfo.NfcIP_Info.SensRes),0,
                                        PH_NCINFCTYPES_SENS_RES_LEN);
                    phOsalNfc_MemCopy(&(pRemDevInf->tRemoteDevInfo.NfcIP_Info.SensRes),
                        (pRfNtfBuff+2),SensFResRespLen);

                    /* Get NFCID3 */
                    pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID_Length = (SensFResRespLen != 0) ? PH_NCINFCTYPES_NFCID2_LEN : 0;
                    phOsalNfc_MemCopy(pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID,
                                    &(pRemDevInf->tRemoteDevInfo.NfcIP_Info.SensRes),
                                    pRemDevInf->tRemoteDevInfo.NfcIP_Info.NFCID_Length);

                    /* Target speed (Target to initiator speed) */
                    switch(pRemDevInf->eRFTechMode)
                    {
                    case phNciNfc_NFCF_Active_Poll:
                        pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active = 1; /* Active communciation */
                    break;
                    default:
                        pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Active = 0; /* Passive communciation */
                    break;
                    }
                }
                else
                {
                    status = NFCSTATUS_FAILED;
                    PH_LOG_NCI_WARN_STR(" Invalid SensFResResp");
                }
            }
        }
        if(NFCSTATUS_SUCCESS == status)
        {
            /* Target speed (Target to initiator speed) */
            pRemDevInf->tRemoteDevInfo.NfcIP_Info.Nfcip_Datarate = (phNciNfc_BitRates_t)pRemDevInf->bTransBitRate;
            /* Obtain the length of Activation parameters from pBuff */
            bCount += RfTechSpecParamsLen + PH_NCINFCTYPES_DATA_XCHG_PARAMS_LEN;
            ActvnParamsLen = pBuff[bCount];

            /* The relevant ATR Length is present in the first activation parameter.
               In the case of NCI 2.0, we ignore the Data Exchange Length Reduction parameter for now */
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
            /* If Activated device is a P2P Target. Only 'Transreceive' function shall be active
               The P2P Initiator (i.e., us) is expected to send data to remote P2P target and expected
               to receive data from Remote P2P target */
            /* Register SendData and ReceiveData P2P functions with DataXchg SequenceHandler */
            gpphNciNfc_RdrDataXchgSequence[0].SequnceInitiate = &phNciNfc_P2P_TransceiveSend;
            gpphNciNfc_RdrDataXchgSequence[0].SequenceProcess = &phNciNfc_P2P_TransceiveReceive;
            gpphNciNfc_RdrDataXchgSequence[1].SequnceInitiate = NULL;
            gpphNciNfc_RdrDataXchgSequence[1].SequenceProcess = &phNciNfc_CompleteDataXchgSequence;
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

NFCSTATUS
phNciNfc_P2P_TransceiveReceive(void *psContext,
                              NFCSTATUS wStatus)
{
    NFCSTATUS                        wStat = wStatus;
    phNciNfc_Context_t               *pNciContext = (phNciNfc_Context_t *)psContext;
    phNciNfc_ActiveDeviceInfo_t      *pActivDev = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciContext)
    {
        wStat = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_NCI_INFO_STR(" Invalid Context Param!");
    }
    else
    {
        pActivDev = (pphNciNfc_ActiveDeviceInfo_t)&pNciContext->tActvDevIf;

        if((0 == pNciContext->RspBuffInfo.wLen) || (NFCSTATUS_SUCCESS != wStat))
        {
            PH_LOG_NCI_INFO_STR("Data Receive Failed!");
        }
        else
        {
            PH_LOG_NCI_INFO_STR("Data received successfully");
            if(NULL != pNciContext->RspBuffInfo.pBuff)
            {
                if(pNciContext->RspBuffInfo.wLen <= pNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen)
                {
                    phOsalNfc_MemCopy(pNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff,
                        pNciContext->RspBuffInfo.pBuff,pNciContext->RspBuffInfo.wLen);
                    pNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen = pNciContext->RspBuffInfo.wLen;
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Output buffer given from upper layer is \
                                    not succifient!");
                    wStat = NFCSTATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Invalid response buffer!");
                wStat = NFCSTATUS_FAILED;
            }
        }
        /* Received data has already been updated in the user sent Receive buffer as the same buffer was used
           for storing received message's payload */
        /* De-allocate memory allocated for send buffer */
        if(NULL != (pNciContext->tTranscvCtxt.tSendPld.pBuff))
        {
            PH_LOG_NCI_INFO_STR("De-allocating Send Request Payload Buffer...");
            phOsalNfc_FreeMemory(pNciContext->tTranscvCtxt.tSendPld.pBuff);
            pNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
            pNciContext->tTranscvCtxt.tSendPld.wLen = 0;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStat;
}

NFCSTATUS
phNciNfc_P2P_TransceiveSend(void   *psContext)
{
    NFCSTATUS               wStat = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreTxInfo_t   TxInfo;
    uint16_t                wPldDataSize = 0;
    pphNciNfc_ActiveDeviceInfo_t  pActivDev = NULL;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciContext))
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameter (Nci Context)!");
    }
    else
    {
        pActivDev = (pphNciNfc_ActiveDeviceInfo_t)&pNciContext->tActvDevIf;

        if((0 != pNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen) &&
           (NULL != pNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff))
        {
            /* Fill the data packet details into TxInfo */
            TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            wStat = phNciNfc_GetConnId((void *)pActivDev->pDevInfo, &(TxInfo.tHeaderInfo.bConn_ID));

            if(NFCSTATUS_SUCCESS == wStat)
            {
                wPldDataSize = (pNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen);
                /* Check for any allocated memory */
                /* Allocate memory for the send buffer */
                (pNciContext->tTranscvCtxt.tSendPld.pBuff) = (uint8_t *)phOsalNfc_GetMemory(wPldDataSize);
                (pNciContext->tTranscvCtxt.tSendPld.wLen) = 0;

                if(NULL != (pNciContext->tTranscvCtxt.tSendPld.pBuff))
                {
                    (pNciContext->tTranscvCtxt.tSendPld.wLen) = (wPldDataSize);
                    phOsalNfc_SetMemory((pNciContext->tTranscvCtxt.tSendPld.pBuff),0,
                        (pNciContext->tTranscvCtxt.tSendPld.wLen));
                    /* Copy the input data to the send buffer */
                    phOsalNfc_MemCopy((pNciContext->tTranscvCtxt.tSendPld.pBuff),
                        (pNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff),wPldDataSize);

                    TxInfo.Buff = (pNciContext->tTranscvCtxt.tSendPld.pBuff);
                    TxInfo.wLen = (pNciContext->tTranscvCtxt.tSendPld.wLen);
                    /* Send data to remote device (P2P Target) */
                    wStat = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext), &TxInfo,
                                &(pNciContext->RspBuffInfo), 0,
                                (pphNciNfc_CoreIfNtf_t)&phNciNfc_RdrDataXchgSequence, pNciContext);
                    if(NFCSTATUS_PENDING != wStat)
                    {
                        phOsalNfc_FreeMemory(pNciContext->tTranscvCtxt.tSendPld.pBuff);
                        pNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
                        pNciContext->tTranscvCtxt.tSendPld.wLen = 0;
                    }
                    /* No need to register for any data message as we are using Sequence handler in
                       P2P Initiator mode. After sendig data to remote P2P target, when a response data
                       message is received, the sequence handler shall invoke 'phNciNfc_P2P_TransreceiveReceive'
                       function */
                }
                else
                {
                    wStat = NFCSTATUS_INSUFFICIENT_RESOURCES;
                    PH_LOG_NCI_CRIT_STR("Payload MemAlloc for Send request Failed!");
                }
            }
            else
            {
                wStat = NFCSTATUS_FAILED;
                PH_LOG_NCI_CRIT_STR(" Couldn't Get ConnId!");
            }
        }
        else
        {
            wStat = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("Send Data Buff not valid!");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStat;
}
