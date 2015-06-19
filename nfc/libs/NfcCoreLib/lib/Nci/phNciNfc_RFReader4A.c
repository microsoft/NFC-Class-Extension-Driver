/*
* =============================================================================
*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*
* =============================================================================
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_RFReader4A.h"

#include "phNciNfc_RFReader4A.tmh"

NFCSTATUS
phNciNfc_Recv4AResp(
                        void                *psContext,
                        NFCSTATUS          wStatus
                       )
{
    NFCSTATUS                   Status = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t          *psNciContext = (phNciNfc_Context_t *)psContext;
    uint16_t                wRecvDataSz = 0;
    uint16_t CopyLen = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciContext)
    {
      Status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
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
            to the status code in Nci Context and use the same in the status below */
            Status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" Data Receive Failed..");
        }
        else
        {
            wRecvDataSz = (psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen);

            if((psNciContext->RspBuffInfo.wLen) > wRecvDataSz)
            {
                CopyLen = wRecvDataSz;
                Status = NFCSTATUS_SUCCESS; // Change to this if handling is added NFCSTATUS_MORE_INFORMATION;
            }else
            {
                CopyLen = psNciContext->RspBuffInfo.wLen;
                Status = NFCSTATUS_SUCCESS;
            }
            /* Extract the data part from pBuff[3] & fill it to be sent to upper layer */
            phOsalNfc_MemCopy((psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff),
                (psNciContext->RspBuffInfo.pBuff),CopyLen);
            psNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen = CopyLen;
        }
        (psNciContext->tTranscvCtxt.wPrevStatus) = Status;
    }


    if((NULL != psNciContext) && (NULL != (psNciContext->tTranscvCtxt.tSendPld.pBuff)))
    {
        (psNciContext->tTranscvCtxt.tSendPld.wLen) = 0;
        PH_LOG_NCI_INFO_STR(" Freeing Send Request Payload Buffer..");
        phOsalNfc_FreeMemory((psNciContext->tTranscvCtxt.tSendPld.pBuff));
        psNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
    }

    PH_LOG_NCI_FUNC_EXIT();

    return Status;
}

NFCSTATUS
phNciNfc_Send4AData(
                    void   *psContext
                    )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t   TxInfo;
    uint16_t                wPldDataSize = 0;

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
            /* Fill the data packet details into TxInfo */
            TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            status = phNciNfc_GetConnId(pActivDev, &(TxInfo.tHeaderInfo.bConn_ID));

            if(NFCSTATUS_SUCCESS == status)
            {
                wPldDataSize = (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen);

                (psNciContext->tTranscvCtxt.tSendPld.wLen) = 0;
                (psNciContext->tTranscvCtxt.tSendPld.pBuff) = (uint8_t *)phOsalNfc_GetMemory(wPldDataSize);

                if(NULL != (psNciContext->tTranscvCtxt.tSendPld.pBuff))
                {
                    (psNciContext->tTranscvCtxt.tSendPld.wLen) = (wPldDataSize);
                    phOsalNfc_SetMemory((psNciContext->tTranscvCtxt.tSendPld.pBuff),0,
                        (psNciContext->tTranscvCtxt.tSendPld.wLen));

                    if(0 != wPldDataSize)
                    {
                        phOsalNfc_MemCopy((psNciContext->tTranscvCtxt.tSendPld.pBuff),
                            (psNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff),wPldDataSize);
                    }
                    PH_LOG_NCI_INFO_STR(" 4A Payload created successfully..");

                    TxInfo.Buff = (psNciContext->tTranscvCtxt.tSendPld.pBuff);
                    TxInfo.wLen = (psNciContext->tTranscvCtxt.tSendPld.wLen);
                    status = phNciNfc_CoreIfTxRx(&(psNciContext->NciCoreContext), &TxInfo,
                        &(psNciContext->RspBuffInfo), psNciContext->tTranscvCtxt.tTranscvInfo.wTimeout,
                        (pphNciNfc_CoreIfNtf_t)&phNciNfc_RdrDataXchgSequence, psContext);
                    /* Clear the timeout value so that it wont be used mistakenly in subsequent transceive */
                    psNciContext->tTranscvCtxt.tTranscvInfo.wTimeout = 0;
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
                PH_LOG_NCI_INFO_STR(" Couldn't Get ConnId..");
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
