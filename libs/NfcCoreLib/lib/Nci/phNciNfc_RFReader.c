/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_RFReaderA.h"
#include "phNciNfc_RFReaderB.h"
#include "phNciNfc_RFReaderIso15693.h"

#include "phNciNfc_RFReader.tmh"

phNciNfc_SequenceP_t *gpphNciNfc_RdrDataXchgSequence = NULL;   /**< Updating seq hdlr with ReaderTech DataXchg seq */

#define SEL_RESP_CONFIG_MASK        (0x64U)     /**< Mask to obtain the target type from SEL_RES byte */

#define MIFARE_UL_SAK               (0x00U)     /**< SAK value for Mifare UL */
#define MIFARE_1K_SAK               (0x08U)     /**< SAK value for Mifare 1k */
#define MIFARE_4K_SAK               (0x18U)     /**< SAK value for Mifare 4k */

#define INTF_ACTVD_MIN_PLD_LEN      (0x0BU)     /**< Minimum length of interface activated ntf pld*/


static
NFCSTATUS
phNciNfc_UpdateRemDevInfo(pphNciNfc_RemoteDevInformation_t pRemDevInf,
                          uint8_t *pBuff,
                          uint16_t wLen
                          );


NFCSTATUS
phNciNfc_RdrMgmtInit(
                                void *psContext,
                                pphNciNfc_RemoteDevInformation_t pRemDevInf,
                                uint8_t *pBuff,
                                uint16_t wLen
                         )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t psNciCtxt = (pphNciNfc_Context_t)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL == psNciCtxt) || (NULL == pBuff) || (NULL == pRemDevInf))
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Param(s)..");
    }
    else
    {
        if(INTF_ACTVD_MIN_PLD_LEN > wLen)
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
            PH_LOG_NCI_INFO_STR(" Intf Actvd Payload Incomplete..");
        }
        else
        {
            if(NULL == gpphNciNfc_RdrDataXchgSequence)
            {
                gpphNciNfc_RdrDataXchgSequence = (phNciNfc_SequenceP_t *)phOsalNfc_GetMemory(
                (2 * sizeof(phNciNfc_SequenceP_t)));
            }

            if(NULL != gpphNciNfc_RdrDataXchgSequence)
            {
                /* Extract info into RemDevInf structure */
                status = phNciNfc_UpdateRemDevInfo(pRemDevInf,pBuff,wLen);

                if(NFCSTATUS_SUCCESS == status)
                {
                    (psNciCtxt->tActvDevIf.pDevInfo) = pRemDevInf;

                    status = phNciNfc_SetConnCredentials(psNciCtxt);
                }
            }
            else
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INSUFFICIENT_RESOURCES);
                PH_LOG_NCI_INFO_STR(" DataXchg SequenceHandler pointer MemAlloc Failed..");
            }
        }
    }

    if(NFCSTATUS_SUCCESS != status)
    {
        if(NULL != gpphNciNfc_RdrDataXchgSequence)
        {
            PH_LOG_NCI_INFO_STR(" Freeing RdrDataXchgSeq Mem..");
            phOsalNfc_FreeMemory(gpphNciNfc_RdrDataXchgSequence);
            gpphNciNfc_RdrDataXchgSequence = NULL;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_RdrMgmtXchgData(
                              void     *psContext,
                              void     *pDevHandle,
                              phNciNfc_TransceiveInfo_t *pTranscvIf,
                              pphNciNfc_TransreceiveCallback_t pNotify,
                              void *pContext
                         )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t psNciCtxt = (pphNciNfc_Context_t)psContext;
    pphNciNfc_RemoteDevInformation_t  pActivDev = pDevHandle;

    PH_LOG_NCI_FUNC_ENTRY();
    if( (NULL == psNciCtxt) )
    {
      status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
      PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if(NULL == pDevHandle)
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
            PH_LOG_NCI_INFO_STR(" Invalid Device Handle Param..");
        }
        else if( (NULL == (pTranscvIf->tSendData.pBuff)) ||
            (0 == (pTranscvIf->tSendData.wLen))
            )
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
            PH_LOG_NCI_INFO_STR(" Invalid Send Buff Params..");
        }
        else if((((NULL == (pTranscvIf->tRecvData.pBuff)) ||
            (0 == (pTranscvIf->tRecvData.wLen))) &&
            (phNciNfc_eT2TAuth != pTranscvIf->uCmd.T2TCmd ))
            )
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
            PH_LOG_NCI_INFO_STR(" Invalid Recv Buff Params..");
        }
        else if((NULL == pNotify) || (NULL == pContext))
        {
            status = PHNFCSTVAL(CID_NFC_NCI,NFCSTATUS_INVALID_PARAMETER);
            PH_LOG_NCI_INFO_STR("Invalid Upper layer inputs..");
        }
        else
        {
            if((pActivDev != psNciCtxt->tActvDevIf.pDevInfo))
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_DEVICE);
                PH_LOG_NCI_INFO_STR(" Device Handle not Valid..");
            }
            else
            {
                if(NULL != gpphNciNfc_RdrDataXchgSequence)
                {
                    psNciCtxt->tTranscvCtxt.tTranscvInfo.uCmd = pTranscvIf->uCmd;
                    psNciCtxt->tTranscvCtxt.tTranscvInfo.bAddr = pTranscvIf->bAddr;
                    psNciCtxt->tTranscvCtxt.tTranscvInfo.bNumBlock = pTranscvIf->bNumBlock;
                    psNciCtxt->tTranscvCtxt.tTranscvInfo.tSendData = pTranscvIf->tSendData;
                    psNciCtxt->tTranscvCtxt.tTranscvInfo.tRecvData = pTranscvIf->tRecvData;
                    psNciCtxt->tTranscvCtxt.tTranscvInfo.wTimeout = pTranscvIf->wTimeout;

                    psNciCtxt->tTranscvCtxt.pNotify = pNotify;
                    psNciCtxt->tTranscvCtxt.pContext = pContext;

                    PHNCINFC_INIT_SEQUENCE(psNciCtxt, gpphNciNfc_RdrDataXchgSequence);

                    status = phNciNfc_RdrDataXchgSequence((void *)psNciCtxt,NULL, NFCSTATUS_SUCCESS);
                    PH_LOG_NCI_INFO_X32MSG( "RdrDataXchgSeq status received is..",status);

                    if(NFCSTATUS_PENDING != status)
                    {
                        status = PHNFCSTVAL(CID_NFC_NCI,NFCSTATUS_FAILED);
                        PH_LOG_NCI_INFO_STR("Data Exchange Request Failed..");
                    }
                }
                else
                {
                    status = NFCSTATUS_FAILED;
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_RdrMgmtRelease(
                                void     *psContext
                             )
{
    NFCSTATUS     status = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t psNciCtxt = (pphNciNfc_Context_t)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciCtxt)
    {
      status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
      PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if(NULL != gpphNciNfc_RdrDataXchgSequence)
        {
            PH_LOG_NCI_INFO_STR(" Freeing RdrDataXchgSeq Mem..");
            phOsalNfc_FreeMemory(gpphNciNfc_RdrDataXchgSequence);
            gpphNciNfc_RdrDataXchgSequence = NULL;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return status;
}

NFCSTATUS
phNciNfc_RdrDataXchgSequence(
                             void *pNciCtx,
                             void *pInfo,
                             NFCSTATUS Status
                             )
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pCtx = pNciCtx;
    pphNciNfc_TransactInfo_t pTransactInfo = pInfo;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCtx && NULL != pInfo)
    {
        pCtx->RspBuffInfo.pBuff = pTransactInfo->pbuffer;
        pCtx->RspBuffInfo.wLen = pTransactInfo->wLength;
    }
    wStatus = phNciNfc_SeqHandler(pNciCtx, Status);
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

/*Refer function declaration for information*/
NFCSTATUS
phNciNfc_CompleteDataXchgSequence(
                                  void *pContext,
                                  NFCSTATUS wStatus
                                  )
{
    NFCSTATUS     status = wStatus;
    pphNciNfc_Context_t pNciCtxt = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == pNciCtxt) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if(NULL != pNciCtxt->tTranscvCtxt.pNotify)
        {
            PH_LOG_NCI_INFO_X32MSG( "Status received is...",wStatus);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_INFO_STR("Resetting received length to 0 for this Failed Scenario!!");
                (pNciCtxt->tTranscvCtxt.tTranscvInfo.tRecvData.wLen) = 0;
            }
            PH_LOG_NCI_INFO_STR("Invoking upper layer call back function");

            //
            // The tTranscvCtxt.tSendPld.pBuff should be NULL unless there was a failure and the
            // response was not called, in that case release the buffer.
            //
            if(NULL != (pNciCtxt->tTranscvCtxt.tSendPld.pBuff))
            {
                pNciCtxt->tTranscvCtxt.tSendPld.wLen = 0;
                PH_LOG_NCI_INFO_STR(" Freeing Send Request Payload Buffer..");
                phOsalNfc_FreeMemory((pNciCtxt->tTranscvCtxt.tSendPld.pBuff));
                pNciCtxt->tTranscvCtxt.tSendPld.pBuff = NULL;
            }
            
            /*TODO:- Convert wStatus to NFCSTATUS i.e status = wStatus mapping? */
            pNciCtxt->tTranscvCtxt.pNotify(pNciCtxt->tTranscvCtxt.pContext, status,&pNciCtxt->tTranscvCtxt.tTranscvInfo.tRecvData);
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" Invalid Caller Param(s)..");
        }
    }

    PH_LOG_NCI_FUNC_EXIT();

   return status;
}

static
NFCSTATUS
phNciNfc_UpdateRemDevInfo(pphNciNfc_RemoteDevInformation_t pRemDevInf,
                          uint8_t *pBuff,
                          uint16_t wLen
                          )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t *pRfNtfBuff = NULL;
    uint8_t RfTechSpecParamsLen = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL == pRemDevInf) || (NULL == pBuff) || (0 == wLen))
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_CRIT_STR(" Invalid Param(s)..");
    }
    else
    {
        pRemDevInf->bRfDiscId = pBuff[0];
        pRemDevInf->eRfIf = (phNciNfc_RfInterfaces_t)pBuff[1];

        if(phNciNfc_e_RfInterfacesNfceeDirect_RF != (pRemDevInf->eRfIf))
        {
            pRemDevInf->eRFProtocol = (phNciNfc_RfProtocols_t)pBuff[2];
            pRemDevInf->eRFTechMode = (phNciNfc_RfTechMode_t)pBuff[3];
            pRemDevInf->bMaxPayLoadSize = pBuff[4];
            pRemDevInf->bInitialCredit = pBuff[5];
            /* Obtain the len of RF tech specific parameters from Resp buff */
            RfTechSpecParamsLen = pBuff[6];
            pRemDevInf->bTechSpecificParamLen = RfTechSpecParamsLen;
            pRfNtfBuff = &(pBuff[7+RfTechSpecParamsLen]);

            pRemDevInf->eDataXchgRFTechMode = (phNciNfc_RfTechMode_t)*(pRfNtfBuff+0);
            pRemDevInf->bTransBitRate = *(pRfNtfBuff+1);
            pRemDevInf->bRecvBitRate = *(pRfNtfBuff+2);

            /* Update Target specific info */
            switch(pRemDevInf->eRFTechMode)
            {
                case phNciNfc_NFCA_Poll:
                case phNciNfc_NFCA_Kovio_Poll:
                case phNciNfc_NFCA_Active_Poll:
                    wStatus = phNciNfc_RdrAInit(pRemDevInf,pBuff,wLen);
                break;

                case phNciNfc_NFCB_Poll:
                    wStatus = phNciNfc_RdrBInit(pRemDevInf,pBuff,wLen);
                break;

                case phNciNfc_NFCF_Poll:
                case phNciNfc_NFCF_Active_Poll:
                    wStatus = phNciNfc_RdrFInit(pRemDevInf,pBuff,wLen);
                break;

                case phNciNfc_NFCISO15693_Poll:
                    wStatus = phNciNfc_RdrIso15693Init(pRemDevInf,pBuff,wLen);
                break;

                default:
                    PH_LOG_NCI_CRIT_STR("Rf Technology and mode not supported");
                    wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                break;
            }
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Interface is NFCEE Direct RF,subsequent payload contents ignored..");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}
