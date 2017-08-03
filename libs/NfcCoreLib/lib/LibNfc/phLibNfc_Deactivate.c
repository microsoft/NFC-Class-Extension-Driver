/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Deactivate.h"
#include "phLibNfc_Discovery.h"

#include "phLibNfc_Deactivate.tmh"

static NFCSTATUS phLibNfc_DeactivateComplete(void *pContext,NFCSTATUS status,void *pInfo);

phLibNfc_Sequence_t gphLibNfc_DeactivateSequence[] = {
    {&phLibNfc_SendDeactDiscCmd, &phLibNfc_ProcessDeactResp},
    {NULL, &phLibNfc_DeactivateComplete}
};

static NFCSTATUS phLibNfc_DeactivateComplete(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (NFCSTATUS_FAILED != status))
    {
        wStatus = phLibNfc_StateHandler(pContext, TrigEvent, pInfo, NULL, NULL);
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
