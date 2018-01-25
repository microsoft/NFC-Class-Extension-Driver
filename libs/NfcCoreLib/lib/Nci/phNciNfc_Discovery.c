/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_Discovery.tmh"

/** Maximum Discovery frequency Allowed */
#define PHNCINFC_MAXDISCFREQ                (0x0A)

/** Minimum length of Technology specific parameters for NFCA-Poll */
#define PHNCINFC_NFCAPOLL_MINLEN            (4U)
/** Minimum length of Technology specific parameters for NFCA-Poll */
#define PHNCINFC_NFCBPOLL_MINLEN            (12U)
/** Minimum length of Technology specific parameters for NFCA-Poll */
#define PHNCINFC_NFCFPOLL_MINLEN            (18U)
/** Minimum length of Technology specific parameters for NFCF-Listen */
#define PHNCINFC_NFCFLISTEN_MINLEN          (1U)

/** Length of NFCA-Poll SENS_RES */
#define PHNCINFC_NFCAPOLL_SENSRESLEN        (2U)

/** Length of NFCB-Poll SENSB_RES */
#define PHNCINFC_NFCBPOLL_SENSBRESLEN11    (11U)
/** Length of NFCB-Poll SENSB_RES */
#define PHNCINFC_NFCBPOLL_SENSBRESLEN12    (12U)
/** Length of NFCF-Poll SENSF_RES */
#define PHNCINFC_NFCFPOLL_SENSFRESLEN16    (16U)
/** Length of NFCF-Poll SENSF_RES */
#define PHNCINFC_NFCFPOLL_SENSFRESLEN18    (18U)

/** Value indicate More notifications to be followed */
#define PHNCINFC_RFDISC_MORENTF             (2U)

/** Value indicates Minimum Length of Discover Notification */
#define PHNCINFC_RFDISC_DISCNTFMINLEN       (5U)

/** Value indicates Minimum Length of Activation Notification */
#define PHNCINFC_RFDISC_DEACTVNTFLEN        (02U)

/** Discover Frequency for Listen Mode configurations */
#define PHNCINFC_LISTEN_DISCFREQ            (1U)

static NFCSTATUS phNciNfc_StoreRfTechParams(uint8_t bBuffLen,
                                            uint8_t *pBuff,
                                            pphNciNfc_RemoteDevInformation_t pRemDevInfo);

static NFCSTATUS phNciNfc_StoreDiscNtfParams(uint8_t *pBuff,
                                             uint8_t *pBuffIndex,
                                             pphNciNfc_RemoteDevInformation_t pRemoteDevInfo);

static NFCSTATUS phNciNfc_StoreRemDevInfo(void *pNciHandle,
                                          uint8_t *pBuff,
                                          uint16_t wLen,
                                          uint8_t *pIndex,
                                          pphNciNfc_RemoteDevInformation_t pRemoteDevInfo);

static NFCSTATUS phNciNfc_RetrieveRemDevInfo(pphNciNfc_DeviceInfo_t pDevInfo,
                                             uint8_t bRfDiscoverId,
                                             pphNciNfc_RemoteDevInformation_t *pRemoteDevInfo,
                                             uint8_t bSelTarget);

static NFCSTATUS phNciNfc_StoreNfcATechParams(uint8_t bBuffLen, uint8_t *pBuff, pphNciNfc_RemoteDevInformation_t pRemDevInfo);
static NFCSTATUS phNciNfc_StoreNfcFTechParams(uint8_t bBuffLen, uint8_t *pBuff, pphNciNfc_RemoteDevInformation_t pRemDevInfo);

static NFCSTATUS phNciNfc_ValidateSuppDeActvType(pphNciNfc_RemoteDevInformation_t pRemdevInfo,
                                                 phNciNfc_DeActivateType_t eDeActvType);

static void phNciNfc_DeactNtfTimeoutHandler(uint32_t TimerId, void *pContext);
static NFCSTATUS phNciNfc_DeActvNtfCb(void *pContext,void *pInfo,NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_SendDiscSelCmd(void *pContext);
static NFCSTATUS phNciNfc_ProcessDiscRsp(void *pContext,NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_CompleteDiscSelSequence(void *pContext, NFCSTATUS wStatus);

static void phNciNfc_SelectNtfTimeoutHandler(uint32_t TimerId, void *pContext);

static NFCSTATUS phNciNfc_EnableDiscovery(void *pNciHandle);
static NFCSTATUS phNciNfc_CompleteDiscSequence(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_DeActivateRemDev(void *pContext);
static NFCSTATUS phNciNfc_DeActivateRsp(void *pContext,NFCSTATUS wDeactvStatus);
static NFCSTATUS phNciNfc_CompleteDeactivate(void *pContext, NFCSTATUS wStatus);

static void phNciNfc_TargetDiscoveryComplete(void *pContext,NFCSTATUS wStatus);

/*Global Varibales for Discover Select and Release Handler*/
phNciNfc_SequenceP_t gphNciNfc_DiscSelSequence[] = {
    {&phNciNfc_SendDiscSelCmd, &phNciNfc_ProcessDiscRsp}, /*Send Discover Select command, Process response */
    {NULL, &phNciNfc_CompleteDiscSelSequence} /*Release resources used for Discover Select */
};

/*Global Varibales for Discover and Release Handler*/
phNciNfc_SequenceP_t gphNciNfc_DiscoverSequence[] = {
    {&phNciNfc_EnableDiscovery, &phNciNfc_ProcessDiscRsp}, /*Send Discovery command, Process response */
    {NULL, &phNciNfc_CompleteDiscSequence} /*Release resources used for Discovery */
};

/*Global Varibales for DeActivate and Release Handler*/
phNciNfc_SequenceP_t gphNciNfc_DeActivateSequence[] = {
    {&phNciNfc_DeActivateRemDev, &phNciNfc_DeActivateRsp}, /*Send DeActivate command, Process response */
    {NULL, &phNciNfc_CompleteDeactivate} /*Release resources used for DeActivate */
};

static NFCSTATUS
phNciNfc_ValidateSuppDeActvType(pphNciNfc_RemoteDevInformation_t pRemdevInfo,
                                          phNciNfc_DeActivateType_t eDeActvType)
{
    NFCSTATUS wValidateStatus;
    /* Deactivation to Idle/Discovery mode is supported for all interfaces */
    if( (phNciNfc_e_IdleMode == eDeActvType) || (phNciNfc_e_DiscMode == eDeActvType) )
    {
        wValidateStatus = NFCSTATUS_SUCCESS;
    }
    /* All Sleep/Sleep_AF modes are supported for Frame RF Interface */
    else if(phNciNfc_e_RfInterfacesFrame_RF == pRemdevInfo->eRfIf)
    {
        wValidateStatus = NFCSTATUS_SUCCESS;
    }
    /* For ISO-DEP Interface in Poll Mode, Only Sleep Mode is Supported */
    else if( ( (phNciNfc_e_RfInterfacesISODEP_RF == pRemdevInfo->eRfIf) ||\
               (phNciNfc_e_RfInterfacesTagCmd_RF == pRemdevInfo->eRfIf) ) && \
               (pRemdevInfo->eRFTechMode <= phNciNfc_NFCF_Poll) && \
               (phNciNfc_e_SleepMode == eDeActvType) )
    {
        wValidateStatus = NFCSTATUS_SUCCESS;
    }
    /* For NFC-DEP Interface in Poll Mode, Only Sleep_AF Mode is Supported */
    else if( (phNciNfc_e_RfInterfacesNFCDEP_RF == pRemdevInfo->eRfIf) && \
             (pRemdevInfo->eRFTechMode <= phNciNfc_NFCF_Poll) && \
             (phNciNfc_e_SleepAfMode == eDeActvType) )
    {
        wValidateStatus = NFCSTATUS_SUCCESS;
    }
    else if(phNciNfc_e_RfInterfacesNfceeDirect_RF == pRemdevInfo->eRfIf)
    {
        wValidateStatus = NFCSTATUS_SUCCESS;
    }
    else
    {
        wValidateStatus = NFCSTATUS_FAILED;
    }
    return wValidateStatus;
}

NFCSTATUS phNciNfc_ValidateDeActvType(PVOID pNciCtx,
                    pphNciNfc_DiscContext_t pDiscCtx,
                    phNciNfc_DeActivateType_t eSrcDeActvType,
                    phNciNfc_DeActivateType_t *pDestDeActvType)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphNciNfc_RemoteDevInformation_t pRemDevInfo;
    pphNciNfc_Context_t pCtx = pNciCtx;

    /* If Discovery is in progress, then stop discovery */
    /* Case 1 : Discovery is in progress
                Devices Discovered = 0 */
    if( (0 == pDiscCtx->tDevInfo.dwNumberOfDevices) )
    {
        /* Only Deactivate with IDLE mode is allowed */
        if(eSrcDeActvType == phNciNfc_e_IdleMode)
        {
            *pDestDeActvType = eSrcDeActvType;
            pCtx->tRegSyncInfo.pDeActvNtfCb = NULL;
            wStatus = NFCSTATUS_SUCCESS;
            PH_LOG_NCI_INFO_STR("Skip wait for Deactivate notification");
        }
        else
        {
            *pDestDeActvType = eSrcDeActvType;
            pCtx->tRegSyncInfo.pDeActvNtfCb = pCtx->IfNtf;
            pCtx->tRegSyncInfo.DeActvNtfCtxt = pCtx->IfNtfCtx;
            wStatus = NFCSTATUS_SUCCESS;
            PH_LOG_NCI_INFO_STR("Wait for Deactivate notification");
        }
    }
    /* Case 2 : Devices Discovered > 0
                Device is Selected/Not Selected */
    else if(pDiscCtx->tDevInfo.dwNumberOfDevices > 0)
    {
        /* Check whether any Remote Device is Activated */
        wStatus = phNciNfc_RetrieveRemDevInfo(&pDiscCtx->tDevInfo,0x00,\
                                    &pRemDevInfo,PHNCINFC_RETRIEVEACTIVE_TARGET);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Case 21 : A Device is Activated */
            wStatus = phNciNfc_ValidateSuppDeActvType(pRemDevInfo,eSrcDeActvType);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                *pDestDeActvType = eSrcDeActvType;
                pCtx->tRegSyncInfo.pDeActvNtfCb = pCtx->IfNtf;
                pCtx->tRegSyncInfo.DeActvNtfCtxt = pCtx->IfNtfCtx;
                wStatus = NFCSTATUS_SUCCESS;
                PH_LOG_NCI_INFO_STR("Wait for Deactivate notification");
            }
        }
        else
        {
            /* Case 22 : No Device is Activated */
            *pDestDeActvType = eSrcDeActvType;
            pCtx->tRegSyncInfo.pDeActvNtfCb = NULL;
            wStatus = NFCSTATUS_SUCCESS;
            PH_LOG_NCI_INFO_STR("Skip wait for Deactivate notification");
        }
    }
    return wStatus;
}

void phNciNfc_HandlePriorityDeactv(pphNciNfc_DiscContext_t pDiscCtx)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphNciNfc_RemoteDevInformation_t pRemDevInfo;
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo;
    uint8_t bUnRegister = 0;
    phNciNfc_RFDevType_t eRemDevType = phNciNfc_eInvalid_DevType;
    pphNciNfc_CoreContext_t pNciCoreCtx = phNciNfc_GetCoreContext();

    if((NULL != pDiscCtx) && (NULL != pNciCoreCtx))
    {
        if(0 == pDiscCtx->tDevInfo.dwNumberOfDevices)
        {
            if(NULL != pDiscCtx->tDevInfo.pRemDevList[0])
            {
                /* Check remote device type and unregister */
                bUnRegister = 1;
                eRemDevType = pDiscCtx->tDevInfo.pRemDevList[0]->RemDevType;
            }
        }
        else if(pDiscCtx->tDevInfo.dwNumberOfDevices > 0)
        {
            wStatus = phNciNfc_RetrieveRemDevInfo(&pDiscCtx->tDevInfo,0x00,\
                                        &pRemDevInfo,PHNCINFC_RETRIEVEACTIVE_TARGET);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                bUnRegister = 1;
                eRemDevType = pRemDevInfo->RemDevType;
            }
        }

        if((1 == bUnRegister) && (phNciNfc_eNfcIP1_Target == eRemDevType))
        {
            /* Default static RF conn id */
            tHeaderInfo.bConn_ID = 0x00;
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            /* Unregister response call back if any registered (for P2P Initiator Transceive */
            (void)phNciNfc_CoreIfUnRegRspNtf(pNciCoreCtx,
                             &tHeaderInfo,
                             (pphNciNfc_CoreIfNtf_t)&phNciNfc_RdrDataXchgSequence);
        }
    }
}

static NFCSTATUS phNciNfc_DeActivateRemDev(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        /* Build the Deactivate Command Header */
        TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        TxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
        TxInfo.tHeaderInfo.Opcode_ID.OidType.RfMgtCmdOid = phNciNfc_e_RfMgtRfDeactivateCmdOid;
        PH_LOG_NCI_INFO_STR("Building Header bytes for Deactivate Command");
        /* Copy the length and Buff Pointer containing the Payload of Discover Command */
        TxInfo.Buff = (uint8_t *)pNciContext->tSendPayload.pBuff;
        TxInfo.wLen = pNciContext->tSendPayload.wPayloadSize;

        wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext), &TxInfo,
            &(pNciContext->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,
            (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence, pContext);
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameter");
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_DeActivateRsp(void *pContext,NFCSTATUS wDeactvStatus)
{
    pphNciNfc_Context_t pNciContext = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if( (NULL != pNciContext) && (NFCSTATUS_SUCCESS == wDeactvStatus) )
    {
        /* Validate the Response of Deactivate Command */
        if( (NULL != pNciContext->RspBuffInfo.pBuff) &&
            (PH_NCINFC_STATUS_OK == pNciContext->RspBuffInfo.pBuff[0])&&
            (PHNCINFC_DEACTIVATE_RESP_LEN == pNciContext->RspBuffInfo.wLen) )
        {
            PH_LOG_NCI_INFO_STR("Deactivate process success");
            wDeactvStatus = NFCSTATUS_SUCCESS;
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Invalid parameters (phNciNfc_DeActivateRsp)");
            wDeactvStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wDeactvStatus;
}

static NFCSTATUS phNciNfc_CompleteDeactivate(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciContext = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        /* Release memory used for constructing payload */
        if(NULL != pNciContext->tSendPayload.pBuff)
        {
            phOsalNfc_FreeMemory(pNciContext->tSendPayload.pBuff);
            pNciContext->tSendPayload.pBuff = NULL;
            pNciContext->tSendPayload.wPayloadSize = 0;
        }
        if(NULL != pNciContext->tTranscvCtxt.tSendPld.pBuff)
        {
            pNciContext->tTranscvCtxt.tSendPld.wLen = 0;
            PH_LOG_NCI_INFO_STR(" Freeing Send Request Payload Buffer..");
            phOsalNfc_FreeMemory(pNciContext->tTranscvCtxt.tSendPld.pBuff);
            pNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
        }
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Invoke the Connect callback if any Notification is not expected */
            if(NULL == pNciContext->tRegSyncInfo.pDeActvNtfCb)
            {
                (void)phNciNfc_ProcessDeActvState(pNciContext);
                phNciNfc_Notify(pNciContext, wStatus, NULL);
            }
            else
            {
                if(PH_OSALNFC_TIMER_ID_INVALID != pNciContext->dwNtfTimerId)
                {
                    wStatus = phOsalNfc_Timer_Start(pNciContext->dwNtfTimerId,
                                                    PH_NCINFC_NTF_TIMEROUT,
                                                    &phNciNfc_DeactNtfTimeoutHandler,
                                                    (void *) pNciContext);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        PH_LOG_NCI_INFO_STR("Deactivate ntf timer started");
                    }
                    else
                    {
                        PH_LOG_NCI_CRIT_STR("Deactivate ntf timer start FAILED");
                    }
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Timer Create had failed");
                }
            }
        }
        else
        {
            phNciNfc_Notify(pNciContext, wStatus, NULL);
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void phNciNfc_DeactNtfTimeoutHandler(uint32_t TimerId, void *pContext)
{
    pphNciNfc_Context_t pNciContext = pContext;
    uint8_t aBuff[2] = {0};
    phNciNfc_TransactInfo_t tTransInfo;
    UNUSED(TimerId);
    PH_LOG_NCI_FUNC_ENTRY();
    PH_LOG_NCI_CRIT_STR("Entering Deactivate Ntf Timeout");

    if(NULL != pNciContext)
    {
        (void)phOsalNfc_Timer_Stop(pNciContext->dwNtfTimerId);
        /* Deactivate Notification simulated */
        aBuff[0] = (uint8_t)pNciContext->NciDiscContext.eDeActvType;
        aBuff[1] = (uint8_t)phNciNfc_e_DhRequest;
        tTransInfo.pbuffer = aBuff;
        tTransInfo.wLength = 0x02;
        tTransInfo.pContext = pContext;
        (void)phNciNfc_DeActvNtfCb(pContext,&tTransInfo,NFCSTATUS_SUCCESS);
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Nci context null (phNciNfc_DeactNtfTimeoutHandler)");
    }
    PH_LOG_NCI_FUNC_EXIT();
}

static void phNciNfc_SelectNtfTimeoutHandler(uint32_t TimerId, void *pContext)
{
    pphNciNfc_Context_t pNciContext = pContext;
    pphNciNfc_IfNotificationCb_t pConnectCb;
    UNUSED(TimerId);
    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL != pNciContext)
    {
        /* Stop timer */
        (void)phOsalNfc_Timer_Stop(pNciContext->dwNtfTimerId);
        /* Invoke Connect Callback if registered */
        if(NULL != pNciContext->tRegSyncInfo.pActvNtfCb)
        {
            pConnectCb = pNciContext->tRegSyncInfo.pActvNtfCb;
            pNciContext->tRegSyncInfo.pActvNtfCb = NULL;
            (void)pConnectCb(pNciContext->tRegSyncInfo.ActvNtfCtxt, NFCSTATUS_FAILED,NULL);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
}

static NFCSTATUS phNciNfc_ProcessActivatedIfDeActv(void *pContext,\
                            pphNciNfc_RemoteDevInformation_t pRemDevInfo)
{
    pphNciNfc_Context_t pNciContext = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_DiscContext_t pDiscCtx = &(pNciContext->NciDiscContext);
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        /* Case 31 : A Device is Activated */
        /*Clear only if deactivated to Disciovery*/
        //wStatus = phNciNfc_RdrMgmtRelease(pNciContext);
        phNciNfc_ListenMgmt_DeActivate(pNciContext,pRemDevInfo);

        /* If the deactivation state is to Idle mode
          Clear notification list */
        if(pDiscCtx->eDeActvType == phNciNfc_e_IdleMode)
        {
            wStatus = phNciNfc_RdrMgmtRelease(pNciContext);
            phNciNfc_ClearDiscContext(pNciContext);
        }
        else if ( (pDiscCtx->eDeActvType == phNciNfc_e_SleepMode) ||
                  (pDiscCtx->eDeActvType == phNciNfc_e_SleepAfMode) )
        {
            /* Clear the Activation flag to indicate Remote devices
               are in HOST_SELECT state*/
            pRemDevInfo->SessionOpened = 0;
        }
        /*If the Deactivation state is Discovery,
          Register for Discovery and Interface activated Notifications*/
        else if(pDiscCtx->eDeActvType == phNciNfc_e_DiscMode)
        {
            wStatus = phNciNfc_RdrMgmtRelease(pNciContext);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                phNciNfc_ClearDiscContext(pNciContext);
                /* Set Discover state to indicate start of Discovery process */
                pDiscCtx->bDiscState = PHNCINFC_RFDISCSTATE_SET;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }
    return wStatus;
}

NFCSTATUS phNciNfc_ProcessDeActvState(void *pContext)
{
    pphNciNfc_Context_t pNciContext = pContext;
    pphNciNfc_RemoteDevInformation_t pRemDevInfo;
    phNciNfc_ADD_Cfg_t tConfig;
    NFCSTATUS wDeActvStatus = NFCSTATUS_FAILED;
    pphNciNfc_DiscContext_t pDiscCtx = &(pNciContext->NciDiscContext);
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        /* Save the Discovery configuration to be present in Discovery
           context */
        phOsalNfc_MemCopy(&tConfig,&(pDiscCtx->tConfig),\
                            sizeof(phNciNfc_ADD_Cfg_t));
        /* Case 1 : Discovery is in progress
                Devices Discovered = 0 */
        if( (PHNCINFC_RFDISCSTATE_SET == pDiscCtx->bDiscState) && \
            (0 == pDiscCtx->tDevInfo.dwNumberOfDevices) )
        {
            (void)phNciNfc_RdrMgmtRelease(pNciContext);
            phNciNfc_ClearDiscContext(pNciContext);
            wDeActvStatus = NFCSTATUS_SUCCESS;
        }
        /* Case 2 : Nfcee device is Activated
                Devices Discovered = 0 */
        if( (PHNCINFC_RFDISCSTATE_RESET == pDiscCtx->bDiscState) && \
            (0 == pDiscCtx->tDevInfo.dwNumberOfDevices) )
        {
            wDeActvStatus = phNciNfc_DeActivateNfcee(&(pNciContext->tNfceeContext));
        }
        /* Case 3 : Devices Discovered > 0
                    Device is Selected/Not Selected */
        if(pDiscCtx->tDevInfo.dwNumberOfDevices > 0)
        {
            /* Check whether any Remote Device is Activated */
            wDeActvStatus = phNciNfc_RetrieveRemDevInfo(&pDiscCtx->tDevInfo,0x00,\
                                        &pRemDevInfo,PHNCINFC_RETRIEVEACTIVE_TARGET);
            if(NFCSTATUS_SUCCESS == wDeActvStatus)
            {
                wDeActvStatus = \
                    phNciNfc_ProcessActivatedIfDeActv(pNciContext,pRemDevInfo);
            }
            else
            {
                /* Transition to Idle mode ONLY is allowed */
                if(pDiscCtx->eDeActvType == phNciNfc_e_IdleMode)
                {
                    (void)phNciNfc_RdrMgmtRelease(pNciContext);
                    phNciNfc_ClearDiscContext(pNciContext);
                }
            }
        }
        /* Copy back the Discovery Configuration */
        phOsalNfc_MemCopy(&(pDiscCtx->tConfig),&tConfig,\
                    sizeof(phNciNfc_ADD_Cfg_t));
    }
    return wDeActvStatus;
}

static NFCSTATUS phNciNfc_EnableDiscovery(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        /* Build the Discover Command Header */
        TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        TxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
        TxInfo.tHeaderInfo.Opcode_ID.OidType.RfMgtCmdOid = phNciNfc_e_RfMgtRfDiscoverCmdOid;
        PH_LOG_NCI_INFO_STR("Building Header bytes for Discover Command");

        /* Copy the length and Buff Pointer containing the Payload of Discover Command */
        TxInfo.Buff = (uint8_t *)pNciContext->NciDiscContext.pDiscPayload;
        TxInfo.wLen = pNciContext->NciDiscContext.bDiscPayloadLen;
        /* Send Discover Command */
        wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext), &TxInfo,
            &(pNciContext->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,
            (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence, pContext);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessDiscRsp(void *pContext,NFCSTATUS wDiscStatus)
{
    pphNciNfc_Context_t pNciContext = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if( (NULL != pNciContext) && (NFCSTATUS_SUCCESS == wDiscStatus) )
    {
        /* Validate the Response of Discover Command */
        if( (NULL != pNciContext->RspBuffInfo.pBuff) &&
            (PH_NCINFC_STATUS_OK == pNciContext->RspBuffInfo.pBuff[0])&&
            (PHNCINFC_DISC_RESP_LEN == pNciContext->RspBuffInfo.wLen) )
        {
            PH_LOG_NCI_INFO_STR("Discovery process Started");
            wDiscStatus = NFCSTATUS_SUCCESS;
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Invalid parameters (phNciNfc_ProcessDiscRsp)");
            wDiscStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wDiscStatus;
}

static NFCSTATUS phNciNfc_CompleteDiscSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        if(NULL != pNciCtx->RspBuffInfo.pBuff)
        {
            /* Release memory used for constructing payload */
            if(NULL != pNciCtx->NciDiscContext.pDiscPayload)
            {
                phOsalNfc_FreeMemory(pNciCtx->NciDiscContext.pDiscPayload);
                pNciCtx->NciDiscContext.pDiscPayload = NULL;
            }
            /* Register notification only if Discover command with Poll configu */
            if( (pNciCtx->NciDiscContext.bDiscPayloadLen > 1) &&
                (NFCSTATUS_SUCCESS == wStatus) )
            {
                /* SET the state of Discovery process. This is to indicate
                   Discovery process has started */
                pNciCtx->NciDiscContext.bDiscState = PHNCINFC_RFDISCSTATE_SET;
            }
        }
        phNciNfc_Notify(pNciCtx, wStatus,NULL);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_SendDiscSelCmd(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        /* Build the Discover Select Command Header */
        TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        TxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
        TxInfo.tHeaderInfo.Opcode_ID.OidType.RfMgtCmdOid = phNciNfc_e_RfMgtRfDiscSelectCmdOid;
        /* Copy the length and Buff Pointer containing the Payload of Discover Select Command */
        TxInfo.Buff = (uint8_t *)pNciContext->NciDiscContext.pDiscPayload;
        TxInfo.wLen = pNciContext->NciDiscContext.bDiscPayloadLen;
        wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext), &TxInfo,
            &(pNciContext->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,
            &phNciNfc_GenericSequence, pContext);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CompleteDiscSelSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciContext = pContext;
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        if(NULL != pNciContext->RspBuffInfo.pBuff)
        {
            /* Release memory used for constructing payload */
            if(NULL != pNciContext->NciDiscContext.pDiscPayload)
            {
                phOsalNfc_FreeMemory(pNciContext->NciDiscContext.pDiscPayload);
                pNciContext->NciDiscContext.pDiscPayload = NULL;
                pNciContext->NciDiscContext.bDiscPayloadLen = 0;
            }
            /* Invoke the Connect callback if any error */
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                phNciNfc_Notify(pNciContext, wStatus,NULL);
            }
            else
            {
                /* Register for Synchronous Activated notification */
                pNciContext->tRegSyncInfo.pActvNtfCb = pNciContext->IfNtf;
                pNciContext->tRegSyncInfo.ActvNtfCtxt = pNciContext->IfNtfCtx;

                tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
                tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
                tHeaderInfo.Opcode_ID.OidType.NciCoreNtfOid = phNciNfc_e_NciCoreGenericErrNtfOid;

                (void)phNciNfc_CoreIfRegRspNtf(&(pNciContext->NciCoreContext),
                                                    &(tHeaderInfo),
                                                    &phNciNfc_ProcessGenericErrNtfMFC,
                                                    pNciContext
                                                   );

                if(PH_OSALNFC_TIMER_ID_INVALID != pNciContext->dwNtfTimerId)
                {
                    wStatus = phOsalNfc_Timer_Start(pNciContext->dwNtfTimerId,
                                                    PH_NCINFC_NTF_TIMEROUT,
                                                    &phNciNfc_SelectNtfTimeoutHandler,
                                                    (void *) pNciContext);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        PH_LOG_NCI_INFO_STR("Select ntf timer started");
                    }
                }
            }
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_StoreDiscNtfParams(uint8_t *pBuff,uint8_t *pBuffIndex,
                                               pphNciNfc_RemoteDevInformation_t pRemoteDevInfo)
{
    NFCSTATUS wStoreStatus = NFCSTATUS_FAILED;
    uint8_t bIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    /* Check whether a valid Discover ID is passed */
    if( (0 != pBuff[bIndex]) &&
        (PHNCINFC_INVALID_DISCID != pBuff[bIndex]) )
    {
        pRemoteDevInfo->bRfDiscId = pBuff[bIndex++];
        /* Check whether the Rf protocol is valid */
        if( (!phNciNfc_ValidateRfProtocol((phNciNfc_RfProtocols_t)pBuff[bIndex])) &&
            (!phNciNfc_ValidateRfTechMode((phNciNfc_RfTechMode_t)pBuff[bIndex + 1])) )
        {
            pRemoteDevInfo->eRFProtocol =
                (phNciNfc_RfProtocols_t)pBuff[bIndex++];
            pRemoteDevInfo->eRFTechMode =
                    (phNciNfc_RfTechMode_t)pBuff[bIndex++];
            /* Index Points to length of Tech params */
            wStoreStatus =
                phNciNfc_StoreRfTechParams(pBuff[bIndex],&pBuff[bIndex+1],
                                        pRemoteDevInfo);
            /* Add index to point to Notification byte */
            bIndex += pBuff[bIndex] + 1;
            *pBuffIndex = bIndex;
        }
    }
    return wStoreStatus;
}

static NFCSTATUS phNciNfc_StoreNfcATechParams(uint8_t bBuffLen, uint8_t *pBuff,
                                               pphNciNfc_RemoteDevInformation_t pRemDevInfo)
{
    NFCSTATUS wStoreStatus = NFCSTATUS_FAILED;
    uint8_t bIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    /* Check whether NFCID1 length is in the desired range */
    if( (PHNCINFC_RFCONFIG_NFCID1_NOTPRESENT == pBuff[bIndex + 2]) ||
        (PHNCINFC_RFCONFIG_NFCID1_4BYTES == pBuff[bIndex + 2]) ||
        (PHNCINFC_RFCONFIG_NFCID1_7BYTES == pBuff[bIndex + 2]) ||
        (PHNCINFC_RFCONFIG_NFCID1_10BYTES == pBuff[bIndex + 2]) )
    {
        /* Store tech mode params in respective structures */
        switch(pRemDevInfo->RemDevType)
        {
            case phNciNfc_eJewel_PICC:
            {
                pRemDevInfo->tRemoteDevInfo.Jewel_Info.bSensResResp[0] = pBuff[bIndex++];
                pRemDevInfo->tRemoteDevInfo.Jewel_Info.bSensResResp[1] = pBuff[bIndex++];
                /* Copy NFCID1 length and value */
                pRemDevInfo->tRemoteDevInfo.Jewel_Info.UidLength = pBuff[bIndex++];
                /* NFCID1 is not present for type 1 tag */
                if(PHNCINFC_RFCONFIG_NFCID1_NOTPRESENT ==
                        pRemDevInfo->tRemoteDevInfo.Jewel_Info.UidLength)
                {
                    /* Sel-Resp is not present for Type-1 Tag */
                    if(0 == pBuff[bIndex])
                    {
                        bIndex++;
                        wStoreStatus = NFCSTATUS_SUCCESS;
                    }
                }
            }
            break;
            case phNciNfc_eNfcIP1_Target:
            {
                pRemDevInfo->tRemoteDevInfo.NfcIP_Info.SensRes[0] = pBuff[bIndex++];
                pRemDevInfo->tRemoteDevInfo.NfcIP_Info.SensRes[1] = pBuff[bIndex++];
                pRemDevInfo->tRemoteDevInfo.NfcIP_Info.SensResLength = PHNCINFC_NFCAPOLL_SENSRESLEN;
                /* Copy NFCID1 length and value */
                pRemDevInfo->tRemoteDevInfo.NfcIP_Info.NFCID_Length = pBuff[bIndex++];
                /* If NFCID1 is not present, set UID bytes to zero */
                if(PHNCINFC_RFCONFIG_NFCID1_NOTPRESENT !=
                        pRemDevInfo->tRemoteDevInfo.NfcIP_Info.NFCID_Length)
                {
                    phOsalNfc_MemCopy(pRemDevInfo->tRemoteDevInfo.NfcIP_Info.NFCID,
                        &pBuff[bIndex],
                        pRemDevInfo->tRemoteDevInfo.NfcIP_Info.NFCID_Length);
                    /* Point the index to Sel-Resp length */
                    bIndex += pRemDevInfo->tRemoteDevInfo.NfcIP_Info.NFCID_Length;
                    if(1 == pBuff[bIndex])
                    {
                        /* Update Sel-Resp length */
                        pRemDevInfo->tRemoteDevInfo.NfcIP_Info.SelResLen = pBuff[bIndex++];
                        pRemDevInfo->tRemoteDevInfo.NfcIP_Info.SelRes = pBuff[bIndex++];
                        wStoreStatus = NFCSTATUS_SUCCESS;
                    }
                }
            }
            break;
            default:
            {
                pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.bSensResResp[0] = pBuff[bIndex++];
                pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.bSensResResp[1] = pBuff[bIndex++];
                /* Index Points to NFCID1 parameter length */
                if(PHNCINFC_RFCONFIG_NFCID1_NOTPRESENT != pBuff[bIndex])
                {
                    /* Copy NFCID1 length and value */
                    pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.UidLength = pBuff[bIndex++];
                    phOsalNfc_MemCopy(pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.Uid,
                        &pBuff[bIndex],
                        pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.UidLength);
                    /* Point the index to Sel-Resp length */
                    bIndex += pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.UidLength;
                    if(1 == pBuff[bIndex])
                    {
                        /* Update Sel-Resp length */
                        pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.bSelResRespLen = pBuff[bIndex++];
                        /* Update Sel-Resp value*/
                        pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.Sak = pBuff[bIndex++];
                        if (phNciNfc_IsVersion1x(phNciNfc_GetContext()))
                        {
                             wStoreStatus = NFCSTATUS_SUCCESS;
                        }
                        else
                        {
                            uint8_t HRxLength = pBuff[bIndex++];

                            // Per NCI v2.0, Section 7.1, Table 68.
                            // HRx Length must be either 0x00 or 0x02.
                            if (HRxLength == 0 || HRxLength == 2)
                            {
                                /* Retrieve HR field from Activiated Notification as per NCI 2.0 spec */
                                pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.bHRxLen = HRxLength;
                                if (HRxLength != 0)
                                {
                                    phOsalNfc_MemCopy(pRemDevInfo->tRemoteDevInfo.Iso14443A_Info.bHRx,
                                        &pBuff[bIndex],
                                        HRxLength);
                                    bIndex += HRxLength;
                                }
                                wStoreStatus = NFCSTATUS_SUCCESS;
                            }
                            else
                            {
                                PH_LOG_NCI_CRIT_STR("Invalid HRx Length value:%d", HRxLength);
                            }
                        }
                    }
                }
            }
            break;
        }
        if(NFCSTATUS_SUCCESS == wStoreStatus)
        {
            if(bIndex != bBuffLen)
            {
                wStoreStatus = NFCSTATUS_FAILED;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStoreStatus;
}

static NFCSTATUS phNciNfc_StoreNfcFTechParams(uint8_t bBuffLen, uint8_t *pBuff,
                                               pphNciNfc_RemoteDevInformation_t pRemDevInfo)
{
    NFCSTATUS wStoreStatus = NFCSTATUS_FAILED;
    uint8_t bIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    /* Validate the Bitrate field */
    if( ((1 == pBuff[bIndex]) || (2 == pBuff[bIndex])) &&\
        ( (PHNCINFC_NFCFPOLL_SENSFRESLEN16 == pBuff[bIndex + 1]) ||
            (PHNCINFC_NFCFPOLL_SENSFRESLEN18 == pBuff[bIndex + 1]) ) )
    {
        if(phNciNfc_eNfcIP1_Target == pRemDevInfo->RemDevType)
        {
            if(1 == pBuff[bIndex++])
            {
                pRemDevInfo->tRemoteDevInfo.NfcIP_Info.Nfcip_Datarate = phNciNfc_e_BitRate212;
            }
            else
            {
                pRemDevInfo->tRemoteDevInfo.NfcIP_Info.Nfcip_Datarate = phNciNfc_e_BitRate424;
            }
            /* Copy SENSF_RES length and value */
            pRemDevInfo->tRemoteDevInfo.NfcIP_Info.SensResLength = pBuff[bIndex++];
            phOsalNfc_MemCopy(pRemDevInfo->tRemoteDevInfo.NfcIP_Info.SensRes,
                &pBuff[bIndex],
                pRemDevInfo->tRemoteDevInfo.NfcIP_Info.SensResLength);
            /* Point the index to Sel-Resp length */
            bIndex += pRemDevInfo->tRemoteDevInfo.NfcIP_Info.SensResLength;
            if(bIndex == bBuffLen)
            {
                wStoreStatus = NFCSTATUS_SUCCESS;
            }
        }
        else
        {
            pRemDevInfo->tRemoteDevInfo.Felica_Info.bBitRate = pBuff[bIndex++];
            /* Copy SENSF_RES length and value */
            pRemDevInfo->tRemoteDevInfo.Felica_Info.bSensFRespLen = pBuff[bIndex++];
            phOsalNfc_MemCopy(pRemDevInfo->tRemoteDevInfo.Felica_Info.aSensFResp,
                &pBuff[bIndex],
                pRemDevInfo->tRemoteDevInfo.Felica_Info.bSensFRespLen);
            bIndex += pRemDevInfo->tRemoteDevInfo.Felica_Info.bSensFRespLen;
            if(bIndex == bBuffLen)
            {
                wStoreStatus = NFCSTATUS_SUCCESS;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStoreStatus;
}

static NFCSTATUS phNciNfc_StoreRfTechParams(uint8_t bBuffLen, uint8_t *pBuff,
                                               pphNciNfc_RemoteDevInformation_t pRemDevInfo)
{
    NFCSTATUS wStoreStatus = NFCSTATUS_FAILED;
    uint8_t bIndex = 0;
    uint8_t bCount = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    /* Determine the Length of sak byte in case of NFC-A technology
    Traversal to Fetch Sak byte is as follows
    (Length of Sens Res) + (Length byte of NFCID1) + Length of NFCID1 */
    if(phNciNfc_NFCA_Poll == pRemDevInfo->eRFTechMode)
    {
        bCount = PHNCINFC_NFCAPOLL_SENSRESLEN +\
                 1 + pBuff[PHNCINFC_NFCAPOLL_SENSRESLEN];
    }
    /* Update Remote device type depending on sak byte */
    phNciNfc_GetRfDevType(pBuff[bCount + 1],
                          pBuff[bCount],
                          pRemDevInfo,
                          &pRemDevInfo->RemDevType);
    switch(pRemDevInfo->eRFTechMode)
    {
        case phNciNfc_NFCA_Poll:
        case phNciNfc_NFCA_Kovio_Poll:
        case phNciNfc_NFCA_Active_Poll:
        if(bBuffLen >= PHNCINFC_NFCAPOLL_MINLEN)
        {
            wStoreStatus = phNciNfc_StoreNfcATechParams(bBuffLen,pBuff,pRemDevInfo);
        }
        break;
        case phNciNfc_NFCB_Poll:
        if(bBuffLen >= PHNCINFC_NFCBPOLL_MINLEN)
        {
            if( (PHNCINFC_NFCBPOLL_SENSBRESLEN11 == pBuff[bIndex]) ||
                (PHNCINFC_NFCBPOLL_SENSBRESLEN12 == pBuff[bIndex]) )
            {
                /* Copy SENSB_RES length and value */
                pRemDevInfo->tRemoteDevInfo.Iso14443B_Info.bSensBRespLen = pBuff[bIndex++];
                phOsalNfc_MemCopy(pRemDevInfo->tRemoteDevInfo.Iso14443B_Info.aSensBResp,
                    &pBuff[bIndex],
                    pRemDevInfo->tRemoteDevInfo.Iso14443B_Info.bSensBRespLen);
                bIndex += pRemDevInfo->tRemoteDevInfo.Iso14443B_Info.bSensBRespLen;
                if(bIndex == bBuffLen)
                {
                    wStoreStatus = NFCSTATUS_SUCCESS;
                }
            }
        }
        break;
        case phNciNfc_NFCF_Poll:
        case phNciNfc_NFCF_Active_Poll:
        if(bBuffLen >= PHNCINFC_NFCFPOLL_MINLEN)
        {
            wStoreStatus = phNciNfc_StoreNfcFTechParams(bBuffLen,pBuff,pRemDevInfo);
        }
        break;
    default:
        break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStoreStatus;
}

static NFCSTATUS phNciNfc_RetrieveRemDevInfo(pphNciNfc_DeviceInfo_t pDevInfo,
                                             uint8_t bRfDiscoverId,
                                             pphNciNfc_RemoteDevInformation_t *pRemoteDevInfo,
                                             uint8_t bSelTarget)
{
    NFCSTATUS wRetrieveStatus = NFCSTATUS_FAILED;
    uint8_t bIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    if(pDevInfo->dwNumberOfDevices > 0)
    {
        /* Update the Remote device Info list depending on flag */
        for(bIndex = 0; ( (bIndex < PH_NCINFC_MAX_REMOTE_DEVICES) && \
                          (NFCSTATUS_SUCCESS != wRetrieveStatus) ); bIndex++)
        {
            /* Check if the Remote device info is available */
            if(NULL != pDevInfo->pRemDevList[bIndex])
            {
                /* Check whether request is to retrieve only the selected Target */
                if(TRUE == bSelTarget)
                {
                    if(1 == pDevInfo->pRemDevList[bIndex]->SessionOpened)
                    {
                        *pRemoteDevInfo = pDevInfo->pRemDevList[bIndex];
                        wRetrieveStatus = NFCSTATUS_SUCCESS;
                    }
                }
                /* Compare Discovery ID with List of Remotedevice info */
                else
                {
                    /* Check whether RF Disc ID is already present in remotedevinfo */
                    if(bRfDiscoverId == pDevInfo->pRemDevList[bIndex]->bRfDiscId)
                    {
                        *pRemoteDevInfo = pDevInfo->pRemDevList[bIndex];
                        wRetrieveStatus = NFCSTATUS_SUCCESS;
                    }
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wRetrieveStatus;
}

NFCSTATUS phNciNfc_UpdateNtfList(pphNciNfc_DeviceInfo_t pDevInfo,
                                 pphNciNfc_RemoteDevInformation_t pRemoteDevInfo,
                                 uint8_t bUpdateList)
{
    NFCSTATUS wUpdateStatus = NFCSTATUS_FAILED;
    uint8_t bIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    if( (TRUE != bUpdateList) ||
        (pDevInfo->dwNumberOfDevices < PH_NCINFC_MAX_REMOTE_DEVICES) )
    {
        /* Update the Remote device Info list depending on flag */
        for(bIndex = 0; ( (bIndex < PH_NCINFC_MAX_REMOTE_DEVICES) && \
                          (NFCSTATUS_SUCCESS != wUpdateStatus) ); bIndex++)
        {
            /* Add Remote Device info to the list */
            if(TRUE == bUpdateList)
            {
                if(NULL == pDevInfo->pRemDevList[bIndex])
                {
                    /* Update list with Remote device information */
                    pDevInfo->dwNumberOfDevices++;
                    pDevInfo->pRemDevList[bIndex] = pRemoteDevInfo;
                    wUpdateStatus = NFCSTATUS_SUCCESS;
                }
            }
            /* Delete Remote Device info from the list */
            else
            {
                if( (NULL != pDevInfo->pRemDevList[bIndex]) &&
                    (pDevInfo->pRemDevList[bIndex] == pRemoteDevInfo) )
                {
                    /* Remove Device Info from the list */
                    pDevInfo->dwNumberOfDevices--;
                    pDevInfo->pRemDevList[bIndex] = NULL;
                    wUpdateStatus = NFCSTATUS_SUCCESS;
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wUpdateStatus;
}

void phNciNfc_ClearDiscContext(void *pContext)
{
    uint8_t bCount = 0;
    pphNciNfc_Context_t     pNciContext = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    /* Free memory used by remotedevice information */
    for(bCount = 0; bCount < PH_NCINFC_MAX_REMOTE_DEVICES; bCount++)
    {
        if(NULL != pNciContext->NciDiscContext.tDevInfo.pRemDevList[bCount])
        {
            phOsalNfc_FreeMemory(pNciContext->NciDiscContext.tDevInfo.pRemDevList[bCount]);
        }
    }
    /* Free payload memory if not done */
    if(NULL != pNciContext->NciDiscContext.pDiscPayload)
    {
        phOsalNfc_FreeMemory(pNciContext->NciDiscContext.pDiscPayload);
    }
    /* Clear All the context variables */
    phOsalNfc_SetMemory(&pNciContext->NciDiscContext,
                    0x00,sizeof(phNciNfc_DiscContext_t));
    PH_LOG_NCI_FUNC_EXIT();
}

NFCSTATUS phNciNfc_UpdateDiscConfigParams(void *pNciHandle,
                                    phNciNfc_ADD_Cfg_t *pPollConfig)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t     pNciContext = pNciHandle;
    /* Index to construct discover command payload */
    uint8_t bIndex=1;
    bool_t fIsNci1x = phNciNfc_IsVersion1x(pNciContext);
    bool_t fIsNci2x = phNciNfc_IsVersion2x(pNciContext);
    PH_LOG_NCI_FUNC_ENTRY();

    /*Note: ListenNfcFActive and PollNfcFActive exist only in NCI1.x specification.*/

    if(pPollConfig->PollNfcAActive ||
        (pPollConfig->PollNfcFActive && fIsNci2x))
    {
        pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
        (uint8_t)phNciNfc_NFCA_Active_Poll;
        pNciContext->NciDiscContext.pDiscPayload[bIndex++] = 1;
    }

    /* Check whether Polling loop to be enabled for NFC-A Technology */
    if(pPollConfig->EnableIso14443A)
    {
        /* Validate the poll frequency */
        if( (pPollConfig->bIso14443A_PollFreq != 0) &&
            (pPollConfig->bIso14443A_PollFreq <= PHNCINFC_MAXDISCFREQ) )
        {
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
            (uint8_t)phNciNfc_NFCA_Poll;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
            (uint8_t)pNciContext->NciDiscContext.tConfig.bIso14443A_PollFreq;
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    /* Check whether Polling loop to be enabled for NFC-A Technology */
    if( (pPollConfig->EnableIso14443B) && (NFCSTATUS_SUCCESS == wStatus) )
    {

        /* Validate the poll frequency */
        if( (pPollConfig->bIso14443B_PollFreq != 0) &&
            (pPollConfig->bIso14443B_PollFreq <= PHNCINFC_MAXDISCFREQ) )
        {
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
            (uint8_t)phNciNfc_NFCB_Poll;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
            (uint8_t)pNciContext->NciDiscContext.tConfig.bIso14443B_PollFreq;
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    /* Check whether Polling loop to be enabled for NFC-F Technology */

    if(pPollConfig->PollNfcFActive && fIsNci1x)
    {
        pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
        (uint8_t)phNciNfc_NFCF_Active_Poll;
        pNciContext->NciDiscContext.pDiscPayload[bIndex++] = 1;
    }
    if( (pPollConfig->EnableFelica) && (NFCSTATUS_SUCCESS == wStatus) )
    {

        /* Validate the poll frequency */
        if( (pPollConfig->bFelica_PollFreq != 0) &&
            (pPollConfig->bFelica_PollFreq <= PHNCINFC_MAXDISCFREQ) )
        {
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
            (uint8_t)phNciNfc_NFCF_Poll;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)pNciContext->NciDiscContext.tConfig.bFelica_PollFreq;
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    /* Check whether Polling loop to be enabled for NFC-I Technology */
    if((pPollConfig->EnableIso15693) && (NFCSTATUS_SUCCESS == wStatus))
    {

        /* Validate the poll frequency */
        if( (pPollConfig->bIso15693_PollFreq != 0) &&
            (pPollConfig->bIso15693_PollFreq <= PHNCINFC_MAXDISCFREQ) )
        {
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
            (uint8_t)phNciNfc_NFCISO15693_Poll;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
            (uint8_t)pNciContext->NciDiscContext.tConfig.bIso15693_PollFreq;
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }

    /* Check whether Polling loop to be enabled for Kovio Technology */
    if ((pPollConfig->EnableKovio) && (NFCSTATUS_SUCCESS == wStatus))
    {
        /* Validate the poll frequency */
        if ((pPollConfig->bKovio_PollFreq != 0) &&
            (pPollConfig->bKovio_PollFreq <= PHNCINFC_MAXDISCFREQ))
        {
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)phNciNfc_NFCA_Kovio_Poll;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)pNciContext->NciDiscContext.tConfig.bKovio_PollFreq;
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }

    if(NFCSTATUS_SUCCESS == wStatus)
    {
        if(1 == pPollConfig->ListenNfcA)
        {
            /* Configure for Listen mode in NFC-A  technology */
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)phNciNfc_NFCA_Listen;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)PHNCINFC_LISTEN_DISCFREQ;
        }

        if(1 == pPollConfig->ListenNfcAActive ||
            (1 == pPollConfig->ListenNfcFActive && fIsNci2x))
        {
            /* Configure for Listen mode in active technology */
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)phNciNfc_NFCA_Active_Listen;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)PHNCINFC_LISTEN_DISCFREQ;
        }

        if(1 == pPollConfig->ListenNfcB)
        {
            /* Configure for Listen mode in NFC-B technology */
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)phNciNfc_NFCB_Listen;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)PHNCINFC_LISTEN_DISCFREQ;
        }
        if(1 == pPollConfig->ListenNfcF)
        {
            /* Configure for Listen mode in NFC-F  technology */
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)phNciNfc_NFCF_Listen;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)PHNCINFC_LISTEN_DISCFREQ;
        }
        if(1 == pPollConfig->ListenNfcFActive && fIsNci1x)
        {
            /* Configure for Listen mode in active technology */
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)phNciNfc_NFCF_Active_Listen;
            pNciContext->NciDiscContext.pDiscPayload[bIndex++] =
                (uint8_t)PHNCINFC_LISTEN_DISCFREQ;
        }
    }

    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_VerifyRemDevInfo(void *pNciHandle,
                    pphNciNfc_RemoteDevInformation_t pRemoteDevInfo)
{
    NFCSTATUS wStatus;
    pphNciNfc_Context_t pNciContext = pNciHandle;
    pphNciNfc_RemoteDevInformation_t  pRetrieveDevInfo = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    wStatus = phNciNfc_RetrieveRemDevInfo(&pNciContext->NciDiscContext.tDevInfo,\
                                    pRemoteDevInfo->bRfDiscId,&pRetrieveDevInfo,
                                    PHNCINFC_RETRIEVE_TARGET);
    if( (NFCSTATUS_SUCCESS == wStatus) && \
        (pRetrieveDevInfo == pRemoteDevInfo) )
    {
        if(0 == pRemoteDevInfo->SessionOpened)
        {
            /* Check whether any other target is already connected */
            wStatus = phNciNfc_RetrieveRemDevInfo(&pNciContext->NciDiscContext.tDevInfo,\
                                    pRemoteDevInfo->bRfDiscId,&pRetrieveDevInfo,\
                                    PHNCINFC_RETRIEVEACTIVE_TARGET);
            if(NFCSTATUS_FAILED == wStatus)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Another Device is already connected");
                wStatus = NFCSTATUS_ANOTHER_DEVICE_CONNECTED;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("The Device is already connected");
            wStatus = NFCSTATUS_ALREADY_CONNECTED;
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid RemoteDevice handle passed");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_ProcessDiscNtf(void *pContext,void *pInfo, NFCSTATUS wStatus)
{
    NFCSTATUS wDiscStatus = wStatus;
    pphNciNfc_Context_t     pNciContext = pContext;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    pphNciNfc_RemoteDevInformation_t pRemoteDevInfo;
    uint8_t *pBuff;
    uint16_t wLen;
    uint8_t bIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS != wDiscStatus)
    {
        /*Invalid Parameter is passed */
        PH_LOG_NCI_CRIT_STR("Received Erroneous Notification");
    }
    else if( (NULL == pNciContext) || (NULL == pTransInfo) ||\
        (NULL == pTransInfo->pbuffer) ||(0 == pTransInfo->wLength) )
    {
        /*Invalid Parameter is passed */
        PH_LOG_NCI_CRIT_STR("Invalid input parameter ");
        wDiscStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if( ((PHNCINFC_RFDISCSTATE_RESET == \
            pNciContext->NciDiscContext.bDiscState) ) || \
            (pTransInfo->wLength < PHNCINFC_RFDISC_DISCNTFMINLEN) )
    {
        /**Invalid Parameter*/
        PH_LOG_NCI_CRIT_STR("Failed,Unexpected NtfOR Wrong parameter");
        wDiscStatus = NFCSTATUS_FAILED;
    }
    else
    {
        wDiscStatus = NFCSTATUS_SUCCESS;
        /* Assign the Ntf buffer pointer and length */
        pBuff = pTransInfo->pbuffer;
        wLen = pTransInfo->wLength;

        pRemoteDevInfo = (pphNciNfc_RemoteDevInformation_t)
                phOsalNfc_GetMemory(sizeof(phNciNfc_RemoteDevInformation_t));
        if(NULL != pRemoteDevInfo)
        {
            /* Reset structure members to store details of Remotedevice */
            phOsalNfc_SetMemory(pRemoteDevInfo,0x00,\
                            sizeof(phNciNfc_RemoteDevInformation_t));
            wDiscStatus = phNciNfc_StoreRemDevInfo(pNciContext,pBuff,wLen,&bIndex,pRemoteDevInfo);
            /* Index points to Notification byte */
            if(NFCSTATUS_SUCCESS == wDiscStatus)
            {
                /* Check whether the last notification is encountered */
                if(pBuff[bIndex] < PHNCINFC_RFDISC_MORENTF)
                {
                    if(pNciContext->NciDiscContext.tDevInfo.dwNumberOfDevices > 0)
                    {
                        PH_LOG_NCI_INFO_STR("Last Notification received");
                        /* RESET state of Discovery process to indicate
                        Completion of Discovery */
                        pNciContext->NciDiscContext.bDiscState = PHNCINFC_RFDISCSTATE_RESET;

                        /* Update status as Multiple targets found */
                        wDiscStatus = NFCSTATUS_MULTIPLE_TAGS;
                        /* Update pInfo pointer and invoke the upper layer */
                        phNciNfc_TargetDiscoveryComplete(pNciContext, wDiscStatus);
                    }
                    else
                    {
                        /* Wrong chain of discover notifications */
                        PH_LOG_NCI_CRIT_STR("Chain of Discover notifications failed");
                        wDiscStatus = NFCSTATUS_FAILED;
                    }
                }
            }
            else
            {
                phOsalNfc_FreeMemory(pRemoteDevInfo);
                pRemoteDevInfo = NULL;
                wDiscStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            /* Memory Allocation to store remote device info failed */
            PH_LOG_NCI_CRIT_STR("Memory allocation for Remotedevice information failed");
            wDiscStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* Invoke upper layer if Error Encountered during processing
       of Discover Notification */
    if( (NFCSTATUS_SUCCESS != wDiscStatus) &&\
        (NFCSTATUS_MULTIPLE_TAGS != wDiscStatus) )
    {
        /* Check to avoid dereferencing in case of NULL */
        if(NULL != pNciContext)
        {
            /* Update pInfo pointer and invoke the upper layer */
            phNciNfc_TargetDiscoveryComplete(pNciContext, wDiscStatus);
            PH_LOG_NCI_CRIT_STR("Failed,phNciNfc_ProcessDiscNtf()");
            /* Clear the context variables in case of error */
            (void)phNciNfc_RdrMgmtRelease(pNciContext);
            phNciNfc_ClearDiscContext(pNciContext);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wDiscStatus;
}

static NFCSTATUS phNciNfc_DeActvNtfCb(void *pContext,void *pInfo,NFCSTATUS wStatus)
{
    NFCSTATUS wDeActvStatus = wStatus;
    pphNciNfc_Context_t     pNciContext = pContext;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    pphNciNfc_IfNotificationCb_t pDeActvCb;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        (void)phOsalNfc_Timer_Stop(pNciContext->dwNtfTimerId);

        if(NFCSTATUS_SUCCESS != wDeActvStatus)
        {
            PH_LOG_NCI_CRIT_STR("Received Erroneous Notification");
        }
        else if((NULL == pTransInfo) || (NULL == pTransInfo->pbuffer) ||\
                (0 == pTransInfo->wLength) )
        {
            PH_LOG_NCI_CRIT_STR("Invalid input parameter");
            wDeActvStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        else if(pTransInfo->wLength != PHNCINFC_RFDISC_DEACTVNTFLEN)
        {
            PH_LOG_NCI_CRIT_STR("Failed, Wrong ntf length");
            wDeActvStatus = NFCSTATUS_FAILED;
        }
        else
        {
            /* Initialize status to failure */
            wDeActvStatus = NFCSTATUS_FAILED;
            /* Check whether notification was preceeded by Command and Response */
            if(NULL != pNciContext->tRegSyncInfo.pDeActvNtfCb)
            {
                if(pTransInfo->pbuffer[1] == (uint8_t)phNciNfc_e_DhRequest)
                {
                    if(pTransInfo->pbuffer[0] != (uint8_t)\
                            pNciContext->NciDiscContext.eDeActvType)
                    {
                        /* Request to Sleep mode, but Deactivation to IDLE mode
                           is allowed */
                        if( ((pNciContext->NciDiscContext.eDeActvType == phNciNfc_e_SleepMode) ||
                            (pNciContext->NciDiscContext.eDeActvType == phNciNfc_e_SleepAfMode)) &&\
                            (pTransInfo->pbuffer[0] == phNciNfc_e_IdleMode) )
                        {
                            wDeActvStatus = NFCSTATUS_SUCCESS;
                        }
                        else if( ((pNciContext->NciDiscContext.eDeActvType == phNciNfc_e_SleepMode) &&\
                                  (pTransInfo->pbuffer[0] == phNciNfc_e_SleepAfMode)) ||\
                                 ((pNciContext->NciDiscContext.eDeActvType == phNciNfc_e_SleepAfMode) &&\
                                  (pTransInfo->pbuffer[0] == phNciNfc_e_SleepMode)) )
                        {
                            wDeActvStatus = NFCSTATUS_SUCCESS;
                        }
                    }
                    else
                    {
                        wDeActvStatus = NFCSTATUS_SUCCESS;
                    }
                }
                else if (pTransInfo->pbuffer[1] == (uint8_t)phNciNfc_e_DhRequestFailed)
                {
                    if ((pNciContext->NciDiscContext.eDeActvType == phNciNfc_e_SleepMode) && \
                        (pTransInfo->pbuffer[0] == phNciNfc_e_SleepMode))
                    {
                        wDeActvStatus = NFCSTATUS_SUCCESS;
                    }
                    if ((pNciContext->NciDiscContext.eDeActvType == phNciNfc_e_SleepAfMode) && \
                        (pTransInfo->pbuffer[0] == phNciNfc_e_SleepAfMode))
                    {
                        wDeActvStatus = NFCSTATUS_SUCCESS;
                    }
                    else if ((pNciContext->NciDiscContext.eDeActvType == phNciNfc_e_IdleMode) && \
                             (pTransInfo->pbuffer[0] == phNciNfc_e_IdleMode))
                    {
                        wDeActvStatus = NFCSTATUS_SUCCESS;
                    }
                }
            }
        }
        /* Invoke upper layer if the Deactivate Notification timeout has occured */
        if(NFCSTATUS_SUCCESS != wDeActvStatus)
        {
            wDeActvStatus = NFCSTATUS_FAILED;
        }
        /* Cleanup related information only if the Deactivation type and reason
           are valid*/
        (void)phNciNfc_ProcessDeActvState(pNciContext);
        if(NULL != pNciContext->tRegSyncInfo.pDeActvNtfCb)
        {
            pDeActvCb = pNciContext->tRegSyncInfo.pDeActvNtfCb;
            pNciContext->tRegSyncInfo.pDeActvNtfCb = NULL;
            pDeActvCb(pNciContext->tRegSyncInfo.DeActvNtfCtxt,\
                        wDeActvStatus,NULL);
        }
    }
    else
    {
        /*Invalid Parameter is passed */
        PH_LOG_NCI_CRIT_STR("Received Erroneous Notification");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wDeActvStatus;
}

NFCSTATUS phNciNfc_ProcessActvNtf(void *pContext,void *pInfo,NFCSTATUS wStatus)
{
    NFCSTATUS wActvStatus = wStatus;
    pphNciNfc_Context_t     pNciContext = pContext;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    pphNciNfc_RemoteDevInformation_t pRemDevInfo = NULL;
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo;
    uint8_t *pBuff;
    phNciNfc_RfTechMode_t eTechMode;
    pphNciNfc_IfNotificationCb_t        pConnectCb;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NFCSTATUS_SUCCESS != wActvStatus)
    {
        /*Invalid Parameter is passed */
        PH_LOG_NCI_CRIT_STR("Received Erroneous Notification");
    }
    else if( (NULL == pNciContext) || (NULL == pTransInfo) ||\
        (NULL == pTransInfo->pbuffer) ||(0 == pTransInfo->wLength) )
    {
        /*Invalid Parameter is passed */
        PH_LOG_NCI_CRIT_STR("Invalid input parameter ");
        wActvStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(pTransInfo->wLength < PHNCINFC_RFDISC_ACTVNTFMINLEN)
    {
        /**Invalid Parameter*/
        PH_LOG_NCI_CRIT_STR("Failed,Wrong ntf parameter");
        wActvStatus = NFCSTATUS_FAILED;
    }
    else
    {
        /* Stop Notification timer */
        (void)phOsalNfc_Timer_Stop(pNciContext->dwNtfTimerId);

        tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
        tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
        tHeaderInfo.Opcode_ID.OidType.NciCoreNtfOid = phNciNfc_e_NciCoreGenericErrNtfOid;

        (void)phNciNfc_CoreIfUnRegRspNtf(&(pNciContext->NciCoreContext),
                                         &(tHeaderInfo),
                                         &phNciNfc_ProcessGenericErrNtfMFC);

        /* Assign the Ntf buffer pointer*/
        pBuff = pTransInfo->pbuffer;
        /* RESET state of Discovery process to indicate
           Completion of Discovery */
        pNciContext->NciDiscContext.bDiscState = PHNCINFC_RFDISCSTATE_RESET;

        /* Validate received Intf Actvd Ntf */
        wActvStatus = phNciNfc_ValidateIntfActvdNtf(pTransInfo->pbuffer,pTransInfo->wLength);
        if(NFCSTATUS_SUCCESS == wActvStatus)
        {
            /*check if Interface Type == 0*/
            if(pBuff[1] == phNciNfc_e_RfInterfacesNfceeDirect_RF)/*Secure elememt is activated by external Reader*/
            {
                wActvStatus = phNciNfc_NfceeProcessRfActvtdNtf(pNciContext,\
                                        pTransInfo->pbuffer,pTransInfo->wLength);
                if(NFCSTATUS_SUCCESS == wActvStatus)
                {
                    wActvStatus = NFCSTATUS_SECURE_ELEMENT_ACTIVATED;
                }
            }
            else
            {
                wActvStatus = phNciNfc_RetrieveRemDevInfo(&pNciContext->NciDiscContext.tDevInfo,\
                                                            pBuff[0],\
                                                            &pRemDevInfo,\
                                                            PHNCINFC_RETRIEVEACTIVE_TARGET);
                if(NFCSTATUS_FAILED == wActvStatus)
                {
                    if(0 == pNciContext->NciDiscContext.tDevInfo.dwNumberOfDevices)
                    {
                        /* Remotedevice entry not present */
                        pRemDevInfo = (pphNciNfc_RemoteDevInformation_t)
                            phOsalNfc_GetMemory(sizeof(phNciNfc_RemoteDevInformation_t));
                        if(NULL != pRemDevInfo)
                        {
                            /* Reset structure members to store details of Remotedevice */
                            phOsalNfc_SetMemory(pRemDevInfo,0x00,\
                                        sizeof(phNciNfc_RemoteDevInformation_t));
                            wActvStatus = NFCSTATUS_SUCCESS;
                        }
                    }
                    else
                    {
                        wActvStatus = phNciNfc_RetrieveRemDevInfo(&pNciContext->NciDiscContext.tDevInfo,\
                                                            pBuff[0],\
                                                            &pRemDevInfo,\
                                                            PHNCINFC_RETRIEVE_TARGET);
                    }
                }
                if(NFCSTATUS_SUCCESS == wActvStatus)
                {
                    if(0 == pNciContext->NciDiscContext.tDevInfo.dwNumberOfDevices)
                    {
                        /* Update to list of Remotedevice information if this is the only notification */
                        wActvStatus = phNciNfc_UpdateNtfList(&pNciContext->NciDiscContext.tDevInfo,
                                                pRemDevInfo,TRUE);
                    }
                    if(NFCSTATUS_SUCCESS == wActvStatus)
                    {
                        eTechMode = (phNciNfc_RfTechMode_t)pBuff[3];
                        /* Based on Active Rf techn and mode, switch to Poll management or listen management */
                        switch(eTechMode)
                        {
                        case phNciNfc_NFCA_Poll:
                        case phNciNfc_NFCA_Active_Poll:
                        case phNciNfc_NFCB_Poll:
                        case phNciNfc_NFCF_Poll:
                        case phNciNfc_NFCF_Active_Poll:
                        case phNciNfc_NFCISO15693_Poll:
                        case phNciNfc_NFCA_Kovio_Poll:
                            wActvStatus = phNciNfc_PollMgmt(pNciContext,pRemDevInfo,pTransInfo->pbuffer,
                                                        pTransInfo->wLength);
                            break;
                        case phNciNfc_NFCA_Listen:
                        case phNciNfc_NFCA_Active_Listen:
                        case phNciNfc_NFCB_Listen:
                        case phNciNfc_NFCF_Listen:
                        case phNciNfc_NFCF_Active_Listen:
                            wActvStatus = phNciNfc_ListenMgmt(pNciContext,pRemDevInfo,pTransInfo->pbuffer,
                                                        pTransInfo->wLength);
                            break;
                        default:
                            PH_LOG_NCI_CRIT_STR("Invalid Rf Techn and mode received");
                            break;
                        }

                        if( (NFCSTATUS_SUCCESS == wActvStatus) &&\
                            (NULL != pRemDevInfo) )
                        {
                            if(phNciNfc_eFelica_PICC == pRemDevInfo->RemDevType)
                            {
                                /*Check if activated */
                                if(0 != (pRemDevInfo->tRemoteDevInfo.Felica_Info.bSensFRespLen))
                                {
                                    pRemDevInfo->SessionOpened = 1;
                                }
                            }
                            else
                            {
                                pRemDevInfo->SessionOpened = 1;
                            }
                        }
                        else
                        {
                            if(1 == pNciContext->NciDiscContext.tDevInfo.dwNumberOfDevices)
                            {
                                /* Remove from the List of Remote dev if this is the only tag found */
                                (void)phNciNfc_UpdateNtfList(&pNciContext->NciDiscContext.tDevInfo,
                                                    pRemDevInfo,FALSE);
                            }
                            phOsalNfc_FreeMemory(pRemDevInfo);
                            pRemDevInfo = NULL;
                        }
                    }
                    else
                    {
                        if(0 == pNciContext->NciDiscContext.tDevInfo.dwNumberOfDevices)
                        {
                            phOsalNfc_FreeMemory(pRemDevInfo);
                            pRemDevInfo = NULL;
                        }
                    }
                }
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Interface activated notification validation failed!");
            wActvStatus = NFCSTATUS_FAILED;
        }

        if(NFCSTATUS_SECURE_ELEMENT_ACTIVATED != wActvStatus)
        {
            /* If NFCC is in listen mode, register a function to receive
            data from remote device.
            This is to ensure that data sent from remote device is not lost if application
            delays in calling 'phLibNfc_RemDev_Receive' API */
            if((NFCSTATUS_SUCCESS == wActvStatus) &&\
                (NULL != pRemDevInfo))
            {
                /* Check the type of Active Rf Techn and mode
                If we are in listen is active, register a funciton to receive data messages
                if sent before phLibNfc_RemDev_Receive is invoked
                by the upper layer (application) */
                switch(pRemDevInfo->eRFTechMode)
                {
                    case phNciNfc_NFCA_Listen:
                    case phNciNfc_NFCA_Active_Listen:
                    case phNciNfc_NFCB_Listen:
                    case phNciNfc_NFCF_Listen:
                    case phNciNfc_NFCF_Active_Listen:
                        /* If we are listeninig to remote device, make sure a Read request is opened
                        in order to not lose the data sent from remote device */
                        wActvStatus = phNciNfc_DummyReadReq(pNciContext);
                        if(NFCSTATUS_FAILED == wActvStatus)
                        {
                            PH_LOG_NCI_CRIT_STR("Dummy Read request for data message failed!");
                            phOsalNfc_FreeMemory(pRemDevInfo);
                            pRemDevInfo = NULL;
                        }
                        break;
                    default:
                        /* Nothing to do */
                        break;
                }
            }
            /* Invoke Connect Callback if registered */
            if(NULL != pNciContext->tRegSyncInfo.pActvNtfCb)
            {
                pConnectCb = pNciContext->tRegSyncInfo.pActvNtfCb;
                pNciContext->tRegSyncInfo.pActvNtfCb = NULL;
                pConnectCb(pNciContext->tRegSyncInfo.ActvNtfCtxt, wActvStatus,pRemDevInfo);
                wActvStatus = NFCSTATUS_SINGLE_TAG_ACTIVATED;
            }
            /* Invoke Register Listener callback if one Target is Discovered */
            else
            {
                /* Update the status only if the activated notification is processed
                   successfully */
                if(NFCSTATUS_SUCCESS == wActvStatus)
                {
                    /* Update the status according to State of the target */
                    if((NULL != pRemDevInfo) && (1 == pRemDevInfo->SessionOpened))
                    {
                        wActvStatus = NFCSTATUS_SINGLE_TAG_ACTIVATED;
                    }
                    else
                    {
                        wActvStatus = NFCSTATUS_SINGLE_TAG_DISCOVERED;
                    }
                }
                phNciNfc_TargetDiscoveryComplete(pNciContext, wActvStatus);
            }
        }
    }
    if( (NFCSTATUS_SECURE_ELEMENT_ACTIVATED != wActvStatus) &&
        (NFCSTATUS_SINGLE_TAG_DISCOVERED != wActvStatus) &&\
        (NFCSTATUS_SINGLE_TAG_ACTIVATED != wActvStatus) )
    {
        PH_LOG_NCI_CRIT_STR("Failed");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wActvStatus;
}

NFCSTATUS phNciNfc_ProcessDeActvNtf(void* pContext, void *pInfo, NFCSTATUS status)
{
    pphNciNfc_Context_t pNciCtx = phNciNfc_GetContext();
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    phNciNfc_NotificationInfo_t tRfDeactvInfo = {0};
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    NFCSTATUS wRetStatus = wStatus;
    NFCSTATUS wRetVal = NFCSTATUS_FAILED;
    pphNciNfc_ActiveDeviceInfo_t  pActivDev = NULL;
    phNciNfc_CoreRegInfo_t tRegInfo;
    uint8_t bConnId = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL != pNciCtx) && (NULL != pTransInfo))
    {
        /* If any data message is registered in ReceiveManager, unregister it */
        pActivDev = (pphNciNfc_ActiveDeviceInfo_t)&pNciCtx->tActvDevIf;

        wRetVal = phNciNfc_GetConnId(pActivDev->pDevInfo,&bConnId);
        if(NFCSTATUS_SUCCESS == wRetVal)
        {
            tRegInfo.bEnabled = (uint8_t)PHNCINFC_ENABLE_AUTO_DEREG;
            tRegInfo.bConnId = bConnId;
            tRegInfo.pContext = pNciCtx;
            tRegInfo.pNotifyCb = (pphNciNfc_CoreIfNtf_t) &phNciNfc_ReceiveCb;

            /* UnRegister for Data message (with auto de-register enabled) */
            wRetVal = phNciNfc_CoreRecvMgrDeRegisterCb((void *)&pNciCtx->NciCoreContext, &tRegInfo,
                            phNciNfc_e_NciCoreMsgTypeData);
        }

        if(1 == pNciCtx->tLstnModeRecvInfo.bDataBuffEnable)
        {
            pNciCtx->tLstnModeRecvInfo.bDataBuffEnable = 0;
            pNciCtx->IfNtf = NULL;
            pNciCtx->IfNtfCtx = NULL;
            if(NULL != pNciCtx->tLstnModeRecvInfo.pBuff)
            {
                phOsalNfc_FreeMemory(pNciCtx->tLstnModeRecvInfo.pBuff);
                pNciCtx->tLstnModeRecvInfo.pBuff = NULL;
            }
            pNciCtx->tLstnModeRecvInfo.wBuffSize = 0;
        }

        /* If dummy data receive call back function is still registered, unregister the same */
        if(NFCSTATUS_SUCCESS == wRetVal)
        {
            tRegInfo.bConnId = bConnId;
            tRegInfo.pContext = pNciCtx;
            tRegInfo.pNotifyCb = (pphNciNfc_CoreIfNtf_t) &phNciNfc_ReceiveDataBufferCb;
            /* UnRegister for Data message (with auto de-register enabled) */
            wRetVal = phNciNfc_CoreRecvMgrDeRegisterCb((void *)&pNciCtx->NciCoreContext, &tRegInfo,
                        phNciNfc_e_NciCoreMsgTypeData);
        }

        if((NULL == pTransInfo->pbuffer) ||(2 != pTransInfo->wLength)
            || (PH_NCINFC_STATUS_OK != status))
        {
            wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_WARN_STR(" Invalid Notification!!");
        }
        else
        {
            PH_LOG_NCI_INFO_STR("RF DeActivate notification Received!!");
            /* Update the deactivation reason */
            switch(pTransInfo->pbuffer[1])
            {
            case phNciNfc_e_DhRequest:
                tRfDeactvInfo.tRfDeactvInfo.eRfDeactvReason = phNciNfc_e_DhRequest;
                break;
            case phNciNfc_e_EndPoint:
                tRfDeactvInfo.tRfDeactvInfo.eRfDeactvReason = phNciNfc_e_EndPoint;
                break;
            case phNciNfc_e_RfLinkLoss:
                tRfDeactvInfo.tRfDeactvInfo.eRfDeactvReason = phNciNfc_e_RfLinkLoss;
                break;
            case phNciNfc_e_NfcB_BadAfi:
                tRfDeactvInfo.tRfDeactvInfo.eRfDeactvReason = phNciNfc_e_NfcB_BadAfi;
                break;
            case phNciNfc_e_DhRequestFailed:
                tRfDeactvInfo.tRfDeactvInfo.eRfDeactvReason = phNciNfc_e_DhRequestFailed;
            default:
                break;
            }

            /* Check if RF_DEACTIVATE_NTF is initiated by Remote device or
               because of Rf Link loss (Async Deactivate Ntf) */
            if((phNciNfc_e_EndPoint == pTransInfo->pbuffer[1]) || /* Endpoint_Request */
                (phNciNfc_e_RfLinkLoss == pTransInfo->pbuffer[1]))  /* RF_Link_Loss */
            {
                if(phNciNfc_e_DiscMode == pTransInfo->pbuffer[0])
                {
                    PH_LOG_NCI_CRIT_STR("Rf LinkLoss in DiscMode..");
                    wStatus = NFCSTATUS_SUCCESS;
                    wRetStatus = NFCSTATUS_TARGET_LOST;
                    tRfDeactvInfo.tRfDeactvInfo.eRfDeactvType = phNciNfc_e_DiscMode;
                }
                else if(phNciNfc_e_IdleMode == pTransInfo->pbuffer[0])
                {
                    PH_LOG_NCI_CRIT_STR("Rf LinkLoss in IdleMode..");
                    wStatus = NFCSTATUS_SUCCESS;
                    wRetStatus = NFCSTATUS_TARGET_LOST;
                    tRfDeactvInfo.tRfDeactvInfo.eRfDeactvType = phNciNfc_e_IdleMode;
                }
                else if((phNciNfc_e_SleepMode == pTransInfo->pbuffer[0]) ||
                    (phNciNfc_e_SleepAfMode == pTransInfo->pbuffer[0]))
                {
                    PH_LOG_NCI_CRIT_STR("Endpoint Req to 'Sleep/SleepAF'...Converting to 'Discovery'");
                    wStatus = NFCSTATUS_SUCCESS;
                    wRetStatus = NFCSTATUS_TARGET_LOST;
                    tRfDeactvInfo.tRfDeactvInfo.eRfDeactvType = phNciNfc_e_DiscMode;
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED; //Invalid deactivation type
                }

                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    wStatus = phNciNfc_ProcessDeActvState(pNciCtx);

                    /* If Nfcc has requested Deactivate to Discovery, set this flag
                       so that De-activate command is allowed from LibNfc */
                    if(0x03 == pTransInfo->pbuffer[0])
                    {
                        /* Nfcc is in discover state */
                        pNciCtx->NciDiscContext.bDiscState = PHNCINFC_RFDISCSTATE_SET;
                    }

                    if(NULL != pNciCtx->tDeActvInfo.pDeActvNotif)
                    {
                        pNciCtx->tDeActvInfo.pDeActvNotif(pNciCtx->tDeActvInfo.DeActvCtxt,eNciNfc_NciRfDeActvNtf,\
                            (void *)&tRfDeactvInfo, wRetStatus);
                    }
                }
            }
            else
            {
                wStatus = phNciNfc_DeActvNtfCb(pContext,pInfo,status);
            }

        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void phNciNfc_TargetDiscoveryComplete(void *pContext, NFCSTATUS wStatus)
{
    NFCSTATUS wCompleteStatus = wStatus;
    pphNciNfc_Context_t pNciContext = pContext;
    phNciNfc_NotificationInfo_t tInfo = {0};
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        if(NULL != pNciContext->tRegListInfo.pDiscoveryNotification)
        {
            PH_LOG_NCI_INFO_STR("Discovery notification registered");
            tInfo.pDiscoveryInfo = &pNciContext->NciDiscContext.tDevInfo;
            pNciContext->tRegListInfo.pDiscoveryNotification(pNciContext->tRegListInfo.DiscoveryCtxt,\
                                                            eNciNfc_DiscoverNtf,\
                                                            &tInfo,wCompleteStatus);
        }
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Context not available");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return;
}

static NFCSTATUS phNciNfc_StoreRemDevInfo(void *pNciHandle,uint8_t *pBuff,uint16_t wLen,
                                          uint8_t *pIndex,pphNciNfc_RemoteDevInformation_t pRemoteDevInfo)
{
    NFCSTATUS wStoreStatus = NFCSTATUS_FAILED;
    pphNciNfc_Context_t pNciContext = pNciHandle;
    uint8_t bIndex = *pIndex;
    wStoreStatus = phNciNfc_StoreDiscNtfParams(pBuff,&bIndex,
                                                   pRemoteDevInfo);
    /* Index points to Notification byte */
    if( (NFCSTATUS_SUCCESS == wStoreStatus) &&\
        (pBuff[bIndex] <= PHNCINFC_RFDISC_MORENTF) )
    {
        /* Store the notification received */
        wStoreStatus = phNciNfc_UpdateNtfList(&pNciContext->NciDiscContext.tDevInfo,
                                pRemoteDevInfo,TRUE);
        /* Check whether processing of discover notification is successful */
        if(NFCSTATUS_SUCCESS == wStoreStatus)
        {
            if(wLen != (bIndex + 1))
            {
                wStoreStatus = NFCSTATUS_FAILED;
            }
            else
            {
                *pIndex = bIndex;
            }
        }
    }
    return wStoreStatus;
}
