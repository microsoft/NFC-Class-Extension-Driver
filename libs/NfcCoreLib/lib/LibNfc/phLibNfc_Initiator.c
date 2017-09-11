/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Initiator.h"

#include "phLibNfc_Initiator.tmh"

NFCSTATUS
phLibNfc_Mgt_SetP2P_ConfigParams(phLibNfc_sNfcIPCfg_t *pConfigInfo,
                                 pphLibNfc_RspCb_t pConfigRspCb,
                                 void *  pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = phLibNfc_GetContext();
    pphNciNfc_RfDiscConfigParams_t pRfDiscConfParam = NULL;
    uint8_t bGeneralBytesLength = 0;
    phLibNfc_DummyInfo_t Info;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventDummy;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    Info.Evt = phLibNfc_DummyEventInvalid;
    if(NULL == pLibContext)
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pConfigInfo) ||(NULL == pConfigRspCb))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        bGeneralBytesLength = pConfigInfo->generalBytesLength;
        if((pLibContext->bDtaFlag || (0 != bGeneralBytesLength)) &&
           (PH_LIBNFC_INTERNAL_MAX_ATR_LENGTH >= bGeneralBytesLength))
        {
            /* Allocate memory for the input parameter that needs to be sent to
               'phNciNfc_SetConfigRfParameters' API */
            pRfDiscConfParam = (pphNciNfc_RfDiscConfigParams_t)phOsalNfc_GetMemory(
                                                sizeof(phNciNfc_RfDiscConfigParams_t));
            if(NULL != pRfDiscConfParam)
            {
                phOsalNfc_SetMemory(pRfDiscConfParam,0,sizeof(phNciNfc_RfDiscConfigParams_t));

                pLibContext->CBInfo.pClientNfcIpCfgCb = pConfigRspCb;
                pLibContext->CBInfo.pClientNfcIpCfgCntx = pContext;

                /* Set general bytes for Poll  Nfc-Dep parameters */
                switch(pConfigInfo->p2pMode)
                {
                    case NFC_DEP_POLL:
                        pRfDiscConfParam->tConfigInfo.PollNfcDepConfig = 1;
                        /* Configuring general bytes for ATR_REQ */
                        pRfDiscConfParam->tPollNfcDepDiscParams.PollNfcDepConfig.EnableConfig = 0;
                        pRfDiscConfParam->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetGenBytes = 1;
                        pRfDiscConfParam->tPollNfcDepDiscParams.bAtrReqGeneBytesSize = bGeneralBytesLength;
                        phOsalNfc_MemCopy(pRfDiscConfParam->tPollNfcDepDiscParams.aAtrReqGenBytes,
                        pConfigInfo->generalBytes,bGeneralBytesLength);
                        /*Enable General Bytes to default values*/
                        /*Configure to default value = 0x30*/
                        pRfDiscConfParam->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetAtrConfig = 1;
                        pRfDiscConfParam->tPollNfcDepDiscParams.AtrReqConfig.bDid = 0; /*0 For LLCP*/
                        pRfDiscConfParam->tPollNfcDepDiscParams.AtrReqConfig.bLr = 0x03;/*0x03 for LLCP*/
                    break;
                    case  NFC_DEP_LISTEN:
                        /* Set general bytes for Listen  Nfc-Dep parameters */
                        pRfDiscConfParam->tConfigInfo.LstnNfcDepConfig = 1;
                        /* Configuring general bytes for ATR_RES */
                        pRfDiscConfParam->tLstnNfcDepDiscParams.LstnNfcDepConfig.EnableConfig = 0;
                        pRfDiscConfParam->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetGenBytes = 1;
                        pRfDiscConfParam->tLstnNfcDepDiscParams.bAtrResGenBytesSize = bGeneralBytesLength;
                        phOsalNfc_MemCopy(pRfDiscConfParam->tLstnNfcDepDiscParams.aAtrResGenBytes,
                        pConfigInfo->generalBytes,bGeneralBytesLength);
                        /*Enable General Bytes in response*/
                        /*Configure to defaule value*/
                        pRfDiscConfParam->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetAtrRespConfig = 1;
                        pRfDiscConfParam->tLstnNfcDepDiscParams.AtrRespConfig.bLengthReduction = 0x03; /*For LLCP*/
                    break;
                    case NFC_DEP_DEFAULT:
                    default:
                        pRfDiscConfParam->tConfigInfo.PollNfcDepConfig = 1;
                        /* Configuring general bytes for ATR_REQ */
                        pRfDiscConfParam->tPollNfcDepDiscParams.PollNfcDepConfig.EnableConfig = 0;
                        pRfDiscConfParam->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetGenBytes = 1;
                        pRfDiscConfParam->tPollNfcDepDiscParams.bAtrReqGeneBytesSize = bGeneralBytesLength;
                        phOsalNfc_MemCopy(pRfDiscConfParam->tPollNfcDepDiscParams.aAtrReqGenBytes,
                                          pConfigInfo->generalBytes,bGeneralBytesLength);
                        /*Enable General Bytes to default values*/
                        /*Configure to default value = 0x30*/
                        pRfDiscConfParam->tPollNfcDepDiscParams.PollNfcDepConfig.Config.bSetAtrConfig = 1;
                        pRfDiscConfParam->tPollNfcDepDiscParams.AtrReqConfig.bDid = 0; /*0 For LLCP*/
                        pRfDiscConfParam->tPollNfcDepDiscParams.AtrReqConfig.bLr = 0x03;/*0x03 for LLCP*/     
                        pRfDiscConfParam->tConfigInfo.LstnNfcDepConfig = 1;
                        /* Configuring general bytes for ATR_RES */
                        pRfDiscConfParam->tLstnNfcDepDiscParams.LstnNfcDepConfig.EnableConfig = 0;
                        pRfDiscConfParam->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetGenBytes = 1;
                        pRfDiscConfParam->tLstnNfcDepDiscParams.bAtrResGenBytesSize = bGeneralBytesLength;
                        phOsalNfc_MemCopy(pRfDiscConfParam->tLstnNfcDepDiscParams.aAtrResGenBytes,
                                          pConfigInfo->generalBytes,bGeneralBytesLength);
                        /*Enable General Bytes in response*/
                        /*Configure to defaule value*/
                        pRfDiscConfParam->tLstnNfcDepDiscParams.LstnNfcDepConfig.Config.bSetAtrRespConfig = 1;
                        pRfDiscConfParam->tLstnNfcDepDiscParams.AtrRespConfig.bLengthReduction = 0x03; /*For LLCP*/		
                    break;
                }

                Info.Evt = phLibNfc_DummyEventSetP2PConfigs;
                Info.Params = (void *)pRfDiscConfParam;

                wStatus = phLibNfc_StateHandler(pLibContext,
                                                TrigEvent,
                                                NULL,
                                                &Info,
                                                NULL);
                if(NFCSTATUS_PENDING != wStatus)
                {
                    phOsalNfc_FreeMemory(pRfDiscConfParam);
                    pLibContext->CBInfo.pClientNfcIpCfgCb = NULL;
                    pLibContext->CBInfo.pClientNfcIpCfgCntx = NULL;
                }
            }
            else
            {
                wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

void phLibNfc_P2pConfigParamsCb(void* pContext,NFCSTATUS status,void* pInfo)
{
    pphNciNfc_RfDiscConfigParams_t pRfDiscConfParam = (pphNciNfc_RfDiscConfigParams_t) pContext;
    NFCSTATUS wStatus = status;
    pphLibNfc_LibContext_t pLibContext = phLibNfc_GetContext();
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        if(NULL != pRfDiscConfParam)
        {
            phOsalNfc_FreeMemory(pRfDiscConfParam);
        }

        (void)phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);
        if(NULL != pLibContext->CBInfo.pClientNfcIpCfgCb)
        {
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                wStatus = NFCSTATUS_FAILED;
            }

            pLibContext->CBInfo.pClientNfcIpCfgCb(pLibContext->CBInfo.pClientNfcIpCfgCntx,
                                        wStatus);
         }
        pLibContext->CBInfo.pClientNfcIpCfgCb = NULL;
        pLibContext->CBInfo.pClientNfcIpCfgCntx = NULL;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}
