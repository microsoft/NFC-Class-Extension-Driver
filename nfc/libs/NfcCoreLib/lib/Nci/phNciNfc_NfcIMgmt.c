/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_NfcIMgmt.tmh"

static
NFCSTATUS
phNciNfc_UpdateNfcIRemDevInfo(pphNciNfc_RemoteDevInformation_t pRemDevInf,
                          uint8_t *pBuff,
                          uint16_t wLen
                          );

NFCSTATUS
phNciNfc_NfcIPollInit(
                          void *psContext,
                          pphNciNfc_RemoteDevInformation_t pRemDevInf,
                          uint8_t *pBuff,
                          uint16_t wLen
                         )
{
    NFCSTATUS                       wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t psNciCtxt = (pphNciNfc_Context_t)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL == psNciCtxt) || (NULL == pBuff) || (NULL == pRemDevInf))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_NCI_INFO_STR(" Invalid Param(s)..");
    }
    else
    {
        gpphNciNfc_RdrDataXchgSequence = (phNciNfc_SequenceP_t *)phOsalNfc_GetMemory(
        (2 * sizeof(phNciNfc_SequenceP_t)));

        if(NULL != gpphNciNfc_RdrDataXchgSequence)
        {
            /* Extract info into RemDevInf structure */
            wStatus = phNciNfc_UpdateNfcIRemDevInfo(pRemDevInf,pBuff,wLen);

            if(NFCSTATUS_SUCCESS == wStatus)
            {
                /* Update Target specific info */
                switch(pRemDevInf->eRFTechMode)
                {
                    case phNciNfc_NFCA_Poll:
                    case phNciNfc_NFCA_Active_Poll:
                        wStatus = phNciNfc_NfcDepPollRdrAInit(pRemDevInf,pBuff,wLen);
                    break;

                    case phNciNfc_NFCF_Poll:
                    case phNciNfc_NFCF_Active_Poll:
                        wStatus = phNciNfc_NfcDepPollRdrFInit(pRemDevInf,pBuff,wLen);
                    break;

                    default:
                        PH_LOG_NCI_INFO_STR("Rf Technology and mode not supported");
                        wStatus = NFCSTATUS_FAILED;
                    break;
                }
            }

            if(NFCSTATUS_SUCCESS == wStatus)
            {
                (psNciCtxt->tActvDevIf.pDevInfo) = pRemDevInf;
                wStatus = phNciNfc_SetConnCredentials(psNciCtxt);
            }
        }
        else
        {
            wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
            PH_LOG_NCI_INFO_STR(" DataXchg SequenceHandler pointer MemAlloc Failed..");
        }
    }

    if(NFCSTATUS_SUCCESS != wStatus)
    {
        if(NULL != gpphNciNfc_RdrDataXchgSequence)
        {
            PH_LOG_NCI_INFO_STR(" Freeing RdrDataXchgSeq Mem..");
            phOsalNfc_FreeMemory(gpphNciNfc_RdrDataXchgSequence);
            gpphNciNfc_RdrDataXchgSequence = NULL;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS
phNciNfc_NfcILstnInit(
                          void *psContext,
                          pphNciNfc_RemoteDevInformation_t pRemDevInf,
                          uint8_t *pBuff,
                          uint16_t wLen
                         )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t psNciCtxt = (pphNciNfc_Context_t)psContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == psNciCtxt) || (0 == wLen) || (NULL == pBuff) || (NULL == pRemDevInf))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_NCI_INFO_STR("Invalid input parameters");
    }
    else
    {
        /* Update Target specific info */
        switch(pRemDevInf->eRFTechMode)
        {
            case phNciNfc_NFCA_Listen:
            case phNciNfc_NFCA_Active_Listen:
                wStatus = phNciNfc_NfcDepLstnRdrAInit(pRemDevInf,pBuff,wLen);
            break;

            case phNciNfc_NFCF_Listen:
            case phNciNfc_NFCF_Active_Listen:
                wStatus = phNciNfc_NfcDepLstnRdrFInit(pRemDevInf,pBuff,wLen);
            break;

            default:
                PH_LOG_NCI_INFO_STR("Rf Technology and mode not supported");
                wStatus = NFCSTATUS_FAILED;
            break;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS
phNciNfc_UpdateNfcIRemDevInfo(pphNciNfc_RemoteDevInformation_t pRemDevInf,
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
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Interface is NFCEE Direct RF,subsequent payload contents ignored..");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}
