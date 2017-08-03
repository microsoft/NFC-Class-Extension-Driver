/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_Jewel.tmh"

/** Mask value to validate Byte 1 of Sens Resp for type-1 tag */
#define PH_NCINFC_SENSRES_MASKBYTE1                     (0x3E)

/** Mask value to validate Byte 2 of Sens Resp for type-1 tag */
#define PH_NCINFC_SENSRES_MASKBYTE2                     (0x0F)

/** Byte 1 value of Sens Resp for type-1 tag */
#define PH_NCINFC_SENSRES_BYTE1VAL                      (0x00)

/** Byte 2 value of Sens Resp for type-1 tag */
#define PH_NCINFC_SENSRES_BYTE2VAL                      (0x0C)

/** Length of UID of Type 1 Tag from NFCC */
#define PHNCINFC_UID_LEN                                (0x04)


NFCSTATUS phNciNfc_JewelInit(uint8_t *pSensRes)
{
    NFCSTATUS wInitStatus = NFCSTATUS_FAILED;
    if( (pSensRes[0] == PH_NCINFC_SENSRES_BYTE1VAL) &&
        ((pSensRes[1] & PH_NCINFC_SENSRES_MASKBYTE2) == PH_NCINFC_SENSRES_BYTE2VAL) )
    {
        wInitStatus = NFCSTATUS_SUCCESS;
    }
    return wInitStatus;
}

NFCSTATUS
phNciNfc_RecvJewelResp(
                        void                *psContext,
                        NFCSTATUS          wStatus
                       )
{
    NFCSTATUS                   wRespstatus = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t          *psNciContext = (phNciNfc_Context_t *)psContext;
    uint16_t                wPldDataSize = 0;
    uint16_t                wRecvDataSz  = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciContext)
    {
      wRespstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
      PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if( (0 == (psNciContext->RspBuffInfo.wLen))\
                || (PH_NCINFC_STATUS_OK != wStatus)
                || (NULL == (psNciContext->RspBuffInfo.pBuff))
                )
    {
        /* NOTE:- ( TODO) In this fail scenario,map the status code from response handler
        to the status code in Nci Context and use the same in the status below */
        wRespstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
        PH_LOG_NCI_INFO_STR(" Data Receive Failed..");
    }
    else
    {
         wPldDataSize = (psNciContext->RspBuffInfo.wLen);
         wRecvDataSz = (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen);

         /* Length of response = Length of data[atleast 1] + status byte[1 byte]
            check the status byte */
         if( (wPldDataSize > 0x01) &&\
             (PH_NCINFC_STATUS_OK == (psNciContext->RspBuffInfo.pBuff[wPldDataSize-1])) )
         {
             wRespstatus = NFCSTATUS_SUCCESS;
             PH_LOG_NCI_INFO_STR(" Jewel XchgData Request is Successful!! ..");

             if((wPldDataSize - 1) <= wRecvDataSz)
             {
                 /* Remove the ADD byte if the response is not of RID */
                 /* Check whether response for RID/RALL command is sent, if not, then
                    remove the ADD byte received as part of response*/
                if( (0x01 < wPldDataSize) &&
                    (0x06 != (wPldDataSize - 1)) &&
                    (0x7A != (wPldDataSize - 1)) )
                {
                    /* ADD byte is removed if the respone is other than RID */
                    /* Extract the data part from pBuff[1] & fill it to be sent to upper layer */
                     phOsalNfc_MemCopy((psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff),
                         &(psNciContext->RspBuffInfo.pBuff[1]),(wPldDataSize - 2));
                     /* update the number of bytes received from lower layer,excluding the status byte */
                     (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen) = (wPldDataSize - 2);

                }
                else if (0x00 < wPldDataSize)
                {
                     /* Extract the data part from pBuff[0] & fill it to be sent to upper layer */
                     phOsalNfc_MemCopy((psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff),
                         &(psNciContext->RspBuffInfo.pBuff[0]),(wPldDataSize - 1));
                     /* update the number of bytes received from lower layer,excluding the status byte */
                     (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen) = (wPldDataSize - 1);
                }
                else
                {
                    wRespstatus = PHNFCSTVAL(CID_NFC_NCI,NFCSTATUS_NOT_ENOUGH_MEMORY);
                    PH_LOG_NCI_CRIT_STR(" Mf XchgData, Not enough Data available..");
                }

             }
             else
             {
                 /* Todo:- Map some status for remaining extra data received to be sent back to caller?? */
                wRespstatus = PHNFCSTVAL(CID_NFC_NCI,NFCSTATUS_MORE_INFORMATION);
                PH_LOG_NCI_INFO_STR(" Mf XchgData,More Data available than requested  ..");
             }
         }
         else
         {
             wRespstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
             PH_LOG_NCI_INFO_STR(" Mf XchgData Failed ..");
         }
    }

    if((NULL != psNciContext) && (NULL != (psNciContext->tTranscvCtxt.tSendPld.pBuff)))
    {
        (psNciContext->tTranscvCtxt.tSendPld.wLen) = 0;
        PH_LOG_NCI_INFO_STR(" Freeing Send Request Payload Buffer..");
        phOsalNfc_FreeMemory((psNciContext->tTranscvCtxt.tSendPld.pBuff));
        psNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
        (psNciContext->tTranscvCtxt.wPrevStatus) = wRespstatus;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wRespstatus;
}

NFCSTATUS
phNciNfc_SendJewelReq(
                        void   *psContext
                      )
{
    NFCSTATUS wReqstatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t   TxInfo;
    uint16_t                wPldHdrSize;
    uint16_t                wPldDataSize;
    pphNciNfc_RemoteDevInformation_t  pActivDev = NULL;

    uint8_t   bSendBuffIdx = 0;

    phNciNfc_Context_t *psNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciContext)
    {
        wReqstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if(NULL == (psNciContext->tActvDevIf.pDevInfo))
    {
        wReqstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
        PH_LOG_NCI_INFO_STR(" Invalid Device..");
    }
    else if(0 == (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen))
    {
        wReqstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
        PH_LOG_NCI_INFO_STR(" Send Data Buff not valid..");
    }
    else
    {
        pActivDev = (psNciContext->tActvDevIf.pDevInfo);
        PH_LOG_NCI_INFO_STR(" Creating Request Payload (Header + Data)..");
        wPldHdrSize = 0;

        /* obtain the size of data & header */
        wPldDataSize = (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen);
        /* Allocate Memory required for Include Request header in
           NCI packet payload */
        (psNciContext->tTranscvCtxt.tSendPld.wLen) = wPldHdrSize + wPldDataSize;
        (psNciContext->tTranscvCtxt.tSendPld.pBuff) = (uint8_t *)phOsalNfc_GetMemory(\
                                        psNciContext->tTranscvCtxt.tSendPld.wLen);

        if(NULL != (psNciContext->tTranscvCtxt.tSendPld.pBuff))
        {
            phOsalNfc_SetMemory((psNciContext->tTranscvCtxt.tSendPld.pBuff),0,
                (psNciContext->tTranscvCtxt.tSendPld.wLen));

            /* Fill the data bytes passed by upper module */
            phOsalNfc_MemCopy(&(psNciContext->tTranscvCtxt.tSendPld.pBuff[bSendBuffIdx]),
	        (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff),wPldDataSize);
            PH_LOG_NCI_INFO_STR(" Payload (Header + Data) created successfully..");
        }
        else
        {
            wReqstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" Payload MemAlloc for Send request Failed..");
        }
        if(NFCSTATUS_SUCCESS == wReqstatus)
        {
            /* Fill the data packet details into TxInfo */
            TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            wReqstatus = phNciNfc_GetConnId( pActivDev,\
                                             &(TxInfo.tHeaderInfo.bConn_ID));

            if(NFCSTATUS_SUCCESS == wReqstatus)
            {
                TxInfo.Buff = (psNciContext->tTranscvCtxt.tSendPld.pBuff);
                TxInfo.wLen = (psNciContext->tTranscvCtxt.tSendPld.wLen);
                /* Call Sequence handler to send data to lower layer */
                wReqstatus = phNciNfc_CoreIfTxRx(&(psNciContext->NciCoreContext), &TxInfo,
                    &(psNciContext->RspBuffInfo), psNciContext->tTranscvCtxt.tTranscvInfo.wTimeout,
                    (pphNciNfc_CoreIfNtf_t)&phNciNfc_RdrDataXchgSequence, psContext);
            }
            else
            {
                /* Release memory allocated to Construct payload */
                phOsalNfc_FreeMemory(psNciContext->tTranscvCtxt.tSendPld.pBuff);
                psNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
                wReqstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                PH_LOG_NCI_INFO_STR(" Couldn't Get ConnId..");
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wReqstatus;
}

NFCSTATUS phNciNfc_UpdateJewelInfo(void *pContext,\
                                 pphNciNfc_RemoteDevInformation_t pRemDev,\
                                 uint8_t *pBuff)
{
    NFCSTATUS wStatus;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)pContext;
    uint8_t bIndex = 0;
    if(NULL != pNciContext)
    {
        /* Store the RID response Type 1 tag info */
        pRemDev->tRemoteDevInfo.Jewel_Info.HeaderRom0 = \
                    pBuff[bIndex++];
        pRemDev->tRemoteDevInfo.Jewel_Info.HeaderRom1 = \
                    pBuff[bIndex++];
        pRemDev->tRemoteDevInfo.Jewel_Info.UidLength = \
                    PHNCINFC_UID_LEN;
        phOsalNfc_MemCopy(pRemDev->tRemoteDevInfo.Jewel_Info.Uid,\
                    &pBuff[bIndex],PHNCINFC_UID_LEN);
        wStatus = NFCSTATUS_SUCCESS;
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}
