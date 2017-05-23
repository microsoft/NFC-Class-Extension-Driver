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
static void phLibNfc_eSE_GetAtrProc(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phHciNfc_CreateSEGetAtrTimer(phHciNfc_HciContext_t  *pHciContext);
void phHciNfc_RegisterForEvents(pphHciNfc_HciContext_t pHciContext, uint8_t bPipeId);

static uint32_t phHciNfc_CalcPower(uint16_t wNum, uint16_t wPow);
static uint32_t phHciNfc_CalcWtxTimeout(uint8_t bWtxBwi);

//ETSI 12 changes
static NFCSTATUS phLibNfc_HciSetHostType(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_HciSetHostTypeProc(void* pContext, NFCSTATUS status, void* pInfo);

extern NFCSTATUS phLibNfc_HciGetHostTypeList(void* pContext, NFCSTATUS status, void* pInfo);
extern NFCSTATUS phLibNfc_HciGetHostTypeListProc(void* pContext, NFCSTATUS status, void* pInfo);
static bool_t phHciNfc_Check_eSECompliancy(uint8_t *p_data, uint8_t data_len);

extern NFCSTATUS phLibNfc_HciCreateApduPipe(void* pContext, NFCSTATUS status, void* pInfo);
extern NFCSTATUS phLibNfc_HciCreateApduPipeProc(void* pContext, NFCSTATUS status, void* pInfo);

extern NFCSTATUS phLibNfc_HciOpenAPDUPipe(void* pContext, NFCSTATUS status, void* pInfo);
extern NFCSTATUS phLibNfc_HciOpenAPDUPipeProc(void* pContext, NFCSTATUS status, void* pInfo);

extern NFCSTATUS phLibNfc_Delay_eSEPipes(void* pContext, NFCSTATUS status, void* pInfo);
extern NFCSTATUS phLibNfc_Delay_eSEPipes_Process(void* pContext, NFCSTATUS wStatus, void* pInfo);

extern NFCSTATUS phLibNfc_Get_eSESessionId(void* pContext, NFCSTATUS status, void* pInfo);
extern NFCSTATUS phLibNfc_Verify_eSESessionId(void* pContext, NFCSTATUS status, void* pInfo);

extern NFCSTATUS phLibNfc_eSESetModeClearAllSeq(void *pContext, NFCSTATUS wStatus, void *pInfo);
extern NFCSTATUS phLibNfc_SetModeSeqEnd(void* pContext, NFCSTATUS status, void*     pInfo);

extern NFCSTATUS phLibNfc_eSEClearALLPipeComplete(void* pContext, NFCSTATUS status, void* pInfo);

static void phLibNfc_NfceeNtfDelayCb(uint32_t dwTimerId, void *pContext);
void phHciNfc_Process_eSE_ClearALLPipes(void);

phLibNfc_Sequence_t gphLibNfc_HciInitSequence[] = {
    {&phLibNfc_OpenLogConn, &phLibNfc_OpenLogConnProcess},
	{ &phLibNfc_NfceeModeSet, &phLibNfc_NfceeModeSetProc },
	{ &phLibNfc_DelayForSeNtf, &phLibNfc_DelayForSeNtfProc },
	{ &phLibNfc_HciOpenAdmPipe, &phLibNfc_HciOpenAdmPipeProc },
	{ &phLibNfc_HciSetHostType, &phLibNfc_HciSetHostTypeProc },
	{ &phLibNfc_HciGetSessionIdentity, &phLibNfc_HciGetSessionIdentityProc },
	{ &phLibNfc_DelayForSeNtf, &phLibNfc_DelayForSeNtfProc },
	{ &phLibNfc_HciGetHostList, &phLibNfc_HciGetHostListProc },
	{ &phLibNfc_HciSetWhiteList, &phLibNfc_HciSetWhiteListProc },
    {NULL, &phLibNfc_HciInitComplete}
};

phLibNfc_Sequence_t gphLibNfc_HciChildDevInitSequence[] = {	
    {&phLibNfc_NfceeModeSet, &phLibNfc_NfceeModeSetProc},	
	{ &phLibNfc_HciGetHostList, &phLibNfc_HciGetHostListProc },
	{ &phLibNfc_DelayForSeNtf, &phLibNfc_DelayForSeNtfProc },
	{ &phLibNfc_HciGetHostTypeList,&phLibNfc_HciGetHostTypeListProc },	
	{ &phLibNfc_HciCreateApduPipe,&phLibNfc_HciCreateApduPipeProc },
	{ &phLibNfc_HciOpenAPDUPipe,&phLibNfc_HciOpenAPDUPipeProc },
	{ &phLibNfc_DelayForSeNtf, &phLibNfc_DelayForSeNtfProc },
	{ &phLibNfc_HciSetSessionIdentity, &phLibNfc_HciSetSessionIdentityProc },
    {NULL, &phLibNfc_HciChildDevInitComplete}
};

static phLibNfc_Sequence_t gphLibNfc_HciTransceiveSequence[] = {
    {&phLibNfc_HciDataSend, &phLibNfc_HciDataSendProc},
    {NULL, &phLibNfc_HciDataSendComplete}
};

phLibNfc_Sequence_t gphLibNfc_eSEHandleClearAllPipesSequence[] =
{
	//{ &phLibNfc_Get_eSESessionId, &phLibNfc_Verify_eSESessionId },
	{ &phLibNfc_DelayForSeNtf, &phLibNfc_DelayForSeNtfProc },
	{ &phLibNfc_eSESetModeClearAllSeq, &phLibNfc_SetModeSeqEnd },
	{ &phLibNfc_HciGetHostTypeList,&phLibNfc_HciGetHostTypeListProc },
	{ &phLibNfc_HciCreateApduPipe,&phLibNfc_HciCreateApduPipeProc },
	{ &phLibNfc_HciOpenAPDUPipe,&phLibNfc_HciOpenAPDUPipeProc },
	{ NULL, &phLibNfc_eSEClearALLPipeComplete }//{Complete clear all pipes handling};
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
        /* Check if any Wired Mode Transactions are in progress */
        wStatus = phHciNfc_CheckTransOnApduPipe();
        if (wStatus == NFCSTATUS_SUCCESS)
        {
            if ((NULL != pSeTransceiveInfo->sSendData.buffer) && (0 != pSeTransceiveInfo->sSendData.length) &&
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
        else
        {
            /* Returns Status as NFCSTATUS_BUSY*/
            PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSE_Transceive: Stack BUSY");
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
	pphHciNfc_HciContext_t pHciCtxt;
    UNUSED(pInfo);
    UNUSED(status);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->pHciContext)
        {
			pHciCtxt = pLibCtx->pHciContext;
			pHciCtxt->bClearALL_eSE_pipes = FALSE;
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
                /* Enable the white list for embedded SE, so that we will get Admin Pipe Created Notification. 
                   If there is no eSE, then NFCEE discovery ntf for eSE will not be generated and enabling white 
                   list for eSE will not have any side effects*/
                aSetWhiteListSe[bCount++] = phHciNfc_e_SEHostID;

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

static void phLibNfc_SeSendCb(void* pContext, NFCSTATUS status, void* pInfo)
{
    phHciNfc_ReceiveParams_t tRxParams = { 0 };
    phHciNfc_HciContext_t *pHciContext;
    pphLibNfc_Context_t pLibCtx;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if (NULL != pContext)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if (NULL != pLibCtx->pHciContext)
        {
            pHciContext = (phHciNfc_HciContext_t *)pLibCtx->pHciContext;
            if (NULL != pHciContext)
            {
                if (NFCSTATUS_SUCCESS != status)
                {
                    tRxParams.bMsgType = pHciContext->pHciCoreContext.tHciCtxSendMsgParams.bMsgType;
                    tRxParams.bIns = pHciContext->pHciCoreContext.tHciCtxSendMsgParams.bIns;
                    tRxParams.bPipeId = pHciContext->pHciCoreContext.tHciCtxSendMsgParams.bPipeId;
                    tRxParams.pData = NULL;
                    tRxParams.wLen = 0;
                    PH_LOG_LIBNFC_CRIT_STR("SeSendCb: Call back received with status != success");
                    phHciNfc_ReceiveHandler((void *)pHciContext, &tRxParams, NFCSTATUS_SUCCESS);
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
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
                    pHciContext->Cb_Info.pClientInitCb = &phLibNfc_SeSendCb;
                    pHciContext->Cb_Info.pClientCntx = pContext;
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
    PH_LOG_LIBNFC_FUNC_ENTRY();
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
        pLibCtx = (pphLibNfc_Context_t)pContext;
        (void)phLibNfc_HciDataSendProc(pLibCtx, NFCSTATUS_RESPONSE_TIMEOUT, NULL);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}
static NFCSTATUS phLibNfc_HciDataSendProc(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phHciNfc_ReceiveParams_t *pReceivedParams = (phHciNfc_ReceiveParams_t *) pInfo;
    pphLibNfc_Context_t pLibCtx;
    phHciNfc_HciContext_t *pHciContext;
    pphLibNfc_TransceiveCallback_t pClientCb = NULL;
    void *pClientCntx = NULL;

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
                    /* Process the received Data only if Transeive Timeout has not occurred
                    ** NFCSTATUS_BUSY indicates we are waiting for Transeive response
                    */
                    wStatus = phHciNfc_CheckTransOnApduPipe();

                    if (wStatus == NFCSTATUS_BUSY)
                    {
                        wStatus = status;/* Sucessfull scenario retain the status*/
                                         /* Stop and Delete the Transeive Timer Started Earlier */
                        if (pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId != PH_OSALNFC_TIMER_ID_INVALID)
                        {
                            (void)phOsalNfc_Timer_Stop(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId);
                            (void)phOsalNfc_Timer_Delete(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId);
                            pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId = 0;
                            pHciContext->tHciSeTxRxTimerInfo.dwTimeOut = PHHCINFC_DEFAULT_HCI_TX_RX_TIME_OUT;
                            PH_LOG_LIBNFC_INFO_STR("SE TxRx Timer Deleted");
                        }
                        PH_LOG_LIBNFC_INFO_STR("Received valid data");
                        if ((NULL != pLibCtx->pSeTransInfo->sRecvData.buffer) && (0 != pLibCtx->pSeTransInfo->sRecvData.length))
                        {
                            if (pReceivedParams->wLen <= pLibCtx->pSeTransInfo->sRecvData.length)
                            {
                                phOsalNfc_MemCopy(pLibCtx->pSeTransInfo->sRecvData.buffer,
                                    pReceivedParams->pData, pReceivedParams->wLen);
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
                        PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciDataSendProc:eSE Transeive received data after Timeout");
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

        pClientCb = pLibCtx->CBInfo.pSeClientTransCb;
        pClientCntx = pLibCtx->CBInfo.pSeClientTransCntx;
        pLibCtx->CBInfo.pSeClientTransCb = NULL;
        pLibCtx->CBInfo.pSeClientTransCntx = NULL;
        if (NULL != pClientCb)
        {
            pClientCb(pClientCntx, NULL, &pLibCtx->pSeTransInfo->sRecvData, wStatus);
            PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciDataSendProc:APP Callback Invoked");
            /* eSE transceive buffer information clean up */
            pLibCtx->pSeTransInfo = NULL;
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

void
phHciNfc_ProcessEventsOnApduPipe(void *pContext, NFCSTATUS wStatus, void *pInfo)
{
    NFCSTATUS wSETxRxTimerStatus;
    pphHciNfc_HciContext_t    pHciContext = NULL;
    phHciNfc_ReceiveParams_t *pReceivedParams = (phHciNfc_ReceiveParams_t *)pInfo;
    phLibNfc_uSeEvtInfo_t tSeEvtInfo = { 0 };
    phLibNfc_Handle hSecureElement = (phLibNfc_Handle)NULL;
    uint8_t bCount = 0;
    pphLibNfc_LibContext_t  pLibContext = PHLIBNFC_GETCONTEXT();

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if ((NULL != pContext) && (wStatus == NFCSTATUS_SUCCESS)  \
        && (NULL != pLibContext) && (NULL != pReceivedParams))
    {
        pHciContext = (pphHciNfc_HciContext_t)pContext;
        switch (pReceivedParams->bIns)
        {
        case PHHCINFC_PROP_DATA_EVENT:
            (void)phLibNfc_HciDataSendProc(pLibContext, wStatus, pInfo);
            break;
        case PHHCINFC_PROP_EVENT_WTX_REQ:
        {
            PH_LOG_LIBNFC_INFO_STR("phHciNfc_ProcessEventsOnApduPipe:EVENT_WTX_REQ received");
            if (pLibContext->CBInfo.pSeClientEvtWtxCb != NULL)
            {
                /* Assuming Only One Byte of Data will be sent by the eSE, calculate the WTX timeout */
                tSeEvtInfo.tWtxInfo.dwWtxTime = phHciNfc_CalcWtxTimeout(pReceivedParams->pData[0]);
                /* Invoke the upper call back */
                for (bCount = 0; bCount< PHHCINFC_TOTAL_NFCEES; bCount++)
                {
                    if ((pLibContext->tSeInfo.tSeList[bCount].eSE_Type == phLibNfc_SE_Type_eSE))
                    {
                        hSecureElement = pLibContext->tSeInfo.tSeList[bCount].hSecureElement;
                        break;
                    }
                }
                pLibContext->CBInfo.pSeClientEvtWtxCb(pLibContext->CBInfo.pSeClientEvtWtxCntx,
                    hSecureElement,
                    &tSeEvtInfo,
                    NFCSTATUS_WAIT_ON_WTX);
                /* Restart the Transeive Timer with WTX timeout value*/
                if (tSeEvtInfo.tWtxInfo.dwWtxTime > 0)
                {
                }
                else
                {
                    tSeEvtInfo.tWtxInfo.dwWtxTime = PHHCINFC_DEFAULT_HCI_TX_RX_TIME_OUT;
                }
                pHciContext->tHciSeTxRxTimerInfo.dwTimeOut = tSeEvtInfo.tWtxInfo.dwWtxTime;

                (void)phOsalNfc_Timer_Stop(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId);
                wSETxRxTimerStatus = phOsalNfc_Timer_Start(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId,
                    pHciContext->tHciSeTxRxTimerInfo.dwTimeOut,
                    &phHciNfc_eSETranseiveTimeOutCb,
                    pLibContext);
                if (wSETxRxTimerStatus != NFCSTATUS_SUCCESS)
                {
                    phOsalNfc_Timer_Delete(pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId);
                    wStatus = NFCSTATUS_SUCCESS;
                    PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciDataSend: SE TxRx Timer Start Failed");
                }
                else
                {
                    PH_LOG_LIBNFC_INFO_STR("phHciNfc_ProcessEventsOnApduPipe:eSe Transeive Timer restarted with WTX timeout value");
                }
            }
            else
            {
                /* WTX Call Back not defined so dont Invoke it*/
                PH_LOG_LIBNFC_INFO_STR("phHciNfc_ProcessEventsOnApduPipe:WTX Call Back not defined by App");
            }
        }
        break;
		case PHHCINFC_EVENT_ATR_RECV:
			(void)phLibNfc_eSE_GetAtrProc(pContext, wStatus, pInfo);
			break;
        default:
            break;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

static uint32_t phHciNfc_CalcWtxTimeout(uint8_t bWtxBwi)
{
    float dwWtxTimeout;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    bWtxBwi = bWtxBwi >> 4 & 0x0F;
    if (bWtxBwi > PH_LIBNFC_MAX_WTX_BWI_VALUE)
    {
        dwWtxTimeout = 0;
    }
    else
    {
        dwWtxTimeout = (float)phHciNfc_CalcPower(2, bWtxBwi);
        dwWtxTimeout = dwWtxTimeout / PH_LIBNFC_WTX_BWI_DIVISOR;
        dwWtxTimeout = dwWtxTimeout*PH_LIBNFC_WTX_MILLI_SECOND_MULT;
    }
    PH_LOG_LIBNFC_INFO_X32MSG("phHciNfc_CalcWtxTimeout:WTX Timeout", (uint32_t)dwWtxTimeout);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return (uint32_t)dwWtxTimeout;
}

static uint32_t phHciNfc_CalcPower(uint16_t wNum, uint16_t wPow)
{
    uint16_t wCnt = 1;
    uint32_t wSum = 1;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    while (wCnt <= wPow)
    {
        wSum = wSum*wNum;
        wCnt++;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wSum;
}

void phHciNfc_RegisterForEvents(pphHciNfc_HciContext_t pHciContext, uint8_t bPipeId)
{
    phHciNfc_HciRegData_t tHciRegData;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if (bPipeId == pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId)
    {
        /* Register a receive handling Funciton to APDU Pipe */
        PH_LOG_LIBNFC_INFO_STR("phHciNfc_RegisterForEvents:APDU Pipe");
        tHciRegData.eMsgType = phHciNfc_e_HciMsgTypeEvent;
        tHciRegData.bPipeId = bPipeId;
        (void)phHciNfc_RegisterCmdRspEvt(pHciContext,
            &tHciRegData,
            &phHciNfc_ProcessEventsOnApduPipe,
            pHciContext);
    }
    else if (bPipeId == pHciContext->aSEPipeList[PHHCI_ESE_CONN_PIPE_LIST_INDEX].bPipeId ||
        pHciContext->aUICCPipeList[PHHCI_UICC_CONN_PIPE_LIST_INDEX].bPipeId)
    {
        /* Register for Events for the pipe on Connectivity Gate */
        PH_LOG_LIBNFC_INFO_STR("phHciNfc_RegisterForEvents:Connectivity Pipe");
        tHciRegData.eMsgType = phHciNfc_e_HciMsgTypeEvent;
        tHciRegData.bPipeId = bPipeId;
        (void)phHciNfc_RegisterCmdRspEvt(pHciContext,
            &tHciRegData,
            &phHciNfc_ProcessEventsOnPipe,
            pHciContext);
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_U32MSG("No registerations done for PipeId", bPipeId);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
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
                    /* Register for Events for the pipe on APDU Gate Pipe*/
                    phHciNfc_RegisterForEvents(pHciCtx, pReadSessionIdentity->pData[PHHCI_ESE_APDU_PIPE_STORAGE_INDEX]);
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

NFCSTATUS phLibNfc_HciLaunchDevInitSequence(void *pContext)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pLibContext != NULL)
    {
        pLibContext->tSeInfo.bSeState[phLibNfc_SE_Index_HciNwk] = phLibNfc_SeStateInitializing;
        pLibContext->sSeContext.pActiveSeInfo = (pphLibNfc_SE_List_t)(&pLibContext->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk]);

        /*Start the Sequence for HCI network*/
        PHLIBNFC_INIT_SEQUENCE(pLibContext, gphLibNfc_HciInitSequence);
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

static void phHciNfc_eSEGetAtrTimeOutCb(uint32_t dwTimerId, void *pContext)
{
    pphLibNfc_Context_t pLibCtx;
    phHciNfc_HciContext_t *pHciContext;
    PHNFC_UNUSED_VARIABLE(dwTimerId); /* No Data Expected from lower Layer */
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_CRIT_STR("******HCI SE Get Atr Timer Expired********");
    if (pContext != NULL)
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        pHciContext = (phHciNfc_HciContext_t *)pLibCtx->pHciContext;
        (void)phOsalNfc_Timer_Stop(dwTimerId);
        (void)phOsalNfc_Timer_Delete(dwTimerId);
        pHciContext->tHciSeGetAtrTimerInfo.dwTimeOut = PHHCINFC_DEFAULT_HCI_GET_ATR_TIMEOUT;
        pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId = PH_OSALNFC_TIMER_ID_INVALID;
        pLibCtx = (pphLibNfc_Context_t)pContext;
        pLibCtx->pAtrInfo->pBuff = NULL;
        pLibCtx->pAtrInfo->dwLength = 0;
        (void)phLibNfc_eSE_GetAtrProc(pLibCtx, NFCSTATUS_RESPONSE_TIMEOUT, NULL);
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("phHciNfc_eSEGetAtrTimeOutCb:Invalid Context");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

static void phLibNfc_eSE_GetAtrProc(void* pContext, NFCSTATUS status, void* pInfo)
{
    pphLibNfc_Context_t pLibCtx = PHLIBNFC_GETCONTEXT();
    phHciNfc_HciContext_t *pHciContext = NULL;
    phHciNfc_ReceiveParams_t *pReceivedParams;
    pphLibNfc_GetAtrCallback_t pClientCb = NULL;
    void *pClientCntx = NULL;
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if (NULL != pLibCtx)
    {
        if ((status == NFCSTATUS_SUCCESS) && (pInfo != NULL))
        {
            pReceivedParams = (phHciNfc_ReceiveParams_t*)pInfo;
            if ((NULL != pLibCtx->pHciContext) && (pContext == pLibCtx->pHciContext))
            {
                pHciContext = pLibCtx->pHciContext;
                /* Stop the timer started earlier */
                if (pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId != PH_OSALNFC_TIMER_ID_INVALID)
                {
                    (void)phOsalNfc_Timer_Stop(pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId);
                    (void)phOsalNfc_Timer_Delete(pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId);
                    pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId = PH_OSALNFC_TIMER_ID_INVALID;
                    pHciContext->tHciSeGetAtrTimerInfo.dwTimeOut = PHHCINFC_DEFAULT_HCI_TX_RX_TIME_OUT;
                    PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciDataSendProc:SE Get Atr Timer Deleted");
                }
                /* Process the received Data only if Transeive Timeout has not occurred
                ** NFCSTATUS_BUSY indicates we are waiting for Transeive response
                */
                wStatus = phHciNfc_CheckTransOnApduPipe();
                if (wStatus == NFCSTATUS_BUSY)
                {
                    pLibCtx->pAtrInfo->dwLength = 0x108;
                    if (pReceivedParams->wLen <= pLibCtx->pAtrInfo->dwLength)
                    {
                        pLibCtx->pAtrInfo->dwLength = pReceivedParams->wLen;
                        phOsalNfc_MemCopy(pLibCtx->pAtrInfo->pBuff, pReceivedParams->pData, pReceivedParams->wLen);
                        /* TBD - Calculate the BWI time (command waiting time)*/
                        /* As per Spec it is BWI = 2^TB1/10 secs */
                        wStatus = NFCSTATUS_SUCCESS;
                        PH_LOG_LIBNFC_INFO_STR("phLibNfc_eSE_GetAtrProc: Sucessfull");
                    }
                    else
                    {
                        PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSE_GetAtrProc: Invalid Input Buffer Length");
                        wStatus = NFCSTATUS_BUFFER_TOO_SMALL;
                    }
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSE_GetAtrProc:eSE Get Atr received data after Timeout!!!");
                }
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSE_GetAtrProc: Invalid Hci context received!");
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSE_GetAtrProc: Received FAILED status or pInfo Invalid");
        }
        pClientCb = pLibCtx->CBInfo.pSeClientGetAtrCb;
        pClientCntx = pLibCtx->CBInfo.pSeClientGetAtrCntx;
        pLibCtx->CBInfo.pSeClientGetAtrCb = NULL;
        pLibCtx->CBInfo.pSeClientGetAtrCntx = NULL;
        /* Invoke Uppler Layer Callback*/
        if (NULL != pClientCb)
        {
            pClientCb(pClientCntx, pLibCtx->pAtrInfo, wStatus);
            PH_LOG_LIBNFC_INFO_STR("phLibNfc_eSE_GetAtrProc: App layer Callback Invoked");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSE_GetAtrProc: LibNfc context invalid");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS
phLibNfc_eSE_GetAtr(phLibNfc_Handle                 hSE_Handle,
    phNfc_sSeAtrInfo_t*          pAtrInfo,
    pphLibNfc_GetAtrCallback_t      pGetAtr_RspCb,
    void*                           pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    NFCSTATUS wSEGetAtrTimerStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t pLibCtx = PHLIBNFC_GETCONTEXT();
    uint8_t bCount = 0;
    phHciNfc_HciContext_t *pHciContext = NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = phLibNfc_IsInitialised(PHLIBNFC_GETCONTEXT());
    if ((NFCSTATUS_SUCCESS != wStatus) || (NULL == pLibCtx))
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if ((NULL == (void *)hSE_Handle) || (NULL == pGetAtr_RspCb) ||
        (NULL == pAtrInfo))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameters");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Check if any Wired Mode Transactions are in progress */
        wStatus = phHciNfc_CheckTransOnApduPipe();
        if (wStatus == NFCSTATUS_SUCCESS)
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
            if ((NULL != pAtrInfo->pBuff) && (0 != pAtrInfo->dwLength))
            {
                for (bCount = 0; bCount< PHHCINFC_TOTAL_NFCEES; bCount++)
                {
                    if ((pLibCtx->tSeInfo.tSeList[bCount].hSecureElement == hSE_Handle) && \
                        (pLibCtx->tSeInfo.tSeList[bCount].eSE_Type == phLibNfc_SE_Type_eSE) && \
                        (pLibCtx->tSeInfo.tSeList[bCount].eSE_ActivationMode == phLibNfc_SE_ActModeApdu))
                    {
                        wStatus = NFCSTATUS_SUCCESS;
                        break;
                    }
                }
                pHciContext = (phHciNfc_HciContext_t *)pLibCtx->pHciContext;
                if ((NULL != pLibCtx->pHciContext) && (wStatus == NFCSTATUS_SUCCESS))
                {
                    if ((pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId != PHHCINFC_NO_PIPE_DATA) &&
                        (pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bGateId != PHHCINFC_NO_PIPE_DATA))
                    {
						if (pHciContext->eSE_Compliancy == PHHCINFC_ESE_COMPLIANT_9)
						{
                        wStatus = phHciNfc_AnyGetParameter(
                            pLibCtx->pHciContext,
                            pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bGateId,
                            PHHCINFC_APDU_GATE_ATR_REG_ID,
                            pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId,
                            &phLibNfc_eSE_GetAtrProc,
                            pLibCtx->pHciContext);
						}
						else if (pHciContext->eSE_Compliancy == PHHCINFC_ESE_COMPLIANT_12)
						{
							wStatus = phHciNfc_eSE_EvtAbort(
								pLibCtx->pHciContext,
								pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId,
								&phLibNfc_eSE_GetAtrProc,
								pLibCtx->pHciContext);
						}
						else
						{
							wStatus = NFCSTATUS_INVALID_PARAMETER;
							PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSE_GetAtr Failed No proper compliancy!");
						}
                        if (NFCSTATUS_PENDING != wStatus)
                        {
                            PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSE_GetAtr Failed!");
                            wStatus = NFCSTATUS_FAILED;
                            pLibCtx->CBInfo.pSeClientGetAtrCb = NULL;
                            pLibCtx->CBInfo.pSeClientGetAtrCntx = NULL;
                        }
                        else
                        {
                            PH_LOG_LIBNFC_INFO_STR("phLibNfc_eSE_GetAtr Cmd Sent to lower layer");
                            /* Store upper layer call back and its context */
                            pLibCtx->CBInfo.pSeClientGetAtrCb = pGetAtr_RspCb;
                            pLibCtx->CBInfo.pSeClientGetAtrCntx = pContext;
                            pLibCtx->pAtrInfo = pAtrInfo;

                            wSEGetAtrTimerStatus = phHciNfc_CreateSEGetAtrTimer(pHciContext);
                            if (wSEGetAtrTimerStatus == NFCSTATUS_SUCCESS)
                            {
                                /* Start the SE TxRx Timer*/
                                pHciContext = (phHciNfc_HciContext_t *)pLibCtx->pHciContext;
                                pHciContext->tHciSeGetAtrTimerInfo.dwTimeOut = PHHCINFC_DEFAULT_HCI_TX_RX_TIME_OUT;
                                wSEGetAtrTimerStatus = phOsalNfc_Timer_Start(pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId,
                                    pHciContext->tHciSeGetAtrTimerInfo.dwTimeOut,
                                    &phHciNfc_eSEGetAtrTimeOutCb,
                                    pLibCtx);
                                if (wSEGetAtrTimerStatus != NFCSTATUS_SUCCESS)
                                {
                                    //wStatus = phOsalNfc_Timer_Delete(pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId);
                                    phOsalNfc_Timer_Delete(pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId);
                                    PH_LOG_LIBNFC_CRIT_STR("SE Get Atr Timer Start Failed");
                                }
                            }
                        }
                    }
                    else
                    {
                        PH_LOG_LIBNFC_CRIT_STR("Failed, Pipe Info not Available");
                        wStatus = NFCSTATUS_FAILED;
                    }
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("Invalid Hci context received or Invalid SE Handle");
                }
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Invalid Input Buffer!");
            }
        }
        else
        {
            /* Returns NFCSTATUS_BUSY to the Application*/
            PH_LOG_LIBNFC_CRIT_STR("BUSY, eSE Transeive or eSE_GetAtr API is in progress");
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phHciNfc_CheckTransOnApduPipe(void)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphLibNfc_Context_t pLibCtx = PHLIBNFC_GETCONTEXT();
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if (pLibCtx->CBInfo.pSeClientTransCb != NULL ||
        pLibCtx->CBInfo.pSeClientGetAtrCb != NULL)
    {
        /* Indicates that either eSE Transeive or
        ** eSE_GetAtr API is in progress
        */
        wStatus = NFCSTATUS_BUSY;
        PH_LOG_LIBNFC_INFO_STR("phHciNfc_CheckTransOnApduPipe:eSE Transeive or Get ATR API in progress");
    }
    else
    {
        wStatus = NFCSTATUS_SUCCESS;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phHciNfc_CreateSEGetAtrTimer(phHciNfc_HciContext_t  *pHciContext)
{

    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    /* Create a TImer for HCI Send and Receive */
    pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId = phOsalNfc_Timer_Create();
    if ((0 == pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId) ||
        (PH_OSALNFC_TIMER_ID_INVALID == pHciContext->tHciSeGetAtrTimerInfo.dwRspTimerId))
    {
        PH_LOG_LIBNFC_CRIT_STR("HCI SE Get Atr Timer Create failed");
        wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("HCI SE Get Atr Timer Created Successfully");
        wStatus = NFCSTATUS_SUCCESS;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

/////////////////////////////////////////////////
static NFCSTATUS phLibNfc_HciSetHostType(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
	pphLibNfc_Context_t pLibCtx;
	uint8_t bIndex = PHHCINFC_PIPE_HOST_TYPE_INDEX;
	uint8_t aHostType[2] = { 0x01,0x00 };
	UNUSED(pInfo);
	UNUSED(status);
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if (NULL != pContext)
	{
		pLibCtx = (pphLibNfc_Context_t)pContext;

		if (NULL != pLibCtx->pHciContext)
		{
			wStatus = phHciNfc_AnySetParameter(pLibCtx->pHciContext,
				bIndex,
				PHHCINFC_PIPE_ADMIN_PIPEID,
				2,
				aHostType,
				&phLibNfc_InternalSequence,
				pContext);
			if (wStatus != NFCSTATUS_PENDING)
			{
				PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciSetHostType:Failed to Send Cmd to NCI");
			}
			else
			{
				PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciSetHostType:Cmd Sent to NCI");
			}
		}
		else
		{
			PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciSetHostType: Invalid Hci context received!");
		}
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciSetHostType: Invalid LibNfc context received!");
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}

static NFCSTATUS phLibNfc_HciSetHostTypeProc(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
	pphLibNfc_Context_t pLibCtx;
	UNUSED(pInfo);
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if (NULL != pContext)
	{
		pLibCtx = (pphLibNfc_Context_t)pContext;
		if (NULL != pLibCtx->pHciContext)
		{
			wStatus = status;
			PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciSetHostTypeProc: Sucessfull");
			PH_LOG_LIBNFC_INFO_X32MSG("phLibNfc_HciSetHostTypeProc: Sucessfull ", wStatus);
		}
		else
		{
			PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciSetHostTypeProc: Invalid Hci context received!");
		}
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciSetHostTypeProc: Invalid LibNfc context received!");
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}

static bool_t phHciNfc_Check_eSECompliancy(uint8_t *p_data, uint8_t data_len)
{
	uint8_t               noofhosts = 0;
	uint8_t               count = 0;
	uint8_t               host_id = 0;


	noofhosts = ((data_len / PHHCINFC_PIPE_HOST_TYPE_LIST_LEN) - PHHCINFC_PIPE_HOST_TYPE_LIST_LEN);
	PH_LOG_HCI_INFO_X32MSG("phHciNfc_Check_eSECompliancy: No of Hosts in the N/w:-", noofhosts);
	for (count = 0; count < data_len; count++)
	{
		PH_LOG_HCI_INFO_X32MSG("phHciNfc_Check_eSECompliancy: Host Types :-", p_data[count]);
	}
	for (count = 0; count < noofhosts; count++)
	{
		host_id = (((count + 1) * PHHCINFC_PIPE_HOST_TYPE_LIST_LEN) + PHHCINFC_PIPE_HOST_TYPE_LIST_LEN);
		PH_LOG_HCI_INFO_X32MSG("phHciNfc_Check_eSECompliancy: Host Type List Index :-", host_id);
		if ((p_data[host_id] & p_data[host_id + 1]) != 0xFF)
		{
			if ((p_data[host_id] == 0x03) && (p_data[host_id + 1] == 0x00))
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

NFCSTATUS phLibNfc_HciGetHostTypeList(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
	pphLibNfc_Context_t pLibCtx;
	UNUSED(pInfo);
	UNUSED(status);
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if (NULL != pContext)
	{
		pLibCtx = (pphLibNfc_Context_t)pContext;
		if (NULL != pLibCtx->pHciContext)
		{
			/*Get the Host Type list on HCI network*/
			wStatus = phHciNfc_AnyGetParameter(
				pLibCtx->pHciContext,
				(uint8_t)PHHCINFC_ADM_GATEID,
				PHHCINFC_PIPE_HOST_TYPE_LIST_INDEX,
				(uint8_t)PHHCINFC_PIPE_ADMIN_PIPEID,
				&phLibNfc_InternalSequence,
				pContext
			);
			if (NFCSTATUS_PENDING != wStatus)
			{
				PH_LOG_LIBNFC_CRIT_X32MSG("phLibNfc_HciGetHostTypeList: Failed to Get Host Type List, error", wStatus);
				wStatus = NFCSTATUS_FAILED;
			}
			else
			{
				PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciGetHostTypeList: Cmd Sent to lower layer");
			}
		}
		else
		{
			PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciGetHostTypeList: Invalid Hci context passed!");
		}
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciGetHostTypeList: Invalid LibNfc context passed!");
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}

NFCSTATUS phLibNfc_HciGetHostTypeListProc(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
	pphLibNfc_Context_t pLibCtx;
	phHciNfc_HciContext_t *pHciCtx;
	phHciNfc_ReceiveParams_t *pReadHostTypeList = NULL;
	uint8_t bLen = 0;

	PH_LOG_LIBNFC_FUNC_ENTRY();
	if (NULL != pContext)
	{
		pLibCtx = (pphLibNfc_Context_t)pContext;
		pHciCtx = pLibCtx->pHciContext;

		if (NULL != pLibCtx->pHciContext && NULL != pInfo)
		{
			if (NFCSTATUS_SUCCESS == status)
			{
				/*Read Host list Success*/
				wStatus = status;
				pReadHostTypeList = (phHciNfc_ReceiveParams_t *)pInfo;
				phOsalNfc_MemCopy(pHciCtx->aHostTypeList, pReadHostTypeList->pData, pReadHostTypeList->wLen);
				bLen = (uint8_t)pReadHostTypeList->wLen;
				if (phHciNfc_Check_eSECompliancy(pReadHostTypeList->pData, bLen))
				{
					pHciCtx->eSE_Compliancy = PHHCINFC_ESE_COMPLIANT_12;
					//pHciCtx->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bGateId = phHciNfc_e_ApduGate;
					PH_LOG_LIBNFC_CRIT_STR("eSE  is ETSI 12 Compliant!!!!");
				}
				else
				{
					pHciCtx->eSE_Compliancy = PHHCINFC_ESE_COMPLIANT_9;
					//pHciCtx->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bGateId = phHciNfc_e_PropApduGate;
					PH_LOG_LIBNFC_CRIT_STR("eSE  is ETSI 9 Compliant!!!!");
				}
				PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciGetHostTypeListProc:Read Host TypeList successfully ");
			}
			else
			{
				PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciGetHostTypeListProc: Failed to Read HostType List");
				wStatus = NFCSTATUS_FAILED;
			}
		}
		else
		{
			PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciGetHostTypeListProc: Invalid Hci context received!");
		}
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciGetHostTypeListProc: Invalid LibNfc context received!");
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}

NFCSTATUS phLibNfc_HciCreateApduPipe(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
	pphLibNfc_Context_t pLibCtx;
	phHciNfc_HciContext_t *pHciContext;
	UNUSED(pInfo);
	UNUSED(status);
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if (NULL != pContext)
	{
		pLibCtx = (pphLibNfc_Context_t)pContext;
		if (NULL != pLibCtx->pHciContext)
		{
			PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciCreateApduPipe:Creating pipe at APDU Gate!!!");
			pHciContext = (phHciNfc_HciContext_t*)pLibCtx->pHciContext;
			PH_LOG_LIBNFC_INFO_X32MSG("phLibNfc_HciCreateApduPipe:pipe data", pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId);
			/* Check if  APDU Pipe is already Present */
			if (((pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId == PHHCINFC_NO_PIPE_DATA) ||
				(pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId == 0x00)) &&
				(pHciContext->eSE_Compliancy == PHHCINFC_ESE_COMPLIANT_12))
			{
				pHciContext->bClearpipes = 0x00;
				wStatus = phHciNfc_CreateApduPipe(pLibCtx->pHciContext, (uint8_t)PHHCINFC_PIPE_ADMIN_PIPEID,
					&phLibNfc_InternalSequence,
					pContext);
				if (NFCSTATUS_PENDING != wStatus)
				{
					PH_LOG_LIBNFC_CRIT_X32MSG("phLibNfc_HciCreateApduPipe:Failed to create pipe for ADM, error", wStatus);
					wStatus = NFCSTATUS_FAILED;
				}
				else
				{
					PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciCreateApduPipe:APDU Create Cmd Sent to NCI");
				}
			}
			else
			{
				/*APDU Pipe is already created*/
				PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciCreateApduPipe:Pipe is present at APDU Gate!!!");
				wStatus = NFCSTATUS_SUCCESS;
			}
		}
		else
		{
			PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciCreateApduPipe: Invalid Hci context passed!");
		}
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciCreateApduPipe: Invalid LibNfc context passed!");
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}

NFCSTATUS phLibNfc_HciCreateApduPipeProc(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
	pphLibNfc_Context_t pLibCtx;
	pphHciNfc_HciContext_t    pHciContext;
	PH_LOG_LIBNFC_FUNC_ENTRY();
	UNUSED(pInfo);
	if (NULL != pContext)
	{
		pLibCtx = (pphLibNfc_Context_t)pContext;
		if (NULL != pLibCtx->pHciContext)
		{
			PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciCreateApduPipeProc:Create pipe Response!!!");
			pHciContext = (pphHciNfc_HciContext_t)pLibCtx->pHciContext;
			if (NFCSTATUS_SUCCESS == status)
			{
				wStatus = status;
				PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciCreateApduPipeProc:Successfull");
				phHciNfc_RegisterForEvents(pLibCtx->pHciContext, pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId);
			}
			else
			{
				PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciCreateApduPipeProc: Failed to Create APDU pipe");
				pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId = 0xFF;
				wStatus = NFCSTATUS_FAILED;
			}
		}
		else
		{
			PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciCreateApduPipeProc: Invalid Hci context received!");
		}
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciCreateApduPipeProc: Invalid LibNfc context received!");
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}

NFCSTATUS phLibNfc_HciOpenAPDUPipe(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
	pphLibNfc_Context_t pLibCtx;
	pphHciNfc_HciContext_t    pHciContext;
	UNUSED(pInfo);
	UNUSED(status);
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if (NULL != pContext)
	{
		pLibCtx = (pphLibNfc_Context_t)pContext;
		if (NULL != pLibCtx->pHciContext)
		{
			pHciContext = (pphHciNfc_HciContext_t)pLibCtx->pHciContext;
			PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciOpenAPDUPipe:Entered!!!");
			if (pHciContext->eSE_Compliancy == PHHCINFC_ESE_COMPLIANT_12)
			{
				/* Open a pipe to APDU (Admin) gate */
				wStatus = phHciNfc_OpenPipe(pLibCtx->pHciContext,
					(uint8_t)pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId,
					&phLibNfc_InternalSequence,
					pContext);
				if (NFCSTATUS_PENDING != wStatus)
				{
					PH_LOG_LIBNFC_CRIT_X32MSG("phLibNfc_HciOpenAPDUPipe:Failed to open APDU pipe, error", wStatus);
					wStatus = NFCSTATUS_FAILED;
				}
				else
				{
					PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciOpenAPDUPipe:APDU Open pipe Cmd Sent to NCI");
				}
			}
			else
			{
				/*APDU Pipe is already created*/
				wStatus = NFCSTATUS_SUCCESS;
			}
		}
		else
		{
			PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciOpenAPDUPipe: Invalid Hci context passed!");
		}
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciOpenAPDUPipe: Invalid LibNfc context passed!");
	}

	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}
NFCSTATUS phLibNfc_HciOpenAPDUPipeProc(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
	pphLibNfc_Context_t pLibCtx;
	pphHciNfc_HciContext_t    pHciContext;
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if (NULL != pContext)
	{
		pLibCtx = (pphLibNfc_Context_t)pContext;
		if (NULL != pLibCtx->pHciContext)
		{
			PH_LOG_LIBNFC_INFO_STR("phLibNfc_HciOpenAPDUPipeProc:Entered!!!");
			pHciContext = (pphHciNfc_HciContext_t)pLibCtx->pHciContext;
			if (NFCSTATUS_SUCCESS == status)
			{
				wStatus = status;
				if (pInfo != NULL)
				{
					PH_LOG_LIBNFC_INFO_U32MSG("Number of APDU pipes opened", *(uint8_t *)pInfo);
				}
				PH_LOG_LIBNFC_INFO_STR("APDU Pipe Opened Sucessfully");
			}
			else
			{
				pHciContext->aSEPipeList[PHHCI_ESE_APDU_PIPE_LIST_INDEX].bPipeId = 0xFF;
				PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciOpenAPDUPipeProc: Failed to open APDU pipe");
				wStatus = NFCSTATUS_SUCCESS;
			}
		}
		else
		{
			PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciOpenAdmPipeProc: Invalid Hci context received!");
		}
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_HciOpenAdmPipeProc: Invalid LibNfc context received!");
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}

static void phLibNfc_NfceeNtfDelayCb(uint32_t dwTimerId, void *pContext)
{
	pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
	/* Internal event is made failed in order to remain in same state */
	NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if ((NULL != pLibContext) && (PHLIBNFC_GETCONTEXT() == pLibContext))
	{
		/* Stop timer and delete the same */
		(void)phOsalNfc_Timer_Stop(dwTimerId);
		(void)phOsalNfc_Timer_Delete(dwTimerId);

		(void)phLibNfc_SeqHandler(pLibContext, wStatus, NULL);
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return;
}

NFCSTATUS phLibNfc_Delay_eSEPipes(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = status;
	pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
	UNUSED(pInfo);
	PH_LOG_LIBNFC_INFO_STR("phLibNfc_Delay_eSEPipes Entered!!!");

	if ((NULL != pCtx) && (NFCSTATUS_SUCCESS == wStatus))
	{
		/* Increment the delay counter */
		pCtx->eSE_delay += PH_LIBNFC_INCR_DELAY_ESE_DELAY;
		PH_LOG_LIBNFC_INFO_U32MSG("Delay to pipe create at eSE", pCtx->eSE_delay);
		wStatus = phLibNfc_DummyDisc(pCtx, &phLibNfc_NfceeNtfDelayCb, pCtx->eSE_delay);
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}
NFCSTATUS phLibNfc_Delay_eSEPipes_Process(void* pContext, NFCSTATUS wStatus, void* pInfo)
{
	NFCSTATUS Status = wStatus;
	pphLibNfc_LibContext_t pLibContext = pContext;
	UNUSED(pInfo);
	PH_LOG_LIBNFC_FUNC_ENTRY();
	PH_LOG_LIBNFC_INFO_STR("phLibNfc_Delay_eSEPipes_Process Entered!!!");
	PH_LOG_LIBNFC_CRIT_STR("Wait for eSE to Initialize ...");
	if ((NULL != pLibContext) && (NFCSTATUS_SUCCESS == Status))
	{
		if (pLibContext->eSE_delay < PH_LIBNFC_INCR_DELAY_ESE_DELAY_TIMEOUT)
		{
			PH_LOG_LIBNFC_INFO_STR("Repeating Delay sequence !!!");
			(void)phLibNfc_RepeatSequenceSeq((void *)pLibContext, gphLibNfc_eSEHandleClearAllPipesSequence, 1);
		}
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return Status;
}

NFCSTATUS phLibNfc_Get_eSESessionId(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = status;
	pphLibNfc_Context_t pLibCtx = (pphLibNfc_Context_t)pContext;
	uint8_t bConfigInfoLength = 3;
	uint8_t aConfigInfo[] = { 0x01,0xA0,0xEB };
	UNUSED(pInfo);
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if ((NFCSTATUS_SUCCESS == wStatus) && (pLibCtx != NULL))
	{
		/* Get Session ID for the connected NFCEE*/
		wStatus = phNciNfc_GetConfigRaw(pLibCtx->sHwReference.pNciHandle,
			aConfigInfo, bConfigInfoLength,
			(pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
			pContext);
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}


NFCSTATUS phLibNfc_Verify_eSESessionId(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = status;
	NFCSTATUS wIntStatus = NFCSTATUS_FAILED;
	pphLibNfc_Context_t pLibCtx = (pphLibNfc_Context_t)pContext;
	uint8_t *pConfigParam;
	uint8_t aTempSessionId[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
	pConfigParam = (uint8_t *)pInfo;
	PH_LOG_LIBNFC_FUNC_ENTRY();

	if ((NFCSTATUS_SUCCESS == wStatus) && (pLibCtx != NULL))
	{
		if (pConfigParam != NULL)
		{
			if (!phOsalNfc_MemCompare(aTempSessionId, &pConfigParam[6], pConfigParam[5]))
			{
				wIntStatus = NFCSTATUS_SUCCESS;
				PH_LOG_LIBNFC_CRIT_STR("Check again eSE init !!!");
				(void)phLibNfc_RepeatSequenceSeq((void *)pLibCtx, gphLibNfc_eSEHandleClearAllPipesSequence, 1);
			}
			else
			{
				wStatus = NFCSTATUS_SUCCESS;
				PH_LOG_LIBNFC_CRIT_STR("eSE is Initialized!!!");
			}
		}
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wStatus;
}

NFCSTATUS phLibNfc_eSESetModeClearAllSeq(void *pContext,
	NFCSTATUS wStatus,
	void *pInfo)
{
	NFCSTATUS wIntStatus = wStatus;
	pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
	//pphHciNfc_HciContext_t    pHciContext;
	UNUSED(pInfo);
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if ((NULL != pLibContext) && (PHLIBNFC_GETCONTEXT() == pLibContext))
	{
		if (NFCSTATUS_SUCCESS == wIntStatus)
		{
			/* Set SE mode */
			PH_LOG_LIBNFC_INFO_STR("phLibNfc_eSESetModeClearAllSeq:eSE is needs to be Initialized!!!");
			wIntStatus = phNciNfc_Nfcee_ModeSet(pLibContext->sHwReference.pNciHandle,
				(void *)pLibContext->sSeContext.pActiveSeInfo->hSecureElement, /*Should be NFCEE Handle*/
				PH_NCINFC_EXT_NFCEEMODE_ENABLE,
				(pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
				(void *)pLibContext);
		}
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSESetModeClearAllSeq: Invalid LibNFc Context!");
	}

	PH_LOG_LIBNFC_FUNC_EXIT();
	return wIntStatus;
}

NFCSTATUS phLibNfc_SetModeSeqEnd(void *pContext, NFCSTATUS wStatus, void *pInfo)
{
	NFCSTATUS wIntStatus = NFCSTATUS_FAILED;
	pphLibNfc_LibContext_t pLibContext = pContext;
	UNUSED(pInfo);
	PH_LOG_LIBNFC_FUNC_ENTRY();
	if ((NULL != pLibContext) && (PHLIBNFC_GETCONTEXT() == pLibContext))
	{
		wIntStatus = wStatus;
	}
	PH_LOG_LIBNFC_FUNC_EXIT();
	return wIntStatus;
}

NFCSTATUS phLibNfc_eSEClearALLPipeComplete(void* pContext, NFCSTATUS status, void* pInfo)
{
	NFCSTATUS wStatus = status;
	pphLibNfc_LibContext_t pLibCtx;
	pphHciNfc_HciContext_t pHciContext;
	UNUSED(pContext);
	UNUSED(pInfo);

	if ((pContext != NULL) && (NFCSTATUS_SUCCESS == wStatus))
	{
		pLibCtx = (pphLibNfc_Context_t)pContext;
		pHciContext = pLibCtx->pHciContext;
		//pHciContext->bClearALL_eSE_pipes = FALSE;
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSEClearALLPipeComplete: Passed!!");
	}
	else
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_eSEClearALLPipeComplete: Failed!!");
		return NFCSTATUS_FAILED;
	}
	return wStatus;
}

void phHciNfc_Process_eSE_ClearALLPipes(void)
{
	pphLibNfc_Context_t pLibCtx = PHLIBNFC_GETCONTEXT();
	//    phHciNfc_HciContext_t *pHciContext= NULL;
	NFCSTATUS wStatus = NFCSTATUS_FAILED;

	pLibCtx->eSE_delay = 0x00;
	PH_LOG_HCI_INFO_X32MSG("phHciNfc_Process_eSE_ClearALLPipes", pLibCtx->eSE_delay);
	PH_LOG_LIBNFC_CRIT_STR("phHciNfc_Process_eSE_ClearALLPipes: entered!!!");
	

	
	PHLIBNFC_INIT_SEQUENCE(pLibCtx, gphLibNfc_eSEHandleClearAllPipesSequence);
	wStatus = phLibNfc_SeqHandler(pLibCtx, NFCSTATUS_SUCCESS, NULL);
	if (NFCSTATUS_PENDING != wStatus)
	{
		PH_LOG_LIBNFC_CRIT_STR("phLibNfc_Launch_eSE_ClearALLPipes_Sequence: could not start!");
		wStatus = NFCSTATUS_FAILED;
	}
	else
	{
		PH_LOG_LIBNFC_INFO_STR("phLibNfc_Launch_eSE_ClearALLPipes_Sequence:  sequence started");
	}
	
}