/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 *
 */

#pragma once

#include "phNciNfc_State.h"

typedef enum phNciNfc_EvtRecv
{
    phNciNfc_EvtRecvPacket, /**< New Packet Recv Event*/
    phNciNfc_EvtRecvComplete, /**< All segment of the packet recvd*/
    phNciNfc_EvtRecvTimeOut,/**< Recv Response time out event.*/
    phNciNfc_EvtRecvHwError,/**< HW error event.*/
    phNciNfc_EVT_RECV_MAX,/**< Max Event limit*/
    phNciNfc_EvtRecvNone/**< Inavalid event*/
}phNciNfc_EvtRecv_t;

typedef enum phNciNfc_StateRecv
{
    phNciNfc_StateRecvIdle,/**< Receive Idle(ready) state to receive new packetS*/
    phNciNfc_StateRecv,/**< Receive state min one packet received*/
    phNciNfc_StateRecvDummy,/**< The state no credit is available to send rest of the packets*/
    phNciNfc_STATE_RECV_MAX,/**< Limit for states*/
    phNciNfc_ConnRecvChkPktType,/**< check packet type to be used to create packet*/
    phNciNfc_ConnChkPbf,   /**< Connector to checks credit availability*/
    phNciNfc_ConnExptdPktType,
    phNciNfc_CONN_RECV_MAX,    /**< Limit for connectors*/
    phNciNfc_StateRecvNone  /**< State for no Action*/
}phNciNfc_StateRecv_t;

typedef struct phNciNfc_RecvStateContext
{
    phNciNfc_StateRecv_t CurrState; /**< Current Recv State*/
    phNciNfc_EvtRecv_t Evt; /**< Event Triggered*/
    phNciNfc_Connector_t *pfConnector; /**< State connectors*/
    phNciNfc_StateFunction_t *pfStateFunction; /**< State fptrs (Entry, Transition, Exit)*/
    phNciNfc_StateTransition_t *pStateTransition; /**< Send (State to State) transition function pointers*/
    NFCSTATUS wProcessStatus;        /*Status of the processed packet e.g, TML status, or timer status etc*/
}phNciNfc_RecvStateContext_t, *pphNciNfc_RecvStateContext_t; /**< Pointer to #phNciNfc_SendStateContext_t structure */


extern NFCSTATUS phNciNfc_CoreInitRecverStateMachine(void *pContext);
extern NFCSTATUS phNciNfc_CoreReleaseRecverStateMachine(void *pContext);
extern NFCSTATUS phNciNfc_RecvStateHandler(void* pContext, phNciNfc_EvtRecv_t CurrEvent);
extern NFCSTATUS phNciNfc_CoreRecvConvertStatus2Evt(NFCSTATUS Status, phNciNfc_EvtRecv_t *pEvt);

extern void phNciNfc_RspTimeOutCb(uint32_t TimerId, void *pContext);
