/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_NfceeMgmt.tmh"

#define PH_NCINFC_NUMCONFIG_NFCEE               0x01
#define PH_NCINFC_LENCONFIG_NFCEE               0x02
#define PH_NCINFC_DEST_TYPE_NFCEE               0x03
#define PH_NCINFC_CONFIG_TYPE_NFCEE             0x01
#define PH_NCINFC_BYTEPOS_START_NFCEE           0x00
#define PH_NCINFC_NFCEE_SIZEOF_TAGLEN           0x02
#define PH_NCINFC_NFCEE_TLVINFO_HWREG           0x00
#define PH_NCINFC_NFCEE_TLVINFO_ATR             0x01
#define PH_NCINFC_NFCEE_TLVINFO_T3T             0x02
#define PH_NCINFC_NFCEE_MAX_TLVINFO_T3T         0xA9
#define PH_NCINFC_NFCEE_MIN_TLVINFO_T3T         0x09

/** Macro to retrieve Empty slot to store NFCEE details */
#define PH_NCINFC_NFCEE_SEARCHEMPTYSLOT         0x00
/** Macro to Search where NFCEE Id details are stored */
#define PH_NCINFC_NFCEE_SEARCHNFCEEIDSLOT       0x01
/** Remove NFCEE Id details from the list */
#define PH_NCINFC_NFCEE_REMOVENFCEEIDSLOT       0x02
/** Fetch the Activated NFCEE id from the list */
#define PH_NCINFC_NFCEE_ACTIVATEDNFCEEIDSLOT    0x03
/** Invalid NFCEE Id dedicated for future use */
#define PH_NCINFC_NFCEE_INVALIDID               0xFF
/** Length of NFCEE Power Supply data */
#define PH_NCINFC_NFCEE_POWER_SUPPLY_LENGTH     0x01

/** Value indicates NFCEE response length */
#define PH_NCINFC_NFCEEDISC_RESP_LEN            (0x02)

static void phNciNfc_NfceeConnectCb(void* pContext, NFCSTATUS wStatus, void *pInfo);
static void phNciNfc_NfceeDisconnectCb(void* pContext, NFCSTATUS wStatus, void *pInfo);

static NFCSTATUS phNfcNfc_NfceeGetDevIndex(pphNciNfc_NfceeContext_t pNfceeCtx,
                                           uint8_t NfceeId,
                                           uint8_t *pIndex,uint8_t bVerifyId);

static NFCSTATUS phNciNfc_ModeSet(void *pContext);
static NFCSTATUS phNciNfc_ProcessModeSetRsp(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_CompleteModeSetSequence(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_PowerAndLinkCtrl(void *pContext);
static NFCSTATUS phNciNfc_PowerAndLinkCtrlRsp(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_CompletePowerAndLinkCtrlSequence(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_NfceeDiscover(void *pContext);
static NFCSTATUS phNciNfc_ProcessNfceeDiscoverRsp(void *pContext, NFCSTATUS Status);
static NFCSTATUS phNciNfc_CompleteNfceeDiscoverSequence(void *pContext, NFCSTATUS Status);
static NFCSTATUS phNciNfc_NfceeDiscNtfHandler(void *pContext, void *pInfo, NFCSTATUS wDiscStatus);
static NFCSTATUS phNciNfc_NfceeModeSetNtfHandler(void *pContext, void *pInfo, NFCSTATUS wModeSetStatus);
static NFCSTATUS phNciNfc_NfceeStatusNtfHandler(void *pContext, void *pInfo, NFCSTATUS wNfceetatus);

static NFCSTATUS phNciNfc_StoreTlvInfo(pphNciNfc_NfceeDevDiscInfo_t pDevInfo, uint8_t bNoOfTlv,uint8_t *pTlv);
static NFCSTATUS phNciNfc_StoreNfceeProtocols(pphNciNfc_NfceeDevDiscInfo_t pDevInfo, uint8_t *pBuff, uint8_t *pIndex);

static uint8_t phNciNfc_VerifyNfceeProtocol(phNciNfc_NfceeIfType_t eNfceeIf);
static NFCSTATUS phNciNfc_ValidateT3tCmdSet(pphNciNfc_NfceeDiscTlvInfo_t pTlvInfo, uint8_t *pTlv);

static NFCSTATUS phNciNfc_NfceeVerifyTlvParams(pphNciNfc_NfceeContext_t pNfceeCtx, uint8_t *pBuff, uint16_t wLen);
static NFCSTATUS phNciNfc_NfceeStoreTlv(pphNciNfc_NfceeDiscReqNtfInfo_t pDiscReqNtfInfo, uint8_t *pBuff, uint16_t wLen);

phNciNfc_SequenceP_t gphNciNfc_ModeSetSequence[] = {
    {&phNciNfc_ModeSet, &phNciNfc_ProcessModeSetRsp},
    {NULL, &phNciNfc_CompleteModeSetSequence}
};

phNciNfc_SequenceP_t gphNciNfc_SePowerAndLinkCtrlSequence[] = {
    { &phNciNfc_PowerAndLinkCtrl, &phNciNfc_PowerAndLinkCtrlRsp },
    { NULL, &phNciNfc_CompletePowerAndLinkCtrlSequence }
};

phNciNfc_SequenceP_t gphNciNfc_NfceeDiscSequence[] = {
    {&phNciNfc_NfceeDiscover, &phNciNfc_ProcessNfceeDiscoverRsp},
    {NULL, &phNciNfc_CompleteNfceeDiscoverSequence}
};

static NFCSTATUS phNciNfc_ModeSet(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo = {0};
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        /* Build the Discover Command Header */
        TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        TxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNfceeMgtGid;
        TxInfo.tHeaderInfo.Opcode_ID.OidType.NfceeMgtCmdOid = phNciNfc_e_NfceeMgtModeSetCmdOid;
        TxInfo.Buff = (uint8_t *)pNciContext->tSendPayload.pBuff;
        TxInfo.wLen = pNciContext->tSendPayload.wPayloadSize;
        wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext),
                                    &TxInfo,
                                    &(pNciContext->RspBuffInfo),
                                    PHNCINFC_NCI_CMD_RSP_TIMEOUT,
                                    (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                                    pContext);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessModeSetRsp(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciContext = pContext;
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        if(NULL != pNciContext)
        {
            /* Validate the Response of Mode Set Command */
            if( (NULL != pNciContext->RspBuffInfo.pBuff) &&
                (PH_NCINFC_STATUS_OK == pNciContext->RspBuffInfo.pBuff[0]))
            {
                if (phNciNfc_IsVersion1x(pNciContext))
                {
                    pNciContext->tNfceeContext.pNfceeDevInfo[0].tDevInfo.eNfceeStatus = \
                        pNciContext->tNfceeContext.eNfceeMode;
                    PH_LOG_NCI_INFO_STR("NFCEE Mode Set process Success");
                }
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }
    return wStatus;
}

static NFCSTATUS phNciNfc_CompleteModeSetSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    pphNciNfc_IfNotificationCb_t pUpperLayerCb = NULL;
    void *pUpperLayerCtx = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pNciCtx)
    {
        if (NULL != pNciCtx->tSendPayload.pBuff)
        {
            phOsalNfc_FreeMemory(pNciCtx->tSendPayload.pBuff);
            pNciCtx->tSendPayload.pBuff = NULL;
            pNciCtx->tSendPayload.wPayloadSize = 0;
        }

        // If operating in NCI2.0+ and NFCEE_MODE_SET_RSP indicates
        // no errors then host should wait for NFCEE_MODE_SET_NTF.
        if (phNciNfc_IsVersion1x(pNciCtx) || wStatus != NFCSTATUS_SUCCESS)
        {
            PH_LOG_NCI_INFO_STR("Invoking upper layer callback for Nfcee Mode Set");
            pUpperLayerCb = pNciCtx->ModeSetCallback;
            pUpperLayerCtx = pNciCtx->ModeSetCallbackContext;
            pNciCtx->ModeSetCallback = NULL;
            pNciCtx->ModeSetCallbackContext = NULL;

            pUpperLayerCb(pUpperLayerCtx, wStatus, NULL);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_PowerAndLinkCtrl(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo = { 0 };
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pNciContext)
    {
        phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        /* Build the Discover Command Header */
        TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        TxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNfceeMgtGid;
        TxInfo.tHeaderInfo.Opcode_ID.OidType.NfceeMgtCmdOid = phNciNfc_e_NfceeMgtPowerAndLinkCtrlCmdOid;
        TxInfo.Buff = (uint8_t *)pNciContext->tSendPayload.pBuff;
        TxInfo.wLen = pNciContext->tSendPayload.wPayloadSize;
        wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext),
                                        &TxInfo,
                                        &(pNciContext->RspBuffInfo),
                                        PHNCINFC_NCI_CMD_RSP_TIMEOUT,
                                        (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                                        pContext);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_PowerAndLinkCtrlRsp(void *pContext, NFCSTATUS wStatus)
{
    PH_LOG_NCI_FUNC_ENTRY();

    NFCSTATUS status = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    if (wStatus != NFCSTATUS_SUCCESS)
    {
        status = wStatus;
    }
    else if (pNciContext == NULL)
    {
        status = NFCSTATUS_FAILED;
    }
    else if (pNciContext->RspBuffInfo.pBuff == NULL ||
        pNciContext->RspBuffInfo.wLen == 0)
    {
        status = NFCSTATUS_FAILED;
    }
    else
    {
        uint8_t nciStatus = pNciContext->RspBuffInfo.pBuff[0];
        if (nciStatus == PH_NCINFC_STATUS_REJECTED)
        {
            status = NFCSTATUS_REJECTED;
        }
        else if (nciStatus != PH_NCINFC_STATUS_OK)
        {
            status = NFCSTATUS_FAILED;
        }
    }

    PH_LOG_NCI_FUNC_ENTRY();
    return status;
}

static NFCSTATUS phNciNfc_CompletePowerAndLinkCtrlSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pNciCtx)
    {
        if (NULL != pNciCtx->tSendPayload.pBuff)
        {
            phOsalNfc_FreeMemory(pNciCtx->tSendPayload.pBuff);
            pNciCtx->tSendPayload.pBuff = NULL;
            pNciCtx->tSendPayload.wPayloadSize = 0;
        }

        phNciNfc_Notify(pNciCtx, wStatus, NULL);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_NfceeDiscover(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        TxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNfceeMgtGid;
        TxInfo.tHeaderInfo.Opcode_ID.OidType.NfceeMgtCmdOid = phNciNfc_e_NfceeMgtNfceeDiscCmdOid;
        TxInfo.Buff = (uint8_t *)pNciContext->tSendPayload.pBuff;
        TxInfo.wLen = pNciContext->tSendPayload.wPayloadSize;
        wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext),
                                    &TxInfo,
                                    &(pNciContext->RspBuffInfo),
                                    PHNCINFC_NCI_CMD_RSP_TIMEOUT,
                                    (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                                    pContext);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessNfceeDiscoverRsp(void *pContext, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        if(NULL != pNciContext)
        {
            /* Validate the Response of Discover Command */
            if ((NULL != pNciContext->RspBuffInfo.pBuff) &&
                (PH_NCINFC_STATUS_OK == pNciContext->RspBuffInfo.pBuff[0]) &&
                (PH_NCINFC_NFCEEDISC_RESP_LEN == pNciContext->RspBuffInfo.wLen))
            {
                PH_LOG_NCI_INFO_STR("NFCEE Discovery process Started");
                /* This parameter is not stored. Once discover notifications are encountered,
                   this variable is incremented */
                pNciContext->tNfceeContext.bNfceeCount = pNciContext->RspBuffInfo.pBuff[1];
                PH_LOG_NCI_INFO_X32MSG("Number of NFCEE Connected with NFCC", pNciContext->tNfceeContext.bNfceeCount);
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    return wStatus;
}

static NFCSTATUS phNciNfc_CompleteNfceeDiscoverSequence(void *pContext, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    uint32_t dwNfceeCount;
    pphNciNfc_Context_t pNciContext = pContext;
    phNciNfc_sCoreHeaderInfo_t tNtfInfo;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        if(NULL != pNciContext->RspBuffInfo.pBuff)
        {
            if(NULL != pNciContext->tSendPayload.pBuff)
            {
                phOsalNfc_FreeMemory(pNciContext->tSendPayload.pBuff);
                pNciContext->tSendPayload.pBuff = NULL;
                pNciContext->tSendPayload.wPayloadSize = 0;
            }
            dwNfceeCount = pNciContext->tNfceeContext.bNfceeCount;
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                /* Check whether to Register/De-Register NFCEE Discover notification */
                if(PH_NCINFC_NFCEE_DISC_ENABLE == pNciContext->tNfceeContext.bNfceeDiscState)
                {
                    PH_LOG_NCI_INFO_STR("Registering for Nfcee Discover Notification");
                    tNtfInfo.bEnabled = PHNCINFC_DISABLE_AUTO_DEREG;
                    tNtfInfo.Group_ID = phNciNfc_e_CoreNfceeMgtGid;
                    tNtfInfo.Opcode_ID.OidType.NfceeMgtNtfOid = phNciNfc_e_NfceeMgtNfceeDiscNtfOid;
                    tNtfInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
                    wStatus = phNciNfc_CoreIfRegRspNtf(&(pNciContext->NciCoreContext),
                                                &(tNtfInfo),
                                                &phNciNfc_NfceeDiscNtfHandler,
                                                pContext
                                               );
                    tNtfInfo.Opcode_ID.OidType.NfceeMgtNtfOid = phNciNfc_e_NfceeMgtModeSetNtfOid;
                    wStatus = phNciNfc_CoreIfRegRspNtf(&(pNciContext->NciCoreContext),
                                                &(tNtfInfo),
                                                &phNciNfc_NfceeModeSetNtfHandler,
                                                pContext
                                               );
                    tNtfInfo.Opcode_ID.OidType.NfceeMgtNtfOid = phNciNfc_e_NfceeMgtStatusNtfOid;
                    wStatus = phNciNfc_CoreIfRegRspNtf(&(pNciContext->NciCoreContext),
                                                &(tNtfInfo),
                                                &phNciNfc_NfceeStatusNtfHandler,
                                                pContext
                                               );
                }
                else
                {
                    PH_LOG_NCI_INFO_STR("DeRegistering for Nfcee Discover Notification");
                    tNtfInfo.Group_ID = phNciNfc_e_CoreNfceeMgtGid;
                    tNtfInfo.Opcode_ID.OidType.NfceeMgtNtfOid = phNciNfc_e_NfceeMgtNfceeDiscNtfOid;
                    tNtfInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
                    wStatus = phNciNfc_CoreIfUnRegRspNtf(&(pNciContext->NciCoreContext),
                                                &(tNtfInfo),
                                                &phNciNfc_NfceeDiscNtfHandler
                                                );
                    tNtfInfo.Opcode_ID.OidType.NfceeMgtNtfOid = phNciNfc_e_NfceeMgtModeSetNtfOid;
                    wStatus = phNciNfc_CoreIfUnRegRspNtf(&(pNciContext->NciCoreContext),
                                                &(tNtfInfo),
                                                &phNciNfc_NfceeModeSetNtfHandler
                                                );
                    tNtfInfo.Opcode_ID.OidType.NfceeMgtNtfOid = phNciNfc_e_NfceeMgtStatusNtfOid;
                    wStatus = phNciNfc_CoreIfUnRegRspNtf(&(pNciContext->NciCoreContext),
                                                &(tNtfInfo),
                                                &phNciNfc_NfceeStatusNtfHandler
                                                );
                }
            }
            phNciNfc_Notify(pNciContext, wStatus,(void *)dwNfceeCount);
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_Nfcee_Connect(void * pNciHandle,
                                 void * pNfceeHandle,\
                                 pphNciNfc_IfNotificationCb_t pNfceeConnectCb,
                                 void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_Dest_Info_t tDestInfo = PHNCINFC_DEST_INFO_INIT;
    phNciNfc_Context_t * pCtx= (phNciNfc_Context_t *)pNciHandle;
    pphNciNfc_NfceeDeviceHandle_t pSeHandle = \
                (pphNciNfc_NfceeDeviceHandle_t)pNfceeHandle;
    if( (NULL != pSeHandle) &&\
        (NULL != pNfceeConnectCb) )
    {
        pCtx->pUpperLayerInfo = (void *)pNfceeHandle;
        /* Set destination type as NFCEE */
        tDestInfo.tDest = phNciNfc_e_NFCEE;
        tDestInfo.bNumDestParams = 1;
        tDestInfo.tDestParams.bDestParamType = 0x01;  /* NFCEE type*/
        tDestInfo.tDestParams.bDestParamLen = sizeof(tDestInfo.tDestParams.bDestParamVal);
        tDestInfo.tDestParams.bDestParamVal[0] = pSeHandle->tDevInfo.bNfceeID;
        tDestInfo.tDestParams.bDestParamVal[1] = (uint8_t)phNciNfc_e_NfceeHciAccessIf;  /*HCI Access type*/
        phNciNfc_SetUpperLayerCallback(pCtx, pNfceeConnectCb, pContext);
        wStatus = phNciNfc_LogConnCreate(pCtx,&tDestInfo,
                    &phNciNfc_NfceeConnectCb,
                    pCtx);
    }
    return wStatus;
}

static void phNciNfc_NfceeConnectCb(void* pContext, NFCSTATUS Status, void *pInfo)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;
    pphNciNfc_NfceeDeviceHandle_t pNfceeHandle;
    uint8_t bConnId;
    UNUSED(pInfo);
    if(NULL != pNciContext)
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            pNfceeHandle = (pphNciNfc_NfceeDeviceHandle_t)pNciContext->pUpperLayerInfo;
            /* Update Nfcee Information */
            wStatus = phNciNfc_UpdateConnDestInfo(pNfceeHandle->tDevInfo.bNfceeID,phNciNfc_e_NFCEE,pNfceeHandle);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                PH_LOG_NCI_INFO_X32MSG("NFCEE connection created for id",0x01);
                /* Retrieve the logical connection id for the Secure element*/
                wStatus = phNciNfc_GetConnId(pNfceeHandle,&bConnId);
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    /* Register for Data packets related to this logical connection*/
                }
            }
        }
        phNciNfc_Notify(pContext, wStatus,NULL);
    }
    else
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
}

NFCSTATUS phNciNfc_Nfcee_Disconnect(void * pNciHandle,
                                    void * pNfceeHandle,
                                    pphNciNfc_IfNotificationCb_t pNfceeDisconnectCb,\
                                    void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_Context_t * pCtx= (phNciNfc_Context_t *)pNciHandle;
    pphNciNfc_NfceeDeviceHandle_t pSeHandle = \
                (pphNciNfc_NfceeDeviceHandle_t)pNfceeHandle;
    if( (NULL != pSeHandle) &&\
        (NULL != pNfceeDisconnectCb) )
    {
        pCtx->pUpperLayerInfo = (void *)pNfceeHandle;
        phNciNfc_SetUpperLayerCallback(pCtx, pNfceeDisconnectCb, pContext);
        wStatus = phNciNfc_LogConnClose(pCtx,
                    pSeHandle->tDevInfo.bNfceeID,
                    phNciNfc_e_NFCEE,
                    &phNciNfc_NfceeDisconnectCb,
                    pCtx);
    }
    return wStatus;
}

static void phNciNfc_NfceeDisconnectCb(void* pContext, NFCSTATUS Status, void *pInfo)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;
    UNUSED(pInfo);
    if(NULL != pNciContext)
    {
        phNciNfc_Notify(pContext, wStatus,NULL);
    }
    else
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
}

NFCSTATUS phNciNfc_DeActivateNfcee(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t bDevIndex = 0;
    phNciNfc_NfceeContext_t * pNfceeCtxt = (phNciNfc_NfceeContext_t *)pContext;
    if(NULL != pNfceeCtxt)
    {
        /* Check whether NFCEE id is already available */
        /*Get the NFCEE Handle for this NFCEE ID*/
        wStatus = phNfcNfc_NfceeGetDevIndex(pNfceeCtxt,
                            0x00,
                            &bDevIndex,PH_NCINFC_NFCEE_ACTIVATEDNFCEEIDSLOT);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            pNfceeCtxt->pNfceeDevInfo[bDevIndex].tDevInfo.bRfDiscId = \
                                            PH_NCINFC_NFCEE_NOT_ACTIVATED;
        }
        else
        {
            wStatus = NFCSTATUS_SUCCESS;
        }
    }
    return wStatus;
}

static NFCSTATUS phNciNfc_NfceeDiscNtfHandler(void *pContext,
                                         void *pInfo,
                                         NFCSTATUS wDiscStatus)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_TransactInfo_t pTransactInfo = pInfo;
    pphNciNfc_Context_t pCtx = pContext;
    phNciNfc_NotificationInfo_t tInfo = {0};
    uint8_t *pBuff;
    uint16_t wLen;
    uint8_t bIndex = 0;
    uint16_t wLeftOver = 0;
    uint16_t wTlvLen = 0;
    uint8_t bDevIndex = 0;
    uint8_t bNfceeStatus = TRUE;
    uint8_t bNewNfceeId = FALSE;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS != wDiscStatus)
    {
        wStatus = wDiscStatus;
    }
    else if(NULL == pCtx)
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if( (NULL == pTransactInfo) ||\
             (NULL == pTransactInfo->pbuffer) ||\
             (0 == pTransactInfo->wLength) )
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pBuff = pTransactInfo->pbuffer;
        wLen = pTransactInfo->wLength;

        /* Validate NFCEE Id & Status*/
        if( (0 == pBuff[bIndex]) ||\
            (PH_NCINFC_NFCEE_INVALIDID == pBuff[bIndex]) ||\
            (pBuff[bIndex + 1] > PH_NCINFC_NFCEE_REMOVED) )
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        else
        {
            /* Check NFCEE status to remove Id details from the list */
            if(PH_NCINFC_NFCEE_REMOVED != pBuff[bIndex + 1])
            {
                bNfceeStatus = TRUE;
                /* Check whether NFCEE id is already available */
                /*Get the NFCEE Handle for this NFCEE ID*/
                wStatus = phNfcNfc_NfceeGetDevIndex(&(pCtx->tNfceeContext),
                                    pBuff[bIndex],
                                    &bDevIndex,PH_NCINFC_NFCEE_SEARCHNFCEEIDSLOT);
                if(NFCSTATUS_FAILED == wStatus)
                {
                    bNewNfceeId = TRUE;
                    /* Get a empty slot to store NFCEE details */
                    wStatus = phNfcNfc_NfceeGetDevIndex(&(pCtx->tNfceeContext),
                                    pBuff[bIndex],
                                    &bDevIndex,(uint8_t)PH_NCINFC_NFCEE_SEARCHEMPTYSLOT);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        /* Store the RF disc ID[which indicates the NFCEE is Activated] to INVALID */
                        pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.bRfDiscId = \
                                    PH_NCINFC_NFCEE_NOT_ACTIVATED;
                    }
                }
            }
            else
            {
                bNfceeStatus = FALSE;
                tInfo.tNfceeInfo.bNfceeStatus = bNfceeStatus;
                tInfo.tNfceeInfo.bNfceeId = pBuff[0];
                tInfo.tNfceeInfo.pNfceeHandle = NULL;
            }
        }
        /* Store Notification info if NFCEE is connected */
        if( (NFCSTATUS_SUCCESS == wStatus) &&\
            (pBuff[bIndex + 1] != PH_NCINFC_NFCEE_REMOVED) )
        {
            /* Store the RF disc ID[which indicates the NFCEE is Activated] to INVALID */
            pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.bRfDiscId = PH_NCINFC_NFCEE_NOT_ACTIVATED;
            /* Store NFCEE ID & status */
            pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.bNfceeID = pBuff[bIndex++];

            if(0x00 == pBuff[bIndex])
            {
                pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.eNfceeStatus = PH_NCINFC_EXT_NFCEEMODE_ENABLE;
            }
            else
            {
                pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.eNfceeStatus = PH_NCINFC_EXT_NFCEEMODE_DISABLE;
                pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.bRfDiscId = PH_NCINFC_NFCEE_NOT_ACTIVATED;
            }
            bIndex++;
            wStatus = phNciNfc_StoreNfceeProtocols(&pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo\
                                                    ,pBuff,&bIndex);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                wStatus = phNciNfc_TlvUtilsValidate(&pBuff[bIndex + 1], (wLen - bIndex - 1), &wLeftOver);
                wTlvLen = wLen - bIndex - 1 - wLeftOver;
                if ((NFCSTATUS_SUCCESS == wStatus) ||
                    (wLeftOver == PH_NCINFC_NFCEE_POWER_SUPPLY_LENGTH))
                {
                    pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.TlvInfoLen = wTlvLen;
                    pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.bNumTypeInfo = (uint8_t)pBuff[bIndex];
                    pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.pTlvInfo = phOsalNfc_GetMemory(wTlvLen);

                    if(NULL != pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.pTlvInfo)
                    {
                        phOsalNfc_MemCopy(pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.pTlvInfo,
                                          &pBuff[bIndex + 1], wTlvLen);
                        wStatus = phNciNfc_StoreTlvInfo(&pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo,
                                                        pBuff[bIndex],
                                                        pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.pTlvInfo);
                    }
                    else
                    {
                        wStatus = NFCSTATUS_FAILED;
                    }
                }
            }
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.bNfceeID = (uint8_t)0x00;
            }
            else
            {
                if(TRUE == bNewNfceeId)
                {
                    pCtx->tNfceeContext.bNumberOfNfcee++;
                }
            }
        }
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if(TRUE == bNfceeStatus)
            {
                tInfo.tNfceeInfo.bNfceeStatus = bNfceeStatus;
                tInfo.tNfceeInfo.bNfceeId = pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.bNfceeID;
                tInfo.tNfceeInfo.pNfceeHandle = &pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex];
            }
            if(NULL != pCtx->tRegListInfo.pNfceeNotification)
            {
                pCtx->tRegListInfo.pNfceeNotification(pCtx,eNciNfc_NfceeDiscoverNtf,\
                                    &tInfo,wStatus);
            }
            if(FALSE == bNfceeStatus)
            {
                /*Remove NFCEE details from the list*/
                (void)phNfcNfc_NfceeGetDevIndex(&(pCtx->tNfceeContext),
                                                tInfo.tNfceeInfo.bNfceeId,
                                                &bDevIndex,PH_NCINFC_NFCEE_REMOVENFCEEIDSLOT);
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_NfceeModeSetNtfHandler(void *pContext,
                                                 void *pInfo,
                                                 NFCSTATUS wModeSetStatus)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_TransactInfo_t pTransactInfo = pInfo;
    pphNciNfc_Context_t pCtx = pContext;
    pphNciNfc_IfNotificationCb_t pUpperLayerCb = NULL;
    void *pUpperLayerCtx = NULL;
    uint8_t *pBuff;
    uint16_t wLen;
    uint8_t bIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NFCSTATUS_SUCCESS != wModeSetStatus)
    {
        wStatus = wModeSetStatus;
    }
    else if (NULL == pCtx)
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if ((NULL == pTransactInfo) || \
        (NULL == pTransactInfo->pbuffer) || \
        (0 == pTransactInfo->wLength))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pBuff = pTransactInfo->pbuffer;
        wLen = pTransactInfo->wLength;
        if (wLen == 1)
        {
            wStatus = pBuff[bIndex];
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }

        if (wStatus == NFCSTATUS_OK)
        {
            /* With NCI2.0 If an NFCEE is disabled, the DH and the NFCC SHALL NOT consider the NFCEE
             * enabled until the DH gets a NFCEE_MODE_SET_NTF with a Status set to STATUS_OK following
             * a NFCEE_MODE_SET_CMD(Enable).
             */
            pCtx->tNfceeContext.pNfceeDevInfo[0].tDevInfo.eNfceeStatus = \
                pCtx->tNfceeContext.eNfceeMode;
            PH_LOG_NCI_INFO_STR("NFCEE_MODE_SET_NTF: Success");
        }
    }

    if (pCtx != NULL && pCtx->ModeSetCallback != NULL)
    {
        PH_LOG_NCI_INFO_STR("Invoking upper layer callback for Nfcee Mode Set");
        pUpperLayerCb = pCtx->ModeSetCallback;
        pUpperLayerCtx = pCtx->ModeSetCallbackContext;
        pCtx->ModeSetCallback = NULL;
        pCtx->ModeSetCallbackContext = NULL;

        pUpperLayerCb(pUpperLayerCtx, wStatus, NULL);
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("NFCEE_MODE_SET_NTF received at unexpected time.");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }

    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phNciNfc_NfceeStatusNtfHandler(void *pContext, void *pInfo, NFCSTATUS wNfceeStatus)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_TransactInfo_t pTransactInfo = pInfo;
    phNciNfc_NotificationInfo_t tInfo = { 0 };
    pphNciNfc_Context_t pCtx = pContext;
    uint8_t *pBuff;
    uint16_t wLen;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NFCSTATUS_SUCCESS != wNfceeStatus)
    {
        wStatus = wNfceeStatus;
    }
    else if (NULL == pCtx)
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if ((NULL == pTransactInfo) || \
        (NULL == pTransactInfo->pbuffer) || \
        (0 == pTransactInfo->wLength))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pBuff = pTransactInfo->pbuffer;
        wLen = pTransactInfo->wLength;

        if (wLen != 2)
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        else
        {
            wStatus = NFCSTATUS_SUCCESS;
            if (NULL != pCtx->tRegListInfo.pNfceeNotification)
            {
                tInfo.tNfceeStatusInfo.bNfceeId = pTransactInfo->pbuffer[0];
                tInfo.tNfceeStatusInfo.bNfceeStatus = pTransactInfo->pbuffer[1];
                pCtx->tRegListInfo.pNfceeNotification(pCtx, eNciNfc_NfceeStatusNtf, \
                    &tInfo, wStatus);
            }
        }
    }

    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phNciNfc_StoreTlvInfo(pphNciNfc_NfceeDevDiscInfo_t pDevInfo,
                                uint8_t bNoOfTlv,uint8_t *pTlv)
{
    NFCSTATUS wStoreStatus;
    uint8_t bTlvIndex = 0;
    uint8_t bCount;/* Variable to traverse through TLVs */
    uint8_t bIndex = 0;
    /* If TLVs are not present, return success*/
    if(0 == bNoOfTlv)
    {
        wStoreStatus = NFCSTATUS_SUCCESS;
    }
    else if(bNoOfTlv > PH_NCINFC_NFCEE_INFO_TLV_MAX)
    {
        wStoreStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Initialize status to success */
        wStoreStatus = NFCSTATUS_SUCCESS;
        for(bCount = 0; ( (bCount < bNoOfTlv) && (NFCSTATUS_SUCCESS == wStoreStatus) ); bCount++)
        {
            switch((phNciNfc_NfceeTlvTypes_t)pTlv[bIndex])
            {
                case phNciNfc_HwRegIdType:
                {
                    /* Store Hardware Id directly as the length of TLV is already validated */
                    pDevInfo->aAdditionalInfo[bTlvIndex].Type = phNciNfc_HwRegIdType;
                    pDevInfo->aAdditionalInfo[bTlvIndex].bInfoLength = pTlv[bIndex + 1];
                    pDevInfo->aAdditionalInfo[bTlvIndex].tNfceeValue.pHwId = &pTlv[bIndex + 2];
                    /* Increment Index to point to the next TLV */
                    bIndex+= (pTlv[bIndex + 1] + PH_NCINFC_NFCEE_MIN_TLV_LEN);
                    bTlvIndex++;
                }break;
                case phNciNfc_AtrBytesType:
                {
                    /* Store Hardware Id directly as the length of TLV is already validated */
                    pDevInfo->aAdditionalInfo[bTlvIndex].Type = phNciNfc_AtrBytesType;
                    pDevInfo->aAdditionalInfo[bTlvIndex].bInfoLength = pTlv[bIndex + 1];
                    pDevInfo->aAdditionalInfo[bTlvIndex].tNfceeValue.pAtrBytes = &pTlv[bIndex + 2];
                    /* Increment Index to point to the next TLV */
                    bIndex+= (pTlv[bIndex + 1] + PH_NCINFC_NFCEE_MIN_TLV_LEN);
                    bTlvIndex++;
                }break;
                case phNciNfc_T3TCmdSetType:
                {
                    wStoreStatus = phNciNfc_ValidateT3tCmdSet(&pDevInfo->aAdditionalInfo[bTlvIndex],\
                                                           &pTlv[bIndex]);
                    if(NFCSTATUS_SUCCESS == wStoreStatus)
                    {
                        pDevInfo->aAdditionalInfo[bTlvIndex].Type = phNciNfc_T3TCmdSetType;
                        pDevInfo->aAdditionalInfo[bTlvIndex].bInfoLength = pTlv[bIndex + 1];
                        pDevInfo->aAdditionalInfo[bTlvIndex].tNfceeValue.pT3tCmdSetInfo = \
                                                (phNciNfc_NfceeT3tInfo_t *)&pTlv[bIndex + 2];
                    }
                    /* Increment Index to point to the next TLV */
                    bIndex+= (pTlv[bIndex + 1] + PH_NCINFC_NFCEE_MIN_TLV_LEN);
                    bTlvIndex++;
                }break;
                default:
                {
                    bIndex+= (pTlv[bIndex + 1] + PH_NCINFC_NFCEE_MIN_TLV_LEN);
                    bTlvIndex++;
                }break;
            }
        }
    }
    return wStoreStatus;
}

static
NFCSTATUS phNciNfc_ValidateT3tCmdSet(pphNciNfc_NfceeDiscTlvInfo_t pTlvInfo,\
                                    uint8_t *pTlv)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bIndex = 0;
    uint8_t bCount;
    UNUSED(pTlvInfo);
    /* Following Validations are Done before storing T3T command set info
       1. Check whether Length of T3T Cmd set info is within specified range
       2. Check whether Number of Entries is within specified range */
    PH_LOG_NCI_FUNC_ENTRY();
    if( (pTlv[bIndex + 1] >= PH_NCINFC_NFCEE_T3T_CMDSET_LEN_MIN) &&\
        (pTlv[bIndex + 1] <= PH_NCINFC_NFCEE_T3T_CMDSET_LEN_MAX) &&\
        ( (pTlv[bIndex + PH_NCINFC_NFCEE_MIN_TLV_LEN + PH_NCINFC_NFCEE_T3T_MAX_PMM_LEN]) <=\
                        PH_NCINFC_NFCEE_T3T_MAX_ENTRIES) )
    {
        /* Validate the Length of T3T command set
           Length of T3T cmd set = Length of PMm + Entries Byte +
                                    (Number of Entries * Length of Each Entry)*/
        bCount = ( PH_NCINFC_NFCEE_T3T_MAX_PMM_LEN + 1 +\
                   ((pTlv[bIndex + PH_NCINFC_NFCEE_MIN_TLV_LEN + PH_NCINFC_NFCEE_T3T_MAX_PMM_LEN]) * \
                    PH_NCINFC_NFCEE_T3T_ENTRY_LEN) );
        if(pTlv[bIndex + 1] == bCount)
        {
            wStatus = NFCSTATUS_SUCCESS;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phNciNfc_StoreNfceeProtocols(pphNciNfc_NfceeDevDiscInfo_t pDevInfo,
                                       uint8_t *pBuff,
                                       uint8_t *pIndex)
{
    NFCSTATUS wStoreStatus = NFCSTATUS_SUCCESS;
    uint8_t bCount;
    uint8_t bIndex = *pIndex;
    PH_LOG_NCI_FUNC_ENTRY();
    if(pBuff[bIndex] <= PH_NCINFC_NFCEE_SUPPORTEDPROTO_MAX)
    {
        pDevInfo->bNumSuppProtocols = pBuff[bIndex++];
        for(bCount = 0;\
            bCount < (pDevInfo->bNumSuppProtocols );\
            bCount++)
        {
            if(phNciNfc_VerifyNfceeProtocol((phNciNfc_NfceeIfType_t)pBuff[bIndex]))
            {
                wStoreStatus = NFCSTATUS_INVALID_PARAMETER;
                break;
            }
            else
            {
                pDevInfo->aSuppProtocols[bCount] = \
                        pBuff[bIndex++];
                PH_LOG_NCI_INFO_STR("NFCEE %02X supported protocol: %02X", pDevInfo->bNfceeID, pDevInfo->aSuppProtocols[bCount]);
            }
        }
        *pIndex = bIndex;
    }
    else
    {
        wStoreStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStoreStatus;
}

/*To Be registred with Receive Manager*/
NFCSTATUS phNciNfc_NfceeActionNtfHandler(void *pContext,
                                            void *pInfo,
                                            NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_TransactInfo_t pTransactInfo = pInfo;
    pphNciNfc_Context_t pCtx = pContext;
    phNciNfc_NfceeDiscReqNtfInfo_t NtfInfo;
    phNciNfc_NotificationInfo_t tInfo = {0};
    phNfc_tNfceeActionInfo_t tNfceeActionInfo = {0};
    uint8_t bDevIndex = 0;
    uint8_t bIndex = 0;
    uint8_t *pBuff;
    uint16_t wLen;
    UNUSED(NtfInfo);
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pCtx)
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if( (NULL == pTransactInfo) ||\
             (NULL == pTransactInfo->pbuffer) )
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Assign the Ntf buffer pointer and length */
        pBuff = pTransactInfo->pbuffer;
        wLen = pTransactInfo->wLength;
        if(wLen >= PH_NCINFC_NFCEEACTION_MIN_LEN)
        {
            /* Set the status to Invalid parameter */
            wStatus = NFCSTATUS_INVALID_PARAMETER;
            tNfceeActionInfo.bNfceeId = pBuff[bIndex++];
            tNfceeActionInfo.eTriggerType = (phNciNfc_RfNfceeTriggers_t)pBuff[bIndex];
            switch((phNciNfc_RfNfceeTriggers_t)pBuff[bIndex++])
            {
                case PH_NCINFC_TRIG_ISO7816_SELECT:
                case PH_NCINFC_TRIG_APP_INITIATION:
                {
                    tNfceeActionInfo.bSupDataLen = pBuff[bIndex++];
                    phOsalNfc_MemCopy(\
                        tNfceeActionInfo.phNciNfc_SupportData_t.aSupData,\
                        &pBuff[bIndex],tNfceeActionInfo.bSupDataLen);
                    /* Increment index to point to end of Data */
                    bIndex += tNfceeActionInfo.bSupDataLen;
                    wStatus = NFCSTATUS_SUCCESS;
                }break;
                case PH_NCINFC_TRIG_RFPROTOCOL_ROUTING:
                {
                    if( (0x01 == pBuff[bIndex])&&\
                        (!phNciNfc_ValidateRfProtocol((phNciNfc_RfProtocols_t)pBuff[bIndex+1])) )
                    {
                        tNfceeActionInfo.bSupDataLen = pBuff[bIndex++];
                        tNfceeActionInfo.phNciNfc_SupportData_t.eRfProtocol =\
                                        (phNciNfc_RfProtocols_t)pBuff[bIndex++];
                        wStatus = NFCSTATUS_SUCCESS;
                    }
                }break;
                case PH_NCINFC_TRIG_RFTECHNOLOGY_ROUTING:
                {
                    if( (0x01 == pBuff[bIndex])&&\
                        (!phNciNfc_ValidateRfTechMode((phNciNfc_RfTechMode_t)pBuff[bIndex+1])) )
                    {
                        tNfceeActionInfo.bSupDataLen = pBuff[bIndex++];
                        tNfceeActionInfo.phNciNfc_SupportData_t.eRfTechMode =\
                                        (phNciNfc_RfTechMode_t)pBuff[bIndex++];
                        wStatus = NFCSTATUS_SUCCESS;
                    }
                }break;
                default:
                {
                    /*do nothing*/
                }break;
            }
            /* Verify the length of notification */
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                if(wLen != bIndex)
                {
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                }
            }
        }
        if(NFCSTATUS_SUCCESS != wStatus)
        {
            /* Clear all the structure members in case of error */
            phOsalNfc_SetMemory(&tNfceeActionInfo,0x00,\
                            sizeof(tNfceeActionInfo));
        }
        else
        {
            /* Determine the handle of NFCEE for which this notification is
               received*/
            wStatus = phNfcNfc_NfceeGetDevIndex(&(pCtx->tNfceeContext),
                                    pBuff[0],
                                    &bDevIndex,PH_NCINFC_NFCEE_SEARCHNFCEEIDSLOT);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                tNfceeActionInfo.pNfceeHandle = \
                            &(pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex]);
            }
        }
        /** TODO: Invoke upper layer with this notification */
        tInfo.pActionInfo = &(tNfceeActionInfo);
        if( (NULL != pCtx->tRegListInfo.pNfceeNotification) &&\
            (NFCSTATUS_SUCCESS == wStatus) )
        {
            pCtx->tRegListInfo.pNfceeNotification(pCtx,eNciNfc_NfceeActionNtf,\
                                &tInfo,wStatus);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_NfceeDiscReqNtfHandler(void *pContext,
                                          void *pInfo,
                                          NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_TransactInfo_t pTransactInfo = pInfo;
    pphNciNfc_Context_t pCtx = pContext;
    phNciNfc_NotificationInfo_t tInfo = {0};
    phNciNfc_NfceeDiscReqNtfParams_t aNfceeDiscReq[16];
    uint8_t *pBuff;
    uint16_t wLen;
    uint8_t bIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pCtx)
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NULL == pTransactInfo)
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pBuff = pTransactInfo->pbuffer;
        wLen = pTransactInfo->wLength;
        /* Validate the Length of TLVs */
        if( wLen == \
            ((pBuff[bIndex] * (PH_NCINFC_NFCEE_MIN_TLV_LEN + \
                                PH_NCINFC_NFCEE_START_STOP_RFCOMM_TLV_LEN)) + 1) )
        {
            /* Validate the TLVs  */
            wStatus = phNciNfc_NfceeVerifyTlvParams(&(pCtx->tNfceeContext),pBuff,wLen);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                tInfo.tNfceeDiscReqInfo.pParams = aNfceeDiscReq;
                wStatus = phNciNfc_NfceeStoreTlv(&(tInfo.tNfceeDiscReqInfo), pBuff, wLen);
            }
            if( (NULL != pCtx->tRegListInfo.pNfceeNotification) &&\
                (NFCSTATUS_SUCCESS == wStatus) )
            {   /* By now, pNfceeDiscReqInfo contains updated data */
                pCtx->tRegListInfo.pNfceeNotification(pCtx,eNciNfc_NfceeDiscReqNtf,\
                                    &tInfo,wStatus);
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_NfceeVerifyTlvParams(pphNciNfc_NfceeContext_t pNfceeCtx,\
                                        uint8_t *pBuff, uint16_t wLen)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t bCount = 0;
    uint8_t bIndex = 0;
    uint8_t bDevIndex;
    uint8_t bTlvIndex;
    UNUSED(wLen);
    PH_LOG_NCI_FUNC_ENTRY();
    /* Validate the Type, Length and value of Each TLV */
    /* Copy the (Number of TLVs) to parse them */
    bCount = pBuff[bIndex++];
    for(bTlvIndex = 0; ((bTlvIndex < bCount) &&\
                        (NFCSTATUS_SUCCESS == wStatus)); bTlvIndex++)
    {
        switch(pBuff[bIndex++])
        {
            case PH_NCINFC_NFCEE_START_RFCOMM_TLV:
            case PH_NCINFC_NFCEE_STOP_RFCOMM_TLV:
            {
                if(PH_NCINFC_NFCEE_START_STOP_RFCOMM_TLV_LEN != pBuff[bIndex++])
                {
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                }
                else
                {
                    /*Get the NFCEE Handle for this NFCEE ID*/
                    wStatus = phNfcNfc_NfceeGetDevIndex(pNfceeCtx,
                                    pBuff[bIndex],
                                    &bDevIndex,PH_NCINFC_NFCEE_SEARCHNFCEEIDSLOT);
                }
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    if( (phNciNfc_ValidateRfTechMode((phNciNfc_RfTechMode_t)pBuff[bIndex + 1])) ||\
                        (phNciNfc_ValidateRfProtocol((phNciNfc_RfProtocols_t)pBuff[bIndex + 2])) )
                    {
                        wStatus = NFCSTATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        /* Point Index to Next TLV */
                        bIndex += PH_NCINFC_NFCEE_START_STOP_RFCOMM_TLV_LEN;
                    }
                }
            }break;
            default:
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_NfceeStoreTlv(pphNciNfc_NfceeDiscReqNtfInfo_t pDiscReqNtfInfo,\
                                        uint8_t *pBuff, uint16_t wLen)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t bIndex = 0;
    uint8_t bTlvIndex;
    pphNciNfc_NfceeDiscReqNtfParams_t pReqInfo = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pBuff) && (PH_NCINFC_NFCEEDISC_MIN_LEN <= wLen))
    {
        /* Copy the (Number of TLVs) to parse them */
        pDiscReqNtfInfo->bCount = pBuff[bIndex++];
        for(bTlvIndex = 0, pReqInfo = pDiscReqNtfInfo->pParams; ((bTlvIndex < pDiscReqNtfInfo->bCount) &&\
                            (NFCSTATUS_SUCCESS == wStatus)); bTlvIndex++, pReqInfo++)
        {
            /* Store the details in the following order
            1. Type of Tlv
            2. Length of Tlv - No Need to store
            3. Nfcee ID of Tlv
            4. RF Technology and Mode of Tlv
            5. RF Protocol of Tlv */
            pReqInfo->bType = pBuff[bIndex++];
            bIndex++;
            pReqInfo->bNfceeId = pBuff[bIndex++];
            pReqInfo->eTechMode = (phNciNfc_RfTechMode_t) pBuff[bIndex++];
            pReqInfo->eProtocol = (phNciNfc_RfProtocols_t) pBuff[bIndex++];
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phNfcNfc_NfceeGetDevIndex(pphNciNfc_NfceeContext_t pNfceeCtx,
                                           uint8_t NfceeId,
                                           uint8_t *pIndex,
                                           uint8_t bVerifyId)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    uint8_t bIndex = 0;
    if( (NULL != pNfceeCtx) && (NULL != pIndex) )
    {
        for (bIndex = 0; (bIndex < PH_NCINFC_NFCEE_DEVICE_MAX); bIndex++)
        {
            /* Check whether empty slot needs to be retrieved */
            if(PH_NCINFC_NFCEE_SEARCHEMPTYSLOT == bVerifyId)
            {
                if(0 == pNfceeCtx->pNfceeDevInfo[bIndex].tDevInfo.bNfceeID)
                {
                    (*pIndex) = bIndex;
                    wStatus = NFCSTATUS_SUCCESS;
                    break;
                }
            }
            /* Get the index at which NFCEE Id Info is stored */
            else if(PH_NCINFC_NFCEE_SEARCHNFCEEIDSLOT == bVerifyId)
            {
                if(pNfceeCtx->pNfceeDevInfo[bIndex].tDevInfo.bNfceeID == NfceeId)
                {
                    (*pIndex) = bIndex;
                    wStatus = NFCSTATUS_SUCCESS;
                    break;
                }
            }
            /* Remove NFCEE Id info from the list */
            else if(PH_NCINFC_NFCEE_REMOVENFCEEIDSLOT == bVerifyId)
            {
                if(pNfceeCtx->pNfceeDevInfo[bIndex].tDevInfo.bNfceeID == NfceeId)
                {
                    if (NULL != pNfceeCtx->pNfceeDevInfo[bIndex].tDevInfo.pTlvInfo) {
                        phOsalNfc_FreeMemory(pNfceeCtx->pNfceeDevInfo[bIndex].tDevInfo.pTlvInfo);
                        pNfceeCtx->pNfceeDevInfo[bIndex].tDevInfo.pTlvInfo = NULL;
                    }

                    phOsalNfc_SetMemory(&pNfceeCtx->pNfceeDevInfo[bIndex].tDevInfo,0,sizeof(phNciNfc_DeviceInfo_t));
                    pNfceeCtx->bNfceeCount--;
                    pNfceeCtx->bNumberOfNfcee--;
                    wStatus = NFCSTATUS_SUCCESS;
                    break;
                }
            }
            /* Get Activated NFCEE id from the list */
            else if(PH_NCINFC_NFCEE_ACTIVATEDNFCEEIDSLOT == bVerifyId)
            {
                if( (bIndex < pNfceeCtx->bNumberOfNfcee) &&\
                    (pNfceeCtx->pNfceeDevInfo[bIndex].tDevInfo.bRfDiscId != PH_NCINFC_NFCEE_NOT_ACTIVATED))
                {
                    (*pIndex) = bIndex;
                    wStatus = NFCSTATUS_SUCCESS;
                    break;
                }
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
                break;
            }
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    return wStatus;
}

static uint8_t
phNciNfc_VerifyNfceeProtocol(phNciNfc_NfceeIfType_t eNfceeIf)
{
    uint8_t bStatus = 1;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Verify whether input Rf Protocol is out of the supported range */
    switch(eNfceeIf)
    {
        case phNciNfc_e_NfceeApduIf:
        case phNciNfc_e_NfceeHciAccessIf:
        case phNciNfc_e_NfceeT3tIf:
        case phNciNfc_e_NfceeTransIf:
            bStatus = 0;
            break;
        default :
            bStatus = (eNfceeIf >= PH_NCINFC_NFCEE_PROTOS_PROP_MIN &&
                       eNfceeIf <= PH_NCINFC_NFCEE_PROTOS_PROP_MAX) ? 0 : 1;
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bStatus;
}

NFCSTATUS phNciNfc_RfFieldInfoNtfHandler(void *pContext, void *pInfo,NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_TransactInfo_t pTransactInfo = pInfo;
    pphNciNfc_Context_t pCtx = pContext;
    /* Default index value of Nfcee */
    uint8_t bDevIndex = 0;
    phNciNfc_NotificationInfo_t tRfFieldInfo = {0};
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL == pCtx)
    {
        /*Lower Layer not behaving properly*/
    }
    else if( (NULL == pTransactInfo) ||\
        (NULL == pTransactInfo->pbuffer) )
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Check whether NFCEE id is already available */
        /*Get the NFCEE Handle for this NFCEE ID*/
        if(pTransactInfo->wLength == 1)
        {
            if( (0x00 == pTransactInfo->pbuffer[0]) ||\
                (0x01 == pTransactInfo->pbuffer[0]) )
            {
                if(0x00 == pTransactInfo->pbuffer[0])
                {
                    PH_LOG_NCI_CRIT_STR("RF Field OFF");
                    tRfFieldInfo.tRfFieldInfo.eRfFieldInfo = phNciNfc_e_RfFieldOff;
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("RF Field ON");
                    tRfFieldInfo.tRfFieldInfo.eRfFieldInfo = phNciNfc_e_RfFieldOn;
                }
                /* Update the device handle */
                tRfFieldInfo.tRfFieldInfo.pNfceeHandle = \
                        &(pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex]);
                if(NULL != pCtx->tRegListInfo.pNfceeNotification)
                {
                   pCtx->tRegListInfo.pNfceeNotification(pCtx,eNciNfc_NciRfFieldInfoNtf,\
                        (void *)&tRfFieldInfo, NFCSTATUS_SUCCESS);
                }
            }
        }
        else
        {
            /*Notification packet corrupted drop it*/
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_NfceeProcessRfActvtdNtf(void *pContext,
                                           uint8_t *pBuff, uint16_t Len)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pCtx = pContext;
    pphNciNfc_RemoteDevInformation_t pRemDevInfo = NULL;
    phNciNfc_NotificationInfo_t tDevInfo;
    uint8_t bDiscId = 0;
    uint8_t bDevIndex = 0;
    uint8_t bIndex = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    if( (NULL == pCtx) ||
        (NULL == pBuff) ||
        (2 > Len))
    {
        /*Invalid Parameter is passed */
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if (pBuff[1] == 0x00)
    {
        bDiscId = pBuff[0];
        wStatus = phNfcNfc_NfceeGetDevIndex(&(pCtx->tNfceeContext),
                                    pBuff[bIndex],
                                    &bDevIndex,PH_NCINFC_NFCEE_SEARCHNFCEEIDSLOT);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            pRemDevInfo = (pphNciNfc_RemoteDevInformation_t)
                phOsalNfc_GetMemory(sizeof(phNciNfc_RemoteDevInformation_t));
            if(NULL != pRemDevInfo)
            {
                /* Reset structure members to store details of Remotedevice */
                phOsalNfc_SetMemory(pRemDevInfo,0x00,\
                            sizeof(phNciNfc_RemoteDevInformation_t));
                pRemDevInfo->bRfDiscId = pBuff[0];
                pRemDevInfo->eRfIf = (phNciNfc_RfInterfaces_t)pBuff[1];
                pRemDevInfo->SessionOpened = 1;
                /* Update to list of Remotedevice information if this is the only notification */
                wStatus = phNciNfc_UpdateNtfList(&pCtx->NciDiscContext.tDevInfo,
                                        pRemDevInfo,TRUE);
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    /*search from the list of NFCEE Device handle for Active NFCEE*/
                    /*It is applicable to the active NFCEE only*/
                    /*Currently ony one NFCEE connected so dont care much*/
                    /* Store the Discover ID as Nfcee ID to indicate the device is activated */
                    pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.bRfDiscId = \
                        pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex].tDevInfo.bNfceeID;
                    tDevInfo.pNfceeHandle = &(pCtx->tNfceeContext.pNfceeDevInfo[bDevIndex]);
                    if(NULL != pCtx->tRegListInfo.pNfceeNotification)
                    {
                       pCtx->tRegListInfo.pNfceeNotification(pCtx,eNciNfc_NciActivateNfceeNtf,\
                            (void *)&tDevInfo, NFCSTATUS_SUCCESS);
                    }
                }
                else
                {
                    phOsalNfc_FreeMemory(pRemDevInfo);
                    pRemDevInfo = NULL;
                }
            }
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
