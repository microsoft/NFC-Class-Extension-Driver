/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_RFReaderFelica.h"

#include "phNciNfc_RFReaderFelica.tmh"

#define PHNCINFC_T3T_CMD_PAYLOAD_LEN        (0X04)
#define PHNCINFC_MAX_TIMESLOT_NUM           (0x0FU)

static NFCSTATUS phNciNfc_ValidateT3TReqParams(pphNciNfc_SensFReqParams_t  pPollReqParams);
static NFCSTATUS phNciNfc_ValidateGidOid(uint8_t bGid, uint8_t bOid);

static NFCSTATUS phNciNfc_UpdateRemDevInf(void *psContext, pphNciNfc_TransactInfo_t pTransInfo);

static NFCSTATUS phNciNfc_T3TPollResp(void *psContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_T3TPollNtf(void* psContext, void *pInfo, NFCSTATUS status);

static NFCSTATUS phNciNfc_SendT3TPollCmd(void *psContext);
static NFCSTATUS phNciNfc_CompleteT3TPollSequence(void *pContext, NFCSTATUS wStatus);

static void phNciNfc_T3tNtfTimeoutHandler(uint32_t TimerId, void *pContext);

phNciNfc_SequenceP_t gphNciNfc_T3TSequence[] = {
    {&phNciNfc_SendT3TPollCmd, &phNciNfc_T3TPollResp},
    {NULL, &phNciNfc_CompleteT3TPollSequence}
};

NFCSTATUS
phNciNfc_RdrFInit(
                   pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                   uint8_t *pBuff,
                   uint16_t wLen
                 )
{
    NFCSTATUS                   wStatus = NFCSTATUS_SUCCESS;
    uint8_t                     *pRfNtfBuff;
    uint8_t                     SensFResRespLen;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (0 != (wLen)) && (NULL != pBuff) && (NULL != pRemDevInf))
    {
        /* Detect and update the remote device type */
        if(phNciNfc_NFCF_Poll == pRemDevInf->eRFTechMode)
        {
            /* Techn and mode is Poll Nfc-F */
            if((phNciNfc_e_RfInterfacesNFCDEP_RF == pRemDevInf->eRfIf) &&
               (phNciNfc_e_RfProtocolsNfcDepProtocol == pRemDevInf->eRFProtocol))
            {
                /* Remote devuce is a P2P target */
                pRemDevInf->RemDevType = phNciNfc_eNfcIP1_Target;
            }
            else
            {
                pRemDevInf->RemDevType = phNciNfc_eFelica_PICC;
            }
        }
        else /* phNciNfc_NFCF_Listen */
        {
            /* Techn and mode is Listen Nfc-F */
            if((phNciNfc_e_RfInterfacesNFCDEP_RF == pRemDevInf->eRfIf) &&
               (phNciNfc_e_RfProtocolsNfcDepProtocol == pRemDevInf->eRFProtocol))
            {
                /* Remote devuce is a P2P Initiator */
                pRemDevInf->RemDevType = phNciNfc_eNfcIP1_Initiator;
            }
            else
            {
                pRemDevInf->RemDevType = phNciNfc_eFelica_PICC;
            }
        }

        /* Obtain the len of RF tech specific parameters from Resp buff */
        pRfNtfBuff = &pBuff[7];

        if(phNciNfc_eFelica_PICC == pRemDevInf->RemDevType)
        {
            (pRemDevInf->tRemoteDevInfo.Felica_Info.bBitRate) = *(pRfNtfBuff+0);

            if((0 == (pRemDevInf->tRemoteDevInfo.Felica_Info.bBitRate)) || (3 <= (pRemDevInf->tRemoteDevInfo.
                Felica_Info.bBitRate)))
            {
                wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                PH_LOG_NCI_INFO_STR(" Invalid BitRate..");
            }
            else
            {
                SensFResRespLen = *(pRfNtfBuff+1);
                (pRemDevInf->tRemoteDevInfo.Felica_Info.bSensFRespLen) = SensFResRespLen;

                if((0 != SensFResRespLen) && ((PH_NCINFCTYPES_MIN_SENF_LEN == SensFResRespLen) ||
                    (PH_NCINFCTYPES_MAX_SENSF_LEN == SensFResRespLen)))
                {
                    phOsalNfc_SetMemory(&(pRemDevInf->tRemoteDevInfo.Felica_Info.aSensFResp),
                            0,PH_NCINFCTYPES_MAX_SENSF_LEN);
                    phOsalNfc_MemCopy(&(pRemDevInf->tRemoteDevInfo.Felica_Info.aSensFResp),
                        (pRfNtfBuff+2),SensFResRespLen);
                }
                else
                {
                    /* not updating status to Failed,because this might be covered up by user calling
                       T3T Poll Req Cmd to get SENS_RES */
                    /*status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED); */
                    PH_LOG_NCI_INFO_STR(" SensFResResp.not present.");
                }
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Invalid remote device!");
            wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
        }

        if((NFCSTATUS_SUCCESS == wStatus) && (phNciNfc_eFelica_PICC == pRemDevInf->RemDevType))
        {
            gpphNciNfc_RdrDataXchgSequence[0].SequnceInitiate = &phNciNfc_SendFelicaReq;
            gpphNciNfc_RdrDataXchgSequence[0].SequenceProcess = &phNciNfc_RecvFelicaResp;
            gpphNciNfc_RdrDataXchgSequence[1].SequnceInitiate = NULL;
            gpphNciNfc_RdrDataXchgSequence[1].SequenceProcess = &phNciNfc_CompleteDataXchgSequence;
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Sequence operations not set!");
        }
    }
    else
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Params..");
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

static
NFCSTATUS
phNciNfc_T3TPollNtf(
                       void*      psContext,
                       void       *pInfo,
                       NFCSTATUS  status
        )
{
    phNciNfc_Context_t *psNciContext = (phNciNfc_Context_t *)psContext;
    pphNciNfc_TransactInfo_t pTransInfo = (pphNciNfc_TransactInfo_t)pInfo;
    phNciNfc_CoreRegInfo_t tNtfInfo;
    pphNciNfc_IfNotificationCb_t pT3tNtfCb;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        (void)phOsalNfc_Timer_Stop(psNciContext->dwNtfTimerId);
        if( (NULL == pTransInfo) ||\
            (NULL == pTransInfo->pbuffer) ||(0 == pTransInfo->wLength)
            || (PH_NCINFC_STATUS_OK != status)
            )
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" T3T Notification invalid..");
        }
        else
        {
            /* DeRegister T3T Poll notification */
            tNtfInfo.bGid = phNciNfc_e_CoreRfMgtGid;
            tNtfInfo.bOid = phNciNfc_e_RfMgtRfT3tPollingNtfOid;
            tNtfInfo.pContext = (void *)psNciContext;
            tNtfInfo.pNotifyCb = (pphNciNfc_CoreIfNtf_t)&phNciNfc_T3TPollNtf;

            (void)phNciNfc_CoreRecvMgrDeRegisterCb((void*)&psNciContext->NciCoreContext,
                                &tNtfInfo,phNciNfc_e_NciCoreMsgTypeCntrlNtf);

            status = pTransInfo->pbuffer[0];

            if(PH_NCINFC_STATUS_OK == status)
            {
                status = phNciNfc_UpdateRemDevInf(psContext,pTransInfo);
            }
            else
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                PH_LOG_NCI_INFO_STR(" T3T Resp Not Recvd..");
            }
        }

        if(NULL != psNciContext->tRegSyncInfo.pT3tNtfCb)
        {
            pT3tNtfCb = psNciContext->tRegSyncInfo.pT3tNtfCb;
            psNciContext->tRegSyncInfo.pT3tNtfCb = NULL;
            (void)pT3tNtfCb(psNciContext->tRegSyncInfo.T3tNtfCtxt,\
                        status,NULL);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return 0;
}

NFCSTATUS
phNciNfc_T3TPollReq(
                         void   *psContext,
                         pphNciNfc_SensFReqParams_t  pPollReqParams,
                         pphNciNfc_IfNotificationCb_t pNotify,
                         void *pContext
                         )
{
    NFCSTATUS    wStatus = NFCSTATUS_FAILED;
    uint8_t *pTargetInfo;
    uint8_t bIndex = 0;
    phNciNfc_Context_t *psNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if((NULL == pPollReqParams) || (NULL == pNotify))
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid T3T Poll Req Param(s)..");
    }
    else
    {
        wStatus = phNciNfc_ValidateT3TReqParams(pPollReqParams);

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            pTargetInfo = \
                (uint8_t *)phOsalNfc_GetMemory((uint32_t)PHNCINFC_T3T_CMD_PAYLOAD_LEN);
            if(NULL != pTargetInfo)
            {
                pTargetInfo[bIndex++] = pPollReqParams->bSysCode[0];
                pTargetInfo[bIndex++] = pPollReqParams->bSysCode[1];
                pTargetInfo[bIndex++] = pPollReqParams->bReqCode;
                pTargetInfo[bIndex++] = pPollReqParams->bTimeSlotNum;
                psNciContext->tSendPayload.pBuff = pTargetInfo;
                psNciContext->tSendPayload.wPayloadSize = PHNCINFC_T3T_CMD_PAYLOAD_LEN;
                PHNCINFC_INIT_SEQUENCE(psNciContext, gphNciNfc_T3TSequence);

                wStatus = phNciNfc_GenericSequence((void *)psNciContext,NULL,NFCSTATUS_SUCCESS);

                if(NFCSTATUS_PENDING == wStatus)
                {
                    phNciNfc_SetUpperLayerCallback(psNciContext, pNotify, pContext);
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("T3t poll Sequence failed!");
                    phNciNfc_FreeSendPayloadBuff(psNciContext);
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

static
NFCSTATUS
phNciNfc_SendT3TPollCmd (
                            void     *psContext
                        )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    phNciNfc_CoreTxInfo_t   tCmdInfo;
    phNciNfc_Context_t *psNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciContext)
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        PH_LOG_NCI_INFO_STR(" Setting up commandInfo to be sent to lower layer ..");

        tCmdInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        tCmdInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
        tCmdInfo.tHeaderInfo.Opcode_ID.OidType.RfMgtCmdOid = phNciNfc_e_RfMgtRfT3tPollingCmdOid;

        tCmdInfo.Buff = psNciContext->tSendPayload.pBuff;
        tCmdInfo.wLen = psNciContext->tSendPayload.wPayloadSize;

        /* Sending Command to Nci Core */
        status = phNciNfc_CoreIfTxRx(&(psNciContext->NciCoreContext), &tCmdInfo,
            &(psNciContext->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,
            (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence, psContext);
    }

    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS
phNciNfc_T3TPollResp(
                        void                *psContext,
                        NFCSTATUS          wStatus
                       )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t          *psNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciContext)
    {
      status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
      PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if( (0 == psNciContext->RspBuffInfo.wLen)
            || (PH_NCINFC_STATUS_OK != wStatus)
            || (NULL == (psNciContext->RspBuffInfo.pBuff))
            )
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" T3T Poll Req Failed ..");
        }
        else
        {
            PH_LOG_NCI_INFO_STR(" T3TPoll Req sent successfully..");
        }
        (psNciContext->tTranscvCtxt.wPrevStatus) = status;
    }

    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS
phNciNfc_CompleteT3TPollSequence(void *pContext,
                                 NFCSTATUS status
                                 )
{
    NFCSTATUS wStatus = status;
    phNciNfc_Context_t     *psNciContext = (phNciNfc_Context_t *)pContext ;
    phNciNfc_sCoreHeaderInfo_t tRegInfo;
    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if(NULL != psNciContext->tSendPayload.pBuff)
        {
            PH_LOG_NCI_INFO_STR(" Freeing T3TPollReq Buff..");
            phNciNfc_FreeSendPayloadBuff(psNciContext);
        }

        if(NFCSTATUS_SUCCESS != wStatus)
        {
            PH_LOG_NCI_CRIT_STR(" Error in T3TPollReq handling ");
            (void)phNciNfc_Notify(psNciContext, wStatus,NULL);
        }
        else
        {
            /* Registering Iso-Dep pres chk notification */
            psNciContext->tRegSyncInfo.pT3tNtfCb = psNciContext->IfNtf;
            psNciContext->tRegSyncInfo.T3tNtfCtxt = psNciContext->IfNtfCtx;
            tRegInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            tRegInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
            tRegInfo.Opcode_ID.Val = phNciNfc_e_RfMgtRfT3tPollingNtfOid;
            wStatus = phNciNfc_CoreIfRegRspNtf((void*)&psNciContext->NciCoreContext,
                                   &tRegInfo,
                                   (pphNciNfc_CoreIfNtf_t)&phNciNfc_T3TPollNtf,
                                   (void *)psNciContext);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_INFO_STR("T3T Ntf Registration failed");
            }
            else
            {
                PH_LOG_NCI_INFO_STR("T3T Ntf successfully Registered");
                if(PH_OSALNFC_TIMER_ID_INVALID != psNciContext->dwNtfTimerId)
                {
                    wStatus = phOsalNfc_Timer_Start(psNciContext->dwNtfTimerId,
                                                    PH_NCINFC_NTF_TIMEROUT,
                                                    &phNciNfc_T3tNtfTimeoutHandler,
                                                    (void *) psNciContext);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        PH_LOG_NCI_INFO_STR("T3T Ntf timer started");
                    }
                    else
                    {
                        PH_LOG_NCI_CRIT_STR("T3T Ntf timer start failed");
                    }
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS
phNciNfc_SendFelicaReq(
                              void   *psContext
                         )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t   TxInfo;
    uint8_t                 bSodByte = 0;
    uint8_t                 bPldDataSize = 0;

    phNciNfc_Context_t *psNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if(NULL == (psNciContext->tActvDevIf.pDevInfo))
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
        PH_LOG_NCI_INFO_STR(" Invalid Device..");
    }
    else
    {
        pphNciNfc_RemoteDevInformation_t  pActivDev = NULL;

        pActivDev = (psNciContext->tActvDevIf.pDevInfo);

        bPldDataSize = (uint8_t) (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen);
        bSodByte = (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff[0]);

        /* as per DIGITAL, SOD byte value should be between 1 & 254 */
        if((0 != bPldDataSize) && ((1 <= bSodByte) && (254 >= bSodByte)))
        {
            /* Fill the data packet details into TxInfo */
            TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            status = phNciNfc_GetConnId(pActivDev, &(TxInfo.tHeaderInfo.bConn_ID));

            if(NFCSTATUS_SUCCESS == status)
            {
                TxInfo.Buff = (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff);
                TxInfo.wLen = (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen);
                /* Call Sequence handler to send data to lower layer */
                status = phNciNfc_CoreIfTxRx(&(psNciContext->NciCoreContext), &TxInfo,
                    &(psNciContext->RspBuffInfo), psNciContext->tTranscvCtxt.tTranscvInfo.wTimeout,
                    (pphNciNfc_CoreIfNtf_t)&phNciNfc_RdrDataXchgSequence, psContext);
                /* Clear the timeout value so that it wont be used mistakenly in subsequent transceive */
                psNciContext->tTranscvCtxt.tTranscvInfo.wTimeout = 0;
            }
            else
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                PH_LOG_NCI_INFO_STR(" Couldn't Get ConnId..");
            }
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" Send Data Buff not valid..");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_RecvFelicaResp(
                        void                *psContext,
                        NFCSTATUS           wStatus
                       )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    uint16_t                wRecvDataSz = 0;
    phNciNfc_Context_t      *psNciContext = (phNciNfc_Context_t *)psContext;
    uint16_t CopyLen = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciContext)
    {
      status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
      PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if((0 == (psNciContext->RspBuffInfo.wLen))
                || (PH_NCINFC_STATUS_OK != wStatus)
                || (NULL == (psNciContext->RspBuffInfo.pBuff))
                )
        {
            /* NOTE:- ( TODO) In this fail scenario,map the status code from response handler
            to the status code in Nci Context and use the same in the status below */
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" Data Receive Failed..");
        }
        else
        {
            (psNciContext->tTranscvCtxt.tRecvPld.wLen) = 0;
            /* Excluding the Status byte */
            (psNciContext->tTranscvCtxt.tRecvPld.wLen) = ((psNciContext->RspBuffInfo.wLen)-1);

            /* If received Rsp contains data apart from lastly appended status byte */
            if(1 != (psNciContext->tTranscvCtxt.tRecvPld.wLen))
            {
                wRecvDataSz = (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen);

                if((psNciContext->tTranscvCtxt.tRecvPld.wLen) > wRecvDataSz)
                {
                    CopyLen = wRecvDataSz;
                    status = NFCSTATUS_SUCCESS; /* Change to this if handling is added NFCSTATUS_MORE_INFORMATION; */
                }
                else
                {
                    CopyLen = (psNciContext->tTranscvCtxt.tRecvPld.wLen);
                    status = NFCSTATUS_SUCCESS;
                }

                /* Extract the data part from pBuff & fill it to be sent to upper layer */
                phOsalNfc_MemCopy((psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff),
                    (psNciContext->RspBuffInfo.pBuff),CopyLen);

                /* update the number of bytes received from lower layer,excluding the status byte */
                (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen) = CopyLen;
            }
            else
            {
                status = NFCSTATUS_SUCCESS;
                PH_LOG_NCI_INFO_STR(" Only Status Byte received in Resp..");
                /* update the number of bytes received from lower layer,excluding the status byte */
                (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen) = 0;
            }
        }
        (psNciContext->tTranscvCtxt.wPrevStatus) = status;
    }

    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS
phNciNfc_ValidateT3TReqParams(pphNciNfc_SensFReqParams_t  pPollReqParams)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();

    if (((pPollReqParams->bReqCode) > 2) ||\
        ((pPollReqParams->bTimeSlotNum) > PHNCINFC_MAX_TIMESLOT_NUM) )
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid SystemCode/RequestCode/TSN Specified..");
    }

    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

static
NFCSTATUS phNciNfc_ValidateGidOid(uint8_t bGid,uint8_t bOid)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();

    if(phNciNfc_e_CoreRfMgtGid == bGid)
    {
        if(phNciNfc_e_RfMgtRfT3tPollingNtfOid == bOid)
        {
            PH_LOG_NCI_INFO_STR(" Valid T3T Poll Ntf Received..");
        }
        else
        {
            /* NOTE:- (TODO) return the exact error status code for invalid OID */
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
            PH_LOG_NCI_INFO_STR(" Invalid Oid received..");
        }
    }
    else
    {
        /* NOTE:- (TODO) return the exact error status code for invalid GID */
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Gid received..");
    }

    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS phNciNfc_UpdateRemDevInf(void *psContext,
                                   pphNciNfc_TransactInfo_t pTransInfo
                                   )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    uint8_t                     *pRfNtfBuff;
    uint8_t                     SensFResRespLen;
    phNciNfc_Context_t *psNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if((NULL != pTransInfo) && (NULL != pTransInfo->pbuffer) &&
                (0 != pTransInfo->pbuffer[1]))
        {
            pRfNtfBuff = &(pTransInfo->pbuffer[2]);

            SensFResRespLen = *(pRfNtfBuff+0);
            (psNciContext->tActvDevIf.pDevInfo->tRemoteDevInfo.Felica_Info.bSensFRespLen) =
                SensFResRespLen;

            if((0 != SensFResRespLen) && ((PH_NCINFCTYPES_MIN_SENF_LEN == SensFResRespLen) ||
            (PH_NCINFCTYPES_MAX_SENSF_LEN == SensFResRespLen)))
            {
                phOsalNfc_SetMemory(&(psNciContext->tActvDevIf.pDevInfo->tRemoteDevInfo.Felica_Info.aSensFResp)
                    ,0,PH_NCINFCTYPES_MAX_SENSF_LEN);
                phOsalNfc_MemCopy(&(psNciContext->tActvDevIf.pDevInfo->tRemoteDevInfo.Felica_Info.aSensFResp),
                    (pRfNtfBuff+1),SensFResRespLen);
            }
            else
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                PH_LOG_NCI_INFO_STR(" SensFResResp.not present in T3T Poll Ntf recvd.");
            }
        }
    }

    PH_LOG_NCI_FUNC_EXIT();

    return status;
}
static void phNciNfc_T3tNtfTimeoutHandler(uint32_t TimerId, void *pContext)
{
    pphNciNfc_Context_t pNciContext = pContext;
    UNUSED(TimerId);
    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL != pNciContext)
    {
        (void)phOsalNfc_Timer_Stop(pNciContext->dwNtfTimerId);
         (void)phNciNfc_T3TPollNtf(pNciContext,NULL,NFCSTATUS_FAILED);
    }
    PH_LOG_NCI_FUNC_EXIT();
}
