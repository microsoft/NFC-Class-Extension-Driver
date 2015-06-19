/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#include <phNciNfc_Pch.h>

#include "phNciNfc_ListenIsoDep.tmh"

NFCSTATUS
phNciNfc_NfcIsoLstnRdrInit(
                            pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                            uint8_t *pBuff,
                            uint16_t wLen
                          )
{
    NFCSTATUS   wStatus = NFCSTATUS_SUCCESS;
    uint8_t     *pRfNtfBuff = NULL;
    uint8_t     RfTechSpecParamsLen = 0;
    uint8_t     bUidLength = 0;
    uint8_t     bSelResRespLen = 0;
    uint8_t     bSelRespVal = 0;

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

        /* Length of NFCID1 */
        bUidLength = *(pRfNtfBuff+2);
        /* Length of SEL_RES response */
        bSelResRespLen = *(pRfNtfBuff + 3 + bUidLength);
        if(1 == bSelResRespLen)
        {
            /* Length of SEL_RES shall be '0' incase of Nfc Forum Type 1 Tag */
            bSelRespVal = *(pRfNtfBuff + 3 + bUidLength + 1);
        }
        else
        {
            bSelRespVal = 0;
        }
        /* Length of SEL_RES shall be '0' incase of Nfc Forum Type 1 Tag */
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
