/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Type1Tag.h"
#include "phLibNfc_Type1Tag.tmh"

static NFCSTATUS phLibNfc_SendRidCmd(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_ProcessRidResp(void *pcontext,NFCSTATUS status,void *pInfo);
static void phLibNfc_RidSequence(void* pContext,NFCSTATUS status,pphNciNfc_Data_t pInfo);

phLibNfc_Sequence_t gphLibNfc_T1tGetUidSequence[] = {
    {&phLibNfc_SendRidCmd, &phLibNfc_ProcessRidResp},
    {NULL, &phLibNfc_ReqInfoComplete}
};

static NFCSTATUS phLibNfc_ProcessRidResp(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_LibContext_t pLibContext  = (pphLibNfc_LibContext_t)pContext;
    pphNciNfc_DeviceInfo_t pDeviceInfo = (pphNciNfc_DeviceInfo_t)pLibContext->pInfo;
    pphNciNfc_Data_t pRecvData = (pphNciNfc_Data_t)pInfo;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if( (NULL != pLibContext) && (NFCSTATUS_SUCCESS == status) )
    {
        /* Validate the Response of RID Command */
        if( (NULL != pRecvData) && (NULL != pRecvData->pBuff) &&\
            (PHLIBNFC_RID_RESP_LEN == pRecvData->wLen) )
        {
            PH_LOG_LIBNFC_INFO_STR("RID response received");
            /* Store the RID response Type 1 tag info */
            wStatus = phNciNfc_UpdateJewelInfo(pLibContext->sHwReference.pNciHandle,\
                pDeviceInfo->pRemDevList[pLibContext->bLastCmdSent],\
                                             pRecvData->pBuff);
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Invalid parameters (phLibNfc_ProcessRidResp)");
            wStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_RidSequence(void* pContext, NFCSTATUS status, pphNciNfc_Data_t pInfo)
{
    status = phLibNfc_SeqHandler(pContext,status,(void *)pInfo);
}

static NFCSTATUS phLibNfc_SendRidCmd(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    phNciNfc_TransceiveInfo_t tType1Info;
    pphNciNfc_DeviceInfo_t pDevInfo;
    uint8_t aRidCmdBuff[] = {0x78,0x00,0x00,0x00,0x00,0x00,0x00};
    uint32_t dwIndex = 0x00;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    UNUSED(pInfo);

    if(NULL != pCtx)
    {
        pDevInfo = (pphNciNfc_DeviceInfo_t)pCtx->pInfo;
        /* Copy the Payload to send RID command */
        phOsalNfc_MemCopy(pCtx->aSendBuff,aRidCmdBuff,sizeof(aRidCmdBuff));
        tType1Info.uCmd.T1TCmd = phNciNfc_eT1TRaw;
        tType1Info.tSendData.pBuff = pCtx->aSendBuff;
        tType1Info.tSendData.wLen = (uint16_t)sizeof(aRidCmdBuff);
        tType1Info.tRecvData.pBuff = pCtx->aRecvBuff;
        tType1Info.tRecvData.wLen = (uint16_t)sizeof(pCtx->aRecvBuff);
        tType1Info.wTimeout = 300;

        wStatus = phNciNfc_Transceive((void *)pCtx->sHwReference.pNciHandle,
                                      (void *)(pDevInfo->pRemDevList[dwIndex]),
                                      &tType1Info,
                                       (pphNciNfc_TransreceiveCallback_t)&phLibNfc_RidSequence,
                                      (void *)pContext);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
