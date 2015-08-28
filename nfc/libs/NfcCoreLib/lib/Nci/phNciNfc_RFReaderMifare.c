/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_RFReaderMifare.h"
#include "phNciNfc_RFReaderMifare.tmh"

#define PHNCINFC_EXTNID_SIZE          (0x01U)     /**< Size of Mifare Extension Req/Rsp Id */
#define PHNCINFC_EXTNSTATUS_SIZE      (0x01U)     /**< Size of Mifare Extension Resp Status Byte */
#define PHNCINFC_MFCKEY_LEN            (0x06)     /**<.Mifare Classic key len*/
#define PHNCINFC_EMBEDKEY_PARAM_INDEX  (0x03)     /**< Index of the parameter of Mifare Auth Cmd*/
#define PHNCINFC_MFC_EMBEDDED_KEY      (0x10U)
#define PHNCINFC_NDEFKEY_LEN           (0x06)     /**<.Ndef key len*/
#define PHNCINFC_RAWKEY_LEN            (0x06)     /**<.Raw key len*/
#define PHNCINFC_MADKEY_LEN            (0x06)     /**<.Mad key len*/

static
NFCSTATUS
phNciNfc_MfCreateWriteHdr(
                        phNciNfc_Context_t   *psNciContext,
                        uint8_t              bBlockAddr
                         );

static
NFCSTATUS
phNciNfc_MfCreateReadHdr(
                        phNciNfc_Context_t  *psNciContext,
                        uint8_t             bBlockAddr,
                        uint8_t             bNumOfBlocks
                       );

static
NFCSTATUS
phNciNfc_MfCreateXchgDataHdr(
                        phNciNfc_Context_t  *psNciContext
                       );


static
NFCSTATUS
phNciNfc_MfCreateAuthCmdHdr(
                        phNciNfc_Context_t   *psNciContext,
                        uint8_t              bBlockAddr
                         );

static
NFCSTATUS
phNciNfc_MfCreateSectorSelCmdHdr(phNciNfc_Context_t     *psNciContext,\
                                 uint8_t    bBlockAddr);


static
NFCSTATUS
phNciNfc_MfCreateWriteHdr(
                        phNciNfc_Context_t     *psNciContext,
                        uint8_t    bBlockAddr
                         )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if(NULL == (psNciContext->tActvDevIf.pDevInfo))
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
        PH_LOG_NCI_INFO_STR(" Invalid Device..");
    }
    else
    {
        pphNciNfc_RemoteDevInformation_t  pActivDev = NULL;

        pActivDev = (psNciContext->tActvDevIf.pDevInfo);

        switch(pActivDev->RemDevType)
        {
            case phNciNfc_eMifareUL_PICC:
            {
                /* For MfUL, Validate BlockAddr to be in the range of 2 to 15 blocks */
                if(((0x02U) > bBlockAddr) || ((0x0FU) < bBlockAddr))
                {
                    status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
                    PH_LOG_NCI_INFO_STR(" Invalid Block Address ..");
                }
                break;
            }
            case phNciNfc_eMifare1k_PICC:
            {
                break;
            }
            default:
            {
                break;
            }
        }
        if(NFCSTATUS_SUCCESS == status)
        {
            phOsalNfc_SetMemory(&(psNciContext->tTranscvCtxt.tActiveExtn),0,
                sizeof(psNciContext->tTranscvCtxt.tActiveExtn));

            PH_LOG_NCI_INFO_STR(" Creating Write Request Header ..");
            /* Update ActivExtn member with write request header details */
            (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId) = phNciNfc_e_MfWriteNReq;
            (psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent) = 1;
            (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]) = bBlockAddr;
            (psNciContext->tTranscvCtxt.tActiveExtn.bParam[1]) = PH_NCINFC_EXTN_INVALID_PARAM_VAL;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS
phNciNfc_MfCreateReadHdr(
                        phNciNfc_Context_t     *psNciContext,
                        uint8_t             bBlockAddr,
                        uint8_t             bNumOfBlocks
                       )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if(NULL == (psNciContext->tActvDevIf.pDevInfo))
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
        PH_LOG_NCI_INFO_STR(" Invalid Device..");
    }
    else
    {
        pphNciNfc_RemoteDevInformation_t  pActivDev = NULL;

        pActivDev = (psNciContext->tActvDevIf.pDevInfo);

        switch(pActivDev->RemDevType)
        {
            case phNciNfc_eMifareUL_PICC:
            {
                /* For MfUL, Validate BlockAddr to be <=15 blocks */
                if((0x0FU) < bBlockAddr)
                {
                    status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
                    PH_LOG_NCI_INFO_STR(" Invalid Block Address ..");
                }
                break;
            }
            case phNciNfc_eMifare1k_PICC:
            {
                break;
            }
            default:
            {
                break;
            }
        }
        if(NFCSTATUS_SUCCESS == status)
        {
            phOsalNfc_SetMemory(&(psNciContext->tTranscvCtxt.tActiveExtn),0,sizeof(psNciContext->tTranscvCtxt.tActiveExtn));

            (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen) = 0x00;
            PH_LOG_NCI_INFO_STR(" Creating Read Request Header ..");
            /* Update ActivExtn member with read request header details */
            (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId) = phNciNfc_e_MfReadNReq;
            (psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent) = 2;
            (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]) = bBlockAddr;
            (psNciContext->tTranscvCtxt.tActiveExtn.bParam[1]) = bNumOfBlocks;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS
phNciNfc_MfCreateXchgDataHdr(
                        phNciNfc_Context_t     *psNciContext
                       )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if(NULL == (psNciContext->tActvDevIf.pDevInfo))
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
        PH_LOG_NCI_INFO_STR(" Invalid Device..");
    }
    else
    {
        pphNciNfc_RemoteDevInformation_t  pActivDev = NULL;

        pActivDev = (psNciContext->tActvDevIf.pDevInfo);

        phOsalNfc_SetMemory(&(psNciContext->tTranscvCtxt.tActiveExtn),0,
            sizeof(psNciContext->tTranscvCtxt.tActiveExtn));

        PH_LOG_NCI_INFO_STR(" Creating XchgData Request Header ..");
        /* Update ActivExtn member with XchgData request header details */
        psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId = phNciNfc_e_MfRawDataXchgHdr;
        psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent = 0;
        psNciContext->tTranscvCtxt.tActiveExtn.bParam[0] = PH_NCINFC_EXTN_INVALID_PARAM_VAL;
        psNciContext->tTranscvCtxt.tActiveExtn.bParam[1] = PH_NCINFC_EXTN_INVALID_PARAM_VAL;
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_SendMfReq(
                              void   *psContext
                         )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t   TxInfo;
    uint16_t                wPldHdrSize;
    uint16_t                wPldDataSize;
    uint8_t                 bSendBuffIdx = 0;

    phNciNfc_Context_t *psNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if(NULL == (psNciContext->tActvDevIf.pDevInfo))
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
        PH_LOG_NCI_INFO_STR(" Invalid Device..");
    }
    else
    {
        pphNciNfc_RemoteDevInformation_t  pActivDev = NULL;

        pActivDev = (psNciContext->tActvDevIf.pDevInfo);

        if(0 != (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen))
        {
            if(pActivDev->eRfIf == phNciNfc_e_RfInterfacesTagCmd_RF)
            {
                switch((psNciContext->tTranscvCtxt.tTranscvInfo.uCmd.T2TCmd))
                {
                    case phNciNfc_eT2TWriteN:
                    {
                        status = phNciNfc_MfCreateWriteHdr(psNciContext,(psNciContext->tTranscvCtxt.tTranscvInfo.bAddr));
                        break;
                    }
                    case phNciNfc_eT2TreadN:
                    {
                        status = phNciNfc_MfCreateReadHdr(psNciContext,(psNciContext->tTranscvCtxt.tTranscvInfo.bAddr),
                            (psNciContext->tTranscvCtxt.tTranscvInfo.bNumBlock));
                        break;
                    }
                    case phNciNfc_eT2TRaw:
                    {
                        status = phNciNfc_MfCreateXchgDataHdr(psNciContext);
                        break;
                    }
                    case phNciNfc_eT2TAuth:
                    {
                        status = phNciNfc_MfCreateAuthCmdHdr(psNciContext,
                            (psNciContext->tTranscvCtxt.tTranscvInfo.bAddr));
                    }
                    break;
                    case phNciNfc_eT2TSectorSel:
                    {
                        status = phNciNfc_MfCreateSectorSelCmdHdr(psNciContext,
                            (psNciContext->tTranscvCtxt.tTranscvInfo.bAddr));
                    }
                    break;
                    default:
                    {
                        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                        PH_LOG_NCI_INFO_STR(" Invalid Transceive Type received ..");
                        break;
                    }
                }
            }

            if(NFCSTATUS_SUCCESS == status)
            {
                PH_LOG_NCI_INFO_STR(" Creating Request Payload (Header + Data)..");
                /* obtain the size of data & header */
                if(pActivDev->eRfIf == phNciNfc_e_RfInterfacesTagCmd_RF)
                {
                    /* Check if Authenticate command with Dynamic key is passed */
                     if( (phNciNfc_eT2TAuth == psNciContext->tTranscvCtxt.tTranscvInfo.uCmd.T2TCmd) &&\
                         (PHNCINFC_EMBEDKEY_PARAM_INDEX == \
                                 psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent) )
                     {
                         wPldHdrSize = (PHNCINFC_EXTNID_SIZE +\
                                 (psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent - 1) +\
                                  PHNCINFC_MFCKEY_LEN);
                     }
                     else
                     {
                         wPldHdrSize = (PHNCINFC_EXTNID_SIZE +
                             (psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent));
                     }
                }
                else
                {
                    wPldHdrSize = 0x00;
                }

                wPldDataSize = (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen);

                (psNciContext->tTranscvCtxt.tSendPld.wLen) = 0;
                (psNciContext->tTranscvCtxt.tSendPld.pBuff) = (uint8_t *)phOsalNfc_GetMemory
                    (wPldHdrSize + wPldDataSize);

                if(NULL != (psNciContext->tTranscvCtxt.tSendPld.pBuff))
                {
                    (psNciContext->tTranscvCtxt.tSendPld.wLen) = (wPldHdrSize + wPldDataSize);
                    phOsalNfc_SetMemory((psNciContext->tTranscvCtxt.tSendPld.pBuff),0,
                        (psNciContext->tTranscvCtxt.tSendPld.wLen));

                    if(pActivDev->eRfIf == phNciNfc_e_RfInterfacesTagCmd_RF)
                    {
                        (psNciContext->tTranscvCtxt.tSendPld.pBuff[bSendBuffIdx++]) = psNciContext->tTranscvCtxt.
                            tActiveExtn.ActivExtnId.ExtnReqId;

                        if(0 != (psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent))
                        {
                            if( (phNciNfc_eT2TAuth == psNciContext->tTranscvCtxt.tTranscvInfo.uCmd.T2TCmd) &&\
                                                    (PHNCINFC_EMBEDKEY_PARAM_INDEX == \
                                                    psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent) )
                            {
                                phOsalNfc_MemCopy(&(psNciContext->tTranscvCtxt.tSendPld.pBuff[bSendBuffIdx]),
                                                  &(psNciContext->tTranscvCtxt.tActiveExtn.bParam),
                                                  ((psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent - 1)+\
                                 PHNCINFC_MFCKEY_LEN));
                            }
                            else
                            {
                                phOsalNfc_MemCopy(&(psNciContext->tTranscvCtxt.tSendPld.pBuff[bSendBuffIdx]),
                                                  &(psNciContext->tTranscvCtxt.tActiveExtn.bParam),
                                                  (psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent));
                            }
                        }
                    }

                    if(0 != wPldDataSize)
                    {
                        phOsalNfc_MemCopy(&(psNciContext->tTranscvCtxt.tSendPld.pBuff[wPldHdrSize]),
                            (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff),wPldDataSize);
                    }
                    PH_LOG_NCI_INFO_STR(" Payload (Header + Data) created successfully..");
                }
                else
                {
                    status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INSUFFICIENT_RESOURCES);
                    PH_LOG_NCI_INFO_STR(" Payload MemAlloc for Send request Failed..");
                }
            }
            else
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                PH_LOG_NCI_INFO_STR(" Request Payload Header Creation Failed ..");
            }

            if(NFCSTATUS_SUCCESS == status)
            {
                /* Fill the data packet details into TxInfo */
                TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
                status = phNciNfc_GetConnId(pActivDev,&(TxInfo.tHeaderInfo.bConn_ID));

                if(NFCSTATUS_SUCCESS == status)
                {
                    TxInfo.Buff = (psNciContext->tTranscvCtxt.tSendPld.pBuff);
                    TxInfo.wLen = (psNciContext->tTranscvCtxt.tSendPld.wLen);
                    /* Call Sequence handler to send data to lower layer */
                    status = phNciNfc_CoreIfTxRx(&(psNciContext->NciCoreContext), &TxInfo,
                        &(psNciContext->RspBuffInfo), psNciContext->tTranscvCtxt.tTranscvInfo.wTimeout,
                        (pphNciNfc_CoreIfNtf_t)&phNciNfc_RdrDataXchgSequence, psContext);
                    /* Clear the timeout value so that it wont be used mistakenly in subsequent transceive */
                    psNciContext->tTranscvCtxt.tTranscvInfo.wTimeout = 0;
                }
                else
                {
                    status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                    PH_LOG_NCI_INFO_STR(" Couldn't Get ConnId..");
                }
            }
            else
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                PH_LOG_NCI_INFO_STR(" Extension Payload Packet creation Failed..");
            }
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" Send Data Buff not valid..");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_RecvMfResp(
                        void                *psContext,
                        NFCSTATUS          wStatus
                       )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t          *psNciContext = (phNciNfc_Context_t *)psContext;
    uint16_t                wPldDataSize = 0;
    uint16_t                wRecvDataSz = 0;
    pphNciNfc_RemoteDevInformation_t  pActivDev = NULL;
    phNciNfc_ExtnRespId_t RecvdExtnRspId = phNciNfc_e_InvalidRsp;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == psNciContext)
    {
      status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
      PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if((0 == (psNciContext->RspBuffInfo.wLen))
                || (PH_NCINFC_STATUS_OK != wStatus)
                || (NULL == (psNciContext->RspBuffInfo.pBuff))
                )
        {
            /* NOTE:- ( TODO) In this fail scenario,map the status code from response handler
            to the status code in Nci Context and use the same in status below */
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" Mf Data Receive Failed..");
        }
        else
        {
            pActivDev = (psNciContext->tActvDevIf.pDevInfo);

            if((NULL != pActivDev) && (pActivDev->eRfIf == phNciNfc_e_RfInterfacesTagCmd_RF))
            {
                RecvdExtnRspId = (phNciNfc_ExtnRespId_t)psNciContext->RspBuffInfo.pBuff[0];

                switch(RecvdExtnRspId)
                {
                    case phNciNfc_e_MfWriteNRsp:
                    {
                        /* verify if the last request was indeed a WriteNReq */
                        if(phNciNfc_e_MfWriteNReq == (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId))
                        {
                            (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnRspId) = RecvdExtnRspId;
                            (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]) = (psNciContext->RspBuffInfo.pBuff[1]);

                            /* check the status byte */
                            if(PH_NCINFC_STATUS_OK == (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]))
                            {
                                status = NFCSTATUS_SUCCESS;
                                PH_LOG_NCI_INFO_STR(" Mf WriteN Request is Successful!! ..");
                            }
                            else
                            {
                                /* TODO:- use other relevant NFCSTATUS below when mapping is ready */
                                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                                PH_LOG_NCI_INFO_STR(" Mf WriteN Request Failed ..");
                            }
                        }
                        else
                        {
                            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                            PH_LOG_NCI_INFO_STR(" Unexpected Mf Write Response Received ..");
                        }
                        break;
                    }
                    case phNciNfc_e_MfReadNRsp:
                    {
                        /* verify if the last request was indeed a ReadNReq */
                        if(phNciNfc_e_MfReadNReq == (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId))
                        {
                            (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnRspId) = RecvdExtnRspId;
                            (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]) = (psNciContext->RspBuffInfo.pBuff[1]);

                            /* check the status byte */
                            if(PH_NCINFC_STATUS_OK == (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]))
                            {
                                status = NFCSTATUS_SUCCESS;
                                PH_LOG_NCI_INFO_STR(" Mf ReadN Request is Successful!! ..");

                                /* DataLen = TotalRecvdLen - (sizeof(RspId) + sizeof(Status)) */
                                wPldDataSize = ((psNciContext->RspBuffInfo.wLen) -
                                    (PHNCINFC_EXTNID_SIZE + PHNCINFC_EXTNSTATUS_SIZE));

                                wRecvDataSz = (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen);

                                if((wPldDataSize) <= wRecvDataSz)
                                {
                                    /* Extract the data part from pBuff[2] & fill it to be sent to upper layer */
                                    phOsalNfc_MemCopy((psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff),
                                        &(psNciContext->RspBuffInfo.pBuff[2]),wRecvDataSz);
                                }
                                else
                                {
                                    //Todo:- Map some status for remaining extra data received to be sent back to caller??
                                }
                            }
                            else
                            {
                                /* TODO:- use other relevant NFCSTATUS below when mapping is ready */
                                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                                PH_LOG_NCI_INFO_STR(" Mf ReadN Request Failed ..");
                            }
                        }
                        else
                        {
                            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                            PH_LOG_NCI_INFO_STR(" Unexpected Mf Read Response Received ..");
                        }
                        break;
                    }
                    case phNciNfc_e_MfXchgDataRsp:
                    {
                        /* verify if the last request was indeed a XchgDataReq */
                        if(phNciNfc_e_MfRawDataXchgHdr == (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId))
                        {
                            (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnRspId) = RecvdExtnRspId;
                            (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]) = psNciContext->RspBuffInfo.pBuff[psNciContext->RspBuffInfo.wLen-1];//(psNciContext->RspBuffInfo.pBuff[1]);

                            /* check the status byte */
                            if(PH_NCINFC_STATUS_OK == (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]))
                            {
                                status = NFCSTATUS_SUCCESS;
                                PH_LOG_NCI_INFO_STR(" Mf XchgData Request is Successful!! ..");

                                /* DataLen = TotalRecvdLen - (sizeof(RspId) + sizeof(Status)) */
                                wPldDataSize = ((psNciContext->RspBuffInfo.wLen) -
                                    (PHNCINFC_EXTNID_SIZE + PHNCINFC_EXTNSTATUS_SIZE));
                                wRecvDataSz = (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen);

                                /* wPldDataSize = wPldDataSize-1; ==> ignoring the last status byte appended with data */
                                if((wPldDataSize) <= wRecvDataSz)
                                {
                                    /* Extract the data part from pBuff[2] & fill it to be sent to upper layer */
                                    phOsalNfc_MemCopy((psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff),
                                        &(psNciContext->RspBuffInfo.pBuff[1]),(wPldDataSize ));
                                    /* update the number of bytes received from lower layer,excluding the status byte */
                                    (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen) = (wPldDataSize);
                                }
                                else
                                {
                                    /*Todo:- Map some status for remaining extra data received to be sent back to caller??*/
                                    status = PHNFCSTVAL(CID_NFC_NCI,NFCSTATUS_MORE_INFORMATION);
                                    PH_LOG_NCI_INFO_STR(" Mf XchgData,More Data available than requested  ..");
                                }
                            }
                            else if((PH_NCINFC_STATUS_RF_TIMEOUT_ERROR == (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0])) &&
                                (pActivDev->eRFProtocol == phNciNfc_e_RfProtocolsMifCProtocol))
                            {
                                /* TODO:- use other relevant NFCSTATUS below when mapping is ready */
                                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_RF_TIMEOUT_ERROR);
                                PH_LOG_NCI_INFO_STR(" Mf XchgData Request Failed RF Time out..");
                            }
                            else
                            {
                                /* TODO:- use other relevant NFCSTATUS below when mapping is ready */
                                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                                PH_LOG_NCI_INFO_STR(" Mf XchgData Request Failed ..");
                            }
                        }
                        else
                        {
                            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                            PH_LOG_NCI_INFO_STR(" Unexpected Mf XchgData Response Received ..");
                        }
                        break;
                    }
                    case phNciNfc_e_MfcAuthRsp:
                    {
                        /* verify if the last request was indeed a WriteNReq */
                        if(phNciNfc_e_MfcAuthReq == (phNciNfc_ExtnRespId_t)(psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId))
                        {
                            (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnRspId) = RecvdExtnRspId;
                            (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]) = (psNciContext->RspBuffInfo.pBuff[1]);

                            /* check the status byte */
                            if(PH_NCINFC_STATUS_OK == (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]))
                            {
                                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_SUCCESS);
                                PH_LOG_NCI_INFO_STR(" Mf Auth Pass ..");
                            }
                            else
                            {
                                /* TODO:- use other relevant NFCSTATUS below when mapping is ready */
                                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                                PH_LOG_NCI_INFO_STR(" Mf Auth Fail ..");
                            }
                        }
                        else
                        {
                            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                            PH_LOG_NCI_INFO_STR(" Unexpected Mf Auth Response Received ..");
                        }
                    }
                    break;
                    case phNciNfc_e_MfSectorSelRsp:
                    {
                        /* verify if the last request was indeed a WriteNReq */
                        if(phNciNfc_e_MfSectorSelReq  == (phNciNfc_ExtnRespId_t)(psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId))
                        {
                            (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnRspId) = RecvdExtnRspId;
                            (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]) = (psNciContext->RspBuffInfo.pBuff[1]);

                            /* check the status byte */
                            if(PH_NCINFC_STATUS_OK == (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]))
                            {
                                status = NFCSTATUS_SUCCESS;
                                PH_LOG_NCI_INFO_STR(" Mf Sector select Request is Successful!! ..");

                                /* DataLen = TotalRecvdLen - (sizeof(RspId) + sizeof(Status)) */
                                wPldDataSize = ((psNciContext->RspBuffInfo.wLen) -
                                    (PHNCINFC_EXTNID_SIZE + PHNCINFC_EXTNSTATUS_SIZE));
                                wRecvDataSz = (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen);

                                /* wPldDataSize = wPldDataSize-1; ==> ignoring the last status byte appended with data */
                                if(wPldDataSize <= wRecvDataSz)
                                {
                                    /* Extract the data part from pBuff[2] & fill it to be sent to upper layer */
                                    phOsalNfc_MemCopy((psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff),
                                        &(psNciContext->RspBuffInfo.pBuff[2]),wPldDataSize);
                                    /* update the number of bytes received from lower layer,excluding the status byte */
                                    (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen) = wPldDataSize;
                                }
                                else
                                {
                                    /*wPldDataSize is zero or extra data received*/
                                    //Todo:- Map some status for remaining extra data received to be sent back to caller??
                                    status = PHNFCSTVAL(CID_NFC_NCI,NFCSTATUS_MORE_INFORMATION);
                                    PH_LOG_NCI_INFO_STR(" Mf XchgData,More Data available than requested  ..");
                                }
                            }
                            else
                            {
                                /* TODO:- use other relevant NFCSTATUS below when mapping is ready */
                                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                                PH_LOG_NCI_INFO_STR(" Mf WriteN Request Failed ..");
                            }
                        }
                        else
                        {
                            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                            PH_LOG_NCI_INFO_STR(" Unexpected Mf Write Response Received ..");
                        }
                    }
                    break;
                    default:
                    {
                        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                        PH_LOG_NCI_INFO_STR(" Unknown RespId Received ..");
                        break;
                    }
                }
            }
            else
            {
                 wPldDataSize = (psNciContext->RspBuffInfo.wLen);
                 wRecvDataSz = (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen);

                 /* check the status byte */
                 if(PH_NCINFC_STATUS_OK == (psNciContext->RspBuffInfo.pBuff[wPldDataSize-1]))
                 {
                     status = NFCSTATUS_SUCCESS;
                     PH_LOG_NCI_INFO_STR(" Mf XchgData Request is Successful!! ..");

                     /* wPldDataSize = wPldDataSize - 1; // ignoring the last status byte appended with data */
                     if((wPldDataSize - 1) <= wRecvDataSz)
                     {
                         /* Extract the data part from pBuff[0] & fill it to be sent to upper layer */
                         phOsalNfc_MemCopy((psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff),
                             &(psNciContext->RspBuffInfo.pBuff[0]),(wPldDataSize - 1));
                         /* update the number of bytes received from lower layer,excluding the status byte */
                         (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen) = (wPldDataSize - 1);
                     }
                     else
                     {
                         /* Todo:- Map some status for remaining extra data received to be sent back to caller?? */
                        status = PHNFCSTVAL(CID_NFC_NCI,NFCSTATUS_MORE_INFORMATION);
                        PH_LOG_NCI_INFO_STR(" Mf XchgData,More Data available than requested  ..");
                     }
                 }
                 else
                 {
                     status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                     PH_LOG_NCI_INFO_STR(" Mf XchgData Failed ..");
                 }
            }
        }
        (psNciContext->tTranscvCtxt.wPrevStatus) = status;
    }

    if((NULL != psNciContext) && (NULL != (psNciContext->tTranscvCtxt.tSendPld.pBuff)))
    {
        (psNciContext->tTranscvCtxt.tSendPld.wLen) = 0;
        PH_LOG_NCI_INFO_STR(" Freeing Send Request Payload Buffer..");
        phOsalNfc_FreeMemory((psNciContext->tTranscvCtxt.tSendPld.pBuff));
        psNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
    }

    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS
phNciNfc_MfCreateAuthCmdHdr(phNciNfc_Context_t     *psNciContext,
                            uint8_t    bBlockAddr)
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    pphNciNfc_RemoteDevInformation_t  pActivDev = NULL;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if(NULL == (psNciContext->tActvDevIf.pDevInfo))
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
        PH_LOG_NCI_INFO_STR(" Invalid Device..");
    }
    else if (3 > psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen)
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid parameter..");
    }
    else
    {
        pActivDev = (psNciContext->tActvDevIf.pDevInfo);

        /*For authentication extension no need to copy tSendData buffer of tTranscvInfo */
        psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen = 0x00;

        phOsalNfc_SetMemory(&(psNciContext->tTranscvCtxt.tActiveExtn),0,
            sizeof(psNciContext->tTranscvCtxt.tActiveExtn));

        PH_LOG_NCI_INFO_STR(" Creating Write Request Header ..");
        /* Update ActivExtn member with write request header details */
        (psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId) = phNciNfc_e_MfcAuthReq;
        (psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent) = 2;
        (psNciContext->tTranscvCtxt.tActiveExtn.bParam[0]) = bBlockAddr;
        (psNciContext->tTranscvCtxt.tActiveExtn.bParam[1]) = 
                            psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff[2];

        /* Check if Dynamic Key is passed */
        if( PHNCINFC_MFC_EMBEDDED_KEY == (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff[2] & PHNCINFC_MFC_EMBEDDED_KEY))
        {
            psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent++;
            phOsalNfc_MemCopy(&psNciContext->tTranscvCtxt.tActiveExtn.bParam[2],\
                             (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff + 3),\
                             PHNCINFC_MFCKEY_LEN);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS
phNciNfc_MfCreateSectorSelCmdHdr(phNciNfc_Context_t     *psNciContext,\
                                 uint8_t    bBlockAddr)
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    pphNciNfc_RemoteDevInformation_t  pActivDev = NULL;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else if(NULL == (psNciContext->tActvDevIf.pDevInfo))
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
        PH_LOG_NCI_INFO_STR(" Invalid Device..");
    }
    else
    {
        pActivDev = (psNciContext->tActvDevIf.pDevInfo);

        phOsalNfc_SetMemory(&(psNciContext->tTranscvCtxt.tActiveExtn),0,
            sizeof(psNciContext->tTranscvCtxt.tActiveExtn));

        psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen = 0x00;

        PH_LOG_NCI_INFO_STR(" Creating Write Request Header ..");
        /* Update ActivExtn member with write request header details */
        psNciContext->tTranscvCtxt.tActiveExtn.ActivExtnId.ExtnReqId = phNciNfc_e_MfSectorSelReq;
        psNciContext->tTranscvCtxt.tActiveExtn.bParamsNumsPresent = 1;
        psNciContext->tTranscvCtxt.tActiveExtn.bParam[0] = bBlockAddr;
        psNciContext->tTranscvCtxt.tActiveExtn.bParam[1] = PH_NCINFC_EXTN_INVALID_PARAM_VAL;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return status;
}
