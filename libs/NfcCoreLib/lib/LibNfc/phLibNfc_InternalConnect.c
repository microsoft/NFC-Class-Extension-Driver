/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_InternalConnect.tmh"

static NFCSTATUS phLibNfc_SendTranscvCmd(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_GetTranscvResp(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_InvokeNciTranscvComplete(void *pContext,NFCSTATUS wStatus,void *pInfo);

static NFCSTATUS phLibNfc_PrepareCnkt_TxvPkt_MifareUL(void *pContext, void *pNciRemoteDevHandle, phNciNfc_TransceiveInfo_t *pNciTranscvInfo);
static NFCSTATUS phLibNfc_PrepareCnkt_TxvPkt_Jewel(void *pContext, void *pNciRemoteDevHandle, phNciNfc_TransceiveInfo_t *pNciTranscvInfo);
static NFCSTATUS phLibNfc_PrepareConnect_Transvpacket_Iso15693(void *pContext, void *pNciRemoteDevHandle, phNciNfc_TransceiveInfo_t *pNciTranscvInfo);
static NFCSTATUS phLibNfc_PrepareCnkt_TxvPkt_ISO14443_4A(void *pContext, void *pNciRemoteDevHandle, phNciNfc_TransceiveInfo_t *pNciTranscvInfo);
static NFCSTATUS phLibNfc_PrepareCnkt_TxvPkt_Felica(void *pContext, void *pNciRemoteDevHandle, phNciNfc_TransceiveInfo_t *pNciTranscvInfo);

static phLibNfc_Sequence_t gphLibNfc_InvokeNciTranscvSeq[] = {
    {&phLibNfc_SendTranscvCmd, &phLibNfc_GetTranscvResp},
    {NULL, &phLibNfc_InvokeNciTranscvComplete}
};

NFCSTATUS
phLibNfc_InternConnect(void *pContext,void *pNciRemoteDevhandle,void *pLibRemotDevhandle)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle=NULL;
    phNciNfc_TransceiveInfo_t tNciTranscvInfo;

    if((NULL != pLibContext) &&(NULL != pNciRemoteDevhandle ) &&(NULL != pLibRemotDevhandle))
    {
        pLibRemoteDevHandle = (phLibNfc_sRemoteDevInformation_t *)pLibRemotDevhandle;

        if((phNfc_eMifare_PICC == pLibRemoteDevHandle->RemDevType) ||
           (phNfc_eISO14443_3A_PICC == pLibRemoteDevHandle->RemDevType))
        {
            wStatus = phLibNfc_PrepareCnkt_TxvPkt_MifareUL(pContext,\
                                                           pNciRemoteDevhandle,\
                                                           &tNciTranscvInfo);
        }
        else if((phNfc_eISO14443_4A_PICC == pLibRemoteDevHandle->RemDevType) ||
                (phNfc_eISO14443_4B_PICC == pLibRemoteDevHandle->RemDevType))
        {
            wStatus = phLibNfc_PrepareCnkt_TxvPkt_ISO14443_4A(pContext,\
                                                              pNciRemoteDevhandle,\
                                                              &tNciTranscvInfo);
        }
        else if(phNfc_eJewel_PICC == pLibRemoteDevHandle->RemDevType )
        {
            wStatus = phLibNfc_PrepareCnkt_TxvPkt_Jewel(pContext,\
                                                        pNciRemoteDevhandle,\
                                                        &tNciTranscvInfo);
        }
        else if(phNfc_eFelica_PICC == pLibRemoteDevHandle->RemDevType )
        {
            wStatus = phLibNfc_PrepareCnkt_TxvPkt_Felica(pContext,\
                                                         pNciRemoteDevhandle,\
                                                         &tNciTranscvInfo);
        }
        else if(phNfc_eISO15693_PICC == pLibRemoteDevHandle->RemDevType)
        {
            wStatus = phLibNfc_PrepareConnect_Transvpacket_Iso15693(pContext,\
                                                                    pNciRemoteDevhandle,\
                                                                    &tNciTranscvInfo);
        }

        if(NFCSTATUS_FAILED != wStatus &&
           NFCSTATUS_INVALID_PARAMETER != wStatus )
        {
            tNciTranscvInfo.wTimeout = 300;

            PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_InvokeNciTranscvSeq);
            wStatus = phLibNfc_SeqHandler(pLibContext,wStatus,&tNciTranscvInfo);
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}

NFCSTATUS
phLibNfc_ConnectTransv_Process_Info(pphNciNfc_Data_t pRecvData,void *pNciRemDevHandle,void *pLibRemDevHandle)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle = NULL;
    phNciNfc_RemoteDevInformation_t *pNciRemoteDevHandle = NULL;
    phNfc_sData_t      tResBuffer;

    if((NULL != pRecvData) && (NULL != pNciRemDevHandle) && (NULL != pLibRemDevHandle))
    {
        pLibRemoteDevHandle = (phLibNfc_sRemoteDevInformation_t *)pLibRemDevHandle;
        pNciRemoteDevHandle = (phNciNfc_RemoteDevInformation_t *)pNciRemDevHandle;

        wStatus = phLibNfc_VerifyResponse(pRecvData,pNciRemoteDevHandle);

        tResBuffer.buffer = pRecvData->pBuff;
        tResBuffer.length = pRecvData->wLen;

        if((phNfc_eISO14443_4A_PICC == pLibRemoteDevHandle->RemDevType) ||
           (phNfc_eISO14443_4B_PICC == pLibRemoteDevHandle->RemDevType))
        {
            if(2 == tResBuffer.length)
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned valid BuffLen");
            }
            else
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Invalid BuffLen!!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(phNfc_eJewel_PICC == (pLibRemoteDevHandle->RemDevType))
        {
            if(0x06 == tResBuffer.length)
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned valid BuffLen");
            }
            else
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Invalid BuffLen!!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(phNfc_eFelica_PICC == (pLibRemoteDevHandle->RemDevType))
        {
            if(tResBuffer.length > 0)
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned valid BuffLen");
            }
            else
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Invalid BuffLen!!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(phNfc_eISO15693_PICC == (pLibRemoteDevHandle->RemDevType))
        {
            if(tResBuffer.length >= 1)
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned valid BuffLen");
            }
            else
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Invalid BuffLen!!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}

void phLibNfc_InternConnect_Cb(void *   pContext,\
                               NFCSTATUS status,\
                               pphNciNfc_Data_t pRecvData)
 {
    NFCSTATUS wStatus = status;
    NFCSTATUS wRetVal = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext=(pphLibNfc_LibContext_t)pContext;
    pphLibNfc_ConnectCallback_t pClientConnectCb=NULL;
    pphNciNfc_Data_t pTransactInfo = (pphNciNfc_Data_t)pRecvData;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle = NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if( (NULL == pLibContext) || (pLibContext != phLibNfc_GetContext()) )
    {
        PH_LOG_LIBNFC_CRIT_STR("Wrong libNfc context received from NCI layer");
        wStatus = NFCSTATUS_FAILED;
    }
    else
    {
        pClientConnectCb = pLibContext->CBInfo.pClientConnectCb;

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            pNciRemoteDevHandle = (pphNciNfc_RemoteDevInformation_t)pLibContext->DummyConnect_handle;
            wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,
                                                  &pNciRemoteDevHandle,
                                                  PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_LIBNFC_CRIT_STR("Mapping of Nci RemoteDev Handle to LibNfc RemoteDev handle Failed");
                wStatus = NFCSTATUS_FAILED;
            }
            else
            {
                wStatus = phLibNfc_ConnectTransv_Process_Info(pTransactInfo,pNciRemoteDevHandle,pLibRemoteDevHandle);
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }

        phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus),&TrigEvent);
        wRetVal = phLibNfc_StateHandler(pLibContext, TrigEvent, pRecvData, NULL, NULL);

        if(wRetVal != NFCSTATUS_SUCCESS)
        {
            PH_LOG_LIBNFC_CRIT_STR("State machine has returned NFCSTATUS_FAILED");
            wStatus = NFCSTATUS_FAILED;
        }

        if((wStatus == NFCSTATUS_SUCCESS) && (NULL != pLibRemoteDevHandle) && (NULL != pNciRemoteDevHandle))
        {
            pLibContext->Connected_handle = pNciRemoteDevHandle;
            pLibRemoteDevHandle->SessionOpened = pNciRemoteDevHandle->SessionOpened;
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }

        if(pClientConnectCb != NULL)
        {
            pLibContext->CBInfo.pClientConnectCb = NULL;
            PH_LOG_LIBNFC_INFO_STR("Invoking upper layer callback");
            (pClientConnectCb)((void *)pLibContext->CBInfo.pClientConCntx,
                                (phLibNfc_Handle)pLibRemoteDevHandle,
                                pLibRemoteDevHandle,
                                wStatus);
        }
    }
}

static NFCSTATUS
phLibNfc_PrepareCnkt_TxvPkt_MifareUL(void *pContext,
                                              void *pNciRemoteDevHandle,
                                              phNciNfc_TransceiveInfo_t *pNciTranscvInfo)
{
    NFCSTATUS wStatus;
    phLibNfc_sTransceiveInfo_t TransceiveInfo;
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    static uint8_t bRATSRespBuff[19];

    if(NULL != pLibContext)
    {
        phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
        TransceiveInfo.addr = 0x02;
        TransceiveInfo.NumBlock = 1;
        TransceiveInfo.cmd.MfCmd = phNfc_eMifareRead16;
        TransceiveInfo.sRecvData.buffer = bRATSRespBuff;
        TransceiveInfo.sRecvData.length = 19;
        TransceiveInfo.sSendData.buffer = NULL;
        TransceiveInfo.sSendData.length = 0;

        wStatus = phLibNfc_MapCmds(((pphNciNfc_RemoteDevInformation_t )pNciRemoteDevHandle)->RemDevType,
                                    &TransceiveInfo,
                                    pNciTranscvInfo);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}

static NFCSTATUS
phLibNfc_PrepareCnkt_TxvPkt_ISO14443_4A(void *pContext,
                                                 void *pNciRemoteDevHandle,
                                                 phNciNfc_TransceiveInfo_t *pNciTranscvInfo)
{
    NFCSTATUS wStatus;
    phLibNfc_sTransceiveInfo_t TransceiveInfo;
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    static uint8_t b4ACmdBuff[] = {0x00,0xA4,0x00,0x00,0x04,0x00,0x00}; /*Send wrong length*/
    static uint8_t b4ARespBuff[2];

    if(NULL != pLibContext)
    {
        phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
        TransceiveInfo.cmd.Iso144434Cmd = phNfc_eIso14443_4_Raw;
        TransceiveInfo.sRecvData.buffer = b4ARespBuff;
        TransceiveInfo.sRecvData.length = sizeof(b4ARespBuff);
        TransceiveInfo.sSendData.buffer = b4ACmdBuff;
        TransceiveInfo.sSendData.length = sizeof(b4ACmdBuff);

        wStatus = phLibNfc_MapCmds(((pphNciNfc_RemoteDevInformation_t )pNciRemoteDevHandle)->RemDevType,
                                    &TransceiveInfo,
                                    pNciTranscvInfo);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}

static
NFCSTATUS phLibNfc_PrepareConnect_Transvpacket_Iso15693(void *pContext,\
                                                        void *pNciRemoteDevHandle,
                                                        phNciNfc_TransceiveInfo_t *pNciTranscvInfo)
{
    NFCSTATUS wStatus;
    phLibNfc_sTransceiveInfo_t TransceiveInfo;
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    pphNciNfc_RemoteDevInformation_t pNciRemDevHandle = pNciRemoteDevHandle;
    uint8_t request_flags = 0;
    static uint8_t bIso15693CmdBuff[16] = {0};
    static uint8_t bIso15693RespBuff[38] = {0};

    if((NULL != pLibContext) && (NULL != pNciRemDevHandle))
    {
        phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
        if (0 != pNciRemDevHandle->tRemoteDevInfo.Iso15693_Info.UidLength)
        {
            request_flags |= ISO15693_FLAG_UID;
        }

        if (ISO15693_PROTOEXT_FLAG_REQUIRED(pNciRemDevHandle->tRemoteDevInfo.Iso15693_Info.Uid))
        {
            request_flags |= ISO15693_FLAG_PROTOEXT;
        }

        request_flags |= ISO15693_FLAG_HIGH_DATARATE;

        bIso15693CmdBuff[TransceiveInfo.sSendData.length++] = request_flags;
        bIso15693CmdBuff[TransceiveInfo.sSendData.length++] = ISO15693_READ_COMMAND;
        
        if ((request_flags & ISO15693_FLAG_UID) != 0)
        {
            phOsalNfc_MemCopy(&bIso15693CmdBuff[TransceiveInfo.sSendData.length],
                            pNciRemDevHandle->tRemoteDevInfo.Iso15693_Info.Uid,
                            PH_NCINFCTYPES_15693_UID_LENGTH);
            TransceiveInfo.sSendData.length += PH_NCINFCTYPES_15693_UID_LENGTH;
        }

        bIso15693CmdBuff[TransceiveInfo.sSendData.length++] = 0x00;
        if ((request_flags & ISO15693_FLAG_PROTOEXT) != 0)
        {
            bIso15693CmdBuff[TransceiveInfo.sSendData.length++] = 0x00;
        }

        TransceiveInfo.cmd.Iso15693Cmd = phNfc_eIso15693_Raw;
        TransceiveInfo.sSendData.buffer = bIso15693CmdBuff;
        TransceiveInfo.sRecvData.buffer = bIso15693RespBuff;
        TransceiveInfo.sRecvData.length = sizeof(bIso15693RespBuff);
        wStatus = phLibNfc_MapCmds(pNciRemDevHandle->RemDevType,
                                    &TransceiveInfo,
                                    pNciTranscvInfo);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}

static
NFCSTATUS phLibNfc_PrepareCnkt_TxvPkt_Jewel(void *pContext,\
                                            void *pNciRemoteDevHandle,\
                                            phNciNfc_TransceiveInfo_t *pNciTranscvInfo)
{
    NFCSTATUS wStatus;
    phLibNfc_sTransceiveInfo_t TransceiveInfo;
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    static uint8_t bJewelCmdBuff[] = {0x78,0x00,0x00,0x00,0x00,0x00,0x00};
    static uint8_t bJewelRespBuff[6];

    if(NULL != pLibContext)
    {
        phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
        TransceiveInfo.cmd.JewelCmd = phNfc_eJewel_Raw;
        TransceiveInfo.sRecvData.buffer = bJewelRespBuff;
        TransceiveInfo.sRecvData.length = sizeof(bJewelRespBuff);
        TransceiveInfo.sSendData.buffer = bJewelCmdBuff;
        TransceiveInfo.sSendData.length = sizeof(bJewelCmdBuff);

        wStatus = phLibNfc_MapCmds(((pphNciNfc_RemoteDevInformation_t )pNciRemoteDevHandle)->RemDevType,
                                    &TransceiveInfo,
                                    pNciTranscvInfo);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}

static
NFCSTATUS phLibNfc_PrepareCnkt_TxvPkt_Felica(void *pContext,\
                                                     void *pNciRemoteDevHandle,
                                                     phNciNfc_TransceiveInfo_t *pNciTranscvInfo)
{
    NFCSTATUS wStatus;
    phLibNfc_sTransceiveInfo_t TransceiveInfo;
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle=NULL;
    static uint8_t bFelicaCmdBuff[0x10] = {0x10,        /*SoD Byte = PayloadLen + 1*/
                                           0x06,        /*Check Cmd Code - 1 byte*/
                                           0x01,0x27,0x00,0x5D,0x1A,0x0B,0xA1,0xAD,  /*IDm - 8 bytes from SENSF_RESP*/
                                           0x01,        /*No.of services - 1 byte*/
                                           0x0B,0x00,   /*Service Code List - (2 * No. of services) bytes*/
                                           0x01,        /*No. of Blocks - 1 byte*/
                                           0x80,        /*Byte0 of 2 byte BlockList => 1(Len)|000(Access Mode)|0000(SCLO)*/
                                           0x05         /*Byte1 of 2 byte BlockList => 0x05 (BlockNumber)*/
                                          };
    static uint8_t bRespBuff[27];
    uint8_t bIndex;
    uint8_t bIndex1;

    if((pNciRemoteDevHandle != NULL) &&
       (pNciTranscvInfo != NULL) &&
       (pLibContext != NULL))
    {
        wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,
                                              (phNciNfc_RemoteDevInformation_t **)&pNciRemoteDevHandle,
                                              PH_LIBNFC_INTERNAL_NCITOLIB_MAP);

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            for(bIndex = 2,bIndex1 = 0 ; bIndex1 < 8; bIndex++ ,bIndex1++)
            {
                bFelicaCmdBuff[bIndex] = pLibRemoteDevHandle->RemoteDevInfo.Felica_Info.IDm[bIndex1];
            }
            phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
            TransceiveInfo.cmd.FelCmd = phNfc_eFelica_Raw;
            TransceiveInfo.sRecvData.buffer = bRespBuff;
            TransceiveInfo.sRecvData.length = sizeof(bRespBuff);
            TransceiveInfo.sSendData.buffer = bFelicaCmdBuff;
            TransceiveInfo.sSendData.length = sizeof(bFelicaCmdBuff);

            wStatus = phLibNfc_MapCmds(((pphNciNfc_RemoteDevInformation_t )pNciRemoteDevHandle)->RemDevType,
                                        &TransceiveInfo,
                                        pNciTranscvInfo);
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

    return wStatus;
}

static NFCSTATUS phLibNfc_SendTranscvCmd(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pCtx)
    {
        wStatus = phNciNfc_Transceive((void *)pCtx->sHwReference.pNciHandle,
                                      (void *)(pCtx->Connected_handle),
                                       pInfo,
                                       &phLibNfc_InternalSeq,
                                      (void *)pCtx);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_GetTranscvResp(void *pContext,NFCSTATUS status,void *pInfo)
{
    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

static NFCSTATUS phLibNfc_InvokeNciTranscvComplete(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    (void)phLibNfc_InternConnect_Cb(pContext,wStatus,pInfo);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
