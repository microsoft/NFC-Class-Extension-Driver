/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Target.tmh"

static void phLibNfc_P2pRemoteDev_ReceiveCb(void* pContext,NFCSTATUS status,void* pInfo);
static void phLibNfc_P2pRemoteDev_SendCb(void* pContext,NFCSTATUS status,void* pInfo);
static void phLibNfc_DeferredReceiveCb(void *pContext);

static NFCSTATUS phLibNfc_HCEQueueDeferredRecv(void *pContext)
{
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    pLibContext->HCE_FirstBuf = 0;
    wStatus = phOsalNfc_QueueDeferredCallback(phLibNfc_DeferredReceiveCb, pContext);
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        wStatus = NFCSTATUS_PENDING;
    }
    else
    {
        if (NULL != pLibContext->HCE_FirstBuffer.buffer)
        {
            phOsalNfc_FreeMemory(pLibContext->HCE_FirstBuffer.buffer);
            pLibContext->HCE_FirstBuffer.buffer = NULL;
        }
        pLibContext->HCE_FirstBuffer.length = 0;
    }
    
    return wStatus;
}

NFCSTATUS phLibNfc_Discovered2Recv(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        /* Perform P2P read */
        wStatus = phNciNfc_SendData(pLibContext->sHwReference.pNciHandle,
                             (pphNciNfc_IfNotificationCb_t)&phLibNfc_P2pRemoteDev_SendCb,
                             (void *)pLibContext,
                             (phNfc_sData_t *)Param1);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Recv2Send(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        /* Perform P2P read */
        /* If the 1st buffer flag is enabled, queue a deferred callback, return pending*/
        if( 1 == pLibContext->HCE_FirstBuf )
        {
            wStatus = phLibNfc_HCEQueueDeferredRecv(pContext);
        }
        else
        {
            wStatus = phNciNfc_ReceiveData(pLibContext->sHwReference.pNciHandle,
                                 (pphNciNfc_IfNotificationCb_t)&phLibNfc_P2pRemoteDev_ReceiveCb,
                                 (void *)pLibContext);
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phLibNfc_RemoteDev_Receive( phLibNfc_Handle            hRemoteDevice,
                            pphLibNfc_Receive_RspCb_t   pReceiveRspCb,
                            void*                       pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = phLibNfc_GetContext();
    phLibNfc_Event_t TrigEvent = phLibNfc_EventRecv;
    pphNciNfc_RemoteDevInformation_t pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pLibContext);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        wStatus=NFCSTATUS_NOT_INITIALISED;
    }
    /* Check for valid state,If De initialize is called then return NFCSTATUS_SHUTDOWN */
    else if(pLibContext->StateContext.TrgtState == phLibNfc_StateReset)
    {
        wStatus = NFCSTATUS_SHUTDOWN;
    }
    else if((NULL == pReceiveRspCb) || (NULL == pLibRemDevHandle))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if((NFCSTATUS_SUCCESS == wStatus) && (NULL != pNciRemDevHandle))
        {
            pLibContext->CBInfo.pClientNfcIpRxCb = pReceiveRspCb;
            pLibContext->CBInfo.pClientNfcIpRxCntx = pContext;

            wStatus = phLibNfc_StateHandler(pLibContext,TrigEvent,
                                        NULL,NULL,NULL);
            if(NFCSTATUS_PENDING == wStatus)
            {
                /* Do nothing */
            }
            else if(NFCSTATUS_INVALID_STATE == wStatus)
            {
                PH_LOG_LIBNFC_WARN_STR("phLibNfc_RemoteDev_Receive: StateHandler returned NFCSTATUS_INVALID_STATE");
                wStatus = NFCSTATUS_DESELECTED;
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_U32MSG("phLibNfc_RemoteDev_Receive: StateHandler returned status:",wStatus);
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_HANDLE;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phLibNfc_RemoteDev_Send(phLibNfc_Handle             hRemoteDevice,
                        phNfc_sData_t*              pTransferData,
                        pphLibNfc_RspCb_t           pSendRspCb,
                        void*                       pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = phLibNfc_GetContext();
    phLibNfc_Event_t TrigEvent = phLibNfc_EventSend;
    pphNciNfc_RemoteDevInformation_t pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pLibContext);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        wStatus=NFCSTATUS_NOT_INITIALISED;
    }
    /* Check for valid state,If De initialize is called then return NFCSTATUS_SHUTDOWN */
    else if(pLibContext->StateContext.TrgtState == phLibNfc_StateReset)
    {
        wStatus = NFCSTATUS_SHUTDOWN;
    }
    else if((NULL == pSendRspCb) || (NULL == pTransferData) || (NULL == pLibRemDevHandle))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        if((NULL != pTransferData->buffer) || (0 == pTransferData->length))
        {
            wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
            if((NFCSTATUS_SUCCESS == wStatus) && (NULL != pNciRemDevHandle))
            {
                pLibContext->CBInfo.pClientNfcIpTxCb = pSendRspCb;
                pLibContext->CBInfo.pClientNfcIpTxCntx = pContext;

                wStatus = phLibNfc_StateHandler(pLibContext,TrigEvent,
                                    (void *)pTransferData,NULL,NULL);
                if(NFCSTATUS_PENDING == wStatus)
                {
                    /* Do nothing */
                }
                else if(NFCSTATUS_INVALID_STATE == wStatus)
                {
                    wStatus = NFCSTATUS_DESELECTED;
                    pLibContext->CBInfo.pClientNfcIpTxCb = NULL;
                    pLibContext->CBInfo.pClientNfcIpTxCntx = NULL;
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED;
                    pLibContext->CBInfo.pClientNfcIpTxCb = NULL;
                    pLibContext->CBInfo.pClientNfcIpTxCntx = NULL;
                }
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_HANDLE;
            }
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_P2pRemoteDev_SendCb(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphLibNfc_RspCb_t pUpperLayerCb = NULL;
    void *UpperLayerCtx = NULL;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        pUpperLayerCb = pLibContext->CBInfo.pClientNfcIpTxCb;
        UpperLayerCtx = pLibContext->CBInfo.pClientNfcIpTxCntx;
        pLibContext->CBInfo.pClientNfcIpTxCb = NULL;
        pLibContext->CBInfo.pClientNfcIpTxCntx = NULL;
        (void)phLibNfc_StateHandler(pLibContext, TrigEvent, NULL, NULL, NULL);
        if(NULL != pUpperLayerCb)
        {
            pUpperLayerCb(UpperLayerCtx,wStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

static void phLibNfc_P2pRemoteDev_ReceiveCb(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = status;
    phNfc_sData_t tRecvBufferInfo;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t) pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphLibNfc_Receive_RspCb_t pUpperLayerCb = NULL;
    void *pUpperLayerCtx = NULL;
    UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext)&& (NULL != pTransInfo))
    {
        pUpperLayerCb = pLibContext->CBInfo.pClientNfcIpRxCb;
        pUpperLayerCtx = pLibContext->CBInfo.pClientNfcIpRxCntx;
        (void)phLibNfc_StateHandler(pLibContext, TrigEvent, NULL, NULL, NULL);
        if(NULL != pUpperLayerCb)
        {
            tRecvBufferInfo.buffer = pTransInfo->pbuffer;
            tRecvBufferInfo.length = (uint32_t)pTransInfo->wLength;
            pUpperLayerCb(pUpperLayerCtx,&tRecvBufferInfo,wStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

static void phLibNfc_DeferredReceiveCb(void *pContext)
{
    phNfc_sData_t tRecvBufferInfo;
    pphLibNfc_Receive_RspCb_t pUpperLayerCb = NULL;
    phLibNfc_LibContext_t *pLibContext = (phLibNfc_LibContext_t *)pContext;
    void *pUpperLayerCtx = NULL;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        pUpperLayerCb = pLibContext->CBInfo.pClientNfcIpRxCb;
        pUpperLayerCtx = pLibContext->CBInfo.pClientNfcIpRxCntx;
        pLibContext->CBInfo.pClientNfcIpRxCb = NULL;
        pLibContext->CBInfo.pClientNfcIpRxCntx = NULL;
        (void)phLibNfc_StateHandler(pLibContext, TrigEvent, NULL, NULL, NULL);
        if(NULL != pUpperLayerCb)
        {
            tRecvBufferInfo.buffer = pLibContext->HCE_FirstBuffer.buffer;
            tRecvBufferInfo.length = pLibContext->HCE_FirstBuffer.length;
            pUpperLayerCb(pUpperLayerCtx,&tRecvBufferInfo,NFCSTATUS_SUCCESS);
        }
        if(pLibContext->HCE_FirstBuffer.buffer )
        {
            phOsalNfc_FreeMemory(pLibContext->HCE_FirstBuffer.buffer);
            pLibContext->HCE_FirstBuffer.buffer = NULL;
            pLibContext->HCE_FirstBuffer.length = 0;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}
