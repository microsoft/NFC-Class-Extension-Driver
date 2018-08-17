/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_RfConfig.tmh"

static NFCSTATUS phNciNfc_ClearConfigParams(void *pNciHandle);

static NFCSTATUS phNciNfc_ProcessRfParams(void *pNciHandle,uint8_t bTotalLen, uint8_t *pRespParam,uint8_t bStoreParams);
static NFCSTATUS phNciNfc_UpdateReqParamIDs(void *pNciHandle,uint8_t *pRespParamIDList, uint8_t bRespParamIDNum,uint8_t bRespParamIDLen);
static NFCSTATUS phNciNfc_StoreRfParams(uint8_t *pTlvParam, pphNciNfc_RfDiscConfigParams_t pDiscConfigParams);

static NFCSTATUS phNciNfc_GetConfig(void *pContext);
static NFCSTATUS phNciNfc_GetConfigRsp(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_CompleteGetConfig(void *pContext,NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_GetConfigRawRsp(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_CompleteGetConfigRaw(void *pContext,NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_SetConfig(void *pContext);
static NFCSTATUS phNciNfc_SetConfigRsp(void *pContext, NFCSTATUS Status);
static NFCSTATUS phNciNfc_CompleteSetConfig(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_GetConfigOpt(void *pContext);
static NFCSTATUS phNciNfc_GetConfigOptRsp(void *pContext, NFCSTATUS wStatus);

static void phNciNfc_BuildGetConfigOpt(pphNciNfc_RfDiscConfigParams_t pSetConfigParams, pphNciNfc_RfDiscConfigParams_t pGetConfigParams);

static void phNciNfc_CmpParamsSetConfigOpt(pphNciNfc_RfDiscConfigParams_t pSetConfigParams, pphNciNfc_RfDiscConfigParams_t pGetConfigParams);
static void phNciNfc_CmpPollParamsSetConfigOpt(pphNciNfc_RfDiscConfigParams_t pSetConfigParams, pphNciNfc_RfDiscConfigParams_t pGetConfigParams);
static void phNciNfc_CmpLstnParamsSetConfigOpt(pphNciNfc_RfDiscConfigParams_t pSetConfigParams, pphNciNfc_RfDiscConfigParams_t pGetConfigParams);
static void phNciNfc_CmpCommonParamsSetConfigOpt(pphNciNfc_RfDiscConfigParams_t pSetConfigParams, pphNciNfc_RfDiscConfigParams_t pGetConfigParams);

static NFCSTATUS phNciNfc_SetRtngConfig(void *pContext);
static NFCSTATUS phNciNfc_SetRtngConfigRsp(void *pContext, NFCSTATUS Status);
static NFCSTATUS phNciNfc_CompleteSetRtngConfig(void *pContext, NFCSTATUS Status);

static NFCSTATUS phNciNfc_GetRtngConfig(void *pContext);
static NFCSTATUS phNciNfc_GetRtngConfigRsp(void *pContext, NFCSTATUS Status);
static NFCSTATUS phNciNfc_CompleteGetRtngConfig(void *pContext, NFCSTATUS Status);
static NFCSTATUS phNciNfc_GetRtngConfigNtfCb(void* pContext, void *pInfo, NFCSTATUS status);

static NFCSTATUS phNciNfc_ProcessGetRtngNtf(pphNciNfc_RtngConfInfo_t pRtngConfInfo, pphNciNfc_GetRtngConfig_t pGetRtngConfig);
static void phNciNfc_ProcessRtngEntry(uint8_t bType, uint8_t bLen, uint8_t *pValue, pphNciNfc_RtngConfig_t pRtngConf);

static NFCSTATUS phNciNfc_RfParamUpdate(void *pContext);
static NFCSTATUS phNciNfc_RfParamUpdateRsp(void *pContext, NFCSTATUS Status);
static NFCSTATUS phNciNfc_CompleteRfParamUpdate(void *pContext, NFCSTATUS Status);

static NFCSTATUS phNciNfc_ProtoIfMap(void *pContext);
static NFCSTATUS phNciNfc_ProtoIfMapRsp(void *pContext, NFCSTATUS Status);
static NFCSTATUS phNciNfc_CompleteProtoIfMap(void *pContext, NFCSTATUS Status);

static uint8_t phNciNfc_CheckRfIntfs(uint8_t *pSupportedRfIntfs, uint8_t bNoOfRfIfSuprt, phNciNfc_RfInterfaces_t tRequestedRfIntf);
static uint8_t phNciNfc_ValidateMode(phNciNfc_MapMode_t Mode);

static NFCSTATUS phNciNfc_ValidateRoutingEntryParams(pphNciNfc_RtngConfig_t pRoutingEntry, uint16_t *pSize);

static uint8_t phNciNfc_WriteRoutingEntryToPayload(uint8_t *buffer, phNciNfc_RtngConfig_t *routingEntry);

static uint8_t phNciNfc_VerifyRfProtocols(uint8_t bNumMapEntries, pphNciNfc_MappingConfig_t pProtoIfMapping);

static NFCSTATUS phNciNfc_ValidateCommonParams(pphNciNfc_CommonDiscParams_t pDiscCommConfig, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidatePollNfcAParams(pphNciNfc_PollNfcADiscParams_t pPollNfcAConf, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidatePollNfcAKovioParams(pphNciNfc_PollNfcAKovioDiscParams_t pPollNfcAKovioConf, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidatePollNfcBParams(pphNciNfc_PollNfcBDiscParams_t pPollNfcBConf, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidatePollNfcFParams(pphNciNfc_PollNfcFDiscParams_t pPollNfcFConf, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidatePollIsoDepParams(pphNciNfc_PollIsoDepDiscParams_t pPollIsoDepConf, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidatePollNfcDepParams(pphNciNfc_PollNfcDepDiscParams_t pPollNfcDepConf, uint16_t *wSize, uint8_t *pNumParams);

static NFCSTATUS phNciNfc_ValidateLstnNfcAParams(pphNciNfc_LstnNfcADiscParams_t pLstnNfcAConf, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidateLstnNfcBParams(pphNciNfc_LstnNfcBDiscParams_t pLstnNfcBConf, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidateLstnNfcFParams(pphNciNfc_LstnNfcFDiscParams_t pLstnNfcFConf, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidateLstnIsoDepParams(pphNciNfc_LstnIsoDepDiscParams_t pLstnIsoDepConf, uint16_t *wSize, uint8_t *pNumParams);
static NFCSTATUS phNciNfc_ValidateLstnNfcDepParams(pphNciNfc_LstnNfcDepDiscParams_t pLstnNfcDepConf, uint16_t *wSize, uint8_t *pNumParams);

static NFCSTATUS phNciNfc_BuildSetConfCmdPayload(pphNciNfc_RfDiscConfigParams_t pDiscConfig, pphNciNfc_TlvUtilInfo_t pTlvInfo);
static uint8_t phNciNfc_BuildCommonParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_CommonDiscParams_t pDiscCommonConf);

static uint8_t phNciNfc_BuildPollNfcAParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_PollNfcADiscParams_t pPollNfcAParams);
static uint8_t phNciNfc_BuildPollNfcKovioAParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_PollNfcAKovioDiscParams_t pPollNfcAKovioParams);
static uint8_t phNciNfc_BuildPollNfcBParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_PollNfcBDiscParams_t pPollNfcBParams);
static uint8_t phNciNfc_BuildPollNfcFParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_PollNfcFDiscParams_t pPollNfcFParams);
static uint8_t phNciNfc_BuildPollIsoDepParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_PollIsoDepDiscParams_t pPollIsoDepParams);
static uint8_t phNciNfc_BuildPollNfcDepParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_PollNfcDepDiscParams_t pPollNfcDepParams);

static uint8_t phNciNfc_BuildLstnNfcAParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_LstnNfcADiscParams_t pLstnNfcADiscConf);
static uint8_t phNciNfc_BuildLstnNfcBParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_LstnNfcBDiscParams_t pLstnNfcBDiscConf);
static uint8_t phNciNfc_BuildLstnNfcFParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_LstnNfcFDiscParams_t pLstnNfcFDiscConf);
static uint8_t phNciNfc_BuildLstnIsoDepParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_LstnIsoDepDiscParams_t pLstnIsoDepDiscConf);
static uint8_t phNciNfc_BuildLstnNfcDepParams(pphNciNfc_TlvUtilInfo_t pTlvInfo, pphNciNfc_LstnNfcDepDiscParams_t pLstnNfcDepDiscConf);

static uint8_t phNciNfc_GetT3tTagId(uint8_t bCount);
static uint8_t phNciNfc_CalNumEntries(pphNciNfc_Context_t pNciContext, uint8_t *pMore, uint16_t *pSize);
static NFCSTATUS phNciNfc_SetConfEntryCheck(pphNciNfc_RfDiscConfigParams_t pDiscConfParams);

static uint8_t phNciNfc_EntrySize(pphNciNfc_RtngConfig_t pRtngEntry);
static uint8_t phNciNfc_EntryValueSize(pphNciNfc_RtngConfig_t pRtngEntry);

/*Global Variables for Set config */

phNciNfc_SequenceP_t gphNciNfc_SetConfigSequence[] = {
    {&phNciNfc_SetConfig, &phNciNfc_SetConfigRsp},
    {NULL, &phNciNfc_CompleteSetConfig}
};

phNciNfc_SequenceP_t gphNciNfc_SetConfigOptSequence[] = {
    {&phNciNfc_GetConfigOpt, &phNciNfc_GetConfigOptRsp},
    {&phNciNfc_SetConfig, &phNciNfc_SetConfigRsp},
    {NULL, &phNciNfc_CompleteSetConfig}
};

phNciNfc_SequenceP_t gphNciNfc_ProtoIfMapSequence[] = {
    {&phNciNfc_ProtoIfMap, &phNciNfc_ProtoIfMapRsp},
    {NULL, &phNciNfc_CompleteProtoIfMap}
};

phNciNfc_SequenceP_t gphNciNfc_GetRtngConfigSequence[] = {
    {&phNciNfc_GetRtngConfig, &phNciNfc_GetRtngConfigRsp},
    {NULL, &phNciNfc_CompleteGetRtngConfig}
};

phNciNfc_SequenceP_t gphNciNfc_RfParamUpdateSequence[] = {
    {&phNciNfc_RfParamUpdate, &phNciNfc_RfParamUpdateRsp},
    {NULL, &phNciNfc_CompleteRfParamUpdate}
};

static phNciNfc_SequenceP_t gphNciNfc_SetRtngConfigSequence[] = {
    {&phNciNfc_SetRtngConfig, &phNciNfc_SetRtngConfigRsp},
    {NULL, &phNciNfc_CompleteSetRtngConfig}
};

/*Global Variables for Get config */

phNciNfc_SequenceP_t gphNciNfc_GetConfigSequence[] = {
    {&phNciNfc_GetConfig, &phNciNfc_GetConfigRsp},
    {NULL, &phNciNfc_CompleteGetConfig}
};

phNciNfc_SequenceP_t gphNciNfc_GetConfigRawSequence[] = {
    {&phNciNfc_GetConfig, &phNciNfc_GetConfigRawRsp},
    {NULL, &phNciNfc_CompleteGetConfigRaw}
};
/*End of Global Variables*/

NFCSTATUS
phNciNfc_ValidateDiscMapParams(uint8_t bNumMapEntries,
                               pphNciNfc_MappingConfig_t    pProtoIfMapping)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bEntryList = 0;
    uint8_t bReturnStat = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pProtoIfMapping)
    {
        wStatus = NFCSTATUS_SUCCESS;
        /* In all input Rf protocol to Rf interface mapping configurations, validate Rf protocol, mode and Rf interface */
        for(bEntryList = 0; bEntryList < bNumMapEntries; bEntryList++)
        {
            /* Validate Rf protocol*/
            bReturnStat = phNciNfc_ValidateRfProtocol(pProtoIfMapping[bEntryList].tRfProtocol);
            if(1 == bReturnStat)
            {
                PH_LOG_NCI_INFO_STR("Invalid RF procotol");
                wStatus = NFCSTATUS_INVALID_PARAMETER;
                break;
            }
            else
            {
                /* Validate mode*/
                bReturnStat = phNciNfc_ValidateMode(pProtoIfMapping[bEntryList].Mode);
            }

            if(1 == bReturnStat)
            {
                PH_LOG_NCI_INFO_STR("Invalid mode");
                wStatus = NFCSTATUS_INVALID_PARAMETER;
                break;
            }
            else
            {
                /* Validate Rf interface */
                bReturnStat = phNciNfc_ValidateRfInterface(pProtoIfMapping[bEntryList].tRfInterface);
                if(1 == bReturnStat)
                {
                    PH_LOG_NCI_INFO_STR("Invalid RF interface");
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                    break;
                }
            }
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* One Rf protocol should be mapped to only one Rf interface always */
            bReturnStat = phNciNfc_VerifyRfProtocols(bNumMapEntries, pProtoIfMapping);

            if(1 == bReturnStat)
            {
                /* An Rf protocol is being mapped to multiple Rf interfaces */
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_VerifySupportedRfIntfs(pphNciNfc_Context_t pNciCtx,
                                uint8_t bNumMapEntries,
                                pphNciNfc_MappingConfig_t    pProtoIfMapping)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t bEntryList = 0;
    uint8_t bStatus = 0;
    pphNci_sInitRspParams_t pNciInit =  &pNciCtx->InitRspParams;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Minimum number of Rf interfaces supported by NFCC should be 1 */
    if(pNciInit->NoOfRfIfSuprt >= PHNCINFC_RFCONFIG_MINSUPPORTED_RFINTFS)
    {
        for(bEntryList = 0; bEntryList < bNumMapEntries; bEntryList++)
        {
            bStatus = phNciNfc_CheckRfIntfs(pNciInit->RfInterfaces,pNciInit->NoOfRfIfSuprt,
                                   pProtoIfMapping[bEntryList].tRfInterface);
            if(1 == bStatus)
            {
                PH_LOG_NCI_WARN_STR("Required Rf interface not supported by NFCC");

                /* Requested Rf interface not supported by NFCC */
                wStatus = NFCSTATUS_FAILED;
                break;
            }
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
        PH_LOG_NCI_CRIT_STR("NFCC does not supported any RF interface");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void
phNciNfc_BuildDiscMapCmdPayload(
    _Out_writes_((bNumMapEntries * PHNCINFC_RFCONFIG_NUMPAYLOADFIELDS) + PHNCINFC_RFCONFIG_NUMMAPENTRIES_SIZE)
        uint8_t                     *pBuffer,
    _In_
        uint8_t                     bNumMapEntries,
    _In_
        pphNciNfc_MappingConfig_t   pProtoIfMapping
        )
{
    uint8_t bNoOfEntries = 0;
    uint8_t bOffset = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    pBuffer[bOffset++] = bNumMapEntries;
    for(bNoOfEntries = 0; bNoOfEntries < bNumMapEntries; bNoOfEntries++)
    {
        pBuffer[bOffset++] = pProtoIfMapping[bNoOfEntries].tRfProtocol;

        pBuffer[bOffset] = 0;
        pBuffer[bOffset] |= pProtoIfMapping[bNoOfEntries].Mode.bPollMode;
        pBuffer[bOffset] = (uint8_t)((pBuffer[bOffset]) | (pProtoIfMapping[bNoOfEntries].Mode.bLstnMode << 1));
        bOffset++;

        pBuffer[bOffset++] = pProtoIfMapping[bNoOfEntries].tRfInterface;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

NFCSTATUS
phNciNfc_ValidateSetConfParams(pphNciNfc_RfDiscConfigParams_t pDiscConfParams,
                                uint16_t *pParamSize,
                                uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pDiscConfParams) && (NULL != pParamSize) && (NULL != pNumParams))
    {
        /* Check whether there any entries to be configured */
        wStatus = phNciNfc_SetConfEntryCheck(pDiscConfParams);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Validate Poll Nfc-A Discovery configuration parameters */
            if(1 == pDiscConfParams->tConfigInfo.PollNfcAConfig)
            {
                wStatus = phNciNfc_ValidatePollNfcAParams(&pDiscConfParams->tPollNfcADiscParams,pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidatePollNfcAParams failed");
                }
            }

            /* Validate Poll Nfc-B Discovery configuration parameters */
            if((NFCSTATUS_INVALID_PARAMETER != wStatus) && (1 == pDiscConfParams->tConfigInfo.PollNfcBConfig))
            {
                wStatus = phNciNfc_ValidatePollNfcBParams(&pDiscConfParams->tPollNfcBDiscParams, pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidatePollNfcBParams failed");
                }
            }

            /* Validate Poll Nfc-F Discovery configuration parameters */
            if((NFCSTATUS_INVALID_PARAMETER != wStatus) && (1 == pDiscConfParams->tConfigInfo.PollNfcFConfig))
            {
                wStatus = phNciNfc_ValidatePollNfcFParams(&pDiscConfParams->tPollNfcFDiscParams, pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidatePollNfcFParams failed");
                }
            }

            /* Validate Poll Iso-Dep Discovery configuration parameters */
            if((NFCSTATUS_INVALID_PARAMETER != wStatus) && (1 == pDiscConfParams->tConfigInfo.PollIsoDepConfig))
            {
                wStatus = phNciNfc_ValidatePollIsoDepParams(&pDiscConfParams->tPollIsoDepDiscParams, pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidatePollIsoDepParams failed");
                }
            }

            /* Validate Poll Nfc-Dep Discovery configuration parameters */
            if((NFCSTATUS_INVALID_PARAMETER != wStatus) && (1 == pDiscConfParams->tConfigInfo.PollNfcDepConfig))
            {
                wStatus = phNciNfc_ValidatePollNfcDepParams(&pDiscConfParams->tPollNfcDepDiscParams, pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidatePollNfcDepParams failed");
                }
            }

            /* Validate Listen Nfc-A Discovery configuration parameters */
            if((NFCSTATUS_INVALID_PARAMETER != wStatus) && (1 == pDiscConfParams->tConfigInfo.LstnNfcAConfig))
            {
                wStatus = phNciNfc_ValidateLstnNfcAParams(&pDiscConfParams->tLstnNfcADiscParams, pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidateLstnNfcAParams failed");
                }
            }

            /* Validate Listen Nfc-B Discovery configuration parameters */
            if((NFCSTATUS_INVALID_PARAMETER != wStatus) && (1 == pDiscConfParams->tConfigInfo.LstnNfcBConfig))
            {
                wStatus = phNciNfc_ValidateLstnNfcBParams(&pDiscConfParams->tLstnNfcBDiscParams, pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidateLstnNfcBParams failed");
                }
            }

            /* Validate Listen Nfc-F Discovery configuration parameters */
            if((NFCSTATUS_INVALID_PARAMETER != wStatus) && (1 == pDiscConfParams->tConfigInfo.LstnNfcFConfig))
            {
                wStatus = phNciNfc_ValidateLstnNfcFParams(&pDiscConfParams->tLstnNfcFDiscParams, pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidateLstnNfcFParams failed");
                }
            }

            /* Validate Listen Iso-Dep Discovery configuration parameters */
            if((NFCSTATUS_INVALID_PARAMETER != wStatus) && (1 == pDiscConfParams->tConfigInfo.LstnIsoDepConfig))
            {
                wStatus = phNciNfc_ValidateLstnIsoDepParams(&pDiscConfParams->tLstnIsoDepDiscParams, pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidateLstnIsoDepParams failed");
                }
            }

            /* Validate Listen Nfc-Dep Discovery configuration parameters */
            if((NFCSTATUS_INVALID_PARAMETER != wStatus) && (1 == pDiscConfParams->tConfigInfo.LstnNfcDepConfig))
            {
                wStatus = phNciNfc_ValidateLstnNfcDepParams(&pDiscConfParams->tLstnNfcDepDiscParams, pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidateLstnNfcDepParams failed");
                }
            }

            if((wStatus != NFCSTATUS_INVALID_PARAMETER) && (1 == pDiscConfParams->tConfigInfo.CommonConfig))
            {
                wStatus = phNciNfc_ValidateCommonParams(&pDiscConfParams->tCommonDiscParams,pParamSize,pNumParams);
                if(NFCSTATUS_INVALID_PARAMETER == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("phNciNfc_ValidateCommonParams failed");
                }
            }
        }
        else
        {
            PH_LOG_NCI_INFO_STR("No set config entries");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_BuildSetConfPayload(pphNciNfc_RfDiscConfigParams_t pDiscConfig,
                              uint8_t *pPayloadBuff,
                              uint16_t wPayloadSize)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_TlvUtilInfo_t tTlvInfo;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pPayloadBuff) && (NULL != pDiscConfig))
    {
        tTlvInfo.dwLength = wPayloadSize;   /* Total size of the payload buffer */
        tTlvInfo.pBuffer = pPayloadBuff;    /* Pointer to the payload buffer */
        tTlvInfo.sizeInfo.dwpacketSize = 1; /* 0th location contains number of parameters */

        /* Format and copy input listen mode routing configuration entries into the payload buffer
           as defined by the NCI spec */
        wStatus = phNciNfc_BuildSetConfCmdPayload(pDiscConfig,&tTlvInfo);
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_ValidateSetRtngParams(uint8_t                 bNumRtngEntries,
                               pphNciNfc_RtngConfig_t pRtngConfig,
                               uint16_t               *pSize)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bEntry = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pRtngConfig) && (NULL != pSize))
    {
        for(bEntry = 0; bEntry < bNumRtngEntries; bEntry++)
        {
            wStatus = phNciNfc_ValidateRoutingEntryParams(&(pRtngConfig[bEntry]), pSize);

            if(NFCSTATUS_SUCCESS != wStatus)
            {
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_SetRtngCmdHandler(pphNciNfc_Context_t pNciContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bNumEntries = 0;
    uint8_t bMore = 0;
    uint16_t wPayloadSize = 0;
    uint8_t *pPayloadBuff = NULL;
    pphNciNfc_RtngConfig_t pRtngConf = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        bNumEntries = phNciNfc_CalNumEntries(pNciContext, &bMore, &wPayloadSize);

        if(0 != bNumEntries)
        {
            wPayloadSize += PHNCINFC_RFCONFIG_LSTN_RTNG_DEFAULT_SIZE;

            /* Allocating memory for the required number of entries */
            pPayloadBuff = phOsalNfc_GetMemory((uint32_t)wPayloadSize);
            if(NULL != pPayloadBuff)
            {
                pRtngConf = (pphNciNfc_RtngConfig_t ) pNciContext->pUpperLayerInfo;
                wStatus = NFCSTATUS_FAILED;
                /* Build pkt with 'bNumEntries' entries starting from 'offset' entry */
                phNciNfc_BuildSetLstnRtngCmdPayload(pPayloadBuff,bNumEntries,
                                &pRtngConf[pNciContext->tRtngConfInfo.bRtngEntryOffset],
                                bMore);

                /* Update Rtng Handler info */
                pNciContext->tRtngConfInfo.bRtngEntryOffset += bNumEntries;
                pNciContext->tRtngConfInfo.bMore = bMore;

                PHNCINFC_INIT_SEQUENCE(pNciContext, gphNciNfc_SetRtngConfigSequence);
                pNciContext->tSendPayload.pBuff = pPayloadBuff;
                pNciContext->tSendPayload.wPayloadSize = wPayloadSize;
                /* Start the Set config sequence */
                wStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
                if(NFCSTATUS_PENDING != wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("Set Rtng table Sequence failed!");
                }
            }
            else
            {
                /* Memory allocation failed */
                PH_LOG_NCI_CRIT_STR("Memory allocation for Set Rtng cmd faild");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("No entries are present");
            wStatus = NFCSTATUS_FAILED;

            pNciContext->tRtngConfInfo.bRtngEntryOffset = 0;
            pNciContext->tRtngConfInfo.bMore = 0;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_VerifySupportedRouting(pphNciNfc_NfccFeatures_t pNfccFeatures,
                                uint8_t                bNumRtngEntries,
                                pphNciNfc_RtngConfig_t pRtngConfig)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t bEntries = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pRtngConfig) && (NULL != pNfccFeatures))
    {
        for(bEntries = 0; bEntries < bNumRtngEntries; bEntries++)
        {
            /* Check if requested type of routing is supported by NFCC */
            switch(pRtngConfig[bEntries].Type)
            {
                case phNciNfc_e_LstnModeRtngTechBased:
                    if(1 != pNfccFeatures->RoutingInfo.TechnBasedRouting)
                    {
                        PH_LOG_NCI_WARN_STR("Techn based routing request but not supported by NFCC");
                        wStatus = NFCSTATUS_FAILED;
                    }
                    break;

                case phNciNfc_e_LstnModeRtngProtocolBased:
                    if(1 != pNfccFeatures->RoutingInfo.ProtocolBasedRouting)
                    {
                        PH_LOG_NCI_WARN_STR("Protocol based routing request but not supported by NFCC");
                        wStatus = NFCSTATUS_FAILED;
                    }
                    break;

                case phNciNfc_e_LstnModeRtngAidBased:
                    if(1 != pNfccFeatures->RoutingInfo.AidBasedRouting)
                    {
                        PH_LOG_NCI_WARN_STR("Aid based routing request but not supported by NFCC");
                        wStatus = NFCSTATUS_FAILED;
                    }
                    break;
                default:
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                    break;
            }

            /* Check if requested power state is supported by NFCC */
            if (NFCSTATUS_SUCCESS == wStatus)
            {
                pphNciNfc_PowerState_t pPowerState = &pRtngConfig[bEntries].PowerState;

                if((1 == pPowerState->bBatteryOff) &&
                   (1 != pNfccFeatures->PowerStateInfo.BatteryOffState))
                {
                    PH_LOG_NCI_WARN_STR("Battery off pow state requested but not supported");
                    wStatus = NFCSTATUS_FAILED;
                }
                if((1 == pPowerState->bSwitchedOff) &&
                   (1 != pNfccFeatures->PowerStateInfo.SwitchOffState))
                {
                    PH_LOG_NCI_WARN_STR("Switched off pow state requested but not supported");
                    wStatus = NFCSTATUS_FAILED;
                }
            }

            if (NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_WARN_STR("Input Routing type not supported by NFCC");
                break;
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

NFCSTATUS
phNciNfc_BuildRfParamUpdateCmdPayload(uint8_t *pBuff,
                                      uint8_t bNumParams,
                                      pphNciNfc_RfParamUpdate_t pRfParams)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_RfParamUpdate_t pRfParamUpdate = NULL;
    phNciNfc_NfcBDataExchgConf_t *pNfcBDataExchgConf = NULL;
    uint8_t bOffset = 0;
    uint8_t bCount = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pBuff) && (NULL != pRfParams))
    {
        for(bCount = 0; bCount < bNumParams; bCount++)
        {
            pRfParamUpdate = (pphNciNfc_RfParamUpdate_t) &pRfParams[bCount];
            pBuff[bOffset++] = (uint8_t)pRfParamUpdate->Type;
            pBuff[bOffset++] = 1;
            switch(pRfParamUpdate->Type)
            {
                case phNciNfc_e_RfParamRfTechAndMode:
                    pBuff[bOffset++] = pRfParamUpdate->RfParamValue.eTechAndMode;
                    break;

                case phNciNfc_e_RfParamTransmitBitrate:
                case phNciNfc_e_RfParamReceiveBitrate:
                    pBuff[bOffset++] = pRfParamUpdate->RfParamValue.eBitRate;
                    break;

                case phNciNfc_e_RfParamNfcBDataExchgConf:

                    pNfcBDataExchgConf = &pRfParamUpdate->RfParamValue.tNfcBDataExchgConf;
                    pBuff[bOffset] = 0;
                    pBuff[bOffset] = (uint8_t)PHNCINFC_RFCONFIG_SET_TR2(pBuff[bOffset],pNfcBDataExchgConf->MinTR2);
                    pBuff[bOffset] = (uint8_t)PHNCINFC_RFCONFIG_SET_TR1(pBuff[bOffset],pNfcBDataExchgConf->MinTR1);
                    pBuff[bOffset] = (uint8_t)PHNCINFC_RFCONFIG_SET_TR0(pBuff[bOffset],pNfcBDataExchgConf->MinTR0);
                    pBuff[bOffset] = (uint8_t)PHNCINFC_RFCONFIG_SET_SUPP_EOS(pBuff[bOffset],pNfcBDataExchgConf->SupressEoS);
                    pBuff[bOffset] = (uint8_t)PHNCINFC_RFCONFIG_SET_SUPP_SOS(pBuff[bOffset],pNfcBDataExchgConf->SupressSoS);
                    bOffset++;
                    break;

                default:
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                    break;
            }
            if(NFCSTATUS_INVALID_PARAMETER == wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Invalid message type");
                break;
            }
        }
    }
    else
    {
        PH_LOG_NCI_INFO_STR("Invalid input parameter");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void
phNciNfc_BuildSetLstnRtngCmdPayload(uint8_t                *pBuffer,
                                    uint8_t                bNumRtngEntries,
                                    pphNciNfc_RtngConfig_t pRtngConfig,
                                    uint8_t                bMore)
{
    uint8_t offset = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    /* As per NCI 2.0, Section 6.3.2 'Configure Listen Mode Routing'
       routing entries must be in the following order */
    const uint8_t s_requiredEntryTypeOrdering[] = {phNciNfc_e_LstnModeRtngAidBased,
                                                   phNciNfc_e_LstnModeRtngProtocolBased,
                                                   phNciNfc_e_LstnModeRtngTechBased};

    /* Update more field into the payload buffer */
    pBuffer[offset++] = bMore;

    /* Update number of routing configuration entries */
    pBuffer[offset++] = bNumRtngEntries;

    /* Write routing entries to the payload ordered by type */
    for (uint8_t i = 0; i < ARRAYSIZE(s_requiredEntryTypeOrdering); i++)
    {
        uint8_t currentType = s_requiredEntryTypeOrdering[i];

        for (uint8_t entryIndex = 0; entryIndex < bNumRtngEntries; entryIndex++)
        {
            if (pRtngConfig[entryIndex].Type == currentType)
            {
                offset += phNciNfc_WriteRoutingEntryToPayload(&pBuffer[offset], &pRtngConfig[entryIndex]);
            }
            else
            {
                /* Should never enter here since all input parameters are already validated */
                PH_LOG_NCI_WARN_STR("Unknown routing type");
                break;
            }
        }
    }

    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

static NFCSTATUS
phNciNfc_SetConfEntryCheck(pphNciNfc_RfDiscConfigParams_t pDiscConfParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Check atleast on category of parameters are being requested to be configured */
    if((1 == pDiscConfParams->tConfigInfo.PollNfcAConfig)    || \
       (1 == pDiscConfParams->tConfigInfo.PollNfcBConfig)    || \
       (1 == pDiscConfParams->tConfigInfo.PollNfcFConfig)    || \
       (1 == pDiscConfParams->tConfigInfo.PollIsoDepConfig)  || \
       (1 == pDiscConfParams->tConfigInfo.PollNfcDepConfig)  || \
       (1 == pDiscConfParams->tConfigInfo.LstnNfcAConfig)    || \
       (1 == pDiscConfParams->tConfigInfo.LstnNfcBConfig)    || \
       (1 == pDiscConfParams->tConfigInfo.LstnNfcFConfig)    || \
       (1 == pDiscConfParams->tConfigInfo.LstnIsoDepConfig)  || \
       (1 == pDiscConfParams->tConfigInfo.LstnNfcDepConfig)  || \
       (1 == pDiscConfParams->tConfigInfo.CommonConfig))
   {
       wStatus = NFCSTATUS_SUCCESS;
   }
   PH_LOG_NCI_FUNC_EXIT();
   return wStatus;
}

static uint8_t
phNciNfc_CalNumEntries(pphNciNfc_Context_t pNciContext, uint8_t *pMore, uint16_t *pSize)
{
    uint8_t bEntries = 0;
    uint8_t bTotalEntries = pNciContext->tRtngConfInfo.bTotalNumEntries;
    uint8_t bCurrEntry = pNciContext->tRtngConfInfo.bRtngEntryOffset;
    uint8_t bCurrEntrySize = 0;
    uint16_t wCumulativeSize = 0;
    pphNciNfc_RtngConfig_t pRtngConf = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    *pMore = 1;
    pRtngConf = (pphNciNfc_RtngConfig_t ) pNciContext->pUpperLayerInfo;
    for(; bCurrEntry < bTotalEntries; bCurrEntry++)
    {
        bCurrEntrySize = phNciNfc_EntrySize(&(pRtngConf[bCurrEntry]));
        wCumulativeSize += bCurrEntrySize;
        /* If size exceeds max control packet size */
        if(wCumulativeSize >= pNciContext->InitRspParams.CntrlPktPayloadLen)
        {
            break;
        }
        else
        {
            *pSize += bCurrEntrySize;
            bEntries++;
        }
    }

    /* If size of single entry exceeds Max Ctrl pkt size */
    if(bEntries == 0)
    {
        *pSize += bCurrEntrySize;
        bEntries = 1;
    }

    if((pNciContext->tRtngConfInfo.bRtngEntryOffset + bEntries) >= bTotalEntries)
    {
        *pMore = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bEntries;
}

static uint8_t
phNciNfc_EntrySize(pphNciNfc_RtngConfig_t pRtngEntry)
{
    return phNciNfc_EntryValueSize(pRtngEntry) + PHNCINFC_RFCONFIG_TLV_HEADER_LEN;
}

static uint8_t
phNciNfc_EntryValueSize(pphNciNfc_RtngConfig_t pRtngEntry)
{
    uint8_t bSize = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    switch(pRtngEntry->Type)
    {
        case phNciNfc_e_LstnModeRtngTechBased:
            bSize = PHNCINFC_RFCONFIG_TECH_RTNG_VALUE_LEN;
            break;

        case phNciNfc_e_LstnModeRtngProtocolBased:
            bSize = PHNCINFC_RFCONFIG_PROTO_RTNG_VALUE_LEN;
            break;

        case phNciNfc_e_LstnModeRtngAidBased:
            bSize = PHNCINFC_RFCONFIG_AID_VALUE_DEFAULT_LEN +
                    pRtngEntry->LstnModeRtngValue.tAidBasedRtngValue.bAidSize;
            break;
        default:
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bSize;
}

static NFCSTATUS
phNciNfc_ProtoIfMap(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_PENDING;
    phNciNfc_CoreTxInfo_t tTxInfo;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t) pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        /* Clear the Tx information structure */
        phOsalNfc_SetMemory(&tTxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));

        tTxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        tTxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
        tTxInfo.tHeaderInfo.Opcode_ID.OidType.RfMgtCmdOid = phNciNfc_e_RfMgtRfDiscoverMapCmdOid;

        tTxInfo.Buff = pNciCtx->tSendPayload.pBuff;
        tTxInfo.wLen = pNciCtx->tSendPayload.wPayloadSize;

        /* Sending command */
        wStatus = phNciNfc_CoreIfTxRx(&(pNciCtx->NciCoreContext), &tTxInfo,
                        &(pNciCtx->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,(pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                        (void *)pNciCtx);
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameter");
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

/*Refer function declaration for information*/
static NFCSTATUS
phNciNfc_ProtoIfMapRsp(void *pContext,
                       NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        /* Validate the length of the response received */
        if(1 == pNciContext->RspBuffInfo.wLen && pNciContext->RspBuffInfo.pBuff != NULL)
        {
            /*Check Status Byte*/
            if(pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
            {
                PH_LOG_NCI_INFO_STR("Rf Proto Intf mapping: SUCCESS");
                /* Rf protocol to Rf interface mapping success */
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Rf Proto Intf mapping: FAILED");
                /* Rf interface mapping failed */
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            /*wStatus = Invalid Paload Lenght*/
            wStatus = NFCSTATUS_FAILED;
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
phNciNfc_CompleteProtoIfMap(void *pContext,
                            NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    /*Dealocate resource allocate before init sequnce*/
    if(NULL != pNciCtx)
    {
        phNciNfc_FreeSendPayloadBuff(pNciCtx);
        phNciNfc_Notify(pNciCtx, wStatus, NULL);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_SetConfig(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreTxInfo_t tTxInfo;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t) pContext;
    uint16_t wParamsSize = 0;
    uint8_t bNumParams = 0;
    uint16_t wPayloadSize = 0;
    uint8_t *pPayloadBuff = NULL;
    uint16_t bNumParamsOffset = 0;
    uint8_t bSkipSeqFlag = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        if(pNciCtx->tSetConfOptInfo.pSetConfParams != NULL)
        {
            wStatus = phNciNfc_ValidateSetConfParams(pNciCtx->tSetConfOptInfo.pSetConfParams,
                                                     &wParamsSize,&bNumParams);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                /* Payload size = Size of all input parameters + (memory for storing type & length fields of
                                  all parameters) + memory for storing number of parameters (tlv's) */
                wPayloadSize = wParamsSize + (bNumParams * PHNCINFC_RFCONFIG_TLV_HEADER_SIZE) + PHNCINFC_RFCONFIG_NUM_CONFIGS_SIZE;
                pPayloadBuff = (uint8_t *) phOsalNfc_GetMemory((uint32_t)wPayloadSize);
                if(NULL != pPayloadBuff)
                {
                    pPayloadBuff[bNumParamsOffset] = bNumParams;
                    wStatus = phNciNfc_BuildSetConfPayload(pNciCtx->tSetConfOptInfo.pSetConfParams,
                                                           pPayloadBuff,wPayloadSize);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        pNciCtx->tSendPayload.pBuff = pPayloadBuff;
                        pNciCtx->tSendPayload.wPayloadSize = wPayloadSize;
                    }
                    else
                    {
                        PH_LOG_NCI_CRIT_STR("Build Set config command payload failed!");
                        phNciNfc_FreeSendPayloadBuff(pNciCtx);
                    }
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED;
                    PH_LOG_NCI_CRIT_STR("Memory allocation for Set Config Cmd payload failed!");
                }
            }
            else
            {
                /*If there are no parameters to set(requested params are already set),
                then validation fails. During this case skip the set config sequence*/
                phNciNfc_SkipSequenceSeq(pNciCtx,pNciCtx->pSeqHandler,1);
                wStatus = NFCSTATUS_SUCCESS;
                bSkipSeqFlag = 1;
            }
        }
        else
        {
            wStatus = NFCSTATUS_SUCCESS;
        }

        if(NFCSTATUS_SUCCESS == wStatus && bSkipSeqFlag == 0)
        {
            /* Clear the Tx information structure */
            phOsalNfc_SetMemory(&tTxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));

            tTxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
            tTxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
            tTxInfo.tHeaderInfo.Opcode_ID.OidType.NciCoreCmdOid = phNciNfc_e_NciCoreSetConfigCmdOid;

            tTxInfo.Buff = pNciCtx->tSendPayload.pBuff;
            tTxInfo.wLen = pNciCtx->tSendPayload.wPayloadSize;

            /* Sending command */
            wStatus = phNciNfc_CoreIfTxRx(&(pNciCtx->NciCoreContext), &tTxInfo,
                            &(pNciCtx->RspBuffInfo), PHNCINFC_RFCONFIG_SETCONFIG_TIMEOUT,
                                (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                            (void *)pNciCtx);
        }
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameter");
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_SetConfigRsp(void *pContext,
                      NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        /* Validate the length of the response received */
        if(pNciContext->RspBuffInfo.wLen >= 2 && pNciContext->RspBuffInfo.pBuff != NULL)
        {
            /*Check Status Byte*/
            if(pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Set Config failed");
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
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_CompleteSetConfig(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    /*Dealocate resource allocate before init sequnce*/
    if(NULL != pNciCtx)
    {
        phNciNfc_FreeSendPayloadBuff(pNciCtx);
        /*Free the allocated memory used for set config optimization*/
        if(pNciCtx->tSetConfOptInfo.pSetConfParams != NULL)
        {
            phOsalNfc_FreeMemory(pNciCtx->tSetConfOptInfo.pSetConfParams);
            pNciCtx->tSetConfOptInfo.pSetConfParams = NULL;
        }
        if(pNciCtx->tSetConfOptInfo.pGetCfgParams != NULL)
        {
            phOsalNfc_FreeMemory(pNciCtx->tSetConfOptInfo.pGetCfgParams);
            pNciCtx->tSetConfOptInfo.pGetCfgParams = NULL;
        }
        phNciNfc_Notify(pNciCtx, wStatus, NULL);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void phNciNfc_BuildGetConfigOpt(pphNciNfc_RfDiscConfigParams_t pSetConfigParams,
                                pphNciNfc_RfDiscConfigParams_t pGetConfigParams)
{
    /*Copy only bit field values from requested set config to get config structure*/
    phOsalNfc_MemCopy(&(pGetConfigParams->tConfigInfo),
                      &(pSetConfigParams->tConfigInfo),
                      sizeof(phNciNfc_RfConfigInfo_t));

    phOsalNfc_MemCopy(&(pGetConfigParams->tPollNfcADiscParams.PollNfcAConfig.Config),
                      &(pSetConfigParams->tPollNfcADiscParams.PollNfcAConfig.Config),
                      sizeof(pGetConfigParams->tPollNfcADiscParams.PollNfcAConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config),
                      &(pSetConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config),
                      sizeof(pGetConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tPollNfcFDiscParams.PollNfcFConfig.Config),
                      &(pSetConfigParams->tPollNfcFDiscParams.PollNfcFConfig.Config),
                      sizeof(pGetConfigParams->tPollNfcFDiscParams.PollNfcFConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config),
                      &(pSetConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config),
                      sizeof(pGetConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config),
                      &(pSetConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config),
                      sizeof(pGetConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config),
                      &(pSetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config),
                      sizeof(pGetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config),
                      &(pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config),
                      sizeof(pGetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config),
                      &(pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config),
                      sizeof(pGetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config),
                      &(pSetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config),
                      sizeof(pGetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config),
                      &(pSetConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config),
                      sizeof(pGetConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config));

    phOsalNfc_MemCopy(&(pGetConfigParams->tCommonDiscParams.ComnParamsConfig.Config),
                      &(pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config),
                      sizeof(pGetConfigParams->tCommonDiscParams.ComnParamsConfig.Config));
}

void phNciNfc_CmpParamsSetConfigOpt(phNciNfc_RfDiscConfigParams_t *pSetConfigParams,
                                    phNciNfc_RfDiscConfigParams_t *pGetConfigParams)
{
    if(phOsalNfc_MemCompare((void*)pSetConfigParams, (void*)pGetConfigParams,
                            sizeof(phNciNfc_RfDiscConfigParams_t)) == 0)
    {
        PH_LOG_NCI_INFO_STR("All requested parameters are already set. Not sending set config command");
        phOsalNfc_SetMemory(&(pSetConfigParams->tConfigInfo), 0x00, sizeof(phNciNfc_RfConfigInfo_t));
    }
    else
    {
        phNciNfc_CmpPollParamsSetConfigOpt(pSetConfigParams,pGetConfigParams);
        phNciNfc_CmpLstnParamsSetConfigOpt(pSetConfigParams,pGetConfigParams);
        phNciNfc_CmpCommonParamsSetConfigOpt(pSetConfigParams,pGetConfigParams);
    }
}

static void phNciNfc_CmpPollParamsSetConfigOpt(phNciNfc_RfDiscConfigParams_t *pSetConfigParams,
                                               phNciNfc_RfDiscConfigParams_t *pGetConfigParams)
{
    /*If set config param and its values are same as get config param response
      then disable the bitfield of that param*/
    if(pSetConfigParams->tConfigInfo.PollNfcAConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tPollNfcADiscParams),
                                (void*)&(pGetConfigParams->tPollNfcADiscParams),
                                sizeof(phNciNfc_PollNfcADiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.PollNfcAConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tPollNfcADiscParams.PollNfcAConfig.Config.SetBailOut == 1)
            {
                if(pSetConfigParams->tPollNfcADiscParams.bBailOut ==
                   pGetConfigParams->tPollNfcADiscParams.bBailOut)
                {
                    pSetConfigParams->tPollNfcADiscParams.PollNfcAConfig.Config.SetBailOut = 0;
                }
            }
        }
    }
    if(pSetConfigParams->tConfigInfo.PollNfcBConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tPollNfcBDiscParams),
                                (void*)&(pGetConfigParams->tPollNfcBDiscParams),
                                sizeof(phNciNfc_PollNfcBDiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.PollNfcBConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config.SetBailOut == 1)
            {
                if(pSetConfigParams->tPollNfcBDiscParams.bBailOut ==
                   pGetConfigParams->tPollNfcBDiscParams.bBailOut)
                {
                    pSetConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config.SetBailOut = 0;
                }
            }
            if(pSetConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config.SetAfi == 1)
            {
                if(pSetConfigParams->tPollNfcBDiscParams.bAfi ==
                   pGetConfigParams->tPollNfcBDiscParams.bAfi)
                {
                    pSetConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config.SetAfi = 0;
                }
            }
        }
    }
    if(pSetConfigParams->tConfigInfo.PollNfcFConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tPollNfcFDiscParams),
                                (void*)&(pGetConfigParams->tPollNfcFDiscParams),
                                sizeof(phNciNfc_PollNfcFDiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.PollNfcFConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate == 1)
            {
                if(pSetConfigParams->tPollNfcFDiscParams.bBitRate ==
                   pGetConfigParams->tPollNfcFDiscParams.bBitRate)
                {
                    pSetConfigParams->tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate = 0;
                }
            }
        }
    }
    if(pSetConfigParams->tConfigInfo.PollIsoDepConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tPollIsoDepDiscParams),
                                (void*)&(pGetConfigParams->tPollIsoDepDiscParams),
                                sizeof(phNciNfc_PollIsoDepDiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.PollIsoDepConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetHigherLayerInfo == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tPollIsoDepDiscParams.aHigherLayerInfo),
                                        (void*)&(pGetConfigParams->tPollIsoDepDiscParams.aHigherLayerInfo),
                                        pGetConfigParams->tPollIsoDepDiscParams.bHigherLayerInfoSize) == 0)
                {
                    pSetConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetHigherLayerInfo = 0;
                }
            }
            if(pSetConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetBitRate == 1)
            {
                if(pSetConfigParams->tPollIsoDepDiscParams.bBitRate ==
                   pGetConfigParams->tPollIsoDepDiscParams.bBitRate)
                {
                    pSetConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetBitRate = 0;
                }
            }
        }
    }
    if(pSetConfigParams->tConfigInfo.PollNfcDepConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tPollNfcDepDiscParams),
                                (void*)&(pGetConfigParams->tPollNfcDepDiscParams),
                                sizeof(phNciNfc_PollNfcDepDiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.PollNfcDepConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetSpeed == 1)
            {
                if(pSetConfigParams->tPollNfcDepDiscParams.bNfcDepSpeed ==
                   pGetConfigParams->tPollNfcDepDiscParams.bNfcDepSpeed)
                {
                    pSetConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetSpeed = 0;
                }
            }
            if(pSetConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetGenBytes == 1)
            {
                if((pGetConfigParams->tPollNfcDepDiscParams.bAtrReqGeneBytesSize ==
                    pSetConfigParams->tPollNfcDepDiscParams.bAtrReqGeneBytesSize) &&
                   (phOsalNfc_MemCompare((void*)&(pSetConfigParams->tPollNfcDepDiscParams.aAtrReqGenBytes),
                                         (void*)&(pGetConfigParams->tPollNfcDepDiscParams.aAtrReqGenBytes),
                                         (pSetConfigParams->tPollNfcDepDiscParams.bAtrReqGeneBytesSize)) == 0))
                {
                    pSetConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetGenBytes = 0;
                }
            }
            if(pSetConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetAtrConfig == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tPollNfcDepDiscParams.AtrReqConfig),
                                        (void*)&(pGetConfigParams->tPollNfcDepDiscParams.AtrReqConfig),
                                        sizeof(pSetConfigParams->tPollNfcDepDiscParams.AtrReqConfig)) == 0)
                {
                    pSetConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetAtrConfig = 0;
                }
            }
        }
    }
}

static void phNciNfc_CmpLstnParamsSetConfigOpt(phNciNfc_RfDiscConfigParams_t *pSetConfigParams,
                                               phNciNfc_RfDiscConfigParams_t *pGetConfigParams)
{
    if(pSetConfigParams->tConfigInfo.LstnNfcAConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcADiscParams),
                                (void*)&(pGetConfigParams->tLstnNfcADiscParams),
                                sizeof(phNciNfc_LstnNfcADiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.LstnNfcAConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetBitFrameSdd == 1)
            {
                if(pSetConfigParams->tLstnNfcADiscParams.bBitFrameSDD ==
                   pGetConfigParams->tLstnNfcADiscParams.bBitFrameSDD)
                {
                    pSetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetBitFrameSdd = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetPlatformConfig == 1)
            {
                if(pSetConfigParams->tLstnNfcADiscParams.bPlatformConfig ==
                   pGetConfigParams->tLstnNfcADiscParams.bPlatformConfig)
                {
                    pSetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetPlatformConfig = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetSelInfo == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcADiscParams.SelInfo),
                                        (void*)&(pGetConfigParams->tLstnNfcADiscParams.SelInfo),
                                        sizeof(pGetConfigParams->tLstnNfcADiscParams.SelInfo)) == 0)
                {
                    pSetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetSelInfo = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetNfcID1 == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcADiscParams.aNfcID1),
                                        (void*)&(pGetConfigParams->tLstnNfcADiscParams.aNfcID1),
                                        (pGetConfigParams->tLstnNfcADiscParams.bNfcID1Size)) == 0)
                {
                    pSetConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetNfcID1 = 0;
                }
            }
        }
    }
    if(pSetConfigParams->tConfigInfo.LstnNfcBConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcBDiscParams),
                                (void*)&(pGetConfigParams->tLstnNfcBDiscParams),
                                sizeof(phNciNfc_LstnNfcBDiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.LstnNfcBConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSensBInfo == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcBDiscParams.SensBInfo),
                                        (void*)&(pGetConfigParams->tLstnNfcBDiscParams.SensBInfo),
                                        sizeof(pSetConfigParams->tLstnNfcBDiscParams.SensBInfo)) == 0)
                {
                    pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSensBInfo = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetNfcID0 == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcBDiscParams.aNfcID0),
                                        (void*)&(pGetConfigParams->tLstnNfcBDiscParams.aNfcID0),
                                        sizeof(pGetConfigParams->tLstnNfcBDiscParams.aNfcID0)) == 0)
                {
                    pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetNfcID0 = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAppData == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcBDiscParams.aAppData),
                                        (void*)&(pGetConfigParams->tLstnNfcBDiscParams.aAppData),
                                        sizeof(pGetConfigParams->tLstnNfcBDiscParams.aAppData)) == 0)
                {
                    pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAppData = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSfgi == 1)
            {
                if(pSetConfigParams->tLstnNfcBDiscParams.bSfgi ==
                   pGetConfigParams->tLstnNfcBDiscParams.bSfgi)
                {
                    pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSfgi = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAdcFo == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcBDiscParams.AdcFo),
                                        (void*)&(pGetConfigParams->tLstnNfcBDiscParams.AdcFo),
                                        sizeof(pGetConfigParams->tLstnNfcBDiscParams.AdcFo)) == 0)
                {
                    pSetConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAdcFo = 0;
                }
            }
        }
    }
    if(pSetConfigParams->tConfigInfo.LstnNfcFConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcFDiscParams),
                                (void*)&(pGetConfigParams->tLstnNfcFDiscParams),
                                sizeof(phNciNfc_LstnNfcFDiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.LstnNfcFConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetConfigBitRate == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcFDiscParams.ConfigBitRate),
                                        (void*)&(pGetConfigParams->tLstnNfcFDiscParams.ConfigBitRate),
                                        sizeof(pGetConfigParams->tLstnNfcFDiscParams.ConfigBitRate)) == 0)
                {
                    pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetConfigBitRate = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetProtocolType == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcFDiscParams.ProtocolType),
                                        (void*)&(pGetConfigParams->tLstnNfcFDiscParams.ProtocolType),
                                        sizeof(pGetConfigParams->tLstnNfcFDiscParams.ProtocolType)) == 0)
                {
                    pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetProtocolType = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tPmm == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcFDiscParams.aT3tPmm),
                                        (void*)&(pGetConfigParams->tLstnNfcFDiscParams.aT3tPmm),
                                        sizeof(pGetConfigParams->tLstnNfcFDiscParams.aT3tPmm)) == 0)
                {
                    pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tPmm = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tFlags == 1)
            {
                if(pSetConfigParams->tLstnNfcFDiscParams.wT3tFlags ==
                   pGetConfigParams->tLstnNfcFDiscParams.wT3tFlags)
                {
                    pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tFlags = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcFDiscParams.aT3tId),
                                        (void*)&(pGetConfigParams->tLstnNfcFDiscParams.aT3tId),
                                        sizeof(pGetConfigParams->tLstnNfcFDiscParams.aT3tId)) == 0)
                {
                    pSetConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId = 0;
                }
            }
        }
    }
    if(pSetConfigParams->tConfigInfo.LstnIsoDepConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnIsoDepDiscParams),
                                (void*)&(pGetConfigParams->tLstnIsoDepDiscParams),
                                sizeof(phNciNfc_LstnIsoDepDiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.LstnIsoDepConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetFwt == 1)
            {
                if(pSetConfigParams->tLstnIsoDepDiscParams.bFrameWaitingTime ==
                   pGetConfigParams->tLstnIsoDepDiscParams.bFrameWaitingTime)
                {
                    pSetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetFwt = 0;
                }
            }
            if(pSetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetLA_HistBytes == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnIsoDepDiscParams.aLA_HistBytes),
                                        (void*)&(pGetConfigParams->tLstnIsoDepDiscParams.aLA_HistBytes),
                                        (pSetConfigParams->tLstnIsoDepDiscParams.bHistBytesSize)) == 0)
                {
                    pSetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetLA_HistBytes = 0;
                }
            }
            if(pSetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetLB_HigherLayerResp == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnIsoDepDiscParams.aLB_HigherLayerResp),
                                        (void*)&(pGetConfigParams->tLstnIsoDepDiscParams.aLB_HigherLayerResp),
                                        (pSetConfigParams->tLstnIsoDepDiscParams.bHigherLayerRespInfoSize)) == 0)
                {
                    pSetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetLB_HigherLayerResp = 0;
                }
            }
            if(pSetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetbBitRate == 1)
            {
                if(pSetConfigParams->tLstnIsoDepDiscParams.bBitRate ==
                   pGetConfigParams->tLstnIsoDepDiscParams.bBitRate)
                {
                    pSetConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetbBitRate = 0;
                }
            }
        }
    }
    if(pSetConfigParams->tConfigInfo.LstnNfcDepConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcDepDiscParams),
                                (void*)&(pGetConfigParams->tLstnNfcDepDiscParams),
                                sizeof(phNciNfc_LstnNfcDepDiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.LstnNfcDepConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetWT == 1)
            {
                if(pSetConfigParams->tLstnNfcDepDiscParams.bWaitingTime ==
                   pGetConfigParams->tLstnNfcDepDiscParams.bWaitingTime)
                {
                    pSetConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetWT = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetGenBytes == 1)
            {
                if((pGetConfigParams->tLstnNfcDepDiscParams.bAtrResGenBytesSize ==
                    pSetConfigParams->tLstnNfcDepDiscParams.bAtrResGenBytesSize) &&
                   (phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcDepDiscParams.aAtrResGenBytes),
                                         (void*)&(pGetConfigParams->tLstnNfcDepDiscParams.aAtrResGenBytes),
                                         (pSetConfigParams->tLstnNfcDepDiscParams.bAtrResGenBytesSize)) == 0))
                {
                    pSetConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetGenBytes = 0;
                }
            }
            if(pSetConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetAtrRespConfig == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tLstnNfcDepDiscParams.AtrRespConfig),
                                        (void*)&(pGetConfigParams->tLstnNfcDepDiscParams.AtrRespConfig),
                                        sizeof(pSetConfigParams->tLstnNfcDepDiscParams.AtrRespConfig)) == 0)
                {
                    pSetConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetAtrRespConfig = 0;
                }
            }
        }
    }
}

static void phNciNfc_CmpCommonParamsSetConfigOpt(phNciNfc_RfDiscConfigParams_t *pSetConfigParams,
                                                 phNciNfc_RfDiscConfigParams_t *pGetConfigParams)
{
    if(pSetConfigParams->tConfigInfo.CommonConfig == 1)
    {
        if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tCommonDiscParams),
                                (void*)&(pGetConfigParams->tCommonDiscParams),
                                sizeof(phNciNfc_CommonDiscParams_t)) == 0)
        {
            pSetConfigParams->tConfigInfo.CommonConfig = 0;
        }
        else
        {
            if(pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetTotalDuration == 1)
            {
                if(pSetConfigParams->tCommonDiscParams.wTotalDuration ==
                   pGetConfigParams->tCommonDiscParams.wTotalDuration)
                {
                    pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetTotalDuration = 0;
                }
            }
            if(pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetConDevLimit == 1)
            {
                if(pSetConfigParams->tCommonDiscParams.bConDevLimit ==
                   pGetConfigParams->tCommonDiscParams.bConDevLimit)
                {
                    pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetConDevLimit = 0;
                }
            }
            if(pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetRfFieldInfo == 1)
            {
                if(pSetConfigParams->tCommonDiscParams.bRfFieldInfo ==
                   pGetConfigParams->tCommonDiscParams.bRfFieldInfo)
                {
                    pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetRfFieldInfo = 0;
                }
            }
            if(pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetRfNfceeAction == 1)
            {
                if(pSetConfigParams->tCommonDiscParams.bRfNfceeAction ==
                   pGetConfigParams->tCommonDiscParams.bRfNfceeAction)
                {
                    pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetRfNfceeAction = 0;
                }
            }
            if(pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetNfcDepOperationParam == 1)
            {
                if(phOsalNfc_MemCompare((void*)&(pSetConfigParams->tCommonDiscParams.NfcDepOperationParam),
                                        (void*)&(pGetConfigParams->tCommonDiscParams.NfcDepOperationParam),
                                        sizeof(pSetConfigParams->tCommonDiscParams.NfcDepOperationParam)) == 0)
                {
                    pSetConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetNfcDepOperationParam = 0;
                }
            }
        }
    }
}

static NFCSTATUS phNciNfc_GetConfigOpt(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreTxInfo_t tTxInfo;
    static pphNciNfc_RfDiscConfigParams_t pGetConfigParams = NULL;
    uint8_t bParamCount = 0;
    uint8_t bParamLen = 0;
    uint16_t wPayloadLen = 0;
    uint8_t *pPayloadBuff = NULL;
    uint8_t aRfConfigParams[PHNCINFC_RFCONFIG_TOTALPARAM_COUNT];
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t) pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        pGetConfigParams = (pphNciNfc_RfDiscConfigParams_t)
                           phOsalNfc_GetMemory(sizeof(phNciNfc_RfDiscConfigParams_t));

        pNciCtx->tSetConfOptInfo.pGetCfgParams = pGetConfigParams;

        if(NULL != pGetConfigParams)
        {
            /* Set allocated memory to Zero */
            phOsalNfc_SetMemory(pGetConfigParams,0,sizeof(phNciNfc_RfDiscConfigParams_t));
            phNciNfc_BuildGetConfigOpt(pNciCtx->tSetConfOptInfo.pSetConfParams, pGetConfigParams);
            wStatus = phNciNfc_GetRfParams(pGetConfigParams, aRfConfigParams, &bParamCount,&bParamLen);

            if( (NFCSTATUS_SUCCESS == wStatus) && (0 != bParamCount) )
            {
                /* Add the parameter count field to payload length*/
                wPayloadLen = bParamLen + 1;
                pPayloadBuff = (uint8_t *) phOsalNfc_GetMemory((uint32_t)wPayloadLen);

                if(NULL != pPayloadBuff)
                {
                    /* Copy the parameters length and payload parameters */
                    *pPayloadBuff = bParamCount;
                    phOsalNfc_MemCopy( (pPayloadBuff + 1),aRfConfigParams,bParamLen);

                    pNciCtx->tSendPayload.pBuff = pPayloadBuff;
                    pNciCtx->tSendPayload.wPayloadSize = wPayloadLen;

                    pNciCtx->pUpperLayerInfo = (void *)pGetConfigParams;

                    /* Clear the memory where configurations shall be stored */
                    phOsalNfc_SetMemory(pGetConfigParams,0x00, sizeof(phNciNfc_RfDiscConfigParams_t));
                    /* Store the Number of Parameters requested to NFCC */
                    pNciCtx->tRfConfContext.bReqParamNum = bParamCount;
                    /* Store the Length of Parameters requested to NFCC */
                    pNciCtx->tRfConfContext.bReqParamLen = bParamLen;
                    /* Store the Payload buffer where parameters requested to NFCC are stored */
                    pNciCtx->tRfConfContext.pReqParamList = pPayloadBuff;
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED;
                    PH_LOG_NCI_CRIT_STR("Memory not available");
                }
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("Memory not available");
        }

        if(wStatus == NFCSTATUS_SUCCESS)
        {
            /* Clear the Tx information structure */
            phOsalNfc_SetMemory(&tTxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));

            tTxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
            tTxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
            tTxInfo.tHeaderInfo.Opcode_ID.OidType.NciCoreCmdOid = phNciNfc_e_NciCoreGetConfigCmdOid;

            tTxInfo.Buff = pNciCtx->tSendPayload.pBuff;
            tTxInfo.wLen = pNciCtx->tSendPayload.wPayloadSize;

            /* Sending command */
            wStatus = phNciNfc_CoreIfTxRx(&(pNciCtx->NciCoreContext), &tTxInfo,
                            &(pNciCtx->RspBuffInfo), 2000,
                            (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                            (void *)pNciCtx);
        }
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameter");
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_GetConfigOptRsp(void *pContext, NFCSTATUS wStatus)
{
    NFCSTATUS wGetStatus = NFCSTATUS_FAILED;
    phNciNfc_RfDiscConfigParams_t *pGetCfgInfo;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if( (NULL != pNciContext) && (NULL != pNciContext->RspBuffInfo.pBuff) &&\
                    (NFCSTATUS_SUCCESS == wStatus) )
    {
        /* Check whether All parameters are accepted by NFCC */
        switch(pNciContext->RspBuffInfo.pBuff[0])
        {
        case PH_NCINFC_STATUS_OK:
            /* Check whether All parameters requested are provided by NFCC */
            if(pNciContext->tRfConfContext.bReqParamNum ==
                    pNciContext->RspBuffInfo.pBuff[1])
            {
                wGetStatus = phNciNfc_ProcessRfParams(pContext,
                    /* Exclude the status and number of parameters byte to get
                       the Total length of TLVs */
                    (uint8_t)(pNciContext->RspBuffInfo.wLen - 2),
                             &pNciContext->RspBuffInfo.pBuff[2],TRUE);
                if(pNciContext->tRfConfContext.bReqParamNum == 0)
                {
                    wGetStatus = NFCSTATUS_SUCCESS;
                }
            }
            break;
        case PH_NCINFC_STATUS_MESSAGE_SIZE_EXCEEDED:
            /* Check whether More parameters are to be requested */
            if(pNciContext->tRfConfContext.bReqParamNum >=
                    pNciContext->RspBuffInfo.pBuff[1])
            {
                wGetStatus = phNciNfc_ProcessRfParams(pContext,
                    /* Exclude the status and number of parameters byte to get
                       the Total length of TLVs */
                                (uint8_t)(pNciContext->RspBuffInfo.wLen - 2),
                                &pNciContext->RspBuffInfo.pBuff[2],TRUE);
                if( (NFCSTATUS_SUCCESS == wGetStatus) &&
                    (pNciContext->tRfConfContext.bReqParamNum > 0) )
                {
                    wGetStatus = NFCSTATUS_SUCCESS;
                }
            }
            break;
        case PH_NCINFC_STATUS_INVALID_PARAM:
            /* Check whether All parameters passed are invalid */
            wGetStatus = phNciNfc_ProcessRfParams(pContext,
                /* Exclude the status and number of parameters byte to get
                   the Total length of TLVs */
                            (uint8_t)(pNciContext->RspBuffInfo.wLen - 2),
                            &pNciContext->RspBuffInfo.pBuff[2],FALSE);
            break;
        default:
            break;
        }

        if(NFCSTATUS_SUCCESS == wGetStatus)
        {
            /* Check whether All required parameters are provided by NFCC, if not
               another command needs to be sent */
            if(0 == pNciContext->tRfConfContext.bReqParamNum)
            {
                if(NULL != pNciContext->tSendPayload.pBuff)
                {
                    pNciContext->tSendPayload.pBuff = NULL;
                    pNciContext->tSendPayload.wPayloadSize = 0;
                }
                pGetCfgInfo = (pphNciNfc_RfDiscConfigParams_t)pNciContext->pUpperLayerInfo;
                phNciNfc_CmpParamsSetConfigOpt(pNciContext->tSetConfOptInfo.pSetConfParams,pGetCfgInfo);

                /* Clear the Context variables used for Get Config Params */
                wGetStatus = phNciNfc_ClearConfigParams(pNciContext);
            }
            else
            {
                /* Update the Buffer pointer where parameters are updated */
                pNciContext->tSendPayload.pBuff = pNciContext->tRfConfContext.pReqParamList;
                /* Payload size is Number of parameters + parameters */
                pNciContext->tSendPayload.wPayloadSize = pNciContext->tRfConfContext.bReqParamLen + 1;
                phNciNfc_RepeatSequenceSeq(pNciContext, pNciContext->pSeqHandler, 1);
            }
        }
    }
    else
    {
        /* Clear the Context variables used for Get Config Params */
        phNciNfc_ClearConfigParams(pNciContext);
        wGetStatus = wStatus;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wGetStatus;
}

static NFCSTATUS
phNciNfc_SetRtngConfig(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreTxInfo_t tTxInfo;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t) pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        /* Clear the Tx information structure */
        phOsalNfc_SetMemory(&tTxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));

        tTxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        tTxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
        tTxInfo.tHeaderInfo.Opcode_ID.OidType.RfMgtCmdOid = phNciNfc_e_RfMgtRfSetRoutingCmdOid;

        tTxInfo.Buff = pNciCtx->tSendPayload.pBuff;
        tTxInfo.wLen = pNciCtx->tSendPayload.wPayloadSize;

        /* Sending command */
        wStatus = phNciNfc_CoreIfTxRx(&(pNciCtx->NciCoreContext), &tTxInfo,
                        &(pNciCtx->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,
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

static NFCSTATUS
phNciNfc_SetRtngConfigRsp(void *pContext,
                               NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        /* Validate the length of the response received */
        if(1 == pNciContext->RspBuffInfo.wLen)
        {
            /*Check Status Byte*/
            if(pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
            {
                PH_LOG_NCI_INFO_STR("Listen mode routing config: SUCCESS");
                /* Set listen mode routing configuration success */
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Listen mode routing config: FAILED");
                /* Set Listen mode routing configuration failed */
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            /*wStatus = Invalid Paload Lenght*/
            wStatus = NFCSTATUS_FAILED;
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
phNciNfc_CompleteSetRtngConfig(void *pContext,
                            NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    uint8_t bNotifyCb = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        phNciNfc_FreeSendPayloadBuff(pNciCtx);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if(1 == pNciCtx->tRtngConfInfo.bMore)
            {
                wStatus = phNciNfc_SetRtngCmdHandler(pNciCtx);
                if(NFCSTATUS_PENDING != wStatus)
                {
                    /* Clear the Rtng config info */
                    pNciCtx->tRtngConfInfo.bMore = 0;
                    pNciCtx->tRtngConfInfo.bRtngEntryOffset = 0;
                    pNciCtx->tRtngConfInfo.bTotalNumEntries = 0;
                    //pNciCtx->tRtngConfInfo.pRtngConf = NULL;
                    pNciCtx->pUpperLayerInfo = NULL;
                    phNciNfc_Notify(pNciCtx, wStatus, NULL);

                }
            }
            else
            {
                /* Listen mode routing table successfully configured.
                Invoke upper layer's call back */
                bNotifyCb = 1;
            }
        }
        else
        {
            /* Set Listen mode routing table failed!.
            Invoking upper layer's call back function */
            bNotifyCb = 1;
        }
        if(1 == bNotifyCb)
        {
            /* Clear the Rtng config info */
            pNciCtx->tRtngConfInfo.bMore = 0;
            pNciCtx->tRtngConfInfo.bRtngEntryOffset = 0;
            pNciCtx->tRtngConfInfo.bTotalNumEntries = 0;
            pNciCtx->pUpperLayerInfo = NULL;
            phNciNfc_Notify(pNciCtx, wStatus, NULL);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_GetRtngConfig(void *pContext)
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
        tTxInfo.tHeaderInfo.Opcode_ID.OidType.RfMgtCmdOid = phNciNfc_e_RfMgtRfGetRoutingCmdOid;
        tTxInfo.Buff = NULL;
        tTxInfo.wLen = 0;

        /* Sending command */
        wStatus = phNciNfc_CoreIfTxRx(&(pNciCtx->NciCoreContext), &tTxInfo,
                        &(pNciCtx->RspBuffInfo),PHNCINFC_NCI_CMD_RSP_TIMEOUT,(pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                        (void *)pNciCtx);
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_GetRtngConfigRsp(void *pContext,
                               NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        /* Validate the length of the response received */
        if(1 == pNciContext->RspBuffInfo.wLen)
        {
            /*Check Status Byte*/
            if(pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
            {
                PH_LOG_NCI_INFO_STR("Get listen mode routing config: SUCCESS");
                /* Get listen mode routing configuration success, notifications shall follow */
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Get listen mode routing config: FAILED");
                /* Get Listen mode routing configuration failed */
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            /*wStatus = Invalid Paload Lenght*/
            wStatus = NFCSTATUS_FAILED;
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
phNciNfc_CompleteGetRtngConfig(void *pContext,
                            NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    phNciNfc_sCoreHeaderInfo_t tRegInfo = {0};

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        /* No memory allocated for send payload buffer */
        /* Register for Get listen mode routing notifications */
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Register for notifications */
            tRegInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            tRegInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
            tRegInfo.Opcode_ID.Val = phNciNfc_e_RfMgtRfGetListenModeRoutingNtfOid;

            wStatus = phNciNfc_CoreIfRegRspNtf((void*)&pNciCtx->NciCoreContext,
                                   &tRegInfo,
                                   (pphNciNfc_CoreIfNtf_t)&phNciNfc_GetRtngConfigNtfCb,
                                   (void *)pNciCtx);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                PH_LOG_NCI_INFO_STR("Get Listen mode routing notification registered");
                /* Clear Routing config info structure parameters (the routing config read from NFCC shall be
                   stored in this structure */
                pNciCtx->tRtngConfInfo.bMore = 0;
                pNciCtx->tRtngConfInfo.bTotalNumEntries = 0;
                pNciCtx->tRtngConfInfo.pRtngNtfPayload = NULL;
                pNciCtx->tRtngConfInfo.wPayloadSize = 0;
            }
            else if(NFCSTATUS_ALREADY_REGISTERED == wStatus)
            {
                PH_LOG_NCI_INFO_STR("Get Listen mode routing notification already registered");
                wStatus = NFCSTATUS_FAILED;
            }
            else if(NFCSTATUS_FAILED == wStatus)
            {
                PH_LOG_NCI_INFO_STR("Get Listen mode routing notification registration failed!");
            }
            else
            {
                /* Should never enter this 'else' case */
                PH_LOG_NCI_INFO_STR("Invalid parameter sent");
                wStatus = NFCSTATUS_FAILED;
            }
        }

        /* If Get listen mode routing response does not return SUCCESS or
           if notification registration fails, invoke upper layer's call back function */
        if(NFCSTATUS_SUCCESS != wStatus)
        {
            phNciNfc_Notify(pNciCtx, wStatus, NULL);
        }
        else
        {
            PH_LOG_NCI_INFO_STR("Waiting for Get listen mode routing notification...");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_GetRtngConfigNtfCb(void*     pContext,
                            void *pInfo,
                            NFCSTATUS status)
{
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t )pContext;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    uint8_t bCurPktMore = 0;
    uint8_t bCurPktPayloadLen = 0;
    uint8_t bNumEntries = 0;
    uint8_t bProcNtf = 0;
    uint8_t bNotify = 0;
    uint8_t bCopyNtfPayload = 0;
    NFCSTATUS wStatus = status;
    NFCSTATUS wDeRegNtf = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_RtngConfInfo_t pRtngConfInfo = NULL;
    phNciNfc_GetRtngConfig_t tGetRtngConfig;
    phNciNfc_CoreRegInfo_t tNtfInfo;
    uint8_t bNoDealloc = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciCtx) && (NULL != pTransInfo))
    {
        pRtngConfInfo = &pNciCtx->tRtngConfInfo;
        if((pTransInfo->wLength >= PHNCINFC_RFCONFIG_LSTN_RTNG_DEFAULT_SIZE) &&
            (NFCSTATUS_SUCCESS == wStatus))
        {
            /* Get more field of the current packet */
            bCurPktMore = pTransInfo->pbuffer[0];
            bNumEntries = pTransInfo->pbuffer[1];
            bCurPktPayloadLen = (uint8_t )pTransInfo->wLength - PHNCINFC_RFCONFIG_LSTN_RTNG_DEFAULT_SIZE;

            /* If we are receiving only one get listen mode routing notification message with more set to '0',
               no need to allocate buffer for storing notificaiton message's payload field */
            if((0 == pRtngConfInfo->bMore) && (0 == bCurPktMore))
            {
                /* Received only one Listen mode routing notification with more set to '0' */
                PH_LOG_NCI_INFO_STR("Only one notification is received to read listen mode routing table");
                /* Update more field and total number of routing table entries */
                pRtngConfInfo->bMore = bCurPktMore;
                pRtngConfInfo->bTotalNumEntries = bNumEntries;

                /* If the Listen Mode Routing Table is empty, the value of 'Num of Rtng Entries' field is 0x00
                   and there are no Routing Entries */
                if(0 != bNumEntries)
                {
                    pRtngConfInfo->pRtngNtfPayload = &pTransInfo->pbuffer[2];
                    /* Excluding length of 'more' and 'NumOfRtngEntries' fields */
                    pRtngConfInfo->wPayloadSize = bCurPktPayloadLen;
                }
                else
                {
                    /* Listen mode routing table is empty */
                    PH_LOG_NCI_INFO_STR("Listen mode routing table is empty. No of entries: 0");
                }
                /* Process notification */
                bProcNtf = 1;
                bNoDealloc = 1;
            }
            /* If we are receiving first listen mode routing notification message (with more = 1), allocate memory
               for storing payload of multiple notifications which are about to be received */
            else if((0 == pRtngConfInfo->bMore) && (1 == bCurPktMore))
            {
                /* Received first Lstn Rtng Ntf with more set to '1' */
                PH_LOG_NCI_INFO_STR("Received first Rtng ntf with more set to '1'");
                /* Maximum routing size is informed to DH through Init response
                   Allocate memory for notif payload buffer (size = max rtng table size) */
                pRtngConfInfo->pRtngNtfPayload = phOsalNfc_GetMemory(
                                      pNciCtx->InitRspParams.RoutingTableSize);
                if(NULL == pRtngConfInfo->pRtngNtfPayload)
                {
                    PH_LOG_NCI_CRIT_STR("Failed to allocate memory for storing notification payload");
                    wStatus = NFCSTATUS_FAILED;
                }
                /* Update more field */
                pRtngConfInfo->bMore = bCurPktMore;
                bCopyNtfPayload = 1;
            }
            /* Receiving multiple notification messages */
            else if((1 == pRtngConfInfo->bMore) && (1 == bCurPktMore))
            {
                PH_LOG_NCI_INFO_STR("Received next Rtng ntf with more set to '1'");
                /* Update more field */
                pRtngConfInfo->bMore = bCurPktMore;
                /* Copy the received notificaiton payload field to the notificaiton payload buffer */
                bCopyNtfPayload = 1;
            }
            /* Received last notification message */
            else /* (1 == pRtngConfInfo->bMore) && (0 == bCurPktMore) */
            {
                PH_LOG_NCI_INFO_STR("Received last Rtng ntf with more set to '0'");
                /* Update more field */
                pRtngConfInfo->bMore = bCurPktMore;
                /* Copy the received notificaiton payload field to the notificaiton payload buffer */
                bCopyNtfPayload = 1;
                /* Process notification */
                bProcNtf = 1;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Invalid notification payload length!");
            /* Invalid Length of notification message payload */
            wStatus = NFCSTATUS_FAILED;
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if((1 == bCopyNtfPayload) && (NULL != pRtngConfInfo) && (NULL != pRtngConfInfo->pRtngNtfPayload))
            {
                PH_LOG_NCI_INFO_STR("Copying received Rtng ntf payload to ntf buffer");
                /* Copy the received ntf payload to the ntf buffer */
                phOsalNfc_MemCopy(&(pRtngConfInfo->pRtngNtfPayload[pRtngConfInfo->wPayloadSize]),
                                  &(pTransInfo->pbuffer[2]),bCurPktPayloadLen);
                /* Update number of entries and total size of ntf buffer */
                pRtngConfInfo->wPayloadSize += bCurPktPayloadLen;
                pRtngConfInfo->bTotalNumEntries += bNumEntries;
            }
            if(1 == bProcNtf)
            {
                PH_LOG_NCI_INFO_STR("Processing Rtng ntf...");
                wStatus = phNciNfc_ProcessGetRtngNtf(pRtngConfInfo,&tGetRtngConfig);
                if(NFCSTATUS_FAILED == wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("Processing failed!");
                    /* Number of entries shall be set to 0 */
                    tGetRtngConfig.bNumRtngConfigs = 0;
                }
                bNotify = 1;
            }
        }
        else
        {
            /* Number of entries shall be set to 0 */
            tGetRtngConfig.bNumRtngConfigs = 0;
            /* Notify Upper layer */
            bNotify = 1;
        }

        if(1 == bNotify)
        {
            /* Deregister ntf call back */
            tNtfInfo.bGid = phNciNfc_e_CoreRfMgtGid;
            tNtfInfo.bOid = phNciNfc_e_RfMgtRfGetListenModeRoutingNtfOid;
            tNtfInfo.pContext = (void *)pNciCtx;
            tNtfInfo.pNotifyCb = (pphNciNfc_CoreIfNtf_t)&phNciNfc_GetRtngConfigNtfCb;
            wDeRegNtf = phNciNfc_CoreRecvMgrDeRegisterCb((void*)&pNciCtx->NciCoreContext, &tNtfInfo,
                                    phNciNfc_e_NciCoreMsgTypeCntrlNtf);
            if(NFCSTATUS_SUCCESS == wDeRegNtf)
            {
                PH_LOG_NCI_INFO_STR("De-register Rtng ntf call back success");
            }
            else if(NFCSTATUS_NOT_REGISTERED == wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Rtng Notificaion call back not registered!");
            }
            else if(NFCSTATUS_FAILED == wStatus)
            {
                PH_LOG_NCI_CRIT_STR("De-register Rtng Notificaion call back failed!");
            }
            else /* (NFCSTATUS_INVALID_PARAMETER == wStatus) */
            {
                PH_LOG_NCI_CRIT_STR("Invalid parameter passed!");
            }
            /* Dealloc any memory if allocated and reset the parameters */
            if((NULL != pRtngConfInfo) && (NULL != pRtngConfInfo->pRtngNtfPayload))
            {
                if(0 == bNoDealloc)
                {
                    /* Deallocating memory allocated for storing received ntf's payload */
                    phOsalNfc_FreeMemory(pRtngConfInfo->pRtngNtfPayload);
                }
                pRtngConfInfo->pRtngNtfPayload = NULL;
                pRtngConfInfo->wPayloadSize = 0;
                pRtngConfInfo->bTotalNumEntries = 0;
                pRtngConfInfo->bMore = 0;
            }

            PH_LOG_NCI_INFO_STR("Notifying upper layer...");
            /* Notify upper layer */
            phNciNfc_Notify(pNciCtx, wStatus,(void*)&tGetRtngConfig);
            if(NULL != tGetRtngConfig.pRtngConfig)
            {
                phOsalNfc_FreeMemory(tGetRtngConfig.pRtngConfig);
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ProcessGetRtngNtf(pphNciNfc_RtngConfInfo_t pRtngConfInfo,
                           pphNciNfc_GetRtngConfig_t pGetRtngConfig)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_TlvUtilInfo_t tTlvInfo;
    uint8_t bNumEntries =  pRtngConfInfo->bTotalNumEntries;
    uint8_t bType = 0;
    uint8_t bLen = 0;
    uint8_t *pValue = NULL;
    uint8_t bCount = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    pGetRtngConfig->bNumRtngConfigs = 0;
    if(0 != bNumEntries)
    {
        /* Parse the buffer */
        wStatus = phNciNfc_TlvUtilsValidate(pRtngConfInfo->pRtngNtfPayload, pRtngConfInfo->wPayloadSize, NULL);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Valid TLVs received */
            /* Allocate memory for entries */
            pGetRtngConfig->pRtngConfig = (pphNciNfc_RtngConfig_t)phOsalNfc_GetMemory(bNumEntries *
                                                            sizeof(phNciNfc_RtngConfig_t));
            if(NULL == pGetRtngConfig->pRtngConfig)
            {
                PH_LOG_NCI_CRIT_STR("Failed to allocate trans info buffer!");
                wStatus = NFCSTATUS_FAILED;
            }
            else
            {
                pGetRtngConfig->bNumRtngConfigs = bNumEntries;
            }
        }
        else if(NFCSTATUS_FAILED == wStatus)
        {
            PH_LOG_NCI_CRIT_STR("Inconsistent TLVs received!");
        }
        else
        {
            /* Will never enter this 'else' case */
            PH_LOG_NCI_CRIT_STR("Invalid input parameter sent!");
            wStatus = NFCSTATUS_FAILED;
        }
    }

    if((NFCSTATUS_SUCCESS == wStatus) && (0 != bNumEntries))
    {
        tTlvInfo.pBuffer = pRtngConfInfo->pRtngNtfPayload;
        tTlvInfo.dwLength = pRtngConfInfo->wPayloadSize;
        tTlvInfo.sizeInfo.dwOffset = 0;

        /* Build output*/
        for(bCount = 0; bCount < bNumEntries; bCount++)
        {
            wStatus = phNciNfc_TlvUtilsGetNxtTlv(&tTlvInfo,&bType,&bLen,&pValue);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                phNciNfc_ProcessRtngEntry(bType,bLen,pValue,&pGetRtngConfig->pRtngConfig[bCount]);
            }
            else if(NFCSTATUS_FAILED == wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Get Tlv failed!");
                break;
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Invalid parameter passed");
                wStatus = NFCSTATUS_FAILED;
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void
phNciNfc_ProcessRtngEntry(uint8_t bType,
                             uint8_t bLen,
                             uint8_t *pValue,
                             pphNciNfc_RtngConfig_t pRtngConf)
{
    uint8_t bAidLen = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    switch(bType)
    {
        case phNciNfc_e_LstnModeRtngTechBased:
            pRtngConf->Type = phNciNfc_e_LstnModeRtngTechBased;
            pRtngConf->Route = pValue[0];

            pRtngConf->PowerState.bSwitchedOn = PHNCINFC_RFCONFIG_GET_SW_ON(pValue[1]);
            pRtngConf->PowerState.bSwitchedOff = PHNCINFC_RFCONFIG_GET_SW_OFF(pValue[1]);
            pRtngConf->PowerState.bBatteryOff = PHNCINFC_RFCONFIG_GET_BATT_OFF(pValue[1]);
            pRtngConf->PowerState.bSwitchedOnSub1 = PHNCINFC_RFCONFIG_GET_SW_ON_SUB1(pValue[1]);
            pRtngConf->PowerState.bSwitchedOnSub2 = PHNCINFC_RFCONFIG_GET_SW_ON_SUB2(pValue[1]);
            pRtngConf->PowerState.bSwitchedOnSub3 = PHNCINFC_RFCONFIG_GET_SW_ON_SUB3(pValue[1]);

            pRtngConf->LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology = (phNciNfc_RfTechnologies_t)pValue[2];
            break;
        case phNciNfc_e_LstnModeRtngProtocolBased:
            pRtngConf->Type = phNciNfc_e_LstnModeRtngProtocolBased;
            pRtngConf->Route = pValue[0];

            pRtngConf->PowerState.bSwitchedOn = PHNCINFC_RFCONFIG_GET_SW_ON(pValue[1]);
            pRtngConf->PowerState.bSwitchedOff = PHNCINFC_RFCONFIG_GET_SW_OFF(pValue[1]);
            pRtngConf->PowerState.bBatteryOff = PHNCINFC_RFCONFIG_GET_BATT_OFF(pValue[1]);
            pRtngConf->PowerState.bSwitchedOnSub1 = PHNCINFC_RFCONFIG_GET_SW_ON_SUB1(pValue[1]);
            pRtngConf->PowerState.bSwitchedOnSub2 = PHNCINFC_RFCONFIG_GET_SW_ON_SUB2(pValue[1]);
            pRtngConf->PowerState.bSwitchedOnSub3 = PHNCINFC_RFCONFIG_GET_SW_ON_SUB3(pValue[1]);

            pRtngConf->LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol = (phNciNfc_RfProtocols_t)pValue[2];
            break;
        case phNciNfc_e_LstnModeRtngAidBased:
            pRtngConf->Type = phNciNfc_e_LstnModeRtngAidBased;
            pRtngConf->Route = pValue[0];

            pRtngConf->PowerState.bSwitchedOn = PHNCINFC_RFCONFIG_GET_SW_ON(pValue[1]);
            pRtngConf->PowerState.bSwitchedOff = PHNCINFC_RFCONFIG_GET_SW_OFF(pValue[1]);
            pRtngConf->PowerState.bBatteryOff = PHNCINFC_RFCONFIG_GET_BATT_OFF(pValue[1]);
            pRtngConf->PowerState.bSwitchedOnSub1 = PHNCINFC_RFCONFIG_GET_SW_ON_SUB1(pValue[1]);
            pRtngConf->PowerState.bSwitchedOnSub2 = PHNCINFC_RFCONFIG_GET_SW_ON_SUB2(pValue[1]);
            pRtngConf->PowerState.bSwitchedOnSub3 = PHNCINFC_RFCONFIG_GET_SW_ON_SUB3(pValue[1]);

            bAidLen = (bLen - PHNCINFC_RFCONFIG_AID_VALUE_DEFAULT_LEN);
            if(bAidLen > PH_NCINFC_MAX_AID_LEN)
            {
                bAidLen = PH_NCINFC_MAX_AID_LEN;
            }
            phOsalNfc_MemCopy(pRtngConf->LstnModeRtngValue.tAidBasedRtngValue.aAid,&pValue[2],bAidLen);
            pRtngConf->LstnModeRtngValue.tAidBasedRtngValue.bAidSize = bAidLen;
            break;
        default:
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

static NFCSTATUS
phNciNfc_RfParamUpdate(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreTxInfo_t tTxInfo;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t) pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        /* Clear the Tx information structure */
        phOsalNfc_SetMemory(&tTxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));

        tTxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        tTxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
        tTxInfo.tHeaderInfo.Opcode_ID.OidType.RfMgtCmdOid = phNciNfc_e_RfMgtRfParamUpdateCmdOid;

        tTxInfo.Buff = pNciCtx->tSendPayload.pBuff;
        tTxInfo.wLen = pNciCtx->tSendPayload.wPayloadSize;

        /* Sending command */
        wStatus = phNciNfc_CoreIfTxRx(&(pNciCtx->NciCoreContext), &tTxInfo,
                        &(pNciCtx->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,(pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                        (void *)pNciCtx);
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameter");
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_RfParamUpdateRsp(void *pContext,
                      NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        /* Validate the length of the response received */
        if(pNciContext->RspBuffInfo.wLen >= 1 && pNciContext->RspBuffInfo.pBuff != NULL)
        {
            /*Check Status Byte*/
            if(pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
            {
                /* Set Set Config success */
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Rf parameter update failed!");
                /* Set Config failed */
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            /*wStatus = Invalid Paload Lenght*/
            wStatus = NFCSTATUS_FAILED;
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
phNciNfc_CompleteRfParamUpdate(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    /*Dealocate resource allocate before init sequnce*/
    if(NULL != pNciCtx)
    {
        phNciNfc_FreeSendPayloadBuff(pNciCtx);
        phNciNfc_Notify(pNciCtx, wStatus, NULL);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_BuildSetConfCmdPayload(pphNciNfc_RfDiscConfigParams_t pDiscConfig,
                                pphNciNfc_TlvUtilInfo_t pTlvInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t bReturnStat = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Check if Poll Nfc-A technology parameters needs to be configured */
    if(1 == pDiscConfig->tConfigInfo.PollNfcAConfig)
    {
        bReturnStat = phNciNfc_BuildPollNfcAParams(pTlvInfo,&pDiscConfig->tPollNfcADiscParams);
    }

    /* Check if Poll Nfc-B technology parameters needs to be configured */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.PollNfcBConfig))
    {
        bReturnStat = phNciNfc_BuildPollNfcBParams(pTlvInfo,&pDiscConfig->tPollNfcBDiscParams);
    }

    /* Check if Poll Nfc-F technology parameters needs to be configured */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.PollNfcFConfig))
    {
        bReturnStat = phNciNfc_BuildPollNfcFParams(pTlvInfo,&pDiscConfig->tPollNfcFDiscParams);
    }

    /* Check if Poll Iso-Dep technology parameters needs to be configured */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.PollIsoDepConfig))
    {
        bReturnStat = phNciNfc_BuildPollIsoDepParams(pTlvInfo,&pDiscConfig->tPollIsoDepDiscParams);
    }

    /* Check if Poll Nfc-Dep technology parameters needs to be configured */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.PollNfcDepConfig))
    {
        bReturnStat = phNciNfc_BuildPollNfcDepParams(pTlvInfo,&pDiscConfig->tPollNfcDepDiscParams);
    }

    /* Check if Lstn Nfc-A technology parameters needs to be configured */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.LstnNfcAConfig))
    {
        bReturnStat = phNciNfc_BuildLstnNfcAParams(pTlvInfo,&pDiscConfig->tLstnNfcADiscParams);
    }

    /* Check if Lstn Nfc-B technology parameters needs to be configured */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.LstnNfcBConfig))
    {
        bReturnStat = phNciNfc_BuildLstnNfcBParams(pTlvInfo,&pDiscConfig->tLstnNfcBDiscParams);
    }

    /* Check if Lstn Nfc-F technology parameters needs to be configured */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.LstnNfcFConfig))
    {
        bReturnStat = phNciNfc_BuildLstnNfcFParams(pTlvInfo,&pDiscConfig->tLstnNfcFDiscParams);
    }

    /* Check if Lstn Iso-Dep technology parameters needs to be configured */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.LstnIsoDepConfig))
    {
        bReturnStat = phNciNfc_BuildLstnIsoDepParams(pTlvInfo,&pDiscConfig->tLstnIsoDepDiscParams);
    }

    /* Check if Lstn Nfc-Dep technology parameters needs to be configured */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.LstnNfcDepConfig))
    {
        bReturnStat = phNciNfc_BuildLstnNfcDepParams(pTlvInfo,&pDiscConfig->tLstnNfcDepDiscParams);
    }

    /* If Common discovery parameters configuration is required */
    if((0 == bReturnStat) && (1 == pDiscConfig->tConfigInfo.CommonConfig))
    {
        bReturnStat = phNciNfc_BuildCommonParams(pTlvInfo,&pDiscConfig->tCommonDiscParams);
    }

    if(1 == bReturnStat)
    {
       PH_LOG_NCI_CRIT_STR("Framing failed!");
       wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bReturnStat;
}

static uint8_t
phNciNfc_BuildPollNfcAParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                             pphNciNfc_PollNfcADiscParams_t pPollNfcAParams)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(1 == pPollNfcAParams->PollNfcAConfig.Config.SetBailOut)
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_PA_BAIL_OUT,1,
                                                &pPollNfcAParams->bBailOut,0);
        if(NFCSTATUS_SUCCESS != bReturnStat)
        {
            PH_LOG_NCI_WARN_STR("Poll Nfc-A disc config framing failed");
            bStatus = 1;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildPollNfcAKovioParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
    pphNciNfc_PollNfcAKovioDiscParams_t pPollNfcAKovioParams)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if (1 == pPollNfcAKovioParams->PollNfcAKovioConfig.Config.SetBailOut)
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo, PHNCINFC_RFCONFIG_PA_BAIL_OUT, 1,
            &pPollNfcAKovioParams->bBailOut, 0);
        if (NFCSTATUS_SUCCESS != bReturnStat)
        {
            PH_LOG_NCI_WARN_STR("Poll Nfc-A Kovio disc config framing failed");
            bStatus = 1;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}


static uint8_t
phNciNfc_BuildPollNfcBParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                             pphNciNfc_PollNfcBDiscParams_t pPollNfcBParams)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 1;

    PH_LOG_NCI_FUNC_ENTRY();
    if(1 == pPollNfcBParams->PollNfcBConfig.Config.SetAfi)
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_PB_AFI,1,
                                                &pPollNfcBParams->bAfi,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) && (1 == pPollNfcBParams->PollNfcBConfig.Config.SetBailOut))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_PB_BAIL_OUT,1,
                                &pPollNfcBParams->bBailOut,0);
    }

    if(NFCSTATUS_SUCCESS == bReturnStat)
    {
        bStatus = 0;
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Poll Nfc-B disc config framing failed");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildPollNfcFParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                             pphNciNfc_PollNfcFDiscParams_t pPollNfcFParams)
{
    uint8_t bReturnStat = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bStatus = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(1 == pPollNfcFParams->PollNfcFConfig.Config.SetBitRate)
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_PF_BIT_RATE,1,
                            &pPollNfcFParams->bBitRate,0);
    }
    if(NFCSTATUS_SUCCESS != bReturnStat)
    {
        PH_LOG_NCI_WARN_STR("Poll Nfc-F disc config framing failed");
        bStatus = 1;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildPollIsoDepParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                               pphNciNfc_PollIsoDepDiscParams_t pPollIsoDepParams)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 1;

    PH_LOG_NCI_FUNC_ENTRY();
    if(1 == pPollIsoDepParams->PollIsoDepConfig.Config.SetBitRate)
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_PI_BIT_RATE,1,
                                            &pPollIsoDepParams->bBitRate,0);
    }
    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pPollIsoDepParams->PollIsoDepConfig.Config.SetHigherLayerInfo))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_PB_H_INFO,pPollIsoDepParams->bHigherLayerInfoSize,
                                pPollIsoDepParams->aHigherLayerInfo,0);
    }
    if(NFCSTATUS_SUCCESS == bReturnStat)
    {
        bStatus = 0;
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Poll Iso-Dep disc config framing failed");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildPollNfcDepParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                               pphNciNfc_PollNfcDepDiscParams_t pPollNfcDepParams)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 1;
    uint8_t bAtrReqConf = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Since there exist some parameters (atleast 1) that needs to be configured
       (identified in 'phNciNfc_ValidateSetConfParams' function), no need to have the same check again */
    if(1 == pPollNfcDepParams->PollNfcDepConfig.Config.bSetSpeed)
    {
        /* Add NfcDep Spped to the buffer */
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_BITR_NFC_DEP,1,
                                    &pPollNfcDepParams->bNfcDepSpeed,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) && (1 == pPollNfcDepParams->PollNfcDepConfig.Config.bSetGenBytes))
    {
        /* Add AtrReq General bytes to the buffer */
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_ATR_REQ_GEN_BYTES,pPollNfcDepParams->bAtrReqGeneBytesSize,
                                                pPollNfcDepParams->aAtrReqGenBytes,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) && (1 == pPollNfcDepParams->PollNfcDepConfig.Config.bSetAtrConfig))
    {
        bAtrReqConf |= (pPollNfcDepParams->AtrReqConfig.bDid << PHNCINFC_RFCONFIG_PNFCDEP_DID_BITMASK);
        bAtrReqConf |= (pPollNfcDepParams->AtrReqConfig.bLr << PHNCINFC_RFCONFIG_PNFCDEP_LR_BITMASK);

        /* Add AtrConfig to the buffer */
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_ATR_REQ_CONFIG,1,
                                            &bAtrReqConf,0);
    }

    if(NFCSTATUS_SUCCESS == bReturnStat)
    {
        bStatus = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildLstnNfcAParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,pphNciNfc_LstnNfcADiscParams_t pLstnNfcADiscConf)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 1;
    uint8_t bSelInfo = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(1 == pLstnNfcADiscConf->LstnNfcAConfig.Config.SetBitFrameSdd)
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LA_BIT_FRAME_SDD,1,
                        &pLstnNfcADiscConf->bBitFrameSDD,0);
    }
    if((NFCSTATUS_SUCCESS == bReturnStat) && (1 == pLstnNfcADiscConf->LstnNfcAConfig.Config.SetPlatformConfig))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LA_PLATFORM_CONFIG,1,
                    &pLstnNfcADiscConf->bPlatformConfig,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) && (1 == pLstnNfcADiscConf->LstnNfcAConfig.Config.SetSelInfo))
    {
        bSelInfo |= pLstnNfcADiscConf->SelInfo.bIsoDepProtoSupport << PHNCINFC_RFCONFIG_LNFCA_ISODEP_OFFSET;
        bSelInfo |= pLstnNfcADiscConf->SelInfo.bNfcDepProtoSupport << PHNCINFC_RFCONFIG_LNFCA_NFCDEP_OFFSET;

        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LA_SEL_INFO,1,
                             &bSelInfo,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) && (1 == pLstnNfcADiscConf->LstnNfcAConfig.Config.SetNfcID1))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LA_NFCID1,pLstnNfcADiscConf->bNfcID1Size,
                    pLstnNfcADiscConf->aNfcID1,0);
    }

    if(NFCSTATUS_SUCCESS == bReturnStat)
    {
        bStatus = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildLstnNfcBParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                             pphNciNfc_LstnNfcBDiscParams_t pLstnNfcBDiscConf)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 1;
    uint8_t bSensBInfo = 0;
    uint8_t bAdcFo = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(1 == pLstnNfcBDiscConf->LstnNfcBConfig.Config.SetSensBInfo)
    {
        bSensBInfo |= pLstnNfcBDiscConf->SensBInfo.bIsoDepProtocolSupport;
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LB_SENSB_INFO,1,
                        &bSensBInfo,0);
    }
    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnNfcBDiscConf->LstnNfcBConfig.Config.SetNfcID0))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LB_NFCID0,PH_NCINFC_NFCID0_LEN,
                        pLstnNfcBDiscConf->aNfcID0,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnNfcBDiscConf->LstnNfcBConfig.Config.SetAppData))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LB_APPLICATION_DATA,PH_NCINFC_APP_DATA_LEN,
                        pLstnNfcBDiscConf->aAppData,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnNfcBDiscConf->LstnNfcBConfig.Config.SetSfgi))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LB_SFGI,1,
                        &pLstnNfcBDiscConf->bSfgi,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnNfcBDiscConf->LstnNfcBConfig.Config.SetAdcFo))
    {
        bAdcFo |= pLstnNfcBDiscConf->AdcFo.bDid;
        bAdcFo |= pLstnNfcBDiscConf->AdcFo.bAdcCodingField << PHNCINFC_RFCONFIG_LNFCB_ADCFO_OFFSET;

        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LB_ADC_FO,1,
                        &pLstnNfcBDiscConf->bSfgi,0);
    }

    if(NFCSTATUS_SUCCESS == bReturnStat)
    {
        bStatus = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildLstnNfcFParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                             pphNciNfc_LstnNfcFDiscParams_t pLstnNfcFDiscConf)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 1;
    uint8_t bBitRate = 0;
    uint8_t bProtocolType = 0;
    uint16_t wBitMask = 1;
    uint8_t bIdCount = 0;
    uint8_t bT3tTag = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(1 == pLstnNfcFDiscConf->LstnNfcFConfig.Config.SetConfigBitRate)
    {
        bBitRate |= pLstnNfcFDiscConf->ConfigBitRate.bLstn212kbps << PHNCINFC_RFCONFIG_LNFCF_LSTN212_OFFSET;
        bBitRate |= pLstnNfcFDiscConf->ConfigBitRate.bLstn424kbps << PHNCINFC_RFCONFIG_LNFCF_LSTN424_OFFSET;
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LF_CON_BITR_F,1,
                                            &bBitRate,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnNfcFDiscConf->LstnNfcFConfig.Config.SetProtocolType))
    {
        bProtocolType |= pLstnNfcFDiscConf->ProtocolType.bNfcDepProtocolSupport <<
                                                PHNCINFC_RFCONFIG_LNFCF_NFCDEP_OFFSET;
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LF_PROTOCOL_TYPE,1,
                                            &bProtocolType,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnNfcFDiscConf->LstnNfcFConfig.Config.SetT3tPmm))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LF_T3T_PMM,PH_NCINFC_T3TPMM_LEN,
                            pLstnNfcFDiscConf->aT3tPmm,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnNfcFDiscConf->LstnNfcFConfig.Config.SetT3tFlags))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LF_T3T_FLAGS2,2,
                            (uint8_t *)&pLstnNfcFDiscConf->wT3tFlags,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnNfcFDiscConf->LstnNfcFConfig.Config.SetT3tId))
    {
        for(bIdCount = 0; bIdCount < PH_NCINFC_MAX_NUM_T3T_IDS; bIdCount++)
        {
            wBitMask = (wBitMask << bIdCount);
            if(pLstnNfcFDiscConf->wT3tFlags & wBitMask)
            {
                bT3tTag = phNciNfc_GetT3tTagId(bIdCount);
                bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,bT3tTag,PH_NCINFC_T3TID_LEN,
                                    pLstnNfcFDiscConf->aT3tId[bIdCount],0);
                if(NFCSTATUS_SUCCESS != bReturnStat)
                {
                    break;
                }
            }
        }
    }

    if(NFCSTATUS_SUCCESS == bReturnStat)
    {
        bStatus = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildLstnIsoDepParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                             pphNciNfc_LstnIsoDepDiscParams_t pLstnIsoDepDiscConf)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 1;

    PH_LOG_NCI_FUNC_ENTRY();
    if(1 == pLstnIsoDepDiscConf->LstnIsoDepConfig.Config.SetFwt)
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_FWI,1,
                            &pLstnIsoDepDiscConf->bFrameWaitingTime,0);
    }
    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnIsoDepDiscConf->LstnIsoDepConfig.Config.SetLA_HistBytes))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LA_HIST_BY,pLstnIsoDepDiscConf->bHistBytesSize,
                            pLstnIsoDepDiscConf->aLA_HistBytes,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnIsoDepDiscConf->LstnIsoDepConfig.Config.SetLB_HigherLayerResp))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LB_H_INFO_RESP,pLstnIsoDepDiscConf->bHigherLayerRespInfoSize,
                            pLstnIsoDepDiscConf->aLB_HigherLayerResp,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pLstnIsoDepDiscConf->LstnIsoDepConfig.Config.SetbBitRate))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_LI_BIT_RATE,1,
                            &pLstnIsoDepDiscConf->bBitRate,0);
    }

    if(NFCSTATUS_SUCCESS == bReturnStat)
    {
        bStatus = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildLstnNfcDepParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                             pphNciNfc_LstnNfcDepDiscParams_t pLstnNfcDepDiscConf)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 1;
    uint8_t bAtrResConf = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Since there exist some parameters (atleast 1) that needs to be configured
       (identified in 'phNciNfc_ValidateSetConfParams' function), no need to have the same check again */
    if(1 == pLstnNfcDepDiscConf->LstnNfcDepConfig.Config.bSetWT)
    {
        /* Add response waiting time to the buffer */
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_WT,1,
                            &pLstnNfcDepDiscConf->bWaitingTime,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) && (1 == pLstnNfcDepDiscConf->LstnNfcDepConfig.Config.bSetGenBytes))
    {
        /* Add AtrRsp General bytes to the buffer */
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_ATR_RES_GEN_BYTES,
                            pLstnNfcDepDiscConf->bAtrResGenBytesSize,pLstnNfcDepDiscConf->aAtrResGenBytes,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) && (1 == pLstnNfcDepDiscConf->LstnNfcDepConfig.Config.bSetAtrRespConfig))
    {
        /* Add AtrRes config to the buffer */
        bAtrResConf |= pLstnNfcDepDiscConf->AtrRespConfig.bLengthReduction << PHNCINFC_RFCONFIG_LNFCDEP_LR_OFFSET;
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_ATR_RES_CONFIG,1,
                            &bAtrResConf,0);
    }

    if(NFCSTATUS_SUCCESS == bReturnStat)
    {
        bStatus = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_BuildCommonParams(pphNciNfc_TlvUtilInfo_t pTlvInfo,
                             pphNciNfc_CommonDiscParams_t pDiscCommonConf)
{
    uint8_t bReturnStat = NFCSTATUS_SUCCESS;
    uint8_t bStatus = 1;
    uint8_t bNfcDepOpParam = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(1 == pDiscCommonConf->ComnParamsConfig.Config.SetTotalDuration)
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_TOTAL_DURATION,2,
                            (uint8_t *)&pDiscCommonConf->wTotalDuration,0);
    }
    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pDiscCommonConf->ComnParamsConfig.Config.SetConDevLimit))
    {
        if(0 != pDiscCommonConf->bConDevLimit)
        {
            bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_CON_DEVICES_LIMIT,1,
                            &pDiscCommonConf->bConDevLimit,0);
        }
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pDiscCommonConf->ComnParamsConfig.Config.SetRfFieldInfo))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_RF_FIELD_INFO,1,
                                &pDiscCommonConf->bRfFieldInfo,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pDiscCommonConf->ComnParamsConfig.Config.SetRfNfceeAction))
    {
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_RF_NFCEE_ACTION,1,
                            &pDiscCommonConf->bRfNfceeAction,0);
    }

    if((NFCSTATUS_SUCCESS == bReturnStat) &&
        (1 == pDiscCommonConf->ComnParamsConfig.Config.SetNfcDepOperationParam))
    {
        bNfcDepOpParam |= pDiscCommonConf->NfcDepOperationParam.bRtoxReq;
        bNfcDepOpParam |= pDiscCommonConf->NfcDepOperationParam.bAttentionCommand <<
                                                        PHNCINFC_RFCONFIG_NFCDEP_ATTCMD_OFFSET;
        bNfcDepOpParam |= pDiscCommonConf->NfcDepOperationParam.bInformationPdu <<
                                                        PHNCINFC_RFCONFIG_NFCDEP_INFPDU_OFFSET;
        bNfcDepOpParam |= pDiscCommonConf->NfcDepOperationParam.bUseMaxTxLen <<
                                                        PHNCINFC_RFCONFIG_NFCDEP_MAXBYTES_OFFSET;
        bReturnStat = phNciNfc_TlvUtilsAddTlv(pTlvInfo,PHNCINFC_RFCONFIG_NFCDEP_OP,1,
                                &bNfcDepOpParam,0);
    }

    if(NFCSTATUS_SUCCESS == bReturnStat)
    {
        bStatus = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_GetT3tTagId(uint8_t bCount)
{
    uint8_t bT3tTag = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    switch(bCount)
    {
        case PHNCINFC_RFCONFIG_T3TID1:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_1;
            break;
        case PHNCINFC_RFCONFIG_T3TID2:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_2;
            break;
        case PHNCINFC_RFCONFIG_T3TID3:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_3;
            break;
        case PHNCINFC_RFCONFIG_T3TID4:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_4;
            break;
        case PHNCINFC_RFCONFIG_T3TID5:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_5;
            break;
        case PHNCINFC_RFCONFIG_T3TID6:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_6;
            break;
        case PHNCINFC_RFCONFIG_T3TID7:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_7;
            break;
        case PHNCINFC_RFCONFIG_T3TID8:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_8;
            break;
        case PHNCINFC_RFCONFIG_T3TID9:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_9;
            break;
        case PHNCINFC_RFCONFIG_T3TID10:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_10;
            break;
        case PHNCINFC_RFCONFIG_T3TID11:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_11;
            break;
        case PHNCINFC_RFCONFIG_T3TID12:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_12;
            break;
        case PHNCINFC_RFCONFIG_T3TID13:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_13;
            break;
        case PHNCINFC_RFCONFIG_T3TID14:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_14;
            break;
        case PHNCINFC_RFCONFIG_T3TID15:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_15;
            break;
        case PHNCINFC_RFCONFIG_T3TID16:
            bT3tTag = PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_16;
            break;
        default:
            bT3tTag = 0;
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bT3tTag;
}

static NFCSTATUS
phNciNfc_ValidateCommonParams(pphNciNfc_CommonDiscParams_t pDiscCommConfig,
                                uint16_t *wSize,
                                uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(0 != pDiscCommConfig->ComnParamsConfig.EnableConfig)
    {
        wStatus = NFCSTATUS_SUCCESS;

        if(1 == pDiscCommConfig->ComnParamsConfig.Config.SetTotalDuration)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            /* Length of Total duration parameter */
            *wSize += PHNCINFC_RFCONFIG_TOTALDURATION_LEN;
        }

        if(1 == pDiscCommConfig->ComnParamsConfig.Config.SetConDevLimit)
        {
            /* If the requested connection devices limit is '0', set it to its default value */
            if(0 != pDiscCommConfig->bConDevLimit)
            {
                *pNumParams += 1;
                *wSize += PHNCINFC_RFCONFIG_CONDEVLIMIT_LEN;
            }
        }

        if(1 == pDiscCommConfig->ComnParamsConfig.Config.SetNfcDepOperationParam)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            /* Length of NfcDep Operation Parameters */
            *wSize += PHNCINFC_RFCONFIG_NFCDEPOP_LEN;
        }

        if(1 == pDiscCommConfig->ComnParamsConfig.Config.SetRfFieldInfo)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            if(1 >= pDiscCommConfig->bRfFieldInfo)
            {
                *wSize += PHNCINFC_RFCONFIG_RFFIELDINFO_LEN;
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }

        if((NFCSTATUS_INVALID_PARAMETER != wStatus) &&
            (1 == pDiscCommConfig->ComnParamsConfig.Config.SetRfNfceeAction))
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            if(1 >=  pDiscCommConfig->bRfNfceeAction)
            {
                *wSize += PHNCINFC_RFCONFIG_NFCEEACTION_LEN;
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidateLstnNfcAParams(pphNciNfc_LstnNfcADiscParams_t pLstnNfcAConf,
                                uint16_t *wSize,
                                uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(0 != pLstnNfcAConf->LstnNfcAConfig.EnableConfig)
    {
        wStatus = NFCSTATUS_SUCCESS;
        if(1 == pLstnNfcAConf->LstnNfcAConfig.Config.SetSelInfo)
        {
            *pNumParams += 1;
            /* Size of SEL_INFO parameter */
            *wSize += PHNCINFC_RFCONFIG_LNFCA_SELINFO_LEN;
        }

        if(1 == pLstnNfcAConf->LstnNfcAConfig.Config.SetBitFrameSdd)
        {
            *pNumParams += 1;
            *wSize += PHNCINFC_RFCONFIG_LNFCA_BITRATE_LEN;
        }

        if(1 == pLstnNfcAConf->LstnNfcAConfig.Config.SetPlatformConfig)
        {
            *pNumParams += 1;
            if(PHNCINFC_RFCONFIG_PLATFORM_CONF_MAX >= pLstnNfcAConf->bPlatformConfig)
            {
                *wSize += PHNCINFC_RFCONFIG_LNFCA_PLATCONFIG_LEN;
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }

        if((NFCSTATUS_INVALID_PARAMETER != wStatus) &&
           (1 == pLstnNfcAConf->LstnNfcAConfig.Config.SetNfcID1))
        {
            *pNumParams += 1;
            if((PHNCINFC_RFCONFIG_NFCID1_4BYTES == pLstnNfcAConf->bNfcID1Size) ||
                (PHNCINFC_RFCONFIG_NFCID1_7BYTES == pLstnNfcAConf->bNfcID1Size) ||
                (PHNCINFC_RFCONFIG_NFCID1_10BYTES == pLstnNfcAConf->bNfcID1Size))
            {
                *wSize += pLstnNfcAConf->bNfcID1Size;
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidateLstnNfcBParams(pphNciNfc_LstnNfcBDiscParams_t pLstnNfcBConf,
                                uint16_t *wSize,
                                uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(0 != pLstnNfcBConf->LstnNfcBConfig.EnableConfig)
    {
        wStatus = NFCSTATUS_SUCCESS;
        if(1 == pLstnNfcBConf->LstnNfcBConfig.Config.SetSensBInfo)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            /* Length of SENSB_INFO parameter */
            *wSize += PHNCINFC_RFCONFIG_LNFCB_SENSBINFO_LEN;
        }

        if(1 == pLstnNfcBConf->LstnNfcBConfig.Config.SetNfcID0)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            /* Length of NFCID0 parameter */
            *wSize += PH_NCINFC_NFCID0_LEN;
        }

        if(1 == pLstnNfcBConf->LstnNfcBConfig.Config.SetAppData)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            /* Length of Application Data parameter */
            *wSize += PH_NCINFC_APP_DATA_LEN;
        }

        if(1 == pLstnNfcBConf->LstnNfcBConfig.Config.SetSfgi)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            /* Length of SFGI parameter */
            *wSize += PHNCINFC_RFCONFIG_LNFCB_SFGI_LEN;
        }

        if(1 == pLstnNfcBConf->LstnNfcBConfig.Config.SetAdcFo)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            /* Length of ADC_FO parameter */
            *wSize += PHNCINFC_RFCONFIG_LNFCB_ADCFO_LEN;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidateLstnNfcFParams(pphNciNfc_LstnNfcFDiscParams_t pLstnNfcFConf,
                                uint16_t *wSize,
                                uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint16_t wBitMask = 1;
    uint8_t bIdCount = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(0 != pLstnNfcFConf->LstnNfcFConfig.EnableConfig)
    {
        wStatus = NFCSTATUS_SUCCESS;
        if(1 == pLstnNfcFConf->LstnNfcFConfig.Config.SetConfigBitRate)
        {
            *pNumParams += 1;
            /* Length of CON_BITR_F parameter */
            *wSize += PHNCINFC_RFCONFIG_LNFCF_BITRATE_LEN;
        }

        if(1 == pLstnNfcFConf->LstnNfcFConfig.Config.SetProtocolType)
        {
            *pNumParams += 1;
            /* Length of PROTOCOL_TYPE parameter */
            *wSize += PHNCINFC_RFCONFIG_LNFCF_PROTOTYPE_LEN;
        }

        if(1 == pLstnNfcFConf->LstnNfcFConfig.Config.SetT3tPmm)
        {
            *pNumParams += 1;
            /* Length of T3T_PMM parameter */
            *wSize += PH_NCINFC_T3TPMM_LEN;
        }

        if(1 == pLstnNfcFConf->LstnNfcFConfig.Config.SetT3tFlags)
        {
            *pNumParams += 1;
            /*Length of T3tFlags parameter*/
            *wSize += PHNCINFC_RFCONFIG_LNFCF_T3TFLAGS_LEN;
        }

        if(1 == pLstnNfcFConf->LstnNfcFConfig.Config.SetT3tId)
        {
            for(bIdCount = 0; bIdCount < PH_NCINFC_MAX_NUM_T3T_IDS; bIdCount++)
            {
                wBitMask = (wBitMask << bIdCount);
                if(pLstnNfcFConf->wT3tFlags & wBitMask)
                {
                    *pNumParams += 1;
                    *wSize += PH_NCINFC_T3TID_LEN;

                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidateLstnIsoDepParams(pphNciNfc_LstnIsoDepDiscParams_t pLstnIsoDepConf,
                                  uint16_t *wSize,
                                  uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(0 != pLstnIsoDepConf->LstnIsoDepConfig.EnableConfig)
    {
        wStatus = NFCSTATUS_SUCCESS;
        if(1 == pLstnIsoDepConf->LstnIsoDepConfig.Config.SetFwt)
        {
            /* Number of Parameters */
            *pNumParams += 1;
            /* Length of FWI parameter */
            *wSize += PHNCINFC_RFCONFIG_LISODEP_FWI_LEN;
        }

        if(1 == pLstnIsoDepConf->LstnIsoDepConfig.Config.SetLA_HistBytes)
        {
            /* Number of Parameters */
            *pNumParams += 1;
            /* Length of Hist Bytes parameter */
            *wSize += pLstnIsoDepConf->bHistBytesSize;
        }

        if(1 == pLstnIsoDepConf->LstnIsoDepConfig.Config.SetLB_HigherLayerResp)
        {
            /* Number of Parameters */
            *pNumParams += 1;
            /* Length of Higher Layer Rsp info parameter */
            *wSize += pLstnIsoDepConf->bHigherLayerRespInfoSize;
        }

        if(1 == pLstnIsoDepConf->LstnIsoDepConfig.Config.SetbBitRate)
        {
            /* Number of Parameters */
            *pNumParams += 1;
            if(phNciNfc_e_BitRate6784 >= pLstnIsoDepConf->bBitRate)
            {
                *wSize += PHNCINFC_RFCONFIG_LISODEP_BITRATE_LEN;
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidateLstnNfcDepParams(pphNciNfc_LstnNfcDepDiscParams_t pLstnNfcDepConf,
                                  uint16_t *wSize,
                                  uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(0 != pLstnNfcDepConf->LstnNfcDepConfig.EnableConfig)
    {
        wStatus = NFCSTATUS_SUCCESS;
        if(1 == pLstnNfcDepConf->LstnNfcDepConfig.Config.bSetAtrRespConfig)
        {
            *pNumParams += 1;
            *wSize += PHNCINFC_RFCONFIG_LNFCDEP_ATRRESCONFIG_LEN;
        }

        if(1 == pLstnNfcDepConf->LstnNfcDepConfig.Config.bSetWT)
        {
            *pNumParams += 1;
            /* Length of Waiting Time parameter */
            *wSize += PHNCINFC_RFCONFIG_LNFCDEP_WT_LEN;
        }

        if(1 == pLstnNfcDepConf->LstnNfcDepConfig.Config.bSetGenBytes)
        {
            *pNumParams += 1;
            /* Length of ATR General Bytes parameter */
            *wSize += pLstnNfcDepConf->bAtrResGenBytesSize;
        }
    }
    else
    {
         PH_LOG_NCI_WARN_STR("No Listen Nfc-Dep parameters are being requested by the user to configure");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidatePollNfcAParams(pphNciNfc_PollNfcADiscParams_t pPollNfcAConf,
                                uint16_t *wSize,
                                uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Check if user is requesting to configure any Poll Nfc-Dep parameters */
    if(0 != pPollNfcAConf->PollNfcAConfig.EnableConfig)
    {
        if(1 == pPollNfcAConf->PollNfcAConfig.Config.SetBailOut)
        {
            /* Number of configurable parameters in Poll Nfc-A technology */
            *pNumParams += PHNCINFC_RFCONFIG_PNFCA_PARAMS;
            /* Range of bail-out parameter is 0 or 1 */
            if(1 >= pPollNfcAConf->bBailOut)
            {
                wStatus = NFCSTATUS_SUCCESS;
                *wSize += PHNCINFC_RFCONFIG_PNFCA_BAILOUT_LEN;
            }
        }
    }
    else
    {
        PH_LOG_NCI_WARN_STR("No Poll Nfc-Dep parameters are being requested by the user to configure");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidatePollNfcAKovioParams(pphNciNfc_PollNfcAKovioDiscParams_t pPollNfcAKovioConf,
    uint16_t *wSize,
    uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Check if user is requesting to configure any Poll Kovio parameters */
    if (0 != pPollNfcAKovioConf->PollNfcAKovioConfig.EnableConfig)
    {
        if (1 == pPollNfcAKovioConf->PollNfcAKovioConfig.Config.SetBailOut)
        {
            /* Number of configurable parameters in Poll Nfc-A technology */
            *pNumParams += PHNCINFC_RFCONFIG_PNFCA_PARAMS;
            /* Range of bail-out parameter is 0 or 1 */
            if (1 >= pPollNfcAKovioConf->bBailOut)
            {
                wStatus = NFCSTATUS_SUCCESS;
                *wSize += PHNCINFC_RFCONFIG_PNFCA_BAILOUT_LEN;
            }
        }
    }
    else
    {
        PH_LOG_NCI_WARN_STR("No Poll Nfc-Kovio parameters are being requested by the user to configure");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidatePollNfcBParams(pphNciNfc_PollNfcBDiscParams_t pPollNfcBConf,
                                uint16_t *wSize,
                                uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(0 != pPollNfcBConf->PollNfcBConfig.EnableConfig)
    {
        wStatus = NFCSTATUS_SUCCESS;
        if(1 == pPollNfcBConf->PollNfcBConfig.Config.SetAfi)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            /*Length of Poll-B Afi parameter*/
            *wSize += PHNCINFC_RFCONFIG_PNFCB_AFI_LEN;
        }

        if(1 == pPollNfcBConf->PollNfcBConfig.Config.SetBailOut)
        {
            *pNumParams += 1;
            /* Range of bail-out parameter is 0 or 1 */
            if(1 >= pPollNfcBConf->bBailOut)
            {
                *wSize += PHNCINFC_RFCONFIG_PNFCB_BAILOUT_LEN;
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidatePollNfcFParams(pphNciNfc_PollNfcFDiscParams_t pPollNfcFConf,
                                uint16_t *wSize,
                                uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(0 != pPollNfcFConf->PollNfcFConfig.EnableConfig)
    {
        if(1 == pPollNfcFConf->PollNfcFConfig.Config.SetBitRate)
        {
            /* Number of configurable parameters */
            *pNumParams += PHNCINFC_RFCONFIG_PNFCF_PARAMS;
            *wSize += PHNCINFC_RFCONFIG_PNFCF_BITRATE_LEN;
            /* Bit rate range should be in the range of (212 to 424 Kbit/s)  */
            if(((phNciNfc_e_BitRate212 <= pPollNfcFConf->bBitRate) &&
               (phNciNfc_e_BitRate424 >= pPollNfcFConf->bBitRate)))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidatePollIsoDepParams(pphNciNfc_PollIsoDepDiscParams_t pPollIsoDepConf,
                                  uint16_t *wSize,
                                  uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(0 != pPollIsoDepConf->PollIsoDepConfig.EnableConfig)
    {
        wStatus = NFCSTATUS_SUCCESS;
        if(1 == pPollIsoDepConf->PollIsoDepConfig.Config.SetBitRate)
        {
            /* Number of configurable parameters */
            *pNumParams += 1;
            if(phNciNfc_e_BitRate848 >= pPollIsoDepConf->bBitRate)
            {
                *wSize += PHNCINFC_RFCONFIG_PISODEP_BITRATE_LEN;
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }

        if((NFCSTATUS_INVALID_PARAMETER != wStatus) &&
            (1 == pPollIsoDepConf->PollIsoDepConfig.Config.SetHigherLayerInfo))
        {
            *pNumParams += 1;
            *wSize += pPollIsoDepConf->bHigherLayerInfoSize;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ValidatePollNfcDepParams(pphNciNfc_PollNfcDepDiscParams_t pPollNfcDepConf,
                                  uint16_t *wSize,
                                  uint8_t *pNumParams)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Check if user is requesting to configure any Poll Nfc-Dep parameters */
    if(0 != pPollNfcDepConf->PollNfcDepConfig.EnableConfig)
    {
        wStatus = NFCSTATUS_SUCCESS;
        if(1 == pPollNfcDepConf->PollNfcDepConfig.Config.bSetSpeed)
        {
            if(1 >= pPollNfcDepConf->bNfcDepSpeed)
            {
                *pNumParams += 1;
                *wSize += PHNCINFC_RFCONFIG_PNFCDEP_SPEED_LEN;
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }

        if((NFCSTATUS_INVALID_PARAMETER != wStatus) &&
            (1 == pPollNfcDepConf->PollNfcDepConfig.Config.bSetAtrConfig))
        {
            *pNumParams += 1;
            *wSize += PHNCINFC_RFCONFIG_PNFCDEP_ATRCONFIG_LEN;
        }

        if((NFCSTATUS_INVALID_PARAMETER != wStatus) &&
            (1 == pPollNfcDepConf->PollNfcDepConfig.Config.bSetGenBytes))
        {
            *pNumParams += 1;
            *wSize += pPollNfcDepConf->bAtrReqGeneBytesSize;
        }
    }
    else
    {
        PH_LOG_NCI_WARN_STR("No Poll Nfc-Dep parameters are being requested by the user to configure");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static uint8_t
phNciNfc_WriteRoutingEntryToPayload(uint8_t *buffer,
                                    phNciNfc_RtngConfig_t *routingEntry)
{
    uint8_t offset = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    /* Type of routing entry (System Code based routing in this case) */
    buffer[offset++] = routingEntry->Type;
    /* Length of the value field (Length shall be in the range of 7-18 bytes) */
    buffer[offset++] = phNciNfc_EntryValueSize(routingEntry);
    /* Value field */
    buffer[offset++] = routingEntry->Route;
    /* Clear the power state byte */
    buffer[offset] = 0;
    PHNCINFC_RFCONFIG_SW_ON(&buffer[offset], routingEntry->PowerState.bSwitchedOn);
    PHNCINFC_RFCONFIG_SW_OFF(&buffer[offset], routingEntry->PowerState.bSwitchedOff);
    PHNCINFC_RFCONFIG_BATT_OFF(&buffer[offset], routingEntry->PowerState.bBatteryOff);
    PHNCINFC_RFCONFIG_SW_ON_SUB1(&buffer[offset], routingEntry->PowerState.bSwitchedOnSub1);
    PHNCINFC_RFCONFIG_SW_ON_SUB2(&buffer[offset], routingEntry->PowerState.bSwitchedOnSub2);
    PHNCINFC_RFCONFIG_SW_ON_SUB3(&buffer[offset], routingEntry->PowerState.bSwitchedOnSub3);
    offset++;

    switch (routingEntry->Type)
    {
    case phNciNfc_e_LstnModeRtngTechBased:
        buffer[offset++] = routingEntry->LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology;
        break;

    case phNciNfc_e_LstnModeRtngProtocolBased:
        buffer[offset++] = routingEntry->LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol;
        break;

    case phNciNfc_e_LstnModeRtngAidBased:
        phOsalNfc_MemCopy(&buffer[offset],
                          routingEntry->LstnModeRtngValue.tAidBasedRtngValue.aAid,
                          routingEntry->LstnModeRtngValue.tAidBasedRtngValue.bAidSize);

        offset += routingEntry->LstnModeRtngValue.tAidBasedRtngValue.bAidSize;
        break;

    default:
        PH_LOG_NCI_WARN_STR("Routing type is not supported!");
        break;
    }

    PH_LOG_NCI_FUNC_EXIT();
    return offset;
}

static NFCSTATUS
phNciNfc_ValidateRoutingEntryParams(pphNciNfc_RtngConfig_t pRoutingEntry, uint16_t *pSize)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    /* 'Route' (NFCEE ID or DH-NFCEE ID) should be in the range of 0-254 */
    if(PHNCINFC_RFCONFIG_NFCEE_ID_RFU != pRoutingEntry->Route)
    {
        switch(pRoutingEntry->Type)
        {
        case phNciNfc_e_LstnModeRtngTechBased:
        {
            /* Validate Technology based routing parameters */
            *pSize = (*pSize) + (PHNCINFC_RFCONFIG_TLV_HEADER_LEN) + (PHNCINFC_RFCONFIG_TECH_RTNG_VALUE_LEN);
            wStatus = NFCSTATUS_SUCCESS;
            break;
        }
        case phNciNfc_e_LstnModeRtngProtocolBased:
        {
            /* Validate Protocol based routing parameters */
            *pSize = (*pSize) + (PHNCINFC_RFCONFIG_TLV_HEADER_LEN) + (PHNCINFC_RFCONFIG_PROTO_RTNG_VALUE_LEN);
            wStatus = NFCSTATUS_SUCCESS;
            break;
        }
        case phNciNfc_e_LstnModeRtngAidBased:
        {
            /* Validate Aid based routing parameters */
            pphNciNfc_AidBasedRtngValue_t pAidBasedRtngVal = &(pRoutingEntry->LstnModeRtngValue.tAidBasedRtngValue);
            if((PHNCINFC_RFCONFIG_AID_MINLEN <= pAidBasedRtngVal->bAidSize) &&
                (PHNCINFC_RFCONFIG_AID_MAXLEN >= pAidBasedRtngVal->bAidSize))
            {
                *pSize = (*pSize) + (PHNCINFC_RFCONFIG_TLV_HEADER_LEN) +
                        (PHNCINFC_RFCONFIG_AID_VALUE_DEFAULT_LEN) + pAidBasedRtngVal->bAidSize;
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        }
        default:
            /* Invalid Type */
            wStatus = NFCSTATUS_INVALID_PARAMETER;
            PH_LOG_NCI_WARN_STR("Unsupported routing type");
            break;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static uint8_t
phNciNfc_VerifyRfProtocols(uint8_t bNumMapEntries,
                           pphNciNfc_MappingConfig_t pProtoIfMapping)
{
    uint8_t bStatus = 0;
    uint8_t bCountA = 0;
    uint8_t bCountB = 0;
    uint8_t bMaxMappings = bNumMapEntries;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Same protocol mapped to same interface with modes differing (2 entries) is not allowed?? */
    for(bCountA = 0; bCountA < bMaxMappings; bCountA++)
    {
        for(bCountB = 0; bCountB < bMaxMappings; bCountB++)
        {
            /* Check if a protocol is being repeated in the input list of mapings */
            if(pProtoIfMapping[bCountA].tRfProtocol ==
                pProtoIfMapping[bCountB].tRfProtocol)
            {
                if(bCountA != bCountB)
                {
                    bStatus = 1;
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_ValidateMode(phNciNfc_MapMode_t tMode)
{
    uint8_t bStatus = 1;
    uint8_t bPollMode = 0;
    uint8_t bLstnMode = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    bPollMode = (uint8_t)tMode.bPollMode;
    bLstnMode = (uint8_t)tMode.bLstnMode;

    /* Verify whether input Mode is out of the supported range */
    if((1 == bPollMode) || (1 == bLstnMode))
    {
        bStatus = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static uint8_t
phNciNfc_CheckRfIntfs(uint8_t *pSupportedRfIntfs,
                      uint8_t bNoOfRfIfSuprt,
                      phNciNfc_RfInterfaces_t eRequestedRfIntf)
{
    uint8_t bRfIntfList = 0;
    uint8_t bStatus = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Check if the Rf interface is present in the list of NFCC supported Rf interfaces */
    for(bRfIntfList = 0; bRfIntfList < bNoOfRfIfSuprt; bRfIntfList++)
    {
        if(eRequestedRfIntf == pSupportedRfIntfs[bRfIntfList])
        {
            bStatus = NFCSTATUS_SUCCESS;
            break;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

static NFCSTATUS phNciNfc_StoreRfParams(uint8_t *pTlvParam,
                                        pphNciNfc_RfDiscConfigParams_t pDiscConfigParams)
{
    NFCSTATUS wStoreStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    switch(pTlvParam[0])
    {
    case PHNCINFC_RFCONFIG_PA_BAIL_OUT:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tPollNfcADiscParams.bBailOut = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.PollNfcAConfig = 1;
            pDiscConfigParams->tPollNfcADiscParams.PollNfcAConfig.Config.SetBailOut =1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_PB_AFI:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tPollNfcBDiscParams.bAfi = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.PollNfcBConfig = 1;
            pDiscConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config.SetAfi = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_PB_BAIL_OUT:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tPollNfcBDiscParams.bBailOut = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.PollNfcBConfig = 1;
            pDiscConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config.SetBailOut = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_PB_ATTRIB_PARAM1:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tPollNfcBDiscParams.bAttriB_Param1 = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.PollNfcBConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_PF_BIT_RATE:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tPollNfcFDiscParams.bBitRate = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.PollNfcFConfig = 1;
            pDiscConfigParams->tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_PB_H_INFO:
        if(pTlvParam[1] <= PH_NCINFC_MAX_HIGHER_LAYER_INF_LEN)
        {
            pDiscConfigParams->tPollIsoDepDiscParams.bHigherLayerInfoSize = pTlvParam[1];
            if(pDiscConfigParams->tPollIsoDepDiscParams.bHigherLayerInfoSize > 0)
            {
                phOsalNfc_MemCopy(&pDiscConfigParams->tPollIsoDepDiscParams.aHigherLayerInfo,
                    &pTlvParam[2],(uint32_t)pTlvParam[1]);
            }
            pDiscConfigParams->tConfigInfo.PollIsoDepConfig = 1;
            pDiscConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetHigherLayerInfo = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_PI_BIT_RATE:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tPollIsoDepDiscParams.bBitRate = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.PollIsoDepConfig = 1;
            pDiscConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetBitRate = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;

    case PHNCINFC_RFCONFIG_BITR_NFC_DEP:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tPollNfcDepDiscParams.bNfcDepSpeed = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.PollNfcDepConfig = 1;
            pDiscConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetSpeed = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_ATR_REQ_GEN_BYTES:
        if(pTlvParam[1] <= PH_NCINFC_MAX_ATR_REQ_GEN_BYTES_LEN)
        {
            pDiscConfigParams->tPollNfcDepDiscParams.bAtrReqGeneBytesSize = pTlvParam[1];
            phOsalNfc_MemCopy(&pDiscConfigParams->tPollNfcDepDiscParams.aAtrReqGenBytes,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.PollNfcDepConfig = 1;
            pDiscConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetGenBytes = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;

    case PHNCINFC_RFCONFIG_ATR_REQ_CONFIG:
        if(1 == pTlvParam[1])
        {
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_PNGET_DIDVAL)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_PNGET_DIDVAL)
            {
                pDiscConfigParams->tPollNfcDepDiscParams.AtrReqConfig.bDid = 1;
            }
            pDiscConfigParams->tPollNfcDepDiscParams.AtrReqConfig.bLr = \
                PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_PNGET_LRVAL) >> PHNCINFC_RFCONFIG_PNFCDEP_LR_BITMASK;

            pDiscConfigParams->tConfigInfo.PollNfcDepConfig = 1;
            pDiscConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetAtrConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;

    case PHNCINFC_RFCONFIG_LA_BIT_FRAME_SDD:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tLstnNfcADiscParams.bBitFrameSDD = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.LstnNfcAConfig = 1;
            pDiscConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetBitFrameSdd = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LA_PLATFORM_CONFIG:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tLstnNfcADiscParams.bPlatformConfig = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.LstnNfcAConfig = 1;
            pDiscConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetPlatformConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LA_SEL_INFO:
        if(1 == pTlvParam[1])
        {
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_LAISODEP_PROTOCOL)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_LAISODEP_PROTOCOL)
            {
                pDiscConfigParams->tLstnNfcADiscParams.SelInfo.bIsoDepProtoSupport = 1;
            }
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_LANFCDEP_PROTOCOL)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_LANFCDEP_PROTOCOL)
            {
                pDiscConfigParams->tLstnNfcADiscParams.SelInfo.bNfcDepProtoSupport = 1;
            }
            pDiscConfigParams->tConfigInfo.LstnNfcAConfig = 1;
            pDiscConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetSelInfo = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LA_NFCID1:
        if( (4 == pTlvParam[1]) ||
            (7 == pTlvParam[1]) ||
            (10 == pTlvParam[1]) )
        {
            pDiscConfigParams->tLstnNfcADiscParams.bNfcID1Size = pTlvParam[1];
            phOsalNfc_MemCopy(&pDiscConfigParams->tLstnNfcADiscParams.aNfcID1,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcAConfig = 1;
            pDiscConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetNfcID1 = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;

    case PHNCINFC_RFCONFIG_LB_SENSB_INFO:
        if(1 == pTlvParam[1])
        {
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_LBISODEP_PROTOCOL_SUPPORT)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_LBISODEP_PROTOCOL_SUPPORT)
            {
                pDiscConfigParams->tLstnNfcBDiscParams.SensBInfo.bIsoDepProtocolSupport = 1;
            }
            pDiscConfigParams->tConfigInfo.LstnNfcBConfig = 1;
            pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSensBInfo =1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LB_NFCID0:
        if(4 == pTlvParam[1])
        {
            phOsalNfc_MemCopy(&pDiscConfigParams->tLstnNfcBDiscParams.aNfcID0,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcBConfig = 1;
            pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetNfcID0 = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LB_APPLICATION_DATA:
        if(4 == pTlvParam[1])
        {
            phOsalNfc_MemCopy(&pDiscConfigParams->tLstnNfcBDiscParams.aAppData,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcBConfig = 1;
            pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAppData = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;

    case PHNCINFC_RFCONFIG_LB_SFGI:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tLstnNfcBDiscParams.bSfgi = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.LstnNfcBConfig = 1;
            pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSfgi = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LB_ADC_FO:
        if(1 == pTlvParam[1])
        {
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_LBGET_DIDVAL)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_LBGET_DIDVAL)
            {
                pDiscConfigParams->tLstnNfcBDiscParams.AdcFo.bDid = 1;
            }
            pDiscConfigParams->tLstnNfcBDiscParams.AdcFo.bAdcCodingField =\
                    ((pTlvParam[2] >> PHNCINFC_RFCONFIG_LNFCB_ADCFO_OFFSET) &\
                     (PHNCINFC_RFCONFIG_LBADC_CODING) );
            pDiscConfigParams->tConfigInfo.LstnNfcBConfig = 1;
            pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAdcFo = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_1:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID1],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_2:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID2],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_3:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID3],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_4:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID4],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_5:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID5],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_6:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID6],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_7:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID7],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_8:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID8],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_9:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID9],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_10:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID10],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_11:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID11],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_12:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID12],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_13:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID13],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_14:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID14],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_15:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID15],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_16:
        if(PH_NCINFC_T3TID_LEN == pTlvParam[1])
        {
            phOsalNfc_MemCopy(pDiscConfigParams->\
                    tLstnNfcFDiscParams.aT3tId[PHNCINFC_RFCONFIG_T3TID16],
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_CON_BITR_F:
        if(1 == pTlvParam[1])
        {
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_LF_212KBPS)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_LF_212KBPS)
            {
                pDiscConfigParams->tLstnNfcFDiscParams.ConfigBitRate.bLstn212kbps = 1;
            }
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_LF_424KBPS)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_LF_424KBPS)
            {
                pDiscConfigParams->tLstnNfcFDiscParams.ConfigBitRate.bLstn424kbps = 1;
            }
            pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetConfigBitRate = 1;
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_PMM:
        if(8 == pTlvParam[1])
        {
            phOsalNfc_MemCopy(&pDiscConfigParams->tLstnNfcFDiscParams.aT3tPmm,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tPmm = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_MAX:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tLstnNfcFDiscParams.bT3tMax = pTlvParam[2];
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetbT3tMax = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_T3T_FLAGS2:
        if(2 == pTlvParam[1])
        {
            phOsalNfc_MemCopy(&pDiscConfigParams->tLstnNfcFDiscParams.wT3tFlags,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tFlags = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LF_PROTOCOL_TYPE:
        if(1 == pTlvParam[1])
        {
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_LFNFCDEP_PROTOCOL_SUPPORT)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_LFNFCDEP_PROTOCOL_SUPPORT)
            {
                pDiscConfigParams->tLstnNfcFDiscParams.ProtocolType.bNfcDepProtocolSupport = 1;
            }
            pDiscConfigParams->tConfigInfo.LstnNfcFConfig = 1;
            pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetProtocolType = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;

    case PHNCINFC_RFCONFIG_FWI:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tLstnIsoDepDiscParams.bFrameWaitingTime= pTlvParam[2];
            pDiscConfigParams->tConfigInfo.LstnIsoDepConfig = 1;
            pDiscConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetFwt = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LA_HIST_BY:
        if(pTlvParam[1] <= PH_NCINFC_MAX_HIST_BYTES_LEN)
        {
            pDiscConfigParams->tLstnIsoDepDiscParams.bHistBytesSize = pTlvParam[1];
            phOsalNfc_MemCopy(&pDiscConfigParams->tLstnIsoDepDiscParams.aLA_HistBytes,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnIsoDepConfig = 1;
            pDiscConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetLA_HistBytes = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;

    case PHNCINFC_RFCONFIG_LB_H_INFO_RESP:
        if(pTlvParam[1] <= PH_NCINFC_MAX_HIGHER_LAYER_RES_LEN)
        {
            pDiscConfigParams->tLstnIsoDepDiscParams.bHigherLayerRespInfoSize = pTlvParam[1];
            phOsalNfc_MemCopy(&pDiscConfigParams->tLstnIsoDepDiscParams.aLB_HigherLayerResp,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnIsoDepConfig = 1;
            pDiscConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetLB_HigherLayerResp = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_LI_BIT_RATE:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tLstnIsoDepDiscParams.bBitRate= pTlvParam[2];
            pDiscConfigParams->tConfigInfo.LstnIsoDepConfig = 1;
            pDiscConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetbBitRate = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;

    case PHNCINFC_RFCONFIG_WT:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tLstnNfcDepDiscParams.bWaitingTime= pTlvParam[2];
            pDiscConfigParams->tConfigInfo.LstnNfcDepConfig = 1;
            pDiscConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetWT = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_ATR_RES_GEN_BYTES:
        if(pTlvParam[1] <= PH_NCINFC_MAX_ATR_RES_GEN_BYTES_LEN)
        {
            pDiscConfigParams->tLstnNfcDepDiscParams.bAtrResGenBytesSize = pTlvParam[1];
            phOsalNfc_MemCopy(&pDiscConfigParams->tLstnNfcDepDiscParams.aAtrResGenBytes,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.LstnNfcDepConfig = 1;
            pDiscConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetGenBytes = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_ATR_RES_CONFIG:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tLstnNfcDepDiscParams.AtrRespConfig.bLengthReduction =
                    ((pTlvParam[2] >> PHNCINFC_RFCONFIG_LNFCDEP_LR_OFFSET) &
                     (PHNCINFC_RFCONFIG_LNNFCDEP_LR) );
            pDiscConfigParams->tConfigInfo.LstnNfcDepConfig = 1;
            pDiscConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetAtrRespConfig = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;

    case PHNCINFC_RFCONFIG_TOTAL_DURATION:
        if(2 == pTlvParam[1])
        {
            phOsalNfc_MemCopy(&pDiscConfigParams->tCommonDiscParams.wTotalDuration,
                &pTlvParam[2],(uint32_t)pTlvParam[1]);
            pDiscConfigParams->tConfigInfo.CommonConfig = 1;
            pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetTotalDuration =1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_CON_DEVICES_LIMIT:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tCommonDiscParams.bConDevLimit= pTlvParam[2];
            pDiscConfigParams->tConfigInfo.CommonConfig = 1;
            pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetConDevLimit = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_RF_FIELD_INFO:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tCommonDiscParams.bRfFieldInfo= pTlvParam[2];
            pDiscConfigParams->tConfigInfo.CommonConfig = 1;
            pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetRfFieldInfo = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_RF_NFCEE_ACTION:
        if(1 == pTlvParam[1])
        {
            pDiscConfigParams->tCommonDiscParams.bRfNfceeAction= pTlvParam[2];
            pDiscConfigParams->tConfigInfo.CommonConfig = 1;
            pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetRfNfceeAction = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    case PHNCINFC_RFCONFIG_NFCDEP_OP:
        if(1 == pTlvParam[1])
        {
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_RTOX_REQ)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_RTOX_REQ)
            {
                pDiscConfigParams->tCommonDiscParams.NfcDepOperationParam.bRtoxReq = 1;
            }
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_NFCDEP_ATTCMD)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_NFCDEP_ATTCMD)
            {
                pDiscConfigParams->tCommonDiscParams.NfcDepOperationParam.bAttentionCommand = 1;
            }
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_NFCDEP_INFPDU)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_NFCDEP_INFPDU)
            {
                pDiscConfigParams->tCommonDiscParams.NfcDepOperationParam.bInformationPdu = 1;
            }
            if((PHNCINFC_RFCONFIG_GETPARAM(pTlvParam[2],PHNCINFC_RFCONFIG_NFCDEP_MAXBYTES)) ==
                                (uint8_t)PHNCINFC_RFCONFIG_NFCDEP_MAXBYTES)
            {
                pDiscConfigParams->tCommonDiscParams.NfcDepOperationParam.bUseMaxTxLen = 1;
            }
            pDiscConfigParams->tConfigInfo.CommonConfig = 1;
            pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetNfcDepOperationParam = 1;
            wStoreStatus = NFCSTATUS_SUCCESS;
        }
        break;
    default:
        break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStoreStatus;
}

static NFCSTATUS phNciNfc_UpdateReqParamIDs(void *pNciHandle,uint8_t *pRespParamIDList,
                                       uint8_t bRespParamIDNum,uint8_t bRespParamIDLen)
{
    NFCSTATUS wUpdateStatus;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)pNciHandle;
    uint8_t bParamIndex = 0;
    uint8_t bRespIndex = 0;
    uint8_t aReqParams[PHNCINFC_RFCONFIG_TOTALPARAM_COUNT] = {0};
    uint8_t bReqParamCount = 0;
    uint8_t bReqParamLength = 0;
    uint8_t *pParamList;
    uint8_t bIDFound;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL != pNciContext) && (NULL != pRespParamIDList) &&
        (0 != bRespParamIDNum) )
    {
        pParamList = pNciContext->tRfConfContext.pReqParamList + 1;
        /* Search whether the Requested params are found in response
           If found, Remove those Param IDs from the Requested parameter list */
        for(bParamIndex = 0;bParamIndex < pNciContext->tRfConfContext.bReqParamLen;bParamIndex++)
        {
            bIDFound = FALSE;
            for(bRespIndex = 0;( (bRespIndex < bRespParamIDLen) && (bIDFound != TRUE) )
                                ;bRespIndex++)
            {
                if(pParamList[bParamIndex] == pRespParamIDList[bRespIndex])
                {
                    if(PHNCINFC_RFCONFIG_PROP_MIN <= pRespParamIDList[bRespIndex])
                    {
                        return NFCSTATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        bIDFound = TRUE;
                    }
                }
            }
            if(bIDFound == FALSE)
            {
                if(PHNCINFC_RFCONFIG_PROP_MIN <= pParamList[bParamIndex])
                {
                    return NFCSTATUS_INVALID_PARAMETER;
                }
                else
                {
                    /* Update Parameter list to be sent in the command */
                    if (bReqParamLength >= ARRAYSIZE(aReqParams))
                    {
                        PH_LOG_NCI_CRIT_STR("Temporary buffer is not big enough!");
                        PH_LOG_NCI_FUNC_EXIT();
                        return NFCSTATUS_INVALID_PARAMETER;
                    }

                    aReqParams[bReqParamLength++] = pParamList[bParamIndex];
                    bReqParamCount++;
                }
            }
        }
        /* Copy the parameters to be requested to NFCC and update the count */
        if(0 != bReqParamCount)
        {
            phOsalNfc_MemCopy(pParamList,aReqParams,(uint32_t)bReqParamLength);
        }
        /* Update the param count and length in Context and Command payload */
        pNciContext->tRfConfContext.bReqParamNum = bReqParamCount;
        pNciContext->tRfConfContext.bReqParamLen = bReqParamLength;
        *(uint8_t *)pNciContext->tRfConfContext.pReqParamList = bReqParamCount;
        wUpdateStatus = NFCSTATUS_SUCCESS;
    }
    else
    {
        wUpdateStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wUpdateStatus;
}

static NFCSTATUS phNciNfc_ProcessRfParams(void *pNciHandle,uint8_t bTotalLen,
                                          uint8_t *pRespParam,uint8_t bStoreParams)
{
    NFCSTATUS wProcStatus = NFCSTATUS_SUCCESS;
    uint8_t bIndex = 0;
    uint8_t aRespParamID[PHNCINFC_RFCONFIG_TOTALPARAM_COUNT];
    uint8_t bRespParamIDNum = 0;
    uint8_t bRespParamIDLen = 0;
    uint8_t *pParamList = NULL;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)pNciHandle;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        pParamList = pNciContext->tRfConfContext.pReqParamList + 1;
        /* Traverse through the TLVs of response */
        while( (bIndex < bTotalLen) && (NFCSTATUS_SUCCESS == wProcStatus) )
        {
            if(TRUE == bStoreParams)
            {
                /* Validate and Store Config Params in the list */
                wProcStatus = phNciNfc_StoreRfParams( &pRespParam[bIndex],
                                    (pphNciNfc_RfDiscConfigParams_t)pNciContext->pUpperLayerInfo);
            }
            else
            {
                /* Parameter length should always be zero -
                   For invalid parameter case */
                if(0 != pRespParam[bIndex + 1])
                {
                    wProcStatus = NFCSTATUS_INVALID_PARAMETER;
                }
            }
            /* Store the parameter Tag and update its count */
            aRespParamID[bRespParamIDLen++] = pRespParam[bIndex];
            bRespParamIDNum++;
            /* Update the index to point to the next TLV in the list */
            bIndex += (pRespParam[bIndex+1] + 2);
        }
        /* Check whether Response contains complete TLV of parameters */
        if( (NFCSTATUS_SUCCESS == wProcStatus) && (bTotalLen != bIndex) )
        {
            wProcStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        /* Check whether Parameter List needs to be Updated to send another command */
        if(NFCSTATUS_SUCCESS == wProcStatus)
        {
            wProcStatus = phNciNfc_UpdateReqParamIDs(pNciHandle,aRespParamID,bRespParamIDNum,
                                                     bRespParamIDLen);
        }
    }
    else
    {
        wProcStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wProcStatus;
}

static NFCSTATUS phNciNfc_GetConfig(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreTxInfo_t tTxInfo;
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t) pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        /* Clear the Tx information structure */
        phOsalNfc_SetMemory(&tTxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));

        tTxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        tTxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
        tTxInfo.tHeaderInfo.Opcode_ID.OidType.NciCoreCmdOid = phNciNfc_e_NciCoreGetConfigCmdOid;

        tTxInfo.Buff = pNciCtx->tSendPayload.pBuff;
        tTxInfo.wLen = pNciCtx->tSendPayload.wPayloadSize;

        /* Sending command */
        wStatus = phNciNfc_CoreIfTxRx(&(pNciCtx->NciCoreContext), &tTxInfo,
                        &(pNciCtx->RspBuffInfo), 2000,
                        (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                        (void *)pNciCtx);
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameter (phNciNfc_SetConfig)");
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ClearConfigParams(void *pNciHandle)
{
    NFCSTATUS wClearStatus = NFCSTATUS_FAILED;
    pphNciNfc_Context_t pNciContext = pNciHandle;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        pNciContext->tRfConfContext.bReqParamNum = 0;
        phOsalNfc_FreeMemory(pNciContext->tRfConfContext.pReqParamList);
        pNciContext->tRfConfContext.pReqParamList = NULL;
        wClearStatus = NFCSTATUS_SUCCESS;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wClearStatus;
}

static NFCSTATUS phNciNfc_GetConfigRawRsp(void *pContext, NFCSTATUS wStatus)
{
    NFCSTATUS wGetStatus = wStatus;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if( (NULL != pNciContext) && (NULL != pNciContext->RspBuffInfo.pBuff) &&\
                    (NFCSTATUS_SUCCESS == wStatus) )
    {
        /* do nothing */
    }
    return wGetStatus;
}

static NFCSTATUS phNciNfc_GetConfigRsp(void *pContext, NFCSTATUS wStatus)
{
    NFCSTATUS wGetStatus = NFCSTATUS_FAILED;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if( (NULL != pNciContext) && (NULL != pNciContext->RspBuffInfo.pBuff) &&\
                    (NFCSTATUS_SUCCESS == wStatus) )
    {
        /* Check whether All parameters are accepted by NFCC */
        switch(pNciContext->RspBuffInfo.pBuff[0])
        {
        case PH_NCINFC_STATUS_OK:
            /* Check whether All parameters requested are provided by NFCC */
            if(pNciContext->tRfConfContext.bReqParamNum ==
                    pNciContext->RspBuffInfo.pBuff[1])
            {
                wGetStatus = phNciNfc_ProcessRfParams(pContext,
                    /* Exclude the status and number of parameters byte to get
                       the Total length of TLVs */
                    (uint8_t)(pNciContext->RspBuffInfo.wLen - 2),
                             &pNciContext->RspBuffInfo.pBuff[2],TRUE);
                if(pNciContext->tRfConfContext.bReqParamNum == 0)
                {
                    wGetStatus = NFCSTATUS_SUCCESS;
                }
            }
            break;
        case PH_NCINFC_STATUS_MESSAGE_SIZE_EXCEEDED:
            /* Check whether More parameters are to be requested */
            if(pNciContext->tRfConfContext.bReqParamNum >=
                    pNciContext->RspBuffInfo.pBuff[1])
            {
                wGetStatus = phNciNfc_ProcessRfParams(pContext,
                    /* Exclude the status and number of parameters byte to get
                       the Total length of TLVs */
                                (uint8_t)(pNciContext->RspBuffInfo.wLen - 2),
                                &pNciContext->RspBuffInfo.pBuff[2],TRUE);
                if( (NFCSTATUS_SUCCESS == wGetStatus) &&
                    (pNciContext->tRfConfContext.bReqParamNum > 0) )
                {
                    wGetStatus = NFCSTATUS_SUCCESS;
                }
            }
            break;
        case PH_NCINFC_STATUS_INVALID_PARAM:
            /* Check whether All parameters passed are invalid */
            wGetStatus = phNciNfc_ProcessRfParams(pContext,
                /* Exclude the status and number of parameters byte to get
                   the Total length of TLVs */
                            (uint8_t)(pNciContext->RspBuffInfo.wLen - 2),
                            &pNciContext->RspBuffInfo.pBuff[2],FALSE);
            break;
        default:
            break;
        }
    }
    else
    {
        wGetStatus = wStatus;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wGetStatus;
}

static NFCSTATUS phNciNfc_CompleteGetConfigRaw(void *pContext,NFCSTATUS wStatus)
{
    NFCSTATUS wGetStatus = wStatus;
    pphNciNfc_Context_t pNciContext = pContext;
    uint8_t aGetConfig[255] = {0};
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        phNciNfc_FreeSendPayloadBuff(pNciContext);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if(pNciContext->RspBuffInfo.wLen > (ARRAYSIZE(aGetConfig) - 1))
            {
                PH_LOG_NCI_CRIT_STR("Buffer is tool small!");
                wGetStatus = NFCSTATUS_BUFFER_TOO_SMALL;
                goto Done;
            }

            aGetConfig[0] = (uint8_t)pNciContext->RspBuffInfo.wLen;
            if(aGetConfig[0] > 0)
            {
                phOsalNfc_MemCopy(&aGetConfig[1],pNciContext->RspBuffInfo.pBuff,\
                    pNciContext->RspBuffInfo.wLen);
            }
        }
        phNciNfc_Notify(pNciContext, wStatus, (void *)aGetConfig);
    }

Done:

    PH_LOG_NCI_FUNC_EXIT();
    return wGetStatus;
}

static NFCSTATUS phNciNfc_CompleteGetConfig(void *pContext,NFCSTATUS wStatus)
{
    NFCSTATUS wGetStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Check whether All required parameters are provided by NFCC, if not
               another command needs to be sent */
            if(0 == pNciContext->tRfConfContext.bReqParamNum)
            {
                if(NULL != pNciContext->tSendPayload.pBuff)
                {
                    pNciContext->tSendPayload.pBuff = NULL;
                    pNciContext->tSendPayload.wPayloadSize = 0;
                }
                phNciNfc_Notify(pNciContext, wStatus, (void *)pNciContext->pUpperLayerInfo);
                /* Clear the Context variables used for Get Config Params */
                wGetStatus = phNciNfc_ClearConfigParams(pNciContext);
            }
            else
            {
                /* ReInitialize the sequence handler to send another Command */
                PHNCINFC_INIT_SEQUENCE(pNciContext, gphNciNfc_GetConfigSequence);
                /* Update the Buffer pointer where parameters are updated */
                pNciContext->tSendPayload.pBuff =
                            pNciContext->tRfConfContext.pReqParamList;
                /* Payload size is Number of parameters + parameters */
                pNciContext->tSendPayload.wPayloadSize =
                    pNciContext->tRfConfContext.bReqParamLen + 1;
                wGetStatus = phNciNfc_GenericSequence(pNciContext, NULL, NFCSTATUS_SUCCESS);
            }
        }
        else
        {
            phNciNfc_Notify(pNciContext, wStatus, (void *)pNciContext->pUpperLayerInfo);
            /* Clear the Context variables used for Get Config Params */
            (void)phNciNfc_ClearConfigParams(pNciContext);
            wGetStatus = wStatus;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wGetStatus;
}

NFCSTATUS phNciNfc_GetRfParams(pphNciNfc_RfDiscConfigParams_t pDiscConfigParams,
                               uint8_t *pRfParams,uint8_t *pParamCount,
                               uint8_t *pParamLen)
{
    NFCSTATUS wCountStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bParamNum = 0;
    uint8_t bParamIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    if( (NULL != pRfParams) &&(NULL != pParamCount) &&\
        (NULL != pParamLen) )
    {
        /* Configure Poll Nfc-A Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.PollNfcAConfig)
        {
            if(1 == pDiscConfigParams->tPollNfcADiscParams.PollNfcAConfig.Config.SetBailOut)
            {
               pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_PA_BAIL_OUT;
               bParamNum++;
            }
        }
        /* Configure Poll Nfc-B Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.PollNfcBConfig)
        {
            if(1 == pDiscConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config.SetAfi)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_PB_AFI;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tPollNfcBDiscParams.PollNfcBConfig.Config.SetBailOut)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_PB_BAIL_OUT;
                bParamNum++;
            }
            /* Read only param. Cannot be set */
            pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_PB_ATTRIB_PARAM1;
            bParamNum++;
        }

        /* Configure Poll Nfc-F Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.PollNfcFConfig)
        {
            if(1 == pDiscConfigParams->tPollNfcFDiscParams.PollNfcFConfig.Config.SetBitRate)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_PF_BIT_RATE;
                bParamNum++;
            }
        }

        /* Configure Poll Iso-Dep Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.PollIsoDepConfig)
        {
            if(1 == pDiscConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetHigherLayerInfo)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_PB_H_INFO;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tPollIsoDepDiscParams.PollIsoDepConfig.Config.SetBitRate)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_PI_BIT_RATE;
                bParamNum++;
            }
        }

        /* Configure Poll Nfc-Dep Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.PollNfcDepConfig)
        {
            if(1 == pDiscConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetSpeed)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_BITR_NFC_DEP;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetGenBytes)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_ATR_REQ_GEN_BYTES;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetAtrConfig)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_ATR_REQ_CONFIG;
                bParamNum++;
            }
        }

        /* Configure Listen Nfc-A Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.LstnNfcAConfig)
        {
            if(1 == pDiscConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetBitFrameSdd)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LA_BIT_FRAME_SDD;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetPlatformConfig)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LA_PLATFORM_CONFIG;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetSelInfo)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LA_SEL_INFO;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcADiscParams.LstnNfcAConfig.Config.SetNfcID1)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LA_NFCID1;
                bParamNum++;
            }
        }

        /* Configure Listen Nfc-B Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.LstnNfcBConfig)
        {
            if(1 == pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSensBInfo)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LB_SENSB_INFO;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetNfcID0)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LB_NFCID0;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAppData)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LB_APPLICATION_DATA;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetSfgi)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LB_SFGI;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcBDiscParams.LstnNfcBConfig.Config.SetAdcFo)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LB_ADC_FO;
                bParamNum++;
            }
        }

        /* Configure Listen Nfc-F Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.LstnNfcFConfig)
        {
            if(1 == pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tId)
            {
                /*Currently for these bit fields are not added. It will added as and when required*/
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_1;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_2;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_3;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_4;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_5;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_6;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_7;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_8;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_9;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_10;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_11;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_12;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_13;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_14;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_15;
                bParamNum++;
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_16;
                bParamNum++;
            }

            if(1 == pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetProtocolType)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_PROTOCOL_TYPE;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tPmm)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_PMM;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetbT3tMax)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_MAX;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetT3tFlags)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_T3T_FLAGS2;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcFDiscParams.LstnNfcFConfig.Config.SetConfigBitRate)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LF_CON_BITR_F;
                bParamNum++;
            }
        }

        /* Configure Listen Iso-Dep Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.LstnIsoDepConfig)
        {
            if(1 == pDiscConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetFwt)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_FWI;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetLA_HistBytes)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LA_HIST_BY;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetLB_HigherLayerResp)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LB_H_INFO_RESP;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnIsoDepDiscParams.LstnIsoDepConfig.Config.SetbBitRate)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_LI_BIT_RATE;
                bParamNum++;
            }
        }

        /* Configure Listen Nfc-Dep Discovery configuration parameters */
        if(1 == pDiscConfigParams->tConfigInfo.LstnNfcDepConfig)
        {
            if(1 == pDiscConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetWT)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_WT;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetGenBytes)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_ATR_RES_GEN_BYTES;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetAtrRespConfig)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_ATR_RES_CONFIG;
                bParamNum++;
            }
        }

        if(1 == pDiscConfigParams->tConfigInfo.CommonConfig)
        {
            /* Configure Common Discovery configuration parameters */
            if(1 == pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetTotalDuration)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_TOTAL_DURATION;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetConDevLimit)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_CON_DEVICES_LIMIT;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetRfFieldInfo)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_RF_FIELD_INFO;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetRfNfceeAction)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_RF_NFCEE_ACTION;
                bParamNum++;
            }
            if(1 == pDiscConfigParams->tCommonDiscParams.ComnParamsConfig.Config.SetNfcDepOperationParam)
            {
                pRfParams[bParamIndex++] = (uint8_t)PHNCINFC_RFCONFIG_NFCDEP_OP;
                bParamNum++;
            }
        }
        wCountStatus = NFCSTATUS_SUCCESS;
        /* Updating the Number of parameters and their length */
        *pParamCount = bParamNum;
        *pParamLen = bParamIndex;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wCountStatus;
}
