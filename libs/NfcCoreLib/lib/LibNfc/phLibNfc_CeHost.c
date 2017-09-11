/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_CeHost.tmh"

void phLibNfc_CardEmulation_DataReceiveCb(void* pContext,NFCSTATUS status,void* pInfo)
{
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    NFCSTATUS wStatus = status;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t) pContext;
    phLibNfc_NtfRegister_RspCb_t pUpperLayerCb = NULL;
    void *pUpperLayerCtx = NULL;
    uint16_t wDataLen = 0;
    uint8_t *pBuff = NULL;
    UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    (void)phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);

    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext)&& (NULL != pTransInfo))
    {
        pUpperLayerCb = pLibContext->CBInfo.pCeHostNtfCb;
        pUpperLayerCtx = pLibContext->CBInfo.pCeHostNtfCntx;
        if(NULL != pLibContext->HCE_FirstBuffer.buffer)
        {
            phOsalNfc_FreeMemory(pLibContext->HCE_FirstBuffer.buffer);
            pLibContext->HCE_FirstBuffer.buffer = NULL;
        }
        pLibContext->HCE_FirstBuffer.length = 0;
        pLibContext->HCE_FirstBuf = 1;

        if((NFCSTATUS_SUCCESS == wStatus) && (0 != pTransInfo->wLength) &&
            (NULL != pTransInfo->pbuffer))
        {
            wDataLen = (uint16_t)pTransInfo->wLength;
            pBuff = (uint8_t *)phOsalNfc_GetMemory(wDataLen);
            if (NULL != pBuff)
            {
                pLibContext->HCE_FirstBuffer.buffer = pBuff;
                pLibContext->HCE_FirstBuffer.length = wDataLen;
                /* First Buffer status has to be stored to propagate it to the user app */
                /* Copy the data received to the temoporary buffer */
                phOsalNfc_MemCopy(pLibContext->HCE_FirstBuffer.buffer,
                                  pTransInfo->pbuffer, wDataLen);
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Data Allocation failed");
            }
        }

        if(NULL != pUpperLayerCb)
        {
            pUpperLayerCb(pLibContext->CBInfo.pCeHostNtfCntx,
                        pLibContext->psRemoteDevList,
                        pLibContext->dev_cnt,
                        PHNFCSTATUS(wStatus)
                        );
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS
phLibNfc_CardEmulation_NtfRegister(
                                   phLibNfc_NtfRegister_RspCb_t    pNotificationHandler,
                                   void                      *pContext
                                   )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL == pNotificationHandler)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }else
    {
        wStatus = phLibNfc_IsInitialised(pCtx);

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if(NULL != pNotificationHandler)
            {
                pCtx->bHceSak = 1;
            }
            else
            {
                pCtx->bHceSak = 0;
            }

            pCtx->CBInfo.pCeHostNtfCb = pNotificationHandler;
            pCtx->CBInfo.pCeHostNtfCntx = pContext;
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Stack is not Initialised");
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_RegisterForHceActivation(void)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pLibContext = phLibNfc_GetContext();
    wStatus = phNciNfc_ReceiveData(pLibContext->sHwReference.pNciHandle,
        (pphNciNfc_IfNotificationCb_t)&phLibNfc_CardEmulation_DataReceiveCb,
        (void *)pLibContext);
    return wStatus;
}

NFCSTATUS phLibNfc_MapRemoteDevCeHost(phNfc_sIso14443AInfo_t     *pNfcAInfo,
                                 pphNciNfc_RemoteDevInformation_t pNciDevInfo)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    phNciNfc_Iso14443AInfo_t tNicNfcAInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pNfcAInfo!=NULL && pNciDevInfo!=NULL)
    {
        /*Currently technology specific parameter and activation paramters are mixed togather it needs to seperated out*/
        /*Only activation parameter to be mapped*/

        /*Compare the sak with the configured one*/
        /*Mapped Bit Rate from Nci RemoteDev Info to LibNfc RemoteDev Info*/
        wStatus = phLibNfc_MapBitRate((phNciNfc_BitRates_t)pNciDevInfo->bTransBitRate,
                                      (phNfc_eDataRate_t *)&pNfcAInfo->MaxDataRate);

        if(wStatus != NFCSTATUS_SUCCESS)
        {
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            tNicNfcAInfo = pNciDevInfo->tRemoteDevInfo.Iso14443A_Info;
            /*Mapped SAK info from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            pNfcAInfo->Sak=tNicNfcAInfo.Sak;
            /*Mapped ATQA from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            pNfcAInfo->AtqA[0]=tNicNfcAInfo.bSensResResp[0];
            pNfcAInfo->AtqA[1]=tNicNfcAInfo.bSensResResp[1];
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
