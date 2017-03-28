/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Hci.tmh"

static NFCSTATUS phLibNfc_OpenLogConn(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_OpenLogConnProcess(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciOpenAdmPipe(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciOpenAdmPipeProc(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciGetHostList(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciGetHostListProc(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciSetWhiteList(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciSetWhiteListProc(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciGetWhiteList(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciGetWhiteListProc(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciGetSessionIdentity(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciGetSessionIdentityProc(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciChildDevInitComplete(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_HciInitComplete(void* pContext,NFCSTATUS status,void* pInfo);

static NFCSTATUS phLibNfc_NfceeModeSet(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_NfceeModeSetProc(void *pContext, NFCSTATUS wStatus, void *pInfo);

static void phLibNfc_WaitForEvtHotPlug(void *pCtx, NFCSTATUS Status, void* pInfo);

static NFCSTATUS phLibNfc_HciDataSend(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciDataSendProc(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_HciDataSendComplete(void* pContext,NFCSTATUS status,void* pInfo);

static void phHciNfc_eSETranseiveTimeOutCb(uint32_t dwTimerId, void *pContext);
static NFCSTATUS phHciNfc_CreateSETranseiveTimer(phHciNfc_HciContext_t  *pHciContext);

phLibNfc_Sequence_t gphLibNfc_HciInitSequenceNci1x[] = {
    {&phLibNfc_OpenLogConn, &phLibNfc_OpenLogConnProcess},
    {&phLibNfc_NfceeModeSet, &phLibNfc_NfceeModeSetProc},
    {&phLibNfc_DelayForSeNtf, &phLibNfc_DelayForSeNtfProc},
    {&phLibNfc_HciOpenAdmPipe, &phLibNfc_HciOpenAdmPipeProc},
    {&phLibNfc_HciGetSessionIdentity, &phLibNfc_HciGetSessionIdentityProc},
    {&phLibNfc_HciGetHostList, &phLibNfc_HciGetHostListProc},
    {&phLibNfc_HciSetWhiteList, &phLibNfc_HciSetWhiteListProc},
    {NULL, &phLibNfc_HciInitComplete}
};

phLibNfc_Sequence_t gphLibNfc_HciChildDevInitSequence[] = {
    {&phLibNfc_HciSetWhiteList, &phLibNfc_HciSetWhiteListProc},
    {&phLibNfc_NfceeModeSet, &phLibNfc_NfceeModeSetProc},
    {NULL, &phLibNfc_HciChildDevInitComplete}
};

static phLibNfc_Sequence_t gphLibNfc_HciTransceiveSequence[] = {
    {&phLibNfc_HciDataSend, &phLibNfc_HciDataSendProc},
    {NULL, &phLibNfc_HciDataSendComplete}
};

void phLibNfc_HciDeInit()
{
    pphLibNfc_Context_t pLibCtx = gpphLibNfc_Context;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibCtx)
    {
        if(NULL != pLibCtx->pHciContext)
        {
            phOsalNfc_FreeMemory(pLibCtx->pHciContext);
            pLibCtx->pHciContext = NULL;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

NFCSTATUS phLibNfc_eSE_Transceive (phLibNfc_Handle hSE_Handle,
                                   phNfc_sSeTransceiveInfo_t     *pSeTransceiveInfo,
                                   pphLibNfc_TransceiveCallback_t  pTransceive_RspCb,
                                   void*                           pContext)
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_SE_List_t *pSeList = NULL;
    uint8_t bIndex = 0;
    pphLibNfc_Context_t pLibCtx = PHLIBNFC_GETCONTEXT();
    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = phLibNfc_IsInitialised(PHLIBNFC_GETCONTEXT());
    if((NFCSTATUS_SUCCESS != wStatus) || (NULL == pLibCtx))
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if( (NULL == (void *)hSE_Handle) || (NULL == pTransceive_RspCb) ||
        (NULL == pSeTransceiveInfo))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameters");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        if((NULL != pSeTransceiveInfo->sSendData.buffer) && (0 != pSeTransceiveInfo->sSendData.length) &&
            (NULL != pSeTransceiveInfo->sRecvData.buffer) && (0 != pSeTransceiveInfo->sRecvData.length))
        {
                /*Validate the SE Handle.Traverse through the SE list saved in PhLibNfc context and check if the
                input SE handle is valid */
                for(bIndex = 0; bIndex < PHHCINFC_TOTAL_NFCEES ; bIndex++)
                {
                    if( hSE_Handle == pLibCtx->tSeInfo.tSeList[bIndex].hSecureElement)
                    {
                        pLibCtx->sSeContext.pActiveSeInfo = &pLibCtx->tSeInfo.tSeList[bIndex];
                        break;
                    }
                }

                if(pLibCtx->sSeContext.pActiveSeInfo != NULL)
                {
                    pSeList = pLibCtx->sSeContext.pActiveSeInfo;
                    /*SE type must be eSE and Activation mode must be set to APDUMode */
                    if( (pSeList->eSE_Type == phLibNfc_SE_Type_eSE) &&
                        (pSeList->eSE_ActivationMode == phLibNfc_SE_ActModeApdu))
                    {
                        pLibCtx->pSeTransInfo = pSeTransceiveInfo;
                        PHLIBNFC_INIT_SEQUENCE(pLibCtx,gphLibNfc_HciTransceiveSequence);
                        wStatus = phLibNfc_SeqHandler(pLibCtx,wStatus,NULL);
                        if(NFCSTATUS_PENDING != wStatus)
                        {
                            PH_LOG_LIBNFC_CRIT_STR("Hci Transceive sequence could not start!");
                            wStatus = NFCSTATUS_FAILED;
                            pLibCtx->CBInfo.pSeClientTransCb = NULL;
                            pLibCtx->CBInfo.pSeClientTransCntx = NULL;
                        }
                        else
                        {
                            pLibCtx->CBInfo.pSeClientTransCb = pTransceive_RspCb;
                            pLibCtx->CBInfo.pSeClientTransCntx = pContext;
                        }
                    }
                    else
                    {
                        wStatus = NFCSTATUS_FAILED;
                        PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSE_Transceive: Invalid SE Type or SE Mode");
                    }
                }
            else
            {
                wStatus = NFCSTATUS_FAILED;
                PH_LOG_LIBNFC_CRIT_STR("Invalid SE Handle");
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Send or Receive data invalid");
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_OpenLogConn(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t) pContext;
    pphHciNfc_HciContext_t pHciContext = (pphHciNfc_HciContext_t)pCtx->pHciContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        if((pHciContext != NULL) && (pHciContext->pSeHandle != NULL))
        {
            wStatus = phNciNfc_Nfcee_Connect(pCtx->sHwReference.pNciHandle,
                                             pHciContext->pSeHandle,
                                             (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                             pCtx);
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_OpenLogConnProcess(void* pContext, NFCSTATUS wStatus, void* pInfo)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    UNUSED(pInfo) ;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NFCSTATUS_SUCCESS == wStatus) && (pCtx !=NULL))
    {
        wStatus = phHciNfc_CoreInit((void *) pCtx->pHciContext);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_HciOpenAdmPipe(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    UNUSED(pInfo);
    UNUSED(status);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
            /* Open a pipe to ADM (Admin) gate */
            wStatus = phHciNfc_OpenPipe(pLibCtx->pHciContext,(uint8_t)PHHCINFC_PIPE_ADMIN_PIPEID,
                                        &phLibNfc_InternalSequence,
                                        pContext);
            if(NFCSTATUS_PENDING != wStatus)
            {
                PH_LOG_LIBNFC_CRIT_X32MSG("Failed to create pipe for ADM pipe, error",wStatus);
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context passed!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciOpenAdmPipe: Invalid LibNfc context passed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_HciOpenAdmPipeProc(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    uint8_t bNumOfPipes;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
            if(NFCSTATUS_SUCCESS == status)
            {
                wStatus = status;
                PH_LOG_LIBNFC_INFO_STR("ADM pipe successfully opened");
                if(NULL != pInfo)
                {
                    bNumOfPipes = (*(uint8_t *)pInfo);
                    PH_LOG_LIBNFC_INFO_U32MSG("Number of ADM pipes opened", bNumOfPipes);
                }
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Failed to open ADM pipe");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context received!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_HciSetWhiteList(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx = (pphLibNfc_Context_t)pContext;
    phHciNfc_HciContext_t *pHciCtx;
    uint8_t bIndex = PHHCINFC_PIPE_WHITELIST_INDEX;
    uint8_t bCount = 0;
    uint8_t aSetWhiteListSe[] = {0xFF,0xFF};
    UNUSED(pInfo);
    UNUSED(status);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pHciCtx = pLibCtx->pHciContext;

        if((NULL != pHciCtx) && (NULL != pLibCtx->sSeContext.pActiveSeInfo))
        {
            if(pLibCtx->sSeContext.pActiveSeInfo->eSE_Type == phLibNfc_SE_Type_HciNwk)
            {
                if((1 == pLibCtx->Config.bHciNwkPerNfcee) && (1 == pHciCtx->bNoOfHosts))
                {
                    /* If NFCC supports a HCI network per NFCEE set the whitelist as part of HCI init sequence */
                    aSetWhiteListSe[bCount++] = pHciCtx->aHostList[0];
                }

                wStatus = NFCSTATUS_SUCCESS;
            }
            else if((pLibCtx->sSeContext.pActiveSeInfo->eSE_Type == phLibNfc_SE_Type_UICC) ||
                    (pLibCtx->sSeContext.pActiveSeInfo->eSE_Type == phLibNfc_SE_Type_eSE))
            {
                if(NULL != pLibCtx->tSeInfo.tSeList[phLibNfc_SE_Index_UICC].hSecureElement)
                {
                    aSetWhiteListSe[bCount++] = phHciNfc_e_UICCHostID;
                }

                if(NULL != pLibCtx->tSeInfo.tSeList[phLibNfc_SE_Index_eSE].hSecureElement)
                {
                    aSetWhiteListSe[bCount++] = phHciNfc_e_SEHostID;
                }

                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
                PH_LOG_LIBNFC_CRIT_STR("Failed: No Active eSE_Type");
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
            PH_LOG_LIBNFC_CRIT_STR("Failed: No Active eSE_Type");
        }

        if((wStatus == NFCSTATUS_SUCCESS) && (bCount > 0))
        {
            wStatus = phHciNfc_AnySetParameter(pLibCtx->pHciContext,
                            bIndex,
                            PHHCINFC_PIPE_ADMIN_PIPEID,
                            bCount,
                            aSetWhiteListSe,
                            &phLibNfc_InternalSequence,
                            pContext);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_HciSetWhiteListProc(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
            wStatus = status;
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context received!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_HciGetWhiteList(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    UNUSED(pInfo);
    UNUSED(status);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
            /*Get the white list on HCI network*/
            wStatus=phHciNfc_AnyGetParameter(
                        pLibCtx->pHciContext,
                        (uint8_t)PHHCINFC_ADM_GATEID,
                        PHHCINFC_PIPE_WHITELIST_INDEX,
                        (uint8_t)PHHCINFC_PIPE_ADMIN_PIPEID,
                        &phLibNfc_InternalSequence,
                        pContext
                     );
            if(NFCSTATUS_PENDING != wStatus)
            {
                PH_LOG_LIBNFC_CRIT_X32MSG("Failed to Get WhiteList, error",wStatus);
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context received!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_HciGetWhiteListProc(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    phHciNfc_HciContext_t *pHciCtx;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        pHciCtx = pLibCtx->pHciContext;
        if(NULL != pHciCtx && NULL != pInfo)
        {
            if(NFCSTATUS_SUCCESS == status)
            {
                wStatus = status;
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Failed to Read Host List");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context received!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_HciInitComplete(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pLibCtx = (pphLibNfc_Context_t)pContext;
    uint8_t bIndex = 0;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pLibCtx != NULL)
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_LIBNFC_INFO_STR("Hci network initialization success");
            pLibCtx->tSeInfo.bSeState[phLibNfc_SE_Index_HciNwk] = phLibNfc_SeStateInitialized;
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Hci network initialization failed!");
            pLibCtx->tSeInfo.bSeState[phLibNfc_SE_Index_HciNwk] = phLibNfc_SeStateInvalid;
            wStatus = NFCSTATUS_FAILED;
        }

        wStatus = phLibNfc_SE_GetIndex(pLibCtx, phLibNfc_SeStateNotInitialized, &bIndex);
        if (wStatus == NFCSTATUS_SUCCESS)
        {
            wStatus = phLibNfc_HciLaunchChildDevInitSequence(pLibCtx, bIndex);
        }

        if((wStatus == NFCSTATUS_FAILED) && (pLibCtx->sSeContext.nNfceeDiscNtf == 0))
        {
            phLibNfc_LaunchNfceeDiscCompleteSequence(pLibCtx,wStatus,NULL);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_HciDataSend(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    NFCSTATUS wSETxRxTimerStatus;
    phHciNfc_HciContext_t *pHciContext;
    pphLibNfc_Context_t pLibCtx;
    uint8_t bPipeId =0;
    UNUSED(pInfo);
    UNUSED(pHciContext);
    UNUSED(status);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
            pHciContext = (phHciNfc_HciContext_t *) pLibCtx->pHciContext;
            wStatus = phHciNfc_GetPipeId(pLibCtx->pHciContext,phHciNfc_e_ApduGate, &bPipeId);
            PH_LOG_HCI_INFO_X32MSG("PIPE ID",bPipeId);
            if((wStatus ==NFCSTATUS_SUCCESS) && (bPipeId !=0xFF))
            {
                wStatus = phHciNfc_Transceive(pLibCtx->pHciContext,
                                        bPipeId,
                                        PHHCINFC_PROP_DATA_EVENT,
                                        pLibCtx->pSeTransInfo->sSendData.length,
                                        pLibCtx->pSeTransInfo->sSendData.buffer,
                                        &phLibNfc_InternalSequence,
                                        pContext);
                if(wStatus == NFCSTATUS_PENDING)
                {
                    /*Store Timeout Value to be used for Transeive*/
                    if(pLibCtx->pSeTransInfo->timeout != 0)
                    {
                        pHciContext->tHciSeTxRxTimerInfo.dwTimeOut = pLibCtx->pSeTransInfo->timeout;
                    }else
                    {
                        pHciContext->tHciSeTxRxTimerInfo.dwTimeOut = PHHCINFC_DEFAULT_HCI_TX_RX_TIME_OUT;
                    }
                    wSETxRxTimerStatus = phHciNfc_CreateSETranseiveTimer(pHciContext);
                    if(wSETxRxTimerStatus == NFCSTATUS_SUCCESS)
                    {
                        /* Start the SE TxRx Timer*/
                        pHciContext  = (phHciNfc_HciContext_t *) pLibCtx->pHciContext;
                        wSETxRxTimerStatus = phOsalNfc_Timer_Start(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId,
                                                pHciContext->tHciSeTxRxTimerInfo.dwTimeOut,
                                                &phHciNfc_eSETranseiveTimeOutCb,
                                                pLibCtx);
                        if(wSETxRxTimerStatus != NFCSTATUS_SUCCESS)
                        {
                            phOsalNfc_Timer_Delete(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId);
                            pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId = 0;
                            PH_LOG_LIBNFC_CRIT_STR("SE TxRx Timer Start Failed");
                        }
                    }
                }
            }
            else
            {
                PH_LOG_HCI_INFO_EXPECT(wStatus == NFCSTATUS_SUCCESS);
                PH_LOG_HCI_INFO_EXPECT(bPipeId != 0xFF);
                PH_LOG_LIBNFC_CRIT_STR("Failure with phHciNfc_GetPipeId!");
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context received!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phHciNfc_CreateSETranseiveTimer(phHciNfc_HciContext_t  *pHciContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
     pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId = phOsalNfc_Timer_Create();
     if(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId == PH_OSALNFC_TIMER_ID_INVALID)
     {
         PH_LOG_LIBNFC_CRIT_STR("HCI SE TxRx Timer Create failed");
         wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
     }else
     {
         PH_LOG_LIBNFC_CRIT_STR("HCI SE TxRx Timer Created Successfully");
         wStatus = NFCSTATUS_SUCCESS;
     }
     return wStatus;
}
static void phHciNfc_eSETranseiveTimeOutCb(uint32_t dwTimerId, void *pContext)
{
    pphLibNfc_Context_t pLibCtx;
    phHciNfc_HciContext_t *pHciContext;
    PHNFC_UNUSED_VARIABLE(dwTimerId); /* No Data Expected from lower Layer */
    PH_LOG_LIBNFC_CRIT_STR("HCI SE TxRx Timer Expired");
    if(pContext != NULL)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        pHciContext = (phHciNfc_HciContext_t *) pLibCtx->pHciContext;
        (void)phOsalNfc_Timer_Stop(dwTimerId);
        (void)phOsalNfc_Timer_Delete(dwTimerId);
        pHciContext->tHciSeTxRxTimerInfo.dwTimeOut = PHHCINFC_DEFAULT_HCI_TX_RX_TIME_OUT;
        pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId= PH_OSALNFC_TIMER_ID_INVALID;
        /* Set the Default Timeout for eSE Transeive */
        (void)phLibNfc_InternalSequence(pLibCtx,NFCSTATUS_RESPONSE_TIMEOUT,NULL);
    }
}
static NFCSTATUS phLibNfc_HciDataSendProc(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phHciNfc_ReceiveParams_t *pReceivedParams = (phHciNfc_ReceiveParams_t *) pInfo;
    pphLibNfc_Context_t pLibCtx;
    phHciNfc_HciContext_t *pHciContext;

    UNUSED(pHciContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext) && (NULL != pReceivedParams))
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NFCSTATUS_SUCCESS == status)
        {
            wStatus = status;
            if(pLibCtx->pHciContext!= NULL)
            {
                pHciContext = (phHciNfc_HciContext_t *) pLibCtx->pHciContext;
                if((0 != pReceivedParams->wLen) && (NULL != pReceivedParams->pData))
                {
                    if(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId != PH_OSALNFC_TIMER_ID_INVALID)
                    {
                        (void)phOsalNfc_Timer_Stop(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId);
                        (void)phOsalNfc_Timer_Delete(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId);
                        pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId = 0;
                        pHciContext->tHciSeTxRxTimerInfo.dwTimeOut = PHHCINFC_DEFAULT_HCI_TX_RX_TIME_OUT;
                        PH_LOG_LIBNFC_INFO_STR("SE TxRx Timer Deleted");
                    }
                    PH_LOG_LIBNFC_INFO_STR("Received valid data");
                    if((NULL != pLibCtx->pSeTransInfo->sRecvData.buffer) && (0 != pLibCtx->pSeTransInfo->sRecvData.length))
                    {
                        if(pReceivedParams->wLen <= pLibCtx->pSeTransInfo->sRecvData.length)
                        {
                            phOsalNfc_MemCopy(pLibCtx->pSeTransInfo->sRecvData.buffer,
                                pReceivedParams->pData,pReceivedParams->wLen);
                            pLibCtx->pSeTransInfo->sRecvData.length = pReceivedParams->wLen;
                        }
                        else
                        {
                            PH_LOG_LIBNFC_CRIT_STR("Could not copy entire received data");
                            wStatus = NFCSTATUS_MORE_INFORMATION;
                        }
                    }
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("Invalid data received!");
                    wStatus = NFCSTATUS_FAILED;
                }
            }else
            {
                PH_LOG_LIBNFC_CRIT_STR("Invalid HCI Context");
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_HciDataSendComplete(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_TransceiveCallback_t pClientCb = NULL;
    void *pClientCntx = NULL;
    pphLibNfc_Context_t pLibCtx;

    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        pClientCb = pLibCtx->CBInfo.pSeClientTransCb;
        pClientCntx = pLibCtx->CBInfo.pSeClientTransCntx;
        pLibCtx->CBInfo.pSeClientTransCb = NULL;
        pLibCtx->CBInfo.pSeClientTransCntx = NULL;
        if(NULL != pClientCb)
        {
            pClientCb(pClientCntx, NULL, &pLibCtx->pSeTransInfo->sRecvData,status);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
void phLibNfc_WaitForEvtHotPlug(void *pCtx, NFCSTATUS Status, void* pInfo)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pCtx;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    UNUSED(pInfo);
    if(NULL != pLibContext)
    {
        /* Continue the sequence if NFCEE is not present */
        if(NFCSTATUS_SUCCESS != Status)
        {
            (void)phLibNfc_InternalSequence(pLibContext,Status,NULL);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

void phLibNfc_SeEventHotPlugCb(void* pContext,  NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_LibContext_t pLibContext = gpphLibNfc_Context;
    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NFCSTATUS_SUCCESS == wStatus)
    {
        (void)phLibNfc_InternalSequence(pLibContext,wStatus,NULL);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

static NFCSTATUS
phLibNfc_HciGetSessionIdentity(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    UNUSED(pInfo);
    UNUSED(status);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
            /* Open a pipe to ADM (Admin) gate */
            wStatus=phHciNfc_AnyGetParameter(
                        pLibCtx->pHciContext,
                        (uint8_t)PHHCINFC_ADM_GATEID,
                        PHHCINFC_PIPE_SESSION_INDEX,
                        (uint8_t)PHHCINFC_PIPE_ADMIN_PIPEID,
                        &phLibNfc_InternalSequence,
                        pContext
                     );
            if(NFCSTATUS_PENDING != wStatus)
            {
                PH_LOG_LIBNFC_CRIT_X32MSG("Failed to Get SessionIdentity, error",wStatus);
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context passed!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context passed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;

}
static NFCSTATUS
phLibNfc_HciGetSessionIdentityProc(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    phHciNfc_ReceiveParams_t *pReadSessionIdentity = NULL;
    phHciNfc_HciContext_t *pHciCtx;
    phHciNfc_HciRegData_t  tHciRegData;
    uint8_t bCheckPipePresence;
    uint8_t bIndex;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        pHciCtx = pLibCtx->pHciContext;
        if((NULL != pLibCtx->pHciContext) &&(NULL != pInfo))
        {
            if(NFCSTATUS_SUCCESS == status)
            {
                wStatus = status;
                pReadSessionIdentity = (phHciNfc_ReceiveParams_t *) pInfo;

                /* Check if Session ID is already updated by Pipe Create Notifications */
                if(pHciCtx->aGetHciSessionId[PHHCI_PIPE_PRESENCE_INDEX] != 0xFF)
                {
                    /*Check if APDU pipe is already created*/
                    if(pHciCtx->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId != 0)
                    {
                        pReadSessionIdentity->pData[PHHCI_PIPE_PRESENCE_INDEX] = SET_BITS8(pReadSessionIdentity->pData[PHHCI_PIPE_PRESENCE_INDEX], PHHCI_ESE_APDU_PIPE_CREATED_BIT_INDEX, 1, 0);
                        pReadSessionIdentity->pData[PHHCI_ESE_APDU_PIPE_STORAGE_INDEX] = pHciCtx->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId;
                    }
                    if(pHciCtx->aSEPipeList[PHHCI_ESE_CONN_PIPE_LIST_INDEX].bPipeId != 0)
                    {
                         pReadSessionIdentity->pData[PHHCI_PIPE_PRESENCE_INDEX] = SET_BITS8(pReadSessionIdentity->pData[PHHCI_PIPE_PRESENCE_INDEX], PHHCI_ESE_CONNECTIVITY_PIPE_CREATED_BIT_INDEX, 1, 0);
                          pReadSessionIdentity->pData[PHHCI_ESE_CONNECTIVITY_PIPE_STORAGE_INDEX] = pHciCtx->aSEPipeList[PHHCI_ESE_CONN_PIPE_LIST_INDEX].bPipeId;
                    }
                    if(pHciCtx->aUICCPipeList[PHHCI_UICC_CONN_PIPE_LIST_INDEX].bPipeId != 0)
                    {
                         pReadSessionIdentity->pData[PHHCI_PIPE_PRESENCE_INDEX] = SET_BITS8(pReadSessionIdentity->pData[PHHCI_PIPE_PRESENCE_INDEX], PHHCI_UICC_CONNECTIVITY_PIPE_CREATED_BIT_INDEX, 1, 0);
                         pReadSessionIdentity->pData[PHHCI_UICC_CONNECTIVITY_PIPE_STORAGE_INDEX] = pHciCtx->aUICCPipeList[PHHCI_UICC_CONN_PIPE_LIST_INDEX].bPipeId;
                    }
                }

                phOsalNfc_MemCopy(pHciCtx->aGetHciSessionId,pReadSessionIdentity->pData,PHHCINFC_PIPE_SESSIONID_LEN);

                /* Read the pipe presence status */
                bCheckPipePresence = pReadSessionIdentity->pData[PHHCI_PIPE_PRESENCE_INDEX];

                /* Check a Pipe present on APDU GATE for eSE */
                if(0 == (uint8_t)GET_BITS8(bCheckPipePresence,PHHCI_ESE_APDU_PIPE_CREATED_BIT_INDEX,1))
                {
                    pHciCtx->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId = pReadSessionIdentity->pData[PHHCI_ESE_APDU_PIPE_STORAGE_INDEX];
                    pHciCtx->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bGateId = phHciNfc_e_ApduGate;
                }
                /* Check a Pipe present on CONNECTIVITY GATE for eSE */
                if(0 == (uint8_t)GET_BITS8(bCheckPipePresence,PHHCI_ESE_CONNECTIVITY_PIPE_CREATED_BIT_INDEX,1))
                {
                    pHciCtx->aSEPipeList[PHHCI_ESE_CONN_PIPE_LIST_INDEX].bPipeId = pReadSessionIdentity->pData[PHHCI_ESE_CONNECTIVITY_PIPE_STORAGE_INDEX];
                    pHciCtx->aSEPipeList[PHHCI_ESE_CONN_PIPE_LIST_INDEX].bGateId = phHciNfc_e_ConnectivityGate;
                    /* Register for Events for the pipe on Connectivity Gate */
                    tHciRegData.eMsgType = phHciNfc_e_HciMsgTypeEvent;
                    tHciRegData.bPipeId = pReadSessionIdentity->pData[PHHCI_ESE_CONNECTIVITY_PIPE_STORAGE_INDEX];
                    (void )phHciNfc_RegisterCmdRspEvt(pHciCtx,
                                            &tHciRegData,
                                            &phHciNfc_ProcessEventsOnPipe,
                                            pHciCtx);
                }
                /* Check a Pipe present on CONNECTIVITY GATE for UICC*/
                if(0 == (uint8_t)GET_BITS8(bCheckPipePresence,PHHCI_UICC_CONNECTIVITY_PIPE_CREATED_BIT_INDEX,1))
                {
                    pHciCtx->aUICCPipeList[PHHCI_UICC_CONN_PIPE_LIST_INDEX].bPipeId = pReadSessionIdentity->pData[PHHCI_UICC_CONNECTIVITY_PIPE_STORAGE_INDEX];
                    pHciCtx->aUICCPipeList[PHHCI_UICC_CONN_PIPE_LIST_INDEX].bGateId = phHciNfc_e_ConnectivityGate;
                    /* Register for Events for the pipe on Connectivity Gate */
                    tHciRegData.eMsgType = phHciNfc_e_HciMsgTypeEvent;
                    tHciRegData.bPipeId = pReadSessionIdentity->pData[PHHCI_UICC_CONNECTIVITY_PIPE_STORAGE_INDEX];
                    (void )phHciNfc_RegisterCmdRspEvt(pHciCtx,
                                            &tHciRegData,
                                            &phHciNfc_ProcessEventsOnPipe,
                                            pHciCtx);
                }

                if(0xFF == bCheckPipePresence)
                {
                    /* Clear the Data as no pipe are created */
                    for(bIndex = 0; bIndex < PHHCINFC_TOTAL_NFCEES; bIndex++)
                    {   /* Clear the eSE data */
                        pHciCtx->aSEPipeList[bIndex].bGateId = PHHCINFC_NO_PIPE_DATA;
                        pHciCtx->aSEPipeList[bIndex].bPipeId = PHHCINFC_NO_PIPE_DATA;
                        if( bIndex ==0)
                        {   /* Clear the UICC data */
                            pHciCtx->aUICCPipeList[bIndex].bGateId = PHHCINFC_NO_PIPE_DATA;
                            pHciCtx->aUICCPipeList[bIndex].bPipeId = PHHCINFC_NO_PIPE_DATA;
                        }
                    }
                }

                PH_LOG_LIBNFC_INFO_STR("Read GetSessionIdentity successfully ");
                PH_LOG_LIBNFC_INFO_U32MSG("GetSessionIdentity",(uint32_t)pReadSessionIdentity->pData[PHHCI_PIPE_PRESENCE_INDEX]);
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Failed to Read Session Identity");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context received!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phLibNfc_HciSetSessionIdentity(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    uint8_t bIndex = PHHCINFC_PIPE_SESSION_INDEX;
    uint8_t bSessionIDDataLen = 8;
    phHciNfc_HciContext_t *pHciContext;

    UNUSED(pInfo);
    UNUSED(status);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
            pHciContext  = (phHciNfc_HciContext_t *) pLibCtx->pHciContext;
            if(pHciContext->bCreatePipe == 0x01)
            {
                pHciContext->bCreatePipe = 0x00;
                wStatus = phHciNfc_AnySetParameter(pLibCtx->pHciContext,
                            bIndex,
                            PHHCINFC_PIPE_ADMIN_PIPEID,
                            bSessionIDDataLen,
                            pHciContext->aGetHciSessionId,
                            &phLibNfc_InternalSequence,
                            pContext);
            }
            else
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
        }
        else
        {
            wStatus = NFCSTATUS_SUCCESS;
            PH_LOG_LIBNFC_CRIT_STR("No Hci network present!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;

}
NFCSTATUS
phLibNfc_HciSetSessionIdentityProc(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
            wStatus = status;
        }
        else
        {
            wStatus = NFCSTATUS_SUCCESS;
            PH_LOG_LIBNFC_CRIT_STR("No Hci network present!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
static NFCSTATUS
phLibNfc_HciGetHostList(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx;
    UNUSED(pInfo);
    UNUSED(status);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
            /*Get the Host list on HCI network*/
            wStatus=phHciNfc_AnyGetParameter(
                        pLibCtx->pHciContext,
                        (uint8_t)PHHCINFC_ADM_GATEID,
                        PHHCINFC_PIPE_HOST_LIST_INDEX,
                        (uint8_t)PHHCINFC_PIPE_ADMIN_PIPEID,
                        &phLibNfc_InternalSequence,
                        pContext
                     );
            if(NFCSTATUS_PENDING != wStatus)
            {
                PH_LOG_LIBNFC_CRIT_X32MSG("Failed to Get HostList, error",wStatus);
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context passed!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context passed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phLibNfc_HciGetHostListProc(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx = (pphLibNfc_Context_t)pContext;
    phHciNfc_HciContext_t *pHciCtx;
    uint8_t bIndex = 0, bCount = 0;
    phHciNfc_ReceiveParams_t *pReadHostList = NULL;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibCtx) && (NULL != pLibCtx->pHciContext) && (NULL != pInfo))
    {
        pHciCtx = pLibCtx->pHciContext;

        if(NFCSTATUS_SUCCESS == status)
        {
            wStatus = status;
            pReadHostList = (phHciNfc_ReceiveParams_t *) pInfo;

            for(bIndex = 0; bIndex < pReadHostList->wLen; bIndex++)
            {
                if((pReadHostList->pData[bIndex] != phHciNfc_e_HostControllerID) &&
                   (pReadHostList->pData[bIndex] != phHciNfc_e_TerminalHostID))
                {
                    pHciCtx->aHostList[bCount] = pReadHostList->pData[bIndex];
                    bCount++;
                }
            }

            pHciCtx->bNoOfHosts = bCount;
            PH_LOG_LIBNFC_INFO_STR("Read HostList successfully. bNoOfHosts: %d", pHciCtx->bNoOfHosts);
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Failed to Read Host List");
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context received!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_NfceeModeSet(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    pphNciNfc_NfceeDeviceHandle_t pNfceeHandle; 
    UNUSED(pInfo);
    UNUSED(wStatus);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (PHLIBNFC_GETCONTEXT() == pLibContext))
    {
        pNfceeHandle = (pphNciNfc_NfceeDeviceHandle_t)pLibContext->sSeContext.pActiveSeInfo->hSecureElement;
        if( pNfceeHandle->tDevInfo.eNfceeStatus == PH_NCINFC_EXT_NFCEEMODE_ENABLE)
        {
            wIntStatus = NFCSTATUS_SUCCESS;
        }
        else
        {
           wIntStatus = phNciNfc_Nfcee_ModeSet(pLibContext->sHwReference.pNciHandle,
                                    pLibContext->sSeContext.pActiveSeInfo->hSecureElement,
                                    PH_NCINFC_EXT_NFCEEMODE_ENABLE,
                                    (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                    (void *)pLibContext);
            if(NFCSTATUS_PENDING != wIntStatus)
            {
                PH_LOG_LIBNFC_CRIT_X32MSG("Failed to set mode, error",wIntStatus);
            }
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid Context passed from lower layer!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS phLibNfc_NfceeModeSetProc(void *pContext, NFCSTATUS wStatus, void *pInfo)
{
    NFCSTATUS wIntStatus = wStatus;
    pphLibNfc_LibContext_t pLibContext = pContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (PHLIBNFC_GETCONTEXT() == pLibContext))
    {
        wIntStatus = wStatus;
        if(NFCSTATUS_SUCCESS == wIntStatus)
        {
            PH_LOG_LIBNFC_INFO_STR("Set Se Mode success");
        }
        else
        {
            PH_LOG_LIBNFC_WARN_STR("Set Se Mode Failed!");
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

NFCSTATUS phLibNfc_HciChildDevInitComplete(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_LibContext_t pLibCtx = (pphLibNfc_Context_t)pContext;
    uint8_t bIndex;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pLibCtx != NULL)
    {
        wStatus = phLibNfc_SE_GetIndex(pLibCtx, phLibNfc_SeStateInitializing, &bIndex);
        if(wStatus == NFCSTATUS_SUCCESS)
        {
            if(status == NFCSTATUS_SUCCESS)
            {
                PH_LOG_LIBNFC_INFO_STR("NFCEE initialization success");
                pLibCtx->tSeInfo.bSeState[bIndex] = phLibNfc_SeStateInitialized;
            }
            else
            {
                PH_LOG_LIBNFC_INFO_STR("NFCEE initialization failed");
                pLibCtx->tSeInfo.bSeState[bIndex] = phLibNfc_SeStateInvalid;
            }
        }

        wStatus = phLibNfc_SE_GetIndex(pLibCtx, phLibNfc_SeStateNotInitialized, &bIndex);
        if(wStatus == NFCSTATUS_SUCCESS)
        {
            wStatus = phLibNfc_HciLaunchChildDevInitSequence(pContext, bIndex);
        }

        if((wStatus == NFCSTATUS_FAILED) && (pLibCtx->sSeContext.nNfceeDiscNtf == 0))
        {
            phLibNfc_LaunchNfceeDiscCompleteSequence(pLibCtx,wStatus,NULL);
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_HciLaunchDevInitSequenceNci1x(void *pContext)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pLibContext != NULL)
    {
        pLibContext->tSeInfo.bSeState[phLibNfc_SE_Index_HciNwk] = phLibNfc_SeStateInitializing;
        pLibContext->sSeContext.pActiveSeInfo = (pphLibNfc_SE_List_t)(&pLibContext->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk]);

        /*Start the Sequence for HCI network*/
        PHLIBNFC_INIT_SEQUENCE(pLibContext, gphLibNfc_HciInitSequenceNci1x);
        wStatus = phLibNfc_SeqHandler(pContext,NFCSTATUS_SUCCESS,NULL);
        if(NFCSTATUS_PENDING != wStatus)
        {
            PH_LOG_LIBNFC_CRIT_STR("Hci init sequence could not start!");
            pLibContext->tSeInfo.bSeState[phLibNfc_SE_Index_HciNwk] = phLibNfc_SeStateInvalid;
            wStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_HciLaunchChildDevInitSequence(void *pContext,phLibNfc_SE_Index_t bIndex)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((pLibContext != NULL) &&
       (bIndex != phLibNfc_SE_Index_HciNwk) &&
       (pLibContext->tSeInfo.tSeList[bIndex].hSecureElement != NULL))
    {
        pLibContext->tSeInfo.bSeState[bIndex] = phLibNfc_SeStateInitializing;
        pLibContext->sSeContext.pActiveSeInfo = &pLibContext->tSeInfo.tSeList[bIndex];

        /*Start the Sequence for active element*/
        PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_HciChildDevInitSequence);
        wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
        if(NFCSTATUS_PENDING != wStatus)
        {
            PH_LOG_LIBNFC_CRIT_STR("NFCEE init sequence could not start!");
            pLibContext->tSeInfo.bSeState[bIndex] = phLibNfc_SeStateInvalid;
            wStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
