/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_ListenMgmt.tmh"

static NFCSTATUS phNciNfc_SendCb(void* pContext, void *pInfo, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_SeSendCb(void* pContext, void *pInfo, NFCSTATUS wStatus);

static void phNciNfc_TempReceiveCb(_At_((phNciNfc_Context_t*)pContext, _In_) void *pContext);

static NFCSTATUS phNciNfc_UpdateLstnMgmtRemDevInfo(pphNciNfc_RemoteDevInformation_t pRemDevInf, uint8_t *pBuff, uint16_t wLen);

NFCSTATUS
phNciNfc_ListenMgmt(void *psContext,
                  pphNciNfc_RemoteDevInformation_t pRemDevInf,
                  uint8_t *pBuff,
                  uint16_t wLen)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t psNciCtxt = (pphNciNfc_Context_t)psContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == psNciCtxt) || (0 == wLen) || (NULL == pBuff) || (NULL == pRemDevInf))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_NCI_INFO_STR("Invalid input parameters");
    }
    else
    {
        /* Extract info into RemDevInf structure */
        wStatus = phNciNfc_UpdateLstnMgmtRemDevInfo(pRemDevInf,pBuff,wLen);

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Based on active Rf Technology and Rf protocol,
            perform reader management init or P2P management init*/
            if(phNciNfc_e_RfInterfacesNFCDEP_RF == pRemDevInf->eRfIf &&
                phNciNfc_e_RfProtocolsNfcDepProtocol == pRemDevInf->eRFProtocol)
            {
                wStatus = phNciNfc_NfcILstnInit(psNciCtxt,pRemDevInf,pBuff,wLen);
            }
            else if(phNciNfc_e_RfInterfacesISODEP_RF == pRemDevInf->eRfIf &&
                phNciNfc_e_RfProtocolsIsoDepProtocol == pRemDevInf->eRFProtocol)
            {
                wStatus = phNciNfc_NfcIsoLstnRdrInit(pRemDevInf,pBuff,wLen);
            }
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            (psNciCtxt->tActvDevIf.pDevInfo) = pRemDevInf;
            wStatus = phNciNfc_SetConnCredentials(psNciCtxt);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS
phNciNfc_UpdateLstnMgmtRemDevInfo(pphNciNfc_RemoteDevInformation_t pRemDevInf,
                          uint8_t *pBuff,
                          uint16_t wLen
                          )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t *pRfNtfBuff = NULL;
    uint8_t RfTechSpecParamsLen = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL == pRemDevInf) || (NULL == pBuff) || (0 == wLen))
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_CRIT_STR(" Invalid Param(s)..");
    }
    else
    {
        pRemDevInf->bRfDiscId = pBuff[0];
        pRemDevInf->eRfIf = (phNciNfc_RfInterfaces_t)pBuff[1];

        if(phNciNfc_e_RfInterfacesNfceeDirect_RF != (pRemDevInf->eRfIf))
        {
            pRemDevInf->eRFProtocol = (phNciNfc_RfProtocols_t)pBuff[2];
            pRemDevInf->eRFTechMode = (phNciNfc_RfTechMode_t)pBuff[3];
            pRemDevInf->bMaxPayLoadSize = pBuff[4];
            pRemDevInf->bInitialCredit = pBuff[5];
            /* Obtain the len of RF tech specific parameters from Resp buff */
            RfTechSpecParamsLen = pBuff[6];
            pRemDevInf->bTechSpecificParamLen = RfTechSpecParamsLen;
            pRfNtfBuff = &(pBuff[7+RfTechSpecParamsLen]);

            pRemDevInf->eDataXchgRFTechMode = (phNciNfc_RfTechMode_t)*(pRfNtfBuff+0);
            pRemDevInf->bTransBitRate = *(pRfNtfBuff+1);
            pRemDevInf->bRecvBitRate = *(pRfNtfBuff+2);
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Interface is NFCEE Direct RF,subsequent payload contents ignored..");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS
phNciNfc_ReceiveData(void *pNciCtx,
                     pphNciNfc_IfNotificationCb_t pReceiveCb,
                     void* pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_NOT_INITIALISED;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)pNciCtx;
    pphNciNfc_ActiveDeviceInfo_t  pActivDev = NULL;
    phNciNfc_sCoreHeaderInfo_t tRegInfo;
    uint8_t                 bConnId;
    uint8_t bFreeTempBuff = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciContext) || (pNciContext != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
    }
    else if(NULL == pReceiveCb)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_NCI_CRIT_STR("Invalid input parameter (Nci Context)!");
    }
    else
    {
        /* Store upper layer call back function and upper layer context */
        pNciContext->IfNtf = pReceiveCb;
        pNciContext->IfNtfCtx = pContext;

        /* Check whether data has been received from remote device already */
        if(1 == pNciContext->tLstnModeRecvInfo.bDataBuffEnable)
        {
            /* Data has already been received and stored in temporary buffer.
            Instead of registering for the data message which has already been received,
            queue a deferred callback */

            wStatus = phOsalNfc_QueueDeferredCallback(phNciNfc_TempReceiveCb,
                                                      pNciContext);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                wStatus = NFCSTATUS_PENDING;
            }
            else
            {
                bFreeTempBuff = 1;
            }

            pNciContext->tLstnModeRecvInfo.bDataBuffEnable = 0;

            if(1 == bFreeTempBuff)
            {
                pNciContext->IfNtf = NULL;
                pNciContext->IfNtfCtx = NULL;
                if(NULL != pNciContext->tLstnModeRecvInfo.pBuff)
                {
                    phOsalNfc_FreeMemory(pNciContext->tLstnModeRecvInfo.pBuff);
                    pNciContext->tLstnModeRecvInfo.pBuff = NULL;
                }
                pNciContext->tLstnModeRecvInfo.wBuffSize = 0;
                pNciContext->tLstnModeRecvInfo.wLstnCbStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            pActivDev = (pphNciNfc_ActiveDeviceInfo_t)&pNciContext->tActvDevIf;

            wStatus = phNciNfc_GetConnId(pActivDev->pDevInfo,&bConnId);
            /* If dummy data receive call back function is still registered, unregister the same first*/
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                tRegInfo.bConn_ID = bConnId;
                tRegInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
                /* UnRegister for Data message (with auto de-register enabled) */
                (void)phNciNfc_CoreIfUnRegRspNtf(&(pNciContext->NciCoreContext),
                    &(tRegInfo),&phNciNfc_ReceiveDataBufferCb);
            }
            /*Now register for the data message */
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                tRegInfo.bEnabled = (uint8_t)PHNCINFC_ENABLE_AUTO_DEREG;
                tRegInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
                tRegInfo.bConn_ID= bConnId;

                /* Register for Data message (with auto de-register enabled) */
                wStatus = phNciNfc_CoreIfRegRspNtf((void *)&pNciContext->NciCoreContext,
                                         &tRegInfo,
                                         &phNciNfc_ReceiveCb,
                                         pNciContext);
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                   /* Data message call back registered successfully, send pending status to upper layer */
                   wStatus = NFCSTATUS_PENDING;
                }
                else
                {
                    pNciContext->IfNtf = NULL;
                    pNciContext->IfNtfCtx = NULL;
                    PH_LOG_NCI_CRIT_STR("Data message call back registration failed!");
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else
            {
                pNciContext->IfNtf = NULL;
                pNciContext->IfNtfCtx = NULL;
                PH_LOG_NCI_CRIT_STR("Couldn't Get ConnId!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_SendData(void *pNciCtx,
                  pphNciNfc_IfNotificationCb_t pSendCb,
                  void* pContext,
                  phNfc_sData_t *pSendData)
{
    NFCSTATUS               wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t   TxInfo;
    uint16_t                wPldDataSize;
    pphNciNfc_ActiveDeviceInfo_t  pActivDev = NULL;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)pNciCtx;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciContext) || (pNciContext != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pSendData) || (NULL == pSendCb))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_NCI_CRIT_STR("Invalid input parameter!");
    }
    else
    {
        pActivDev = (pphNciNfc_ActiveDeviceInfo_t)&pNciContext->tActvDevIf;

        /* Store upper layer call back function and upper layer context */
        pNciContext->IfNtf = pSendCb;
        pNciContext->IfNtfCtx = pContext;

        if((0 == pSendData->length) || (NULL == pSendData->buffer))
        {
            PH_LOG_NCI_CRIT_STR("Invalid Send buffer!");
            pNciContext->IfNtf = NULL;
            pNciContext->IfNtfCtx = NULL;
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            /* Fill the data packet details into TxInfo */
            TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            wStatus = phNciNfc_GetConnId(pActivDev->pDevInfo, &(TxInfo.tHeaderInfo.bConn_ID));
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                PH_LOG_NCI_INFO_STR("P2P Transreceive (Send) payload created successfully!");
                wPldDataSize = (uint16_t)pSendData->length;

                (pNciContext->tTranscvCtxt.tSendPld.wLen) = 0;
                (pNciContext->tTranscvCtxt.tSendPld.pBuff) = (uint8_t *)phOsalNfc_GetMemory(wPldDataSize);

                if(NULL != (pNciContext->tTranscvCtxt.tSendPld.pBuff))
                {
                    (pNciContext->tTranscvCtxt.tSendPld.wLen) = (wPldDataSize);
                    phOsalNfc_SetMemory((pNciContext->tTranscvCtxt.tSendPld.pBuff),0,
                        (pNciContext->tTranscvCtxt.tSendPld.wLen));

                    /* Copy data to be sent to the local buffer */
                    phOsalNfc_MemCopy((pNciContext->tTranscvCtxt.tSendPld.pBuff),
                        (pSendData->buffer),wPldDataSize);

                    TxInfo.Buff = (pNciContext->tTranscvCtxt.tSendPld.pBuff);
                    TxInfo.wLen = (pNciContext->tTranscvCtxt.tSendPld.wLen);

                    wStatus = phNciNfc_CoreIfTxOnly(&(pNciContext->NciCoreContext), &TxInfo,
                                (pphNciNfc_CoreIfNtf_t)&phNciNfc_SendCb, pNciContext);
                    if(NFCSTATUS_PENDING != wStatus)
                    {
                        /* Deallocate memory allcoated for send buffer */
                        phOsalNfc_FreeMemory(pNciContext->tTranscvCtxt.tSendPld.pBuff);
                        pNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
                        pNciContext->tTranscvCtxt.tSendPld.wLen = 0;
                        pNciContext->IfNtf = NULL;
                        pNciContext->IfNtfCtx = NULL;
                    }
                }
                else
                {
                    pNciContext->IfNtf = NULL;
                    pNciContext->IfNtfCtx = NULL;
                    wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
                    PH_LOG_NCI_CRIT_STR("Payload MemAlloc for Send request Failed!");
                }
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
                pNciContext->IfNtf = NULL;
                pNciContext->IfNtfCtx = NULL;
                PH_LOG_NCI_CRIT_STR(" Couldn't Get ConnId!");
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_SeSendData(void* pNciCtx,
                    void* pSeHandle,
                    pphNciNfc_IfNotificationCb_t pSendCb,
                    void* pContext,
                    phNfc_sData_t *pSendData)
{
    NFCSTATUS               wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t   TxInfo;
    uint16_t                wPldDataSize;
    phNciNfc_Context_t* pNciContext = (phNciNfc_Context_t *)pNciCtx;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciContext) || (pNciContext != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pSendData) || (NULL == pSendCb))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_NCI_CRIT_STR("Invalid input parameter!");
    }
    else
    {
        if((0 == pSendData->length) || (NULL == pSendData->buffer))
        {
            PH_LOG_NCI_CRIT_STR("Invalid Send buffer!");
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            /* Fill the data packet details into TxInfo */
            TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            if (!phNciNfc_IsVersion1x(pNciContext))
            {
                TxInfo.tHeaderInfo.bConn_ID = CONNHCITYPE_STATIC;
            }
            else
            {
                wStatus = phNciNfc_GetConnId(pSeHandle, &(TxInfo.tHeaderInfo.bConn_ID));
            }

            if(NFCSTATUS_SUCCESS == wStatus)
            {
                wPldDataSize = (uint16_t)pSendData->length;

                (pNciContext->tTranscvCtxt.tSendPld.wLen) = 0;
                (pNciContext->tTranscvCtxt.tSendPld.pBuff) = (uint8_t *)phOsalNfc_GetMemory(wPldDataSize);

                if(NULL != (pNciContext->tTranscvCtxt.tSendPld.pBuff))
                {
                    (pNciContext->tTranscvCtxt.tSendPld.wLen) = (wPldDataSize);
                    phOsalNfc_SetMemory((pNciContext->tTranscvCtxt.tSendPld.pBuff),0,
                        (pNciContext->tTranscvCtxt.tSendPld.wLen));

                    /* Copy data to be sent to the local buffer */
                    phOsalNfc_MemCopy((pNciContext->tTranscvCtxt.tSendPld.pBuff),
                        (pSendData->buffer),wPldDataSize);

                    TxInfo.Buff = (pNciContext->tTranscvCtxt.tSendPld.pBuff);
                    TxInfo.wLen = (pNciContext->tTranscvCtxt.tSendPld.wLen);

                    /* Send data */
                    wStatus = phNciNfc_CoreIfTxOnly(&(pNciContext->NciCoreContext), &TxInfo,
                                (pphNciNfc_CoreIfNtf_t)&phNciNfc_SeSendCb, pNciContext);
                    if(NFCSTATUS_PENDING == wStatus)
                    {
                        /* Store upper layer call back function and upper layer context */
                        pNciContext->IfNtf = pSendCb;
                        pNciContext->IfNtfCtx = pContext;
                    }
                    else
                    {
                        /* Deallocate memory allcoated for send buffer */
                        phOsalNfc_FreeMemory(pNciContext->tTranscvCtxt.tSendPld.pBuff);
                        pNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
                        pNciContext->tTranscvCtxt.tSendPld.wLen = 0;
                    }
                }
                else
                {
                    wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
                    PH_LOG_NCI_CRIT_STR("Payload MemAlloc for Send request Failed!");
                }
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
                PH_LOG_NCI_CRIT_STR(" Couldn't Get ConnId!");
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void phNciNfc_ListenMgmt_DeActivate(void* pContext,
                                    pphNciNfc_RemoteDevInformation_t pRemDevInfo)
{
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t ) pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciCtx) && (NULL != pRemDevInfo))
    {
        switch(pRemDevInfo->eRFTechMode)
        {
            case phNciNfc_NFCA_Listen:
            case phNciNfc_NFCA_Active_Listen:
            case phNciNfc_NFCB_Listen:
            case phNciNfc_NFCF_Listen:
            case phNciNfc_NFCF_Active_Listen:
            {
                if(1 == pNciCtx->tLstnModeRecvInfo.bDataBuffEnable)
                {
                    /* Data has already been received but there is not RemDev_Receive call from upper layer.
                    Just free the temporary buffer and reset the 'bDataBuffEnable' flag */
                    pNciCtx->tLstnModeRecvInfo.bDataBuffEnable = 0;
                    if(NULL != pNciCtx->tLstnModeRecvInfo.pBuff)
                    {
                        phOsalNfc_FreeMemory(pNciCtx->tLstnModeRecvInfo.pBuff);
                        pNciCtx->tLstnModeRecvInfo.pBuff = NULL;
                    }
                    pNciCtx->tLstnModeRecvInfo.wBuffSize = 0;
                    pNciCtx->tLstnModeRecvInfo.wLstnCbStatus = NFCSTATUS_TARGET_LOST;
                }
            }
            break;
            default:
                /* Nothing to be done */
                break;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

NFCSTATUS phNciNfc_ReceiveCb(void* pContext, void *pInfo, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciContext = pContext;
    NFCSTATUS wStat;

    PH_LOG_NCI_FUNC_ENTRY();
    wStat = wStatus;
    if(NULL != pNciContext)
    {
        if(NULL != pNciContext->IfNtf)
        {
            pNciContext->IfNtf(pNciContext->IfNtfCtx,wStat,pInfo);
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid context received from RecvMgrHdlr!");
    }

    PH_LOG_NCI_FUNC_EXIT();
    return wStat;
}

static NFCSTATUS phNciNfc_SendCb(void* pContext, void *pInfo, NFCSTATUS wStatus)
{
    NFCSTATUS wStat = wStatus;
    pphNciNfc_Context_t pNciContext = pContext;
    UNUSED(pInfo);
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        /* Opening a dummy read request inorder not to loose data sent from remote device */
        (void )phNciNfc_DummyReadReq((void *)pNciContext);
        if(NULL != pNciContext->IfNtf)
        {
            pNciContext->IfNtf(pNciContext->IfNtfCtx,wStat,NULL);
        }
        if(NULL != pNciContext->tTranscvCtxt.tSendPld.pBuff)
        {
            PH_LOG_NCI_INFO_STR("De-allocating Send Request Payload Buffer...");
            phOsalNfc_FreeMemory(pNciContext->tTranscvCtxt.tSendPld.pBuff);
            pNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
            pNciContext->tTranscvCtxt.tSendPld.wLen = 0;
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid context received from RecvMgrHdlr!");
    }

    PH_LOG_NCI_FUNC_EXIT();
    return wStat;
}

static NFCSTATUS phNciNfc_SeSendCb(void* pContext, void *pInfo, NFCSTATUS wStatus)
{
    NFCSTATUS wStat = wStatus;
    pphNciNfc_Context_t pNciContext = pContext;

    UNUSED(pInfo);
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        if(NULL != pNciContext->tTranscvCtxt.tSendPld.pBuff)
        {
            PH_LOG_NCI_INFO_STR("De-allocating Send Request Payload Buffer...");
            phOsalNfc_FreeMemory(pNciContext->tTranscvCtxt.tSendPld.pBuff);
            pNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
            pNciContext->tTranscvCtxt.tSendPld.wLen = 0;
        }
        phNciNfc_Notify(pNciContext, wStatus, NULL);
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid context received from RecvMgrHdlr!");
    }

    PH_LOG_NCI_FUNC_EXIT();
    return wStat;
}

static
void
phNciNfc_TempReceiveCb(
    _At_((phNciNfc_Context_t*)pContext, _In_) void *pContext
    )
{
    phNciNfc_TransactInfo_t tTransInfo;
    pphNciNfc_IfNotificationCb_t pUpperLayerCb = NULL;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)pContext;
    void *pUpperLayerCtx = NULL;
    NFCSTATUS wStatus;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        tTransInfo.pbuffer = pNciContext->tLstnModeRecvInfo.pBuff;
        tTransInfo.wLength = pNciContext->tLstnModeRecvInfo.wBuffSize;
        wStatus = pNciContext->tLstnModeRecvInfo.wLstnCbStatus;

        /* Get upper layer call back function */
        pUpperLayerCb = pNciContext->IfNtf;
        pUpperLayerCtx = pNciContext->IfNtfCtx;
        pNciContext->IfNtf = NULL;
        pNciContext->IfNtfCtx = NULL;
        if(NULL != pUpperLayerCb)
        {
            pUpperLayerCb(pUpperLayerCtx,wStatus,&tTransInfo);
        }
        if(NULL != pNciContext->tLstnModeRecvInfo.pBuff)
        {
            phOsalNfc_FreeMemory(pNciContext->tLstnModeRecvInfo.pBuff);
            pNciContext->tLstnModeRecvInfo.pBuff = NULL;
        }
        pNciContext->tLstnModeRecvInfo.wBuffSize = 0;
        pNciContext->tLstnModeRecvInfo.wLstnCbStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return;
}
