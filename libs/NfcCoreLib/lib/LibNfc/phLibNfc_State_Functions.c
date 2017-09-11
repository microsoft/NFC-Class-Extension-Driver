/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_State_Functions.tmh"

#define PHLIBNFC_MFCKEY_LEN             0x06
#define PHLIBNFC_MFCWRITE16_SNDBFRLEN   0x02
#define PHLIBNFC_READBUFFER_SIZE        0x20
#define PHLIBNFC_AUTHRAW_CMDLEN         0x0C
#define PHLIBNFC_AUTHRAW_UIDSTART       0x02
#define PHLIBNFC_AUTHRAW_UIDLEN         0x04
#define PHLIBNFC_AUTHRAW_KEYSTART       0x06
#define PHLIBNFC_AUTH_KEYA              0x60
#define PHLIBNFC_AUTH_KEYB              0x61
#define PHLIBNFC_RAWAUTH_BUFFER_LEN     0x0a
#define PHLIBNFC_RAWAUTH_CMD            0x00
#define PHLIBNFC_RAWAUTH_ADD            0x01
#define PHLIBNFC_WRTE16_RAW             0x12
#define PHLIBNFC_WRTE16_SEDBUFF         0x10

static NFCSTATUS phLibNfc_SendWrt16Cmd(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_SendWrt16CmdResp(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_SendWrt16CmdPayload(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_Wrt16CmdPayloadResp(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MFCWrite16Complete(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_SendRd16Cmd(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_Rd16CmdResp(void *pContext,NFCSTATUS wStatus,void *pInfo);

static NFCSTATUS phLibNfc_MFCIncDecRestoreTransferCmd(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MFCIncDecRestoreResp(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MFCIncDecRestorePayload(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MFCIncDecRestorePayloadResp(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MFCIncDecRestorePayloadComplete(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MFCIncDecRestoreTransferComplete(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MFCTransferResp(void *pContext,NFCSTATUS wStatus,void *pInfo);

static NFCSTATUS phLibNfc_SendAuthCmd(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_SendAuthCmdResp(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MFCSendAuthCmdComplete(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MfcChkPresAuth(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MfcChkPresAuthProc(void *pContext,NFCSTATUS wStatus,void *pInfo);
static NFCSTATUS phLibNfc_MfcChkPresAuthComplete(void *pContext,NFCSTATUS wStatus,void *pInfo);

static NFCSTATUS phLibNfc_RawtoCmd(void *pContext,
                                   pphNciNfc_RemoteDevInformation_t pRemDevHandle,
                                   phLibNfc_sTransceiveInfo_t* psTransceiveInfo,
                                   phLibNfc_sTransceiveInfo_t** psTransceiveInfo1);

static NFCSTATUS phLibNfc_ChkMFCAuthWrtCmd(phLibNfc_sTransceiveInfo_t* psTransceiveInfo,
                                           pphNciNfc_RemoteDevInformation_t RemoteDevInfo);

/* Send Write16 command In case of Mifare classic tag */
static phLibNfc_Sequence_t gphLibNfc_MFCWrite16[] = {
    {&phLibNfc_SendWrt16Cmd, &phLibNfc_SendWrt16CmdResp},
    {&phLibNfc_SendWrt16CmdPayload, &phLibNfc_Wrt16CmdPayloadResp},
    {NULL, &phLibNfc_MFCWrite16Complete}
};

/* Send Increment, Decrement, or Restore command In case of Mifare classic tag */
static phLibNfc_Sequence_t gphLibNfc_MFCIncDecRestore[] = {
    {&phLibNfc_MFCIncDecRestoreTransferCmd, &phLibNfc_MFCIncDecRestoreResp},
    {NULL, &phLibNfc_MFCIncDecRestoreTransferComplete}
};

/* Send Increment, Decrement, or Restore payload In case of Mifare classic tag */
static phLibNfc_Sequence_t gphLibNfc_MFCIncDecRestorePayload[] = {
    {&phLibNfc_MFCIncDecRestorePayload, &phLibNfc_MFCIncDecRestorePayloadResp},
    {NULL, &phLibNfc_MFCIncDecRestorePayloadComplete}
};

/* Send Transfer command In case of Mifare classic tag */
static phLibNfc_Sequence_t gphLibNfc_MFCTransfer[] = {
    {&phLibNfc_MFCIncDecRestoreTransferCmd, &phLibNfc_MFCTransferResp},
    {NULL, &phLibNfc_MFCIncDecRestoreTransferComplete}
};

/* Set Authentication key and send authentication command In case of Mifare classic tag */
static phLibNfc_Sequence_t gphLibNfc_MFCSendAuthCmd[] = {
    {&phLibNfc_SendAuthCmd, &phLibNfc_SendAuthCmdResp},
    {NULL, &phLibNfc_MFCSendAuthCmdComplete}
};

/* Set Authentication key and send authentication command In case of Mifare classic tag */
phLibNfc_Sequence_t gphLibNfc_MFCSendAuthCmdForPresChk[] = {
    {&phLibNfc_MfcChkPresAuth, &phLibNfc_MfcChkPresAuthProc},
    {NULL, &phLibNfc_MfcChkPresAuthComplete}
};

NFCSTATUS phLibNfc_Init2Discovery(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    // Warning C4305: 'type cast': truncation from 'void *' to 'phLibNfc_eDiscAndDisconnMode_t'
#pragma warning(suppress:4305)
    phLibNfc_eDiscAndDisconnMode_t eDiscMode = (phLibNfc_eDiscAndDisconnMode_t)Param1;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if( (NULL != pCtx) && (NULL != Param2) )
    {
        wStatus = phLibNfc_ProcessDiscReq(pCtx,\
                                    eDiscMode,\
                                    (phLibNfc_sADD_Cfg_t *)Param2,\
                                    Param3);
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Actv2Discovery(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    // Warning C4305: 'type cast': truncation from 'void *' to 'phLibNfc_eDiscAndDisconnMode_t'
#pragma warning(suppress:4305)
    phLibNfc_eDiscAndDisconnMode_t eDiscMode = (phLibNfc_eDiscAndDisconnMode_t)Param1;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        wStatus = phLibNfc_ProcessReDiscReq(pCtx,\
                                           eDiscMode,\
                                           (phLibNfc_sADD_Cfg_t *)Param2,\
                                           Param3);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_LsnAc2Dscv_Def(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    // Warning C4305: 'type cast': truncation from 'void *' to 'phLibNfc_eDiscAndDisconnMode_t'
#pragma warning(suppress:4305)
    phLibNfc_eDiscAndDisconnMode_t eDiscMode = (phLibNfc_eDiscAndDisconnMode_t)Param1;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        wStatus = phLibNfc_ProcessReDiscReq(pCtx,\
                                           eDiscMode,\
                                           (phLibNfc_sADD_Cfg_t *)Param2,\
                                           Param3);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus ;
}

NFCSTATUS phLibNfc_Discovery2Active(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    // Warning C4305: 'type cast': truncation from 'void *' to 'phNciNfc_RfInterfaces_t'
#pragma warning(suppress:4305)
    phNciNfc_RfInterfaces_t eRfInterface = (phNciNfc_RfInterfaces_t)Param2;

    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        wStatus = phNciNfc_Connect(pCtx->sHwReference.pNciHandle,
                  (pphNciNfc_RemoteDevInformation_t)Param1,
                  eRfInterface,
                  NULL,
                  (void *)pCtx);
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Discovery2Recv(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        /*Call Discovery Configuration and Enable Discovery Sequence*/
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_TranscvEnter(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        /* Nothing to do */
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_TranscvExit(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_TransceiveInfo_t tNciTranscvInfo;
    phLibNfc_sTransceiveInfo_t* psTransceiveInfo = NULL;
    phLibNfc_sTransceiveInfo_t* psTransceiveInfo1 = NULL;
    pphNciNfc_RemoteDevInformation_t pRemDevHandle = NULL;

    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    /* Since 'phLibNfc_TranscvExit' is part of 'phLibNfc_StateFptr' exit function,
       this function gets invoked during RemoteDev_Disconnect in which case transceive is
       not required. Inorder to skip the same, this flag has been used */
    if(pCtx->bSkipTransceive != PH_LIBNFC_INTERNAL_INPROGRESS)
    {
        if((NULL != pCtx) && (NULL != Param1)) /*Parms one holds tranceive info*/
        {
            psTransceiveInfo = (phLibNfc_sTransceiveInfo_t *)Param1;
            pRemDevHandle = (pphNciNfc_RemoteDevInformation_t)pCtx->Connected_handle;

            /*In Mifare Classic Tag and Command is phNfc_eMifareWrite16 or authentication*/
            if(NFCSTATUS_SUCCESS == phLibNfc_ChkMFCAuthWrtCmd(psTransceiveInfo,pRemDevHandle))
            {
                if(((psTransceiveInfo->cmd.MfCmd == phNfc_eMifareWrite16) ||
                  (psTransceiveInfo->cmd.MfCmd == phNfc_eMifareRaw &&
                   psTransceiveInfo->sSendData.buffer != NULL &&
                   psTransceiveInfo->sSendData.buffer[0] == phNfc_eMifareWrite16)))
                {
                    PHLIBNFC_INIT_SEQUENCE(pCtx,gphLibNfc_MFCWrite16);
                    if(psTransceiveInfo->cmd.MfCmd == phNfc_eMifareRaw)
                    {
                        wStatus = phLibNfc_RawtoCmd((void *)pCtx,pRemDevHandle,psTransceiveInfo,&psTransceiveInfo1);

                        if(NFCSTATUS_SUCCESS == wStatus && NULL != psTransceiveInfo1)
                        {
                            wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,psTransceiveInfo1);
                        }
                    }
                    else
                    {
                        wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,psTransceiveInfo);
                    }
                }
                else if((psTransceiveInfo->sSendData.buffer != NULL) &&
                        ((psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthentA ||
                         psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthentB ) ||
                         (psTransceiveInfo->cmd.MfCmd == phNfc_eMifareRaw &&
                         (psTransceiveInfo->sSendData.buffer[0] == phNfc_eMifareAuthentA ||
                         psTransceiveInfo->sSendData.buffer[0] == phNfc_eMifareAuthentB))))
                {
                    PHLIBNFC_INIT_SEQUENCE(pCtx,gphLibNfc_MFCSendAuthCmd);

                    if(psTransceiveInfo->cmd.MfCmd == phNfc_eMifareRaw)
                    {
                        wStatus = phLibNfc_RawtoCmd((void *)pCtx,pRemDevHandle,psTransceiveInfo,&psTransceiveInfo1);

                        if(NFCSTATUS_SUCCESS == wStatus && NULL != psTransceiveInfo1)
                        {
                            wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,psTransceiveInfo1);
                        }
                    }
                    else
                    {
                        wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,psTransceiveInfo);
                    }
                }
                else if(psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthKeyNumA ||
                        psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthKeyNumB)
                {
                    PHLIBNFC_INIT_SEQUENCE(pCtx,gphLibNfc_MFCSendAuthCmd);
                    wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,psTransceiveInfo);
                }
                else if(psTransceiveInfo->sSendData.buffer != NULL &&
                        (psTransceiveInfo->cmd.MfCmd == phNfc_eMifareInc ||
                         psTransceiveInfo->cmd.MfCmd == phNfc_eMifareDec ||
                         psTransceiveInfo->cmd.MfCmd == phNfc_eMifareRestore))
                {
                    PHLIBNFC_INIT_SEQUENCE(pCtx,gphLibNfc_MFCIncDecRestore);
                    wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,psTransceiveInfo);
                }
                else if(psTransceiveInfo->sSendData.buffer != NULL &&
                        psTransceiveInfo->cmd.MfCmd == phNfc_eMifareTransfer)
                {
                    PHLIBNFC_INIT_SEQUENCE(pCtx,gphLibNfc_MFCTransfer);
                    wStatus = phLibNfc_SeqHandler(pCtx,NFCSTATUS_SUCCESS,psTransceiveInfo);
                }
            }
            else
            {
                wStatus = phLibNfc_MapCmds(pRemDevHandle->RemDevType,psTransceiveInfo,&tNciTranscvInfo);

                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    /* Use application preferred timeout value during transceive */
                    wStatus = phLibNfc_NciTranscv(pCtx->sHwReference.pNciHandle,
                                          pCtx->Connected_handle,
                                          &tNciTranscvInfo,
                                          (void *)pCtx);
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("phLibNfc_MapCmds failed: %!NFCSTATUS!", wStatus);
                    wStatus = NFCSTATUS_FAILED;
                }
            }
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        PH_LOG_LIBNFC_INFO_STR("bSkipTransceive flag set. Skipping transceive.");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_IsInitialisedState(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t )pContext;

    if(pLibContext != NULL)
    {
        if(pLibContext->StateContext.CurrState == phLibNfc_StateInit)
        {
            wStatus = NFCSTATUS_SUCCESS;
        }
        else
        {
            wStatus = NFCSTATUS_NOT_INITIALISED;
        }
    }
    else
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    return wStatus;
}


NFCSTATUS phLibNfc_IsInitialised(void *pContext)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;

    if (NULL == pLibContext ||
        pLibContext->StateContext.CurrState < phLibNfc_StateInit)
    {
        return NFCSTATUS_NOT_INITIALISED;
    }

    return NFCSTATUS_SUCCESS;
}

NFCSTATUS phLibNfc_IsShutDownInProgress(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t )pContext;

    if(pLibContext != NULL)
    {
        if(pLibContext->StateContext.TrgtState == phLibNfc_StateReset)
        {
            wStatus = NFCSTATUS_SUCCESS;
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}

void phLibNfc_InternalSeq(void *pCtx, NFCSTATUS Status, pphNciNfc_Data_t pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    (void)phLibNfc_SeqHandler(pCtx,Status,(void *)pInfo);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

static NFCSTATUS phLibNfc_SendWrt16Cmd(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    phNciNfc_TransceiveInfo_t tNciTranscvInfo;
    uint8_t bBuffIdx = 0x00;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phLibNfc_sTransceiveInfo_t* psTransceiveInfo = (phLibNfc_sTransceiveInfo_t *)pInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = NFCSTATUS_FAILED;

    if( (NULL != pCtx )&&
        (NULL != psTransceiveInfo ) &&
        (NULL != psTransceiveInfo->sRecvData.buffer) &&
        (0 != psTransceiveInfo->sRecvData.length) &&
        (NULL != psTransceiveInfo->sSendData.buffer) &&
        (0 != psTransceiveInfo->sSendData.length))
    {
        pCtx->psTransceiveInfo = phOsalNfc_GetMemory(sizeof(phLibNfc_sTransceiveInfo_t));
        phOsalNfc_MemCopy(pCtx->psTransceiveInfo,psTransceiveInfo,sizeof(phLibNfc_sTransceiveInfo_t));

        pCtx->aSendBuff[bBuffIdx++] = phNfc_eMifareWrite16;
        pCtx->aSendBuff[bBuffIdx++] = psTransceiveInfo->addr;

        tNciTranscvInfo.tSendData.pBuff = pCtx->aSendBuff;
        tNciTranscvInfo.tSendData.wLen = bBuffIdx;
        tNciTranscvInfo.uCmd.T2TCmd = phNciNfc_eT2TRaw;
        tNciTranscvInfo.tRecvData.pBuff = psTransceiveInfo->sRecvData.buffer;
        tNciTranscvInfo.tRecvData.wLen = (uint16_t)psTransceiveInfo->sRecvData.length;
        tNciTranscvInfo.wTimeout = psTransceiveInfo->timeout;

        wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                     pCtx->Connected_handle,
                                     &tNciTranscvInfo,
                                     &phLibNfc_InternalSeq,
                                     (void *)pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_SendWrt16CmdResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("SendWrt16 Command palyload Header success");
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("SendWrt16 Command palyload Header failed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_SendWrt16CmdPayload(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    phNciNfc_TransceiveInfo_t tNciTranscvInfo;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phLibNfc_sTransceiveInfo_t *psTransceiveInfo = NULL;
    wStatus = NFCSTATUS_FAILED;
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pCtx &&
       NULL != pCtx->psTransceiveInfo)
    {
        psTransceiveInfo = pCtx->psTransceiveInfo;

        if((NULL != psTransceiveInfo->sSendData.buffer) &&
            (0 != psTransceiveInfo->sSendData.length) &&
            (NULL != psTransceiveInfo->sRecvData.buffer) &&
            (0 != psTransceiveInfo->sRecvData.length))
        {
            tNciTranscvInfo.tSendData.pBuff = psTransceiveInfo->sSendData.buffer;
            tNciTranscvInfo.tSendData.wLen = (uint16_t)psTransceiveInfo->sSendData.length;
            tNciTranscvInfo.uCmd.T2TCmd = phNciNfc_eT2TRaw;
            tNciTranscvInfo.tRecvData.pBuff = psTransceiveInfo->sRecvData.buffer;
            tNciTranscvInfo.tRecvData.wLen = (uint16_t)psTransceiveInfo->sRecvData.length;
            tNciTranscvInfo.wTimeout = psTransceiveInfo->timeout;
            wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                         pCtx->Connected_handle,
                                         &tNciTranscvInfo,
                                         &phLibNfc_InternalSeq,
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

static NFCSTATUS phLibNfc_Wrt16CmdPayloadResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("SendWrt16 Command palyload success");
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("SendWrt16 Command palyload failed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;

}

static NFCSTATUS phLibNfc_MFCWrite16Complete(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phNciNfc_Data_t pInf;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    UNUSED(pInfo)
    phOsalNfc_SetMemory(&pInf,0x00,sizeof(phNciNfc_Data_t));

    if(NULL != pCtx &&
       NULL != pCtx->psTransceiveInfo &&
       NULL != pCtx->psTransceiveInfo->sSendData.buffer &&
       0 != pCtx->psTransceiveInfo->sRecvData.length)
    {

        if(NULL != pCtx->psTransceiveInfo)
        {
            pInf.pBuff = pCtx->psTransceiveInfo->sSendData.buffer;
            pInf.wLen = 0;

            phOsalNfc_FreeMemory(pCtx->psTransceiveInfo);
            pCtx->psTransceiveInfo = NULL;
        }

        if(NULL != pCtx->psDummyTransceiveInfo)
        {
            if(NULL != pCtx->psDummyTransceiveInfo->sRecvData.buffer)
            {
                phOsalNfc_FreeMemory(pCtx->psDummyTransceiveInfo->sRecvData.buffer);
                pCtx->psDummyTransceiveInfo->sRecvData.buffer = NULL;
            }
            phOsalNfc_FreeMemory(pCtx->psDummyTransceiveInfo);
            pCtx->psDummyTransceiveInfo = NULL;
        }
        if(NULL != pCtx->psTransceiveInfo1)
        {
            if(NULL != pCtx->psTransceiveInfo1->sSendData.buffer)
            {
                phOsalNfc_FreeMemory(pCtx->psTransceiveInfo1->sSendData.buffer);
                pCtx->psTransceiveInfo1->sSendData.buffer = NULL;
            }
            phOsalNfc_FreeMemory(pCtx->psTransceiveInfo1);
            pCtx->psTransceiveInfo1 = NULL;
        }
    }

    (void )phLibNfc_TranscvCb(pContext,wStatus,&pInf);

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_SendRd16Cmd(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    phLibNfc_sTransceiveInfo_t* psTransceiveInfo = NULL;
    phLibNfc_sTransceiveInfo_t* psTransvInfo = NULL;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle = NULL;
    phNciNfc_TransceiveInfo_t tNciTranscvInfo;

    UNUSED(pInfo);
    if(NULL != pCtx &&
       NULL != pCtx->psTransceiveInfo)
    {
        psTransceiveInfo = pCtx->psTransceiveInfo;

        pCtx->psDummyTransceiveInfo = phOsalNfc_GetMemory(sizeof(phLibNfc_sTransceiveInfo_t));

        if(NULL != pCtx->psDummyTransceiveInfo)
        {
            phOsalNfc_SetMemory(pCtx->psDummyTransceiveInfo,0x00,sizeof(phNciNfc_TransceiveInfo_t));

            psTransvInfo = pCtx->psDummyTransceiveInfo;
            psTransvInfo->sRecvData.buffer = phOsalNfc_GetMemory(PHLIBNFC_READBUFFER_SIZE);
            psTransvInfo->sRecvData.length = PHLIBNFC_READBUFFER_SIZE;

            psTransvInfo->addr = psTransceiveInfo->addr;
            psTransvInfo->cmd.MfCmd = phNfc_eMifareRead16;

            if(NULL != psTransvInfo &&
               NULL != psTransvInfo->sRecvData.buffer &&
               0 != psTransvInfo->sRecvData.length &&
               NULL != pCtx->Connected_handle)
            {
                pNciRemoteDevHandle = pCtx->Connected_handle;
                wStatus = phLibNfc_MapCmds(pNciRemoteDevHandle->RemDevType,\
                                           psTransvInfo,
                                           &tNciTranscvInfo);

                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                                  pCtx->Connected_handle,
                                                  &tNciTranscvInfo,
                                                  &phLibNfc_InternalSeq,
                                                  (void *)pCtx);
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
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }
    return wStatus;
}

static NFCSTATUS phLibNfc_Rd16CmdResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("SendRead16 Command success");
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("SendRead16 Command failed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_SendAuthCmd(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    phNciNfc_TransceiveInfo_t tNciTranscvInfo;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phLibNfc_sTransceiveInfo_t *psTransceiveInfo = NULL;
    pphNciNfc_RemoteDevInformation_t pRemDevHandle = NULL;
    wStatus = NFCSTATUS_FAILED;
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pCtx &&
       NULL != pInfo &&
       NULL != pCtx->Connected_handle)
    {
        psTransceiveInfo = (phLibNfc_sTransceiveInfo_t *)pInfo;
        pCtx->psTransceiveInfo = phOsalNfc_GetMemory(sizeof(phLibNfc_sTransceiveInfo_t));
        phOsalNfc_MemCopy(pCtx->psTransceiveInfo,psTransceiveInfo,sizeof(phLibNfc_sTransceiveInfo_t));
        pRemDevHandle = (pphNciNfc_RemoteDevInformation_t )pCtx->Connected_handle;

         if((psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthKeyNumA ||
            psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthKeyNumB))
         {
             wStatus = phLibNfc_MapCmds(pRemDevHandle->RemDevType,psTransceiveInfo,&tNciTranscvInfo);

             if(NFCSTATUS_SUCCESS == wStatus)
             {
                wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                              pCtx->Connected_handle,
                                              &tNciTranscvInfo,
                                              &phLibNfc_InternalSeq,
                                              (void *)pCtx);
                if(NFCSTATUS_PENDING == wStatus)
                {
                    /* Update the actual address which shall be used for authentication */
                    pCtx->psTransceiveInfo->addr = tNciTranscvInfo.bAddr;
                }
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
         }
         else
         {
            if(((NULL != pRemDevHandle) &&
                (NULL != pCtx->psTransceiveInfo)&&
                (NULL != psTransceiveInfo->sSendData.buffer) &&
                (0 != psTransceiveInfo->sSendData.length)))
            {
                wStatus = phLibNfc_MapCmds(pRemDevHandle->RemDevType,psTransceiveInfo,&tNciTranscvInfo);

                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                                  pCtx->Connected_handle,
                                                  &tNciTranscvInfo,
                                                  &phLibNfc_InternalSeq,
                                                  (void *)pCtx);
                    if(NFCSTATUS_PENDING == wStatus)
                    {
                        /* Update the actual address which shall be used for authentication */
                        pCtx->psTransceiveInfo->addr = tNciTranscvInfo.bAddr;
                    }
                }
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
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
static NFCSTATUS phLibNfc_SendAuthCmdResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    uint8_t bKey = 0;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Authentication command success");

        if((phNfc_eMifareAuthentB == pCtx->psTransceiveInfo->cmd.MfCmd)||
           (phNfc_eMifareAuthKeyNumB == pCtx->psTransceiveInfo->cmd.MfCmd))
        {
            bKey = bKey | PH_LIBNFC_ENABLE_KEY_B;
        }

        /* Get the key type (key number) */
        if(pCtx->psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthKeyNumA ||
           pCtx->psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthKeyNumB)
        {
            pCtx->tMfcInfo.cmd = pCtx->psTransceiveInfo->cmd.MfCmd;
            pCtx->tMfcInfo.addr= pCtx->psTransceiveInfo->addr;
            pCtx->tMfcInfo.key = bKey | pCtx->psTransceiveInfo->bKeyNum;
        }
        else
        {
            bKey = bKey | PHLIBNFC_MFC_EMBEDDED_KEY;

            /* Store the authentication info for using the same during Presence Check */
            pCtx->tMfcInfo.cmd = pCtx->psTransceiveInfo->cmd.MfCmd;
            pCtx->tMfcInfo.addr= pCtx->psTransceiveInfo->addr;
            pCtx->tMfcInfo.key = bKey;

            phOsalNfc_MemCopy(pCtx->tMfcInfo.MFCKey,
                              pCtx->psTransceiveInfo->sSendData.buffer + PHLIBNFC_MFCUIDLEN_INAUTHCMD,
                              PHLIBNFC_MFC_AUTHKEYLEN);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Authentication command failed!");
        phLibNfc_MfcAuthInfo_Clear(pCtx);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MFCSendAuthCmdComplete(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phNciNfc_Data_t pInf = {0};

    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = PHNFCSTATUS(wStatus);
    phOsalNfc_SetMemory(&pInfo,0x00,sizeof(pphNciNfc_Data_t));
    phOsalNfc_SetMemory(&pInf, 0x00,sizeof(phNciNfc_Data_t));

    if(NULL != pCtx &&
       NULL != pCtx->psTransceiveInfo &&
       NULL != pCtx->psTransceiveInfo->sRecvData.buffer &&
       0 != pCtx->psTransceiveInfo->sRecvData.length)
    {
        pInf.pBuff = pCtx->psTransceiveInfo->sRecvData.buffer;
        pInf.wLen = 0;

        if(NULL != pCtx->psTransceiveInfo)
        {
            phOsalNfc_FreeMemory(pCtx->psTransceiveInfo);
            pCtx->psTransceiveInfo = NULL;
        }

        if(NULL != pCtx->psTransceiveInfo1)
        {
            if(NULL != pCtx->psTransceiveInfo1->sSendData.buffer)
            {
                phOsalNfc_FreeMemory(pCtx->psTransceiveInfo1->sSendData.buffer);
                pCtx->psTransceiveInfo1->sSendData.buffer = NULL;
            }

            phOsalNfc_FreeMemory(pCtx->psTransceiveInfo1);
            pCtx->psTransceiveInfo1 = NULL;
        }
    }

    PH_LOG_LIBNFC_FUNC_EXIT();

    /*Call Transceive callback*/
    (void )phLibNfc_TranscvCb(pContext,wStatus,&pInf);

    return wStatus;
}

static NFCSTATUS phLibNfc_MfcChkPresAuth(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    phNciNfc_TransceiveInfo_t tNciTranscvInfo;
    uint8_t bIndex = 0;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    pphNciNfc_RemoteDevInformation_t pRemDevHandle = NULL;
    wStatus = NFCSTATUS_FAILED;
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pCtx &&
       NULL != pCtx->Connected_handle)
    {
        pRemDevHandle = (pphNciNfc_RemoteDevInformation_t )pCtx->Connected_handle;

        pCtx->aSendBuff[bIndex++] = pCtx->tMfcInfo.cmd;
        pCtx->aSendBuff[bIndex++] = pCtx->tMfcInfo.addr;
        pCtx->aSendBuff[bIndex++] = pCtx->tMfcInfo.key;

        phOsalNfc_MemCopy(&(pCtx->aSendBuff[bIndex]),
                            pCtx->tMfcInfo.MFCKey,
                            PHLIBNFC_MFC_AUTHKEYLEN);

        bIndex += PHLIBNFC_MFC_AUTHKEYLEN;

        phOsalNfc_SetMemory(&tNciTranscvInfo,0x00,sizeof(phNciNfc_TransceiveInfo_t));

        tNciTranscvInfo.uCmd.T2TCmd = phNciNfc_eT2TAuth;
        tNciTranscvInfo.bAddr = pCtx->tMfcInfo.addr;
        tNciTranscvInfo.tSendData.pBuff = pCtx->aSendBuff;
        tNciTranscvInfo.tSendData.wLen = bIndex;
        tNciTranscvInfo.tRecvData.pBuff = pCtx->aRecvBuff;
        tNciTranscvInfo.tRecvData.wLen = PH_LIBNFC_INTERNAL_RECV_BUFF_SIZE;

        wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                              pCtx->Connected_handle,
                                              &tNciTranscvInfo,
                                              &phLibNfc_InternalSeq,
                                              (void *)pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MfcChkPresAuthProc(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_LIBNFC_INFO_STR("Authentication command success");
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Authentication command failed!");
            phLibNfc_MfcAuthInfo_Clear(pCtx);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
static NFCSTATUS phLibNfc_MfcChkPresAuthComplete(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    NFCSTATUS wRetVal = NFCSTATUS_SUCCESS;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphLibNfc_Context_t pLibCtx = NULL;
    phNfc_sData_t tResData;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext) && (pContext == phLibNfc_GetContext()))
    {
        pLibCtx = (pphLibNfc_Context_t)pContext;
        if(NULL != pLibCtx->psTransceiveInfo)
        {
            phOsalNfc_FreeMemory(pLibCtx->psTransceiveInfo);
            pLibCtx->psTransceiveInfo = NULL;
        }
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_LIBNFC_INFO_STR("Auth command of Mifare classic Success");
            phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus),&TrigEvent);
            wRetVal = phLibNfc_StateHandler((pphLibNfc_LibContext_t)pContext,
                                        TrigEvent, NULL, NULL, NULL);
            if(NFCSTATUS_SUCCESS != wRetVal)
            {
                wStatus = NFCSTATUS_FAILED;
            }
            tResData.buffer = NULL;
            tResData.length = 0;
            phLibNfc_RemoteDev_ChkPresence_Cb(pContext,&tResData,wStatus);
        }
        else
        {
            PH_LOG_LIBNFC_WARN_STR("Mifare classic - Auth failed");
            phLibNfc_MfcAuthInfo_Clear(pLibCtx);

            /* Mif Classic authentication failed,
              Re-Check using Deactv to Sleep and Re-activation sequence */
            if(pLibCtx->bReactivation_Flag == PH_LIBNFC_REACT_ONLYSELECT)
            {
                PHLIBNFC_INIT_SEQUENCE(pLibCtx, gphLibNfc_ReActivate_MFCSeq2Select);
            }
            else
            {
                PHLIBNFC_INIT_SEQUENCE(pLibCtx, gphLibNfc_ReActivate_MFCSeq2);
            }

            wStatus = phLibNfc_SeqHandler(pLibCtx, NFCSTATUS_SUCCESS, NULL);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc Context passed by lower layer");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_RawtoCmd(void *pContext,
                            pphNciNfc_RemoteDevInformation_t pRemDevHandle ,
                            phLibNfc_sTransceiveInfo_t* psTransceiveInfo,
                            phLibNfc_sTransceiveInfo_t** psTransceiveInfo1)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    int32_t wStat = 0x01;
    uint8_t bUidIndex = 0;
    uint8_t bUidLength = 0;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx &&
       NULL != psTransceiveInfo &&
       NULL != psTransceiveInfo->sSendData.buffer &&
       0 != psTransceiveInfo->sSendData.length &&
       NULL != psTransceiveInfo->sRecvData.buffer &&
       0 != psTransceiveInfo->sRecvData.length &&
       NULL != pRemDevHandle)
    {

        /*Check RAW command is Autheticate command*/
        if(psTransceiveInfo->sSendData.length == PHLIBNFC_AUTHRAW_CMDLEN &&
           (psTransceiveInfo->sSendData.buffer[0] == phNfc_eMifareAuthentA  ||
           psTransceiveInfo->sSendData.buffer[0] == phNfc_eMifareAuthentB ))
        {
            bUidLength = pRemDevHandle->tRemoteDevInfo.Iso14443A_Info.UidLength;

            if(PHLIBNFC_MFCWITH_7BYTEUID == bUidLength)
            {
                bUidIndex = PHLIBNFC_MFCUIDINDEX_7BYTEUID;
            }
             wStat = phOsalNfc_MemCompare(&pRemDevHandle->tRemoteDevInfo.Iso14443A_Info.Uid[bUidIndex],
                                          &psTransceiveInfo->sSendData.buffer[PHLIBNFC_AUTHRAW_UIDSTART],
                                          PHLIBNFC_AUTHRAW_UIDLEN);

            if(0 == wStat)
            {
                pCtx->psTransceiveInfo1 = phOsalNfc_GetMemory(sizeof(phLibNfc_sTransceiveInfo_t));

                if(NULL != pCtx->psTransceiveInfo1)
                {
                    pCtx->psTransceiveInfo1->sSendData.buffer = phOsalNfc_GetMemory(PHLIBNFC_RAWAUTH_BUFFER_LEN);
                    pCtx->psTransceiveInfo1->sSendData.length = PHLIBNFC_RAWAUTH_BUFFER_LEN;

                    pCtx->psTransceiveInfo1->sRecvData.buffer = psTransceiveInfo->sRecvData.buffer;
                    pCtx->psTransceiveInfo1->sRecvData.length = psTransceiveInfo->sRecvData.length;

                    if(NULL != pCtx->psTransceiveInfo1->sSendData.buffer &&
                       NULL != pCtx->psTransceiveInfo1->sRecvData.buffer &&
                       0 != pCtx->psTransceiveInfo1->sSendData.length &&
                       0 != pCtx->psTransceiveInfo1->sRecvData.length)
                    {

                        phOsalNfc_SetMemory(pCtx->psTransceiveInfo1->sSendData.buffer,
                                            0x00,
                                            PHLIBNFC_RAWAUTH_BUFFER_LEN);

                        phOsalNfc_SetMemory(pCtx->psTransceiveInfo1->sRecvData.buffer,
                                            0x00,
                                            PHLIBNFC_RAWAUTH_BUFFER_LEN);

                        pCtx->psTransceiveInfo1->cmd.MfCmd = (phNfc_eMifareCmdList_t)psTransceiveInfo->sSendData.buffer[PHLIBNFC_RAWAUTH_CMD];
                        pCtx->psTransceiveInfo1->addr = psTransceiveInfo->sSendData.buffer[PHLIBNFC_RAWAUTH_ADD];
                        phOsalNfc_MemCopy(pCtx->psTransceiveInfo1->sSendData.buffer,
                                          &psTransceiveInfo->sSendData.buffer[PHLIBNFC_AUTHRAW_UIDSTART],
                                          PHLIBNFC_RAWAUTH_BUFFER_LEN);

                        *psTransceiveInfo1 = pCtx->psTransceiveInfo1;
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
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(psTransceiveInfo->sSendData.length == PHLIBNFC_WRTE16_RAW &&
                psTransceiveInfo->sSendData.buffer[0] == phNfc_eMifareWrite16)
        {
            pCtx->psTransceiveInfo1 = phOsalNfc_GetMemory(sizeof(phLibNfc_sTransceiveInfo_t));

            if(NULL != pCtx->psTransceiveInfo1)
            {
                pCtx->psTransceiveInfo1->sSendData.buffer = phOsalNfc_GetMemory(PHLIBNFC_WRTE16_SEDBUFF);
                pCtx->psTransceiveInfo1->sSendData.length = PHLIBNFC_WRTE16_SEDBUFF;

                pCtx->psTransceiveInfo1->sRecvData.buffer = psTransceiveInfo->sRecvData.buffer;
                pCtx->psTransceiveInfo1->sRecvData.length = psTransceiveInfo->sRecvData.length;

                if(NULL != pCtx->psTransceiveInfo1->sSendData.buffer &&
                   NULL != pCtx->psTransceiveInfo1->sRecvData.buffer &&
                   0 != pCtx->psTransceiveInfo1->sSendData.length &&
                   0 != pCtx->psTransceiveInfo1->sRecvData.length)
                {

                    phOsalNfc_SetMemory(pCtx->psTransceiveInfo1->sSendData.buffer,
                                        0x00,
                                        PHLIBNFC_RAWAUTH_BUFFER_LEN);

                    phOsalNfc_SetMemory(pCtx->psTransceiveInfo1->sRecvData.buffer,
                                        0x00,
                                        PHLIBNFC_RAWAUTH_BUFFER_LEN);

                    pCtx->psTransceiveInfo1->cmd.MfCmd = (phNfc_eMifareCmdList_t )psTransceiveInfo->sSendData.buffer[PHLIBNFC_RAWAUTH_CMD];
                    pCtx->psTransceiveInfo1->addr = psTransceiveInfo->sSendData.buffer[PHLIBNFC_RAWAUTH_ADD];
                    phOsalNfc_MemCopy(pCtx->psTransceiveInfo1->sSendData.buffer,
                                      &psTransceiveInfo->sSendData.buffer[PHLIBNFC_WRTE16_SEDBUFF],
                                      PHLIBNFC_WRTE16_SEDBUFF);

                    *psTransceiveInfo1 = pCtx->psTransceiveInfo1;
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
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }
   else
    {
            wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Transceive2Discovered(void *pContext, void *Param1, void *Param2, void *Param3)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_DiscSeqWithDeactSleep);
    wStatus = phLibNfc_SeqHandler(pContext,NFCSTATUS_SUCCESS,NULL);
    PH_LOG_LIBNFC_FUNC_EXIT();

    return wStatus;
}

static NFCSTATUS phLibNfc_ChkMFCAuthWrtCmd(phLibNfc_sTransceiveInfo_t* psTransceiveInfo,
                                           pphNciNfc_RemoteDevInformation_t RemoteDevInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != psTransceiveInfo &&
       NULL != RemoteDevInfo)
    {
        wStatus = phLibNfc_ChkMfCTag(RemoteDevInfo);

        if(NFCSTATUS_SUCCESS != wStatus)
        {
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            if(((psTransceiveInfo->cmd.MfCmd == phNfc_eMifareWrite16) ||
              (psTransceiveInfo->cmd.MfCmd == phNfc_eMifareRaw &&
               psTransceiveInfo->sSendData.buffer != NULL &&
               psTransceiveInfo->sSendData.buffer[0] == phNfc_eMifareWrite16 &&
               psTransceiveInfo->sSendData.length == PHLIBNFC_WRTE16_RAW)))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            else if(((psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthKeyNumA ||
                      psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthKeyNumB ) ||
                    (psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthentA ||
                      psTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthentB ) ||
                    (psTransceiveInfo->cmd.MfCmd == phNfc_eMifareRaw &&
                     psTransceiveInfo->sSendData.buffer != NULL &&
                     psTransceiveInfo->sSendData.length == PHLIBNFC_AUTHRAW_CMDLEN &&
                     (psTransceiveInfo->sSendData.buffer[0] == phNfc_eMifareAuthentA  ||
                      psTransceiveInfo->sSendData.buffer[0] == phNfc_eMifareAuthentB ))))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            else if(((psTransceiveInfo->cmd.MfCmd == phNfc_eMifareInc ||
                psTransceiveInfo->cmd.MfCmd == phNfc_eMifareDec ) &&
                (psTransceiveInfo->sSendData.buffer != NULL &&
                psTransceiveInfo->sSendData.length == 4)))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            else if(((psTransceiveInfo->cmd.MfCmd == phNfc_eMifareTransfer ||
                psTransceiveInfo->cmd.MfCmd == phNfc_eMifareRestore ) &&
                (psTransceiveInfo->sSendData.buffer != NULL &&
                psTransceiveInfo->sSendData.length > 0)))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    else
    {
        wStatus= NFCSTATUS_FAILED;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MFCIncDecRestoreTransferCmd(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    phNciNfc_TransceiveInfo_t tNciTranscvInfo;
    uint8_t bBuffIdx = 0x00;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phLibNfc_sTransceiveInfo_t* psTransceiveInfo = (phLibNfc_sTransceiveInfo_t *)pInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = NFCSTATUS_FAILED;

    if( (NULL != pCtx )&&
        (NULL != psTransceiveInfo ) &&
        (NULL != psTransceiveInfo->sRecvData.buffer) &&
        (0 != psTransceiveInfo->sRecvData.length) &&
        (NULL != psTransceiveInfo->sSendData.buffer) &&
        (0 != psTransceiveInfo->sSendData.length))
    {
        pCtx->psTransceiveInfo = phOsalNfc_GetMemory(sizeof(phLibNfc_sTransceiveInfo_t));
        phOsalNfc_MemCopy(pCtx->psTransceiveInfo,psTransceiveInfo,sizeof(phLibNfc_sTransceiveInfo_t));

        pCtx->aSendBuff[bBuffIdx++] = psTransceiveInfo->cmd.MfCmd;
        pCtx->aSendBuff[bBuffIdx++] = psTransceiveInfo->addr;

        tNciTranscvInfo.tSendData.pBuff = pCtx->aSendBuff;
        tNciTranscvInfo.tSendData.wLen = bBuffIdx;
        tNciTranscvInfo.uCmd.T2TCmd = phNciNfc_eT2TRaw;
        tNciTranscvInfo.tRecvData.pBuff = psTransceiveInfo->sRecvData.buffer;
        tNciTranscvInfo.tRecvData.wLen = (uint16_t)psTransceiveInfo->sRecvData.length;
        tNciTranscvInfo.wTimeout = psTransceiveInfo->timeout;

        wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                     pCtx->Connected_handle,
                                     &tNciTranscvInfo,
                                     &phLibNfc_InternalSeq,
                                     (void *)pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MFCIncDecRestoreResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phNciNfc_Data_t *pReceiveData;
    pReceiveData = (phNciNfc_Data_t *)pInfo;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus && pReceiveData->pBuff[0] == 0x0A)
    {
        PH_LOG_LIBNFC_INFO_STR("Increment/Decrement/Restore Command Header success");
        PHLIBNFC_INIT_SEQUENCE(pCtx,gphLibNfc_MFCIncDecRestorePayload);
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Increment/Decrement/Restore Command Header failed!");
        if(NFCSTATUS_SUCCESS == wStatus)
            wStatus = NFCSTATUS_FAILED;
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MFCTransferResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    phNciNfc_Data_t *pReceiveData;
    pReceiveData = (phNciNfc_Data_t *)pInfo;
    UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus && pReceiveData->pBuff[0] == 0x0A)
    {
        PH_LOG_LIBNFC_INFO_STR("Transfer Command Header success");
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Transfer Command Header failed!");
        if(NFCSTATUS_SUCCESS == wStatus)
            wStatus = NFCSTATUS_FAILED;
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MFCIncDecRestorePayloadResp(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Increment/Decrement/Restore Payload Command Header success");
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Increment/Decrement/Restore Payload Command Header failed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MFCIncDecRestorePayload(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    phNciNfc_TransceiveInfo_t tNciTranscvInfo;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phLibNfc_sTransceiveInfo_t *psTransceiveInfo = NULL;
    wStatus = NFCSTATUS_FAILED;
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pCtx &&
       NULL != pCtx->psTransceiveInfo)
    {
        psTransceiveInfo = pCtx->psTransceiveInfo;

        if((NULL != psTransceiveInfo->sSendData.buffer) &&
            (0 != psTransceiveInfo->sSendData.length) &&
            (NULL != psTransceiveInfo->sRecvData.buffer) &&
            (0 != psTransceiveInfo->sRecvData.length))
        {
            tNciTranscvInfo.tSendData.pBuff = psTransceiveInfo->sSendData.buffer;
            tNciTranscvInfo.tSendData.wLen = (uint16_t)psTransceiveInfo->sSendData.length;
            tNciTranscvInfo.uCmd.T2TCmd = phNciNfc_eT2TRaw;
            tNciTranscvInfo.tRecvData.pBuff = psTransceiveInfo->sRecvData.buffer;
            tNciTranscvInfo.tRecvData.wLen = (uint16_t)psTransceiveInfo->sRecvData.length;
            tNciTranscvInfo.wTimeout = psTransceiveInfo->timeout;

            wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                         pCtx->Connected_handle,
                                         &tNciTranscvInfo,
                                         &phLibNfc_InternalSeq,
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

static NFCSTATUS phLibNfc_MFCIncDecRestoreTransferComplete(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phNciNfc_Data_t pInf;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    UNUSED(pInfo)
    phOsalNfc_SetMemory(&pInf,0x00,sizeof(phNciNfc_Data_t));

    if(NULL != pCtx &&
       NULL != pCtx->psTransceiveInfo &&
       NULL != pCtx->psTransceiveInfo->sSendData.buffer &&
       0 != pCtx->psTransceiveInfo->sRecvData.length)
    {
        pInf.pBuff = pCtx->psTransceiveInfo->sSendData.buffer;
        pInf.wLen = 0;
        phOsalNfc_FreeMemory(pCtx->psTransceiveInfo);
        pCtx->psTransceiveInfo = NULL;

        if(NULL != pCtx->psDummyTransceiveInfo)
        {
            if(NULL != pCtx->psDummyTransceiveInfo->sRecvData.buffer)
            {
                phOsalNfc_FreeMemory(pCtx->psDummyTransceiveInfo->sRecvData.buffer);
                pCtx->psDummyTransceiveInfo->sRecvData.buffer = NULL;
            }
            phOsalNfc_FreeMemory(pCtx->psDummyTransceiveInfo);
            pCtx->psDummyTransceiveInfo = NULL;
        }
        if(NULL != pCtx->psTransceiveInfo1)
        {
            if(NULL != pCtx->psTransceiveInfo1->sSendData.buffer)
            {
                phOsalNfc_FreeMemory(pCtx->psTransceiveInfo1->sSendData.buffer);
                pCtx->psTransceiveInfo1->sSendData.buffer = NULL;
            }
            phOsalNfc_FreeMemory(pCtx->psTransceiveInfo1);
            pCtx->psTransceiveInfo1 = NULL;
        }
    }

    PH_LOG_LIBNFC_FUNC_EXIT();

    (void )phLibNfc_TranscvCb(pContext,wStatus,&pInf);

    return wStatus;
}

static NFCSTATUS phLibNfc_MFCIncDecRestorePayloadComplete(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    NFCSTATUS wTempStatus = wStatus;
    phNciNfc_Data_t pInf;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    UNUSED(pInfo)
    phOsalNfc_SetMemory(&pInf,0x00,sizeof(phNciNfc_Data_t));

    wTempStatus = PHNFCSTATUS(wStatus);

    if(wTempStatus == NFCSTATUS_RF_TIMEOUT_ERROR)
    {
        wStatus = NFCSTATUS_SUCCESS;
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }

    if(NULL != pCtx &&
       NULL != pCtx->psTransceiveInfo &&
       NULL != pCtx->psTransceiveInfo->sSendData.buffer &&
       0 != pCtx->psTransceiveInfo->sRecvData.length)
    {
        pInf.pBuff = pCtx->psTransceiveInfo->sSendData.buffer;
        pInf.wLen = 0;

        phOsalNfc_FreeMemory(pCtx->psTransceiveInfo);
        pCtx->psTransceiveInfo = NULL;

        if(NULL != pCtx->psDummyTransceiveInfo)
        {
            if(NULL != pCtx->psDummyTransceiveInfo->sRecvData.buffer)
            {
                phOsalNfc_FreeMemory(pCtx->psDummyTransceiveInfo->sRecvData.buffer);
                pCtx->psDummyTransceiveInfo->sRecvData.buffer = NULL;
            }
            phOsalNfc_FreeMemory(pCtx->psDummyTransceiveInfo);
            pCtx->psDummyTransceiveInfo = NULL;
        }
        if(NULL != pCtx->psTransceiveInfo1)
        {
            if(NULL != pCtx->psTransceiveInfo1->sSendData.buffer)
            {
                phOsalNfc_FreeMemory(pCtx->psTransceiveInfo1->sSendData.buffer);
                pCtx->psTransceiveInfo1->sSendData.buffer = NULL;
            }
            phOsalNfc_FreeMemory(pCtx->psTransceiveInfo1);
            pCtx->psTransceiveInfo1 = NULL;
        }
    }

    (void )phLibNfc_TranscvCb(pContext,wStatus,&pInf);

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
