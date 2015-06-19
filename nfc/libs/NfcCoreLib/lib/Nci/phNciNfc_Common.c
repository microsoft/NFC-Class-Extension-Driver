/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 *
 */

#include "phNciNfc_Pch.h"

#include "phNciNfc_Common.tmh"

NFCSTATUS
phNciNfc_SetConnCredentials(void *psNciContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t psNciCtxt = (pphNciNfc_Context_t )psNciContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciCtxt)
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_CRIT_STR(" Invalid Context Param..");
    }
    else
    {
        /* Update Static conn info with conn details & device handle */
        wStatus = phNciNfc_UpdateConnDestInfo(psNciCtxt->tActvDevIf.pDevInfo->bRfDiscId,
                                              phNciNfc_e_REMOTE_NFC_ENDPOINT,
                                              psNciCtxt->tActvDevIf.pDevInfo);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Conn Credentials updated successfully..");
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Conn Credentials update failed!!..");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_ProcessGenericErrNtf(void *pContext, void *pInfo, NFCSTATUS wStatus)
{
    NFCSTATUS wRetStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t )pContext;
    pphNciNfc_CoreContext_t pCtx = NULL;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    phNciNfc_NotificationInfo_t tNtfInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciCtx) && (NULL != pTransInfo))
    {
        /* Stop Notification timer */
        (void)phOsalNfc_Timer_Stop(pNciCtx->dwNtfTimerId);

        pCtx = (pphNciNfc_CoreContext_t)&pNciCtx->NciCoreContext;

        if((NULL == pTransInfo->pbuffer) ||(1 != pTransInfo->wLength)
            || (NFCSTATUS_SUCCESS != wStatus))
        {
            wRetStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("Invalid buffer or incorrect payload length or \
                                 Tml receive failed receive failed");
        }
        else
        {
            switch(pTransInfo->pbuffer[0])
            {
            case PH_NCINFC_STATUS_REJECTED:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_Rejected;
                break;
            case PH_NCINFC_STATUS_RF_FRAME_CORRUPTED:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_RfFrameCorrupted;
                break;
            case PH_NCINFC_STATUS_FAILED:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_Failed;
                break;
            case PH_NCINFC_STATUS_NOT_INITIALIZED:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_NotInitiatlized;
                break;
            case PH_NCINFC_STATUS_SYNTAX_ERROR:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_SyntaxErr;
                break;
            case PH_NCINFC_STATUS_SEMANTIC_ERROR:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_SemanticErr;
                break;
            case PH_NCINFC_STATUS_INVALID_PARAM:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_InvalidParam;
                break;
            case PH_NCINFC_STATUS_MESSAGE_SIZE_EXCEEDED:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_MsgSizeExceeded;
                break;
            case PH_NCINFC_STATUS_DISCOVERY_ALREADY_STARTED:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_DiscAlreadyStarted;
                break;
            case PH_NCINFC_STATUS_DISCOVERY_TARGET_ACTIVATION_FAILED:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_DiscTgtActvnFailed;
                pNciCtx->tRegSyncInfo.pActvNtfCb = NULL;
                break;
            case PH_NCINFC_STATUS_DISCOVERY_TEAR_DOWN:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_DiscTearDown;
                break;
            default:
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_ErrorNotDefined;
                break;
            }

            PH_LOG_NCI_CRIT_STR("Generic error received: %!phNciNfc_GenericErrCode_t!", tNtfInfo.tGenericErrInfo.eGenericErrInfo);
            pNciCtx->tRegListInfo.pGenericErrNtfCb(pNciCtx->tRegListInfo.GenericErrNtfCtxt,\
                                                            eNciNfc_NciInvalidEvt,\
                                                            &tNtfInfo,wRetStatus);
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Can not process Generic Error Ntf - Invalid input parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wRetStatus;
}

NFCSTATUS
phNciNfc_ProcessGenericErrNtfMFC(void *pContext, void *pInfo, NFCSTATUS wStatus)
{
    NFCSTATUS wRetStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t )pContext;
    pphNciNfc_CoreContext_t pCtx = NULL;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    phNciNfc_NotificationInfo_t tNtfInfo;
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciCtx) && (NULL != pTransInfo))
    {
        pCtx = (pphNciNfc_CoreContext_t)&pNciCtx->NciCoreContext;

        (void)phOsalNfc_Timer_Stop(pNciCtx->dwNtfTimerId);

        if((NULL == pTransInfo->pbuffer) ||(1 != pTransInfo->wLength)
            || (NFCSTATUS_SUCCESS != wStatus))
        {
            wRetStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("NtfMFC: Invalid buffer or incorrect payload length or \
                                 Tml receive failed receive failed");
        }
        else
        {
            if(PH_NCINFC_STATUS_DISCOVERY_TARGET_ACTIVATION_FAILED == pTransInfo->pbuffer[0])
            {
                PH_LOG_NCI_CRIT_STR("NtfMFC: Generic error received: PH_NCINFC_STATUS_DISCOVERY_TARGET_ACTIVATION_FAILED");
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_DiscTgtActvnFailed;
                phNciNfc_Notify(pNciCtx,NFCSTATUS_FAILED,NULL);
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("NtfMFC: Generic error received: Unknown error code");
                tNtfInfo.tGenericErrInfo.eGenericErrInfo = phNciNfc_e_ErrorNotDefined;
            }

            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
            tHeaderInfo.Opcode_ID.OidType.NciCoreNtfOid = phNciNfc_e_NciCoreGenericErrNtfOid;

            (void)phNciNfc_CoreIfUnRegRspNtf(&(pNciCtx->NciCoreContext),
                             &tHeaderInfo,
                             (pphNciNfc_CoreIfNtf_t)&phNciNfc_ProcessGenericErrNtfMFC);
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("NtfMFC: Can not process Generic Error Ntf - Invalid input parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wRetStatus;
}

NFCSTATUS
phNciNfc_ProcessIntfErrNtf(void *pContext, void *pInfo, NFCSTATUS wStatus)
{
    NFCSTATUS wActStatus = NFCSTATUS_FAILED;
    NFCSTATUS wRetStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t )pContext;
    pphNciNfc_CoreContext_t pCtx = NULL;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    phNciNfc_EvtRecv_t TrigEvt = phNciNfc_EvtRecvTimeOut;
    uint8_t bConnId = PHNCINFC_CORE_MAX_CONNID_VALUE;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciCtx) && (NULL != pTransInfo))
    {
        pCtx = (pphNciNfc_CoreContext_t)&pNciCtx->NciCoreContext;

        if((NULL == pTransInfo->pbuffer) ||(2 != pTransInfo->wLength)
            || (NFCSTATUS_SUCCESS != wStatus))
        {
            wRetStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("IntfErrNtf: Invalid buffer or incorrect payload length or \
                                 Tml receive failed receive failed");
        }
        else
        {
            if((pTransInfo->pbuffer[0] == PH_NCINFC_STATUS_RF_TRANSMISSION_ERROR) ||
                (pTransInfo->pbuffer[0] == PH_NCINFC_STATUS_RF_PROTOCOL_ERROR) ||
                (pTransInfo->pbuffer[0] == PH_NCINFC_STATUS_RF_TIMEOUT_ERROR) ||
                (pTransInfo->pbuffer[0] == PH_NCINFC_STATUS_NFCEE_INTERFACE_ACTIVATION_FAILED) ||
                (pTransInfo->pbuffer[0] == PH_NCINFC_STATUS_NFCEE_TRANSMISSION_ERROR) ||
                (pTransInfo->pbuffer[0] == PH_NCINFC_STATUS_NFCEE_PROTOCOL_ERROR) ||
                (pTransInfo->pbuffer[0] == PH_NCINFC_STATUS_NFCEE_TIMEOUT_ERROR))
            {
                bConnId = pTransInfo->pbuffer[1];
                PH_LOG_NCI_CRIT_U32MSG("Interface err received over connection ID",bConnId);
                if(bConnId <= PHNCINFC_CORE_MAX_CONNID_VALUE)
                {
                    /* Translating the error */
                    wActStatus = NFCSTATUS_RF_ERROR;
                }
            }
            if((NFCSTATUS_RF_ERROR == wActStatus) && (1 == pCtx->TimerInfo.TimerStatus))
            {
                /* Check if Timer was started after sending data packet */
                if(phNciNfc_e_NciCoreMsgTypeData == pCtx->TimerInfo.PktHeaderInfo.eMsgType)
                {
                    PH_LOG_NCI_CRIT_STR("Deleting response timer");
                    /* If connection id present in TimerInfo matchs with the connection
                    id received with interface error ntf, stop the timer */
                    if(bConnId == pCtx->TimerInfo.PktHeaderInfo.bConn_ID)
                    {
                        (void)phOsalNfc_Timer_Stop(pCtx->TimerInfo.dwRspTimerId);
                        pCtx->TimerInfo.TimerStatus = 0;
                    }
                }
            }
            if(NFCSTATUS_RF_ERROR == wActStatus)
            {
                /* Reset CoreSend StateMachine */
                phNciNfc_CoreResetSendStateMachine(pCtx);
                /* Update Receive info with the received messages header */
                pCtx->tReceiveInfo.HeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
                pCtx->tReceiveInfo.HeaderInfo.bConn_ID = bConnId;
                pCtx->RecvStateContext.wProcessStatus = wActStatus;
                PH_LOG_NCI_CRIT_STR("Interface err received, initiating RecvStateHandler with 'NFCSTATUS_RF_ERROR' status");
                (void)phNciNfc_RecvStateHandler(pCtx, TrigEvt);
            }
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("IntfErrNtf: Can not process Intf Error Ntf - Invalid input parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wRetStatus;
}

NFCSTATUS
phNciNfc_DummyReadReq(void *pNciContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS,wIntStatus;
    pphNciNfc_Context_t psNciCtxt = (pphNciNfc_Context_t )pNciContext;
    uint8_t bConnId = 0;
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != psNciCtxt)
    {
        /* Get logical connection ID for the activated interface */
        wIntStatus = phNciNfc_GetConnId(psNciCtxt->tActvDevIf.pDevInfo, &bConnId);
        if(NFCSTATUS_SUCCESS == wIntStatus)
        {
            /* To avoid memory leaks, perform this check */
            if(NULL != psNciCtxt->tLstnModeRecvInfo.pBuff)
            {
                phOsalNfc_FreeMemory(psNciCtxt->tLstnModeRecvInfo.pBuff);
                psNciCtxt->tLstnModeRecvInfo.pBuff = NULL;
                psNciCtxt->tLstnModeRecvInfo.wBuffSize = 0;
                psNciCtxt->tLstnModeRecvInfo.wLstnCbStatus = 0;
            }
            psNciCtxt->tLstnModeRecvInfo.bDataBuffEnable = 0;
            tHeaderInfo.bEnabled = PHNCINFC_ENABLE_AUTO_DEREG;
            tHeaderInfo.bConn_ID = bConnId;
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            /* Register for Data message (with auto de-register enabled) */
            wIntStatus = phNciNfc_CoreIfRegRspNtf(&(psNciCtxt->NciCoreContext),
                                                &(tHeaderInfo),
                                                &phNciNfc_ReceiveDataBufferCb,
                                                psNciCtxt);
            if(NFCSTATUS_SUCCESS != wIntStatus)
            {
                PH_LOG_NCI_CRIT_STR("Dummy receive call back registration failed!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Get Logical Conn ID failed!");
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_ReceiveDataBufferCb(void* pContext, void *pInfo, NFCSTATUS wStatus)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciContext = pContext;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    uint16_t wDataLen = 0;
    uint8_t *pBuff = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        pNciContext->tLstnModeRecvInfo.bDataBuffEnable = 1;
        pNciContext->tLstnModeRecvInfo.wLstnCbStatus = wStatus;
        pNciContext->tLstnModeRecvInfo.pBuff = NULL;
        pNciContext->tLstnModeRecvInfo.wBuffSize = 0;

        wDataLen = (uint16_t)pTransInfo->wLength;
        if((NFCSTATUS_SUCCESS == wStatus) && (0 != wDataLen) &&
            (NULL != pTransInfo->pbuffer))
        {
            wDataLen = (uint16_t)pTransInfo->wLength;

            /* Allocate memory for temporarily storing the data received */
            pBuff = (uint8_t *) phOsalNfc_GetMemory(wDataLen);
            if(NULL != pBuff)
            {
                pNciContext->tLstnModeRecvInfo.pBuff = pBuff;
                pNciContext->tLstnModeRecvInfo.wBuffSize = wDataLen;
                phOsalNfc_MemCopy(pNciContext->tLstnModeRecvInfo.pBuff,
                    pTransInfo->pbuffer, wDataLen);
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Failed to allocate temporary buffer for storing the received data message");
            }
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Data receive failed!");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return status;
}

NFCSTATUS phNciNfc_GenericSequence(void *pNciCtx, void *pInfo, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pCtx = pNciCtx;
    pphNciNfc_TransactInfo_t pTransactInfo = pInfo;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCtx && NULL != pInfo)
    {
        pCtx->RspBuffInfo.pBuff = pTransactInfo->pbuffer;
        pCtx->RspBuffInfo.wLen = pTransactInfo->wLength;
    }
    wStatus = phNciNfc_SeqHandler(pNciCtx, Status);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void phNciNfc_FreeSendPayloadBuff(void* pContext)
{
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t )pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext->tSendPayload.pBuff)
    {
        phOsalNfc_FreeMemory(pNciContext->tSendPayload.pBuff);
        pNciContext->tSendPayload.pBuff = NULL;
        pNciContext->tSendPayload.wPayloadSize = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return;
}

void phNciNfc_Notify(void* pContext,NFCSTATUS wStatus,void* pBuff)
{
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t )pContext;
    pphNciNfc_IfNotificationCb_t pUpperLayerCb = NULL;
    void *pUpperLayerCtx = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if(pNciContext->IfNtf != NULL)
    {
        pUpperLayerCb = pNciContext->IfNtf;
        pUpperLayerCtx = pNciContext->IfNtfCtx;
        pNciContext->IfNtf = NULL;
        pNciContext->IfNtfCtx = NULL;
        if(NULL != pUpperLayerCb)
        {
            PH_LOG_NCI_INFO_STR("Invoking upper layer call back function");
            pUpperLayerCb(pUpperLayerCtx, wStatus, pBuff);
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}
