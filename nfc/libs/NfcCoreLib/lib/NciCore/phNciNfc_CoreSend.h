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

#define PHNCINFC_CORESEND_TIMEOUT_MULTIPLIER           (100)

typedef enum phNciNfc_EvtSend
{
    phNciNfc_EvtSendPacket, /**< New Packet Send Request Event*/
    phNciNfc_EvtSendComplete,/**< previous packet is sent*/
    phNciNfc_EvtCreditAvail,/**< Credit Available event*/
    phNciNfc_EvtWaitCreditTimeOut,/**< Wait credit time out event.*/
    phNciNfc_EVT_SEND_MAX,/**< Max Even limit*/
    phNciNfc_EvtNone/**< Inavalid event*/
}phNciNfc_EvtSend_t;

typedef enum phNciNfc_StateSends
{
    phNciNfc_StateSendIdle,/**< Send Idle(ready) state for new send request */
    phNciNfc_StateSend,/**< Send state min one packet to sent to the lower layer*/
    phNciNfc_StateWaitCredit,/**< The state no credit is available to send rest of the packets*/
    phNciNfc_STATE_SEND_MAX,/**< Limit for states*/
    phNciNfc_ConnChkPktType,/**< check packet type to be used to create packet*/
    phNciNfc_ConnChkCredit,   /**< Connector to checks credit availability*/
    phNciNfc_ConnChkSize,     /**< Connector to checks remaining size to be sent*/
    phNciNfc_CONN_SEND_MAX,    /**< Limit for connectors*/
    phNciNfc_StateSendNone  /**< State for no Action*/
}phNciNfc_StateSend_t;

typedef struct phNciNfc_SendStateContext
{
    phNciNfc_StateSend_t CurrState; /**< Current send State*/
    phNciNfc_EvtSend_t Evt; /**< Event Triggered*/
    phNciNfc_Connector_t *pfConnector; /**< State connectors*/
    phNciNfc_StateFunction_t *pfStateFunction; /**< State fptrs (Entry, Transition, Exit)*/
    phNciNfc_StateTransition_t *pStateTransition; /**< Send (State to State) transition function pointers*/
}phNciNfc_SendStateContext_t, *pphNciNfc_SendStateContext_t; /**< Pointer to #phNciNfc_SendStateContext_t structure */

extern NFCSTATUS phNciNfc_CoreInitSenderStateMachine(void *pContext);
extern NFCSTATUS phNciNfc_CoreResetSenderStateMachine(void *pContext);
extern NFCSTATUS phNciNfc_CoreReleaseSenderStateMachine(void *pContext);
extern void phNciNfc_CoreResetSendStateMachine(void *pContext);

extern NFCSTATUS phNciNfc_StateHandler(void* pContext, phNciNfc_EvtSend_t CurrEvent);
