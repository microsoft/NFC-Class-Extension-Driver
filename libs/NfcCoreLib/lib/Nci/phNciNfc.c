/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc.tmh"

/** Value indicates length of Discover select command payload */
#define PHNCINFC_DISCSEL_PAYLOADLEN                 (0x03)
/** Value indicates length of Deactivate command payload */
#define PHNCINFC_DEACTIVATE_PAYLOADLEN              (0x01)
/** Value indicates length of Nfcee Discover command payload */
#define PHNCINFC_NFCEEDISC_PAYLOADLEN_1x            (0x01)
#define PHNCINFC_NFCEEDISC_PAYLOADLEN_2x            (0x00)

/** Value indicates length of Set Power substate command payload */
#define PHNCINFC_SETPOWERSUBSTATE_PAYLOADLEN        (0x01)

#define PHNCINFC_NUM_PARAMS_SIZE                    (1U)
/** Size of type and length fields (header) of a TLV */
#define PHNCINFC_TLV_HEADER_SIZE                    (2U)
/** Size of number of configurations field of Set config command */
#define PHNCINFC_NUM_CONFIGS_SIZE                   (1U)
/** Size of number of configurations field of Set config command */
#define PHNCINFC_RESET_RSP_PAYLOAD_SIZE             (3U)
/** Size of configurations field of Discover command */
#define PHNCINFC_DISC_CONFIGS_SIZE                  (2U)

/** ISO-DEP presence check notification timeout */
#define PHNCINFC_ISODEP_PRESCHK_NTF_TIMEROUT        (750)

pphNciNfc_Context_t volatile gpphNciNfc_Context = NULL;
pphNciNfc_CoreContext_t volatile gpphNciNfc_CoreContext = NULL;

static NFCSTATUS
phNciNfc_Release(
                 void*                        pNciHandle,
                 pphNciNfc_IfNotificationCb_t pReleaseCb,
                 void*                        pContext,
                 phNciNfc_ResetType_t         eResetType
                );

static void phNciNfc_ClearNciContext(void* pNciContext);
static void phNciNfc_ReleaseNfceeCntx(void);

static NFCSTATUS
phNciNfc_ResetNfcc(
                 void*                        pNciHandle,
                 phNciNfc_ResetType_t         eResetType,
                 pphNciNfc_IfNotificationCb_t pResetNfccCb,
                 void*                        pContext
                );

static NFCSTATUS phNciNfc_SendTxData(void *pContext);
static NFCSTATUS phNciNfc_ProcessDataRsp(void *pContext, NFCSTATUS Status);
static NFCSTATUS phNciNfc_CompleteDataSequence(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_SeEventCb(void* pContext, void *pInfo, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_SendIsoDepPresChkCmd(void *pContext);
static NFCSTATUS phNciNfc_ProcessIsoDepPresChkRsp(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_CompleteIsoDepPresChkSequence(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_SendSetPowerSubStateCmd(void *pContext);
static NFCSTATUS phNciNfc_ProcessSetPowerSubStateRsp(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_CompleteSetPowerSubStateSequence(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phLibNfc_GetAvailableSlotIndex(phNciNfc_SeEventList_t *pSeEventList, uint8_t *pSlotIndex);
static NFCSTATUS phLibNfc_GetRegisteredSlotIndex(phNciNfc_SeEventList_t *pSeEventList, void *pSeHandle, uint8_t *pSlotIndex);

/*Global Varibales for Transceive Sequence Handler*/
static phNciNfc_SequenceP_t phNciNfc_DataXchgSequence[] = {
    {&phNciNfc_SendTxData, &phNciNfc_ProcessDataRsp},
    {NULL, &phNciNfc_CompleteDataSequence}
};

/*Global Variables for ISO-DEP Presence Chk Sequence Handler */
static phNciNfc_SequenceP_t gphNciNfc_IsoDepPresChkSequence[] = {
    {&phNciNfc_SendIsoDepPresChkCmd, &phNciNfc_ProcessIsoDepPresChkRsp},
    {NULL, &phNciNfc_CompleteIsoDepPresChkSequence}
};

/*Global Variables for Power Sub-State Sequence Handler */
static phNciNfc_SequenceP_t gphNciNfc_SetPowerSubStateSequence[] = {
    {&phNciNfc_SendSetPowerSubStateCmd, &phNciNfc_ProcessSetPowerSubStateRsp},
    {NULL, &phNciNfc_CompleteSetPowerSubStateSequence}
};

inline pphNciNfc_Context_t phNciNfc_GetContext()
{
    return gpphNciNfc_Context;
}

void phNciNfc_SetContext(_In_opt_ pphNciNfc_Context_t pNciCtx)
{
    gpphNciNfc_Context = pNciCtx;
}

inline pphNciNfc_CoreContext_t phNciNfc_GetCoreContext()
{
    return gpphNciNfc_CoreContext;
}

void phNciNfc_SetCoreContext(_In_opt_ pphNciNfc_CoreContext_t pCoreCtx)
{
    gpphNciNfc_CoreContext = pCoreCtx;
}

inline bool_t
phNciNfc_IsVersion1x(_In_ pphNciNfc_Context_t pNciContext)
{
    return ((pNciContext->ResetInfo.NciVer & PH_NCINFC_VERSION_MAJOR_MASK) <= (PH_NCINFC_VERSION_1x & PH_NCINFC_VERSION_MAJOR_MASK));
}

inline bool_t
phNciNfc_IsVersion2x(_In_ pphNciNfc_Context_t pNciContext)
{
    return ((pNciContext->ResetInfo.NciVer & PH_NCINFC_VERSION_MAJOR_MASK) == (PH_NCINFC_VERSION_2x & PH_NCINFC_VERSION_MAJOR_MASK));
}

static NFCSTATUS phNciNfc_SendTxData(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
        TxInfo.tHeaderInfo.bConn_ID = (uint8_t)pNciContext->tTranscvCtxt.bConnId;
        TxInfo.Buff = pNciContext->tSendPayload.pBuff;
        TxInfo.wLen = pNciContext->tSendPayload.wPayloadSize;
        wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext), &TxInfo,
            &(pNciContext->RspBuffInfo), pNciContext->tTranscvCtxt.tTranscvInfo.wTimeout,
            (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence, pContext);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessDataRsp(void *pContext, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            pNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff = \
                pNciContext->RspBuffInfo.pBuff;
            pNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen = \
                pNciContext->RspBuffInfo.wLen;
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_CompleteDataSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if( (NULL != pNciCtx) && (NULL != pNciCtx->RspBuffInfo.pBuff) )
    {
        pNciCtx->tTranscvCtxt.pNotify(pNciCtx->tTranscvCtxt.pContext, wStatus,&pNciCtx->tTranscvCtxt.tTranscvInfo.tRecvData);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_Initialise(
                    void*                        pHwRef,
                    phNciNfc_Config_t*           pConfig,
                    pphNciNfc_IfNotificationCb_t pInitNotifyCb,
                    void*                        pContext,
                    phNciNfc_ResetType_t         eResetType)
{
    pphNciNfc_CoreContext_t pCoreContext = NULL;
    pphNciNfc_Context_t     pNciContext = phNciNfc_GetContext();
    NFCSTATUS               wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    if ((NULL == pHwRef) || (NULL == pConfig) || (NULL == pInitNotifyCb))
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameter");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(NULL == pNciContext)
    {
        pNciContext = (pphNciNfc_Context_t)phOsalNfc_GetMemory(sizeof(phNciNfc_Context_t));
        if(NULL != pNciContext)
        {
            phOsalNfc_SetMemory(pNciContext, 0, sizeof(phNciNfc_Context_t));
            phOsalNfc_MemCopy(&pNciContext->Config, pConfig, sizeof(phNciNfc_Config_t));

            phNciNfc_SetCoreContext(&pNciContext->NciCoreContext);
            phNciNfc_SetContext(pNciContext);

            phNciNfc_NciCtxInitialize(pNciContext);
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Failed to allocate memory, insufficient resuorces");
            wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Stack already initialized");
        wStatus = NFCSTATUS_ALREADY_INITIALISED;
    }

    if((NFCSTATUS_SUCCESS == wStatus) && (NULL != pNciContext))
    {
        phNciNfc_SetUpperLayerCallback(pNciContext, pInitNotifyCb, pContext);

        /*Update default Values*/
        PHNCINFC_INIT_SEQUENCE(pNciContext, gphNciNfc_InitSequence);
        pCoreContext = &(pNciContext->NciCoreContext);
        pCoreContext->pHwRef = pHwRef;
        pCoreContext->bLogDataMessages = pNciContext->Config.bLogDataMessages;
        wStatus = phNciNfc_CoreInitialise(pCoreContext);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            wStatus =  phNciNfc_LogConnInit(pNciContext);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                pNciContext->ResetInfo.ResetTypeReq = eResetType;
                /* Init payload to display the build number in the init response */
                pNciContext->tInitInfo.bExtension = 0x00;
                pNciContext->tInitInfo.bSkipRegisterAllNtfs = 0x00;
                wStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
                if(NFCSTATUS_PENDING != wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("Init Sequence failed!");
                    phNciNfc_FreeSendPayloadBuff(pNciContext);
                    phOsalNfc_FreeMemory(pNciContext);
                    phNciNfc_SetContext(NULL);
                    phNciNfc_SetCoreContext(NULL);
                }
            }
            else
            {
                PH_LOG_NCI_WARN_STR("phNciNfc_LogConnInit failed!");
                phOsalNfc_FreeMemory(pNciContext);
                phNciNfc_SetContext(NULL);
                phNciNfc_SetCoreContext(NULL);
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_NCI_WARN_STR("phNciNfc_CoreInitialise failed!");
            phOsalNfc_FreeMemory(pNciContext);
            phNciNfc_SetContext(NULL);
            phNciNfc_SetCoreContext(NULL);
            wStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_ReInitialise(void* pNciHandle,
                      pphNciNfc_IfNotificationCb_t pReInitNotifyCb,
                      void*                        pContext)
{
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;
    NFCSTATUS               wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciHandle) || (pNciHandle != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL != pReInitNotifyCb)
    {
        phNciNfc_SetUpperLayerCallback(pNciContext, pReInitNotifyCb, pContext);
        pNciContext->ResetInfo.ResetTypeReq = phNciNfc_ResetType_KeepConfig;
        /* Init payload to display the build number in the init response */
        pNciContext->tInitInfo.bExtension = 0x00;
        pNciContext->tInitInfo.bSkipRegisterAllNtfs = 0x01;
        PHNCINFC_INIT_SEQUENCE(pNciContext, gphNciNfc_InitSequence);
        wStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
        if(NFCSTATUS_PENDING != wStatus)
        {
            PH_LOG_NCI_CRIT_STR("Re-Initialize Sequence failed!");
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_StartDiscovery(void* pNciHandle,
                        phNciNfc_ADD_Cfg_t *pPollConfig,
                        pphNciNfc_IfNotificationCb_t pDiscoveryCb,
                        void *pContext)
{
    pphNciNfc_Context_t     pNciContext = pNciHandle;
    NFCSTATUS               wStatus = NFCSTATUS_SUCCESS;
    /* Store the Number of Discovery configurations */
    uint8_t bNoofConfigs=0;
    bool_t fIsNci1x = phNciNfc_IsVersion1x(pNciContext);
    bool_t fIsNci2x = phNciNfc_IsVersion2x(pNciContext);

    /*Note: ListenNfcFActive and PollNfcFActive exist only in NCI1.x specification.*/

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciHandle)
    {
        PH_LOG_NCI_CRIT_STR("Nci stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if( (NULL == pPollConfig) || (NULL == pDiscoveryCb) )
    {
        /**Invalid Parameter*/
        PH_LOG_NCI_CRIT_STR("Invalid input parameter");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Clear the Discovery context and start new Discover process */
        /* Update the DeActivation type to Idle Mode */
        pNciContext->NciDiscContext.eDeActvType = phNciNfc_e_IdleMode;
        (void )phNciNfc_ProcessDeActvState(pNciContext);

        /* Check whether Polling loop to be enabled for NFC-A Technology */
        if(pPollConfig->EnableIso14443A)
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for NFC-B Technology */
        if(pPollConfig->EnableIso14443B)
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for NFC-F Technology */
        if(pPollConfig->EnableFelica)
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for NFC-I Technology */
        if(pPollConfig->EnableIso15693)
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for Listen NFC-A Technology */
        if(pPollConfig->ListenNfcA)
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for Listen NFC-B Technology */
        if(pPollConfig->ListenNfcB)
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for Listen NFC-F Technology */
        if(pPollConfig->ListenNfcF)
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for Listen NFC-F Technology */
        if(pPollConfig->ListenNfcAActive ||
            (1 == pPollConfig->ListenNfcFActive && fIsNci2x))
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for Listen NFC-F Technology */
        if(pPollConfig->ListenNfcFActive && fIsNci1x)
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for Listen NFC-F Technology */
        if(pPollConfig->PollNfcAActive ||
            (1 == pPollConfig->PollNfcFActive && fIsNci2x))
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for Listen NFC-F Technology */
        if(pPollConfig->PollNfcFActive && fIsNci1x)
        {
            bNoofConfigs++;
        }
        /* Check whether Polling loop to be enabled for Kovio Technology */
        if (pPollConfig->EnableKovio)
        {
            bNoofConfigs++;
        }

        /* Update the ADD config pointer which is used during formation
            of packet */
        phOsalNfc_MemCopy(&(pNciContext->NciDiscContext.tConfig),pPollConfig,sizeof(phNciNfc_ADD_Cfg_t));
        /* Calculate and Allocate memory required to construct Discover Command payload */
        pNciContext->NciDiscContext.bDiscPayloadLen = (bNoofConfigs * PHNCINFC_DISC_CONFIGS_SIZE) + \
                                PHNCINFC_NUM_PARAMS_SIZE;
        pNciContext->NciDiscContext.pDiscPayload = (uint8_t *)
            phOsalNfc_GetMemory(pNciContext->NciDiscContext.bDiscPayloadLen);
        if(NULL != pNciContext->NciDiscContext.pDiscPayload)
        {
            wStatus = phNciNfc_UpdateDiscConfigParams(pNciContext,\
                                                        pPollConfig);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                pNciContext->NciDiscContext.pDiscPayload[0] = bNoofConfigs;
                phNciNfc_SetUpperLayerCallback(pNciContext, pDiscoveryCb, pContext);
                PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_DiscoverSequence);
                wStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
            }
            else
            {
                phOsalNfc_FreeMemory(pNciContext->NciDiscContext.pDiscPayload);
                pNciContext->NciDiscContext.pDiscPayload = NULL;
                pNciContext->IfNtf = NULL;
                pNciContext->IfNtfCtx = NULL;
            }
        }
        else
        {
            wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_Nfcee_StartDiscovery(void * pNciHandle,
                                        pphNciNfc_IfNotificationCb_t pNfceeDisCb,
                                        void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t * pNciContext= (phNciNfc_Context_t *)pNciHandle;
    uint8_t *pTargetInfo = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciContext)
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized (phNciNfc_Nfcee_StartDiscovery)");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if( (NULL == pNfceeDisCb)||\
             (PH_NCINFC_NFCEE_DISC_ENABLE == pNciContext->tNfceeContext.bNfceeDiscState) )
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameter passed(phNciNfc_Nfcee_StartDiscovery)");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Reset tSeEventList and NFCEE context */
        phOsalNfc_SetMemory(&pNciContext->tSeEventList,0,sizeof(phNciNfc_SeEventList_t));
        phNciNfc_ReleaseNfceeCntx();

        /* Store the state of NFCEE discovery to enabled */
        pNciContext->tNfceeContext.bNfceeDiscState = \
                    PH_NCINFC_NFCEE_DISC_ENABLE;

        if (phNciNfc_IsVersion1x(pNciContext))
        {
            pTargetInfo = (uint8_t *)phOsalNfc_GetMemory(PHNCINFC_NFCEEDISC_PAYLOADLEN_1x);
            if (NULL == pTargetInfo)
            {
                PH_LOG_NCI_CRIT_STR("Memory not available(phNciNfc_Nfcee_StartDiscovery)");
                wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                /* Build the Payload */
                pTargetInfo[0] = PH_NCINFC_NFCEE_DISC_ENABLE;
                pNciContext->tSendPayload.pBuff = pTargetInfo;
                pNciContext->tSendPayload.wPayloadSize = \
                    (uint16_t)PHNCINFC_NFCEEDISC_PAYLOADLEN_1x;
            }
        }
        else if (phNciNfc_IsVersion2x(pNciContext))
        {
            pNciContext->tSendPayload.pBuff = NULL;
            pNciContext->tSendPayload.wPayloadSize = \
                (uint16_t)PHNCINFC_NFCEEDISC_PAYLOADLEN_2x;
        }

        if (wStatus == NFCSTATUS_SUCCESS)
        {
            phNciNfc_SetUpperLayerCallback(pNciContext, pNfceeDisCb, pContext);
            PHNCINFC_INIT_SEQUENCE(pNciContext, gphNciNfc_NfceeDiscSequence);
            wStatus = phNciNfc_GenericSequence(pNciContext, NULL, wStatus);
            if (NFCSTATUS_PENDING != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Nfcee Discover Sequence failed!");
                phOsalNfc_FreeMemory(pTargetInfo);
                pNciContext->tSendPayload.pBuff = NULL;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_Nfcee_StopDiscovery(void * pNciHandle,
                                       pphNciNfc_IfNotificationCb_t pNfceeDisCb,
                                       void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t * pNciContext= (phNciNfc_Context_t *)pNciHandle;
    uint8_t *pTargetInfo = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciContext)
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized (phNciNfc_Nfcee_StopDiscovery)");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if( (NULL == pNfceeDisCb)||\
             (PH_NCINFC_NFCEE_DISC_DISABLE == pNciContext->tNfceeContext.bNfceeDiscState) )
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameter passed(phNciNfc_Nfcee_StopDiscovery)");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pTargetInfo = (uint8_t *)phOsalNfc_GetMemory(PHNCINFC_NFCEEDISC_PAYLOADLEN_1x);
        if(NULL == pTargetInfo)
        {
            PH_LOG_NCI_CRIT_STR("Memory not available(phNciNfc_Nfcee_StopDiscovery)");
            wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            /* Store the state of NFCEE discovery to enabled */
            pNciContext->tNfceeContext.bNfceeDiscState = \
                        PH_NCINFC_NFCEE_DISC_DISABLE;
            /* Build the Payload */
            pTargetInfo[0] = PH_NCINFC_NFCEE_DISC_DISABLE;
            pNciContext->tSendPayload.pBuff = pTargetInfo;
            pNciContext->tSendPayload.wPayloadSize = PHNCINFC_NFCEEDISC_PAYLOADLEN_1x;
            phNciNfc_SetUpperLayerCallback(pNciContext, pNfceeDisCb, pContext);
            PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_NfceeDiscSequence);
            wStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
            if(NFCSTATUS_PENDING != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Nfcee Discover Sequence failed!");
                phOsalNfc_FreeMemory(pTargetInfo);
                pNciContext->tSendPayload.pBuff = NULL;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_GetNfceeList(void *pNciHandle,
                                pphNciNfc_NfceeList_t pNfceeList)
{
    NFCSTATUS wListStatus = NFCSTATUS_SUCCESS;
    uint8_t bCount = 0;
    uint8_t bIndex = 0;
    phNciNfc_Context_t * pNciContext= (phNciNfc_Context_t *)pNciHandle;
    if(NULL == pNciContext)
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized (phNciNfc_GetNfceeList)");
        wListStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL == pNfceeList)
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameter passed(phNciNfc_GetNfceeList)");
        wListStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Update the Nfcee count */
        pNfceeList->bNfceeCount = pNciContext->tNfceeContext.bNumberOfNfcee;
        if(pNfceeList->bNfceeCount > 0)
        {
            /* Traverse through the Nfcee List to extract Nci Handle */
            for(bCount = 0; bCount < PH_NCINFC_NFCEE_DEVICE_MAX; bCount++)
            {
                if(0 != pNciContext->tNfceeContext.pNfceeDevInfo[bCount].tDevInfo.bNfceeID)
                {
                    pNfceeList->apNfceeList[bIndex++] = \
                        &pNciContext->tNfceeContext.pNfceeDevInfo[bCount];
                }
            }
            /* Verify whether the count and list of nfcee are similar */
            if(pNfceeList->bNfceeCount != bIndex)
            {
                wListStatus = NFCSTATUS_FAILED;
            }
        }
    }
    return wListStatus;
}

NFCSTATUS phNciNfc_Nfcee_ModeSet(void * pNciHandle,
                                  uint8_t bNfceeID,
                                  phNciNfc_NfceeModes_t eNfceeMode,
                                  pphNciNfc_IfNotificationCb_t pNotifyCb,
                                  void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t * pNciContext = (phNciNfc_Context_t *)pNciHandle;
    uint8_t *pTargetInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL == pNciContext)
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if ((eNfceeMode > PH_NCINFC_EXT_NFCEEMODE_ENABLE) ||
             (NULL == pNotifyCb))
    {
        PH_LOG_NCI_CRIT_STR("Invalid eNfceeMode parameter passed:%!phNciNfc_NfceeModes_t!", eNfceeMode);
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pTargetInfo = (uint8_t *)phOsalNfc_GetMemory(PHNCINFC_NFCEEMODESET_PAYLOADLEN);
        if (NULL == pTargetInfo)
        {
            PH_LOG_NCI_CRIT_STR("Memory not available.");
            wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            /* Store the Payload */
            pNciContext->tNfceeContext.eNfceeMode = eNfceeMode;
            pTargetInfo[0] = bNfceeID;
            pTargetInfo[1] = (uint8_t)eNfceeMode;
            pNciContext->tSendPayload.pBuff = pTargetInfo;
            pNciContext->tSendPayload.wPayloadSize = (uint16_t)PHNCINFC_NFCEEMODESET_PAYLOADLEN;
            phNciNfc_SetUpperLayerCallback(pNciContext, pNotifyCb, pContext);

            PHNCINFC_INIT_SEQUENCE(pNciContext, gphNciNfc_ModeSetSequence);
            wStatus = phNciNfc_GenericSequence(pNciContext, NULL, wStatus);

            if (NFCSTATUS_PENDING != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Nfcee ModeSet Sequence failed with status:%!NFCSTATUS!", wStatus);
                phOsalNfc_FreeMemory(pTargetInfo);
                pTargetInfo = NULL;
                pNciContext->tSendPayload.pBuff = NULL;
                pNciContext->tSendPayload.wPayloadSize = 0;
            }
        }

    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_Nfcee_SePowerAndLinkCtrlSet(void * pNciHandle,
                                               void * pNfceeHandle,
                                               phNciNfc_PowerLinkModes_t eActivationMode,
                                               pphNciNfc_IfNotificationCb_t pNotifyCb,
                                               void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t * pNciContext = (phNciNfc_Context_t *)pNciHandle;
    pphNciNfc_NfceeDeviceHandle_t pDevHandle =
        (pphNciNfc_NfceeDeviceHandle_t)pNfceeHandle;
    uint8_t *pTargetInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL == pNciContext)
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if (NULL == pNotifyCb)
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameter passed");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        if ((NULL != pDevHandle) &&
            (0 != pDevHandle->tDevInfo.bNfceeID) &&
            (PHNCINFC_INVALID_DISCID != pDevHandle->tDevInfo.bNfceeID))
        {
            pTargetInfo = (uint8_t *)phOsalNfc_GetMemory(PH_NCINFC_NFCEEPOWERLINKCTRL_PAYLOADLEN);
            if (NULL == pTargetInfo)
            {
                PH_LOG_NCI_CRIT_STR("Memory not available");
                wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                /* Store the Payload */
                pTargetInfo[0] = pDevHandle->tDevInfo.bNfceeID;
                pTargetInfo[1] = (uint8_t)eActivationMode;
                pNciContext->tSendPayload.pBuff = pTargetInfo;
                pNciContext->tSendPayload.wPayloadSize =
                    (uint16_t)PH_NCINFC_NFCEEPOWERLINKCTRL_PAYLOADLEN;
                phNciNfc_SetUpperLayerCallback(pNciContext, pNotifyCb, pContext);
                PHNCINFC_INIT_SEQUENCE(pNciContext, gphNciNfc_SePowerAndLinkCtrlSequence);
                wStatus = phNciNfc_GenericSequence(pNciContext, NULL, wStatus);
                if (NFCSTATUS_PENDING != wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("Nfcee PowerAndLinkCtrlSet Sequence failed!");
                    phOsalNfc_FreeMemory(pTargetInfo);
                    pNciContext->tSendPayload.pBuff = NULL;
                }
            }
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_RegisterNotification(
                        void                        *pNciHandle,
                        phNciNfc_RegisterType_t     eRegisterType,
                        pphNciNfc_Notification_t    pRegNotifyCb,
                        void                        *pContext)
{
    pphNciNfc_Context_t     pNciContext = pNciHandle;
    NFCSTATUS               wRegStatus = NFCSTATUS_SUCCESS;
    UNUSED(eRegisterType);
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciHandle)
    {
        PH_LOG_NCI_CRIT_STR("Nci stack not initialized");
        wRegStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL == pRegNotifyCb)
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameter");
        wRegStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        switch(eRegisterType)
        {
            case phNciNfc_e_RegisterTagDiscovery:
            {
                /* Register Callback functions to be invoked upon
                   Discovery of remote devices */
                pNciContext->tRegListInfo.pDiscoveryNotification =
                        pRegNotifyCb;
                pNciContext->tRegListInfo.DiscoveryCtxt = pContext;
            }break;
            case phNciNfc_e_RegisterSecureElement:
            {
                /* Register Callback functions to be invoked upon
                   NFCEE notifications */
                pNciContext->tRegListInfo.pNfceeNotification =
                        pRegNotifyCb;
                pNciContext->tRegListInfo.NfceeCtxt = pContext;
            }break;
            case phNciNfc_e_RegisterRfDeActivate:
            {
                /* Register Callback functions to be invoked upon
                   NFCEE notifications */
                pNciContext->tDeActvInfo.pDeActvNotif =
                        pRegNotifyCb;
                pNciContext->tDeActvInfo.DeActvCtxt = pContext;
            }break;
            case phNciNfc_e_RegisterGenericError:
            {
                /* Register Callback functions to be invoked upon
                   Generic error notification */
                pNciContext->tRegListInfo.pGenericErrNtfCb = pRegNotifyCb;
                pNciContext->tRegListInfo.GenericErrNtfCtxt = pContext;
            }break;
            case phNciNfc_e_RegisterReset:
            {
                /* Register Callback functions to be invoked upon
                   Reset notification */
                pNciContext->tRegListInfo.pResetNtfCb = pRegNotifyCb;
                pNciContext->tRegListInfo.ResetNtfCtxt = pContext;
            }break;
            default:
            {
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wRegStatus;
}

NFCSTATUS phNciNfc_DeregisterNotification(
                        void                        *pNciHandle,
                        phNciNfc_RegisterType_t     eRegisterType)
{
    pphNciNfc_Context_t     pNciContext = pNciHandle;
    NFCSTATUS               wDeregStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciHandle)
    {
        PH_LOG_NCI_CRIT_STR("Nci stack not initialized");
        wDeregStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else
    {
        switch(eRegisterType)
        {
            case phNciNfc_e_RegisterTagDiscovery:
            {
                /* DeRegister Callback functions registered to be
                   invoked upon receiving tag notifications */
                pNciContext->tRegListInfo.pDiscoveryNotification =
                        NULL;
                pNciContext->tRegListInfo.DiscoveryCtxt = NULL;
            }break;
            case phNciNfc_e_RegisterSecureElement:
            {
                /* Register Callback functions to be invoked upon
                   Discovery of remote devices */
                pNciContext->tRegListInfo.pDiscoveryNotification =
                        NULL;
                pNciContext->tRegListInfo.NfceeCtxt = NULL;
            }break;
            case phNciNfc_e_RegisterGenericError:
            {
                /* Register Callback functions to be invoked upon
                   Generic error notification */
                pNciContext->tRegListInfo.pGenericErrNtfCb = NULL;
                pNciContext->tRegListInfo.GenericErrNtfCtxt = NULL;
            }break;
            case phNciNfc_e_RegisterReset:
            {
                pNciContext->tRegListInfo.pResetNtfCb = NULL;
                pNciContext->tRegListInfo.ResetNtfCtxt = NULL;
            }break;
            default:
            {
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wDeregStatus;
}

NFCSTATUS phNciNfc_Connect(void *pNciHandle,
                    pphNciNfc_RemoteDevInformation_t pRemoteDevInfo,
                    phNciNfc_RfInterfaces_t eRfInterface,
                    pphNciNfc_IfNotificationCb_t pDiscSelCb,
                    void *pContext)
{
    NFCSTATUS wConnectStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciContext = pNciHandle;
    uint8_t *pTargetInfo;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciContext)
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized (phNciNfc_Connect)");
        wConnectStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL != pRemoteDevInfo) && (NULL != pDiscSelCb) &&
        (0 != pRemoteDevInfo->bRfDiscId) &&
        (PHNCINFC_INVALID_DISCID != pRemoteDevInfo->bRfDiscId) )
    {
        pTargetInfo = (uint8_t *)phOsalNfc_GetMemory(PHNCINFC_DISCSEL_PAYLOADLEN);
        if(NULL != pTargetInfo)
        {
            pTargetInfo[0] = pRemoteDevInfo->bRfDiscId;
            pTargetInfo[1] = pRemoteDevInfo->eRFProtocol;
            pTargetInfo[2] = eRfInterface;
            pNciContext->NciDiscContext.pDiscPayload = pTargetInfo;
            pNciContext->NciDiscContext.bDiscPayloadLen = PHNCINFC_DISCSEL_PAYLOADLEN;
            phNciNfc_SetUpperLayerCallback(pNciContext, pDiscSelCb, pContext);
            PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_DiscSelSequence);
            wConnectStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
            if(NFCSTATUS_PENDING != wConnectStatus)
            {
                PH_LOG_NCI_CRIT_STR("Connect Sequence failed!");
                phOsalNfc_FreeMemory(pTargetInfo);
                pNciContext->NciDiscContext.pDiscPayload = NULL;
                pNciContext->IfNtf = NULL;
                pNciContext->IfNtfCtx = NULL;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Memory not available(phNciNfc_Connect)");
            wConnectStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameter passed(phNciNfc_Connect)");
        wConnectStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wConnectStatus;
}

NFCSTATUS phNciNfc_Deactivate(void *pNciHandle,
                    phNciNfc_DeActivateType_t eDeActivateType,
                    pphNciNfc_IfNotificationCb_t pDeActivateCb,
                    void *pContext)
{
    NFCSTATUS wDeActivateStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciContext = pNciHandle;
    uint8_t *pTargetInfo;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciContext)
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized (phNciNfc_InterfaceDeactivate)");
        wDeActivateStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL == pDeActivateCb)
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameter passed (phNciNfc_InterfaceDeactivate)");
        wDeActivateStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pTargetInfo = (uint8_t *)phOsalNfc_GetMemory(PHNCINFC_DEACTIVATE_PAYLOADLEN);
        if(NULL != pTargetInfo)
        {
            phNciNfc_SetUpperLayerCallback(pNciContext, pDeActivateCb, pContext);

            wDeActivateStatus = phNciNfc_ValidateDeActvType(pNciContext, &pNciContext->NciDiscContext,
                                    eDeActivateType,&pNciContext->NciDiscContext.eDeActvType);
            if(NFCSTATUS_SUCCESS == wDeActivateStatus)
            {
                /* If remote device type is a P2P Target, handle priority de-activate request */
                phNciNfc_HandlePriorityDeactv(&pNciContext->NciDiscContext);

                /* Store the Deactivate Type for future reference */
                pNciContext->NciDiscContext.eDeActvType = eDeActivateType;
                /* Fill the buffer details */
                pTargetInfo[0] = (uint8_t)eDeActivateType;
                /* Store the payload info in the context */
                pNciContext->tSendPayload.pBuff = pTargetInfo;
                pNciContext->tSendPayload.wPayloadSize = PHNCINFC_DEACTIVATE_PAYLOADLEN;

                /* Un-register any data message registered on logical connection '0' if found */
                phNciNfc_CoreRecvMgrDeRegDataCb((void *)&pNciContext->NciCoreContext,0);

                /* Update the Upper layer callback function and parameter */
                PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_DeActivateSequence);
                /* Start the Deactivate Sequence */
                wDeActivateStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
            }
            else
            {
                phOsalNfc_FreeMemory(pTargetInfo);
                pNciContext->tSendPayload.pBuff = NULL;
                pNciContext->tSendPayload.wPayloadSize = 0x00;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Memory not available(phNciNfc_Deactivate)");
            wDeActivateStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wDeActivateStatus;
}


NFCSTATUS
phNciNfc_SetConfigRfParameters(
                            void*                      pNciHandle,
                            pphNciNfc_RfDiscConfigParams_t pDiscConfigParams,
                            pphNciNfc_IfNotificationCb_t pConfigRfNotifyCb,
                            void*                      pContext)
{
    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;
    uint16_t wParamsSize = 0;
    uint8_t bNumParams = 0;
    uint16_t wPayloadSize = 0;
    uint8_t *pPayloadBuff = NULL;
    uint16_t bNumParamsOffset = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciHandle) || (pNciHandle != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL != pDiscConfigParams) && (NULL != pConfigRfNotifyCb))
    {
        wStatus = phNciNfc_ValidateSetConfParams(pDiscConfigParams,&wParamsSize,&bNumParams);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if (pNciContext->Config.bConfigOpt)
            {
                pNciContext->tSetConfOptInfo.pSetConfParams =
                                (pphNciNfc_RfDiscConfigParams_t)phOsalNfc_GetMemory(sizeof(phNciNfc_RfDiscConfigParams_t));
                if(NULL != pNciContext->tSetConfOptInfo.pSetConfParams)
                {
                    phOsalNfc_SetMemory(pNciContext->tSetConfOptInfo.pSetConfParams,0,
                                        sizeof(phNciNfc_RfDiscConfigParams_t));
                    phOsalNfc_MemCopy(pNciContext->tSetConfOptInfo.pSetConfParams,
                                      pDiscConfigParams, sizeof(phNciNfc_RfDiscConfigParams_t));

                    phNciNfc_SetUpperLayerCallback(pNciContext, pConfigRfNotifyCb, pContext);
                    PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_SetConfigOptSequence);

                    wStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED;
                    PH_LOG_NCI_CRIT_STR("Memory allocation for Set Config Cmd payload failed!");
                }
            }
            else
            {
                /* Payload size = Size of all input parameters + (memory for storing type & length fields of
                                  all parameters) + memory for storing number of parameters (tlv's) */
                wPayloadSize = wParamsSize + (bNumParams * PHNCINFC_TLV_HEADER_SIZE) + PHNCINFC_NUM_CONFIGS_SIZE;
                pPayloadBuff = (uint8_t *) phOsalNfc_GetMemory((uint32_t)wPayloadSize);
                if(NULL != pPayloadBuff)
                {
                    pPayloadBuff[bNumParamsOffset] = bNumParams;
                    wStatus = phNciNfc_BuildSetConfPayload(pDiscConfigParams,pPayloadBuff,wPayloadSize);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        phNciNfc_SetUpperLayerCallback(pNciContext, pConfigRfNotifyCb, pContext);
                        PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_SetConfigSequence);
                        pNciContext->tSendPayload.pBuff = pPayloadBuff;
                        pNciContext->tSendPayload.wPayloadSize = wPayloadSize;

                        wStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
                        if(NFCSTATUS_PENDING != wStatus)
                        {
                            PH_LOG_NCI_CRIT_STR("Set Config Sequence failed!");
                            phNciNfc_FreeSendPayloadBuff(pNciContext);
                        }
                    }
                    else
                    {
                        PH_LOG_NCI_CRIT_STR("Build Set config command payload failed!");
                        phOsalNfc_FreeMemory(pPayloadBuff);
                        pPayloadBuff = NULL;
                    }
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Memory allocation for Set Config Cmd payload failed!");
                    wStatus = NFCSTATUS_FAILED;
                }
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Set Config parameter validation failed!");
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_GetConfigRaw(
                            void*                      pNciHandle,
                            uint8_t *pBuff, uint16_t Length,
                            pphNciNfc_IfNotificationCb_t pConfigRfNotifyCb,
                            void*                      pContext)
{

    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciHandle) || (pNciHandle != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL != pBuff) && (0 != Length))
    {
        pNciContext->tSendPayload.pBuff = (uint8_t *) phOsalNfc_GetMemory((uint32_t)Length);
        phOsalNfc_MemCopy(pNciContext->tSendPayload.pBuff, pBuff,Length);
        /* Input parameters are valid and payload field has been framed successfully */
        phNciNfc_SetUpperLayerCallback(pNciContext, pConfigRfNotifyCb, pContext);
        PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_GetConfigRawSequence);

        pNciContext->tSendPayload.wPayloadSize = Length;
        wStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
        if(NFCSTATUS_PENDING != wStatus)
        {
              PH_LOG_NCI_CRIT_STR("Get Config Raw Sequence failed!");
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_ConfigMapping (void*                        pNciHandle,
                        uint8_t                      bNumMapEntries,
                        pphNciNfc_MappingConfig_t    pProtoIfMapping,
                        pphNciNfc_IfNotificationCb_t pConfigMappingNotifyCb,
                        void*                        pContext)
{
    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;
    uint16_t wPayloadSize = 0;
    uint8_t *pPayloadBuff = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciHandle) || (pNciHandle != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL != pProtoIfMapping) && (NULL != pConfigMappingNotifyCb))
    {
        if(0 != bNumMapEntries)
        {
            /* Validate input parameters (Rf interface, RF protocol, mode) */
            wStatus = phNciNfc_ValidateDiscMapParams(bNumMapEntries,pProtoIfMapping);
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("No entries");
            wStatus = NFCSTATUS_FAILED;
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            wStatus = phNciNfc_VerifySupportedRfIntfs(pNciContext,bNumMapEntries,pProtoIfMapping);
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Parameter validation failed");
            wStatus = NFCSTATUS_FAILED;
        }

        if(NFCSTATUS_SUCCESS  == wStatus)
        {
            wPayloadSize = (bNumMapEntries * PHNCINFC_RFCONFIG_NUMPAYLOADFIELDS) +
                            (PHNCINFC_RFCONFIG_NUMMAPENTRIES_SIZE);

            pPayloadBuff = phOsalNfc_GetMemory((uint32_t)wPayloadSize);

            if(NULL != pPayloadBuff)
            {
                phNciNfc_BuildDiscMapCmdPayload(pPayloadBuff,bNumMapEntries,pProtoIfMapping);

                phNciNfc_SetUpperLayerCallback(pNciContext, pConfigMappingNotifyCb, pContext);
                PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_ProtoIfMapSequence);
                pNciContext->tSendPayload.pBuff = pPayloadBuff;
                pNciContext->tSendPayload.wPayloadSize = wPayloadSize;

                wStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
                if(NFCSTATUS_PENDING != wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("Config Mapping Sequence failed!");
                    phNciNfc_FreeSendPayloadBuff(pNciContext);
                }
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Memory allcation for command payload failed");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_SetRtngTableConfig(void*                     pNciHandle,
                         uint8_t                      bNumRtngEntries,
                         pphNciNfc_RtngConfig_t       pRtngConfig,
                         pphNciNfc_IfNotificationCb_t pSetRtngTableNotifyCb,
                         void*                        pContext)
{
    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;
    phNciNfc_NfccFeatures_t tNfccFeatures;
    uint16_t wPayloadSize = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciHandle) || (pNciHandle != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL != pRtngConfig) && (NULL != pSetRtngTableNotifyCb))
    {
        if(0 != bNumRtngEntries)
        {
            wStatus = phNciNfc_ValidateSetRtngParams(bNumRtngEntries,pRtngConfig,&wPayloadSize);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Input parameter validation failed");
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("No entries");
            wStatus = NFCSTATUS_FAILED;
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Check if total size of the routing configuration information
               exceeds the 'Max Routing Table Size' (indicated during Initialization) supported by NFCC */
            if(wPayloadSize > pNciContext->InitRspParams.RoutingTableSize)
            {
                PH_LOG_NCI_WARN_STR("Input Routing config size exceeds Max routing table size \
                                    supported by NFCC");
                wStatus = NFCSTATUS_FAILED;
            }
            else
            {
                wStatus  = phNciNfc_GetNfccFeatures((void *)pNciContext,&tNfccFeatures);
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    /* Verify input routing entires are supported by NFCC */
                    wStatus = phNciNfc_VerifySupportedRouting(&tNfccFeatures,
                                        bNumRtngEntries,pRtngConfig);
                }
                else
                {
                    PH_LOG_NCI_WARN_STR("Failed to get NFCC features!");
                }
            }
        }

        if(NFCSTATUS_SUCCESS  == wStatus)
        {
            phNciNfc_SetUpperLayerCallback(pNciContext, pSetRtngTableNotifyCb, pContext);

            /* Initializing Rtng config structure with default values */
            pNciContext->tRtngConfInfo.bMore = 0;
            pNciContext->tRtngConfInfo.bRtngEntryOffset = 0;
            pNciContext->tRtngConfInfo.bTotalNumEntries = bNumRtngEntries;
            pNciContext->pUpperLayerInfo = (void *) pRtngConfig;

            wStatus = phNciNfc_SetRtngCmdHandler(pNciContext);
            if(NFCSTATUS_PENDING != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Set Rtng Config Sequence failed!");
                phNciNfc_FreeSendPayloadBuff(pNciContext);
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_GetRtngTableConfig(void*                        pNciHandle,
                            pphNciNfc_IfNotificationCb_t pGetRtngTableNotifyCb,
                            void*                        pContext)
{
    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciHandle) || (pNciHandle != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL != pGetRtngTableNotifyCb)
    {
        /* If max Rtng size of '0', it means Listen mode routing table is not supported by NFCC.*/
        if(0 != pNciContext->InitRspParams.RoutingTableSize)
        {
            wStatus = NFCSTATUS_FAILED;
            {
                phNciNfc_SetUpperLayerCallback(pNciContext, pGetRtngTableNotifyCb, pContext);

                PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_GetRtngConfigSequence);

                wStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
                if(NFCSTATUS_PENDING != wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("Get Rtng Config Sequence failed!");
                    phNciNfc_FreeSendPayloadBuff(pNciContext);
                }
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Max Rtng table size is '0', can not read Rtng table!");
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_RfParameterUpdate(void*                        pNciHandle,
                           uint8_t                      bNumOfParams,
                           pphNciNfc_RfParamUpdate_t    pRfParamUpdate,
                           pphNciNfc_IfNotificationCb_t pRfParamUpdateCb,
                           void*                        pContext)
{
    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;
    uint16_t wPayloadSize = 0;
    uint8_t *pPayloadBuff = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciContext) || (pNciContext != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL != pRfParamUpdate) && (NULL != pRfParamUpdateCb))
    {
        if(0 != bNumOfParams)
        {
            wStatus = NFCSTATUS_FAILED;
            /* Based on number of params calculate the total size required for storing the command payload */
            wPayloadSize = (bNumOfParams * PHNCINFC_RFCONFIG_NUMPAYLOADFIELDS) +
                            PHNCINFC_NUM_PARAMS_SIZE;
            /* Allocating memory for storing command payload */
            pPayloadBuff = phOsalNfc_GetMemory((uint32_t)wPayloadSize);
            if(NULL != pPayloadBuff)
            {
                wStatus = NFCSTATUS_SUCCESS;
                pPayloadBuff[0] = bNumOfParams;
                /* Build pkt with 'bNumEntries' entries starting from 'offset' entry */
                wStatus = phNciNfc_BuildRfParamUpdateCmdPayload(&pPayloadBuff[1],bNumOfParams,pRfParamUpdate);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("Invalid parameter passed!");
                    phOsalNfc_FreeMemory(pPayloadBuff);
                    wStatus = NFCSTATUS_FAILED;
                }
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("No entries");
            wStatus = NFCSTATUS_FAILED;
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            phNciNfc_SetUpperLayerCallback(pNciContext, pRfParamUpdateCb, pContext);

            PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_RfParamUpdateSequence);
            pNciContext->tSendPayload.pBuff = pPayloadBuff;
            pNciContext->tSendPayload.wPayloadSize = wPayloadSize;

            wStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
            if(NFCSTATUS_PENDING != wStatus)
            {
                phNciNfc_FreeSendPayloadBuff(pNciContext);
                PH_LOG_NCI_CRIT_STR("Rf parameter update failed!");
            }
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_GetConfigRfParameters(
                            void*                          pNciHandle,
                            pphNciNfc_RfDiscConfigParams_t pDiscConfigParams,
                            pphNciNfc_IfNotificationCb_t   pConfigRfNotifyCb,
                            void*                          pContext)
{
    NFCSTATUS wGetStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bParamCount = 0;
    uint8_t bParamLen = 0;
    uint16_t wPayloadLen = 0;
    uint8_t *pPayloadBuff = NULL;
    uint8_t aRfConfigParams[PHNCINFC_RFCONFIG_TOTALPARAM_COUNT];
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciContext)
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wGetStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL != pDiscConfigParams) && (NULL != pConfigRfNotifyCb))
    {
        wGetStatus = phNciNfc_GetRfParams(pDiscConfigParams, aRfConfigParams, &bParamCount,\
                                            &bParamLen);
        if( (NFCSTATUS_SUCCESS == wGetStatus) && (0 != bParamCount) )
        {
            wPayloadLen = bParamLen + 1;
            pPayloadBuff = (uint8_t *) phOsalNfc_GetMemory((uint32_t)wPayloadLen);
            if(NULL != pPayloadBuff)
            {
                *pPayloadBuff = bParamCount;
                phOsalNfc_MemCopy( (pPayloadBuff + 1),aRfConfigParams,bParamLen);

                phNciNfc_SetUpperLayerCallback(pNciContext, pConfigRfNotifyCb, pContext);
                PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_GetConfigSequence);
                pNciContext->tSendPayload.pBuff = pPayloadBuff;
                pNciContext->tSendPayload.wPayloadSize = wPayloadLen;

                pNciContext->pUpperLayerInfo = (void *)pDiscConfigParams;
                phOsalNfc_SetMemory(pDiscConfigParams,0x00,
                                    sizeof(phNciNfc_RfDiscConfigParams_t));
                pNciContext->tRfConfContext.bReqParamNum = bParamCount;
                pNciContext->tRfConfContext.bReqParamLen = bParamLen;
                pNciContext->tRfConfContext.pReqParamList = pPayloadBuff;
                wGetStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
            }
            else
            {
                wGetStatus = NFCSTATUS_FAILED;
                PH_LOG_NCI_CRIT_STR("Memory not available");
            }
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wGetStatus;
}

NFCSTATUS
phNciNfc_Transceive(  void*                               pNciHandle,
                        void*                               pDevicehandle,
                        pphNciNfc_TransceiveInfo_t         psTransceiveInfo,
                        pphNciNfc_TransreceiveCallback_t    pTrcvCallback,
                        void*                               pContext)
{
    NFCSTATUS wTransceiveStatus = NFCSTATUS_PENDING;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;
    uint8_t bConnId = 0x00;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciHandle)
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wTransceiveStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if( (NULL == pDevicehandle) ||\
             (NULL == psTransceiveInfo) ||\
             (NULL == pTrcvCallback) )
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameters");
        wTransceiveStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if( (NULL == psTransceiveInfo->tSendData.pBuff) ||\
             (0 == psTransceiveInfo->tSendData.wLen) )
    {
        wTransceiveStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Send Buff Params.");
    }
    else if(((NULL == psTransceiveInfo->tRecvData.pBuff) ||\
             (0 == psTransceiveInfo->tRecvData.wLen))&&
             (phNciNfc_eT2TAuth != psTransceiveInfo->uCmd.T2TCmd ))
    {
        wTransceiveStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Recv Buff Params.");
    }
    else
    {
        if(psTransceiveInfo->wTimeout == 0)
        {
            psTransceiveInfo->wTimeout = PHNCINFC_NCI_TRANSCEIVE_TIMEOUT;
        }

        PH_LOG_NCI_INFO_U32MSG("Timeout for transceive", psTransceiveInfo->wTimeout);

        wTransceiveStatus = phNciNfc_GetConnId(pDevicehandle,&bConnId);
        if(NFCSTATUS_SUCCESS == wTransceiveStatus)
        {
            /* Invoke Reader management if the Connection ID is static */
            if(0x00 == bConnId)
            {
                /* Invoke the Reader Management to exchange data */
                wTransceiveStatus = phNciNfc_RdrMgmtXchgData(pNciHandle,
                                                pDevicehandle,
                                                psTransceiveInfo,
                                                pTrcvCallback,
                                                pContext);
            }
            else
            {
                /* Store Transceive Info */
                pNciContext->tTranscvCtxt.tTranscvInfo.uCmd = psTransceiveInfo->uCmd;
                pNciContext->tTranscvCtxt.tTranscvInfo.bAddr = psTransceiveInfo->bAddr;
                pNciContext->tTranscvCtxt.tTranscvInfo.bNumBlock = psTransceiveInfo->bNumBlock;
                pNciContext->tTranscvCtxt.tTranscvInfo.tSendData = psTransceiveInfo->tSendData;
                pNciContext->tTranscvCtxt.tTranscvInfo.tRecvData = psTransceiveInfo->tRecvData;
                pNciContext->tTranscvCtxt.tTranscvInfo.wTimeout = psTransceiveInfo->wTimeout;
                pNciContext->tTranscvCtxt.bConnId = bConnId;
                pNciContext->tSendPayload.pBuff = psTransceiveInfo->tSendData.pBuff;
                pNciContext->tSendPayload.wPayloadSize = psTransceiveInfo->tSendData.wLen;

                PHNCINFC_INIT_SEQUENCE(pNciContext, phNciNfc_DataXchgSequence);
                wTransceiveStatus = phNciNfc_GenericSequence(pNciContext,NULL,wTransceiveStatus);
                if(NFCSTATUS_PENDING == wTransceiveStatus)
                {
                    pNciContext->tTranscvCtxt.pNotify = pTrcvCallback;
                    pNciContext->tTranscvCtxt.pContext = pContext;
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wTransceiveStatus;
}

NFCSTATUS phNciNfc_RegisterHciSeEvent(void* pNciCtx,
                                      void* pSeHandle,
                                      pphNciNfc_RegDataCb_t pSeEventCb,
                                      void* pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t )pNciCtx;
    uint8_t bConnId = 0x00;
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo;
    uint8_t bSlotIndex;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL == pContext || NULL == pSeEventCb)
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameters");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }

    if (wStatus == NFCSTATUS_SUCCESS &&
        (NULL == pNciContext || phNciNfc_GetContext() != pNciContext) )
    {
        PH_LOG_NCI_CRIT_STR("Invalid Nci context!");
        wStatus = NFCSTATUS_INVALID_STATE;
    }

    if (wStatus == NFCSTATUS_SUCCESS)
    {
        if (!phNciNfc_IsVersion1x(pNciContext))
        {
            bConnId = CONNHCITYPE_STATIC;
        }
        else
        {
            wStatus = phNciNfc_GetConnId(pSeHandle,&bConnId);
        }

        if (NFCSTATUS_SUCCESS == wStatus)
        {
            /* Get available free slot index */
            wStatus = phLibNfc_GetAvailableSlotIndex(&pNciContext->tSeEventList, &bSlotIndex);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                /* Register local static function with the NCI core receive manager */
                PH_LOG_NCI_INFO_STR("Registering SE event with NCI Core");
                tHeaderInfo.bEnabled = PHNCINFC_DISABLE_AUTO_DEREG;
                tHeaderInfo.bConn_ID = bConnId;
                tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
                /* Register with Data manager the Callback function to be invoked
                    when there is data on this logical connection */
                wStatus = phNciNfc_CoreIfRegRspNtf(&(pNciContext->NciCoreContext),
                                                    &(tHeaderInfo),
                                                    &phNciNfc_SeEventCb,
                                                    pSeHandle);
                if (NFCSTATUS_SUCCESS == wStatus)
                {
                    /* Register upper layer call back function (Store SE handle also) */
                    pNciContext->tSeEventList.aSeEventList[bSlotIndex].pSeHandle = pSeHandle;
                    pNciContext->tSeEventList.aSeEventList[bSlotIndex].pUpperLayerCb = pSeEventCb;
                    pNciContext->tSeEventList.aSeEventList[bSlotIndex].pUpperLayerCtx = pContext;
                    pNciContext->tSeEventList.aSeEventList[bSlotIndex].bEnable = 1;
                }
                else
                {
                    /* Failed to register with Nci core */
                    wStatus = NFCSTATUS_FAILED;
                    PH_LOG_NCI_CRIT_STR("Failed to register with Nci core");
                }
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("No free slots available, registration failed!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void phNciNfc_ClearNciContext(void* pNciContext)
{
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t ) pNciContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciCtx) || (pNciCtx != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
    }
    else
    {
        /* Release Memory related to Discovery module */
        /* Update the DeActivation type to Idle Mode */
        pNciCtx->NciDiscContext.eDeActvType = phNciNfc_e_IdleMode;
        (void )phNciNfc_ProcessDeActvState(pNciCtx);

        phNciNfc_CoreRecvMgrDeRegisterAll(&(pNciCtx->NciDiscContext));
        phNciNfc_CleanCoreContext(&pNciCtx->NciCoreContext);

        phOsalNfc_SetMemory(&pNciCtx->InitRspParams,0,sizeof(phNciNfc_sInitRspParams_t));
        phOsalNfc_SetMemory(&pNciCtx->ResetInfo,0,sizeof(phNciNfc_ResetInfo_t));
        phOsalNfc_SetMemory(&pNciCtx->NciDiscContext.tConfig,0,sizeof(phNciNfc_ADD_Cfg_t));
        phOsalNfc_SetMemory(&pNciCtx->NciDiscContext.tDevInfo,0,sizeof(phNciNfc_DeviceInfo_t));
        phOsalNfc_SetMemory(&pNciCtx->tNfceeContext,0,sizeof(phNciNfc_NfceeContext_t));
        phOsalNfc_SetMemory(&pNciCtx->tActvDevIf,0,sizeof(phNciNfc_ActiveDeviceInfo_t));
        phOsalNfc_SetMemory(&pNciCtx->tTranscvCtxt.tTranscvInfo,0,sizeof(phNciNfc_TransceiveInfo_t));

        pNciCtx->IfNtf = NULL;
        pNciCtx->IfNtfCtx = NULL;
        pNciCtx->pSeqHandler = NULL;
        pNciCtx->SeqNext = 0;
        pNciCtx->SeqMax = 0;
    }
    return ;
}

NFCSTATUS
phNciNfc_Reset(void*                        pNciHandle,
               phNciNfc_NciReset_t          eNciResetType,
               pphNciNfc_IfNotificationCb_t pResetCb,
               void*                        pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_ResetType_t         eNfccResetType;
    pphNciNfc_Context_t pNciContext = phNciNfc_GetContext();
    UNUSED(pNciHandle);
    /*Dont rely on Ncihandle passed, this a priority call*/
    /*Check if internally NciHandle is availble*/
    PH_LOG_NCI_FUNC_ENTRY();
    if (pNciContext != NULL)
    {
        switch(eNciResetType)
        {
            case phNciNfc_NciReset_DeInit_KeepConfig:
                PH_LOG_NCI_INFO_STR("Nci Reset - phNciNfc_NciReset_DeInit_KeepConfig");
                (void )phNciNfc_CoreRecvMgrRelease(&pNciContext->NciCoreContext);
                (void )phNciNfc_CoreResetSenderStateMachine(&pNciContext->NciCoreContext);
                (void )phTmlNfc_WriteAbort(pNciContext->NciCoreContext.pHwRef);
                eNfccResetType = phNciNfc_ResetType_KeepConfig;
                /* Send reset command to Nfcc and if success, release Nci handle */
                wStatus = phNciNfc_Release(pNciContext,pResetCb,pContext,eNfccResetType);
                break;
            case phNciNfc_NciReset_DeInit_ResetConfig:
                PH_LOG_NCI_INFO_STR("Nci Reset - phNciNfc_NciReset_DeInit_ResetConfig");
                (void )phNciNfc_CoreRecvMgrRelease(&pNciContext->NciCoreContext);
                (void )phNciNfc_CoreResetSenderStateMachine(&pNciContext->NciCoreContext);
                (void )phTmlNfc_WriteAbort(pNciContext->NciCoreContext.pHwRef);
                eNfccResetType = phNciNfc_ResetType_ResetConfig;
                /* Send reset command to Nfcc and if success, release Nci handle */
                wStatus = phNciNfc_Release(pNciContext,pResetCb,pContext,eNfccResetType);
                break;
            case phNciNfc_NciReset_ResetNfcc_KeepConfig:
                PH_LOG_NCI_INFO_STR("Nci Reset - phNciNfc_NciReset_ResetNfcc_KeepConfig");
                eNfccResetType = phNciNfc_ResetType_KeepConfig;
                wStatus = phNciNfc_ResetNfcc(pNciContext,eNfccResetType,
                                    pResetCb,pContext);
                break;
            case phNciNfc_NciReset_ResetNfcc_ResetConfig:
                PH_LOG_NCI_INFO_STR("Nci Reset - phNciNfc_NciReset_ResetNfcc_ResetConfig");
                eNfccResetType = phNciNfc_ResetType_ResetConfig;
                wStatus = phNciNfc_ResetNfcc(pNciContext,eNfccResetType,
                                    pResetCb,pContext);
                break;
            case phNciNfc_NciReset_Mgt_Reset:
                PH_LOG_NCI_INFO_STR("Nci Reset - phNciNfc_NciReset_Mgt_Reset");
                (void )phNciNfc_CoreResetSenderStateMachine(&pNciContext->NciCoreContext);
                /* Reelase Nci handle without reseting Nfcc */
                wStatus = phNciNfc_ReleaseNciHandle();
                break;
            case phNciNfc_NciReset_ClearNci:
                PH_LOG_NCI_INFO_STR("Nci Reset - phNciNfc_NciReset_ClearNci");
                /* Just clear/reset the content of Nci context structure */
                phNciNfc_ClearNciContext(pNciContext);
                wStatus = NFCSTATUS_SUCCESS;
                break;
            default:
                PH_LOG_NCI_WARN_STR("Invalid Nci Reset type");
                break;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void phNciNfc_EnterNciRecoveryMode(void *pNciCtx)
{
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciCtx;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        (void )phNciNfc_CoreResetSenderStateMachine(&pNciContext->NciCoreContext);
        (void)phOsalNfc_Timer_Stop(pNciContext->dwNtfTimerId);
    }
    PH_LOG_NCI_FUNC_EXIT();
}

NFCSTATUS phNciNfc_GetNfccFeatures(void *pNciCtx,
                                   pphNciNfc_NfccFeatures_t pNfccFeatures)
{
    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciCtx;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciContext) || (pNciContext != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL != pNfccFeatures)
    {
        /* Get NFCC features from Init response message */
        pNfccFeatures->DiscConfigInfo.DiscFreqConfig = (pNciContext->InitRspParams.NfccFeatures.DiscConfSuprt &
                                                        PHNCINFC_DISCFREQCONFIG_BITMASK);
        pNfccFeatures->DiscConfigInfo.DiscConfigMode = ((pNciContext->InitRspParams.NfccFeatures.DiscConfSuprt &
                                                        PHNCINFC_DISCCONFIGMODE_BITMASK) >> 1);
        pNfccFeatures->RoutingInfo.TechnBasedRouting = ((pNciContext->InitRspParams.NfccFeatures.RoutingType &
                                                        PHNCINFC_TECHNBASEDRTNG_BITMASK) >> 1);
        pNfccFeatures->RoutingInfo.ProtocolBasedRouting = ((pNciContext->InitRspParams.NfccFeatures.RoutingType &
                                                        PHNCINFC_PROTOBASEDRTNG_BITMASK) >> 2);
        pNfccFeatures->RoutingInfo.AidBasedRouting = ((pNciContext->InitRspParams.NfccFeatures.RoutingType &
                                                        PHNCINFC_AIDBASEDRTNG_BITMASK) >> 3);
        pNfccFeatures->PowerStateInfo.BatteryOffState = (pNciContext->InitRspParams.NfccFeatures.PwrOffState &
                                                        PHNCINFC_BATTOFFSTATE_BITMASK);
        pNfccFeatures->PowerStateInfo.SwitchOffState = ((pNciContext->InitRspParams.NfccFeatures.PwrOffState &
                                                        PHNCINFC_SWITCHOFFSTATE_BITMASK) >> 1);
        /* Store the Manufacturer ID */
        pNfccFeatures->ManufacturerId = pNciContext->InitRspParams.ManufacturerId;
        /* Store the Manufacturer specific info */
        pNfccFeatures->ManufactureInfo.Length = pNciContext->InitRspParams.ManufacturerInfo.Length;
        pNfccFeatures->ManufactureInfo.Buffer = pNciContext->InitRspParams.ManufacturerInfo.Buffer;

        /* Store the version Number */
        pNfccFeatures->NciVer = pNciContext->ResetInfo.NciVer;
        /* Store the maximum routing table size */
        pNfccFeatures->RoutingTableSize = pNciContext->InitRspParams.RoutingTableSize;
        wStatus = NFCSTATUS_SUCCESS;
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameters!");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_SetConfigRaw(
                            void*                      pNciHandle,
                            uint8_t *pBuff, uint16_t Length,
                            pphNciNfc_IfNotificationCb_t pConfigRfNotifyCb,
                            void*                      pContext)
{
    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciHandle) || (pNciHandle != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL != pBuff) && (0 != Length))
    {
        pNciContext->tSendPayload.pBuff = (uint8_t *) phOsalNfc_GetMemory((uint32_t)Length);
        phOsalNfc_MemCopy(pNciContext->tSendPayload.pBuff, pBuff,Length);
        /* Input parameters are valid and payload field has been framed successfully */
        phNciNfc_SetUpperLayerCallback(pNciContext, pConfigRfNotifyCb, pContext);
        PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_SetConfigSequence);
        pNciContext->tSendPayload.wPayloadSize = Length;
        wStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
        if(NFCSTATUS_PENDING != wStatus)
        {
            PH_LOG_NCI_CRIT_STR("Set Config Raw Sequence failed!");
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_IsoDepPresenceChk(void* pNciHandle,
                           pphNciNfc_IfNotificationCb_t pIsoDepPresChkCb,
                           void* pContext)
{
    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciHandle) || (pNciHandle != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL == pIsoDepPresChkCb)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        phNciNfc_SetUpperLayerCallback(pNciContext, pIsoDepPresChkCb, pContext);
        PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_IsoDepPresChkSequence);

        wStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
        if(NFCSTATUS_PENDING != wStatus)
        {
            PH_LOG_NCI_CRIT_STR("IsoDep Presence check command failed!");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_SetPowerSubState(void* pNciHandle,
                          phNciNfc_PowerSubState_t bSubState,
                          pphNciNfc_IfNotificationCb_t pSetPowerSubStateCb,
                          void* pContext)
{
    NFCSTATUS               wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t     pNciContext = (pphNciNfc_Context_t )pNciHandle;
    uint8_t                 *pTargetInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciHandle) || (pNciHandle != phNciNfc_GetContext()))
    {
        PH_LOG_NCI_CRIT_STR("Stack not initialized");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL == pSetPowerSubStateCb)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pTargetInfo = (uint8_t *)phOsalNfc_GetMemory(PHNCINFC_SETPOWERSUBSTATE_PAYLOADLEN);
        if(NULL != pTargetInfo)
        {
            phNciNfc_SetUpperLayerCallback(pNciContext, pSetPowerSubStateCb, pContext);
            /* Fill the buffer details */
            pTargetInfo[0] = (uint8_t)bSubState;
            /* Store the payload info in the context */
            pNciContext->tSendPayload.pBuff = pTargetInfo;
            pNciContext->tSendPayload.wPayloadSize = PHNCINFC_SETPOWERSUBSTATE_PAYLOADLEN;
            PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_SetPowerSubStateSequence);

            wStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
            if(NFCSTATUS_PENDING != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Set Power Substate sequence failed!");
                phOsalNfc_FreeMemory(pTargetInfo);
                pNciContext->tSendPayload.pBuff = NULL;
                pNciContext->tSendPayload.wPayloadSize = 0x00;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Memory not available");
            wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void phNciNfc_AbortDataTransfer(void *pNciHandle)
{
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t )pNciHandle;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciHandle) && (pNciHandle == phNciNfc_GetContext()))
    {
        /* Invalidate the Transceive call back function and Remote device receive call back
        function incase of P2P*/
        pNciContext->tTranscvCtxt.pNotify = NULL;
        pNciContext->tTranscvCtxt.pContext = NULL;
        pNciContext->IfNtf = NULL;
        pNciContext->IfNtfCtx = NULL;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return;
}

void phNciNfc_PrepareNciShutDown(void* pNciHandle)
{
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t )pNciHandle;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciHandle) && (pNciHandle == phNciNfc_GetContext()))
    {
        /* Invalidate upper layer call back function if any already exists */
        pNciContext->tTranscvCtxt.pNotify = NULL;
        pNciContext->tTranscvCtxt.pContext = NULL;
        pNciContext->IfNtf = NULL;
        pNciContext->IfNtfCtx = NULL;
        pNciContext->pSeqHandler = NULL;
        pNciContext->SeqNext = 0;
        pNciContext->SeqMax = 0;
        (VOID)phNciNfc_CoreResetSenderStateMachine((void *) &pNciContext->NciCoreContext);
        phNciNfc_CoreRecvMgrDeRegisterAll((void *) &pNciContext->NciCoreContext);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return;
}

static
NFCSTATUS
phNciNfc_Release(
                 void*                        pNciHandle,
                 pphNciNfc_IfNotificationCb_t pReleaseCb,
                 void*                        pContext,
                 phNciNfc_ResetType_t         eResetType)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t )pNciHandle;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciContext)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(pNciContext != phNciNfc_GetContext())
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(NULL == pReleaseCb)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        phNciNfc_SetUpperLayerCallback(pNciContext, pReleaseCb, pContext);

        PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_ReleaseSequence);
        pNciContext->ResetInfo.ResetTypeReq = eResetType;
        wStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
        if(NFCSTATUS_PENDING != wStatus)
        {
            wStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("Failed to Reset!");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ResetNfcc(
                 void*                        pNciHandle,
                 phNciNfc_ResetType_t         eResetType,
                 pphNciNfc_IfNotificationCb_t pResetNfccCb,
                 void*                        pContext
                )
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t )pNciHandle;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciContext)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(pNciContext != phNciNfc_GetContext())
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(NULL == pResetNfccCb)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        phNciNfc_SetUpperLayerCallback(pNciContext, pResetNfccCb, pContext);
        PHNCINFC_INIT_SEQUENCE(pNciContext,gphNciNfc_NfccResetSequence);
        pNciContext->ResetInfo.ResetTypeReq = eResetType;
        wStatus = phNciNfc_GenericSequence((void *)pNciContext, NULL, NFCSTATUS_SUCCESS);
        if(NFCSTATUS_PENDING != wStatus)
        {
            wStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("Failed to Reset!");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void
phNciNfc_ReleaseNfceeCntx(void )
{
    pphNciNfc_Context_t pNciCtx = phNciNfc_GetContext();
    uint8_t bIndex = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    if (NULL != pNciCtx)
    {
        for (bIndex = 0; (bIndex < PH_NCINFC_NFCEE_DEVICE_MAX) && (pNciCtx->tNfceeContext.bNfceeCount > 0); bIndex++) {
            if(0 != pNciCtx->tNfceeContext.pNfceeDevInfo[bIndex].tDevInfo.bNfceeID) {
                if (NULL != pNciCtx->tNfceeContext.pNfceeDevInfo[bIndex].tDevInfo.pTlvInfo) {
                    phOsalNfc_FreeMemory(pNciCtx->tNfceeContext.pNfceeDevInfo[bIndex].tDevInfo.pTlvInfo);
                    pNciCtx->tNfceeContext.pNfceeDevInfo[bIndex].tDevInfo.pTlvInfo = NULL;
                }
                phOsalNfc_SetMemory(&pNciCtx->tNfceeContext.pNfceeDevInfo[bIndex].tDevInfo,0,sizeof(phNciNfc_DeviceInfo_t));
                pNciCtx->tNfceeContext.bNfceeCount--;
            }
        }
    }

    PH_LOG_NCI_FUNC_EXIT();
}

NFCSTATUS
phNciNfc_ReleaseNciHandle(void )
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t pNciCtx = phNciNfc_GetContext();

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pNciCtx)
    {
        (void)phOsalNfc_Timer_Delete(pNciCtx->dwNtfTimerId);
        pNciCtx->dwNtfTimerId = PH_OSALNFC_TIMER_ID_INVALID;

        /* Release Nci resources and Release Nci Handle (Nci context structure) */
        /* Release Memory related to Discovery module */
        /* Update the DeActivation type to Idle Mode */
        pNciCtx->NciDiscContext.eDeActvType = phNciNfc_e_IdleMode;
        (void )phNciNfc_ProcessDeActvState(pNciCtx);
        /* Release Core and its sub-modules */
        wStatus = phNciNfc_CoreRelease(&pNciCtx->NciCoreContext);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Nci Release invoked - Release Nci Context handle");

            if (NULL != pNciCtx->tTranscvCtxt.tSendPld.pBuff) {
                phOsalNfc_FreeMemory(pNciCtx->tTranscvCtxt.tSendPld.pBuff);
                pNciCtx->tTranscvCtxt.tSendPld.pBuff = NULL;
                pNciCtx->tTranscvCtxt.tSendPld.wLen = 0;
            }
            if (NULL != pNciCtx->tTranscvCtxt.tRecvPld.pBuff) {
                phOsalNfc_FreeMemory(pNciCtx->tTranscvCtxt.tRecvPld.pBuff);
                pNciCtx->tTranscvCtxt.tRecvPld.pBuff = NULL;
                pNciCtx->tTranscvCtxt.tRecvPld.wLen = 0;
            }

            if (NULL != pNciCtx->tSendPayload.pBuff) {
                phOsalNfc_FreeMemory(pNciCtx->tSendPayload.pBuff);
                pNciCtx->tSendPayload.pBuff = NULL;
                pNciCtx->tSendPayload.wPayloadSize = 0;
            }

            if (NULL != pNciCtx->InitRspParams.ManufacturerInfo.Buffer) {
                phOsalNfc_FreeMemory(pNciCtx->InitRspParams.ManufacturerInfo.Buffer);
                pNciCtx->InitRspParams.ManufacturerInfo.Buffer = NULL;
                pNciCtx->InitRspParams.ManufacturerInfo.Length = 0;
            }

            phNciNfc_ReleaseNfceeCntx();

            phOsalNfc_FreeMemory(pNciCtx);
            phNciNfc_SetCoreContext(NULL);
            phNciNfc_SetContext(NULL);
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("Nci Core release failed!");
        }
    }
    else
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_SeEventCb(void* pContext, void *pInfo, NFCSTATUS wStatus)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    pphNciNfc_Context_t pNciContext = phNciNfc_GetContext();
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    phNciNfc_TransactInfo_t tTransInfo;
    pphNciNfc_RegDataCb_t pUpperLayerCb = NULL;
    uint8_t bCount = 0;
    void* pSeHandle = pContext;
    void* pUpperLayerCtx;

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL == pNciContext)
    {
        PH_LOG_NCI_CRIT_STR("NciContext is NULL.");
        status = NFCSTATUS_INVALID_PARAMETER;
    }
    else if (phNciNfc_IsVersion1x(pNciContext) && NULL == pSeHandle)
    {
        // In NCI1.0 NULL is invalid value for pSeHandle .
        PH_LOG_NCI_CRIT_STR("pSeHandle is NULL while operating in NCI1x mode.");
        status = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        if (NULL == pTransInfo)
        {
            pTransInfo = &tTransInfo;
            pTransInfo->pbuffer = NULL;
            pTransInfo->wLength = 0;
        }

        for(bCount = 0; bCount < PHNCINFC_MAX_SE_EVENT_REGS; bCount++)
        {
            if((1 == pNciContext->tSeEventList.aSeEventList[bCount].bEnable) &&
                (pSeHandle == pNciContext->tSeEventList.aSeEventList[bCount].pSeHandle))
            {
                pUpperLayerCtx = pNciContext->tSeEventList.aSeEventList[bCount].pUpperLayerCtx;
                pUpperLayerCb = pNciContext->tSeEventList.aSeEventList[bCount].pUpperLayerCb;
                if(NULL != pUpperLayerCb)
                {
                    pTransInfo->pContext = pSeHandle;
                    pUpperLayerCb(pUpperLayerCtx,(void *)pTransInfo,wStatus);
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Found matching entry in tSeEventList, but pUpperLayerCb is NULL.");
                    status = NFCSTATUS_FAILED;
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS phLibNfc_GetAvailableSlotIndex(phNciNfc_SeEventList_t *pSeEventList,
                                         uint8_t *pSlotIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bCount;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pSeEventList) && (NULL != pSlotIndex))
    {
        for(bCount = 0; bCount < PHNCINFC_MAX_SE_EVENT_REGS; bCount++)
        {
            if(0 == pSeEventList->aSeEventList[bCount].bEnable)
            {
                /* Available free slot identified */
                (*pSlotIndex) = bCount;
                wStatus = NFCSTATUS_SUCCESS;
                break;
            }
        }
        if(NFCSTATUS_SUCCESS != wStatus)
        {
            /* No Free slot available, return fails */
            wStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("phLibNfc_GetAvailableSlotIndex: No Free slot available");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_GetRegisteredSlotIndex(phNciNfc_SeEventList_t *pSeEventList,
                                          void *pSeHandle,
                                         uint8_t *pSlotIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bCount;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pSeEventList) && (NULL != pSlotIndex) && (NULL != pSeHandle))
    {
        for(bCount = 0; bCount < PHNCINFC_MAX_SE_EVENT_REGS; bCount++)
        {
            if(1 == pSeEventList->aSeEventList[bCount].bEnable)
            {
                /* Identify the registration using SeHandle */
                if(pSeHandle == pSeEventList->aSeEventList[bCount].pSeHandle)
                {
                    (*pSlotIndex) = bCount;
                    wStatus = NFCSTATUS_SUCCESS;
                    break;
                }
            }
        }
        if(NFCSTATUS_SUCCESS != wStatus)
        {
            /* No Free slot available, return fails */
            wStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("phLibNfc_GetAvailableSlotIndex: Registration not found");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_SendIsoDepPresChkCmd(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreTxInfo_t tTxInfo;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t) pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        /* Clear the Tx information structure */
        phOsalNfc_SetMemory(&tTxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));

        /* Update Tx info structure */
        tTxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        tTxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
        tTxInfo.tHeaderInfo.Opcode_ID.OidType.RfMgtCmdOid = phNciNfc_e_RfMgtRfIsoDepPresChkCmdOid;
        tTxInfo.Buff = NULL;
        tTxInfo.wLen = 0;

        /* Sending command */
        wStatus = phNciNfc_CoreIfTxRx(&(pNciCtx->NciCoreContext), &tTxInfo,
                        &(pNciCtx->RspBuffInfo),PHNCINFC_NCI_CMD_RSP_TIMEOUT,
                        (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                        (void *)pNciCtx);
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessIsoDepPresChkRsp(void *pContext, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if(1 == pNciContext->RspBuffInfo.wLen)
            {
                /*Check Status Byte*/
                if(pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
                {
                    PH_LOG_NCI_INFO_STR("Presence check extension command accepted by NFCC");
                    wStatus = NFCSTATUS_SUCCESS;
                }
                else if(pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_REJECTED)
                {
                    PH_LOG_NCI_WARN_STR("Presence check extension command rejected by NFCC");
                    wStatus = NFCSTATUS_REJECTED;
                }
                else if(pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_SEMANTIC_ERROR)
                {
                    PH_LOG_NCI_WARN_STR("Presence check extension semantic error received");
                    wStatus = NFCSTATUS_FAILED;
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Presence check extension response received with invalid payload length");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Presence check extension response received with failure status");
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_IsoDepChkPresNtfCb(void* pContext, void *pInfo, NFCSTATUS status)
{
    NFCSTATUS wStatus = status;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t )pContext;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    NFCSTATUS wDeRegNtf = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreRegInfo_t tNtfInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciCtx) && (NULL != pTransInfo))
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            (void)phOsalNfc_Timer_Stop(pNciCtx->dwNtfTimerId);

            /* Validate the length of the notification received */
            if(1 == pTransInfo->wLength)
            {
                /*Check Status Byte*/
                if(pNciCtx->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
                {
                    PH_LOG_NCI_INFO_STR("Iso-Dep Presence check extension command accepted by NFCC");
                    wStatus = NFCSTATUS_SUCCESS;
                }
                else if(pNciCtx->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_SEMANTIC_ERROR)
                {
                    PH_LOG_NCI_WARN_STR("Iso-Dep Presence check extension semantic error received");
                    wStatus = NFCSTATUS_FAILED;
                }
                else
                {
                    wStatus = NFCSTATUS_TARGET_LOST;
                }
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Iso-Dep pres chk ntf: Invalid length of payload");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Iso-Dep pres chk ntf: Receiption failed");
            wStatus = NFCSTATUS_FAILED;
        }

        /* Deregister Iso-Dep pres chk ntf call back */
        tNtfInfo.bGid = phNciNfc_e_CoreRfMgtGid;
        tNtfInfo.bOid = phNciNfc_e_RfMgtRfIsoDepPresChkNtfOid;
        tNtfInfo.pContext = (void *)pNciCtx;
        tNtfInfo.pNotifyCb = (pphNciNfc_CoreIfNtf_t)&phNciNfc_IsoDepChkPresNtfCb;
        wDeRegNtf = phNciNfc_CoreRecvMgrDeRegisterCb((void*)&pNciCtx->NciCoreContext, &tNtfInfo,
                                                     phNciNfc_e_NciCoreMsgTypeCntrlNtf);
        if(NFCSTATUS_SUCCESS == wDeRegNtf)
        {
            PH_LOG_NCI_INFO_STR("De-register pres chk extension ntf call back success");
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("De-register pres chk extension ntf call back failed!");
        }

        /* Notify upper layer */
        phNciNfc_Notify(pNciCtx, wStatus,NULL);
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Iso-Dep pres chk : Invalid input parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void phNciNfc_IsoDepChkPresNtfTimeoutHandler(uint32_t TimerId, void *pContext)
{
    pphNciNfc_Context_t pNciContext = pContext;
    phNciNfc_TransactInfo_t tTransInfo = {NULL,NULL,0x00};

    UNUSED(TimerId)
    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL != pNciContext)
    {
        (void)phOsalNfc_Timer_Stop(pNciContext->dwNtfTimerId);
    }

    /* Invoke notification callback function with fail status */
    (void)phNciNfc_IsoDepChkPresNtfCb(pNciContext,&tTransInfo,NFCSTATUS_FAILED);
    PH_LOG_NCI_FUNC_EXIT();
}

static NFCSTATUS phNciNfc_CompleteIsoDepPresChkSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    phNciNfc_sCoreHeaderInfo_t tRegInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        /* Is Nfcc returns success, register for Iso-Dep pres chk notification */
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Registering Iso-Dep pres chk notification */
            tRegInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            tRegInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
            tRegInfo.Opcode_ID.Val = phNciNfc_e_RfMgtRfIsoDepPresChkNtfOid;
            wStatus = phNciNfc_CoreIfRegRspNtf((void*)&pNciCtx->NciCoreContext,
                                   &tRegInfo,
                                   (pphNciNfc_CoreIfNtf_t)&phNciNfc_IsoDepChkPresNtfCb,
                                   (void *)pNciCtx);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                if(PH_OSALNFC_TIMER_ID_INVALID != pNciCtx->dwNtfTimerId)
                {
                    wStatus = phOsalNfc_Timer_Start(pNciCtx->dwNtfTimerId,
                                                    PHNCINFC_ISODEP_PRESCHK_NTF_TIMEROUT,
                                                    &phNciNfc_IsoDepChkPresNtfTimeoutHandler,
                                                    (void *) pNciCtx);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        PH_LOG_NCI_INFO_STR("IsoDep Chk Pres ntf timer started");
                    }
                    else
                    {
                        PH_LOG_NCI_CRIT_STR("IsoDep Chk Pres ntf timer start FAILED");
                    }
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Timer Create had failed");
                }

            }
            else if(NFCSTATUS_ALREADY_REGISTERED == wStatus)
            {
                PH_LOG_NCI_INFO_STR("IsoDep Check presence extension notification already registered");
                wStatus = NFCSTATUS_FAILED;
            }
            else if(NFCSTATUS_FAILED == wStatus)
            {
                PH_LOG_NCI_CRIT_STR("IsoDep Check presence extension notification registration failed!");
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Invalid parameter sent");
                wStatus = NFCSTATUS_FAILED;
            }
        }

        if(NFCSTATUS_SUCCESS != wStatus)
        {
            phNciNfc_Notify(pNciCtx, wStatus, NULL);
        }
        else
        {
            PH_LOG_NCI_INFO_STR("Waiting for IsoDep Check presence notification...");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_SendSetPowerSubStateCmd(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreTxInfo_t tTxInfo;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t) pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        /* Clear the Tx information structure */
        phOsalNfc_SetMemory(&tTxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));

        /* Update Tx info structure */
        tTxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        tTxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
        tTxInfo.tHeaderInfo.Opcode_ID.OidType.NciCoreCmdOid = phNciNfc_e_NciCoreSetPowerSubStateCmdOid;
        tTxInfo.Buff = (uint8_t *)pNciCtx->tSendPayload.pBuff;
        tTxInfo.wLen = pNciCtx->tSendPayload.wPayloadSize;

        /* Sending command */
        wStatus = phNciNfc_CoreIfTxRx(&(pNciCtx->NciCoreContext), &tTxInfo,
                        &(pNciCtx->RspBuffInfo),PHNCINFC_NCI_CMD_RSP_TIMEOUT,
                        (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                        (void *)pNciCtx);
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessSetPowerSubStateRsp(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if(1 == pNciContext->RspBuffInfo.wLen)
            {
                /*Check Status Byte*/
                if(pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
                {
                    PH_LOG_NCI_INFO_STR("Set Power substate command accepted by NFCC");
                    wStatus = NFCSTATUS_SUCCESS;
                }
                else
                {
                    PH_LOG_NCI_INFO_STR("Set Power substate command failed");
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Set Power substate response received with invalid payload length");
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CompleteSetPowerSubStateSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        if(NULL != pNciCtx->tSendPayload.pBuff)
        {
            phOsalNfc_FreeMemory(pNciCtx->tSendPayload.pBuff);
            pNciCtx->tSendPayload.pBuff = NULL;
            pNciCtx->tSendPayload.wPayloadSize = 0;
        }
        phNciNfc_Notify(pNciCtx, wStatus, NULL);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
