/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Se.tmh"

static NFCSTATUS phLibNfc_SetModeSeq(void *pContext, NFCSTATUS wStatus, void *pInfo);
static NFCSTATUS phLibNfc_SetModeSeqEnd(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_SetSeModeSeqComplete(void* pContext, NFCSTATUS Status, void *pInfo);

static NFCSTATUS phLibNfc_SePowerAndLinkCtrlSeq(void *pContext, NFCSTATUS wStatus, void *pInfo);
static NFCSTATUS phLibNfc_SePowerAndLinkCtrlEnd(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_SePowerAndLinkCtrlCompleteSequence(void* pContext, NFCSTATUS Status, void *pInfo);

static NFCSTATUS phLibNfc_StartNfceeDisc(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_ProcessStartNfceeDiscRsp(void* pContext, NFCSTATUS wStatus, void* pInfo);
static NFCSTATUS phLibNfc_NfceeStartDiscSeqComplete(void *pContext, NFCSTATUS Status, void *pInfo);

static NFCSTATUS phLibNfc_StopNfceeDisc(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_ProcessStopNfceeDiscRsp(void* pContext, NFCSTATUS wStatus, void* pInfo);
static NFCSTATUS phLibNfc_NfceeDiscSeqComplete(void* pContext, NFCSTATUS status, void* pInfo);

static NFCSTATUS phLibNfc_StartWdTimer(void *pContext, pphOsalNfc_TimerCallbck_t pTimerCb, uint32_t dwTimeOut);
static void phLibNfc_WdTimerExpiredCb(uint32_t dwTimerId, void *pContext);

static void phLibNfc_UpdateSeInfo(void* pContext, pphNciNfc_NfceeInfo_t pNfceeInfo, NFCSTATUS Status);
static void phLibNfc_UpdateNfceeDiscTechnType(void* pLibContext, pphNciNfc_NfceeDiscReqNtfInfo_t pNfceeDiscReq, NFCSTATUS Status);

static NFCSTATUS
phLibNfc_UpdateRtngCfg(pphNciNfc_RtngConfig_t  pRtngBuff,
                       uint8_t                 bNumRtngConfigs,
                       phLibNfc_RtngConfig_t   *pRtngCfg,
                       uint8_t                 *pNumEntriesAdded);

static NFCSTATUS
phLibNfc_UpdateRtngInfo(pphNciNfc_RtngConfig_t pNciRtngCfg,
                        phLibNfc_RtngConfig_t  *pLibNfcRtngCfg,
                        uint8_t                bNfceeId);

static NFCSTATUS phLibNfc_ValidateInputRtngInfo(uint8_t bNumRtngConfigs, phLibNfc_RtngConfig_t *pRoutingCfg);

/* NFCEE set mode sequence */
phLibNfc_Sequence_t gphLibNfc_SetSeModeSeq[] = {
    {&phLibNfc_SetModeSeq, &phLibNfc_SetModeSeqEnd},
    {&phLibNfc_DelayForSeNtf, &phLibNfc_DelayForSeNtfProc},
    {NULL, &phLibNfc_SetSeModeSeqComplete}
};

/* NFCEE power and link control sequence */
phLibNfc_Sequence_t gphLibNfc_SePowerAndLinkCtrlSeq[] = {
    { &phLibNfc_SePowerAndLinkCtrlSeq, &phLibNfc_SePowerAndLinkCtrlEnd },
    { NULL, &phLibNfc_SePowerAndLinkCtrlCompleteSequence }
};

/* NFCEE discovery sequence */
phLibNfc_Sequence_t gphLibNfc_NfceeStartDiscSeq[] = {
    {&phLibNfc_StartNfceeDisc, &phLibNfc_ProcessStartNfceeDiscRsp},
    {NULL, &phLibNfc_NfceeStartDiscSeqComplete}
};

/* NFCEE discovery complete sequence */
phLibNfc_Sequence_t gphLibNfc_NfceeDiscCompleteSeq[] = {
    {&phLibNfc_HciSetSessionIdentity, &phLibNfc_HciSetSessionIdentityProc},
    {NULL, &phLibNfc_NfceeDiscSeqComplete}
};

NFCSTATUS phLibNfc_SE_Enumerate(pphLibNfc_RspCb_t     pSEDiscoveryCb,
                                void*                 pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pCtx = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pCtx);

    if (NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_LIBNFC_CRIT_STR("Libnfc Stack is not initialised");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if ((NULL == pSEDiscoveryCb) || (NULL == pContext))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameters");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if ((TRUE == pCtx->status.GenCb_pending_status) || (NULL != pCtx->CBInfo.pNFCEEDiscoveryCb))
    {
        /* Previous callback is pending */
        wStatus = NFCSTATUS_REJECTED;
    }
    else if (pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk].hSecureElement == NULL)
    {
        pCtx->dwHciInitDelay = 300;

        PHLIBNFC_INIT_SEQUENCE(pCtx, gphLibNfc_NfceeStartDiscSeq);
        wStatus = phLibNfc_SeqHandler(pCtx,wStatus,NULL);

        if (NFCSTATUS_PENDING != wStatus)
        {
            PH_LOG_LIBNFC_CRIT_STR("NFCEE discovery sequence could not start!");
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            pCtx->CBInfo.pNFCEEDiscoveryCb = pSEDiscoveryCb;
            pCtx->CBInfo.pNFCEEDiscoveryCntx = pContext;
        }
    }
    else
    {
        wStatus = NFCSTATUS_ALREADY_INITIALISED;
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SE_NtfRegister   (pphLibNfc_SE_NotificationCb_t  pSE_NotificationCb,
                                     void   *                       pContext
    )
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pCtx);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        wStatus=NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL == pSE_NotificationCb)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(pCtx->StateContext.TrgtState == phLibNfc_StateReset)
    {
        PH_LOG_LIBNFC_INFO_STR("De-Initialize in progress");
        wStatus = NFCSTATUS_SHUTDOWN;
    }
    else
    {
        pCtx->CBInfo.pSeListenerNtfCb = pSE_NotificationCb;
        pCtx->CBInfo.pSeListenerCtxt = pContext;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SE_NtfUnregister(void)
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pCtx);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        wStatus=NFCSTATUS_NOT_INITIALISED;
    }
    else if(pCtx->StateContext.TrgtState == phLibNfc_StateReset)
    {
        PH_LOG_LIBNFC_INFO_STR("De-Initialize in progress");
        wStatus = NFCSTATUS_SHUTDOWN;
    }
    else
    {
        /* Reset the Callback to be invoked upon getting Secure element notifications */
        /* Register with Nci for notification */
        wStatus = phNciNfc_DeregisterNotification((void *)pCtx->sHwReference.pNciHandle,\
                                phNciNfc_e_RegisterSecureElement);
        pCtx->CBInfo.pSeListenerNtfCb = NULL;
        pCtx->CBInfo.pSeListenerCtxt = NULL;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SE_GetSecureElementList( _Out_writes_to_(PHLIBNFC_MAXNO_OF_SE, *uSE_count) phLibNfc_SE_List_t* pSE_List,
                                            _Out_range_(0, PHLIBNFC_MAXNO_OF_SE) uint8_t* uSE_count )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pLibContext = phLibNfc_GetContext();
    phHciNfc_HciContext_t *pHciCtx;
    uint8_t bIndex = 0;
    uint8_t bCount = 0;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pLibContext);

    if(NFCSTATUS_SUCCESS != wStatus)
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if( (NULL == pSE_List) || (NULL == uSE_count) )
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        if(1 == pLibContext->Config.bHciNwkPerNfcee)
        {
            pHciCtx = pLibContext->pHciContext;

            if((NULL != pHciCtx) && (1 == pHciCtx->hostListSize))
            {
                bCount++;
                pSE_List[0].eSE_Type = (pHciCtx->hostList[0] == phHciNfc_e_UICCHostID) ? phLibNfc_SE_Type_UICC : phLibNfc_SE_Type_eSE;
                pSE_List[0].eSE_ActivationMode = pLibContext->tSeInfo.tSeList[bIndex].eSE_ActivationMode;
                pSE_List[0].eSE_PowerLinkMode = pLibContext->tSeInfo.tSeList[bIndex].eSE_PowerLinkMode;
                pSE_List[0].hSecureElement = pLibContext->tSeInfo.tSeList[bIndex].hSecureElement;
            }
        }
        else
        {
            for(bIndex = 1; bIndex < PHHCINFC_TOTAL_NFCEES; bIndex++)
            {
                if(pLibContext->tSeInfo.tSeList[bIndex].hSecureElement != NULL &&
                   pLibContext->tSeInfo.bSeState[bIndex] != phLibNfc_SeStateInvalid)
                {
                    bCount++;
                    pSE_List[bCount-1].eSE_Type = pLibContext->tSeInfo.tSeList[bIndex].eSE_Type;
                    pSE_List[bCount-1].eSE_ActivationMode = pLibContext->tSeInfo.tSeList[bIndex].eSE_ActivationMode;
                    pSE_List[bCount-1].eSE_PowerLinkMode = pLibContext->tSeInfo.tSeList[bIndex].eSE_PowerLinkMode;
                    pSE_List[bCount-1].hSecureElement = pLibContext->tSeInfo.tSeList[bIndex].hSecureElement;
                }
            }
        }

        *uSE_count = bCount;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SE_SetMode ( phLibNfc_Handle              hSE_Handle,
                                phLibNfc_eSE_ActivationMode  eActivation_mode,
                                pphLibNfc_SE_SetModeRspCb_t  pSE_SetMode_Rsp_cb,
                                void *                       pContext
    )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = phLibNfc_GetContext();
    phLibNfc_Event_t TrigEvent = phLibNfc_EventDummy;
    phLibNfc_DummyInfo_t Info = {phLibNfc_DummyEventInvalid};
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pCtx);

    if(NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == (void *)hSE_Handle)||
            (NULL == pSE_SetMode_Rsp_cb))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        Info.Evt = phLibNfc_DummyEventSetMode;
        Info.Params = (void *)&eActivation_mode;
        pCtx->sSeContext.eActivationMode = eActivation_mode;
        wStatus = phLibNfc_StateHandler(pCtx,
                                        TrigEvent,
                                        (void *)hSE_Handle, /*Secure Element Handle*/
                                        (void *)&Info, /*Information for setting Mode for SE*/
                                        NULL);
        if(NFCSTATUS_PENDING == wStatus)
        {
            /*Store CB info in SE Context*/
            pCtx->CBInfo.pSeSetModeCb = pSE_SetMode_Rsp_cb;
            pCtx->CBInfo.pSeSetModeCtxt = pContext;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SE_PowerAndLinkControl(phLibNfc_Handle              hSE_Handle,
                                          phLibNfc_PowerLinkModes_t    powerLinkModes,
                                          pphLibNfc_RspCb_t            pSE_PowerAndLinkControl_Rsp_cb,
                                          void *                       pContext
                                         )
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = phLibNfc_GetContext();
    phLibNfc_Event_t TrigEvent = phLibNfc_EventDummy;
    phLibNfc_DummyInfo_t Info;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pCtx);

    if (NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if ((NULL == (void *)hSE_Handle) ||
             (NULL == pSE_PowerAndLinkControl_Rsp_cb))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if (phNciNfc_IsVersion2x(pCtx->sHwReference.pNciHandle))
    {
        Info.Evt = phLibNfc_DummyEventPowerAndLinkCtrl;

        Info.Params = (void *)&powerLinkModes;
        wStatus = phLibNfc_StateHandler(pCtx,
                                        TrigEvent,
                                        (void *)hSE_Handle, /*Secure Element Handle*/
                                        (void *)&Info, /*Information for setting Power and Link for SE*/
                                        NULL);
        if (NFCSTATUS_PENDING == wStatus)
        {
            /*Store CB info in SE Context*/
            pCtx->CBInfo.pPowerCtrlLinkCb = pSE_PowerAndLinkControl_Rsp_cb;
            pCtx->CBInfo.pPowerCtrlLinkCntx = pContext;
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("NFCC is not operating in NCI2.0 mode!");
        wStatus = NFCSTATUS_FEATURE_NOT_SUPPORTED;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Mgt_ConfigRoutingTable(uint8_t               bNumRtngConfigs,
                                          phLibNfc_RtngConfig_t *pRoutingCfg,
                                          pphLibNfc_RspCb_t     pRoutingCfg_RspCb,
                                          void*                 pContext)
{
    NFCSTATUS               wStatus         = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_Context_t     pLibCtx         = phLibNfc_GetContext();
    uint8_t                 bNoRtngEntries  = 1;
    uint8_t                 bNumRtngAdded   = 0;
    phLibNfc_DummyInfo_t    Info;
    phLibNfc_Event_t        TrigEvent       = phLibNfc_EventDummy;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL == pLibCtx)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pRoutingCfg) ||
            (NULL == pRoutingCfg_RspCb) || (0 == bNumRtngConfigs))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameters received");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(pLibCtx->StateContext.TrgtState == phLibNfc_StateReset)
    {
        PH_LOG_LIBNFC_INFO_STR("De-Initialize in progress");
        wStatus = NFCSTATUS_SHUTDOWN;
    }
    else
    {
        /* Validate input SE handles and ensure that there is atleast one routing configuration requested */
        wStatus = phLibNfc_ValidateInputRtngInfo(bNumRtngConfigs, pRoutingCfg);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Find out number of total routing entries to calculate size of memory to be allocated */
            bNoRtngEntries += bNumRtngConfigs;
            pLibCtx->sSeContext.pRoutingCfgBuffer = (pphNciNfc_RtngConfig_t)phOsalNfc_GetMemory(sizeof(phNciNfc_RtngConfig_t) * bNoRtngEntries);

            if(NULL != pLibCtx->sSeContext.pRoutingCfgBuffer)
            {
                wStatus = phLibNfc_UpdateRtngCfg(pLibCtx->sSeContext.pRoutingCfgBuffer,
                                                bNumRtngConfigs, pRoutingCfg,
                                                &bNumRtngAdded);
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    pLibCtx->sSeContext.pRoutingCfgBuffer[bNumRtngAdded].Type = phNciNfc_e_LstnModeRtngProtocolBased;
                    pLibCtx->sSeContext.pRoutingCfgBuffer[bNumRtngAdded].LstnModeRtngValue.tProtoBasedRtngValue.bRoute = 0;
                    pLibCtx->sSeContext.pRoutingCfgBuffer[bNumRtngAdded].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOff = 0x00;
                    pLibCtx->sSeContext.pRoutingCfgBuffer[bNumRtngAdded].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bBatteryOff = 0x00;
                    pLibCtx->sSeContext.pRoutingCfgBuffer[bNumRtngAdded].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOn = 0x01;
                    pLibCtx->sSeContext.pRoutingCfgBuffer[bNumRtngAdded].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOnSub1 = 0x00;
                    pLibCtx->sSeContext.pRoutingCfgBuffer[bNumRtngAdded].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOnSub2 = 0x00;
                    pLibCtx->sSeContext.pRoutingCfgBuffer[bNumRtngAdded].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOnSub3 = 0x00;
                    pLibCtx->sSeContext.pRoutingCfgBuffer[bNumRtngAdded].LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol = phNciNfc_e_RfProtocolsNfcDepProtocol;
                    bNumRtngAdded++;
                }
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }

            if(NFCSTATUS_SUCCESS == wStatus)
            {
                Info.Evt = phLibNfc_DummyEventSetRtngCfg;
                Info.Params = (void *)&bNoRtngEntries;

                wStatus = phLibNfc_StateHandler(pLibCtx,
                                                TrigEvent,
                                                NULL,
                                                &Info,
                                                NULL);
                if(NFCSTATUS_PENDING == wStatus)
                {
                    pLibCtx->CBInfo.pClientRoutingCfgCb = pRoutingCfg_RspCb;
                    pLibCtx->CBInfo.pClientRoutingCfgCntx = pContext;
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("Nci Set routing table failed!");
                    phOsalNfc_FreeMemory(pLibCtx->sSeContext.pRoutingCfgBuffer);
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_UpdateNfceeDiscTechnType(void* pLibContext,
                                              pphNciNfc_NfceeDiscReqNtfInfo_t pNfceeDiscReq,
                                              NFCSTATUS status)
{
    pphLibNfc_Context_t pLibCtx = (pphLibNfc_Context_t) pLibContext;
    uint8_t bCount = 0x00;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (NULL != pNfceeDiscReq) && (NFCSTATUS_SUCCESS == status))
    {
        for(bCount = 0; bCount < pNfceeDiscReq->bCount; bCount++)
        {
            PH_LOG_LIBNFC_INFO_STR("Type:%!phNciNfc_RfNfceeDiscReqType_t! TechMode:%!phNciNfc_RfTechMode_t!",
                                   pNfceeDiscReq->pParams[bCount].bType, pNfceeDiscReq->pParams[bCount].eTechMode);

            switch(pNfceeDiscReq->pParams[bCount].eTechMode)
            {
            case phNciNfc_NFCA_Listen:
                switch(pNfceeDiscReq->pParams[bCount].bType)
                {
                case phNciNfc_e_RfNfceeDiscReqAdd:
                    pLibCtx->tSeInfo.bNfceeSuppNfcTech |=  PH_LIBNFC_SE_LSTN_NFC_A_SUPP;
                    break;
                case phNciNfc_e_RfNfceeDiscReqRemove:
                    pLibCtx->tSeInfo.bNfceeSuppNfcTech &= ~PH_LIBNFC_SE_LSTN_NFC_A_SUPP;
                    break;
                }
                break;
            case phNciNfc_NFCB_Listen:
                switch(pNfceeDiscReq->pParams[bCount].bType)
                {
                case phNciNfc_e_RfNfceeDiscReqAdd:
                    pLibCtx->tSeInfo.bNfceeSuppNfcTech |=  PH_LIBNFC_SE_LSTN_NFC_B_SUPP;
                    break;
                case phNciNfc_e_RfNfceeDiscReqRemove:
                    pLibCtx->tSeInfo.bNfceeSuppNfcTech &= ~PH_LIBNFC_SE_LSTN_NFC_B_SUPP;
                    break;
                }
                break;
            case phNciNfc_NFCF_Listen:
                switch(pNfceeDiscReq->pParams[bCount].bType)
                {
                case phNciNfc_e_RfNfceeDiscReqAdd:
                    pLibCtx->tSeInfo.bNfceeSuppNfcTech |=  PH_LIBNFC_SE_LSTN_NFC_F_SUPP;
                    break;
                case phNciNfc_e_RfNfceeDiscReqRemove:
                    pLibCtx->tSeInfo.bNfceeSuppNfcTech &= ~PH_LIBNFC_SE_LSTN_NFC_F_SUPP;
                    break;
                }
                break;
            default:
                break;
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

static void phLibNfc_UpdateSeInfo(void* pContext, pphNciNfc_NfceeInfo_t pNfceeInfo, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phHciNfc_HciContext_t *pHciContext = NULL;
    bool_t bNewNfceeId = TRUE;
    pphNciNfc_NfceeDeviceHandle_t pNfceeHandle;
    uint8_t bIndex = 0x00;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if( (NULL != pCtx) && (NULL != pNfceeInfo) && (NFCSTATUS_SUCCESS == wStatus) )
    {
        PH_LOG_LIBNFC_INFO_STR("NFCEE Id: %d Status: %d", pNfceeInfo->bNfceeId, pNfceeInfo->bNfceeStatus);

        if(FALSE == pNfceeInfo->bNfceeStatus) /* NFCEE is removed */
        {
            for (phLibNfc_SE_Index_t seIndex = phLibNfc_SE_Index_HciNwk; seIndex < phLibNfc_SE_Index_MaxCount; seIndex++)
            {
                pNfceeHandle = (pphNciNfc_NfceeDeviceHandle_t)pCtx->tSeInfo.tSeList[seIndex].hSecureElement;
                if (NULL != pNfceeHandle && pNfceeHandle->tDevInfo.bNfceeID == pNfceeInfo->bNfceeId)
                {
                    pCtx->tSeInfo.bSeState[seIndex] = phLibNfc_SeStateInvalid;
                    pCtx->tSeInfo.tSeList[seIndex].hSecureElement = NULL;
                    pCtx->tSeInfo.bSeCount--;
                    bNewNfceeId = FALSE;
                    break;
                }
            }
        }
        else
        {
            phHciNfc_HostID_t hciHostId = phLibNfc_SE_GetHciHostId(pContext, pNfceeInfo);

            if (hciHostId == phHciNfc_e_TerminalHostID)
            {
                // *** HCI Network NFCEE type ***
                PH_LOG_LIBNFC_INFO_STR("NFCEE Type: %!phLibNfc_SE_Type_t!", phLibNfc_SE_Type_HciNwk);
                if(NULL == pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk].hSecureElement)
                {
                    pCtx->tSeInfo.bSeCount++;
                    pCtx->tSeInfo.bSeState[phLibNfc_SE_Index_HciNwk] = phLibNfc_SeStateNotInitialized;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk].hSecureElement = pNfceeInfo->pNfceeHandle;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk].eSE_Type = phLibNfc_SE_Type_HciNwk;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk].hciHostId = hciHostId;

                    if (pCtx->pHciContext == NULL)
                    {
                        pHciContext = (phHciNfc_HciContext_t*) phOsalNfc_GetMemory(sizeof(phHciNfc_HciContext_t));
                    }
                    else
                    {
                        pHciContext = pCtx->pHciContext;
                    }

                    if(NULL != pHciContext)
                    {
                        phOsalNfc_SetMemory(pHciContext,0,sizeof(phHciNfc_HciContext_t));
                        pHciContext->pNciContext = pCtx->sHwReference.pNciHandle;
                        pHciContext->pSeHandle = pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk].hSecureElement;
                        pHciContext->bLogDataMessages = !!pCtx->Config.bLogNciDataMessages;

                        pCtx->pHciContext = pHciContext;

                        /* Initializing HCI Initialization sequence */
                        phLibNfc_HciLaunchDevInitSequence(pCtx);
                    }
                }
                else
                {
                    bNewNfceeId = (pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk].hSecureElement != pNfceeInfo->pNfceeHandle);
                }

            }
            else if (hciHostId == phHciNfc_e_UICCHostID)
            {
                // *** UICC NFCEE type ***
                PH_LOG_LIBNFC_INFO_STR("NFCEE Type: %!phLibNfc_SE_Type_t!", phLibNfc_SE_Type_UICC);
                if(NULL == pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_UICC].hSecureElement)
                {
                    pCtx->tSeInfo.bSeCount++;
                    pCtx->tSeInfo.bSeState[phLibNfc_SE_Index_UICC] = phLibNfc_SeStateNotInitialized;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_UICC].hSecureElement = pNfceeInfo->pNfceeHandle;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_UICC].eSE_Type = phLibNfc_SE_Type_UICC;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_UICC].hciHostId = hciHostId;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_UICC].eSE_ActivationMode =
                        (PH_NCINFC_EXT_NFCEEMODE_ENABLE == pNfceeInfo->pNfceeHandle->tDevInfo.eNfceeStatus) ? phLibNfc_SE_ActModeOn : phLibNfc_SE_ActModeOff;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_UICC].eSE_PowerLinkMode = phLibNfc_PLM_NfccDecides;

                    wStatus = phLibNfc_SE_GetIndex(pCtx, phLibNfc_SeStateInitializing, &bIndex);
                    if(wStatus == NFCSTATUS_FAILED)
                    {
                        /*Ensure no other NFCEE init sequence is in progress and launch UICC init sequence*/
                        phLibNfc_HciLaunchChildDevInitSequence(pCtx, phLibNfc_SE_Index_UICC);
                    }
                }
                else
                {
                    bNewNfceeId = (pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_UICC].hSecureElement != pNfceeInfo->pNfceeHandle);
                }
            }
            else if (hciHostId >= phHciNfc_e_ProprietaryHostID_Min && hciHostId <= phHciNfc_e_ProprietaryHostID_Max)
            {
                // *** eSE NFCEE type ***
                PH_LOG_LIBNFC_INFO_STR("NFCEE Type: %!phLibNfc_SE_Type_t!", phLibNfc_SE_Type_eSE);
                if(NULL == pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_eSE].hSecureElement)
                {
                    pCtx->tSeInfo.bSeCount++;
                    pCtx->tSeInfo.bSeState[phLibNfc_SE_Index_eSE] = phLibNfc_SeStateNotInitialized;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_eSE].hSecureElement = pNfceeInfo->pNfceeHandle;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_eSE].eSE_Type = phLibNfc_SE_Type_eSE;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_eSE].hciHostId = hciHostId;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_eSE].eSE_ActivationMode =
                        (PH_NCINFC_EXT_NFCEEMODE_ENABLE == pNfceeInfo->pNfceeHandle->tDevInfo.eNfceeStatus) ? phLibNfc_SE_ActModeOn : phLibNfc_SE_ActModeOff;
                    pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_eSE].eSE_PowerLinkMode = phLibNfc_PLM_NfccDecides;

                    wStatus = phLibNfc_SE_GetIndex(pCtx, phLibNfc_SeStateInitializing, &bIndex);
                    if(wStatus == NFCSTATUS_FAILED)
                    {
                        /*Ensure no other NFCEE init sequence is in progress and launch ESE init sequence*/
                        phLibNfc_HciLaunchChildDevInitSequence(pCtx, phLibNfc_SE_Index_eSE);
                    }
                }
                else
                {
                    bNewNfceeId = (pCtx->tSeInfo.tSeList[phLibNfc_SE_Index_eSE].hSecureElement != pNfceeInfo->pNfceeHandle);
                }
            }
            else
            {
                // *** Unsupported NFCEE type ***
                wStatus = NFCSTATUS_FAILED;
                PH_LOG_LIBNFC_CRIT_STR("Unrecognized HCI Host ID: 0x%02X", hciHostId);
            }
        }

        if((TRUE == bNewNfceeId) &&
           (NULL != pCtx->CBInfo.pNFCEEDiscoveryCb) &&
           (0 < pCtx->sSeContext.nNfceeDiscNtf))
        {
            pCtx->sSeContext.nNfceeDiscNtf--;
            wStatus = phLibNfc_SE_GetIndex(pCtx, phLibNfc_SeStateInitializing, &bIndex);
            if(pCtx->sSeContext.nNfceeDiscNtf == 0)
            {
                if(wStatus == NFCSTATUS_FAILED)
                {
                    /*Decrement the count of the NFCEE discovery notification and if the discovery process has completed
                    and there is no other NFCEE init sequence in progress launch the NFCEE discovery complete sequence*/
                    phLibNfc_LaunchNfceeDiscCompleteSequence(pCtx, NFCSTATUS_SUCCESS, NULL);
                }
                else if(!phNciNfc_IsVersion1x(phNciNfc_GetContext()))
                {
                    phLibNfc_HciLaunchDevInitSequence(pCtx);
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

void phLibNfc_SENtfHandler(
    void*      pContext,
    phNciNfc_NotificationType_t eNtfType,
    pphNciNfc_NotificationInfo_t pSEInfo,
    NFCSTATUS status
    )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = phLibNfc_GetContext();
    phLibNfc_uSeEvtInfo_t tSeEvtInfo = {0};
    phLibNfc_Handle hSecureElement = (phLibNfc_Handle)NULL;
    uint16_t bIndex = 0x00;
    uint8_t bTag = 0x00, bLen = 0x00;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    UNUSED(pContext);

    if(NULL != pCtx)
    {
        PH_LOG_LIBNFC_INFO_STR("NFCEE Notification Type: %!phNciNfc_NotificationType_t!", eNtfType);

        switch(eNtfType)
        {
            case eNciNfc_NfceeActionNtf:
            {
                hSecureElement = (phLibNfc_Handle)(pSEInfo->pActionInfo->pNfceeHandle);
                if(PH_NCINFC_TRIG_APP_INITIATION == pSEInfo->pActionInfo->eTriggerType)
                {
                    while((bIndex+2) < pSEInfo->pActionInfo->bSupDataLen)
                    {
                        bTag = pSEInfo->pActionInfo->phNciNfc_SupportData_t.aSupData[bIndex++];
                        bLen = pSEInfo->pActionInfo->phNciNfc_SupportData_t.aSupData[bIndex++];

                        if((bTag == PHLIBNFC_TRANSACTION_AID) && (bLen > 0) && ((bIndex+bLen) <= pSEInfo->pActionInfo->bSupDataLen))
                        {
                            tSeEvtInfo.UiccEvtInfo.aid.length = bLen;
                            tSeEvtInfo.UiccEvtInfo.aid.buffer = &pSEInfo->pActionInfo->phNciNfc_SupportData_t.aSupData[bIndex];
                        }
                        else if((bTag == PHLIBNFC_TRANSACTION_PARAM) && (bLen > 0) && ((bIndex+bLen) <= pSEInfo->pActionInfo->bSupDataLen))
                        {
                            tSeEvtInfo.UiccEvtInfo.param.length = bLen;
                            tSeEvtInfo.UiccEvtInfo.param.buffer = &pSEInfo->pActionInfo->phNciNfc_SupportData_t.aSupData[bIndex];
                        }

                        bIndex += bLen;
                    }

                    if((tSeEvtInfo.UiccEvtInfo.aid.buffer == NULL) || (tSeEvtInfo.UiccEvtInfo.aid.length == 0))
                    {
                        PH_LOG_LIBNFC_INFO_STR("No AID available in the event payload");
                        goto Done;
                    }

                    if(NULL != pCtx->CBInfo.pSeListenerNtfCb)
                    {
                        PH_LOG_LIBNFC_INFO_STR("Invoking pSeListenerNtfCb");
                        pCtx->CBInfo.pSeListenerNtfCb(pCtx->CBInfo.pSeListenerCtxt,phLibNfc_eSE_EvtTypeTransaction,hSecureElement,\
                                        &tSeEvtInfo,status);
                    }
                }
                else if (PH_NCINFC_TRIG_RFPROTOCOL_ROUTING == \
                    pSEInfo->pActionInfo->eTriggerType)
                {
                    tSeEvtInfo.UiccEvtInfo.param.buffer = \
                        (uint8_t *)&pSEInfo->pActionInfo->phNciNfc_SupportData_t.eRfProtocol;
                    tSeEvtInfo.UiccEvtInfo.param.length = \
                        pSEInfo->pActionInfo->bSupDataLen;
                }
                else
                {
                    PH_LOG_LIBNFC_INFO_STR("Unsupported event: %d", pSEInfo->pActionInfo->eTriggerType);
                    goto Done;
                }
            }
            break;
            case eNciNfc_NciActivateNfceeNtf:
            {
                hSecureElement = (phLibNfc_Handle)(pSEInfo->pNfceeHandle);
                /* Invoke state machine only if it is not BUSY */
                if(phLibNfc_StateTrasitionBusy != pCtx->StateContext.Flag)
                {
                    wStatus = phLibNfc_StateHandler(pCtx, phLibNfc_EventSEActivated, hSecureElement, NULL, NULL);
                    PH_LOG_LIBNFC_CRIT_STR("State machine has returned %!NFCSTATUS!", wStatus);
                }
            }
            break;
            case eNciNfc_NciRfFieldInfoNtf:
            {
                PH_LOG_LIBNFC_CRIT_STR("RF Field Info: %!phNciNfc_RfFieldInfo_t!", pSEInfo->tRfFieldInfo.eRfFieldInfo);

                if (NULL != pCtx->CBInfo.pSeListenerNtfCb)
                {
                    PH_LOG_LIBNFC_INFO_STR("Invoking pSeListenerNtfCb for field on/off.");
                    phLibNfc_eSE_EvtType_t eventType = (pSEInfo->tRfFieldInfo.eRfFieldInfo == phNciNfc_e_RfFieldOff) ?
                        phLibNfc_eSE_EvtRfFieldExit :
                        phLibNfc_eSE_EvtRfFieldEnter;

                    pCtx->CBInfo.pSeListenerNtfCb(pCtx->CBInfo.pSeListenerCtxt, eventType, NULL, &tSeEvtInfo, NFCSTATUS_SUCCESS);
                }

                if(pSEInfo->tRfFieldInfo.eRfFieldInfo == phNciNfc_e_RfFieldOff && pCtx->bPcdConnected)
                {
                    if(NULL != pCtx->CBInfo.pSeListenerNtfCb)
                    {
                        PH_LOG_LIBNFC_INFO_STR("Invoking pSeListenerNtfCb for reader off.");
                        pCtx->CBInfo.pSeListenerNtfCb(pCtx->CBInfo.pSeListenerCtxt,phLibNfc_eSE_EvtReaderDeparture,NULL,
                                                      &tSeEvtInfo,NFCSTATUS_SUCCESS);
                    }

                    pCtx->bPcdConnected = FALSE;
                }
            }
            break;
            case eNciNfc_NfceeDiscoverNtf:
            {
                phLibNfc_UpdateSeInfo(pCtx, &pSEInfo->tNfceeInfo, status);
            }
            break;
            case eNciNfc_NfceeStatusNtf:
            {
                switch (pSEInfo->tNfceeStatusInfo.bNfceeStatus)
                {
                case phNciNfc_NfceeInitSequenceError:
                    PH_LOG_LIBNFC_CRIT_STR("Unrecoverable error while communicating with NFCEE. NFCEE ID = %d",
                        pSEInfo->tNfceeStatusInfo.bNfceeId);
                    break;
                case phNciNfc_NfceeInitSequenceStarted:
                    PH_LOG_LIBNFC_INFO_STR("NFCEE initialization started");
                    break;
                case phNciNfc_NfceeInitSequenceCompleted:
                    if (0 != pCtx->dwHciTimerId)
                    {
                        PH_LOG_LIBNFC_INFO_STR("Stopping Hci Timer");
                        (void)phOsalNfc_Timer_Stop(pCtx->dwHciTimerId);
                        (void)phOsalNfc_Timer_Delete(pCtx->dwHciTimerId);
                        pCtx->dwHciTimerId = 0;

                        phLibNfc_InternalSequence(pCtx, wStatus, NULL);
                    }
                    break;
                }
            }
            break;
            case eNciNfc_NfceeDiscReqNtf:
            {
                phLibNfc_UpdateNfceeDiscTechnType(pCtx, &pSEInfo->tNfceeDiscReqInfo, status);
            }
            break;
            default:
            {
                PH_LOG_LIBNFC_INFO_STR("Unsupported Notification Type: %!phNciNfc_NotificationType_t!", eNtfType);
            }
            break;
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid Context Param received!!");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }

Done:
    PH_LOG_LIBNFC_FUNC_EXIT();
}

void phLibNfc_InvokeSeNtfCallback(void* pContext,void* pInfo,NFCSTATUS status,uint8_t bPipeId,phLibNfc_eSE_EvtType_t eEventType)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pLibCtx = phLibNfc_GetContext();
    pphHciNfc_HciContext_t pHciContext = (pphHciNfc_HciContext_t)pContext;
    phLibNfc_Handle hSecureElement = (phLibNfc_Handle)NULL;
    uint8_t bCount = 0;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext) && (NULL != pLibCtx) && (pInfo != NULL))
    {
        for (bCount =0;bCount< PHHCINFC_TOTAL_NFCEES;bCount++)
        {
           if( (bPipeId == pHciContext->aGetHciSessionId[PHHCI_UICC_CONNECTIVITY_PIPE_STORAGE_INDEX])&&\
               (pLibCtx->tSeInfo.tSeList[bCount].eSE_Type == phLibNfc_SE_Type_UICC) )
           {
               hSecureElement = pLibCtx->tSeInfo.tSeList[bCount].hSecureElement;
           }
           else if( ((bPipeId == pHciContext->aGetHciSessionId[PHHCI_ESE_APDU_PIPE_STORAGE_INDEX])||
                    (bPipeId == pHciContext->aGetHciSessionId[PHHCI_ESE_CONNECTIVITY_PIPE_STORAGE_INDEX])) &&\
                    (pLibCtx->tSeInfo.tSeList[bCount].eSE_Type == phLibNfc_SE_Type_eSE))
           {
               hSecureElement = pLibCtx->tSeInfo.tSeList[bCount].hSecureElement;
           }
        }

        if(NULL != pLibCtx->CBInfo.pSeListenerNtfCb)
        {
            pLibCtx->CBInfo.pSeListenerNtfCb(pLibCtx->CBInfo.pSeListenerCtxt,eEventType,hSecureElement,\
                            pInfo,wStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

void phLibNfc_ConfigRoutingTableCb(void *pContext, NFCSTATUS wStatus, void *pInfo)
{
    void                    *pUpperLayerContext=NULL;
    pphLibNfc_RspCb_t       pClientCb=NULL;
    phLibNfc_Event_t        TrigEvent = phLibNfc_EventReqCompleted;
    phLibNfc_LibContext_t* pLibContext = (phLibNfc_LibContext_t *)pContext;

    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL == pContext) ||
      (pLibContext != phLibNfc_GetContext()))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context passed by lower layer");
        wStatus = NFCSTATUS_FAILED;
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        if(NULL != pLibContext->sSeContext.pRoutingCfgBuffer)
        {
            phOsalNfc_FreeMemory(pLibContext->sSeContext.pRoutingCfgBuffer);
            pLibContext->sSeContext.pRoutingCfgBuffer = NULL;
        }

        (void)phLibNfc_StateHandler(pLibContext,\
                                    TrigEvent,\
                                    NULL,\
                                    NULL,\
                                    NULL);

        pClientCb =pLibContext->CBInfo.pClientRoutingCfgCb ;
        pUpperLayerContext = pLibContext->CBInfo.pClientRoutingCfgCntx ;
        pLibContext->CBInfo.pClientRoutingCfgCntx = NULL;
        pLibContext->CBInfo.pClientRoutingCfgCb =NULL;

        if(NULL != pClientCb)
        {
            PH_LOG_LIBNFC_INFO_STR("Invoke the upper layer callback function");
            pClientCb(pUpperLayerContext,wStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

static NFCSTATUS phLibNfc_UpdateRtngCfg(pphNciNfc_RtngConfig_t  pRtngBuff,
                                        uint8_t                 bNumRtngConfigs,
                                        phLibNfc_RtngConfig_t   *pRtngCfg,
                                        uint8_t                 *pNumEntriesAdded)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bRtngCfgsAdded = 0;
    uint8_t bNumSeEntries = 0;
    pphNciNfc_NfceeDeviceHandle_t pNfceeInfo;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pRtngCfg) && (NULL != pRtngBuff) && (NULL != pNumEntriesAdded))
    {
        (*pNumEntriesAdded) = 0;
        /* Traverse through the enntire list of SE's */
        for(bNumSeEntries = 0; bNumSeEntries < bNumRtngConfigs; bNumSeEntries++)
        {
            if((NULL != &pRtngCfg[bNumSeEntries]) &&
                (NULL != pRtngCfg[bNumSeEntries].hSecureElement) && ((void*)0xFF != pRtngCfg[bNumSeEntries].hSecureElement))
            {
                pNfceeInfo = (pphNciNfc_NfceeDeviceHandle_t) pRtngCfg[bNumSeEntries].hSecureElement;
                wStatus = phLibNfc_UpdateRtngInfo(&pRtngBuff[bRtngCfgsAdded],
                                                  &pRtngCfg[bNumSeEntries],
                                                  pNfceeInfo->tDevInfo.bNfceeID);
                if(NFCSTATUS_SUCCESS != wStatus)
                {
                    PH_LOG_LIBNFC_CRIT_STR("Update Rtng config failed!");
                    break;
                }
                else
                {
                    bRtngCfgsAdded++;
                }
            }
            else if((NULL != &pRtngCfg[bNumSeEntries]) &&
                    ((void*)0xFF != pRtngCfg[bNumSeEntries].hSecureElement) && (NULL == pRtngCfg[bNumSeEntries].hSecureElement)) /*In case of DH NFCEE ID */
            {
                wStatus = phLibNfc_UpdateRtngInfo(&pRtngBuff[bRtngCfgsAdded],
                                                  &pRtngCfg[bNumSeEntries],
                                                  0x00);
                if(NFCSTATUS_SUCCESS != wStatus)
                {
                    PH_LOG_LIBNFC_CRIT_STR("Update Rtng config failed!");
                    break;
                }
                else
                {
                    bRtngCfgsAdded++;
                }
            }
        }
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            (*pNumEntriesAdded) = bRtngCfgsAdded;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_UpdateRtngInfo(pphNciNfc_RtngConfig_t pNciRtngCfg,
                                         phLibNfc_RtngConfig_t *pLibNfcRtngCfg,
                                         uint8_t bNfceeId)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pLibCtx = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibCtx) && (NULL != pNciRtngCfg) && (NULL != pLibNfcRtngCfg))
    {
        switch(pLibNfcRtngCfg->Type)
        {
            case phNfc_LstnModeRtngTechBased:
                pNciRtngCfg->Type = phNciNfc_e_LstnModeRtngTechBased;
                pNciRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.bRoute = bNfceeId;
                pNciRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bBatteryOff =
                    pLibNfcRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bBatteryOff;
                pNciRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOff =
                    pLibNfcRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOff;
                pNciRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOn =
                    pLibNfcRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOn;
                pNciRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOnSub1 = 0;
                pNciRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOnSub2 = 0;
                pNciRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOnSub3 =
                    (0 == pLibNfcRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOn) ? 0 : pLibCtx->Config.bSwitchedOnSubState;
                pNciRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology = (phNciNfc_RfTechnologies_t)
                    pLibNfcRtngCfg->LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology;
                break;
            case phNfc_LstnModeRtngProtocolBased:
                pNciRtngCfg->Type = phNciNfc_e_LstnModeRtngProtocolBased;
                pNciRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.bRoute = bNfceeId;
                pNciRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bBatteryOff =
                    pLibNfcRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bBatteryOff;
                pNciRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOff =
                    pLibNfcRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOff;
                pNciRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOn =
                    pLibNfcRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOn;
                pNciRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOnSub1 = 0;
                pNciRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOnSub2 = 0;
                pNciRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOnSub3 =
                    (0 == pLibNfcRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOn) ? 0 : pLibCtx->Config.bSwitchedOnSubState;
                pNciRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol = (phNciNfc_RfProtocols_t)
                    pLibNfcRtngCfg->LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol;
                break;
            case phNfc_LstnModeRtngAidBased:
                pNciRtngCfg->Type = phNciNfc_e_LstnModeRtngAidBased;
                pNciRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.bRoute = bNfceeId;
                pNciRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bBatteryOff =
                    pLibNfcRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bBatteryOff;
                pNciRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOff =
                    pLibNfcRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOff;
                pNciRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOn =
                    pLibNfcRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOn;
                pNciRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOnSub1 = 0;
                pNciRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOnSub2 = 0;
                pNciRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOnSub3 =
                    (0 == pLibNfcRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOn) ? 0 : pLibCtx->Config.bSwitchedOnSubState;
                if(pLibNfcRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.bAidSize <= PH_NCINFC_MAX_AID_LEN)
                {
                    phOsalNfc_MemCopy( pNciRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.aAid,
                        pLibNfcRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.aAid,
                        pLibNfcRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.bAidSize);
                    pNciRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.bAidSize =
                    pLibNfcRtngCfg->LstnModeRtngValue.tAidBasedRtngValue.bAidSize;
                }
                else
                {
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                    PH_LOG_LIBNFC_CRIT_STR("Invalid input AID size");
                }
                break;
            default:
                PH_LOG_LIBNFC_CRIT_STR("Input SE handle not valid!");
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            break;
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_ValidateInputRtngInfo(uint8_t               bNumRtngConfigs,
                                                phLibNfc_RtngConfig_t *pRtngCfg)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bCount = 0;
    uint8_t bNfceeCnt = 0;
    pphLibNfc_Context_t pLibCtx = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pRtngCfg))
    {
        for(bCount = 0; bCount < bNumRtngConfigs; bCount++)
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
            /* There should be atleast one routing config available for each input SE handle */
            if(NULL != &pRtngCfg[bCount])
            {
                /*Validate the SE Handle */
                for(bNfceeCnt = 0 ; bNfceeCnt < PHHCINFC_TOTAL_NFCEES; bNfceeCnt++)
                {
                    if( pRtngCfg[bCount].hSecureElement == pLibCtx->tSeInfo.tSeList[bNfceeCnt].hSecureElement \
                        || pRtngCfg[bCount].hSecureElement == NULL)
                    {
                        wStatus = NFCSTATUS_SUCCESS;
                        break;
                    }
                }
                if(NFCSTATUS_SUCCESS != wStatus)
                {
                    PH_LOG_LIBNFC_CRIT_STR("Input SE handle not valid!");
                    break;
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_StartNfceeDisc(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t) pContext;

    UNUSED(status);
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        wStatus = phNciNfc_Nfcee_StartDiscovery(pCtx->sHwReference.pNciHandle,\
                                        (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                        pContext);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_ProcessStartNfceeDiscRsp(void* pContext, NFCSTATUS wStatus, void* pInfo)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t) pContext;

    // Warning C4311: 'type cast': pointer truncation from 'void *' to 'uint32_t'
#pragma warning(suppress:4311)
    uint32_t dwNfceeCount = (uint32_t)pInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if ((NULL != pCtx) && (NFCSTATUS_SUCCESS == wStatus))
    {
        pCtx->sSeContext.nNfceeDiscNtf = dwNfceeCount;

        if(dwNfceeCount == 0)
        {
            /*No NFCEE discovered so invoke the completion sequence*/
            phLibNfc_LaunchNfceeDiscCompleteSequence(pCtx,NFCSTATUS_SUCCESS,NULL);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_StopNfceeDisc(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t) pContext;

    UNUSED(status);
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        wStatus = phNciNfc_Nfcee_StopDiscovery(pCtx->sHwReference.pNciHandle,\
                                        (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                        pContext);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_ProcessStopNfceeDiscRsp(void* pContext, NFCSTATUS wStatus, void* pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();

    UNUSED(pContext);
    UNUSED(wStatus);
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_NfceeStartDiscSeqComplete(void *pContext, NFCSTATUS Status, void * pInfo)
{
    NFCSTATUS wStatus = Status;
    pphLibNfc_LibContext_t pCtx = (pphLibNfc_LibContext_t)pContext;
    pphLibNfc_RspCb_t pClientCb;
    void * pUpperLayerContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Request Successful");
        (void)phLibNfc_StartWdTimer((void *)pCtx,
                                   &phLibNfc_WdTimerExpiredCb,
                                   PH_LIBNFC_WD_TIMEOUT);
    }
    else
    {
        (void)phLibNfc_StateHandler(pCtx, TrigEvent, pInfo, NULL, NULL);

        pClientCb = pCtx->CBInfo.pNFCEEDiscoveryCb;
        pUpperLayerContext = pCtx->CBInfo.pNFCEEDiscoveryCntx;
        pCtx->CBInfo.pNFCEEDiscoveryCb = NULL;
        pCtx->CBInfo.pNFCEEDiscoveryCntx = NULL;

        if(NULL != pClientCb)
        {
            (*pClientCb)(pUpperLayerContext,wStatus);
        }

        PH_LOG_LIBNFC_CRIT_STR("Request Failed!!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_NfceeNtfDelayCb(uint32_t dwTimerId, void *pContext)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    /* Internal event is made failed in order to remain in same state */
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    UNUSED(dwTimerId);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        (void)phOsalNfc_Timer_Stop(pLibContext->dwHciTimerId);
        (void)phOsalNfc_Timer_Delete(pLibContext->dwHciTimerId);

        pLibContext->dwHciTimerId = 0;

        (void)phLibNfc_SeqHandler(pLibContext,wStatus,NULL);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

NFCSTATUS phLibNfc_DelayForSeNtf(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if ((NULL != pCtx) && (NFCSTATUS_SUCCESS == wStatus))
    {
        if (0 != pCtx->dwHciTimerId)
        {
            PH_LOG_LIBNFC_WARN_STR("Existing dwHciTimerId=%u will be overwritten!", pCtx->dwHciTimerId);
        }

        pCtx->dwHciTimerId = phOsalNfc_Timer_Create();

        PH_LOG_LIBNFC_INFO_STR(
            "Sequence will wait for NCI notification, timeout: %u, dwHciTimerId: %u",
            pCtx->dwHciInitDelay,
            pCtx->dwHciTimerId);

        wStatus = phOsalNfc_Timer_Start(pCtx->dwHciTimerId,
                                        pCtx->dwHciInitDelay,
                                        &phLibNfc_NfceeNtfDelayCb,
                                        (void *)pCtx);

        if (NFCSTATUS_SUCCESS == wStatus)
        {
            wStatus = NFCSTATUS_PENDING;
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Failed to set a timer for NCI notification, status:%!NFCSTATUS!", wStatus);
            phOsalNfc_Timer_Delete(pCtx->dwHciTimerId);
            pCtx->dwHciTimerId = 0;
            wStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_DelayForSeNtfProc(void* pContext,NFCSTATUS status,void* pInfo)
{
    UNUSED(pInfo);
    UNUSED(status);
    UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return NFCSTATUS_SUCCESS;
}

NFCSTATUS phLibNfc_DelayForNfceeAtr(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if ((NULL != pCtx) && (NFCSTATUS_SUCCESS == wStatus))
    {
        PH_LOG_LIBNFC_INFO_U32MSG("Delay to receive ATR ntf", pCtx->dwHciInitDelay);

        pCtx->dwHciTimerId = phOsalNfc_Timer_Create();
        if (PH_OSALNFC_TIMER_ID_INVALID != pCtx->dwHciTimerId)
        {
            wStatus = phOsalNfc_Timer_Start(pCtx->dwHciTimerId,
                pCtx->dwHciInitDelay,
                &phLibNfc_NfceeNtfDelayCb,
                (void *)pCtx);
            if (NFCSTATUS_SUCCESS == wStatus)
            {
                wStatus = NFCSTATUS_PENDING;
            }
            else
            {
                (void)phOsalNfc_Timer_Delete(pCtx->dwHciTimerId);
                pCtx->dwHciTimerId = 0;
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_DelayForNfceeAtrProc(void* pContext, NFCSTATUS status, void* pInfo)
{
    UNUSED(pInfo);
    UNUSED(status);
    UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return NFCSTATUS_SUCCESS;
}

static NFCSTATUS phLibNfc_NfceeDiscSeqComplete(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_LibContext_t pLibContext  = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphLibNfc_RspCb_t pClientCb;
    void * pUpperLayerContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        if(0 != pLibContext->WdTimerId)
        {
            (void)phOsalNfc_Timer_Stop(pLibContext->WdTimerId);
            (void)phOsalNfc_Timer_Delete(pLibContext->WdTimerId);
            pLibContext->WdTimerId = 0;
        }

        pClientCb = pLibContext->CBInfo.pNFCEEDiscoveryCb;
        pUpperLayerContext = pLibContext->CBInfo.pNFCEEDiscoveryCntx;
        pLibContext->CBInfo.pNFCEEDiscoveryCb = NULL;
        pLibContext->CBInfo.pNFCEEDiscoveryCntx = NULL;

        (void)phLibNfc_StateHandler(pContext, TrigEvent, pInfo, NULL, NULL);

        if(NULL != pClientCb)
        {
            (*pClientCb)(pUpperLayerContext,wStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phLibNfc_SetModeSeq(void *pContext,
                    NFCSTATUS wStatus,
                    void *pInfo)
{
    NFCSTATUS wIntStatus = wStatus;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    pphNciNfc_NfceeDeviceHandle_t pNfceeHandle;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        if(NFCSTATUS_SUCCESS == wIntStatus)
        {
            pNfceeHandle = (pphNciNfc_NfceeDeviceHandle_t)pLibContext->sSeContext.pActiveSeInfo->hSecureElement;
            wIntStatus = phNciNfc_Nfcee_ModeSet(pLibContext->sHwReference.pNciHandle,
                        pNfceeHandle->tDevInfo.bNfceeID,
                        pLibContext->sSeContext.eNfceeMode,
                        (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                        (void *)pLibContext);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid Context passed from lower layer!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS phLibNfc_SetModeSeqEnd(void *pContext, NFCSTATUS wStatus, void *pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = pContext;
    uint8_t bIndex = 0x00;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        wIntStatus = wStatus;
        if(NFCSTATUS_SUCCESS == wIntStatus)
        {
            PH_LOG_LIBNFC_INFO_STR("Set Se Mode success");
            for(bIndex = 0; bIndex <PHHCINFC_TOTAL_NFCEES; bIndex++)
            {
                if(pLibContext->sSeContext.pActiveSeInfo->hSecureElement == pLibContext->tSeInfo.tSeList[bIndex].hSecureElement)
                {
                    if(PH_NCINFC_EXT_NFCEEMODE_DISABLE == pLibContext->sSeContext.eNfceeMode)
                    {
                        pLibContext->tSeInfo.tSeList[bIndex].eSE_ActivationMode = phLibNfc_SE_ActModeOff;
                        break;
                    }
                    else if (PH_NCINFC_EXT_NFCEEMODE_ENABLE == pLibContext->sSeContext.eNfceeMode)
                    {
                        pLibContext->tSeInfo.tSeList[bIndex].eSE_ActivationMode = phLibNfc_SE_ActModeOn;
                        break;
                    }
                }
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Set Se Mode Failed!");
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS phLibNfc_SetSeModeSeqComplete (void* pContext,NFCSTATUS Status,void *pInfo)
{
    NFCSTATUS wStatus = Status;
    pphLibNfc_LibContext_t pCtx = phLibNfc_GetContext();
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphLibNfc_SE_SetModeRspCb_t ClientCb = NULL;
    void *ClientContext = NULL;
    phLibNfc_Handle hSeHandle;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pCtx) && (pContext == pCtx))
    {
        ClientCb = pCtx->CBInfo.pSeSetModeCb;
        ClientContext = pCtx->CBInfo.pSeSetModeCtxt;
        hSeHandle = pCtx->sSeContext.pActiveSeInfo->hSecureElement;

        if(NFCSTATUS_SUCCESS != wStatus)
        {
            TrigEvent = phLibNfc_EventFailed;
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            /* If SE mode change is success */
            pCtx->sSeContext.pActiveSeInfo->eSE_ActivationMode =
                pCtx->sSeContext.eActivationMode;
        }

        (void)phLibNfc_StateHandler(pCtx, TrigEvent, pInfo, NULL, NULL);
        pCtx->CBInfo.pSeSetModeCb = NULL;
        pCtx->CBInfo.pSeSetModeCtxt = NULL;
        pCtx->sSeContext.eNfceeMode = PH_NCINFC_NFCEEDISC_UNKNOWN;

        if(NULL != ClientCb)
        {
            ClientCb(ClientContext, hSeHandle, wStatus);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameter!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phLibNfc_SePowerAndLinkCtrlSeq(void *pContext,
                               NFCSTATUS wStatus,
                               void *pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    UNUSED(pInfo);
    UNUSED(wStatus);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if ((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        wIntStatus = phNciNfc_Nfcee_SePowerAndLinkCtrlSet(pLibContext->sHwReference.pNciHandle,
                                (void *)pLibContext->sSeContext.pActiveSeInfo->hSecureElement,
                                pLibContext->sSeContext.ePowerLinkMode,
                                (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                (void *)pLibContext);
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid Context passed from lower layer!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS phLibNfc_SePowerAndLinkCtrlEnd(void *pContext, NFCSTATUS wStatus, void *pInfo)
{
    pphLibNfc_LibContext_t pLibContext = pContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if ((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        if (NFCSTATUS_SUCCESS != wStatus)
        {
            PH_LOG_LIBNFC_CRIT_STR("Set Se Power Mode Failed!");
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_SePowerAndLinkCtrlCompleteSequence(void* pContext, NFCSTATUS wStatus, void *pInfo)
{
    pphLibNfc_LibContext_t pCtx = phLibNfc_GetContext();
    pphLibNfc_RspCb_t clientCb = NULL;
    void *clientContext = NULL;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if ((NULL != pCtx) && (pContext == pCtx))
    {
        phLibNfc_Event_t trigEvent;
        if (NFCSTATUS_SUCCESS == wStatus)
        {
            pCtx->sSeContext.pActiveSeInfo->eSE_PowerLinkMode = (phLibNfc_PowerLinkModes_t)pCtx->sSeContext.ePowerLinkMode;
            trigEvent = phLibNfc_EventReqCompleted;
        }
        else
        {
            trigEvent = phLibNfc_EventFailed;
        }

        clientCb = pCtx->CBInfo.pPowerCtrlLinkCb;
        clientContext = pCtx->CBInfo.pPowerCtrlLinkCntx;

        // Close out sequence
        (void)phLibNfc_StateHandler(pCtx, trigEvent, pInfo, NULL, NULL);

        if (NULL != clientCb)
        {
            clientCb(clientContext, wStatus);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameter!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_StartWdTimer(void *pContext, pphOsalNfc_TimerCallbck_t pTimerCb, uint32_t dwTimeOut)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint32_t dwTimerId = PH_OSALNFC_TIMER_ID_INVALID;
    PH_LOG_LIBNFC_FUNC_EXIT();
    if(NULL != pCtx)
    {
        dwTimerId = phOsalNfc_Timer_Create();
        if(PH_OSALNFC_TIMER_ID_INVALID != dwTimerId)
        {
            wStatus = phOsalNfc_Timer_Start(dwTimerId,
                                            dwTimeOut,
                                            pTimerCb,
                                            (void *) pCtx);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                pCtx->WdTimerId = dwTimerId;
            }
            else
            {
                (void)phOsalNfc_Timer_Delete(dwTimerId);
                pCtx->WdTimerId = 0;
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Failed to create Timer!");
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context passed");
    }
    return wStatus;
}

static void phLibNfc_WdTimerExpiredCb(uint32_t dwTimerId, void *pContext)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        PH_LOG_LIBNFC_CRIT_STR("Timer expired: Timer restart count reached limit");
        (void)phOsalNfc_Timer_Stop(dwTimerId);
        (void)phOsalNfc_Timer_Delete(dwTimerId);
        pLibContext->WdTimerId = 0;
        phLibNfc_LaunchNfceeDiscCompleteSequence(pLibContext,NFCSTATUS_SUCCESS,NULL);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_LaunchNfceeDiscCompleteSequence(void *pContext, NFCSTATUS wStatus, void *pInfo)
{
    pphLibNfc_LibContext_t pLibContext = pContext;
    pphHciNfc_HciContext_t pHciCtx;
    static bool_t bInitialNfceeDisc = TRUE;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = NFCSTATUS_SUCCESS;
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        pHciCtx = pLibContext->pHciContext;

        if(NULL == pHciCtx)
        {
            pLibContext->dwHciInitDelay = 0;
            PH_LOG_LIBNFC_INFO_STR("No HCI network present!");
        }
        else if(pHciCtx->bClearpipes)
        {
            /* Wait for pipe creation */
            pLibContext->dwHciInitDelay = 1000;
            PH_LOG_LIBNFC_INFO_STR("HCI network initialization in progress...");
        }
        else if((0 == pLibContext->Config.bHciNwkPerNfcee) && (pLibContext->tSeInfo.bSeCount <= 1))
        {
            pLibContext->dwHciInitDelay = 0;
            PH_LOG_LIBNFC_INFO_STR("No NFCEE present!");
        }
        else
        {
            pLibContext->dwHciInitDelay = bInitialNfceeDisc
                                          ? PHHCINFC_NFCEE_DISCOVERY_INITIAL_NTF_DELAY
                                          : PHHCINFC_NFCEE_DISCOVERY_DEFAULT_NTF_DELAY;
            bInitialNfceeDisc = FALSE;
            PH_LOG_LIBNFC_INFO_STR("HCI network initialization complete!");
        }

        PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_NfceeDiscCompleteSeq);
        wStatus = phLibNfc_SeqHandler(pLibContext,wStatus,NULL);
        if(NFCSTATUS_PENDING != wStatus)
        {
            PH_LOG_LIBNFC_CRIT_STR("Sequence could not start!");
            wStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SE_GetIndex(void *pContext, phLibNfc_SE_Status_t bSeState, uint8_t *pbIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = pContext;
    uint8_t bIndex;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (NULL != pbIndex))
    {
        for (bIndex=0; bIndex<ARRAYSIZE(pLibContext->tSeInfo.bSeState); bIndex++)
        {
            if (pLibContext->tSeInfo.bSeState[bIndex] == bSeState)
            {
                *pbIndex = bIndex;
                wStatus = NFCSTATUS_SUCCESS;
                break;
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

phHciNfc_HostID_t phLibNfc_SE_GetHciHostId(void* pContext, pphNciNfc_NfceeInfo_t pNfceeInfo)
{
    pphLibNfc_LibContext_t pLibContext = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t bType = 0;
    uint8_t bLen = 0;
    uint8_t bCount = 0;
    phNciNfc_TlvUtilInfo_t tTlvInfo;
    uint8_t *pValue = NULL;

    /*If the NFCC supports the HCI Network, it SHALL return NFCEE_DISCOVER_NTF with a Protocol type of "HCI Access"
        An NFCEE_DISCOVER_NTF that contains a Protocol type of "HCI Access" SHALL NOT contain any other additional Protocol*/
    if((NULL != pNfceeInfo->pNfceeHandle) &&
        (pNfceeInfo->pNfceeHandle->tDevInfo.bNumSuppProtocols == 1) &&
        (pNfceeInfo->pNfceeHandle->tDevInfo.aSuppProtocols[0] == phNciNfc_e_NfceeHciAccessIf))
    {
        return phHciNfc_e_TerminalHostID;
    }

    /*The NFCEE ID returned by the NFCC in the NFCEE_DISCOVER_NTF is used by the DH to address the HCI network*/
    if(0 == pLibContext->Config.bHciNwkPerNfcee)
    {
        // In NCI2.0, HCI Host ID is stored in TLV fields per NCI
        // In NCI1.0 check if Host ID data was assigned anyway
        tTlvInfo.pBuffer = pNfceeInfo->pNfceeHandle->tDevInfo.pTlvInfo;
        tTlvInfo.dwLength = pNfceeInfo->pNfceeHandle->tDevInfo.TlvInfoLen;
        tTlvInfo.sizeInfo.dwOffset = 0;
        for (bCount = 0; bCount < pNfceeInfo->pNfceeHandle->tDevInfo.bNumTypeInfo; bCount++)
        {
            wStatus = phNciNfc_TlvUtilsGetNxtTlv(&tTlvInfo, &bType, &bLen, &pValue);
            if (NFCSTATUS_SUCCESS == wStatus)
            {
                if (bType == PHNCINFC_TLVUTIL_NCI_PROP_HCINWK_HOST_ID && bLen == 1)
                {
                    return pValue[0];
                }
            }
        }
        if (phNciNfc_IsVersion1x(phNciNfc_GetContext()))
        {
            /* In NCI1.0, HCI Host ID equals NFCEE ID */
            return pNfceeInfo->bNfceeId;
        }
    }
    return phHciNfc_e_HostControllerID;
}
