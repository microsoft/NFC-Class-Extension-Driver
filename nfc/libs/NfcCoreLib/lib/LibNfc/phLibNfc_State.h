/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcStatus.h>
#include "phNciNfcTypes.h"

typedef uint8_t (*phLibNfc_Connector_t) (void *pContext,void *pInfo);
typedef NFCSTATUS (*phLibNfc_StateEntry_t)(void *pContext, void *Param1, void *Param2, void *Param3);
typedef NFCSTATUS (*phLibNfc_StateExit_t)(void *pContext, void *Param1, void *Param2, void *Param3);
typedef NFCSTATUS (*phLibNfc_StateTransition_t)(void *pContext, void *Param1, void *Param2, void *Param3);

typedef struct phLibNfc_StateFunction
{
    phLibNfc_StateEntry_t pfEntry; /**< Entry function to be called before entering to the new state*/
    phLibNfc_StateExit_t pfExit;/**< to be performed before exit from the current state*/
}phLibNfc_StateFunction_t, *pphLibNfc_StateFunction_t;

typedef enum phLibNfc_State
{
    phLibNfc_StateIdle = 0x00,      /**< LibNfc Idle state - LibNfc not operational*/
    phLibNfc_StateInit,             /**< LibNfc Init State - LibNfc stack initialised and operational*/
    phLibNfc_StateReset,            /**< LibNfc Reset State - LibNfc stack is resetted*/
    phLibNfc_StateDiscovery,        /**< LibNfc Discovery State - LibNfc has configured properly and poll is running*/
    phLibNfc_StateDiscovered,       /**< LibNfc Discovered State - LibNfc has received one or more device discovered information*/
    phLibNfc_StateTransceive,       /**< LibNfc Device Active/transceive State - LibNfc is ready to do/doing communication with remote device*/
    phLibNfc_StateSend,             /**< LibNfc is in Send state - This state applied to if remote device is P2P target*/
    phLibNfc_StateRecv,             /**< LibNfc is in Recv state - This state applied to if remote device is P2P target*/
    phLibNfc_StateSEListen,         /**< LibNfc in SE Listen state - applies to NFCEE in listen mode */
    phLibNfc_StateDummy,            /**< Dummy State never stay , this state used for setting or getting configuration etc*/
    phLibNfc_STATE_MAX,             /**< LibNfc Max State Count*/
    phLibNfc_ConnChkListnerNtf,     /**< Connector to check that Lisner for device discovery is registered*/
    phLibNfc_ConnChkDeviceType,     /**< Connector to check the device type after activation*/
    phLibNfc_ChkTgtType,            /**< Connector to check the type of Protocol and interface being used in listen phase*/
    phLibNfc_ConnChkDeactType,      /**< Connetor to check Disconnect type from the user*/
    phLibNfc_ConnChkDiscReqType,    /**< Connector to check the type of discovery user is requesting (disc start, disc cont, disc resume)*/
    phLibNfc_STATE_CONN_MAX,
    phLibNfc_StateNone,             /**< State none indicates mapping has no effect or maaping doesnt exist*/
    phLibNfc_StateInvalid           /**< LibNfc Invalid State */
}phLibNfc_State_t;

typedef enum phLibNfc_TransitionFlag
{
    phLibNfc_StateTransitionComplete = 0x00,    /**< The previous events triggered is completed*/
    phLibNfc_StateTrasitionBusy,                /**< Acting on Triggered event or waiting for completetion*/
    phLibNfc_StateTrasitionInvalid              /*Invalid Transition flag*/
}phLibNfc_TransitionFlag_t;

typedef enum phLibNfc_Event
{
    phLibNfc_EventInit = 0x00,      /**< Inerally generated event on the client request to initialise*/
    phLibNfc_EventReset,            /**< Reset event Internally generated*/
    phLibNfc_EventDiscovery,        /**< Inerally generated event on the client request to start discovery*/
    phLibNfc_EventActivate,          /**< Inerally generated event on the client request to activate
                                         remote device*/
    phLibNfc_EventDeActivate,        /**< Inerally generated event on the client request to Dectivate
                                         remote device*/
    phLibNfc_EventDeActivateSleep,   /**< Inerally generated event on the client request to Dectivate
                                         remote device*/
    phLibNfc_EventTransceive,           /**< Transceive event*/
    phLibNfc_EventSend,                 /**< */
    phLibNfc_EventRecv,                 /**< */
    phLibNfc_EventDummy,                /**< The event used to complete the user request which are
                                        independent of state e.g RfConfig, CE config etc*/
    phLibNfc_EVENT_MAX,              /**< Max Event count of Upper(User) request*/
    phLibNfc_EVENT_USER_MAX,
    phLibNfc_EventReqCompleted,     /**< Event in response to event any user request which is completed*/
    phLibNfc_EventReseted,          /**< This event triggred by NFC controller as a result of self reset*/
    phLibNfc_EventDeviceDiscovered, /**< NFCC discovered devices*/
    phLibNfc_EventDeviceActivated,  /**< This event could be response to phLibNfc_EventActivate
                                     or NFCC discovered and activated device by itself*/
    phLibNfc_EventTimeOut,          /**< NFCC has not responded to the request forwarded in specified time*/
    phLibNfc_EventBoardError,        /**< There was hardware error during transmit or Receive*/
    phLibNfc_EventFailed,           /**< Failure in processing the API */
    phLibNfc_EventDeactivated,           /**< Asynchronous deactivate notification from the NFCC */
    phLibNfc_EventSEActivated,          /**< SE device in listen mode has been activated */
    phLibNfc_EventPCDActivated,          /**< PCD device in listen mode has been activated */
    phLibNfc_EVENT_INT_MAX,          /**< Max Event Count*/
    phLibNfc_EventInvalid           /*Invalid Event Value*/

}phLibNfc_Event_t;

struct phLibNfc_Event_Queue;
typedef struct phLibNfc_Event_Queue
{
    phLibNfc_Event_t Event;
    void * Param1;
    void * Param2;
    void * Param3;
    struct phLibNfc_Event_Queue * Next;
} phLibNfc_Event_Queue_t;

typedef enum phLibNfc_EventType
{
    phLibNfc_EventTypeInternal,
    phLibNfc_EventTypeUser,
    phLibNfc_EventTypeInvalid
}phLibNfc_EventType_t;

typedef struct phLibNfc_TrigEvent
{
    phLibNfc_Event_t TrigEvent;
    phLibNfc_EventType_t EventType;
}phLibNfc_TrigEvent_t;

typedef struct phLibNfc_StateContext
{
    phLibNfc_TransitionFlag_t Flag;         /**< Flag to indicate busy in processing triggered event*/
    phLibNfc_State_t CurrState;             /**< Current state of the LibNfc*/
    phLibNfc_State_t TrgtState;             /**< Current state of the LibNfc*/
    phLibNfc_Event_t TrigEvent;             /**< Triggered Event*/
    phLibNfc_Event_t IntEvent;              /**< internal event in execution*/
    phLibNfc_Event_t UsrEvent;              /**< User event in execution*/
    phLibNfc_Event_Queue_t * pDeferredEvent;  /**< Deferred events, mostly from User space */
}phLibNfc_StateContext_t, pphLibNfc_StateContext_t; /**< Libnfc Nfc state information*/


extern NFCSTATUS phLibNfc_Idle2InitTransition(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Actv2Reset(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Init2Reset(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Actv2Idle(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Init2Idle(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Discovered2Discovery(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Discovered2Transceive(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Transceive2Discovered(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Send2Discovered(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Recv2Discovered(void *pContext, void *Param1, void *Param2, void *Param3);

extern NFCSTATUS phLibNfc_Init2Discovery(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Actv2Discovery(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_LsnAc2Dscv_Def(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Actv2Init(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_DummyInit(void *pContext, void *Param1, void *Param2, void *Param3);

extern NFCSTATUS phLibNfc_TranscvEnter(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_TranscvExit(void *pContext, void *Param1, void *Param2, void *Param3);

extern NFCSTATUS phLibNfc_InitStateMachine(void *pContext);
extern NFCSTATUS phLibNfc_StateHandler(void *pContext, phLibNfc_Event_t TrigEvent, void *Param1, void *Param2, void *Param3);

extern NFCSTATUS phLibNfc_Discovery2Recv(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Discovery2Active(void *pContext, void *Param1, void *Param2, void *Param3);

extern uint8_t phLibNfc_ChkDiscoveryType(void *pContext,void *pInfo);
extern uint8_t phLibNfc_ChkDiscoveryTypeAndMode(void *DidcMode,void *pInfo, void *TrigEvnt);

extern NFCSTATUS phLibNfc_StatePrepareShutdown(void *pContext);
extern NFCSTATUS phLibNfc_EnablePriorityDiscDiscon(void *pContext);
extern NFCSTATUS phLibNfc_EnablePriorityDiscovery(void *pContext);
extern NFCSTATUS phLibNfc_IsInitialised(void *pContext);

extern NFCSTATUS phLibNfc_IsInitialisedState(void *pContext);
extern NFCSTATUS phLibNfc_IsShutDownInProgress(void *pContext);
extern NFCSTATUS phLibNfc_RestartDiscovery(void);

phLibNfc_State_t phLibNfc_GetState(void *pContext);
