/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Discovery.tmh"

#define PH_LIBNFC_MICRO_TO_MILLI_SEC        (1000)
#define PH_LIBNFC_MILLI_TO_MICRO_SEC        (PH_LIBNFC_MICRO_TO_MILLI_SEC)

#define PH_LIBNFC_DUMMY_INIT_DELAY          (100)
#define PH_LIBNFC_DISCOVERY_DELAY           (25)

static void phLibNfc_DummyInitTimerCb(uint32_t TimerId, void *pContext);
static void phLibNfc_DelayDiscTimerCb(uint32_t dwTimerId, void *pContext);

static NFCSTATUS phLibNfc_SetConfigSeq(void *pContext, NFCSTATUS wStatus, void * pInfo);
static NFCSTATUS phLibNfc_ReConfigSeq(void *pContext, NFCSTATUS wStatus, void * pInfo);
static NFCSTATUS phLibNfc_SetConfigSeqEnd(void *pContext, NFCSTATUS wStatus, void *pInfo);
static NFCSTATUS phLibNfc_StartDiscSeq(void *pContext, NFCSTATUS wStatus, void *pInfo);
static NFCSTATUS phLibNfc_StartDiscSeqEnd(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_StartDiscoverSeqComplete(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_ConfigDiscParams(void *pContext, phLibNfc_sADD_Cfg_t *pADDSetup);
static NFCSTATUS phLibNfc_StartDisc(void *pContext, NFCSTATUS wStatus, void *pInfo);
static NFCSTATUS phLibNfc_DelayDisc(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_DelayDiscProc(void* pContext, NFCSTATUS wStatus, void* pInfo);
static NFCSTATUS phLibNfc_SetPowerSubStateSeq(void *pContext, NFCSTATUS wStatus, void * pInfo);
static NFCSTATUS phLibNfc_SetPowerSubStateSeqEnd(void *pContext, NFCSTATUS wStatus, void * pInfo);

static NFCSTATUS phLibNfc_SendDeactIdleCmd(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_ReDiscoveryComplete(void *pContext,NFCSTATUS wStatus,void *pInfo);

static NFCSTATUS phLibNfc_DiscDeactivateComplete(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_ProcessDeactComplete(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_InvokeDiscDisconnCb(void* pContext,NFCSTATUS wStatus,void* pInfo);

static uint8_t phLibNfc_CheckDiscParams(phLibNfc_sADD_Cfg_t *pDiscConfig);
static void phLibNfc_SetDiscPayload(void *pContext,phLibNfc_sADD_Cfg_t *pADDSetup, phNciNfc_ADD_Cfg_t *pNciConfig);

/* Following are the sequence which are being used in ConfigureDiscovery API */
/* Set config + Start Discovery */
phLibNfc_Sequence_t gphLibNfc_DiscoverSeq[] = {
    {&phLibNfc_SetPowerSubStateSeq, &phLibNfc_SetPowerSubStateSeqEnd},
    {&phLibNfc_SetConfigSeq, &phLibNfc_SetConfigSeqEnd},
    {&phLibNfc_StartDiscSeq, &phLibNfc_StartDiscSeqEnd},
    {NULL, &phLibNfc_StartDiscoverSeqComplete}
};

/* Re-start discovery without setting any configuration */
phLibNfc_Sequence_t gphLibNfc_ReStartDiscoverSeq[] = {
    {&phLibNfc_StartDiscSeq, &phLibNfc_StartDiscSeqEnd},
    {NULL, &phLibNfc_StartDiscoverSeqComplete}
};

/* Following are the sequence which are being used in Disconnect API */
/* Deactivate to Idle + Set config + Start discovery */
phLibNfc_Sequence_t gphLibNfc_ReDiscSeqWithDisc[] = {
    {&phLibNfc_SendDeactIdleCmd, &phLibNfc_ProcessDeactResp},
    {&phLibNfc_SetPowerSubStateSeq, &phLibNfc_SetPowerSubStateSeqEnd},
    {&phLibNfc_ReConfigSeq, &phLibNfc_SetConfigSeqEnd},
    {&phLibNfc_StartDiscSeq, &phLibNfc_StartDiscSeqEnd},
    {NULL, &phLibNfc_ReDiscoveryComplete}
};

/* Following are the sequence which are being used in Disconnect API */
/* Deactivate to Idle + Set config + Start discovery + Delay */
phLibNfc_Sequence_t gphLibNfc_ReDiscSeqWithDiscDelay[] = {
    {&phLibNfc_SendDeactIdleCmd, &phLibNfc_ProcessDeactResp},
    {&phLibNfc_DelayDisc, &phLibNfc_DelayDiscProc},
    {&phLibNfc_SetPowerSubStateSeq, &phLibNfc_SetPowerSubStateSeqEnd},
    {&phLibNfc_ReConfigSeq, &phLibNfc_SetConfigSeqEnd},
    {&phLibNfc_StartDiscSeq, &phLibNfc_StartDiscSeqEnd},
    {NULL, &phLibNfc_ReDiscoveryComplete}
};

/* Deactivate to Discovery */
phLibNfc_Sequence_t gphLibNfc_ReDiscSeqWithDeact[] = {
    {&phLibNfc_SendDeactDiscCmd, &phLibNfc_ProcessDeactToDiscResp},
    {NULL, &phLibNfc_ReDiscoveryComplete}
};

/* Deactivate to Sleep */
phLibNfc_Sequence_t gphLibNfc_DiscSeqWithDeactSleep[] = {
    {&phLibNfc_SendDeactSleepCmd, &phLibNfc_ProcessDeactResp},
    {NULL, &phLibNfc_ProcessDeactComplete}
};

/* Rediscovery without setting configuration */
phLibNfc_Sequence_t gphLibNfc_ReDiscSeqWithDeactAndDisc[] = {
    {&phLibNfc_SendDeactIdleCmd, &phLibNfc_ProcessDeactResp},
    {&phLibNfc_StartDiscSeq, &phLibNfc_StartDiscSeqEnd},
    {NULL, &phLibNfc_ReDiscoveryComplete}
};

/* Rediscovery without setting configuration + Delay */
phLibNfc_Sequence_t gphLibNfc_ReDiscrySeqWithDeactAndDiscDelay[] = {
    {&phLibNfc_SendDeactIdleCmd, &phLibNfc_ProcessDeactResp},
    {&phLibNfc_DelayDisc, &phLibNfc_DelayDiscProc},
    {&phLibNfc_StartDiscSeq, &phLibNfc_StartDiscSeqEnd},
    {NULL, &phLibNfc_ReDiscoveryComplete}
};

/* Deactivate to Idle */
phLibNfc_Sequence_t gphLibNfc_DeactivateToIdle[] = {
    {&phLibNfc_SendDeactIdleCmd, &phLibNfc_ProcessDeactResp},
    {NULL, &phLibNfc_DiscDeactivateComplete}
};

/* Deactivate to Idle + Delay */
phLibNfc_Sequence_t gphLibNfc_DeactivateToIdleDelay[] = {
    {&phLibNfc_SendDeactIdleCmd, &phLibNfc_ProcessDeactResp},
    {&phLibNfc_DelayDisc, &phLibNfc_DelayDiscProc},
    {NULL, &phLibNfc_DiscDeactivateComplete}
};

static void phLibNfc_DummyDiscCb(_In_ void* pContext)
{
    pphLibNfc_LibContext_t pLibContext=NULL;
    pphLibNfc_RspCb_t pClientCb ;
    pLibContext  = (pphLibNfc_LibContext_t)pContext;
    pClientCb = pLibContext->CBInfo.pClientDisConfigCb;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        if(NULL != pLibContext->CBInfo.pClientDisConfigCb)
        {
            pLibContext->CBInfo.pClientDisConfigCb = NULL;
            (pClientCb)( \
                    pLibContext->CBInfo.pClientDisCfgCntx,NFCSTATUS_SUCCESS);
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Dummy discovery No CB info Stored...");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Dummy discovery: Context is NULL");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_Mgt_ConfigureDiscovery (phLibNfc_eDiscoveryConfigMode_t  DiscoveryMode,
                                           phLibNfc_sADD_Cfg_t              sADDSetup,
                                           pphLibNfc_RspCb_t                pConfigDiscovery_RspCb,
                                           void*                            pContext
    )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventDiscovery;
    pphLibNfc_Context_t pLibContext = phLibNfc_GetContext();
    phLibNfc_sADD_Cfg_t *pSrcAddCfg = NULL;
    phNfc_eDiscAndDisconnMode_t DiscMode = NFC_DISC_CONFIG_DISCOVERY;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle;
    pphNciNfc_RemoteDevInformation_t pTempHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = NULL;
    pphLibNfc_TransceiveCallback_t pTransceiveCb =NULL;
    pphLibNfc_Receive_RspCb_t pRemDevReceiveCb =NULL;
    pphLibNfc_RspCb_t pRemDevSendCb =NULL;
    uint8_t bRetVal;
    void *pUpperLayerCtx = NULL;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = phLibNfc_IsInitialised(pLibContext);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
    }
    else if(NULL == pConfigDiscovery_RspCb)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Map actual discovery config mode to internal disc config mode */
        switch(DiscoveryMode)
        {
            /* Consider new discovery configuration */
            case NFC_DISCOVERY_CONFIG:
            {
                /* Store ADD config only if the mode is configure */
                phOsalNfc_MemCopy(&(pLibContext->tADDconfig),\
                              &(sADDSetup),\
                              sizeof(phLibNfc_sADD_Cfg_t));
                pSrcAddCfg = &sADDSetup;
                DiscMode = NFC_DISC_CONFIG_DISCOVERY;
            }
            break;
            /* Start discovery with previously set configuration */
            case NFC_DISCOVERY_START:
            {
                pSrcAddCfg = &(pLibContext->tADDconfig);
                DiscMode = NFC_DISC_START_DISCOVERY;
            }
            break;
            case NFC_DISCOVERY_RESUME:
            {
                /*Check if Device Handle Available*/
                /*Check if Session is opened*/
                DiscMode = NFC_DISC_RESUME_DISCOVERY;
                pSrcAddCfg = &(pLibContext->tADDconfig);
                if(NULL == pLibContext->Connected_handle)
                {
                    if(pLibContext->dev_cnt > 1)
                    {
                        DiscMode = NFC_DISC_START_DISCOVERY;
                    }
                }
                else
                {
                    pTempHandle = pLibContext->Connected_handle;
                    wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,
                                                          &pTempHandle,
                                                          PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
                    if(wStatus != NFCSTATUS_SUCCESS)
                    {
                         PH_LOG_LIBNFC_CRIT_STR("Mapping of LibNfc RemoteDev Handle to NCI RemoteDev Handle Failed");
                    }
                    else
                    {
                        if((NULL != pLibRemDevHandle) && (pLibRemDevHandle->SessionOpened != 1))
                        {
                            DiscMode = NFC_DISC_START_DISCOVERY;
                        }
                    }
                }
            }
            break;
            default:
            {
                pSrcAddCfg = &sADDSetup;
                DiscMode = NFC_DISC_CONFIG_DISCOVERY;
            }
            break;
        }

        if(NULL != pLibContext->Connected_handle)
        {
            pNciRemoteDevHandle = (pphNciNfc_RemoteDevInformation_t)pLibContext->Connected_handle;
            /*If remote device is a P2P Target, raise the priority of Discovery.
            If Transceive is in progress, invoke transceive call back with 'NFCSTATUS_TARGET_LOST' status */
            if(phNciNfc_eNfcIP1_Target == pNciRemoteDevHandle->RemDevType)
            {
                PH_LOG_LIBNFC_INFO_STR("P2P Target detected as remote device type,\
                       try priority discovery");
                /* Raise the priority of Disconnect functionality */
                (void)phLibNfc_EnablePriorityDiscDiscon((void *)pLibContext);
                phNciNfc_AbortDataTransfer(pLibContext->sHwReference.pNciHandle);
                if(NULL != pLibContext->CBInfo.pClientTranscvCb)
                {
                    PH_LOG_LIBNFC_INFO_STR("Found pClientTranscvCb as valid");
                    wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemoteDevHandle,
                                    PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        pTransceiveCb = pLibContext->CBInfo.pClientTranscvCb;
                        pUpperLayerCtx = pLibContext->CBInfo.pClientTranscvCntx;
                        pLibContext->CBInfo.pClientTranscvCb = NULL;
                        pLibContext->CBInfo.pClientTranscvCntx = NULL;
                        PH_LOG_LIBNFC_INFO_STR("Invoking pClientTranscvCb...");
                        pTransceiveCb(pUpperLayerCtx,(phLibNfc_Handle)pLibRemDevHandle,
                                        NULL,NFCSTATUS_TARGET_LOST);
                    }
                }
                /* No Transceive operation in progress. To overcome the LLCP Symmetry Timer delay w.r.t LLCP send
                operation which was creating issue, a delay of 1 sec shall be introduced after Discovery to Idle 
                if UICC is not present or between Discovery to Idle and Start polling if UICC is present */
                else
                {
                    PH_LOG_LIBNFC_INFO_STR("In P2P Initiator mode: No P2P Transceive call back found");
                    /* Delay to be introduced in case of configure discovery mode */
                    pLibContext->bDiscDelayFlag = 1;
                }
            }
        }
        /* If we are P2P Target, connected handle would be invalid always */
        /* If RemoteDev_Receive or RemoteDev_Send call back functions are valid, invoke them with
           'NFCSTATUS_TARGET_LOST' status */
        else if(pLibContext->psRemoteDevInfo != NULL)
        {
            if(phNfc_eNfcIP1_Initiator == pLibContext->psRemoteDevInfo->RemDevType ||
               phNfc_eISO14443_A_PCD == pLibContext->psRemoteDevInfo->RemDevType ||
               phNfc_eISO14443_B_PCD == pLibContext->psRemoteDevInfo->RemDevType)
            {
                PH_LOG_LIBNFC_INFO_STR("P2P Initiator or PCD detected as remote device type,\
                       try priority discovery");
                /* Raise the priority of Disconnect functionality */
                (void)phLibNfc_EnablePriorityDiscovery((void *)pLibContext);
                phNciNfc_AbortDataTransfer(pLibContext->sHwReference.pNciHandle);
                if(NULL != pLibContext->CBInfo.pClientNfcIpRxCb)
                {
                    pRemDevReceiveCb = pLibContext->CBInfo.pClientNfcIpRxCb;
                    pUpperLayerCtx = pLibContext->CBInfo.pClientNfcIpRxCntx;
                    pLibContext->CBInfo.pClientNfcIpRxCb = NULL;
                    pLibContext->CBInfo.pClientNfcIpRxCntx = NULL;
                    PH_LOG_LIBNFC_INFO_STR("Found pClientNfcIpRxCb as valid, invoking the same");
                    /* Receive in progress, Notify application through Receive call back function */
                    pRemDevReceiveCb(pUpperLayerCtx,NULL,NFCSTATUS_TARGET_LOST);
                }
                else if(NULL != pLibContext->CBInfo.pClientNfcIpTxCb)
                {
                    pRemDevSendCb = pLibContext->CBInfo.pClientNfcIpTxCb;
                    pUpperLayerCtx = pLibContext->CBInfo.pClientNfcIpTxCntx;
                    pLibContext->CBInfo.pClientNfcIpTxCb = NULL;
                    pLibContext->CBInfo.pClientNfcIpTxCntx = NULL;
                    PH_LOG_LIBNFC_INFO_STR("Found pClientNfcIpTxCb as valid, invoking the same");
                    /* Send in progress, Notify application through Send call back function */
                    pRemDevSendCb(pUpperLayerCtx,NFCSTATUS_TARGET_LOST);
                }
                else
                {
                    PH_LOG_LIBNFC_INFO_STR("In Target mode: No Send/Receive call back found");
                    pLibContext->bDiscDelayFlag = 1;
                }
            }
        }

        /* Check if the input 'sADDSetup' has valid configurations */
        bRetVal = phLibNfc_ChkDiscoveryTypeAndMode((void *)&DiscMode, (void *) pSrcAddCfg, (void *)&TrigEvent);
        if(0 == bRetVal)
        {
            pLibContext->DiscDisconnMode = DiscMode;
            /* To skip Transceive exit function in libnfc statemachine */
            if(NFC_DISCOVERY_CONFIG != DiscoveryMode)
            {
                pLibContext->bDiscDelayFlag = 0;
            }
            pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_INPROGRESS;
            wStatus = phLibNfc_StateHandler(pLibContext,
                                    TrigEvent,
                                    (void *)DiscMode,
                                    (void *)pSrcAddCfg,
                                    NULL);

            if(NFCSTATUS_PENDING == wStatus)
            {
                pLibContext->CBInfo.pClientDisConfigCb = pConfigDiscovery_RspCb;
                pLibContext->CBInfo.pClientDisCfgCntx = pContext;
            }
            else if(NFCSTATUS_SHUTDOWN == wStatus)
            {
                pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
                wStatus = NFCSTATUS_SHUTDOWN;
            }
            else if(NFCSTATUS_FAILED == wStatus)
            {
                pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
            }
            else
            {
                pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
            }
        }
        else
        {
            /* sADDSetup is empty */
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SendDeactDiscCmd(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    phNciNfc_DeActivateType_t eDeactType = phNciNfc_e_DiscMode;
    UNUSED(status);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    /*Derive NCI Deactivate type from  pLibContext->ReleaseType = ReleaseType;*/
    wStatus = phNciNfc_Deactivate((void *)pCtx->sHwReference.pNciHandle,
                                  eDeactType,
                                  (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                  (void *)pContext);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SendDeactSleepCmd(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    phNciNfc_DeActivateType_t eDeactType;
    pphNciNfc_RemoteDevInformation_t pConnected_Handle = NULL;
    UNUSED(status);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    pConnected_Handle = (pphNciNfc_RemoteDevInformation_t)pCtx->Connected_handle;
    eDeactType = (phNciNfc_e_RfProtocolsNfcDepProtocol == pConnected_Handle->eRFProtocol) ? phNciNfc_e_SleepAfMode : phNciNfc_e_SleepMode;

    /*Derive NCI Deactivate type from  pLibContext->ReleaseType = ReleaseType;*/
    wStatus = phNciNfc_Deactivate((void *)pCtx->sHwReference.pNciHandle,
                                  eDeactType,
                                  (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                  (void *)pContext);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_ProcessDeactToDiscResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Discovery started, update flag which tracks discovery progress */
            pLibContext->bDiscoverInProgress = 1;
            PH_LOG_LIBNFC_INFO_STR("Deactivation to discovery success");
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Deactivate discovery failed!");
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_ProcDeact2IdleResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Deactive to Idle success");
    }
    else
    {
        /* If Deactv to Idle fails, consider it as succeded and go on with the next sequence */
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Deinit: Deactivate to Idile failed, overwriting status with SUCCESS");
        wStatus = NFCSTATUS_SUCCESS;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_ProcessDeactResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Deactivation success");
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Deactivate failed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phLibNfc_ProcessDeactComplete(void *pContext, NFCSTATUS status, void *pInfo)
{
    NFCSTATUS    wStatus = status;
    NFCSTATUS    wRetVal = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t )pContext;
    pphLibNfc_DisconnectCallback_t pDisconnNtfCb = NULL;
    void  *pUpper_Context = NULL;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pLibContext)
    {
        pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;

        pDisconnNtfCb = pLibContext->CBInfo.pClientDisConnectCb;
        pUpper_Context = pLibContext->CBInfo.pClientDConCntx;
        pLibContext->CBInfo.pClientDisConnectCb = NULL;
        pLibContext->CBInfo.pClientDConCntx = NULL;
        if(NFCSTATUS_SUCCESS == PHNFCSTATUS(status))
        {
            PH_LOG_LIBNFC_INFO_STR("Lower layer has returned NFCSTATUS_SUCCESS");
            pLibContext->Connected_handle = NULL;
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Buffer passed by Lower layer is NULL");
            wStatus = NFCSTATUS_FAILED;
        }

        phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus), &TrigEvent);
        wRetVal = phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);
        /* Disconnect call back needs to be invoked */
        if(NULL != pDisconnNtfCb)
        {
            pDisconnNtfCb(pUpper_Context, (phLibNfc_Handle)NULL, wStatus);
        }
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_ProcessDiscReq(void *pCtx,
                                  phNfc_eDiscAndDisconnMode_t RequestedMode,
                                  phLibNfc_sADD_Cfg_t         *pADDSetup,
                                  void *Param3)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pContext = pCtx;
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL == pContext)
    {
        wStatus=NFCSTATUS_NOT_INITIALISED;
    }
    else
    {
        switch(RequestedMode)
        {
            case NFC_DISC_CONFIG_DISCOVERY:
            {
                if(NULL != pADDSetup)
                {
                    /* Initialize sequence handler with normal Set Config +
                    Start Disc sequence */
                    PHLIBNFC_INIT_SEQUENCE(pContext,gphLibNfc_DiscoverSeq);
                    wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,(void *)pADDSetup);
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("pADDSetup is NULL");
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                }
            }
            break;
            case NFC_DISC_START_DISCOVERY:
            {
                /* Previous configuration exists, initializing sequence handler */
                PHLIBNFC_INIT_SEQUENCE(pContext,gphLibNfc_ReStartDiscoverSeq);
                /* ReStart discover sequence */
                wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,(void *)pADDSetup);
            }
            break;
            case NFC_DISC_RESUME_DISCOVERY:
            {
                /* This option can not be supported as discovery process is not started */
                wStatus = NFCSTATUS_FAILED;
            }
            break;
            default:
            {
                PH_LOG_LIBNFC_CRIT_STR("Invalid Request Mode ");
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_DummyDisc(void *pContext,pphOsalNfc_TimerCallbck_t pTimerCb,
                             uint32_t dwTimeOut)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint32_t dwTimerId = PH_OSALNFC_TIMER_ID_INVALID;
    PH_LOG_LIBNFC_FUNC_ENTRY();
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
                wStatus = NFCSTATUS_PENDING;
            }
            else
            {
                (void)phOsalNfc_Timer_Delete(dwTimerId);
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

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_ProcessReDiscReq(void *pCtx,
                                    phNfc_eDiscAndDisconnMode_t RequestedMode,
                                    phLibNfc_sADD_Cfg_t         *pADDSetup,
                                    void *Param3)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    NFCSTATUS wRetVal = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pContext = pCtx;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle = NULL;
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL == pContext)
    {
        wStatus=NFCSTATUS_NOT_INITIALISED;
    }
    else
    {
        /*In case of Reactivation failed then Discovery mode always be Restart not continue As per Nci state machine*/
        if(NULL != pContext->Connected_handle)
        {
            pNciRemoteDevHandle = pContext->Connected_handle;

            wRetVal = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,&pNciRemoteDevHandle,PH_LIBNFC_INTERNAL_NCITOLIB_MAP);

            if((NFCSTATUS_SUCCESS == wRetVal) &&\
               (NULL != pLibRemoteDevHandle) &&\
               (pLibRemoteDevHandle->SessionOpened == 0x00))
            {
                RequestedMode = NFC_DISCONN_RESTART_DISCOVERY;
            }
        }

        if(NULL != pContext->Connected_handle)
        {
            pNciRemoteDevHandle = pContext->Connected_handle;

            if(NFCSTATUS_SUCCESS == phLibNfc_ChkMfCTag(pNciRemoteDevHandle))
            {
                RequestedMode = NFC_DISCONN_RESTART_DISCOVERY;
            }
        }

        switch(RequestedMode)
        {
            /* Deactivate and then start discovery (take new configuration - SetConfig) */
            case NFC_DISC_CONFIG_DISCOVERY:
            {
                if(NULL != pADDSetup)
                {
                    if(1 == pContext->bDiscDelayFlag)
                    {
                        pContext->bDiscDelayFlag = 0;
                        /* Deactivate with Idle mode + Set Config + Discovery + Delay sequence */
                        PHLIBNFC_INIT_SEQUENCE(pContext,gphLibNfc_ReDiscSeqWithDiscDelay);
                    }
                    else
                    {
                        /* Deactivate with Idle mode + Set Config + Discovery sequence */
                        PHLIBNFC_INIT_SEQUENCE(pContext,gphLibNfc_ReDiscSeqWithDisc);
                    }

                    wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,(void *)pADDSetup);
                }
                else
                {
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                }
            }
            break;
            /* Deactivate with discovery (NFCC takes previous configuration) */
            case NFC_DISC_RESUME_DISCOVERY:
            case NFC_DISCONN_CONTINUE_DISCOVERY:
            {
                /* This check is to avoid Discovery resume when discovery is already in progress
                   Also when a remote device is already connected */
                if((NFC_DISC_RESUME_DISCOVERY == RequestedMode) &&
                   (1 == pContext->bDiscoverInProgress))
                {
                    wStatus = NFCSTATUS_FAILED;
                }
                else
                {
                    /* Deactivate with Discovery mode */
                    if(1 == pContext->bDiscDelayFlag)
                    {
                        pContext->bDiscDelayFlag = 0;
                        if(NULL != pLibRemoteDevHandle)
                        {
                            /* Delay to be introduced only when session opened=0 */
                            if(pLibRemoteDevHandle->SessionOpened != 0x00)
                            {
                                PHLIBNFC_INIT_SEQUENCE(pContext,gphLibNfc_ReDiscrySeqWithDeactAndDiscDelay);
                            }
                            else
                            {
                                PHLIBNFC_INIT_SEQUENCE(pContext,gphLibNfc_ReDiscSeqWithDeact);
                            }
                        }
                    }
                    else
                    {
                        PHLIBNFC_INIT_SEQUENCE(pContext,gphLibNfc_ReDiscSeqWithDeact);
                    }

                    wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,NULL);
                }
            }
            break;
            case NFC_DISC_START_DISCOVERY: /* Configure_Discovery with 'Start' */
            case NFC_DISCONN_RESTART_DISCOVERY: /* Disconnect with 'Restart' - Start disc cycle from beginning */
            {
                /* This check is to avoid Discovery start when discovery is already in progress
                   Also when a remote device is already connected */
                if((NFC_DISC_START_DISCOVERY == RequestedMode) &&
                   (1 == pContext->bDiscoverInProgress))
                {
                    wStatus = NFCSTATUS_FAILED;
                }
                else
                {
                    if(1 == pContext->bDiscDelayFlag)
                    {
                        pContext->bDiscDelayFlag = 0;
                        /* Deactivate to Idle and restart discovery mode */
                        PHLIBNFC_INIT_SEQUENCE(pContext,gphLibNfc_ReDiscrySeqWithDeactAndDiscDelay);
                    }
                    else
                    {
                        /* Deactivate to Idle and restart discovery mode */
                        PHLIBNFC_INIT_SEQUENCE(pContext,gphLibNfc_ReDiscSeqWithDeactAndDisc);
                    }
                    wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,NULL);
                }
            }
            break;
            /* Async Deactivate ntf received from NFCC */
            case NFC_INTERNAL_CONTINUE_DISCOVERY: /* Nfcc requested Deactivate to discover state */
            {
                /* Nfcc has started discovery, update the status flag */
                pContext->bDiscoverInProgress = 1;
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
            default:
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Actv2Init(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    // Warning C4305: 'type cast': truncation from 'void *' to 'phNfc_eDiscAndDisconnMode_t'
#pragma warning(suppress:4305)
    phNfc_eDiscAndDisconnMode_t ReleaseType = (phNfc_eDiscAndDisconnMode_t)Param1;

    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        if(NFC_INTERNAL_STOP_DISCOVERY != ReleaseType)
        {
            if(1 == pCtx->bDiscDelayFlag)
            {
                PHLIBNFC_INIT_SEQUENCE(pCtx,gphLibNfc_DeactivateToIdleDelay);
                pCtx->bDiscDelayFlag = 0;
            }
            else
            {
                /* Deactivate with Idle mode */
                PHLIBNFC_INIT_SEQUENCE(pCtx,gphLibNfc_DeactivateToIdle);
            }
            wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,NULL);
        }
        else
        {
            /* Async Deactivate ntf received from NFCC */
            /* Nfcc requested Deactivate to Idle state */
            wStatus = NFCSTATUS_SUCCESS;
        }
        /* Update discovery progress status flag */
        pCtx->bDiscoverInProgress = 0;
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context passed");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_DummyInit(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint32_t dwTimerId = PH_OSALNFC_TIMER_ID_INVALID;

    // Warning C4305: 'type cast': truncation from 'void *' to 'phNfc_eDiscAndDisconnMode_t'
#pragma warning(suppress:4305)
    phNfc_eDiscAndDisconnMode_t RequestedMode = (phNfc_eDiscAndDisconnMode_t)Param1;

    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_EXIT();
    if(NULL != pCtx)
    {
        /* If disc mode is config, then handle it, else return failed */
        if(NFC_DISC_CONFIG_DISCOVERY == RequestedMode)
        {
            dwTimerId = phOsalNfc_Timer_Create();
            if(PH_OSALNFC_TIMER_ID_INVALID != dwTimerId)
            {
                wStatus = phOsalNfc_Timer_Start(dwTimerId,
                                                PH_LIBNFC_DUMMY_INIT_DELAY,
                                                &phLibNfc_DummyInitTimerCb,
                                                (void *) pCtx);
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    wStatus = NFCSTATUS_PENDING;
                }
                else
                {
                    (void)phOsalNfc_Timer_Delete(dwTimerId);
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
            PH_LOG_LIBNFC_CRIT_STR("No previous configuration available, discovery failed!");
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context passed");
    }
    return wStatus;
}

static void phLibNfc_DummyInitTimerCb(uint32_t dwTimerId, void *pContext)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    pphLibNfc_RspCb_t pClientCb;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        (void)phOsalNfc_Timer_Stop(dwTimerId);
        (void)phOsalNfc_Timer_Delete(dwTimerId);

        pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
        pClientCb = pLibContext->CBInfo.pClientDisConfigCb;
        pLibContext->CBInfo.pClientDisConfigCb = NULL;
        (void)phLibNfc_StateHandler(pLibContext, TrigEvent, NULL, NULL, NULL);
        if(NULL != pClientCb)
        {
            pClientCb(pLibContext->CBInfo.pClientDisCfgCntx,wStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

static NFCSTATUS phLibNfc_SetConfigSeq( void *pContext, NFCSTATUS wStatus, void * pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_LibContext_t pLibContext = pContext;
    UNUSED(wStatus);
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        wIntStatus = phLibNfc_ConfigDiscParams(pContext,&pLibContext->tADDconfig);
        if(NFCSTATUS_SUCCESS == wIntStatus)
        {
            PH_LOG_LIBNFC_CRIT_STR("No Set Config requested before starting discovery");
            /* As, set configuration is not required, jumping to next sequence */
            pLibContext->SeqNext++;
            wIntStatus = phLibNfc_StartDisc(pContext,NFCSTATUS_SUCCESS,&pLibContext->tADDconfig);
            if(NFCSTATUS_PENDING != wIntStatus)
            {
                PH_LOG_LIBNFC_CRIT_STR("Failed to start discovery...");
                /* Decrement next sequence so that we return proper error to the upper layer */
                pLibContext->SeqNext--;
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS phLibNfc_ReConfigSeq( void *pContext, NFCSTATUS wStatus, void * pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_LibContext_t pLibContext = pContext;
    UNUSED(wStatus);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        wIntStatus = phLibNfc_ConfigDiscParams(pContext,&pLibContext->tADDconfig);
        if(NFCSTATUS_PENDING == wIntStatus)
        {
            PH_LOG_LIBNFC_INFO_STR("Re-configure success");
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Re-configure failed!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS
phLibNfc_StartDiscSeq(void *pContext,
                      NFCSTATUS wStatus,
                      void *pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_INVALID_PARAMETER;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    uint8_t bRetVal = 1;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        /* Check if AddConfig is empty or not */
        bRetVal = phLibNfc_ChkDiscoveryType(NULL, (void *) &pLibContext->tADDconfig);
        if(0 == bRetVal)
        {
            wIntStatus = phLibNfc_StartDisc(pContext,wStatus,&pLibContext->tADDconfig);
        }
        else
        {
            /* AddConfig contains nothing, pass success to upper layer */
            PH_LOG_LIBNFC_INFO_STR("Recovery Seq: Discovery 'AddConfig' empty, passing SUCCESS to caller");
            wStatus = phLibNfc_DummyDisc(pContext,&phLibNfc_DelayDiscTimerCb,PH_LIBNFC_DISCOVERY_DELAY);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS
phLibNfc_StartDisc(void *pContext,
                      NFCSTATUS wStatus,
                      void *pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_sADD_Cfg_t *pADDSetup = pInfo;
    phNciNfc_ADD_Cfg_t tNciConfig = {0};
    UNUSED(wStatus);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL == pLibContext) || (NULL == pADDSetup))
    {
        wIntStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        phLibNfc_SetDiscPayload(pLibContext,pADDSetup,&tNciConfig);
        /* TODO - Check how to configure Listen mode */
        wIntStatus = phNciNfc_StartDiscovery(pLibContext->sHwReference.pNciHandle,
                                            &tNciConfig,
                                            (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                            pLibContext);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS phLibNfc_SetConfigSeqEnd(void *pContext, NFCSTATUS wStatus, void *pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = pContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        wIntStatus = wStatus;
        if(NFCSTATUS_SUCCESS == wIntStatus)
        {
            PH_LOG_LIBNFC_INFO_STR("Set Config before Start Disc success");
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Set Config before Start Disc Failed!");
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS phLibNfc_StartDiscSeqEnd(void* pContext,NFCSTATUS wStatus,void* pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        wIntStatus = wStatus;
        if(NFCSTATUS_SUCCESS == wIntStatus)
        {
            /* Discovery started, update flag which tracks discovery progress */
            pLibContext->bDiscoverInProgress = 1;
            PH_LOG_LIBNFC_INFO_STR("Discovery started...");
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Discover start Failed!");
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wIntStatus;
}

static NFCSTATUS phLibNfc_StartDiscoverSeqComplete(void* pContext,NFCSTATUS wStatus,void* pInfo)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
        if( NFCSTATUS_SUCCESS != (PHNFCSTATUS(wStatus)))
        {
            /*If status is not NFCSTATUS_SUCCESS then pass NFCSTATUS_FAILED to application layer*/
            /* TrigEvent = phLibNfc_EventFailed; */
            wStatus = NFCSTATUS_FAILED;
        }

        phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus),&TrigEvent);
        (void)phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);
        
        wStatus = phLibNfc_InvokeDiscDisconnCb(pLibContext, wStatus, pInfo);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_ConfigDiscParams(void *pContext,
                                    phLibNfc_sADD_Cfg_t *pADDSetup)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = pContext;
    phNciNfc_RfDiscConfigParams_t tNciDiscConfig = {0};

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL == pLibContext)
    {
        wStatus=NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL == pADDSetup)
    {
        wStatus=NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Check whether P2P is configured */
        pADDSetup->PollDevInfo.PollCfgInfo.EnableIso14443A = \
            ( (pADDSetup->PollDevInfo.PollCfgInfo.EnableIso14443A)\
                    || ( (pADDSetup->NfcIP_Mode) & (uint8_t)phNfc_ePassive106) );

        /* Check whether bail out is enabled for NFC-A technology */
        if((1 == pADDSetup->PollDevInfo.PollCfgInfo.EnableIso14443A) &&
            (pADDSetup->aPollParms[0].Bailout != (uint8_t)-1))
        {
            tNciDiscConfig.tConfigInfo.PollNfcAConfig = 1;
            tNciDiscConfig.tPollNfcADiscParams.PollNfcAConfig.Config.SetBailOut = 1;
            /* If Bailout provided by user is not correct, reset bailout */
            if(1 == pADDSetup->aPollParms[0].Bailout)
            {
                tNciDiscConfig.tPollNfcADiscParams.bBailOut = 1;
            }
            else
            {
                tNciDiscConfig.tPollNfcADiscParams.bBailOut = 0;
            }
        }

        /* Check whether bail out is enabled for NFC-B technology */
        if((1 == pADDSetup->PollDevInfo.PollCfgInfo.EnableIso14443B) &&
            (pADDSetup->aPollParms[1].Bailout != (uint8_t)-1))
        {
            tNciDiscConfig.tConfigInfo.PollNfcBConfig = 1;
            tNciDiscConfig.tPollNfcBDiscParams.PollNfcBConfig.Config.SetBailOut = 1;
            if(1 == pADDSetup->aPollParms[1].Bailout)
            {
                tNciDiscConfig.tPollNfcBDiscParams.bBailOut = 1;
            }
            else
            {
                tNciDiscConfig.tPollNfcBDiscParams.bBailOut = 0;
            }
        }

#ifdef NFC_PF_BIT_RATE
        /* Bit rate configuration for P2P 212 or 424 */
        tNciDiscConfig.tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate = 0;
        /* Enable all bit rates */
        if((phNfc_eP2P_ALL == pADDSetup->NfcIP_Mode) ||
            (((phNfc_ePassive212 & pADDSetup->NfcIP_Mode) == phNfc_ePassive212) &&
                ((phNfc_ePassive424 & pADDSetup->NfcIP_Mode) == phNfc_ePassive424)))
        {
            tNciDiscConfig.tConfigInfo.PollNfcFConfig = 1;
            tNciDiscConfig.tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate = 1;
            /* Consider 212 as priority bit rate */
            tNciDiscConfig.tPollNfcFDiscParams.bBitRate = (uint8_t)phNciNfc_e_BitRate212;
        }
        else if((phNfc_ePassive212 == (phNfc_ePassive212 & pADDSetup->NfcIP_Mode)) || (phNfc_eActive212 == (phNfc_eActive212 & pADDSetup->NfcIP_Mode)))
        {
            tNciDiscConfig.tConfigInfo.PollNfcFConfig = 1;
            tNciDiscConfig.tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate = 1;
            tNciDiscConfig.tPollNfcFDiscParams.bBitRate = (uint8_t)phNciNfc_e_BitRate212;
        }
        else if((phNfc_ePassive424 == (phNfc_ePassive424 & pADDSetup->NfcIP_Mode)) || (phNfc_eActive424 == (phNfc_eActive424 & pADDSetup->NfcIP_Mode)))
        {
            tNciDiscConfig.tConfigInfo.PollNfcFConfig = 1;
            tNciDiscConfig.tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate = 1;
            tNciDiscConfig.tPollNfcFDiscParams.bBitRate = (uint8_t)phNciNfc_e_BitRate424;
        }
        else
        {
            /* P2P not enabled for Nfc-F technology, no bitrate configuration is needed */
        }
#endif

        /* Check if Felica Tag polling is enabled. If yes, enable both bit rates (212 and 424) */
        if((1 == pADDSetup->PollDevInfo.PollCfgInfo.EnableFelica424) &&
            (1 == pADDSetup->PollDevInfo.PollCfgInfo.EnableFelica212))
        {
            tNciDiscConfig.tConfigInfo.PollNfcFConfig = 1;
            tNciDiscConfig.tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate = 1;
            /* Consider 212 as priority bit rate */
            tNciDiscConfig.tPollNfcFDiscParams.bBitRate = (uint8_t)phNciNfc_e_BitRate212;
        }
        /* Check if Felica Tag polling is enabled for 212 bit rate */
        else if(1 == pADDSetup->PollDevInfo.PollCfgInfo.EnableFelica212)
        {
            tNciDiscConfig.tConfigInfo.PollNfcFConfig = 1;
            tNciDiscConfig.tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate = 1;
            tNciDiscConfig.tPollNfcFDiscParams.bBitRate = (uint8_t)phNciNfc_e_BitRate212;
        }
        /* Check if Felica Tag polling is enabled for 424 bit rate */
        else if(1 == pADDSetup->PollDevInfo.PollCfgInfo.EnableFelica424)
        {
            tNciDiscConfig.tConfigInfo.PollNfcFConfig = 1;
            tNciDiscConfig.tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate = 1;
            tNciDiscConfig.tPollNfcFDiscParams.bBitRate = (uint8_t)phNciNfc_e_BitRate424;
        }

#ifdef NFC_TOTAL_DURATION
        tNciDiscConfig.tConfigInfo.CommonConfig = 1;
        tNciDiscConfig.tCommonDiscParams.ComnParamsConfig.Config.SetTotalDuration = 1;
        /* Check if input duration is less than min required duration */
        if(pADDSetup->Duration < (NFC_TOTAL_DURATION_VALUE * PH_LIBNFC_MILLI_TO_MICRO_SEC))
        {
            tNciDiscConfig.tCommonDiscParams.wTotalDuration = (uint16_t) NFC_TOTAL_DURATION_VALUE;
        }
        else
        {
            /* Converting use input duration from micro seconds to milli seconds */
            tNciDiscConfig.tCommonDiscParams.wTotalDuration = (uint16_t)((pADDSetup->Duration)/
                                        PH_LIBNFC_MICRO_TO_MILLI_SEC);
        }
#endif

#ifdef NFC_LA_SEL_INFO
        /* P2P Target enable/disable configuration */
        if(1 == pADDSetup->NfcIP_Tgt_Disable && pLibContext->PwrSubState == phNciNfc_e_SwitchedOnState)
        {
            /*Disable NFC-DEP Protocol support*/
            tNciDiscConfig.tLstnNfcADiscParams.SelInfo.bNfcDepProtoSupport = 0;
        }
        else
        {
            /* Enable Nfc-A P2P 106 Target */
            tNciDiscConfig.tLstnNfcADiscParams.SelInfo.bNfcDepProtoSupport = 1;
        }

        /* Listen Nfc-A Nfc-Dep protocol support */
        tNciDiscConfig.tConfigInfo.LstnNfcAConfig = 1;
        tNciDiscConfig.tLstnNfcADiscParams.LstnNfcAConfig.Config.SetSelInfo = 1;
        tNciDiscConfig.tLstnNfcADiscParams.SelInfo.bIsoDepProtoSupport = 0;

        if(0 == pADDSetup->PollDevInfo.PollCfgInfo.DisableCardEmulation)
        {
            if(pLibContext->bHceSak > 0)
            {
                tNciDiscConfig.tLstnNfcADiscParams.SelInfo.bIsoDepProtoSupport = 1;
            }
            else if(pLibContext->tSeInfo.bSeCount > 0)
            {
                if ((PH_LIBNFC_SE_LSTN_NFC_A_SUPP & pLibContext->tSeInfo.bNfceeSuppNfcTech) != 0)
                {
                    if (pLibContext->tNfccFeatures.DiscConfigInfo.DiscConfigMode == 0 ||
                        pLibContext->tNfccFeatures.ManufacturerId == PH_LIBNFC_MANUFACTURER_QC)
                    {
                        PH_LOG_LIBNFC_CRIT_STR("Enabling SAK for Nfc-A Iso-Dep since NFCEE doesn't configure NFCC");
                        tNciDiscConfig.tLstnNfcADiscParams.SelInfo.bIsoDepProtoSupport = 1;
                    }
                }
            }
        }
#endif

#ifdef NFC_LF_PROTOCOL_TYPE_NFC_DEP_SUPPORT
        /* Listen Nfc-F Nfc-Dep protocol support */
        tNciDiscConfig.tConfigInfo.LstnNfcFConfig = 1;
        tNciDiscConfig.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetProtocolType = 1;

        if(1 == pADDSetup->NfcIP_Tgt_Disable && pLibContext->PwrSubState == phNciNfc_e_SwitchedOnState)
        {
            tNciDiscConfig.tLstnNfcFDiscParams.ProtocolType.bNfcDepProtocolSupport = 0;
        }
        else
        {
            tNciDiscConfig.tLstnNfcFDiscParams.ProtocolType.bNfcDepProtocolSupport = 1;
        }
#endif

        /* Check whether any configuration is set. If yes, then invoke
           Set Config Command */
        if((1 == tNciDiscConfig.tConfigInfo.PollNfcAConfig)      || \
           (1 == tNciDiscConfig.tConfigInfo.PollNfcBConfig)      || \
           (1 == tNciDiscConfig.tConfigInfo.PollNfcFConfig)      || \
           (1 == tNciDiscConfig.tConfigInfo.PollIsoDepConfig)    || \
           (1 == tNciDiscConfig.tConfigInfo.PollNfcDepConfig)    || \
           (1 == tNciDiscConfig.tConfigInfo.LstnNfcAConfig)      || \
           (1 == tNciDiscConfig.tConfigInfo.LstnNfcBConfig)      || \
           (1 == tNciDiscConfig.tConfigInfo.LstnNfcFConfig)      || \
           (1 == tNciDiscConfig.tConfigInfo.LstnIsoDepConfig)    || \
           (1 == tNciDiscConfig.tConfigInfo.LstnNfcDepConfig)    || \
           (1 == tNciDiscConfig.tConfigInfo.PollNfcAKovioConfig) || \
           (1 == tNciDiscConfig.tConfigInfo.CommonConfig))
        {
            wStatus = phNciNfc_SetConfigRfParameters(pLibContext->sHwReference.pNciHandle,
                                            &tNciDiscConfig,
                                            (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                            (void *)pLibContext);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_SetDiscPayload(void *pContext,phLibNfc_sADD_Cfg_t *pADDSetup,
                                    phNciNfc_ADD_Cfg_t *pNciConfig)
{
    pphLibNfc_LibContext_t pLibContext = pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    /* Check whether Poll mode needs to be enabled */
    if(pADDSetup->PollDevInfo.PollEnabled)
    {
        pNciConfig->EnableIso14443A = pADDSetup->PollDevInfo.PollCfgInfo.EnableIso14443A;
        pNciConfig->EnableIso14443B = pADDSetup->PollDevInfo.PollCfgInfo.EnableIso14443B;
        pNciConfig->EnableFelica = ((pADDSetup->PollDevInfo.PollCfgInfo.EnableFelica212) ||
                                    (pADDSetup->PollDevInfo.PollCfgInfo.EnableFelica424));
        pNciConfig->EnableIso15693 = pADDSetup->PollDevInfo.PollCfgInfo.EnableIso15693;
        pNciConfig->EnableKovio = pADDSetup->PollDevInfo.PollCfgInfo.EnableKovio;
    }
    /* Configure for P2P Initiator Device */
    /* Enable active mode */
    if(pADDSetup->PollDevInfo.PollCfgInfo.EnableNfcActive == 1)
    {
        pNciConfig->PollNfcAActive = 0;
        pNciConfig->PollNfcFActive = 0;
        
        if ((phNfc_eActive106 & pADDSetup->NfcIP_Mode) == phNfc_eActive106)
        {
            pNciConfig->PollNfcAActive = 1;
        }

        if (((phNfc_eActive212 & pADDSetup->NfcIP_Mode) == phNfc_eActive212) ||
            ((phNfc_eActive424 & pADDSetup->NfcIP_Mode) == phNfc_eActive424))
        {
            pNciConfig->PollNfcFActive = 1;
        }
    }
    /*Enable passive mode */
    else
    {
        pNciConfig->EnableIso14443A  = ((pNciConfig->EnableIso14443A) ||\
                                       ((pADDSetup->NfcIP_Mode) & (uint8_t)phNfc_ePassive106));
        pNciConfig->EnableFelica = ((pNciConfig->EnableFelica) ||\
                                   ((pADDSetup->NfcIP_Mode) & (uint8_t)phNfc_ePassive212));
        pNciConfig->EnableFelica = ((pNciConfig->EnableFelica) ||\
                                   ((pADDSetup->NfcIP_Mode) & (uint8_t)phNfc_ePassive424));
    }

    /* Check if all baud rates are enabled (incase of P2P Initiator) */
    if ((phNfc_eP2P_ALL & pADDSetup->NfcIP_Mode ) == phNfc_eP2P_ALL)
    {
        pNciConfig->EnableIso14443A  = 1;
        pNciConfig->EnableFelica  = 1;
        pNciConfig->PollNfcAActive = 1;
        pNciConfig->PollNfcFActive = 1;
    }

    /* Configure for P2P target device */
    if(0 == pADDSetup->NfcIP_Tgt_Disable)
    {
        if(phNfc_eP2PTargetNfcATech == (phNfc_eP2PTargetNfcATech & pADDSetup->NfcIP_Tgt_Mode_Config))
        {
            pNciConfig->ListenNfcA = 1;
        }
        if(phNfc_eP2PTargetNfcFTech == (phNfc_eP2PTargetNfcFTech & pADDSetup->NfcIP_Tgt_Mode_Config))
        {
            pNciConfig->ListenNfcF = 1;
        }
        if(phNfc_eP2PTargetNfcActiveATech == (phNfc_eP2PTargetNfcActiveATech & pADDSetup->NfcIP_Tgt_Mode_Config))
        {
            pNciConfig->ListenNfcAActive = 1;
        }
        if(phNfc_eP2PTargetNfcActiveFTech == (phNfc_eP2PTargetNfcActiveFTech & pADDSetup->NfcIP_Tgt_Mode_Config))
        {
            pNciConfig->ListenNfcFActive = 1;
        }
        if(phNfc_eP2PTargetNfcInvalidTech == (phNfc_eP2PTargetEnableAllTech & pADDSetup->NfcIP_Tgt_Mode_Config))
        {
            pNciConfig->ListenNfcA = 1;
            pNciConfig->ListenNfcF = 1;
        }
    }

    /* Copy the listen mode configuration */
    if (0 == pADDSetup->PollDevInfo.PollCfgInfo.DisableCardEmulation)
    {
        if (!pLibContext->bDtaFlag)
        {
            if (phNfc_eCENfcATech == (phNfc_eCENfcATech & pADDSetup->CE_Mode_Config) ||
                phNfc_eCENfcInvalidTech == (phNfc_eCEEnableAllTech & pADDSetup->CE_Mode_Config))
            {
                pNciConfig->ListenNfcA = 1;
            }
            if ((phNfc_eCENfcBTech == (phNfc_eCENfcBTech & pADDSetup->CE_Mode_Config) ||
                phNfc_eCENfcInvalidTech == (phNfc_eCEEnableAllTech & pADDSetup->CE_Mode_Config)) &&
                (pLibContext->tSeInfo.bSeCount > 0) && (PH_LIBNFC_SE_LSTN_NFC_B_SUPP & pLibContext->tSeInfo.bNfceeSuppNfcTech) != 0)
            {
                pNciConfig->ListenNfcB = 1;
            }
            if (phNfc_eCENfcFTech == (phNfc_eCENfcFTech & pADDSetup->CE_Mode_Config) ||
                phNfc_eCENfcInvalidTech == (phNfc_eCEEnableAllTech & pADDSetup->CE_Mode_Config))
            {
                pNciConfig->ListenNfcF = 1;
            }
            // Disable listen A if UICC supports B only and P2P is enabled
            if ((0 == pADDSetup->NfcIP_Tgt_Disable) && (pLibContext->tSeInfo.bSeCount > 0) && (PH_LIBNFC_SE_LSTN_NFC_B_SUPP == pLibContext->tSeInfo.bNfceeSuppNfcTech))
            {
                pNciConfig->ListenNfcA = 0;
            }
        }
        else
        {
            pNciConfig->ListenNfcA = phNfc_eCENfcATech == (phNfc_eCENfcATech & pADDSetup->CE_Mode_Config) ? 1 : 0;
            pNciConfig->ListenNfcB = phNfc_eCENfcBTech == (phNfc_eCENfcBTech & pADDSetup->CE_Mode_Config) ? 1 : 0;
            pNciConfig->ListenNfcF = phNfc_eCENfcFTech == (phNfc_eCENfcFTech & pADDSetup->CE_Mode_Config) ? 1 : 0;
        }
    }

    /* Update Poll Frequency for each technology */
    if(pNciConfig->EnableIso14443A)
    {
        if((pADDSetup->aPollParms[0].PollFreq == 0x00) ||\
            (pADDSetup->aPollParms[0].PollFreq > 0x0A))
        {
            pNciConfig->bIso14443A_PollFreq = 0x01;
        }
        else
        {
            pNciConfig->bIso14443A_PollFreq = pADDSetup->aPollParms[0].PollFreq;
        }
    }
    if(pNciConfig->EnableIso14443B)
    {
        if((pADDSetup->aPollParms[1].PollFreq == 0x00) ||\
            (pADDSetup->aPollParms[1].PollFreq > 0x0A))
        {
            pNciConfig->bIso14443B_PollFreq = 0x01;
        }
        else
        {
            pNciConfig->bIso14443B_PollFreq = pADDSetup->aPollParms[1].PollFreq;
        }
    }
    if(pNciConfig->EnableFelica)
    {
        if((pADDSetup->aPollParms[2].PollFreq == 0x00) ||\
            (pADDSetup->aPollParms[2].PollFreq > 0x0A))
        {
            pNciConfig->bFelica_PollFreq = 0x01;
        }
        else
        {
            pNciConfig->bFelica_PollFreq = pADDSetup->aPollParms[2].PollFreq;
        }
    }
    if(pNciConfig->EnableIso15693)
    {
        if((pADDSetup->aPollParms[3].PollFreq == 0x00) ||\
            (pADDSetup->aPollParms[3].PollFreq > 0x0A))
        {
            pNciConfig->bIso15693_PollFreq = 0x01;
        }
        else
        {
            pNciConfig->bIso15693_PollFreq = pADDSetup->aPollParms[3].PollFreq;
        }
    }
    if(pNciConfig->EnableKovio)
    {
        pNciConfig->bKovio_PollFreq = 0x01;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

static NFCSTATUS phLibNfc_SendDeactIdleCmd(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    phNciNfc_DeActivateType_t eDeactType = phNciNfc_e_IdleMode;
    UNUSED(status);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    /*Derive NCI Deactivate type from  pLibContext->ReleaseType = ReleaseType;*/
    wStatus = phNciNfc_Deactivate((void *)pCtx->sHwReference.pNciHandle,
                                  eDeactType,
                                  (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                  (void *)pContext);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_ReDiscoveryComplete(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
        if(NFCSTATUS_SUCCESS != (PHNFCSTATUS(wStatus)))
        {
            wStatus = NFCSTATUS_FAILED;
        }

        phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus),&TrigEvent);
        (void)phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);

        wStatus = phLibNfc_InvokeDiscDisconnCb(pLibContext, wStatus, pInfo);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_DiscDeactivateComplete(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    pphLibNfc_RspCb_t pDiscNtfCb = NULL;
    pphLibNfc_DisconnectCallback_t pDisconnNtfCb = NULL;
    void  *pUpper_Context = NULL;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
        if(NULL != pLibContext->CBInfo.pClientDisConfigCb)
        {
            pDiscNtfCb = pLibContext->CBInfo.pClientDisConfigCb;
            pUpper_Context = pLibContext->CBInfo.pClientDisCfgCntx;
            pLibContext->CBInfo.pClientDisConfigCb = NULL;
            pLibContext->CBInfo.pClientDisCfgCntx = NULL;
        }
        if(NULL != pLibContext->CBInfo.pClientDisConnectCb)
        {
            pDisconnNtfCb = pLibContext->CBInfo.pClientDisConnectCb;
            pUpper_Context = pLibContext->CBInfo.pClientDConCntx;
            pLibContext->CBInfo.pClientDisConnectCb = NULL;
            pLibContext->CBInfo.pClientDConCntx = NULL;
        }

        if(NFCSTATUS_SUCCESS != (PHNFCSTATUS(wStatus)))
        {
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            pLibContext->Connected_handle=NULL;
            pLibContext->DummyConnect_handle = NULL;
        }

        phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus),&TrigEvent);
        (void)phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);

        if(NULL != pDiscNtfCb)
        {
            /* Discovery call back function has to be invoked */
            pDiscNtfCb(pUpper_Context,wStatus);
        }
        if(NULL != pDisconnNtfCb)
        {
            /* Disconnect call back needs to be invoked */
            pDisconnNtfCb(pUpper_Context,(phLibNfc_Handle)NULL,wStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SendSelectCmd(void *pContext,
                                 NFCSTATUS wStatus,
                                 void *pInfo
                                 )
{
    pphLibNfc_Context_t pCtx = pContext;
    pphNciNfc_RemoteDevInformation_t pRemoteDevNciHandle = NULL;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        pRemoteDevNciHandle = pCtx->Connected_handle;

        if(NULL != pRemoteDevNciHandle)
        {
            if(FALSE == (pCtx->tSelInf.bSelectInpAvail))
            {
                /*Call Nci connect to connect to target*/
                wStatus = phNciNfc_Connect(pCtx->sHwReference.pNciHandle,
                                           pRemoteDevNciHandle,
                                           pRemoteDevNciHandle->eRfIf,
                                           &phLibNfc_InternalSequence,
                                          (void *)pCtx);
            }
            else
            {
                /*Call Nci connect to connect to target*/
                wStatus = phNciNfc_Connect(pCtx->sHwReference.pNciHandle,
                                           pRemoteDevNciHandle,
                                           (pCtx->tSelInf.eRfIf),
                                           &phLibNfc_InternalSequence,
                                          (void *)pCtx);
            }
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SendSelectCmd1(void *pContext,
                                  NFCSTATUS wStatus,
                                  void *pInfo
                                 )
{
    pphLibNfc_Context_t pCtx = pContext;
    pphNciNfc_RemoteDevInformation_t pRemoteDevNciHandle = NULL;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        pRemoteDevNciHandle = pCtx->Connected_handle;
        if(NULL != pRemoteDevNciHandle)
        {
            /*Call Nci connect to connect to target*/
            wStatus = phNciNfc_Connect(pCtx->sHwReference.pNciHandle,
                                       pRemoteDevNciHandle,
                                       pRemoteDevNciHandle->eRfIf,
                                       &phLibNfc_InternalSequence,
                                      (void *)pCtx);
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_SelectCmdResp(void *pContext,
                                 NFCSTATUS wStatus,
                                 void *pInfo
                                 )
{
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle = (pphNciNfc_RemoteDevInformation_t)pInfo;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();
    UNUSED(pContext);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Discovery select command success");

        if(NULL != pNciRemoteDevHandle)
        {
            if(pLibContext->Connected_handle == pNciRemoteDevHandle)
            {
                PH_LOG_LIBNFC_INFO_STR("Valid remoteDev Handle!!");
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Invalid remoteDev Handle!!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Discovery select command failed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_P2pActivate(void *pContext,
                                  NFCSTATUS wStatus,
                                  void *pInfo
                                 )
{
    pphLibNfc_Context_t pCtx = pContext;
    pphNciNfc_RemoteDevInformation_t pRemoteDevNciHandle;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pCtx) && (NULL != pCtx->Connected_handle))
    {
        pRemoteDevNciHandle = pCtx->Connected_handle;

        /*Call Nci connect to connect to target*/
        wStatus = phNciNfc_Connect(pCtx->sHwReference.pNciHandle,
                                   pRemoteDevNciHandle,
                                   pRemoteDevNciHandle->eRfIf,
                                   &phLibNfc_InternalSequence,
                                  (void *)pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_P2pActivateResp(void *pContext,
                                 NFCSTATUS wStatus,
                                 void *pInfo)
{
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle = (pphNciNfc_RemoteDevInformation_t)pInfo;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    UNUSED(pContext);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("P2p activate: Discovery select response received");
        if(NULL != pNciRemoteDevHandle)
        {
            if(pLibContext->Connected_handle == pNciRemoteDevHandle)
            {
                PH_LOG_LIBNFC_INFO_STR("P2p activate: Valid remoteDev Handle!");
                /* Check for activated protocol and interface */
                if((phNciNfc_e_RfProtocolsNfcDepProtocol == pNciRemoteDevHandle->eRFProtocol) &&
                    (phNciNfc_e_RfInterfacesNFCDEP_RF == pNciRemoteDevHandle->eRfIf))
                {
                    PH_LOG_LIBNFC_INFO_STR("P2p activate: Interface and protocol are NFC-DEP");
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("P2p activate: Interface and protocol are not NFC-DEP");
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("P2p activate: Invalid remoteDev Handle!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("P2p activate: Discovery select command failed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}


NFCSTATUS phLibNfc_ReqInfoComplete(void *pContext, NFCSTATUS status, void *pInfo)
{
    NFCSTATUS wStatus = status;
    phLibNfc_NtfRegister_RspCb_t pClientCb = NULL;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventInvalid;

    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pLibContext)
    {
        pClientCb = pLibContext->CBInfo.pClientNtfRegRespCB;
        /* Copy the Trigger event which was present in Notify register callback */
        if(NFCSTATUS_SUCCESS != status)
        {
            TrigEvent = phLibNfc_EventInvalid;
            status = NFCSTATUS_FAILED;
        }
        else
        {
            TrigEvent = pLibContext->DiscTagTrigEvent;
        }

        (void)phLibNfc_ProcessDevInfo(pLibContext, TrigEvent, pLibContext->pInfo, status);
    }
    else
    {
        status = NFCSTATUS_FAILED;
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static uint8_t phLibNfc_CheckDiscParams(phLibNfc_sADD_Cfg_t *pDiscConfig)
{
    uint8_t bSkipSetConfig = 0;
    phNfc_sPollDevInfo_t *pPollInfo = NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pDiscConfig)
    {
        pPollInfo = &(pDiscConfig->PollDevInfo.PollCfgInfo);

        if(0 == pPollInfo->DisableCardEmulation)
        {
            if((1 == pPollInfo->EnableIso14443A) ||   \
               (1 == pPollInfo->EnableIso14443B) ||   \
               (1 == pPollInfo->EnableFelica212) ||   \
               (1 == pPollInfo->EnableFelica424) ||   \
               (1 == pPollInfo->EnableIso15693)  ||   \
               (1 == pPollInfo->EnableKovio))
            {
                PH_LOG_LIBNFC_INFO_STR("Polling for one of the tag's is enabled");
                bSkipSetConfig = 0;
            }
            else if(((phNfc_eDefaultP2PMode != pDiscConfig->NfcIP_Mode) && (phNfc_eInvalidP2PMode != pDiscConfig->NfcIP_Mode)) &&
                ((phNfc_ePassive106 == (phNfc_ePassive106 & pDiscConfig->NfcIP_Mode)) ||
                (phNfc_eP2P_ALL == pDiscConfig->NfcIP_Mode) ||
                (phNfc_ePassive212 == (phNfc_ePassive212 & pDiscConfig->NfcIP_Mode)) ||
                (phNfc_eActive106 == (phNfc_eActive106 & pDiscConfig->NfcIP_Mode)) ||
                (phNfc_eActive212 == (phNfc_eActive212 & pDiscConfig->NfcIP_Mode)) ||
                (phNfc_eActive424 == (phNfc_eActive424 & pDiscConfig->NfcIP_Mode)) ||
                (phNfc_ePassive424 == (phNfc_ePassive424 & pDiscConfig->NfcIP_Mode))))
            {
                PH_LOG_LIBNFC_INFO_STR("Peer to Peer Initiator is enabled");
                bSkipSetConfig = 0;
            }
            else if(0 == pDiscConfig->NfcIP_Tgt_Disable)
            {
                PH_LOG_LIBNFC_INFO_STR("Peer to Peer Target mode is enabled");
                bSkipSetConfig = 0;
            }
            else
            {
                /* Only Card emulation is enabled, we can skip set configuration */
                bSkipSetConfig = 1;
            }
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid i/p parameters");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return bSkipSetConfig;
}

NFCSTATUS phLibNfc_RestartDiscovery(void)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    /* Initiate discovery sequence */
    pLibContext->DiscDisconnMode = NFC_DISC_START_DISCOVERY;
    /* To skip Transceive exit function in libnfc statemachine */
    pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_INPROGRESS;
    wStatus = phLibNfc_StateHandler(pLibContext,
                            phLibNfc_EventDiscovery,
                            (void *)pLibContext->DiscDisconnMode,
                            (void *)&(pLibContext->tADDconfig),
                            NULL);

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_InvokeDiscDisconnCb(void* pContext,NFCSTATUS wStatus,void* pInfo)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    pphLibNfc_RspCb_t pDiscNtfCb = NULL;
    pphLibNfc_DisconnectCallback_t pDisconnNtfCb = NULL;
    void  *pUpper_Context = NULL;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext->CBInfo.pClientDisConfigCb)
    {
        pDiscNtfCb = pLibContext->CBInfo.pClientDisConfigCb;
        pUpper_Context = pLibContext->CBInfo.pClientDisCfgCntx;
        pLibContext->CBInfo.pClientDisConfigCb = NULL;
        pLibContext->CBInfo.pClientDisCfgCntx = NULL;
    }
    if(pLibContext->CBInfo.pClientDisConnectCb)
    {
        pDisconnNtfCb = pLibContext->CBInfo.pClientDisConnectCb;
        pUpper_Context = pLibContext->CBInfo.pClientDConCntx;
        pLibContext->CBInfo.pClientDisConnectCb = NULL;
        pLibContext->CBInfo.pClientDConCntx = NULL;
    }

    if(NULL != pDiscNtfCb)
    {
        /* Discovery call back function has to be invoked */
        pDiscNtfCb(pUpper_Context,wStatus);
    }
    if(NULL != pDisconnNtfCb)
    {
        /* Disconnect call back needs to be invoked */
        pDisconnNtfCb(pUpper_Context,(phLibNfc_Handle)NULL,wStatus);
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_DelayDisc(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    UNUSED(pInfo);
    wStatus = phLibNfc_DummyDisc(pCtx,&phLibNfc_DelayDiscTimerCb,PH_LIBNFC_DISCOVERY_DELAY);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_DelayDiscProc(void* pContext, NFCSTATUS wStatus, void* pInfo)
{
    NFCSTATUS wModeStatus = wStatus;
    UNUSED(pInfo) ;UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wModeStatus;
}

static void phLibNfc_DelayDiscTimerCb(uint32_t dwTimerId, void *pContext)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        (void)phOsalNfc_Timer_Stop(dwTimerId);
        (void)phOsalNfc_Timer_Delete(dwTimerId);

        (void)phLibNfc_SeqHandler(pLibContext,wStatus,NULL);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

static NFCSTATUS phLibNfc_SetPowerSubStateSeq(void *pContext, NFCSTATUS wStatus, void * pInfo)
{
    pphLibNfc_LibContext_t pLibContext = pContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        if((1 == pLibContext->Config.bSwitchedOnSubState) &&
           ((pLibContext->tSeInfo.bSeCount > 0) || (pLibContext->bHceSak > 0)))
        {
            pLibContext->TgtPwrSubState = phNciNfc_e_SwitchedOnState;

            if(0 == pLibContext->tADDconfig.PollDevInfo.PollCfgInfo.DisableCardEmulation &&
               1 == pLibContext->tADDconfig.NfcIP_Tgt_Disable)
            {
                pLibContext->TgtPwrSubState = phNciNfc_e_SwitchedOnSubState3;
            }

            if (pLibContext->TgtPwrSubState != pLibContext->PwrSubState)
            {
                wStatus = phNciNfc_SetPowerSubState((void *)pLibContext->sHwReference.pNciHandle,
                                                    pLibContext->TgtPwrSubState,
                                                    (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                                    (void *)pContext);
                if(NFCSTATUS_PENDING != wStatus)
                {
                    /* Skip the current sequence and proceed to the next sequence
                       since the failure of setting the power sub-state is optional */
                    wStatus = NFCSTATUS_SUCCESS;
                }
            }
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_SetPowerSubStateSeqEnd(void *pContext, NFCSTATUS wStatus, void * pInfo)
{
    pphLibNfc_LibContext_t pLibContext = pContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        if(wStatus == NFCSTATUS_SUCCESS)
        {
            pLibContext->PwrSubState = pLibContext->TgtPwrSubState;
        }
        else
        {
            /* Proceed to the next sequence even in the case of failure */
            wStatus = NFCSTATUS_SUCCESS;
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
