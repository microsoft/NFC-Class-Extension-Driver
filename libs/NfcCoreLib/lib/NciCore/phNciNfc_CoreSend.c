/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h"

#include "phNciNfc_CoreSend.tmh"

#define PHNCINFC_WAITCREDIT_TO_REDUCTION    (50)

typedef NFCSTATUS (*phNciNfc_CoreSend_t)(phNciNfc_CoreContext_t *pContext);

static uint8_t phNciNfc_CoreSendChkPktType(void *pContext);
static uint8_t phNciNfc_CoreSendChkCreditAvail(void *pContext);
static uint8_t phNciNfc_CoreSendChkSize(void *pContext);

static NFCSTATUS phNciNfc_CoreSendIdleEntry(void *pContext);
static NFCSTATUS phNciNfc_CoreSendIdleExit(void *pContext);
static NFCSTATUS phNciNfc_CoreSendEntry(void *pContext);
static NFCSTATUS phNciNfc_CoreSendExit(void *pContext);

static NFCSTATUS phNciNfc_CoreWaitCreditEntry(void *pContext);
static NFCSTATUS phNciNfc_CoreWaitCreditExit(void *pContext);


static NFCSTATUS phNciNfc_StateIdle2Send(void *pContext);
static NFCSTATUS phNciNfc_StateIdle2WaitCredit(void *pContext);

static NFCSTATUS phNciNfc_StateSend2Idle(void *pContext);
static NFCSTATUS phNciNfc_StateSend2WaitCredit(void *pContext);
static NFCSTATUS phNciNfc_StateWaitCredit2Send(void *pContext);
static NFCSTATUS phNciNfc_StateWaitCredit2Idle(void *pContext);

static void phNciNfc_SendEvt2Status(phNciNfc_EvtSend_t Evt, pphNciNfc_CoreContext_t pCtx, NFCSTATUS *Status);
static NFCSTATUS phNciNfc_UnRegCallBack(void *pContext);

static void phNciNfc_CreditsUpdateCB(void* pContext, uint8_t bCredits, NFCSTATUS wStatus);

/**< CoreSend: State to state Connectors for Send state machine*/
static phNciNfc_Connector_t phNciNfc_SendConnector[(phNciNfc_CONN_SEND_MAX - phNciNfc_STATE_SEND_MAX - 1)] =
{
    &phNciNfc_CoreSendChkPktType,
    &phNciNfc_CoreSendChkCreditAvail,
    &phNciNfc_CoreSendChkSize
};
/**< CoreSend: State Entry and Exit functions*/
static phNciNfc_StateFunction_t phNciNfc_StateFptr[phNciNfc_STATE_SEND_MAX] = {
    {NULL, &phNciNfc_CoreSendIdleExit},
    {&phNciNfc_CoreSendEntry, &phNciNfc_CoreSendExit},
    {&phNciNfc_CoreWaitCreditEntry, &phNciNfc_CoreWaitCreditExit}
};
/**< CoreSend: State to next state mapping*/
static phNciNfc_StateSend_t phNciNfc_SendState2State[phNciNfc_STATE_SEND_MAX][(phNciNfc_CONN_SEND_MAX - phNciNfc_STATE_SEND_MAX -1)*2] =
{
                                    /*phNciNfc_ConnChkPktType*/                     /*phNciNfc_ConnCheckCredit*/                        /*phNciNfc_ConnChkSize*/
    /*States*/                  /*TRUE*/                 /*FALSE*/               /*TRUE*/                    /*FALSE*/               /*TRUE*/                /*FALSE*/
    /*phNciNfc_SendIdle*/       {phNciNfc_StateSend,     phNciNfc_ConnChkCredit, phNciNfc_StateWaitCredit,   phNciNfc_StateSend,     phNciNfc_StateSendIdle, phNciNfc_StateSendNone},
    /*phNciNfc_Send*/           {phNciNfc_ConnChkCredit, phNciNfc_ConnChkCredit, phNciNfc_StateWaitCredit,   phNciNfc_StateSend,     phNciNfc_StateSendIdle, phNciNfc_ConnChkPktType},
    /*phNciNfc_SendWaitCredit*/ {phNciNfc_ConnChkCredit, phNciNfc_StateSendIdle, phNciNfc_StateSendNone,     phNciNfc_StateSendNone, phNciNfc_StateSendNone, phNciNfc_StateSendNone}

};
/**< CoreSend: State to event mapping */
static phNciNfc_StateSend_t phNciNfc_SendState2Event[phNciNfc_STATE_SEND_MAX][phNciNfc_EVT_SEND_MAX] =
{
    /*States*/                  /*sendPkt*/                 /*SendComplete*/        /*CreditAvail*/         /*phNciNfc_EvtWaitCreditTimeOut*/
    /*phNciNfc_SendIdle*/      {phNciNfc_ConnChkPktType,    phNciNfc_StateSendNone, phNciNfc_StateSendNone, phNciNfc_StateSendNone},
    /*phNciNfc_Send*/          {phNciNfc_StateSendNone,     phNciNfc_ConnChkSize,   phNciNfc_StateSendNone, phNciNfc_StateSendNone},
    /*phNciNfc_SendWaitCredit*/ {phNciNfc_StateSendNone,    phNciNfc_StateSendNone, phNciNfc_StateSend,     phNciNfc_StateSendIdle},

};
/**< CoreSend: State to Next State transition function*/
static phNciNfc_StateTransition_t phNciNfc_SendTransition[phNciNfc_STATE_SEND_MAX][phNciNfc_STATE_SEND_MAX] = {

    /*States*/
    /*States*/                 /*phNciNfc_SendIdle*/            /*phNciNfc_Send*/                /*phNciNfc_SendWaitCredit*/
    /*phNciNfc_SendIdle*/      {NULL,                           &phNciNfc_StateIdle2Send,        &phNciNfc_StateIdle2WaitCredit},
    /*phNciNfc_Send*/          {&phNciNfc_StateSend2Idle,       NULL,                            &phNciNfc_StateSend2WaitCredit},
    /*phNciNfc_SendWaitCredit*/{&phNciNfc_StateWaitCredit2Idle, &phNciNfc_StateWaitCredit2Send,  NULL}
};

NFCSTATUS phNciNfc_CoreInitSenderStateMachine(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = (pphNciNfc_CoreContext_t)pContext;
    pphNciNfc_SendStateContext_t pStateCtx = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pCtx)
    {
        pStateCtx = &(pCtx->SendStateContext);
        pStateCtx->pfConnector = phNciNfc_SendConnector;
        pStateCtx->pfStateFunction = phNciNfc_StateFptr;
        pStateCtx->pStateTransition = &(phNciNfc_SendTransition[0][0]);
        pStateCtx->CurrState = phNciNfc_StateSendIdle;
        pStateCtx->Evt = phNciNfc_EvtNone;
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_NCI_INFO_STR("phNciNfc_CoreInitSenderStateMachine: SUCCESS");
    }else
    {
        PH_LOG_NCI_INFO_STR("phNciNfc_CoreInitSenderStateMachine: FAILED");
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS phNciNfc_CoreResetSenderStateMachine(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = (pphNciNfc_CoreContext_t)pContext;
    pphNciNfc_SendStateContext_t pStateCtx = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pCtx)
    {
        pStateCtx = &(pCtx->SendStateContext);
        pStateCtx->pfConnector = phNciNfc_SendConnector;
        pStateCtx->pfStateFunction = phNciNfc_StateFptr;
        pStateCtx->pStateTransition = &(phNciNfc_SendTransition[0][0]);
        pStateCtx->CurrState = phNciNfc_StateSendIdle;
        pStateCtx->Evt = phNciNfc_EvtNone;
        PH_LOG_NCI_INFO_STR("phNciNfc_CoreResetSenderStateMachine: SUCCESS");
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreReleaseSenderStateMachine(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = (pphNciNfc_CoreContext_t)pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void phNciNfc_CoreResetSendStateMachine(void *pContext)
{
    pphNciNfc_CoreContext_t pCoreCtx = (pphNciNfc_CoreContext_t)pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (pCoreCtx == phNciNfc_GetCoreContext()))
    {
        /* Reset number of bytes to be sent */
        pCoreCtx->dwBytes_Remaining = 0;
        pCoreCtx->dwRspTimeOutMs = 0;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

#define STATE_IS_CONNECTOR(STATE)  \
    ((STATE) > phNciNfc_STATE_SEND_MAX && (STATE)< phNciNfc_CONN_SEND_MAX)

/*Before calling the State Engine caller must ensure that events is in the difined range*/

NFCSTATUS phNciNfc_StateHandler(void* pContext, phNciNfc_EvtSend_t CurrEvent)
{
    phNciNfc_StateSend_t TrgtState = phNciNfc_StateSendNone;
    phNciNfc_StateSend_t CurrState = phNciNfc_StateSendNone;
    pphNciNfc_CoreContext_t pCtx = pContext;
    pphNciNfc_SendStateContext_t pStateCtx = NULL;
    uint16_t Result = 0;
    uint8_t Index = 0;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL == pCtx) || (pCtx != phNciNfc_GetCoreContext()))
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameter!");
        Result = NFCSTATUS_INVALID_PARAMETER;
        goto Done;
    }

    pStateCtx = &(pCtx->SendStateContext);
    if (NULL != pStateCtx)
    {
        CurrState = pStateCtx->CurrState;
    }

    PH_LOG_NCI_INFO_STR("Current State %!phNciNfc_StateSend_t!",CurrState);
    PH_LOG_NCI_INFO_STR("Current Event %!phNciNfc_EvtSend_t!",CurrEvent);
    /*Get next state/connector for the event*/
    if((CurrState < phNciNfc_StateSendIdle) ||
       (CurrState >= phNciNfc_STATE_SEND_MAX) ||
       (CurrEvent < phNciNfc_EvtSendPacket) ||
       (CurrEvent >= phNciNfc_EVT_SEND_MAX))
    {
        PH_LOG_NCI_CRIT_STR("Unknown Current State or Event ");
        Result = NFCSTATUS_INVALID_STATE;
        goto Done;
    }

    TrgtState = phNciNfc_SendState2Event[CurrState][CurrEvent];
    /* Update current event in SendContext */
    pStateCtx->Evt = CurrEvent;
    PH_LOG_NCI_INFO_STR("Target State %!phNciNfc_StateSend_t!",TrgtState);
    do
    {
        if (STATE_IS_CONNECTOR(TrgtState))
        {
            /*Get Connetor Index*/
            Index = (uint8_t)(TrgtState - phNciNfc_STATE_SEND_MAX - 1);
            /*State = Connector*/
            Result = pStateCtx->pfConnector[Index](pCtx);
            if (0 == Result)
            {
                TrgtState = phNciNfc_SendState2State[CurrState][(uint8_t)(Index * 2)];
            }else
            {
                TrgtState = phNciNfc_SendState2State[CurrState][((uint8_t)Index * 2) + 1];
            }
            PH_LOG_NCI_INFO_STR("Target State after connector, %!phNciNfc_StateSend_t!",TrgtState);

        }else if (TrgtState >= phNciNfc_StateSendNone)
        {
            /*No Action to be taken Event ocuured can not be handled in this state*/
            /*set to invalid state*/
            PH_LOG_NCI_CRIT_STR("Send State Machine: Invalid Target State");
            Result = NFCSTATUS_FAILED;
            break;
        }else
        {
            /*Debug Print*/
            PH_LOG_NCI_INFO_STR("Continue... ");
        }
    }while(TrgtState >= phNciNfc_STATE_SEND_MAX);

    if ((TrgtState < phNciNfc_STATE_SEND_MAX))// && (CurrState < phNciNfc_STATE_SEND_MAX))
    {
        if(NULL != pStateCtx->pfStateFunction[CurrState].pfExit)
        {
            Result = pStateCtx->pfStateFunction[CurrState].pfExit(pContext);
        }
        if (CurrState != TrgtState)
        {
            if (NULL != phNciNfc_SendTransition[CurrState][TrgtState])
            {
                PH_LOG_NCI_INFO_STR( "CurrState = %!phNciNfc_StateSend_t!", CurrState);
                PH_LOG_NCI_INFO_STR( "TrgtState = %!phNciNfc_StateSend_t!", TrgtState);
                pStateCtx->CurrState = TrgtState;
                Result = phNciNfc_SendTransition[CurrState][TrgtState](pContext);
            }
        }

        /*Entry of next state*/
        if((NULL != phNciNfc_GetCoreContext()) && (NULL != pStateCtx->pfStateFunction[TrgtState].pfEntry))
        {
            Result = pStateCtx->pfStateFunction[TrgtState].pfEntry(pContext);
            /* Check if send is successful */
            if((NFCSTATUS_PENDING != Result) && (phNciNfc_StateSendIdle == CurrState) &&
                (phNciNfc_StateSend == TrgtState))
            {
                /* Unregister call back function */
                (void) phNciNfc_UnRegCallBack(pContext);
            }
        }
    }

Done:

    PH_LOG_NCI_FUNC_EXIT();
    return Result;
}

static NFCSTATUS phNciNfc_CoreSendIdleEntry(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CoreSendIdleExit(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        phOsalNfc_MemCopy(&(pCtx->TxInfo), pCtx->tTemp.pTxInfo, sizeof(phNciNfc_CoreTxInfo_t));
        pCtx->IntNtf = pCtx->tTemp.NciCb; /*Needs to register with Response Manager*/
        if(1 == pCtx->bCoreTxOnly)
        {
            pCtx->IntNtfFlag = 1; /*Notify upper layer after sending command about SEND status*/
        }else
        {
            pCtx->IntNtfFlag = 0; /*Notify upper layer once response is received for the sent command or
                                    sending command fails*/
        }
        pCtx->dwBytes_Remaining = pCtx->tTemp.pTxInfo->wLen; /*Update remaing Bytes to send*/
        pCtx->pNtfContext = pCtx->tTemp.pContext;
        pCtx->dwRspTimeOutMs = pCtx->tTemp.dwTimeOutMs;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}


static NFCSTATUS phNciNfc_CoreSendEntry(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        if (phNciNfc_e_NciCoreMsgTypeData == pCtx->TxInfo.tHeaderInfo.eMsgType)
        {
            /* Build Data packet */
             wStatus = phNciNfc_CoreBuildDataPkt(pCtx, &(pCtx->TxInfo));
             PH_LOG_NCI_INFO_STR("Build Data packet returns = %!NFCSTATUS!",wStatus);
             /* disabling credit decrement for now ,since wait credit logic is not done yet!! */

             if(NFCSTATUS_SUCCESS == wStatus)
             {
                 /* decrement credits here before sending data packet */
                 wStatus = phNciNfc_DecrConnCredit(pCtx->TxInfo.tHeaderInfo.bConn_ID);
             }
        }else if (phNciNfc_e_NciCoreMsgTypeCntrlCmd == pCtx->TxInfo.tHeaderInfo.eMsgType)
        {
            /* Build Command packet */
            wStatus = phNciNfc_CoreBuildCmdPkt(pCtx, &(pCtx->TxInfo));
            PH_LOG_NCI_INFO_STR("Build Control packet returns = %!NFCSTATUS!",wStatus);
        }else
        {
            PH_LOG_NCI_WARN_STR("Build Comand packet: = %!NFCSTATUS!", NFCSTATUS_INVALID_PARAMETER);
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Writing to TML */
            wStatus = phNciNfc_CoreSend(pCtx);
            if(NFCSTATUS_PENDING == wStatus)
            {
                /* Calculate remaining number of bytes to be sent */
                pCtx->dwBytes_Remaining = pCtx->dwBytes_Remaining - (pCtx->tSendInfo.dwSendlength - PHNCINFC_CORE_PKT_HEADER_LEN);
            }
        }else
        {
            PH_LOG_NCI_INFO_STR("phNciNfc_CoreSendEntry: Build Packet Failed");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}


static NFCSTATUS phNciNfc_CoreSendExit(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
    }else
    {
       wStatus = NFCSTATUS_SUCCESS;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CoreWaitCreditEntry(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_PENDING;
    PH_LOG_NCI_FUNC_ENTRY();
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CoreWaitCreditExit(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

/*Send State Connectors*/

static uint8_t phNciNfc_CoreSendChkPktType(void *pContext)
{
    uint8_t Ret = 0;
    pphNciNfc_CoreContext_t pCtx = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        if (phNciNfc_e_NciCoreMsgTypeData == pCtx->TxInfo.tHeaderInfo.eMsgType)
        {
            Ret = 1;
        }else if (phNciNfc_e_NciCoreMsgTypeCntrlCmd == pCtx->TxInfo.tHeaderInfo.eMsgType)
        {
            Ret = 0;
        }else
        {
            Ret = 0xFF;
        }
    }else
    {
        Ret = 0xFF;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return Ret;
}
/*Check the ramaining size*/
static uint8_t phNciNfc_CoreSendChkSize(void *pContext)
{
    uint8_t Ret = 0;
    pphNciNfc_CoreContext_t pCtx = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        if (pCtx->dwBytes_Remaining == 0)
        {
            Ret = 0;
        }else
        {
            Ret = 1;
        }
    }else
    {
        Ret = 0xFF;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return Ret;
}

static uint8_t phNciNfc_CoreSendChkCreditAvail(void *pContext)
{
    uint8_t Ret = 0;
    uint8_t Val = 0;
    uint32_t CreditTo = PHNCINFC_MIN_WAITCREDIT_TO;
    pphNciNfc_CoreContext_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        wStatus = phNciNfc_GetConnCredits(pCtx->TxInfo.tHeaderInfo.bConn_ID, &Val);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if(Val > 0)
            {
                Ret = 1;
            }else
            {
                /* Calculate the credit timeout for the command */
                if(pCtx->tTemp.dwTimeOutMs > PHNCINFC_MIN_WAITCREDIT_TO)
                {
                    CreditTo = pCtx->tTemp.dwTimeOutMs - PHNCINFC_WAITCREDIT_TO_REDUCTION;
                }

                /* Register for credits availability with LogConn Module */
                wStatus = phNciNfc_RegForConnCredits(pCtx->TxInfo.tHeaderInfo.bConn_ID,
                                                     &phNciNfc_CreditsUpdateCB, pContext, CreditTo);
                if(NFCSTATUS_SUCCESS != wStatus)
                {
                    PH_LOG_NCI_CRIT_STR("Conn Credits Registration failed!!");
                }

                Ret = 0;
            }
        }else
        {
            Ret = 0;
            PH_LOG_NCI_WARN_X32MSG("Get Credit Avail Failed = %d", wStatus);
        }
    }else
    {
        PH_LOG_NCI_WARN_STR("Invalid Parameter");
        Ret = 0xFF;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return Ret;
}


static NFCSTATUS phNciNfc_CoreRegisterForNotifications(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo;
    phNciNfc_NciCoreMsgType_t eMsgType = phNciNfc_e_NciCoreMsgTypeInvalid;
    uint8_t bMaxPayloadLength = 0;
    uint16_t wTimeOutMultiplier = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        /* Incase of TxRx, register call back function which shall be invoked when
           a response is received */
        if(pCtx->IntNtfFlag == 0)
        {
            eMsgType = pCtx->TxInfo.tHeaderInfo.eMsgType;
            if(phNciNfc_e_NciCoreMsgTypeCntrlCmd == eMsgType)
            {
                eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlRsp;
            }
            else if(phNciNfc_e_NciCoreMsgTypeData == eMsgType)
            {
                eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            }
            else
            {
                /* Critical error: No message shall be sent except Command and Data */
            }
            /*Register with Response Manager and start timer*/
            phOsalNfc_MemCopy(&tHeaderInfo,
                                &(pCtx->TxInfo.tHeaderInfo),
                                sizeof(phNciNfc_sCoreHeaderInfo_t));
            tHeaderInfo.eMsgType = eMsgType;
            wStatus = phNciNfc_CoreIfRegRspNtf(pCtx,
                                                &(tHeaderInfo),
                                                pCtx->IntNtf,
                                                pCtx->pNtfContext
                                                );
            if((NFCSTATUS_SUCCESS == wStatus) && (0 != pCtx->TimerInfo.dwRspTimerId))
            {
                /*Start Timer*/
                if(pCtx->dwRspTimeOutMs > 0)
                {
                    /* Calculate timer time out value */
                    if(phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType)
                    {
                        bMaxPayloadLength = pCtx->bMaxCtrlPktPayloadLen;
                    }
                    else if(phNciNfc_e_NciCoreMsgTypeData == eMsgType)
                    {
                        wStatus = phNciNfc_GetConnMaxPldSz(tHeaderInfo.bConn_ID,
                                                           &bMaxPayloadLength);
                    }
                    else
                    {
                        /* Dont modify time out value as we are trying to send an invalid packet */
                        wStatus = NFCSTATUS_INVALID_PARAMETER;
                    }

                    if(0 != bMaxPayloadLength)
                    {
                        if((NFCSTATUS_SUCCESS == wStatus) && (pCtx->TxInfo.wLen > bMaxPayloadLength))
                        {
                            wTimeOutMultiplier = (pCtx->TxInfo.wLen / bMaxPayloadLength);
                            pCtx->dwRspTimeOutMs += (PHNCINFC_CORESEND_TIMEOUT_MULTIPLIER * wTimeOutMultiplier);
                        }
                    }

                    wStatus = phOsalNfc_Timer_Start(pCtx->TimerInfo.dwRspTimerId,
                                                    pCtx->dwRspTimeOutMs,
                                                    &phNciNfc_RspTimeOutCb,
                                                    pCtx);
                    if (NFCSTATUS_SUCCESS == wStatus)
                    {
                        PH_LOG_NCI_INFO_STR("Response timer started");
                        phOsalNfc_MemCopy(&(pCtx->TimerInfo.PktHeaderInfo),
                                        &(pCtx->TxInfo.tHeaderInfo),
                                        sizeof(phNciNfc_sCoreHeaderInfo_t));
                        pCtx->TimerInfo.TimerStatus = 1;
                        pCtx->TimerInfo.PktHeaderInfo.eMsgType = eMsgType;
                    }else
                    {
                        PH_LOG_NCI_INFO_STR("Response timer start failed");
                    }
                }else
                {
                    PH_LOG_NCI_INFO_STR("Response Time out is not managed for this request");
                }
            }else
            {
                PH_LOG_NCI_INFO_STR("No Timer for responses !!!");
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}


static NFCSTATUS phNciNfc_StateIdle2Send(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();

    wStatus = phNciNfc_CoreRegisterForNotifications(pContext);

    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
static NFCSTATUS phNciNfc_StateIdle2WaitCredit(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_PENDING;
    PH_LOG_NCI_FUNC_ENTRY();
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_StateSend2Idle(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        /* Incase of TxRx, if sending fails upper layer call back shall be invoked  */
        /* Incase of TxOnly, after send complete or if sending fails upper layer call back shall be invoked  */
        if(pCtx->IntNtfFlag == 1)
        {
            /*invoke Cb with error status*/
            phNciNfc_SendEvt2Status(pCtx->SendStateContext.Evt,pCtx,&wStatus);
            if (NULL != pCtx->IntNtf)
            {
                pCtx->IntNtf(pCtx->pNtfContext, NULL, wStatus);
            }
        }
    }else
    {
        /*Wrong Context*/
        PH_LOG_NCI_CRIT_STR("SendStateMachine - Send2Idle - Wrong context passed");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_UnRegCallBack(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo;
    phNciNfc_NciCoreMsgType_t eMsgType;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        /* Only incase of TxRx, call back function would be registered */
        if(pCtx->IntNtfFlag == 0)
        {
            eMsgType = pCtx->TxInfo.tHeaderInfo.eMsgType;
            if(phNciNfc_e_NciCoreMsgTypeCntrlCmd == eMsgType)
            {
                eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlRsp;
            }
            else if(phNciNfc_e_NciCoreMsgTypeData == eMsgType)
            {
                eMsgType = phNciNfc_e_NciCoreMsgTypeData;
            }

            /*Un-Register with Response Manager and start timer*/
            phOsalNfc_MemCopy(&tHeaderInfo,
                                &(pCtx->TxInfo.tHeaderInfo),
                                sizeof(phNciNfc_sCoreHeaderInfo_t));
            tHeaderInfo.eMsgType = eMsgType;
            wStatus = phNciNfc_CoreIfUnRegRspNtf(pCtx,
                                                 &(tHeaderInfo),
                                                 pCtx->IntNtf);
            /* Stop the timer if running */
            if((NFCSTATUS_SUCCESS == wStatus) && (1 == pCtx->TimerInfo.TimerStatus))
            {
                /*Stop Timer*/
                (void)phOsalNfc_Timer_Stop(pCtx->TimerInfo.dwRspTimerId);
                pCtx->TimerInfo.TimerStatus = 0;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }else
    {
        /*Wrong Context*/
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_StateSend2WaitCredit(void *pContext)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_ENTRY();

    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_StateWaitCredit2Send(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    
    wStatus = phNciNfc_CoreRegisterForNotifications(pContext);
    
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
static NFCSTATUS phNciNfc_StateWaitCredit2Idle(void *pContext)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        if(NULL != pCtx->IntNtf)
        {
            PH_LOG_NCI_CRIT_STR("Send Credits not received, invoking call back with NFCSTATUS_CREDIT_TIMEOUT");
            pCtx->IntNtf(pCtx->pNtfContext, NULL, NFCSTATUS_CREDIT_TIMEOUT);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void phNciNfc_SendEvt2Status(phNciNfc_EvtSend_t Evt, pphNciNfc_CoreContext_t pCtx,
                                    NFCSTATUS *Status)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    switch(Evt)
    {
        case phNciNfc_EvtSendComplete:
        {
            /* During 'TxOnly',upper layer shall always be invoked after
               1. Send successfully completed
               2. Send fails (NFCSTATUS_BOARD_COMMUNICATION_ERROR or NFCSTATUS_FAILED)
               During 'TxRx',upper layer shall be invoked after
               1. a response for the send command is received
               2. Send fails (NFCSTATUS_BOARD_COMMUNICATION_ERROR or NFCSTATUS_FAILED)
              */
            /* Get the Tml send status */
            wStatus = pCtx->TxInfo.wWriteStatus;
        }
        break;
        case phNciNfc_EvtWaitCreditTimeOut:
        {
            wStatus = NFCSTATUS_CREDIT_TIMEOUT;
        }
        break;
        default:
        {
            wStatus = NFCSTATUS_UNKNOWN_ERROR;
        }
        break;
    }
    *Status = wStatus;
    PH_LOG_NCI_FUNC_EXIT();
}

static
void
phNciNfc_CreditsUpdateCB(void* pContext, uint8_t bCredits, NFCSTATUS wStatus)
{
    pphNciNfc_CoreContext_t pCoreCtxt = pContext;
    phNciNfc_EvtSend_t tTrigEvent = phNciNfc_EvtNone;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL != pCoreCtxt) && (phNciNfc_GetCoreContext() == pCoreCtxt))
    {
        if((NFCSTATUS_SUCCESS != wStatus) || (0 == bCredits))
        {
            /* setting generic WaitCreditTimeout event to enable processing of state machine even in failure case */
            tTrigEvent = phNciNfc_EvtWaitCreditTimeOut;
            PH_LOG_NCI_CRIT_STR("Credits Update from LogConn Module Failed");
        }
        else
        {
            tTrigEvent = phNciNfc_EvtCreditAvail;
        }
    }
    else
    {
        /* setting generic WaitCreditTimeout event to enable processing of state machine even in failure case */
        tTrigEvent = phNciNfc_EvtWaitCreditTimeOut;
        PH_LOG_NCI_CRIT_STR("Invalid Core context passed from LogConn Module!");
    }

    (void)phNciNfc_StateHandler(pContext,tTrigEvent);

    PH_LOG_NCI_FUNC_EXIT();
}
