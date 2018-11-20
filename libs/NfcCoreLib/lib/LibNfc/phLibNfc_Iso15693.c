/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Iso15693.h"
#include "phLibNfc_Iso15693.tmh"

/** Min length of Get system info response */
#define PHLIBNFC_GETSYSINFO_RESP_MINLEN         (0x02)


/* This function shall send Get system information command to ISO15693 tag */
static NFCSTATUS phLibNfc_GetSysInfoCmd(void *pContext, NFCSTATUS status, void *pInfo);

/* This function shall Process system information received from ISO15693 tag */
static NFCSTATUS phLibNfc_GetSysInfoResp(void *pcontext, NFCSTATUS status, void *pInfo);

static NFCSTATUS phLibNfc_GetSysInfoComplete(void *pContext,NFCSTATUS status,void *pInfo);

phLibNfc_Sequence_t gphLibNfc_Iso15693GetSysInfoSeq[] = {
    {&phLibNfc_GetSysInfoCmd, &phLibNfc_GetSysInfoResp},
    {NULL, &phLibNfc_GetSysInfoComplete}
};

static void phLibNfc_Iso15693Sequence(void* pContext,NFCSTATUS status,pphNciNfc_Data_t pInfo);

static NFCSTATUS phLibNfc_GetSysInfoCmd(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    phNciNfc_TransceiveInfo_t tIso15693Info;
    pphNciNfc_DeviceInfo_t pDevInfo = NULL;
    uint8_t aGetSysInfoCmdBuff[12] = {0};
    uint32_t dwIndex = 0x00;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    UNUSED(pInfo);
    if(NULL != pCtx)
    {
        pDevInfo = (pphNciNfc_DeviceInfo_t)pCtx->pInfo;
        if((NULL != pDevInfo->pRemDevList[dwIndex]) &&
            (phNciNfc_eISO15693_PICC == pDevInfo->pRemDevList[dwIndex]->RemDevType))
        {
            if(0 != pDevInfo->pRemDevList[dwIndex]->tRemoteDevInfo.Iso15693_Info.UidLength)
            {
                aGetSysInfoCmdBuff[0] = 0x22;
                phOsalNfc_MemCopy(&aGetSysInfoCmdBuff[2],
                    pDevInfo->pRemDevList[dwIndex]->tRemoteDevInfo.Iso15693_Info.Uid,
                    PH_NCINFCTYPES_15693_UID_LENGTH);
                tIso15693Info.tSendData.wLen = 10;
            }
            else
            {
                aGetSysInfoCmdBuff[0] = 0x02;
                tIso15693Info.tSendData.wLen = 2;
            }
            aGetSysInfoCmdBuff[1] = 0x2B;

            phOsalNfc_MemCopy(pCtx->aSendBuff,aGetSysInfoCmdBuff,tIso15693Info.tSendData.wLen);
            tIso15693Info.uCmd.Iso15693Cmd = phNciNfc_eIso15693Raw;
            tIso15693Info.tSendData.pBuff = pCtx->aSendBuff;
            tIso15693Info.tRecvData.pBuff = pCtx->aRecvBuff;
            tIso15693Info.tRecvData.wLen = (uint16_t)sizeof(pCtx->aRecvBuff);
            tIso15693Info.wTimeout = 300;

            wStatus = phNciNfc_Transceive((void *)pCtx->sHwReference.pNciHandle,
                                          (void *)(pDevInfo->pRemDevList[dwIndex]),
                                          &tIso15693Info,
                                           (pphNciNfc_TransreceiveCallback_t)&phLibNfc_Iso15693Sequence,
                                          (void *)pContext);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_GetSysInfoResp(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext  = (pphLibNfc_LibContext_t)pContext;
    pphNciNfc_DeviceInfo_t pDeviceInfo = NULL;
    pphNciNfc_Data_t pRecvData = (pphNciNfc_Data_t)pInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        pDeviceInfo = pLibContext->pInfo;
        if(NFCSTATUS_SUCCESS == status)
        {
            if((NULL != pRecvData) && (NULL != pRecvData->pBuff) &&
                (pRecvData->wLen >= PHLIBNFC_GETSYSINFO_RESP_MINLEN))
            {
                if(0 == (pRecvData->pBuff[0] & 0x01))
                {
                    PH_LOG_LIBNFC_INFO_STR("Updating ISO15693 system information");
                    (void)phNciNfc_Update15693SysInfo(pLibContext->sHwReference.pNciHandle,
                                                     pDeviceInfo->pRemDevList[0],
                                                     pRecvData->pBuff);
                }
                else
                {
                    PH_LOG_LIBNFC_WARN_STR("ISO15693 Get Sys Info command not supported");
                }
            }
            else
            {
                PH_LOG_LIBNFC_WARN_STR("ISO15693 Invalid response buffer or response length");
            }
        }
        else
        {
            PH_LOG_LIBNFC_WARN_STR("ISO15693 Get Sys Info no response received");
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

static void phLibNfc_Iso15693Sequence(void*   pContext,
                               NFCSTATUS status,
                               pphNciNfc_Data_t    pInfo)
{
    status = phLibNfc_SeqHandler(pContext,status,(void *)pInfo);
    return;
}

static NFCSTATUS phLibNfc_GetSysInfoComplete(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = status;
    phLibNfc_NtfRegister_RspCb_t pClientCb=NULL;
    pphLibNfc_LibContext_t pLibContext  = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t TrigEvent;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pLibContext)
    {
        pClientCb = pLibContext->CBInfo.pClientNtfRegRespCB;
        if((wStatus != NFCSTATUS_SUCCESS) && (wStatus != NFCSTATUS_INVALID_PARAMETER))
        {
            PH_LOG_LIBNFC_WARN_STR("ISO15693 Get Sys Info no response received");
            wStatus = NFCSTATUS_SUCCESS;
        }

        if(NFCSTATUS_SUCCESS != wStatus)
        {
            TrigEvent = phLibNfc_EventInvalid;
        }
        else
        {
            TrigEvent = pLibContext->DiscTagTrigEvent;
        }
        (void)phLibNfc_ProcessDevInfo(pLibContext, TrigEvent, pLibContext->pInfo ,wStatus);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
