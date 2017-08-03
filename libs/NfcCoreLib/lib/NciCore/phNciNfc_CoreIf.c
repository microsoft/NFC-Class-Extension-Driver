/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h"

#include "phNciNfc_CoreIf.tmh"


NFCSTATUS phNciNfc_CoreIfTxRx(pphNciNfc_CoreContext_t pCtx,
                               pphNciNfc_CoreTxInfo_t pTxInfo,
                               pphNciNfc_Buff_t  pRxBuffInfo,
                               uint32_t dwTimeOutMs,
                               pphNciNfc_CoreIfNtf_t NciCb,
                               void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();
    if ((NULL != pCtx) &&
        (NULL != pTxInfo) &&
        (NULL != pRxBuffInfo) &&
        (NULL != NciCb))
    {
        if(pCtx->SendStateContext.CurrState == phNciNfc_StateSendIdle)
        {
            /* Either command or data message can be sent */
            if(phNciNfc_e_NciCoreMsgTypeCntrlCmd == pTxInfo->tHeaderInfo.eMsgType)
            {
                pTxInfo->tHeaderInfo.bConn_ID = 0;
            }
            else /* Sending a data message */
            {
                pTxInfo->tHeaderInfo.Group_ID = (phNciNfc_CoreGid_t)0;
                pTxInfo->tHeaderInfo.Opcode_ID.Val = 0;
            }
            /* All calling function which use 'phNciNfc_CoreIfTxRx' to send and receive cmd-Rsp or Data
            shall go for Auto deregistration */
            pTxInfo->tHeaderInfo.bEnabled = PHNCINFC_ENABLE_AUTO_DEREG;
            pCtx->tTemp.pTxInfo = pTxInfo;
            pCtx->tTemp.dwTimeOutMs = dwTimeOutMs;
            pCtx->tTemp.NciCb = NciCb;
            pCtx->tTemp.pContext = pContext;

            pCtx->bCoreTxOnly = 0; /* Notify upper layer after response is received for the command sent or
                                      sending command fails*/
            phOsalNfc_MemCopy(&(pCtx->TxInfo), pTxInfo, sizeof(phNciNfc_CoreTxInfo_t));
            /* Print the NCI packet details */
            phNciNfc_PrintPacketDescription(&pTxInfo->tHeaderInfo, pTxInfo->Buff, pTxInfo->wLen, pCtx->bLogDataMessages);
            wStatus = phNciNfc_StateHandler(pCtx, phNciNfc_EvtSendPacket);
        }
        else
        {
            wStatus = NFCSTATUS_BUSY;
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreIfTxOnly(pphNciNfc_CoreContext_t pCtx,
                               pphNciNfc_CoreTxInfo_t pTxInfo,
                               pphNciNfc_CoreIfNtf_t NciCb,
                               void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();
    if ((NULL != pCtx) &&
        (NULL != pTxInfo) &&
        (NULL != NciCb))
    {
        if(pCtx->SendStateContext.CurrState == phNciNfc_StateSendIdle)
        {
            pCtx->tTemp.pTxInfo = pTxInfo;
            /* No timer is used */
            pCtx->tTemp.dwTimeOutMs = 0;
            pCtx->tTemp.NciCb = NciCb;
            pCtx->tTemp.pContext = pContext;

            pCtx->bCoreTxOnly = 1; /* Send and receive operation is in progress */
            phOsalNfc_MemCopy(&(pCtx->TxInfo), pTxInfo, sizeof(phNciNfc_CoreTxInfo_t));
            /* Print the NCI packet details */
            phNciNfc_PrintPacketDescription(&pTxInfo->tHeaderInfo, pTxInfo->Buff, pTxInfo->wLen, pCtx->bLogDataMessages);
            wStatus = phNciNfc_StateHandler(pCtx, phNciNfc_EvtSendPacket);
        }
        else
        {
            wStatus = NFCSTATUS_BUSY;
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreIfRegRspNtf(void *pCtx,
                                   pphNciNfc_sCoreHeaderInfo_t pInfo,
                                   pphNciNfc_CoreIfNtf_t pNotify,
                                   void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCoreCtx = pCtx;
    phNciNfc_CoreRegInfo_t tRegInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (NULL != pInfo) &&  (NULL != pNotify))
    {
        tRegInfo.pContext = pContext;
        tRegInfo.pNotifyCb = pNotify;

        if (pInfo->eMsgType == phNciNfc_e_NciCoreMsgTypeData)
        {
            /*Register with data manager*/
            tRegInfo.bEnabled = pInfo->bEnabled;
            tRegInfo.bConnId = pInfo->bConn_ID;
            wStatus = phNciNfc_CoreRecvMgrRegisterCb((void *)pCoreCtx, &tRegInfo,
                                    phNciNfc_e_NciCoreMsgTypeData);
        }else if (pInfo->eMsgType == phNciNfc_e_NciCoreMsgTypeCntrlRsp)
        {
            /*Register with Response Manager*/
            tRegInfo.bEnabled = (uint8_t) PHNCINFC_ENABLE_AUTO_DEREG;
            tRegInfo.bGid = (uint8_t)pInfo->Group_ID;
            tRegInfo.bOid = (uint8_t)pInfo->Opcode_ID.Val;
            wStatus = phNciNfc_CoreRecvMgrRegisterCb((void *)pCoreCtx, &tRegInfo,
                                    phNciNfc_e_NciCoreMsgTypeCntrlRsp);
        }else if (pInfo->eMsgType == phNciNfc_e_NciCoreMsgTypeCntrlNtf)
        {
            /*Register with NTF Manager*/
            tRegInfo.bEnabled = (uint8_t) PHNCINFC_DISABLE_AUTO_DEREG;
            tRegInfo.bGid = (uint8_t)pInfo->Group_ID;
            tRegInfo.bOid = (uint8_t)pInfo->Opcode_ID.Val;
            wStatus = phNciNfc_CoreRecvMgrRegisterCb((void *)pCoreCtx, &tRegInfo,
                                    phNciNfc_e_NciCoreMsgTypeCntrlNtf);
        }else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
NFCSTATUS phNciNfc_CoreIfUnRegRspNtf(void *pCtx,
                                     pphNciNfc_sCoreHeaderInfo_t pInfo,
                                     pphNciNfc_CoreIfNtf_t pNotify
                                     )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCoreCtx = pCtx;
    phNciNfc_CoreRegInfo_t tRegInfo;
    phNciNfc_NciCoreMsgType_t eMsgType = phNciNfc_e_NciCoreMsgTypeInvalid;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (NULL != pInfo))
    {
        tRegInfo.pNotifyCb = pNotify;
        if (pInfo->eMsgType == phNciNfc_e_NciCoreMsgTypeData)
        {
            /*UnRegister From data manager*/
            tRegInfo.bConnId = pInfo->bConn_ID;
            eMsgType = phNciNfc_e_NciCoreMsgTypeData;
        }else if (pInfo->eMsgType == phNciNfc_e_NciCoreMsgTypeCntrlRsp)
        {
            /*UnRegister From Response Manager*/
            tRegInfo.bGid = (uint8_t)pInfo->Group_ID;
            tRegInfo.bOid = (uint8_t)pInfo->Opcode_ID.Val;
            eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlRsp;
        }else if (pInfo->eMsgType == phNciNfc_e_NciCoreMsgTypeCntrlNtf)
        {
            /*UnRegister From NTF Manager*/
            tRegInfo.bGid = (uint8_t)pInfo->Group_ID;
            tRegInfo.bOid = (uint8_t)pInfo->Opcode_ID.Val;
            eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
        }else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        if(NFCSTATUS_INVALID_PARAMETER != wStatus)
        {
            wStatus = phNciNfc_CoreRecvMgrDeRegisterCb((void*)pCoreCtx, &tRegInfo,
                                    eMsgType);
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreIfSetMaxCtrlPacketSize(void *pContext,
                                                     uint8_t PktSize)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;
    if(NULL != pCtx)
    {
        pCtx->bMaxCtrlPktPayloadLen = PktSize;
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    return wStatus;
}
