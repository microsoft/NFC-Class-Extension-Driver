/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h"

#include "phNciNfc_Core.tmh"

#define PHNCINFC_CORE_RESET_CMD_PAYLOADLEN                  (1U)
#define PHNCINFC_CORE_RESET_CMD_PAYLOADLEN                  (1U)
#define PHNCINFC_CORE_SETCONFIG_CMD_PAYLOADLEN              (3U)
#define PHNCINFC_CORE_GETCONFIG_CMD_PAYLOADLEN              (2U)
#define PHNCINFC_CORE_DHCONN_CMD_PAYLOADLEN_NLB             (4U)
#define PHNCINFC_CORE_DHCONN_CMD_PAYLOADLEN_LB              (2U)
#define PHNCINFC_CORE_CONNCLOSE_CMD_PAYLOADLEN              (1U)
#define PHNCINFC_CORE_SETPOWERSUBSTATE_CMD_PAYLOADLEN       (1U)
#define PHNCINFC_CORE_DISCMAP_CMD_PAYLOADLEN                (4U)
#define PHNCINFC_CORE_SETRTNG_CMD_PAYLOADLEN                (7U)
#define PHNCINFC_CORE_DISCOVER_CMD_PAYLOADLEN               (3U)
#define PHNCINFC_CORE_DISCSEL_CMD_PAYLOADLEN                (3U)
#define PHNCINFC_CORE_DEACTIVATE_CMD_PAYLOADLEN             (1U)
#define PHNCINFC_CORE_T3TPOLL_CMD_PAYLOADLEN                (4U)
#define PHNCINFC_CORE_PARAMUPDATE_CMD_PAYLOADLEN            (3U)
#define PHNCINFC_CORE_NFCEEDISC_CMD_PAYLOADLEN_1x           (1U)
#define PHNCINFC_CORE_NFCEEDISC_CMD_PAYLOADLEN_2x           (0U)
#define PHNCINFC_CORE_NFCEEMODESET_CMD_PAYLOADLEN           (2U)

#define PHNCINFC_CORE_TESTANTENNA_CMD_PAYLOADMINLEN         (2U)
#define PHNCINFC_CORE_TESTANTENNA_CMD_PAYLOADMAXLEN         (4U)
#define PHNCINFC_CORE_TESTSWP_CMD_PAYLOADLEN                (1U)
#define PHNCINFC_CORE_SETLISFILTER_CMD_PAYLOADLEN           (1U)
#define PHNCINFC_CORE_TESTPRBS_CMD_PAYLOADLEN               (4U)

static void phNciNfc_CoreSendCb(void *pContext, phTmlNfc_TransactInfo_t *pInfo);
static void phNciNfc_CoreRecvCb(void *pContext, phTmlNfc_TransactInfo_t *pInfo);

static NFCSTATUS phNciNfc_CoreReceiveManager_Init(phNciNfc_CoreContext_t *pContext);
static void phNciNfc_CoreReleaseSendRecvStateMachines(void *pContext);

static NFCSTATUS phNciNfc_CoreValidateNciCoreCmd(pphNciNfc_CoreTxInfo_t pTxInfo, uint8_t OidVal);
static NFCSTATUS phNciNfc_CoreValidateRfMgtCmd(pphNciNfc_CoreTxInfo_t pTxInfo, uint8_t OidVal);
static NFCSTATUS phNciNfc_CoreValidateNfceeMgtCmd(pphNciNfc_CoreTxInfo_t pTxInfo, uint8_t OidVal);
static NFCSTATUS phNciNfc_CoreValidateNciPropCmd(pphNciNfc_CoreTxInfo_t pTxInfo, uint8_t OidVal);

NFCSTATUS phNciNfc_CoreInitialise(pphNciNfc_CoreContext_t pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pContext)
    {
        pContext->bNciRelease = 0;

        wStatus = phNciNfc_LogConnMgmtInit();
        if(NFCSTATUS_SUCCESS != wStatus)
        {
            PH_LOG_NCI_CRIT_STR("Log Connection Mgmt Init Failed");
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            wStatus = phNciNfc_CoreRecvMgrInit(pContext);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("NCI Core Receive Init Failed");
            }
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            wStatus = phNciNfc_CoreInitRecverStateMachine(pContext);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("NCI Core Receive State Machine Init Failed");
                PH_LOG_NCI_INFO_STR("Releasing RecvMgr...");
                (void)phNciNfc_CoreRecvMgrRelease(pContext);
            }
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            wStatus = phNciNfc_CoreInitSenderStateMachine(pContext);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("NCI Core Sender State Machine Init Failed");
                PH_LOG_NCI_INFO_STR("Releasing RecvMgr...");
                (void)phNciNfc_CoreRecvMgrRelease(pContext);
                PH_LOG_NCI_INFO_STR("Recever State machine release...");
                (void)phNciNfc_CoreReleaseRecverStateMachine(pContext);
            }
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            wStatus = phNciNfc_CoreReceiveManager_Init(pContext);
            if(NFCSTATUS_PENDING != wStatus)
            {
                /* Memory allocation or TML read failed */
                PH_LOG_NCI_INFO_STR("Releasing RecvMgr...");
                (void)phNciNfc_CoreRecvMgrRelease(pContext);
                PH_LOG_NCI_INFO_STR("Recever State machine release...");
                (void)phNciNfc_CoreReleaseRecverStateMachine(pContext);
                PH_LOG_NCI_INFO_STR("Sender State machine release...");
                (void)phNciNfc_CoreReleaseSenderStateMachine(pContext);
            }
            else
            {
                wStatus = NFCSTATUS_SUCCESS;
                PH_LOG_NCI_INFO_STR("NCI Core Initialization success!");
            }
        }
    }
    else
    {
         PH_LOG_NCI_CRIT_STR("Invalid input parameter");
    }
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        wStatus = phNciNfc_CoreIfSetMaxCtrlPacketSize(pContext,
                                                         PHNCINFC_CORE_MIN_CTRL_PKT_PAYLOAD_LEN);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreRelease(pphNciNfc_CoreContext_t pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pContext)
    {
        /* Initialize notification handler */
        wStatus = phNciNfc_CoreRecvMgrRelease(pContext);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Notification manager release success");
             /* Abort TML read operation which is always kept open */
            wStatus  = phTmlNfc_ReadAbort(pContext->pHwRef);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Tml read abort failed!");
            }

            /* Abort TML write operation if in progress */
            wStatus  = phTmlNfc_WriteAbort(pContext->pHwRef);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Tml write abort failed!");
            }
            /* Since Core RecvMgr is already released, the return status of
               'phNciNfc_CoreReleaseSendRecvStateMachines' is not much of importance */
            phNciNfc_CoreReleaseSendRecvStateMachines((void *)pContext);
            /* Delete all nodes from the list and set list head to NULL */
            phNciNfc_CoreDeleteList(pContext);
            phNciNfc_LogConnMgmtDeInit();
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Ntf Handler release failed");
        }
    }
    else
    {
         PH_LOG_NCI_CRIT_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void phNciNfc_CleanCoreContext(pphNciNfc_CoreContext_t pContext)
{
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pContext)
    {
        phOsalNfc_SetMemory(&pContext->tRspCtx, 0x00, sizeof(phNciNfc_CoreRspRegContext_t));
        phOsalNfc_SetMemory(&pContext->tNtfCtx, 0x00, sizeof(phNciNfc_CoreNtfRegContext_t));

        /* Clear Nci Core and Context structures content */
        phOsalNfc_SetMemory(&pContext->SendStateContext,0,sizeof(phNciNfc_SendStateContext_t));
        phOsalNfc_SetMemory(&pContext->RecvStateContext,0,sizeof(phNciNfc_RecvStateContext_t));
        phOsalNfc_SetMemory(&pContext->tSendInfo,0,sizeof(phNciNfc_CoreSendInfo_t));
        phOsalNfc_SetMemory(&pContext->TxInfo,0,sizeof(phNciNfc_CoreTxInfo_t));

        pContext->IntNtf = NULL;
        pContext->pNtfContext = NULL;
        pContext->bMaxCtrlPktPayloadLen = 0;
        pContext->dwBytes_Remaining = 0;

        phOsalNfc_SetMemory(&pContext->tReceiveInfo.HeaderInfo,0,sizeof(phNciNfc_sCoreHeaderInfo_t));

        /* Delete linked list */
        phNciNfc_CoreDeleteList(pContext);
    }
    else
    {
         PH_LOG_NCI_CRIT_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

static void
phNciNfc_CoreReleaseSendRecvStateMachines(void *pContext)
{
    NFCSTATUS wStatus;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Release send state machine */
    wStatus = phNciNfc_CoreReleaseSenderStateMachine(pContext);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_NCI_CRIT_STR("Release Sender State Machine failed!");
    }
    /* Release receiver state machine */
    wStatus = phNciNfc_CoreReleaseRecverStateMachine(pContext);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_NCI_CRIT_STR("Release Recever State Machine failed!");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

/*Check the function declaration for details*/
static NFCSTATUS phNciNfc_CoreReceiveManager_Init(phNciNfc_CoreContext_t *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pContext)
    {
        /* This function creates a new node and does a tml read request */
        wStatus = phNciNfc_CoreRecv(pContext);
        if(NFCSTATUS_PENDING != wStatus)
        {
            PH_LOG_NCI_CRIT_STR("Tml Read request failed!");
            phNciNfc_CoreDeleteList(pContext);
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid Core context!");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreRecv(pphNciNfc_CoreContext_t pCoreCtx)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_sCoreRecvBuff_List_t pNode = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (pCoreCtx == phNciNfc_GetCoreContext()))
    {
        /* Add new node the linked list.
        This memory shall always be shared with TML for storing the received packet */
        pNode = phNciNfc_CoreGetNewNode(pCoreCtx);
        if(NULL != pNode)
        {
            /* Call TML_Read function and register the call back function */
            wStatus = phTmlNfc_Read( pCoreCtx->pHwRef,
                                     pNode->tMem.aBuffer,
                                     pNode->tMem.wLen,
                                     (pphTmlNfc_TransactCompletionCb_t)&phNciNfc_CoreRecvCb,
                                     (void *)pCoreCtx);
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Failed to add a new node to the list!");
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void phNciNfc_CoreRecvCb(void *pContext, phTmlNfc_TransactInfo_t *pInfo)
{
    NFCSTATUS           wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreContext_t *ptNciCoreCtx = (phNciNfc_CoreContext_t *)pContext;
    pphNciNfc_RecvStateContext_t pRecvStateContext = NULL;
    phNciNfc_EvtRecv_t TrigEvt = phNciNfc_EvtRecvNone;

    UNUSED(pRecvStateContext);
    UNUSED(TrigEvt);
    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL != ptNciCoreCtx) && (NULL != pInfo))
    {
        if(phNciNfc_GetCoreContext() == ptNciCoreCtx)
        {
            if (ptNciCoreCtx->bLogDataMessages)
            {
                PH_LOG_NCI_INFO_HEXDUMP("Received packet<-<- : %!HEXDUMP!",
                    WppLogHex((VOID*)pInfo->pBuff, (USHORT)pInfo->wLength));
            }
            else
            {
                PH_LOG_NCI_INFO_X32MSG("Received packet with length : ", pInfo->wLength);
            }

            pRecvStateContext = &(ptNciCoreCtx->RecvStateContext);
            wStatus = phNciNfc_CoreRecvConvertStatus2Evt(pInfo->wStatus, &TrigEvt);
        }else
        {
            PH_LOG_NCI_INFO_X32MSG("Received packet with length : ", pInfo->wLength);

            wStatus = NFCSTATUS_INVALID_PARAMETER;
            UNREFERENCED_PARAMETER(wStatus);
            goto Done;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        UNREFERENCED_PARAMETER(wStatus);
        goto Done;
    }

    if((NFCSTATUS_SUCCESS == wStatus)  && (NULL != pInfo) && (NULL != ptNciCoreCtx) && (ptNciCoreCtx == phNciNfc_GetCoreContext()))
    {
        /*Convert Status to */
        ptNciCoreCtx->pInfo.pBuff = pInfo->pBuff;
        ptNciCoreCtx->pInfo.wLength = pInfo->wLength;
        ptNciCoreCtx->pInfo.wStatus = pInfo->wStatus;
        ptNciCoreCtx->RecvStateContext.wProcessStatus = wStatus;

        /* phNciNfc_RecvStateHandler may free ptNciCoreCtx, ptNciCoreCtx->pInfo.pBuff and other data associated with ptNciCoreCtx */
        (void )phNciNfc_RecvStateHandler(ptNciCoreCtx, TrigEvt);
    }else
    {
        /*Drop Packet do Nothing, Invalid TrigEvt */
    }

    /* If Nci release is requested */
    if(NULL == phNciNfc_GetCoreContext())
    {
        PH_LOG_NCI_INFO_STR("NciCore context is being released!");;
    }
    else
    {
        /* Open a TML read request */
        wStatus = phNciNfc_CoreRecv(ptNciCoreCtx);
        if(NFCSTATUS_PENDING != wStatus)
        {
            PH_LOG_NCI_CRIT_STR("Tml Read request failed!");
        }
    }

Done:
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

/*Check Segmentation Handling in this function*/
NFCSTATUS phNciNfc_CoreBuildCmdPkt(pphNciNfc_CoreContext_t pContext, pphNciNfc_CoreTxInfo_t pTxInfo)
{
    NFCSTATUS                   wStatus = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_CoreGid_t          Gid;
    uint8_t                     OidVal;
    phNciNfc_NciCoreMsgType_t   MsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
    uint8_t                     MsgTypeVal = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if ((NULL != pTxInfo) && (NULL != pContext))
    {
        Gid = pTxInfo->tHeaderInfo.Group_ID;
        OidVal = (uint8_t)pTxInfo->tHeaderInfo.Opcode_ID.Val;

        /* Check whether GID & OID falls within range according to specification */
        switch (Gid)
        {
            case phNciNfc_e_CoreNciCoreGid:
                wStatus = phNciNfc_CoreValidateNciCoreCmd(pTxInfo,OidVal);
                break;
            case phNciNfc_e_CoreRfMgtGid:
                wStatus = phNciNfc_CoreValidateRfMgtCmd(pTxInfo,OidVal);
                break;
            case phNciNfc_e_CoreNfceeMgtGid:
                wStatus = phNciNfc_CoreValidateNfceeMgtCmd(pTxInfo,OidVal);
                break;
            case phNciNfc_e_CorePropGid:
                wStatus = phNciNfc_CoreValidateNciPropCmd(pTxInfo,OidVal);
                break;
            default:
                PH_LOG_NCI_WARN_STR("Build Comand packet: Invalid GID");
                break;
        }

        if (pContext->bMaxCtrlPktPayloadLen == 0)
        {
            wStatus = PH_NCINFC_INVALID_CNTRL_PAYLOAD_LENGTH;
            PH_LOG_NCI_CRIT_STR("Max Control Paload Length is not available !!!!!");
        }

        if (NFCSTATUS_SUCCESS == wStatus)
        {
            /*Prepare NCI Packet*/
            MsgTypeVal = (uint8_t)MsgType;
            PHNCINFC_CORE_SET_MT(pContext->tSendInfo.aSendPktBuff, MsgTypeVal);
            PHNCINFC_CORE_SET_GID(pContext->tSendInfo.aSendPktBuff, (uint8_t)Gid);
            PHNCINFC_CORE_SET_OID(pContext->tSendInfo.aSendPktBuff, OidVal);
            PHNCINFC_CORE_SET_LENBYTE(pContext->tSendInfo.aSendPktBuff, (uint8_t)pTxInfo->wLen);/*will be taken care*/
            if (pTxInfo->wLen > pContext->bMaxCtrlPktPayloadLen)
            {
                PHNCINFC_CORE_SET_PBF(pContext->tSendInfo.aSendPktBuff);
                /*Do needful to handle Segmentation*/
            }
            else
            {
                PHNCINFC_CORE_CLR_PBF(pContext->tSendInfo.aSendPktBuff);
            }

            if(pTxInfo->wLen > 0)
            {
                phOsalNfc_MemCopy(&(pContext->tSendInfo.aSendPktBuff[PHNCINFC_CORE_PAYLOAD_OFFSET]),
                    pTxInfo->Buff, pTxInfo->wLen);
            }

            /*Update pkt Info for Ref*/
            pContext->tSendInfo.dwSendlength = pTxInfo->wLen + PHNCINFC_CORE_PKT_HEADER_LEN;
            PH_LOG_NCI_INFO_STR("Send packet length: 0x%x",pContext->tSendInfo.dwSendlength);
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Build Comand packet failed!");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreSend(pphNciNfc_CoreContext_t pCtx)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        if (pCtx->bLogDataMessages)
        {
            PH_LOG_NCI_INFO_HEXDUMP("Sending packet->-> %!HEXDUMP!",
                WppLogHex((VOID*)pCtx->tSendInfo.aSendPktBuff, (USHORT)pCtx->tSendInfo.dwSendlength));
        }
        else
        {
            PH_LOG_NCI_INFO_X32MSG("Sending packet with length: ", pCtx->tSendInfo.dwSendlength);
        }

        wStatus = phTmlNfc_Write(pCtx->pHwRef, pCtx->tSendInfo.aSendPktBuff,
                                    (uint16_t)pCtx->tSendInfo.dwSendlength,
                                    (pphTmlNfc_TransactCompletionCb_t)&phNciNfc_CoreSendCb,
                                    pCtx);
    }
    else
    {
        PH_LOG_NCI_INFO_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
NFCSTATUS phNciNfc_CoreBuildDataPkt(pphNciNfc_CoreContext_t pContext, pphNciNfc_CoreTxInfo_t pTxInfo)
{
    NFCSTATUS                   wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t                     bConnId = 0xFF;
    phNciNfc_NciCoreMsgType_t   MsgType = phNciNfc_e_NciCoreMsgTypeData;
    uint8_t                     MsgTypeVal = 0;
    uint8_t                     Len = 0;
    uint8_t                     MaxPaylodLength = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pTxInfo) && (NULL != pContext))
    {
        bConnId = pTxInfo->tHeaderInfo.bConn_ID;
        wStatus = phNciNfc_GetConnMaxPldSz(bConnId, &MaxPaylodLength);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if((pTxInfo->wLen > 0) && (NULL != pTxInfo->Buff))
            {
                /* the starting position of send buffer.  This is needed when we are sending segmented buffers */
                uint8_t * txBufferSource = 0;
                /*Prepare NCI Packet*/
                phOsalNfc_SetMemory(pContext->tSendInfo.aSendPktBuff, 0x00, PHNCINFC_CORE_MAX_PKT_SIZE);

                MsgTypeVal = (uint8_t)MsgType;
                PHNCINFC_CORE_SET_MT(pContext->tSendInfo.aSendPktBuff, MsgTypeVal);
                PHNCINFC_CORE_SET_CONID(pContext->tSendInfo.aSendPktBuff, bConnId);

                if (pContext->dwBytes_Remaining > MaxPaylodLength)
                {
                    PHNCINFC_CORE_SET_PBF(pContext->tSendInfo.aSendPktBuff);
                    Len = MaxPaylodLength;
                    /*FIXME: Do needful to handle Segmentation*/
                }
                else
                {
                    PHNCINFC_CORE_CLR_PBF(pContext->tSendInfo.aSendPktBuff);
                    Len = (uint8_t)pContext->dwBytes_Remaining;
                }
                PHNCINFC_CORE_SET_LENBYTE(pContext->tSendInfo.aSendPktBuff, Len);

                txBufferSource = pTxInfo->Buff + (pTxInfo->wLen - pContext->dwBytes_Remaining);
                phOsalNfc_MemCopy(
                    &(pContext->tSendInfo.aSendPktBuff[PHNCINFC_CORE_PAYLOAD_OFFSET]),
                    txBufferSource,
                    Len);
                /*Update pkt Info for Ref*/
                pContext->tSendInfo.dwSendlength = Len + PHNCINFC_CORE_PKT_HEADER_LEN;
                PH_LOG_NCI_INFO_X32MSG("Sending packet with length: ", pContext->tSendInfo.dwSendlength);
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Invalid send payload buffer or length!");
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Data Max Payload Length is not available !!!!!");
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }

    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_CoreValidateNciPropCmd(pphNciNfc_CoreTxInfo_t pTxInfo,
                              uint8_t OidVal)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    switch(OidVal)
    {
        case phNciNfc_e_CorePropSetPwrModeCmdOid:
            if((PHNCINFC_CORE_NFCEEDISC_CMD_PAYLOADLEN_1x == pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_CorePropIsoDepPresChkCmdOid:
            if(0 == pTxInfo->wLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_CorePropDhListenFilterCmdOid:
            if(PHNCINFC_CORE_SETLISFILTER_CMD_PAYLOADLEN == pTxInfo->wLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_CorePropEnableExtnCmdOid:
            if(0 == pTxInfo->wLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_CorePropTestSwpCmdOid:
            if(PHNCINFC_CORE_TESTSWP_CMD_PAYLOADLEN == pTxInfo->wLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_CorePropTestPrbsCmdOid:
            if(PHNCINFC_CORE_TESTPRBS_CMD_PAYLOADLEN == pTxInfo->wLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_CorePropTestAntennaCmdOid:
            if(PHNCINFC_CORE_TESTANTENNA_CMD_PAYLOADMINLEN <= pTxInfo->wLen && PHNCINFC_CORE_TESTANTENNA_CMD_PAYLOADMAXLEN >= pTxInfo->wLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        default:
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_CoreValidateNfceeMgtCmd(pphNciNfc_CoreTxInfo_t pTxInfo,
                              uint8_t OidVal)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    switch(OidVal)
    {
        case phNciNfc_e_NfceeMgtNfceeDiscCmdOid:
            if(((PHNCINFC_CORE_NFCEEDISC_CMD_PAYLOADLEN_1x == pTxInfo->wLen) && (NULL != pTxInfo->Buff)) ||
               ((PHNCINFC_CORE_NFCEEDISC_CMD_PAYLOADLEN_2x == pTxInfo->wLen) && (NULL == pTxInfo->Buff)))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_NfceeMgtModeSetCmdOid:
            if((PHNCINFC_CORE_NFCEEMODESET_CMD_PAYLOADLEN == pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        default:
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_CoreValidateRfMgtCmd(pphNciNfc_CoreTxInfo_t pTxInfo,
                              uint8_t OidVal)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    switch(OidVal)
    {
        case phNciNfc_e_RfMgtRfDiscoverMapCmdOid:
            if((PHNCINFC_CORE_DISCMAP_CMD_PAYLOADLEN <= pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_RfMgtRfSetRoutingCmdOid:
            if((PHNCINFC_CORE_SETRTNG_CMD_PAYLOADLEN <= pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_RfMgtRfGetRoutingCmdOid:
            if(0 == pTxInfo->wLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_RfMgtRfDiscoverCmdOid:
            if((PHNCINFC_CORE_DISCOVER_CMD_PAYLOADLEN <= pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Discovery command payload empty!!");
            }
            break;
        case phNciNfc_e_RfMgtRfDiscSelectCmdOid:
            if((PHNCINFC_CORE_DISCSEL_CMD_PAYLOADLEN == pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_RfMgtRfDeactivateCmdOid:
            if((PHNCINFC_CORE_DEACTIVATE_CMD_PAYLOADLEN == pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_RfMgtRfT3tPollingCmdOid:
            if((PHNCINFC_CORE_T3TPOLL_CMD_PAYLOADLEN == pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_RfMgtRfParamUpdateCmdOid:
            if((PHNCINFC_CORE_PARAMUPDATE_CMD_PAYLOADLEN <= pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_RfMgtRfIsoDepPresChkCmdOid:
            if(0 == pTxInfo->wLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        default:
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_CoreValidateNciCoreCmd(pphNciNfc_CoreTxInfo_t pTxInfo,
                                uint8_t OidVal)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    switch(OidVal)
    {
        case phNciNfc_e_NciCoreResetCmdOid:
            if((PHNCINFC_CORE_RESET_CMD_PAYLOADLEN == pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_NciCoreInitCmdOid:
            if(0 == pTxInfo->wLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            else if( (pTxInfo->wLen > 0) && (NULL != pTxInfo->Buff) )
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_NciCoreSetConfigCmdOid:
            if((PHNCINFC_CORE_SETCONFIG_CMD_PAYLOADLEN <= pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_NciCoreGetConfigCmdOid:
            if((PHNCINFC_CORE_GETCONFIG_CMD_PAYLOADLEN <= pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_NciCoreConnCreateCmdOid:
            if(((PHNCINFC_CORE_DHCONN_CMD_PAYLOADLEN_NLB <= pTxInfo->wLen) ||
                (PHNCINFC_CORE_DHCONN_CMD_PAYLOADLEN_LB == pTxInfo->wLen)) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_NciCoreConnCloseCmdOid:
            if((PHNCINFC_CORE_CONNCLOSE_CMD_PAYLOADLEN <= pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        case phNciNfc_e_NciCoreSetPowerSubStateCmdOid:
            if((PHNCINFC_CORE_SETPOWERSUBSTATE_CMD_PAYLOADLEN <= pTxInfo->wLen) && (NULL != pTxInfo->Buff))
            {
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        default:
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void phNciNfc_CoreSendCb(void *pContext, phTmlNfc_TransactInfo_t *pInfo)
{
    pphNciNfc_CoreContext_t pCoreCtx = pContext;
    NFCSTATUS wStatus;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (phNciNfc_GetCoreContext() == pCoreCtx))
    {
        /*Check status*/
        if (NULL == pInfo)
        {
            PH_LOG_NCI_CRIT_STR("Invalid 'pInfo' from TML!");
        }
        else
        {
            PH_LOG_NCI_INFO_STR("Send status = %!NFCSTATUS!", pInfo->wStatus);
            wStatus = PHNFCSTATUS(pInfo->wStatus);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_NCI_CRIT_STR("Tml write error!");
                /* Incase of error while sending command, notify upper layer about the
                   same by calling its call back function. This is applicable for both TxRx and TxOnly cases */
                pCoreCtx->IntNtfFlag = 1;
            }

            /* Update the Tml write error */
            pCoreCtx->TxInfo.wWriteStatus = wStatus;
            (void)phNciNfc_StateHandler(pContext, phNciNfc_EvtSendComplete);
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid Core context passed from TML!");
    }
    PH_LOG_NCI_FUNC_EXIT();
}

