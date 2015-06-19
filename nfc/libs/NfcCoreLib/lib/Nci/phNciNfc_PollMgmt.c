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

#include "phNciNfc_RFReader.h"
#include "phNciNfc_RFReaderA.h"

#include "phNciNfc_PollMgmt.tmh"

NFCSTATUS
phNciNfc_PollMgmt(void *psContext,
                  pphNciNfc_RemoteDevInformation_t pRemDevInf,
                  uint8_t *pBuff,
                  uint16_t wLen)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t psNciCtxt = (pphNciNfc_Context_t)psContext;
    phNciNfc_RfProtocols_t eProto;
    phNciNfc_RfInterfaces_t eIntf;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL == psNciCtxt) || (0 == wLen) || (NULL == pBuff) || (NULL == pRemDevInf))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_NCI_INFO_STR("Invalid input parameters");
    }
    else
    {
        eIntf = (phNciNfc_RfInterfaces_t)pBuff[1];
        eProto = (phNciNfc_RfProtocols_t)pBuff[2];
        /* Based on active Rf Technology and Rf protocol,
        perform reader management init or P2P management init*/
        if(phNciNfc_e_RfInterfacesNFCDEP_RF == eIntf &&
            phNciNfc_e_RfProtocolsNfcDepProtocol == eProto)
        {
            wStatus = phNciNfc_NfcIPollInit(psNciCtxt,pRemDevInf,pBuff,wLen);
        }
        else
        {
            wStatus = phNciNfc_RdrMgmtInit(psNciCtxt,pRemDevInf,pBuff,wLen);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
