/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"
#include "phLibNfc_Internal.h"

#include "phFriNfc_OvrHal.tmh"

#define MAX_MIF_PACKET_LEN                      0x0FU
#define MIFARE_PLUS_UID_INDEX_TO_COPY           0x03U
#define MIFARE_PLUS_UID_LENGTH                  0x07U
#define MIFARE_CLASSIC_UID_LENGTH               0x04U
#define MIFARE_UID_LEN_TO_COPY                  0x04U

static void phFriNfc_OvrHal_CB_Send(void *context,
                                    NFCSTATUS status);
static void phFriNfc_OvrHal_CB_Receive(void *context,
                                       phNfc_sData_t *pDataInfo,
                                       NFCSTATUS status);

static void phFriNfc_OvrHal_CB_Transceive(void *context,
                               phLibNfc_Handle hRemoteDev,
                               phNfc_sData_t  *pRecvdata,
                               NFCSTATUS status
                               );
static void phFriNfc_OvrHal_CB_ConnectDisconnect(void *context,
                               phLibNfc_Handle                     hRemoteDev,
                               phLibNfc_sRemoteDevInformation_t    *psRemoteDevInfo,
                               NFCSTATUS                           status);

static void  phFriNfc_OvrHal_SetComplInfo(phFriNfc_OvrHal_t *OvrHal,
                                   phFriNfc_CplRt_t  *CompletionInfo,
                                   uint8_t            Operation);

NFCSTATUS
phFriNfc_OvrHal_Transceive(
    _Out_                           phFriNfc_OvrHal_t                   *OvrHal,
    _In_                            phFriNfc_CplRt_t                    *CompletionInfo,
    _In_                            phLibNfc_sRemoteDevInformation_t    *RemoteDevInfo,
    _In_                            phNfc_uCmdList_t                    Cmd,
    _In_                            phNfc_sDepAdditionalInfo_t          *DepAdditionalInfo,
    _In_reads_bytes_(SendLength)    uint8_t                             *SendBuf,
    _In_                            uint16_t                            SendLength,
    _In_reads_bytes_(*RecvLength)   uint8_t                             *RecvBuf,
    _In_                            uint16_t                            *RecvLength
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;
    uint8_t i = 0;
    uint32_t length = SendLength;
    PH_LOG_FRI_FUNC_ENTRY();

    PHNFC_UNUSED_VARIABLE(DepAdditionalInfo);

    if ((NULL == OvrHal) || (NULL == CompletionInfo) || (NULL == RemoteDevInfo)
        || (NULL == (void*)SendBuf) || (NULL == RecvBuf) || (NULL == RecvLength)
        || ((phHal_eJewel_PICC != RemoteDevInfo->RemDevType) && (0 == SendLength)))

    {
        status = PHNFCSTVAL(CID_FRI_NFC_OVR_HAL, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* 16 is the maximum data, that can be sent to the mifare standard */
        static uint8_t      mif_send_buf[MAX_MIF_PACKET_LEN] = {0};
        /* Populate the Transfer info structure */
        OvrHal->TranceiveInfo.cmd = Cmd;

        /* Populate the Send Buffer Info */
        if((phNfc_eMifare_PICC == RemoteDevInfo->RemDevType)
            || (phNfc_eISO14443_3A_PICC == RemoteDevInfo->RemDevType))
        {
            OvrHal->TranceiveInfo.addr = SendBuf[i++];
            length = (SendLength - i);

            if ((phNfc_eMifareAuthentA == Cmd.MfCmd)
                || (phNfc_eMifareAuthentB == Cmd.MfCmd))
            {
                uint8_t     uid_index = 0;
                /* Authentication requires UID in the send buffer */
                uint8_t     uid_len =
                    RemoteDevInfo->RemoteDevInfo.Iso14443A_Info.UidLength;
                OvrHal->TranceiveInfo.sSendData.buffer = mif_send_buf;

                switch (uid_len)
                {
                    case MIFARE_PLUS_UID_LENGTH:
                    {
                        uid_index = MIFARE_PLUS_UID_INDEX_TO_COPY;
                        uid_len = MIFARE_UID_LEN_TO_COPY;
                        break;
                    }

                    case MIFARE_CLASSIC_UID_LENGTH:
                    {
                        uid_index = 0;
                        break;
                    }

                    default:
                    {
                        status = PHNFCSTVAL (CID_FRI_NFC_OVR_HAL,
                                            NFCSTATUS_READ_FAILED);
                        break;
                    }
                }

                if (NFCSTATUS_PENDING == status)
                {
                    /* copy uid to the send buffer for the authentication */
                    phOsalNfc_MemCopy((void *)mif_send_buf,
                        (void *)&RemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Uid[uid_index],
                        uid_len);

                    phOsalNfc_MemCopy((mif_send_buf + uid_len), &(SendBuf[i]), length);
                    length += uid_len;
                }
            }
            else
            {
                OvrHal->TranceiveInfo.sSendData.buffer = &SendBuf[i++];
            }
            OvrHal->TranceiveInfo.sSendData.length = length;
        }
        else
        {
            OvrHal->TranceiveInfo.sSendData.buffer = &SendBuf[i++];
            OvrHal->TranceiveInfo.sSendData.length = length;
        }

        if (NFCSTATUS_PENDING == status)
        {
            /* Populate the Receive buffer */
            OvrHal->TranceiveInfo.sRecvData.buffer = RecvBuf;
            OvrHal->TranceiveInfo.sRecvData.length = *RecvLength;
            OvrHal->pndef_recv_length = RecvLength;
            phFriNfc_OvrHal_SetComplInfo(OvrHal,CompletionInfo, PH_FRINFC_OVRHAL_TRX);
            /* Increase the timeout values for Desfire Tag */
            if( ((phNfc_eISO14443_4A_PICC == RemoteDevInfo->RemDevType) ||\
                 (phNfc_eISO14443_4B_PICC == RemoteDevInfo->RemDevType) ||\
                 (phNfc_eISO14443_3A_PICC == RemoteDevInfo->RemDevType)) &&\
                 (0x20 == (RemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak & 0x20)) )
            {
                OvrHal->TranceiveInfo.timeout = 500;
            }
            else
            {
                OvrHal->TranceiveInfo.timeout = 300;
            }
            status = phLibNfc_RemoteDev_Transceive((phLibNfc_Handle)RemoteDevInfo,
                        (phLibNfc_sTransceiveInfo_t *)&OvrHal->TranceiveInfo,
                        (pphLibNfc_TransceiveCallback_t)  &phFriNfc_OvrHal_CB_Transceive,
                        (void *)OvrHal);
        }
    }
    PH_LOG_FRI_FUNC_EXIT();
    return status;
}

NFCSTATUS
phFriNfc_OvrHal_Receive(
    _In_                    phFriNfc_OvrHal_t               *OvrHal,
    _In_                    phFriNfc_CplRt_t                *CompletionInfo,
    _In_                    phNfc_sRemoteDevInformation_t   *RemoteDevInfo,
    _In_reads_(*RecvLength) uint8_t                         *RecvBuf,
    _In_                    uint16_t                        *RecvLength
    )
{
   NFCSTATUS status = NFCSTATUS_PENDING;

   if(   (NULL==OvrHal)  || (NULL==CompletionInfo) || (NULL==RemoteDevInfo)
      || (NULL==RecvBuf) || (NULL==RecvLength) )
   {
      status = PHNFCSTVAL(CID_FRI_NFC_OVR_HAL ,NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Get the remote dev type */
      OvrHal->TransactInfo.remotePCDType = RemoteDevInfo->RemDevType;
      /* Save the receive buffer for use in callback */
      OvrHal->sReceiveData.buffer = RecvBuf;
      OvrHal->sReceiveData.length = *RecvLength;

      OvrHal->pndef_recv_length = RecvLength;

      /* Set the callback */
      phFriNfc_OvrHal_SetComplInfo(OvrHal, CompletionInfo, PH_FRINFC_OVRHAL_RCV);

      /* Call the HAL 4.0 Receive Function */
      status = phLibNfc_RemoteDev_Receive((phLibNfc_Handle)RemoteDevInfo,
                                  phFriNfc_OvrHal_CB_Receive,
                                  (void *)OvrHal);
      if(NFCSTATUS_PENDING != status)
      {
          OvrHal->bRecvReq = 1;
          OvrHal->pRemoteDevInfo = (void *)RemoteDevInfo;
          status = NFCSTATUS_PENDING;
      }
   }
   return status;
}

NFCSTATUS phFriNfc_OvrHal_Send(phFriNfc_OvrHal_t              *OvrHal,
                               phFriNfc_CplRt_t               *CompletionInfo,
                               phNfc_sRemoteDevInformation_t  *RemoteDevInfo,
                               uint8_t                        *SendBuf,
                               uint16_t                       SendLength)
{
   NFCSTATUS status = NFCSTATUS_PENDING;

   if(   (NULL==OvrHal) || (NULL==CompletionInfo) || (NULL==RemoteDevInfo) || (NULL==SendBuf)  )
   {
      status = PHNFCSTVAL(CID_FRI_NFC_OVR_HAL ,NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Get the remote dev type */
      OvrHal->TransactInfo.remotePCDType = RemoteDevInfo->RemDevType;
      /* Save the receive buffer for use in callback */
      OvrHal->sSendData.buffer = SendBuf;
      OvrHal->sSendData.length = SendLength;

      /* Set the callback */
      phFriNfc_OvrHal_SetComplInfo(OvrHal, CompletionInfo, PH_FRINFC_OVRHAL_SND);

      status = phLibNfc_RemoteDev_Send((phLibNfc_Handle)RemoteDevInfo,
                                    &(OvrHal->sSendData),
                                    phFriNfc_OvrHal_CB_Send,
                                    (void *)OvrHal
                                    );
   }
   return status;
}

NFCSTATUS phFriNfc_OvrHal_Reconnect(phFriNfc_OvrHal_t              *OvrHal,
                                     phFriNfc_CplRt_t               *CompletionInfo,
                                     phNfc_sRemoteDevInformation_t  *RemoteDevInfo)
{
    NFCSTATUS status = NFCSTATUS_PENDING;
    NFCSTATUS wStatus;
    phNciNfc_RemoteDevInformation_t *pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = NULL;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_FRI_FUNC_ENTRY();
    if((NULL == OvrHal) || (NULL == CompletionInfo) || (NULL == RemoteDevInfo))
    {
        status = PHNFCSTVAL(CID_FRI_NFC_OVR_HAL ,NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        phFriNfc_OvrHal_SetComplInfo(OvrHal, CompletionInfo, PH_FRINFC_OVRHAL_DIS);
        pNciRemDevHandle = (phNciNfc_RemoteDevInformation_t *)pLibContext->Connected_handle;

        wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemDevHandle,PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            status = phLibNfc_RemoteDev_Connect((phLibNfc_Handle)pLibRemDevHandle,
                                                (pphLibNfc_ConnectCallback_t) &phFriNfc_OvrHal_CB_ConnectDisconnect,
                                                (void *)OvrHal);
        }
        else
        {
            status = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_FRI_FUNC_EXIT();
    return status;
}

NFCSTATUS phFriNfc_OvrHal_Connect(phFriNfc_OvrHal_t              *OvrHal,
                                        phFriNfc_CplRt_t               *CompletionInfo,
                                        phHal_sRemoteDevInformation_t  *RemoteDevInfo,
                                        phHal_sDevInputParam_t         *DevInputParam)
{
    NFCSTATUS status = NFCSTATUS_PENDING;
    phNciNfc_RemoteDevInformation_t *pNciRemDevHandle = NULL;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_FRI_FUNC_ENTRY();
    if((NULL == OvrHal) || (NULL == CompletionInfo) || (NULL == RemoteDevInfo) ||
        (NULL == DevInputParam))
    {
        status = PHNFCSTVAL(CID_FRI_NFC_OVR_HAL ,NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        phFriNfc_OvrHal_SetComplInfo(OvrHal, CompletionInfo, PH_FRINFC_OVRHAL_CON);

        /* Map Nci remote device to LibNfc remote device handle handle */
        pNciRemDevHandle = (phNciNfc_RemoteDevInformation_t *)pLibContext->Connected_handle;
        if(NULL != pNciRemDevHandle)
        {
            status = phLibNfc_MapRemoteDevHandle(&RemoteDevInfo,
                                            &pNciRemDevHandle,
                                            PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
        }
        else
        {
            status = NFCSTATUS_FAILED;
        }


        if(status != NFCSTATUS_FAILED)
        {
            status = phLibNfc_RemoteDev_Connect((phLibNfc_Handle)RemoteDevInfo,
                                                &phFriNfc_OvrHal_CB_ConnectDisconnect,
                                                (void *)OvrHal);
        }
    }
    PH_LOG_FRI_FUNC_EXIT();
    return status;
}


static void phFriNfc_OvrHal_CB_Transceive(void *context,
                               phLibNfc_Handle hRemoteDev,
                               phNfc_sData_t  *pRecvdata,
                               NFCSTATUS status
                               )

{
    phFriNfc_OvrHal_t       *OvrHal = (phFriNfc_OvrHal_t *)context;
    PH_LOG_FRI_FUNC_ENTRY();
    UNUSED(hRemoteDev);

    if (NULL != OvrHal)
    {
        if (NULL != pRecvdata)
        {
            *OvrHal->pndef_recv_length = (uint16_t) pRecvdata->length;
        }

        if (NULL != OvrHal->TemporaryCompletionInfo.CompletionRoutine)
        {
            OvrHal->TemporaryCompletionInfo.CompletionRoutine(
                OvrHal->TemporaryCompletionInfo.Context,
                status);
        }
    }
    PH_LOG_FRI_FUNC_EXIT();
}

static void phFriNfc_OvrHal_CB_Send(void *context,
                                    NFCSTATUS status)
{
    phFriNfc_OvrHal_t *OvrHal = (phFriNfc_OvrHal_t *)context;
    phNfc_sRemoteDevInformation_t  *RemoteDevInfo = NULL;
    if (NULL != OvrHal)
    {
        if (NULL != OvrHal->TemporarySndCompletionInfo.CompletionRoutine)
        {
            /* Check if LLCP Read has failed earlier */
            if(1 == OvrHal->bRecvReq)
            {
                OvrHal->bRecvReq = 0;
                RemoteDevInfo = (phNfc_sRemoteDevInformation_t *)OvrHal->pRemoteDevInfo;
                /* Call the HAL 4.0 Receive Function */
                (void )phLibNfc_RemoteDev_Receive((phLibNfc_Handle)RemoteDevInfo,
                                          phFriNfc_OvrHal_CB_Receive,
                                          (void *)OvrHal);
            }

            OvrHal->TemporarySndCompletionInfo.CompletionRoutine(
                OvrHal->TemporarySndCompletionInfo.Context,
                status);
        }
    }
}

static void phFriNfc_OvrHal_CB_Receive(void *context,
                                       phNfc_sData_t *pDataInfo,
                                       NFCSTATUS status)
{
    phFriNfc_OvrHal_t *OvrHal = (phFriNfc_OvrHal_t *)context;

    if (NULL != OvrHal)
    {
        /* Copy the received buffer */
        if(NULL != pDataInfo && OvrHal->sReceiveData.buffer != NULL && pDataInfo->buffer != NULL)
        {
            phOsalNfc_MemCopy(OvrHal->sReceiveData.buffer, pDataInfo->buffer, pDataInfo->length);
            *OvrHal->pndef_recv_length = (uint16_t) pDataInfo->length;
        }

        if (NULL != OvrHal->TemporaryRcvCompletionInfo.CompletionRoutine)
        {
            OvrHal->TemporaryRcvCompletionInfo.CompletionRoutine(
                OvrHal->TemporaryRcvCompletionInfo.Context,
                status);
        }
    }
}

static void phFriNfc_OvrHal_CB_ConnectDisconnect(void *context,
                               phLibNfc_Handle                     hRemoteDev,
                               phLibNfc_sRemoteDevInformation_t    *psRemoteDevInfo,
                               NFCSTATUS                           status)

{
    phFriNfc_OvrHal_t   *OvrHal = (phFriNfc_OvrHal_t *)context;
    PH_LOG_FRI_FUNC_ENTRY();
    UNUSED(psRemoteDevInfo);
    if (NULL != OvrHal)
    {
        if (hRemoteDev == NULL)
        {
            status = NFCSTATUS_FAILED;
        }

        OvrHal->TemporaryCompletionInfo.CompletionRoutine(
            OvrHal->TemporaryCompletionInfo.Context, status);
    }
    PH_LOG_FRI_FUNC_EXIT();
}

static void phFriNfc_OvrHal_SetComplInfo(phFriNfc_OvrHal_t *OvrHal,
                                   phFriNfc_CplRt_t  *CompletionInfo,
                                   uint8_t            Operation)

{
    OvrHal->Operation = Operation;
    switch (Operation)
    {
        case PH_FRINFC_OVRHAL_RCV:
        {
            OvrHal->TemporaryRcvCompletionInfo.CompletionRoutine = CompletionInfo->CompletionRoutine;
            OvrHal->TemporaryRcvCompletionInfo.Context = CompletionInfo->Context;
            break;
        }
        case PH_FRINFC_OVRHAL_SND:
        {
            OvrHal->TemporarySndCompletionInfo.CompletionRoutine = CompletionInfo->CompletionRoutine;
            OvrHal->TemporarySndCompletionInfo.Context = CompletionInfo->Context;
            break;
        }
        default:
        {
            OvrHal->TemporaryCompletionInfo.CompletionRoutine = CompletionInfo->CompletionRoutine;
            OvrHal->TemporaryCompletionInfo.Context = CompletionInfo->Context;
            break;
        }
    }
}
