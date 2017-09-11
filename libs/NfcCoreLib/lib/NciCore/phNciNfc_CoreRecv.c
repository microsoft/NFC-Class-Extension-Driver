/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h"

#include "phNciNfc_CoreRecv.tmh"

static uint8_t phNciNfc_CoreRecvChkPktType(void *pContext);
static uint8_t phNciNfc_CoreRecvChkPbfAndUpdate(void *pContext);
static uint8_t phNciNfc_CoreRecvChkExptdPktType(void *pContext);

static NFCSTATUS phNciNfc_CoreRecvIdleEntry(void *pContext);
static NFCSTATUS phNciNfc_CoreRecvIdleExit(void *pContext);
static NFCSTATUS phNciNfc_CoreRecvEntry(void *pContext);
static NFCSTATUS phNciNfc_CoreRecvExit(void *pContext);
static NFCSTATUS phNciNfc_CoreRecvDummyEntry(void *pContext);
static NFCSTATUS phNciNfc_CoreRecvDummyExit(void *pContext);

static NFCSTATUS phNciNfc_StateIdle2Recv(void *pContext);
static NFCSTATUS phNciNfc_StateIdle2Dummy(void *pContext);
static NFCSTATUS phNciNfc_StateRecv2Dummy(void *pContext);
static NFCSTATUS phNciNfc_StateDummy2Idle(void *pContext);

/**< CoreRecv: State to state Connectors for Recv state machine*/
static phNciNfc_Connector_t phNciNfc_RecvConnector[(phNciNfc_CONN_RECV_MAX - phNciNfc_STATE_RECV_MAX - 1)] =
{
    &phNciNfc_CoreRecvChkPktType,
    &phNciNfc_CoreRecvChkPbfAndUpdate,
    &phNciNfc_CoreRecvChkExptdPktType
};
/**< CoreRecv: State Entry and Exit functions*/
static phNciNfc_StateFunction_t phNciNfc_RecvStateFptr[phNciNfc_STATE_RECV_MAX] = {
    {NULL,NULL},//{&phNciNfc_CoreRecvIdleEntry, &phNciNfc_CoreRecvIdleExit},
    {NULL,NULL},//{&phNciNfc_CoreRecvEntry, &phNciNfc_CoreRecvExit},
    {NULL,NULL},//{&phNciNfc_CoreRecvDummyEntry, &phNciNfc_CoreRecvDummyExit}
};
/**< CoreRecv: State to next state or Connector mapping*/
static phNciNfc_StateRecv_t phNciNfc_RecvState2State[phNciNfc_STATE_RECV_MAX][(phNciNfc_CONN_RECV_MAX - phNciNfc_STATE_RECV_MAX -1)*2] =
{
                                    /*phNciNfc_ConnChkPktType*/                         /*phNciNfc_ConnChkPbf*/                     /*phNciNfc_ConnChkExptdPktType*/
    /*States*/                  /*TRUE*/                 /*FALSE*/              /*TRUE*/                 /*FALSE*/                  /*TRUE*/                 /*FALSE*/
    /*phNciNfc_RecvIdle*/       {phNciNfc_ConnChkPbf, phNciNfc_StateRecvIdle, phNciNfc_StateRecvDummy,   phNciNfc_StateRecv, phNciNfc_StateRecvNone, phNciNfc_StateRecvNone},
    /*phNciNfc_Recv*/           {phNciNfc_ConnExptdPktType, phNciNfc_StateRecv, phNciNfc_StateRecvDummy,   phNciNfc_StateRecv, phNciNfc_ConnChkPbf, phNciNfc_StateRecvIdle},
    /*phNciNfc_Recvdummy*/      {phNciNfc_StateRecvNone, phNciNfc_StateRecvNone, phNciNfc_StateRecvNone,     phNciNfc_StateRecvNone, phNciNfc_StateRecvNone, phNciNfc_StateRecvNone}
};
/**< CoreRecv: State to event mapping */
static phNciNfc_StateRecv_t phNciNfc_RecvState2Event[phNciNfc_STATE_RECV_MAX][phNciNfc_EVT_RECV_MAX] =
{
    /*States*/                 /*phNciNfc_EvtRecvPacket*/ /*phNciNfc_EvtRecvComplete*/    /*phNciNfc_EvtRecvTimeOut*/ /*phNciNfc_EvtRecvHwError*/
    /*phNciNfc_RecvIdle*/      {phNciNfc_ConnRecvChkPktType, phNciNfc_StateRecvNone,phNciNfc_StateRecvDummy, phNciNfc_StateRecvDummy},
    /*phNciNfc_Recv*/          {phNciNfc_ConnRecvChkPktType, phNciNfc_StateRecvNone, phNciNfc_StateRecvDummy, phNciNfc_StateRecvDummy},
    /*phNciNfc_Recvdummy*/     {phNciNfc_StateRecvNone, phNciNfc_StateRecvIdle,  phNciNfc_StateRecvNone, phNciNfc_StateRecvNone},
};
/**< CoreRecv: State to Next State transition function*/
static phNciNfc_StateTransition_t phNciNfc_RecvTransition[phNciNfc_STATE_RECV_MAX][phNciNfc_STATE_RECV_MAX] = {
    /*States*/                  /*phNciNfc_RecvIdle*/  /*phNciNfc_Recv*/         /*phNciNfc_Recvdummy*/
    /*phNciNfc_RecvIdle*/       {NULL,                   &phNciNfc_StateIdle2Recv,  &phNciNfc_StateIdle2Dummy},
    /*phNciNfc_Recv*/           {NULL,                   NULL,                      &phNciNfc_StateRecv2Dummy},
    /*phNciNfc_Recvdummy*/      {&phNciNfc_StateDummy2Idle, NULL,                   NULL}
};

NFCSTATUS phNciNfc_CoreInitRecverStateMachine(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = (pphNciNfc_CoreContext_t)pContext;
    pphNciNfc_RecvStateContext_t pStateCtx = NULL;
    uint32_t TimerId;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pCtx)
    {
        pStateCtx = &(pCtx->RecvStateContext);
        /*Assgn Connectors*/
        pStateCtx->pfConnector = phNciNfc_RecvConnector;
        pStateCtx->pfStateFunction = phNciNfc_RecvStateFptr;
        pStateCtx->pStateTransition = &(phNciNfc_RecvTransition[0][0]);
        pStateCtx->CurrState = phNciNfc_StateRecvIdle;
        pStateCtx->Evt = phNciNfc_EvtRecvNone;
        TimerId = phOsalNfc_Timer_Create();
        if (PH_OSALNFC_TIMER_ID_INVALID == TimerId)
        {
            PH_LOG_NCI_WARN_STR("Response Timer Create failed");
            wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }else
        {
            PH_LOG_NCI_INFO_STR("Response Timer Created Successfully");
            pCtx->TimerInfo.dwRspTimerId = TimerId;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_NCI_INFO_STR("InitRecverStateMachine: SUCCESS");
    }else
    {
        PH_LOG_NCI_WARN_STR("InitRecverStateMachine: FAILED");
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS phNciNfc_CoreReleaseRecverStateMachine(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = (pphNciNfc_CoreContext_t)pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        if(0 != pCtx->TimerInfo.dwRspTimerId)
        {
            (void)phOsalNfc_Timer_Delete(pCtx->TimerInfo.dwRspTimerId);
            pCtx->TimerInfo.dwRspTimerId = 0;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

#define STATE_IS_CONNECTOR(STATE)  \
    ((STATE) > phNciNfc_STATE_RECV_MAX && (STATE)< phNciNfc_CONN_RECV_MAX)

/*Before calling the State Engine caller must ensure that events is in the difined range*/

NFCSTATUS phNciNfc_RecvStateHandler(void* pContext, phNciNfc_EvtRecv_t CurrEvent)
{
    phNciNfc_StateRecv_t TrgtState = phNciNfc_StateRecvNone;
    phNciNfc_StateRecv_t CurrState = phNciNfc_StateRecvNone;
    pphNciNfc_CoreContext_t pCtx = pContext;
    pphNciNfc_RecvStateContext_t pStateCtx = NULL;
    uint16_t Result = 0;
    uint8_t Index = 0;
    PH_LOG_NCI_FUNC_ENTRY();

    if ((NULL == pCtx) || (pCtx != phNciNfc_GetCoreContext()))
    {
        Result = NFCSTATUS_INVALID_PARAMETER;
        goto Done;
    }

    pStateCtx = &(pCtx->RecvStateContext);
    if (NULL == pStateCtx)
    {
        Result = NFCSTATUS_INVALID_PARAMETER;
        goto Done;
    }

    /* Get current state */
    CurrState = pStateCtx->CurrState;
    /*Get next state/connector for the event*/
    if((CurrState < phNciNfc_StateRecvIdle) ||
       (CurrState >= phNciNfc_STATE_RECV_MAX) ||
       (CurrEvent < phNciNfc_EvtRecvPacket) ||
       (CurrEvent >= phNciNfc_EVT_RECV_MAX))
    {
        PH_LOG_NCI_CRIT_STR("Unknown Current State or Event!");
        Result = NFCSTATUS_INVALID_STATE;
        goto Done;
    }

    TrgtState = phNciNfc_RecvState2Event[CurrState][CurrEvent];
    do
    {
        if (STATE_IS_CONNECTOR(TrgtState))
        {
            /*Get Connector Index*/
            Index = (uint8_t)(TrgtState - phNciNfc_STATE_RECV_MAX - 1);
            /*State = Connector*/
            Result = pStateCtx->pfConnector[Index](pCtx);
            if (0 == Result)
            {
                TrgtState = phNciNfc_RecvState2State[CurrState][(uint8_t)(Index * 2)];
            }else
            {
                TrgtState = phNciNfc_RecvState2State[CurrState][((uint8_t)Index * 2) + 1];
            }
        }else if (TrgtState >= phNciNfc_StateRecvNone)
        {
            /*No Action to be taken Event ocuured can not be handled in this state*/
            /*set to invalid state*/
            PH_LOG_NCI_CRIT_STR("Recv State Machine: Invalid Target State- State Machine might be corrupted");
            break;
        }else
        {
            PH_LOG_NCI_INFO_STR("Continue... ");
        }
    }while(TrgtState >= phNciNfc_STATE_RECV_MAX);

    if ((TrgtState < phNciNfc_STATE_RECV_MAX))
    {
        if(NULL != pStateCtx->pfStateFunction[CurrState].pfExit)
        {
            Result = pStateCtx->pfStateFunction[CurrState].pfExit(pContext);
        }

        if (CurrState != TrgtState)
        {
            if (NULL != phNciNfc_RecvTransition[CurrState][TrgtState])
            {
                PH_LOG_NCI_INFO_STR("Recv CurrState = %!phNciNfc_StateRecv_t!", CurrState);
                PH_LOG_NCI_INFO_STR("Recv TrgtState = %!phNciNfc_StateRecv_t!", TrgtState);
                pStateCtx->CurrState = TrgtState; /*Update to Target State*/
                Result = phNciNfc_RecvTransition[CurrState][TrgtState](pContext);

                if(TrgtState == phNciNfc_StateRecvDummy)
                {
                    CurrState = phNciNfc_StateRecvDummy;
                    TrgtState = phNciNfc_StateRecvIdle;
                    pStateCtx->CurrState = TrgtState; /*Update to Target State*/
                    /*Call the Transition function*/
                    Result = phNciNfc_RecvTransition[CurrState][TrgtState](pContext);
                }
            }
        }

        if(NULL != phNciNfc_GetCoreContext())
        {
            /*Entry of next state*/
            if(NULL != pStateCtx->pfStateFunction[TrgtState].pfEntry)
            {
                Result = pStateCtx->pfStateFunction[TrgtState].pfEntry(pContext);
            }
        }
    }

    /*if Current State == Dummy state then move to Idle State*/

Done:

    PH_LOG_NCI_FUNC_EXIT();
    return Result;
}

static uint8_t phNciNfc_CoreRecvChkPktType(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t Status = 0;
    uint8_t bMsgType = 0;
    pphNciNfc_CoreContext_t pCtx = pContext;
    pphNciNfc_RecvStateContext_t pStateCtx = NULL;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        pStateCtx = &(pCtx->RecvStateContext);
        /* Validate the buffer sent from Tml and length of received data should be 0 */
        if((NULL != pCtx->pInfo.pBuff) && (0 != pCtx->pInfo.wLength)
             && (NFCSTATUS_SUCCESS == pCtx->pInfo.wStatus))
        {
            bMsgType = PHNCINFC_CORE_GET_MT(pCtx->pInfo.pBuff);
            switch(bMsgType)
            {
                case phNciNfc_e_NciCoreMsgTypeCntrlCmd:
                {
                    PH_LOG_NCI_WARN_STR("NFCC Sends Command Packet Type, dropping...");
                    phNciNfc_CoreRemoveLastChainedNode(pCtx);
                    Status = 1;
                }
                break;
                case phNciNfc_e_NciCoreMsgTypeCntrlNtf:
                {
                    if ((phNciNfc_STATE_RECV_MAX > pStateCtx->CurrState) && (PHNCINFC_CORE_PKT_HEADER_LEN + 1 < pCtx->pInfo.wLength) &&
                        (phNciNfc_e_CoreNciCoreGid == PHNCINFC_CORE_GET_GID(pCtx->pInfo.pBuff)) &&
                        (phNciNfc_e_NciCoreInterfaceErrNtfOid == PHNCINFC_CORE_GET_OID(pCtx->pInfo.pBuff)) &&
                        (PH_NCINFC_STATUS_RF_TRANSMISSION_ERROR == pCtx->pInfo.pBuff[PHNCINFC_CORE_PKT_HEADER_LEN]))
                    {
                        PH_LOG_NCI_WARN_STR("Received RF transmission error ntf in state %d for conn id %d, dropping...",
                                            pStateCtx->CurrState, PHNCINFC_CORE_GET_CONNID(pCtx->pInfo.pBuff));

                        phNciNfc_CoreRemoveLastChainedNode(pCtx);
                        Status = 1;
                        break;
                    }
                }
                // Falling through the current case to the subsequent one for ntf processing.
                case phNciNfc_e_NciCoreMsgTypeData:
                case phNciNfc_e_NciCoreMsgTypeCntrlRsp:
                {
                    if (pCtx->tReceiveInfo.wNumOfNodes > 1 && phNciNfc_CoreRecvChkExptdPktType(pCtx))
                    {
                        PH_LOG_NCI_INFO_STR("New packet received before completion of previous segmented packet");
                        phOsalNfc_MemCopy(pCtx->tReceiveInfo.ListHead.tMem.aBuffer, pCtx->pInfo.pBuff, pCtx->pInfo.wLength);
                        pCtx->pInfo.pBuff = pCtx->tReceiveInfo.ListHead.tMem.aBuffer;
                        phNciNfc_CoreDeleteList(pCtx);
                    }

                    /*Update Header Info of the received packet*/
                    wStatus = phNciNfc_CoreUtilsUpdatePktInfo(pCtx,pCtx->pInfo.pBuff,pCtx->pInfo.wLength);
                    if(PH_NCINFC_STATUS_OK == wStatus)
                    {
                        PH_LOG_NCI_INFO_STR("Received packet validation success %!phNciNfc_NciCoreMsgType_t!", bMsgType);
                        Status = 0;
                    }
                    else
                    {
                        /* NFCC send Invalid GID/OID or packet length and payload length mismatch, dropping... */
                        PH_LOG_NCI_CRIT_STR("Received packet validation failed %!phNciNfc_NciCoreMsgType_t!", bMsgType);
                        phNciNfc_CoreDeleteList(pCtx);
                        Status = 1;
                    }
                }
                break;
                default:
                {
                    PH_LOG_NCI_WARN_STR("NFCC Sends Unknown Packet Type, dropping...");
                    /* Delete list */
                    phNciNfc_CoreDeleteList(pCtx);
                    Status = 1;
                }
                break;
            }
        }else
        {
            PH_LOG_NCI_CRIT_STR("Critical failure: Invalid Tml buffer received or length of payload in \
                          tml buffer is 0 or Tml read is not success!");
            Status = 1;
        }
    }else
    {
        PH_LOG_NCI_CRIT_STR("Critical failure: Invalid Nci Core context!");
        Status = 1;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return Status;
}
static uint8_t phNciNfc_CoreRecvChkPbfAndUpdate(void *pContext)
{
    uint8_t Status = 0;
    uint8_t bPbf = 0;
    pphNciNfc_CoreContext_t pCtx = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        /* Get pbf status */
        bPbf = PHNCINFC_CORE_GET_PBF(pCtx->pInfo.pBuff);
        if (bPbf == 1)
        {
            Status = 1;
        }else
        {
            Status = 0;
        }
        /* Update payload length of the received packet in its respective node (last node) */
        phNciNfc_CoreUpdatePacketLen(pCtx,pCtx->pInfo.wLength);
        /* Since received packet looks to be fine, update the total payload size */
        pCtx->tReceiveInfo.wPayloadSize += pCtx->pInfo.wLength;
    }else
    {
        /* Shall never enter here as the handle is being validated before invoking this function */
        PH_LOG_NCI_CRIT_STR("Critical failure: Invalid Core context handle!");
        /* Can not be handled, if it enters */
    }
    PH_LOG_NCI_FUNC_EXIT();
    return Status;
}

static uint8_t phNciNfc_CoreRecvChkExptdPktType(void *pContext)
{
    uint8_t Status = 0xFF;
    uint8_t bMsgType = 0;
    uint8_t temp = 0;
    pphNciNfc_CoreContext_t pCtx = pContext;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
       bMsgType = PHNCINFC_CORE_GET_MT(pCtx->pInfo.pBuff);
       if(bMsgType == pCtx->tReceiveInfo.HeaderInfo.eMsgType)
       {
            switch (bMsgType)
            {
                case phNciNfc_e_NciCoreMsgTypeData:
                {
                    temp = PHNCINFC_CORE_GET_CONNID(pCtx->pInfo.pBuff);
                    if(temp == pCtx->tReceiveInfo.HeaderInfo.bConn_ID)
                    {
                        Status = 0;
                    }else
                    {
                        Status = 1;
                    }
               }
               break;
               case phNciNfc_e_NciCoreMsgTypeCntrlRsp:
               case phNciNfc_e_NciCoreMsgTypeCntrlNtf:
               {
                   temp = PHNCINFC_CORE_GET_GID(pCtx->pInfo.pBuff);
                   if(temp == pCtx->tReceiveInfo.HeaderInfo.Group_ID)
                   {
                        temp = PHNCINFC_CORE_GET_OID(pCtx->pInfo.pBuff);
                        if(temp == pCtx->tReceiveInfo.HeaderInfo.Opcode_ID.Val)
                        {
                            Status = 0;
                        }else
                        {
                            Status = 1;
                        }
                   }else
                   {
                       Status = 1;
                   }
               }
               break;
           }
        }else
        {
            Status = 1;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return Status;
}

static NFCSTATUS phNciNfc_CoreRecvIdleEntry(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;
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

static NFCSTATUS phNciNfc_CoreRecvIdleExit(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();

    UNUSED(pContext);

    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CoreRecvEntry(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();

    UNUSED(pContext);

    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CoreRecvExit(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CoreRecvDummyEntry(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CoreRecvDummyExit(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_StateIdle2Recv(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();

    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_StateIdle2Dummy(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();

    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_StateRecv2Dummy(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_StateDummy2Idle(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;
    uint8_t bStopTimer = 0;
    phNciNfc_sCoreHeaderInfo_t HeaderInfo;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        if (1 == pCtx->TimerInfo.TimerStatus)/*Is Timer Running*/
        {
            if(pCtx->TimerInfo.PktHeaderInfo.eMsgType == pCtx->tReceiveInfo.HeaderInfo.eMsgType)
            {
                switch(pCtx->TimerInfo.PktHeaderInfo.eMsgType)
                {
                    /* Compare Gid and Oid */
                    case phNciNfc_e_NciCoreMsgTypeCntrlRsp:
                    case phNciNfc_e_NciCoreMsgTypeCntrlNtf:
                        if((pCtx->TimerInfo.PktHeaderInfo.Group_ID ==
                            pCtx->tReceiveInfo.HeaderInfo.Group_ID)
                            && (pCtx->TimerInfo.PktHeaderInfo.Opcode_ID.Val ==
                            pCtx->tReceiveInfo.HeaderInfo.Opcode_ID.Val))
                        {
                            bStopTimer = 1;
                        }
                        break;
                    /* Compare Conn ID */
                    case phNciNfc_e_NciCoreMsgTypeData:
                        if(pCtx->TimerInfo.PktHeaderInfo.bConn_ID ==
                            pCtx->tReceiveInfo.HeaderInfo.bConn_ID)
                        {
                            bStopTimer = 1;
                        }
                        break;
                    default:
                        break;
                }
                if(1 == bStopTimer)
                {
                    /*Stop Timer*/
                    (void)phOsalNfc_Timer_Stop(pCtx->TimerInfo.dwRspTimerId);
                    pCtx->TimerInfo.TimerStatus = 0;/*timer stopped*/
                }
            }
        }
        wStatus = pCtx->RecvStateContext.wProcessStatus;
        if(NFCSTATUS_RESPONSE_TIMEOUT == wStatus)
        {
            phOsalNfc_MemCopy(&HeaderInfo, &(pCtx->TimerInfo.PktHeaderInfo), sizeof(phNciNfc_sCoreHeaderInfo_t));
        }else
        {
            phOsalNfc_MemCopy(&HeaderInfo, &(pCtx->tReceiveInfo.HeaderInfo), sizeof(phNciNfc_sCoreHeaderInfo_t));
        }
        /* Message type is being validated earlier, shall never be invalid */
        if((HeaderInfo.eMsgType == phNciNfc_e_NciCoreMsgTypeData) ||
            (HeaderInfo.eMsgType == phNciNfc_e_NciCoreMsgTypeCntrlRsp) ||
            (HeaderInfo.eMsgType == phNciNfc_e_NciCoreMsgTypeCntrlNtf))
        {
            /*invoke Response manager*/
            (void )phNciNfc_CoreRecvManager(pCtx, wStatus, &HeaderInfo, HeaderInfo.eMsgType);
        }
    }else
    {
        PH_LOG_NCI_CRIT_STR("Invalid core context!");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreRecvConvertStatus2Evt(NFCSTATUS Status, phNciNfc_EvtRecv_t *pEvt)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pEvt)
    {
        switch(Status)
        {
            case NFCSTATUS_SUCCESS:
            {
                *pEvt = phNciNfc_EvtRecvPacket;
            }
            break;
            default:
            {
                wStatus = NFCSTATUS_FAILED;
                PH_LOG_NCI_WARN_STR("Unknown Event");
            }
            break;
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_NCI_WARN_STR("Invalid parameter!");
    }
     PH_LOG_NCI_FUNC_EXIT();
     return wStatus;
}

void phNciNfc_RspTimeOutCb(uint32_t TimerId, void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t pCtx = pContext;
    phNciNfc_EvtRecv_t TrigEvt = phNciNfc_EvtRecvTimeOut;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCtx) && (pCtx == phNciNfc_GetCoreContext()))
    {
        /* No response received and the timer expired */
        (void)phOsalNfc_Timer_Stop(TimerId);
        pCtx->TimerInfo.TimerStatus = 0;

        /* Abort pending read to prevent the TML layer from using core recv list buffers after
         it's freed. This can happen if we are currently processing a segmented packet */

        (void)phTmlNfc_ReadAbort(pCtx->pHwRef);

        switch(pCtx->TimerInfo.PktHeaderInfo.eMsgType)
        {
            case phNciNfc_e_NciCoreMsgTypeCntrlRsp:
            {
                PH_LOG_NCI_WARN_STR("Timer expired before response is received!");
                wStatus = NFCSTATUS_RESPONSE_TIMEOUT;
            }
            break;
            case phNciNfc_e_NciCoreMsgTypeData:
            {
                PH_LOG_NCI_WARN_STR("Timer expired before data is received!");
                wStatus = NFCSTATUS_RESPONSE_TIMEOUT;
            }
            break;
            default:
            {
                PH_LOG_NCI_WARN_STR("Timer expired - Should never enter here");
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
        pCtx->RecvStateContext.wProcessStatus = wStatus;
        (void)phNciNfc_RecvStateHandler(pCtx, TrigEvt);

        /* Requeue the read with the TML layer */
        (void)phNciNfc_CoreRecv(pCtx);
    }

    PH_LOG_NCI_FUNC_EXIT();
}
