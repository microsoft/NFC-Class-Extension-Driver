/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_Context.tmh"

void phNciNfc_NciCtxInitialize(pphNciNfc_Context_t pNciCtx)
{
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        pNciCtx->NciDiscContext.pDiscPayload = NULL;
        pNciCtx->tDeActvInfo.pDeActvNotif = NULL;
        pNciCtx->tDeActvInfo.DeActvCtxt = NULL;
        pNciCtx->tRegListInfo.DiscoveryCtxt = NULL;
        pNciCtx->tRegListInfo.GenericErrNtfCtxt = NULL;
        pNciCtx->tRegListInfo.NfceeCtxt = NULL;
        pNciCtx->tRegListInfo.pDiscoveryNotification = NULL;
        pNciCtx->tRegListInfo.pGenericErrNtfCb = NULL;
        pNciCtx->tRegListInfo.pNfceeNotification = NULL;
        pNciCtx->tRfConfContext.pReqParamList = NULL;
        pNciCtx->tSendPayload.pBuff = NULL;
        pNciCtx->IfNtf = NULL;
        pNciCtx->IfNtfCtx = NULL;
        pNciCtx->pSeqHandler = NULL;
        pNciCtx->RspBuffInfo.pBuff = NULL;
        pNciCtx->tTranscvCtxt.pNotify = NULL;
        pNciCtx->tTranscvCtxt.pContext = NULL;
        pNciCtx->tActvDevIf.pDevInfo = NULL;
        pNciCtx->tSendPayload.pBuff = NULL;
        pNciCtx->tTranscvCtxt.tSendPld.pBuff = NULL;
        pNciCtx->tTranscvCtxt.tRecvPld.pBuff = NULL;
        pNciCtx->pUpperLayerInfo = NULL;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

void phNciNfc_SetUpperLayerCallback(pphNciNfc_Context_t nciContext, pphNciNfc_IfNotificationCb_t callbackFunction, void* callbackContext)
{
    PH_LOG_NCI_FUNC_ENTRY();

    if (nciContext->IfNtf != NULL || nciContext->IfNtfCtx != NULL)
    {
        PH_LOG_NCI_CRIT_STR(
            "Overwriting upper layer callback in nciContext! Lost values: nciContext->IfNtf=%p, nciContext->IfNtfCtx=%p",
            (void*)nciContext->IfNtf,
            nciContext->IfNtfCtx);
    }

    nciContext->IfNtf = callbackFunction;
    nciContext->IfNtfCtx = callbackContext;

    PH_LOG_NCI_INFO_STR("New values: nciContext->IfNtf=%p, nciContext->IfNtfCtx=%p", (void*)nciContext->IfNtf, nciContext->IfNtfCtx);
    PH_LOG_NCI_FUNC_EXIT();
}
