/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Init.tmh"

static NFCSTATUS phLibNfc_Initialize(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_InitializeProcess(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_InitSetConfig(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_InitSetConfigProcess(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_GetT3tMaxValue(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_GetT3tMaxValueProc(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_InitSetMapping(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_InitSetMappingProcess(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_SetLsntModeRtng(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_SetLstnModeRtngProcess(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_InitializeComplete(void* pContext,NFCSTATUS status,void* pInfo);

static NFCSTATUS phLibNfc_GetNfccFeatures(void *pNciHandle);
static void phLibNfc_ReleaseLibNfcContext(void *pContext, NFCSTATUS wStatus);

/**
    Note: Adding/Removing sequences must be made accounting for references elsewhere
 */
phLibNfc_Sequence_t gphLibNfc_InitializeSequence[] = {
    {&phLibNfc_Initialize, &phLibNfc_InitializeProcess},
    {&phLibNfc_GetT3tMaxValue, &phLibNfc_GetT3tMaxValueProc}, /* Get T3T Max value */
    {&phLibNfc_InitSetConfig, &phLibNfc_InitSetConfigProcess}, /* Configure Standard NFCC parameters */
    {&phLibNfc_InitSetMapping, &phLibNfc_InitSetMappingProcess}, /*Setup interface mapping */
    {&phLibNfc_SetLsntModeRtng, &phLibNfc_SetLstnModeRtngProcess}, /*Set all static routing configuration */
    {NULL, &phLibNfc_InitializeComplete},
};

static NFCSTATUS phLibNfc_InitCb(void* pContext,NFCSTATUS wStatus,void* pInfo)
{
    pphLibNfc_LibContext_t      pLibContext = NULL;
    pphNciNfc_Context_t         pNciContext = NULL;
    pphHciNfc_HciContext_t      pHciContext = NULL;
    pphNciNfc_TransactInfo_t pTransactInfo = (pphNciNfc_TransactInfo_t)pInfo;
    NFCSTATUS tempStatus = wStatus;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    /* Initialize the local variable */
    pLibContext  = (pphLibNfc_LibContext_t)pContext;

    if((NULL != pLibContext) && (phLibNfc_GetContext() == pLibContext))
    {
        if(NFCSTATUS_SUCCESS == tempStatus)
        {
            if(NULL != pInfo)
            {
                pLibContext->sHwReference.pNciHandle = pTransactInfo->pContext;
                pNciContext = pTransactInfo->pContext;

                wStatus = phLibNfc_GetNfccFeatures(pNciContext);
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    /*Register for Tag discovery*/
                    wStatus = phNciNfc_RegisterNotification(
                        pNciContext,
                        phNciNfc_e_RegisterTagDiscovery,
                        &phLibNfc_NotificationRegister_Resp_Cb,
                        pLibContext);

                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        /*Register for Rf Deactivate notification*/
                        wStatus = phNciNfc_RegisterNotification(
                                pNciContext,
                                phNciNfc_e_RegisterRfDeActivate,
                                &phLibNfc_DeActvNtfRegister_Resp_Cb,
                                pLibContext);
                    }

                    /* Register for SE notification */
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        wStatus = phNciNfc_RegisterNotification(
                                pNciContext,\
                                phNciNfc_e_RegisterSecureElement,\
                                &phLibNfc_SENtfHandler,\
                                pLibContext);
                    }

                    /* Register for Generic error notification */
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        wStatus = phNciNfc_RegisterNotification(
                                pNciContext,\
                                phNciNfc_e_RegisterGenericError,\
                                &phLibNfc_GenericErrorHandler,\
                                pLibContext);
                    }

                    /* Register for Reset notification */
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        wStatus = phNciNfc_RegisterNotification(
                                pNciContext,\
                                phNciNfc_e_RegisterReset,\
                                &phLibNfc_ResetNtfHandler,\
                                pLibContext);
                    }

                    /*The Static HCI Connection exists after NFCC initialization without needing to be
                    *created using the connection Control Messages defined in Section 4.4.2 and is never closed
                    */
                    if (pLibContext->pHciContext == NULL &&
                        !phNciNfc_IsVersion1x(pNciContext) &&
                        pNciContext->InitRspParams.DataHCIPktPayloadLen > 0)
                    {
                        pHciContext = (phHciNfc_HciContext_t*)phOsalNfc_GetMemory(sizeof(phHciNfc_HciContext_t));
                        if (pHciContext == NULL)
                        {
                            /* Failed to allocate memory for HCI packet */
                            wStatus = NFCSTATUS_FAILED;
                        }
                        else
                        {
                            pLibContext->pHciContext = pHciContext;
                            phOsalNfc_SetMemory(pHciContext, 0, sizeof(phHciNfc_HciContext_t));
                            pHciContext->pNciContext = pNciContext;
                            pHciContext->bLogDataMessages = !!pLibContext->Config.bLogNciDataMessages;

                            pLibContext->tSeInfo.bSeState[phLibNfc_SE_Index_HciNwk] = phLibNfc_SeStateInitializing;
                            pLibContext->sSeContext.pActiveSeInfo = (pphLibNfc_SE_List_t)(&pLibContext->tSeInfo.tSeList[phLibNfc_SE_Index_HciNwk]);
                            wStatus = phNciNfc_UpdateConnDestInfo(UNASSIGNED_DESTID, phNciNfc_e_NFCEE, NULL);
                        }
                    }
                }else
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }else
            {
                PH_LOG_LIBNFC_CRIT_STR("Buffer passed by lower layer is NULL");
                tempStatus = NFCSTATUS_FAILED;
            }
        }else
        {
            PH_LOG_LIBNFC_CRIT_STR("NFCSTATUS_FAILED passed by lower layer");
            tempStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid Libnfc context passed by lower layer");
        tempStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return tempStatus;
}

static NFCSTATUS phLibNfc_Initialize(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    phNciNfc_Config_t config = {0};
    PH_LOG_LIBNFC_FUNC_ENTRY();
    UNUSED(status);
    UNUSED(pInfo);

    config.bConfigOpt = (uint8_t)pCtx->Config.bConfigOpt;
    config.bLogDataMessages = (uint8_t)pCtx->Config.bLogNciDataMessages;
    wStatus=phNciNfc_Initialise(
        pCtx->sHwReference.pDriverHandle,
        &config,
        (pphNciNfc_IfNotificationCb_t) &phLibNfc_InternalSequence,pContext,
        phNciNfc_ResetType_KeepConfig
        );
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_InitializeProcess(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t) pContext;
    pphNciNfc_TransactInfo_t pTransactInfo = (pphNciNfc_TransactInfo_t)pInfo;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != pCtx) && (phLibNfc_GetContext() == pCtx))
    {
        wStatus = phLibNfc_InitCb(pContext,status,pInfo);
        if (wStatus == NFCSTATUS_SUCCESS)
        {
            pCtx->eConfigStatus = *((phNciNfc_ResetType_t *)pTransactInfo->pbuffer);

            if((pCtx->eInitType == phLibNfc_InitType_SkipConfig) ||
               (pCtx->eInitType == phLibNfc_InitType_Default && pCtx->eConfigStatus == phNciNfc_ResetType_KeepConfig))
            {
                phLibNfc_SkipSequenceSeq(pContext, gphLibNfc_InitializeSequence, 4);
            }
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid Libnfc context passed by lower layer");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_InitSetConfig(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t) pContext;
#ifdef NFC_LA_NFCID1
    uint8_t aNfcId1[NFC_LA_NFCID1_LEN] = NFC_LA_NFCID1_VALUE;
#endif
#ifdef NFC_PB_H_INFO
    uint8_t aInfo[NFC_PB_H_INFO_LEN] = NFC_PB_H_INFO_VALUE;
#endif
#ifdef NFC_PN_ATR_REQ_GEN_BYTES
    uint8_t aGenBytes[NFC_NFC_PN_ATR_REQ_GEN_BYTES_LEN] = NFC_NFC_PN_ATR_REQ_GEN_BYTES_VALUE;
#endif
#ifdef NFC_LB_NFCID0
    uint8_t aNfcId0[NFC_LB_NFCID0_LEN] = NFC_LB_NFCID0_VALUE;
#endif
#ifdef NFC_LB_APPLICATION_DATA
    uint8_t aAppData[NFC_LB_APPLICATION_DATA_LEN] = NFC_LB_APPLICATION_DATA_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_1
    uint8_t aT3TId1[NFC_LF_T3T_IDENTIFIERS_1_LEN] = NFC_LF_T3T_IDENTIFIERS_1_VALUE;
    uint8_t bT3tId1Len = (uint8_t) NFC_LF_T3T_IDENTIFIERS_1_LEN;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_2
    uint8_t aT3TId2[NFC_LF_T3T_IDENTIFIERS_2_LEN] = NFC_LF_T3T_IDENTIFIERS_2_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_3
    uint8_t aT3TId3[NFC_LF_T3T_IDENTIFIERS_3_LEN] = NFC_LF_T3T_IDENTIFIERS_3_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_4
    uint8_t aT3TId4[NFC_LF_T3T_IDENTIFIERS_4_LEN] = NFC_LF_T3T_IDENTIFIERS_4_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_5
    uint8_t aT3TId5[NFC_LF_T3T_IDENTIFIERS_15_LEN] = NFC_LF_T3T_IDENTIFIERS_5_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_6
    uint8_t aT3TId6[NFC_LF_T3T_IDENTIFIERS_6_LEN] = NFC_LF_T3T_IDENTIFIERS_6_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_7
    uint8_t aT3TId7[NFC_LF_T3T_IDENTIFIERS_7_LEN] = NFC_LF_T3T_IDENTIFIERS_7_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_8
    uint8_t aT3TId8[NFC_LF_T3T_IDENTIFIERS_8_LEN] = NFC_LF_T3T_IDENTIFIERS_8_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_9
    uint8_t aT3TId9[NFC_LF_T3T_IDENTIFIERS_9_LEN] = NFC_LF_T3T_IDENTIFIERS_9_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_10
    uint8_t aT3TId10[NFC_LF_T3T_IDENTIFIERS_10_LEN] = NFC_LF_T3T_IDENTIFIERS_10_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_11
    uint8_t aT3TId11[NFC_LF_T3T_IDENTIFIERS_11_LEN] = NFC_LF_T3T_IDENTIFIERS_11_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_12
    uint8_t aT3TId12[NFC_LF_T3T_IDENTIFIERS_12_LEN] = NFC_LF_T3T_IDENTIFIERS_12_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_13
    uint8_t aT3TId13[NFC_LF_T3T_IDENTIFIERS_13_LEN] = NFC_LF_T3T_IDENTIFIERS_13_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_14
    uint8_t aT3TId14[NFC_LF_T3T_IDENTIFIERS_14_LEN] = NFC_LF_T3T_IDENTIFIERS_14_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_15
    uint8_t aT3TId15[NFC_LF_T3T_IDENTIFIERS_15_LEN] = NFC_LF_T3T_IDENTIFIERS_15_VALUE;
#endif
#ifdef NFC_LF_T3T_IDENTIFIERS_16
    uint8_t aT3TId16[NFC_LF_T3T_IDENTIFIERS_16_LEN] = NFC_LF_T3T_IDENTIFIERS_16_VALUE;
#endif
#ifdef NFC_LF_T3T_PMM
    uint8_t aT3TPmm[NFC_LF_T3T_PMM_LEN] = NFC_LF_T3T_PMM_VALUE;
#endif
    phNciNfc_RfDiscConfigParams_t ConfigParams = {0};

    UNUSED(pContext);
    UNUSED(status);
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();

    ConfigParams.tConfigInfo.CommonConfig = 1;
    ConfigParams.tCommonDiscParams.ComnParamsConfig.Config.SetRfFieldInfo = 1;
    #if PH_LIBNFC_ENABLE_RFFIELD_INFO_NTF
        ConfigParams.tCommonDiscParams.bRfFieldInfo = 1;
    #else
        ConfigParams.tCommonDiscParams.bRfFieldInfo = 0;
    #endif
    #if PH_LIBNFC_ENABLE_RF_NFCEE_ACTION_NTF
        ConfigParams.tConfigInfo.CommonConfig = 1;
        ConfigParams.tCommonDiscParams.ComnParamsConfig.Config.SetRfNfceeAction = 1;
        ConfigParams.tCommonDiscParams.bRfNfceeAction = (uint8_t)pCtx->Config.bNfceeActionNtf;
    #endif
    #if PH_LIBNFC_ENABLE_NFCDEP_RTOX
        ConfigParams.tConfigInfo.CommonConfig = 1;
        ConfigParams.tCommonDiscParams.ComnParamsConfig.Config.SetNfcDepOperationParam = 1;
        ConfigParams.tCommonDiscParams.NfcDepOperationParam.bRtoxReq = 0;
        ConfigParams.tCommonDiscParams.NfcDepOperationParam.bAttentionCommand = 1;
        ConfigParams.tCommonDiscParams.NfcDepOperationParam.bInformationPdu = 1;
        ConfigParams.tCommonDiscParams.NfcDepOperationParam.bUseMaxTxLen = 1;
    #endif
    #ifdef NFC_LA_NFCID1
        ConfigParams.tConfigInfo.LstnNfcAConfig = 1;
        ConfigParams.tLstnNfcADiscParams.LstnNfcAConfig.Config.SetNfcID1 = 1;
        phOsalNfc_MemCopy(ConfigParams.tLstnNfcADiscParams.aNfcID1,\
                          aNfcId1,(uint32_t)NFC_LA_NFCID1_LEN);
        ConfigParams.tLstnNfcADiscParams.bNfcID1Size = (uint8_t)NFC_LA_NFCID1_LEN;
    #endif
    #ifdef NFC_PB_H_INFO
        if(NFC_PB_H_INFO_LEN <= PH_NCINFC_MAX_HIGHER_LAYER_INF_LEN)
        {
            ConfigParams.tConfigInfo.PollIsoDepConfig = 1;
            ConfigParams.tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetHigherLayerInfo = 1;
            phOsalNfc_MemCopy(ConfigParams.tPollIsoDepDiscParams.aHigherLayerInfo,\
                              aInfo,(uint32_t)NFC_PB_H_INFO_LEN);
            ConfigParams.tPollIsoDepDiscParams.bHigherLayerInfoSize = (uint8_t)NFC_PB_H_INFO_LEN;
        }
    #endif
    #ifdef NFC_PN_ATR_REQ_GEN_BYTES
        if(NFC_NFC_PN_ATR_REQ_GEN_BYTES_LEN <= PH_NCINFC_MAX_ATR_REQ_GEN_BYTES_LEN)
        {
            ConfigParams.tConfigInfo.PollNfcDepConfig = 1;
            ConfigParams.tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetGenBytes = 1;
            phOsalNfc_MemCopy(ConfigParams.tPollNfcDepDiscParams.aAtrReqGenBytes,\
                              aGenBytes,(uint32_t)NFC_NFC_PN_ATR_REQ_GEN_BYTES_LEN);
            ConfigParams.tPollNfcDepDiscParams.bAtrReqGeneBytesSize = (uint8_t)NFC_NFC_PN_ATR_REQ_GEN_BYTES_LEN;
        }
    #endif
    #ifdef NFC_LB_NFCID0
        if(NFC_LB_NFCID0_LEN <= PH_NCINFC_MAX_NFCID0_LEN)
        {
            ConfigParams.tConfigInfo.LstnNfcBConfig = 1;
            ConfigParams.tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetNfcID0 = 1;
            phOsalNfc_MemCopy(ConfigParams.tLstnNfcBDiscParams.aNfcID0,\
                              aNfcId0,(uint32_t)NFC_LB_NFCID0_LEN);
            ConfigParams.tLstnNfcBDiscParams.bNfcID0Size = (uint8_t)NFC_LB_NFCID0_LEN;
        }
    #endif
    #ifdef NFC_LB_APPLICATION_DATA
        if(NFC_LB_APPLICATION_DATA_LEN <= PH_NCINFC_APP_DATA_LEN)
        {
            ConfigParams.tConfigInfo.LstnNfcBConfig = 1;
            ConfigParams.tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAppData = 1;
            phOsalNfc_MemCopy(ConfigParams.tLstnNfcBDiscParams.aAppData,\
                              aAppData,(uint32_t)NFC_LB_APPLICATION_DATA_LEN);
        }
    #endif
    #ifdef NFC_LF_T3T_IDENTIFIERS_1
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 1)
            {
                if(bT3tId1Len == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[0],\
                                      aT3TId1,(uint32_t)NFC_LF_T3T_IDENTIFIERS_1_LEN);
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tFlags = 1;
                    ConfigParams.tLstnNfcFDiscParams.wT3tFlags |= 1;
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_1 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_2
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 2)
            {
                if(NFC_LF_T3T_IDENTIFIERS_2_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[1],\
                                      aT3TId2,(uint32_t)NFC_LF_T3T_IDENTIFIERS_2_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_2 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_3
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 3)
            {
                if(NFC_LF_T3T_IDENTIFIERS_3_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[2],\
                                      aT3TId3,(uint32_t)NFC_LF_T3T_IDENTIFIERS_3_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_3 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_4
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 4)
            {
                if(NFC_LF_T3T_IDENTIFIERS_4_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[3],\
                                      aT3TId4,(uint32_t)NFC_LF_T3T_IDENTIFIERS_4_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_4 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_5
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 5)
            {
                if(NFC_LF_T3T_IDENTIFIERS_5_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[4],\
                                      aT3TId5,(uint32_t)NFC_LF_T3T_IDENTIFIERS_5_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_5 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_6
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 6)
            {
                if(NFC_LF_T3T_IDENTIFIERS_6_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[5],\
                                      aT3TId6,(uint32_t)NFC_LF_T3T_IDENTIFIERS_6_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_6 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_7
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 7)
            {
                if(NFC_LF_T3T_IDENTIFIERS_7_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[6],\
                                      aT3TId7,(uint32_t)NFC_LF_T3T_IDENTIFIERS_7_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_7 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_8
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 8)
            {
                if(NFC_LF_T3T_IDENTIFIERS_8_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[7],\
                                      aT3TId8,(uint32_t)NFC_LF_T3T_IDENTIFIERS_8_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_8 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_9
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 9)
            {
                if(NFC_LF_T3T_IDENTIFIERS_9_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[8],\
                                      aT3TId9,(uint32_t)NFC_LF_T3T_IDENTIFIERS_9_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_9 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_10
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 10)
            {
                if(NFC_LF_T3T_IDENTIFIERS_10_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[9],\
                                      aT3TId10,(uint32_t)NFC_LF_T3T_IDENTIFIERS_10_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_10 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_11
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 11)
            {
                if(NFC_LF_T3T_IDENTIFIERS_11_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[10],\
                                      aT3TId11,(uint32_t)NFC_LF_T3T_IDENTIFIERS_11_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_11 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_12
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 12)
            {
                if(NFC_LF_T3T_IDENTIFIERS_12_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[11],\
                                      aT3TId12,(uint32_t)NFC_LF_T3T_IDENTIFIERS_12_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_12 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_13
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 13)
            {
                if(NFC_LF_T3T_IDENTIFIERS_13_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[12],\
                                      aT3TId13,(uint32_t)NFC_LF_T3T_IDENTIFIERS_13_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_13 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_14
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 14)
            {
                if(NFC_LF_T3T_IDENTIFIERS_14_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[13],\
                                      aT3TId14,(uint32_t)NFC_LF_T3T_IDENTIFIERS_14_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_14 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_15
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 15)
            {
                if(NFC_LF_T3T_IDENTIFIERS_15_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[14],\
                                      aT3TId15,(uint32_t)NFC_LF_T3T_IDENTIFIERS_15_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_15 */
    #ifdef NFC_LF_T3T_IDENTIFIERS_16
        if(pCtx->bT3tMax <= PH_NCINFC_MAX_NUM_T3T_IDS)
        {
            if(pCtx->bT3tMax >= 16)
            {
                if(NFC_LF_T3T_IDENTIFIERS_16_LEN == PH_NCINFC_T3TID_LEN)
                {
                    ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
                    ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 1;
                    phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tId[15],\
                                      aT3TId16,(uint32_t)NFC_LF_T3T_IDENTIFIERS_16_LEN);
                }
            }
        }
    #endif /* NFC_LF_T3T_IDENTIFIERS_16 */
    #ifdef NFC_LF_T3T_PMM
        if(NFC_LF_T3T_PMM_LEN == PH_NCINFC_T3TPMM_LEN)
        {
            ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
            ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tPmm = 1;
            phOsalNfc_MemCopy(ConfigParams.tLstnNfcFDiscParams.aT3tPmm,\
                              aT3TPmm,(uint32_t)NFC_LF_T3T_PMM_LEN);
        }
    #endif

    #ifdef NFC_LF_T3T_FLAGS
        if(NFC_LF_T3T_FLAGS_LEN == 0x02)
        {
            ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
            ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tFlags = 1;
            ConfigParams.tLstnNfcFDiscParams.wT3tFlags = NFC_LF_T3T_FLAGS_VALUE;
        }
    #endif
    #ifdef NFC_PB_AFI
        ConfigParams.tConfigInfo.PollNfcBConfig = 1;
        ConfigParams.tPollNfcBDiscParams.PollNfcBConfig.Config.SetAfi = 1;
        ConfigParams.tPollNfcBDiscParams.bAfi = (uint8_t)NFC_PB_AFI;
    #endif

    /* Poll Iso-Dep bit rate configuration */
    #ifdef NFC_PI_BIT_RATE
        ConfigParams.tConfigInfo.PollIsoDepConfig = 1;
        ConfigParams.tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetBitRate = 1;
        ConfigParams.tPollIsoDepDiscParams.bBitRate = (uint8_t)NFC_PI_BIT_RATE_SPEED;
    #endif

    /* Poll Nfc-Dep Data exchange bit rate configuration
       (0-Use max bit rate supported during data exchange
       1-use the same bit rate used during activation) */
    #ifdef NFC_PN_NFC_DEP_SPEED
        ConfigParams.tConfigInfo.PollNfcDepConfig = 1;
        ConfigParams.tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetSpeed = 1;
        ConfigParams.tPollNfcDepDiscParams.bNfcDepSpeed = (uint8_t)NFC_PN_NFC_DEP_SPEED_VALUE;
    #endif
    #ifdef NFC_PN_ATR_REQ_CONFIG
        ConfigParams.tConfigInfo.PollNfcDepConfig = 1;
        ConfigParams.tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetAtrConfig = 1;
        ConfigParams.tPollNfcDepDiscParams.AtrReqConfig.bDid = NFC_PN_ATR_REQ_DID;
        ConfigParams.tPollNfcDepDiscParams.AtrReqConfig.bLr = NFC_PN_ATR_REQ_LR;
    #endif
    #ifdef NFC_LA_BIT_FRAME_SDD
        ConfigParams.tConfigInfo.LstnNfcAConfig = 1;
        ConfigParams.tLstnNfcADiscParams.LstnNfcAConfig.Config.SetBitFrameSdd = 1;
        ConfigParams.tLstnNfcADiscParams.bBitFrameSDD = (uint8_t)NFC_LA_BIT_FRAME_SDD;
    #endif
    #ifdef NFC_LA_PLATFORM_CONFIG
        ConfigParams.tConfigInfo.LstnNfcAConfig = 1;
        ConfigParams.tLstnNfcADiscParams.LstnNfcAConfig.Config.SetPlatformConfig = 1;
        ConfigParams.tLstnNfcADiscParams.bPlatformConfig = (uint8_t)NFC_LA_PLATFORM_CONFIG;
    #endif
    #ifdef NFC_LB_SENSB_INFO
        ConfigParams.tConfigInfo.LstnNfcBConfig = 1;
        ConfigParams.tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSensBInfo = 1;
        ConfigParams.tLstnNfcBDiscParams.SensBInfo = (uint8_t)NFC_LB_SENSB_INFO;
    #endif
    #ifdef NFC_LB_SFGI
        ConfigParams.tConfigInfo.LstnNfcBConfig = 1;
        ConfigParams.tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSfgi = 1;
        ConfigParams.tLstnNfcBDiscParams.bSfgi = (uint8_t)NFC_LB_SFGI;
    #endif
    #ifdef NFC_LB_ADC_FO
        ConfigParams.tConfigInfo.LstnNfcBConfig = 1;
        ConfigParams.tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAdcFo = 1;
        ConfigParams.tLstnNfcBDiscParams.AdcFo.bDid = 1;
        ConfigParams.tLstnNfcBDiscParams.AdcFo.bAdcCodingField = 0x01;
    #endif
    #ifdef NFC_LF_CON_BITR_F
        ConfigParams.tConfigInfo.LstnNfcFConfig = 1;
        ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetConfigBitRate = 1;

        switch(NFC_LISTEN_F_BITRATE_SEL)
        {
        case BITRATE_LISTEN_F_212:
            ConfigParams.tLstnNfcFDiscParams.ConfigBitRate.bLstn212kbps = 1;
            ConfigParams.tLstnNfcFDiscParams.ConfigBitRate.bLstn424kbps = 0;
            break;
        case BITRATE_LISTEN_F_424:
            ConfigParams.tLstnNfcFDiscParams.ConfigBitRate.bLstn212kbps = 0;
            ConfigParams.tLstnNfcFDiscParams.ConfigBitRate.bLstn424kbps = 1;
            break;
        case BITRATE_LISTEN_F_212_424:
            ConfigParams.tLstnNfcFDiscParams.ConfigBitRate.bLstn212kbps = 1;
            ConfigParams.tLstnNfcFDiscParams.ConfigBitRate.bLstn424kbps = 1;
            break;
        default:
            /* Invalid bitrate selected */
            ConfigParams.tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetConfigBitRate = 0;
            break;
        }
    #endif
    #ifdef NFC_LI_FWI
        ConfigParams.tConfigInfo.LstnIsoDepConfig = 1;
        ConfigParams.tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetFwt = 1;
        ConfigParams.tLstnIsoDepDiscParams.bFrameWaitingTime = (uint8_t)NFC_LI_FWI;
    #endif
    #ifdef NFC_LI_BIT_RATE
        ConfigParams.tConfigInfo.LstnIsoDepConfig = 1;
        ConfigParams.tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetbBitRate = 1;
        ConfigParams.tLstnIsoDepDiscParams.bBitRate = (uint8_t)NFC_LI_BIT_RATE_SPEED;
    #endif

    #ifdef NFC_LN_WT
        ConfigParams.tConfigInfo.LstnNfcDepConfig = 1;
        ConfigParams.tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetWT = 1;
        ConfigParams.tLstnNfcDepDiscParams.bWaitingTime = (uint8_t)NFC_LN_WT;
    #endif

    wStatus = phNciNfc_SetConfigRfParameters(pCtx->sHwReference.pNciHandle,
                                            &ConfigParams,
                                            (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                            pContext);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_InitSetConfigProcess(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    UNUSED(pContext); UNUSED(status); UNUSED(pInfo);

    return wStatus;
}

static NFCSTATUS phLibNfc_GetT3tMaxValue(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    uint8_t aConfigInfo[] = {0x01, PHNCINFC_RFCONFIG_LF_T3T_MAX};
    UNUSED(status); UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = phNciNfc_GetConfigRaw(pCtx->sHwReference.pNciHandle,
                                    aConfigInfo,sizeof(aConfigInfo),
                                    (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                    pContext);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_GetT3tMaxValueProc(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    uint8_t *pConfigParam;
    pConfigParam = (uint8_t *)pInfo;
    UNUSED(status);
    if(NULL != pConfigParam)
    {
        pCtx->bT3tMax = 0;
        if(5 == pConfigParam[0])
        {
            if((0 == pConfigParam[1]) && (0x01 == pConfigParam[2]) &&
               (PHNCINFC_RFCONFIG_LF_T3T_MAX == pConfigParam[3]) && (1 == pConfigParam[4]))
            {
                pCtx->bT3tMax = pConfigParam[5];
            }
        }
        else if(1 == pConfigParam[0])
        {
            PH_LOG_LIBNFC_WARN_X32MSG("Error while reading T3T Max",pConfigParam[1]);
        }
    }
    return wStatus;
}

static NFCSTATUS phLibNfc_InitSetMapping(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    phNciNfc_MappingConfig_t ProtoIfMapping[7] = {0};
    uint8_t count = 0;
    UNUSED(status);
    UNUSED(pInfo);

    ProtoIfMapping[count].Mode.bPollMode = 1;
    ProtoIfMapping[count].Mode.bLstnMode = 1;
    ProtoIfMapping[count].tRfInterface = phNciNfc_e_RfInterfacesISODEP_RF;
    ProtoIfMapping[count].tRfProtocol = phNciNfc_e_RfProtocolsIsoDepProtocol;
    count++;

    ProtoIfMapping[count].Mode.bPollMode = 1;
    ProtoIfMapping[count].Mode.bLstnMode = 1;
    ProtoIfMapping[count].tRfInterface = phNciNfc_e_RfInterfacesNFCDEP_RF;
    ProtoIfMapping[count].tRfProtocol = phNciNfc_e_RfProtocolsNfcDepProtocol;
    count++;

    ProtoIfMapping[count].Mode.bPollMode = 1;
    ProtoIfMapping[count].Mode.bLstnMode = 0;
    ProtoIfMapping[count].tRfInterface = phNciNfc_e_RfInterfacesFrame_RF;
    ProtoIfMapping[count].tRfProtocol = phNciNfc_e_RfProtocolsT1tProtocol;
    count++;

    ProtoIfMapping[count].Mode.bPollMode = 1;
    ProtoIfMapping[count].Mode.bLstnMode = 0;
    ProtoIfMapping[count].tRfInterface = phNciNfc_e_RfInterfacesFrame_RF;
    ProtoIfMapping[count].tRfProtocol = phNciNfc_e_RfProtocolsT2tProtocol;
    count++;

    ProtoIfMapping[count].Mode.bPollMode = 1;
    ProtoIfMapping[count].Mode.bLstnMode = 0;
    ProtoIfMapping[count].tRfInterface = phNciNfc_e_RfInterfacesFrame_RF;
    ProtoIfMapping[count].tRfProtocol = phNciNfc_e_RfProtocolsT3tProtocol;
    count++;

    if(pCtx->tNfccFeatures.ManufacturerId == PH_LIBNFC_MANUFACTURER_NXP)
    {
        if(1 == pCtx->tADDconfig.PollDevInfo.PollCfgInfo.EnableKovio)
        {
            ProtoIfMapping[count].Mode.bPollMode = 1;
            ProtoIfMapping[count].Mode.bLstnMode = 0;
            ProtoIfMapping[count].tRfInterface = phNciNfc_e_RfInterfacesFrame_RF;
            ProtoIfMapping[count].tRfProtocol = phNciNfc_e_RfProtocolsKovioProtocol;
            count++;
        }

        ProtoIfMapping[count].Mode.bPollMode = 1;
        ProtoIfMapping[count].Mode.bLstnMode = 0;
        ProtoIfMapping[count].tRfInterface = phNciNfc_e_RfInterfacesTagCmd_RF;
        ProtoIfMapping[count].tRfProtocol = phNciNfc_e_RfProtocolsMifCProtocol;
        count++;
    }

    wStatus = phNciNfc_ConfigMapping(pCtx->sHwReference.pNciHandle,
                                     count,
                                     ProtoIfMapping,
                                     (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                     pContext);

    return wStatus;
}

static NFCSTATUS phLibNfc_InitSetMappingProcess(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    UNUSED(pContext); UNUSED(status); UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_SetLsntModeRtng(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    uint8_t bCount = 0;
    phNciNfc_RtngConfig_t tRtngConfig[6] = {0};

    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if (!pCtx->tNfccFeatures.RoutingInfo.ProtocolBasedRouting)
    {
        PH_LOG_LIBNFC_INFO_STR("Protocol routing not supported");
        wStatus = NFCSTATUS_FAILED;
    }
    else
    {
        /* Including Nfc-Dep routing which is required for P2P */
        tRtngConfig[bCount].Type = phNciNfc_e_LstnModeRtngProtocolBased;
        tRtngConfig[bCount].LstnModeRtngValue.tProtoBasedRtngValue.bRoute = 0;
        tRtngConfig[bCount].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOn = 0x01;
        tRtngConfig[bCount].LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol = phNciNfc_e_RfProtocolsNfcDepProtocol;
        bCount++;

        wStatus = phNciNfc_SetRtngTableConfig(pCtx->sHwReference.pNciHandle,\
            bCount,
            tRtngConfig,
            (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
            pCtx);
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_SetLstnModeRtngProcess(void* pContext, NFCSTATUS wStatus, void* pInfo)
{
    NFCSTATUS wModeStatus = wStatus;
    UNUSED(pInfo) ;UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wModeStatus;
}

static NFCSTATUS phLibNfc_InitializeComplete(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_LibContext_t pLibContext  = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphLibNfc_InitCallback_t pClientCb;
    void * pUpperLayerContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NFCSTATUS_SUCCESS == wStatus)
    {
        /* Initializing the Flags during init*/
        pLibContext->HCE_FirstBuf = 0;
        pLibContext->bPcdConnected = FALSE;
        pLibContext->dev_cnt = 0;

        wStatus = phLibNfc_StateHandler(pContext, TrigEvent, pInfo, NULL, NULL);
    }
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        pClientCb = pLibContext->CBInfo.pClientInitCb;
        pUpperLayerContext = pLibContext->CBInfo.pClientInitCntx;
        pLibContext->CBInfo.pClientInitCb = NULL;
        pLibContext->CBInfo.pClientInitCntx = NULL;

        if(NULL != pClientCb)
        {
            (*pClientCb)(pUpperLayerContext,pLibContext->eConfigStatus,wStatus);
        }
    }
    else
    {
        phLibNfc_ReleaseLibNfcContext(pContext,NFCSTATUS_FAILED);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_GetNfccFeatures(void *pNciHandle)
{
    phNciNfc_NfccFeatures_t tNfccFeatures;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = phNciNfc_GetNfccFeatures(pNciHandle,&tNfccFeatures);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        wStatus = NFCSTATUS_FAILED;
    }
    else
    {
        pLibContext->tNfccFeatures = tNfccFeatures;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_ReleaseLibNfcContext(void *pContext, NFCSTATUS wStatus)
{
    pphLibNfc_LibContext_t pLibContext = pContext;
    pphLibNfc_InitCallback_t pClientCb = NULL;
    void * pUpperLayerContext=NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pLibContext  = (pphLibNfc_LibContext_t ) pContext;
        if(wStatus != NFCSTATUS_SUCCESS)
        {
            PH_LOG_LIBNFC_CRIT_STR("Failed Status from Libnfc state machine, Deinitialising stack");
            pClientCb = pLibContext->CBInfo.pClientInitCb;
            pUpperLayerContext = pLibContext->CBInfo.pClientInitCntx;
            pLibContext->CBInfo.pClientInitCb = NULL;
            pLibContext->CBInfo.pClientInitCntx = NULL;
            if(NULL != pClientCb)
            {
                (*pClientCb)(pUpperLayerContext,pLibContext->eConfigStatus,wStatus);
            }
            (void)phNciNfc_Reset(NULL,
                          phNciNfc_NciReset_Mgt_Reset,
                          NULL,
                          NULL);

            phLibNfc_ClearLibContext(pLibContext);
            pLibContext = NULL;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}
