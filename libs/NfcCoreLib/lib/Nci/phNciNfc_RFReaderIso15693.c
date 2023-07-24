/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_RFReaderIso15693.h"

#include "phNciNfc_RFReaderIso15693.tmh"

NFCSTATUS
phNciNfc_RdrIso15693Init(pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                        uint8_t *pBuff,
                        uint16_t wLen)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    uint8_t RfTechSpecParamsLen = 0;
    uint8_t ActvnParamsLen = 0;
    uint8_t bActvParamOffset = 0;
    PH_LOG_NCI_FUNC_ENTRY();

    if((0 != (wLen)) && (NULL != pBuff) && (NULL != pRemDevInf))
    {
        PH_LOG_NCI_INFO_STR("Updating NFC-I (ISO15693) passive poll Mode Techn Specific info");

        (pRemDevInf->RemDevType) = phNciNfc_eISO15693_PICC;

        /* Length of technology specific parameters */
        RfTechSpecParamsLen = pBuff[6];
        if(0 == RfTechSpecParamsLen)
        {
            PH_LOG_NCI_WARN_STR("Tech Spec parameters not available for the discovered ISO15693 Tag");
        }

        bActvParamOffset = (7+RfTechSpecParamsLen+PH_NCINFCTYPES_DATA_XCHG_PARAMS_LEN);

        /* Length of Activation parameters */
        ActvnParamsLen = pBuff[bActvParamOffset];
        if(0 == ActvnParamsLen)
        {
            PH_LOG_NCI_WARN_STR("Activation parameters not available for the discovered ISO15693 Tag");
        }

        if(NFCSTATUS_SUCCESS == status)
        {
            gpphNciNfc_RdrDataXchgSequence[0].SequnceInitiate = &phNciNfc_Iso15693Send;
            gpphNciNfc_RdrDataXchgSequence[0].SequenceProcess = &phNciNfc_Iso15693Receive;
            gpphNciNfc_RdrDataXchgSequence[1].SequnceInitiate = NULL;
            gpphNciNfc_RdrDataXchgSequence[1].SequenceProcess = &phNciNfc_CompleteDataXchgSequence;
        }
    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_CRIT_STR("Invalid input parameters!");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return status;
}

NFCSTATUS
phNciNfc_Iso15693Receive(void *psContext,NFCSTATUS wStatus)
{
    NFCSTATUS          wRespstatus = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)psContext;
    uint16_t           wPldDataSize = 0;
    uint16_t           wRecvDataSz  = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pNciContext)
    {
        wRespstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_WARN_STR(" Invalid Context Param..");
    }
    else if((0 == (pNciContext->RspBuffInfo.wLen))\
            || (PH_NCINFC_STATUS_OK != wStatus)
            || (NULL == (pNciContext->RspBuffInfo.pBuff)))
    {
        /* NOTE:- ( TODO) In this fail scenario,map the status code from response handler
        to the status code in Nci Context and use the same in the status below */
        wRespstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
        PH_LOG_NCI_WARN_STR("ISO15693 XchgData receive Failed...");
    }
    else
    {
        wPldDataSize = (pNciContext->RspBuffInfo.wLen);
        wRecvDataSz = (pNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen);

        /* Check the status byte */
        if(PH_NCINFC_STATUS_OK == (pNciContext->RspBuffInfo.pBuff[wPldDataSize-1]))
        {
            wRespstatus = NFCSTATUS_SUCCESS;
            PH_LOG_NCI_INFO_STR("ISO15693 XchgData Request is Successful!");

            if((wPldDataSize - 1) <= wRecvDataSz)
            {
                /* Extract the data part from pBuff[0] & fill it to be sent to upper layer */
                phOsalNfc_MemCopy((pNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.pBuff),
                            &(pNciContext->RspBuffInfo.pBuff[0]),(wPldDataSize - 1));
                /* update the number of bytes received from lower layer,excluding the status byte */
                pNciContext->tTranscvCtxt.tTranscvInfo.tRecvData.wLen = wPldDataSize - 1;
            }
            else
            {
                /* Todo:- Map some status for remaining extra data received to be sent back to caller?? */
                wRespstatus = PHNFCSTVAL(CID_NFC_NCI,NFCSTATUS_MORE_INFORMATION);
                PH_LOG_NCI_WARN_STR("ISO15693 XchgData,More Data available than requested...");
            }
        }
        else
        {
            wRespstatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_WARN_STR("ISO15693 XchgData Failed ..");
        }
    }

    if((NULL != pNciContext) && (NULL != (pNciContext->tTranscvCtxt.tSendPld.pBuff)))
    {
        pNciContext->tTranscvCtxt.tSendPld.wLen = 0;
        PH_LOG_NCI_INFO_STR(" Freeing Send Request Payload Buffer..");
        phOsalNfc_FreeMemory(pNciContext->tTranscvCtxt.tSendPld.pBuff);
        pNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wRespstatus;
}

NFCSTATUS
phNciNfc_Iso15693Send(void *psContext)
{
    NFCSTATUS               wStat = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreTxInfo_t   TxInfo;
    uint16_t                wPldDataSize = 0;
    pphNciNfc_ActiveDeviceInfo_t  pActivDev = NULL;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == pNciContext))
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameter (Nci Context)!");
    }
    else
    {
        pActivDev = (pphNciNfc_ActiveDeviceInfo_t)&pNciContext->tActvDevIf;

        if((0 != pNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen) &&
           (NULL != pNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff))
        {
            /* Fill the data packet details into TxInfo */
            TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            wStat = phNciNfc_GetConnId((void *)pActivDev->pDevInfo, &(TxInfo.tHeaderInfo.bConn_ID));

            if(NFCSTATUS_SUCCESS == wStat)
            {
                wPldDataSize = pNciContext->tTranscvCtxt.tTranscvInfo.tSendData.wLen;

                /* Allocate memory for the send buffer */
                pNciContext->tTranscvCtxt.tSendPld.pBuff = (uint8_t *)phOsalNfc_GetMemory(wPldDataSize);
                pNciContext->tTranscvCtxt.tSendPld.wLen = 0;

                if(NULL != (pNciContext->tTranscvCtxt.tSendPld.pBuff))
                {
                    (pNciContext->tTranscvCtxt.tSendPld.wLen) = wPldDataSize;
                    phOsalNfc_SetMemory((pNciContext->tTranscvCtxt.tSendPld.pBuff),0,
                            (pNciContext->tTranscvCtxt.tSendPld.wLen));

                    /* Copy the input data to the send buffer */
                    phOsalNfc_MemCopy((pNciContext->tTranscvCtxt.tSendPld.pBuff),
                        (pNciContext->tTranscvCtxt.tTranscvInfo.tSendData.pBuff),wPldDataSize);

                    TxInfo.Buff = (pNciContext->tTranscvCtxt.tSendPld.pBuff);
                    TxInfo.wLen = (pNciContext->tTranscvCtxt.tSendPld.wLen);

                    /* Send data to remote device (P2P Target) */
                    wStat = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext), &TxInfo,
                        &(pNciContext->RspBuffInfo), pNciContext->tTranscvCtxt.tTranscvInfo.wTimeout,
                                (pphNciNfc_CoreIfNtf_t)&phNciNfc_RdrDataXchgSequence, pNciContext);
                    if(NFCSTATUS_PENDING != wStat)
                    {
                        phOsalNfc_FreeMemory(pNciContext->tTranscvCtxt.tSendPld.pBuff);
                        pNciContext->tTranscvCtxt.tSendPld.pBuff = NULL;
                        pNciContext->tTranscvCtxt.tSendPld.wLen = 0;
                        wStat = NFCSTATUS_FAILED;
                    }

                    /* Clear the timeout value so that it wont be used mistakenly in subsequent transceive */
                    pNciContext->tTranscvCtxt.tTranscvInfo.wTimeout = 0;
                    /* No need to register for any data message as we are using Sequence handler.
                       After sendig data to remote connected device, when a response message is
                       received, the sequence handler shall invoke 'phNciNfc_Iso15693Receive'
                       function */
                }
                else
                {
                    wStat = NFCSTATUS_INSUFFICIENT_RESOURCES;
                    PH_LOG_NCI_CRIT_STR("Payload MemAlloc for Send request Failed!");
                }
            }
            else
            {
                wStat = NFCSTATUS_FAILED;
                PH_LOG_NCI_CRIT_STR(" Couldn't Get ConnId!");
            }
        }
        else
        {
            wStat = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("Send Data Buff not valid!");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStat;
}

NFCSTATUS phNciNfc_Update15693SysInfo(void *pContext,\
                                      pphNciNfc_RemoteDevInformation_t pRemDev,\
                                      uint8_t *pBuff)
{
    NFCSTATUS wStatus;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)pContext;
    uint8_t bOffset = 1;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciContext) && (NULL != pBuff))
    {
        pRemDev->tRemoteDevInfo.Iso15693_Info.Flags = pBuff[bOffset++];

        pRemDev->tRemoteDevInfo.Iso15693_Info.UidLength = PH_NCINFCTYPES_15693_UID_LENGTH;
        phOsalNfc_MemCopy(&pRemDev->tRemoteDevInfo.Iso15693_Info.Uid,
                          &pBuff[bOffset],PH_NCINFCTYPES_15693_UID_LENGTH);

        bOffset += PH_NCINFCTYPES_15693_UID_LENGTH;

        /* Check is DSFID field is present */
        if(0x01 == (pRemDev->tRemoteDevInfo.Iso15693_Info.Flags & 0x01))
        {
            PH_LOG_NCI_INFO_STR("DSFID supported");
            pRemDev->tRemoteDevInfo.Iso15693_Info.Dsfid = pBuff[bOffset++];
        }
        else
        {
            PH_LOG_NCI_INFO_STR("DSFID not supported");
        }

        /* Check if AFI is supported/present */
        if(0x02 == (pRemDev->tRemoteDevInfo.Iso15693_Info.Flags & 0x02))
        {
            PH_LOG_NCI_INFO_STR("AFI supported");
            pRemDev->tRemoteDevInfo.Iso15693_Info.Afi = pBuff[bOffset++];
        }
        else
        {
            PH_LOG_NCI_INFO_STR("AFI not supported");
        }
        wStatus = NFCSTATUS_SUCCESS;
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_Update15693InventoryInfo(void *pContext,\
                                            pphNciNfc_RemoteDevInformation_t pRemDev,\
                                            uint8_t *pBuff)
{
    NFCSTATUS wStatus;
    phNciNfc_Context_t *pNciContext = (phNciNfc_Context_t *)pContext;
    uint8_t bOffset = 1;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciContext) && (NULL != pBuff))
    {
        pRemDev->tRemoteDevInfo.Iso15693_Info.Dsfid = pBuff[bOffset++];

        pRemDev->tRemoteDevInfo.Iso15693_Info.UidLength = PH_NCINFCTYPES_15693_UID_LENGTH;
        phOsalNfc_MemCopy(&pRemDev->tRemoteDevInfo.Iso15693_Info.Uid,
                          &pBuff[bOffset],PH_NCINFCTYPES_15693_UID_LENGTH);

        bOffset += PH_NCINFCTYPES_15693_UID_LENGTH;

        wStatus = NFCSTATUS_SUCCESS;
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
