/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_State.tmh"

#define PH_LIBNFC_NO_REMOTEDEVICE           (0xFF) /**< No active remote device connected */
#define PH_LIBNFC_CURRENTMODE_POLL          (0x00) /**< Remote device is either a Tag or P2P Target */
#define PH_LIBNFC_CURRENTMODE_LISTEN        (0x01) /**< Remote device is either a P2P Initiator or a Reader */
#define PH_LIBNFC_INVALID_REMOTEDEVICE      (0x01) /**< Unable to identify remote device */

static uint8_t phLibNfc_CheckCurrentMode(pphLibNfc_LibContext_t pLibCtx);
static uint8_t phLibNfc_ConnChkDevType(void *pContext,void *pInfo);
static uint8_t phLibNfc_ConnChkTgtType(void *pContext,void *pInfo);
static uint8_t phLibNfc_ConnIsRfListnerRegisterd(void *pContext, void *pInfo);

static NFCSTATUS phLibNfc_RfListnerRegisterd(void *pContext,uint32_t DevType, uint8_t bSak );

static NFCSTATUS phLibNfc_ChkRfListnerforNFCAPoll(void *pContext, pphNciNfc_DeviceInfo_t pNciDevInfo, uint8_t bIndex);
static NFCSTATUS phLibNfc_ChkRfListnerforNFCBPoll(void *pContext, pphNciNfc_DeviceInfo_t pNciDevInfo, uint8_t bIndex );
static NFCSTATUS phLibNfc_ChkRfListnerforNFCISO15693Poll(void *pContext, pphNciNfc_DeviceInfo_t pNciDevInfo, uint8_t bIndex);
static NFCSTATUS phLibNfc_ChkRfListnerforNFCFPoll(void *pContext, pphNciNfc_DeviceInfo_t pNciDevInfo, uint8_t bIndex);
static NFCSTATUS phLibNfc_ChkRfListnerforNFCAListen(void *pContext, pphNciNfc_DeviceInfo_t pNciDevInfo, uint8_t bIndex);
static NFCSTATUS phLibNfc_ChkRfListnerforNFCBListen(void *pContext, pphNciNfc_DeviceInfo_t pNciDevInfo, uint8_t bIndex);
static NFCSTATUS phLibNfc_ChkRfListnerforNFCFListen(void *pContext, pphNciNfc_DeviceInfo_t pNciDevInfo, uint8_t bIndex);

static NFCSTATUS phLibNfc_MapRemDevType(phNciNfc_RFDevType_t NciNfc_RemDevType, phNfc_eRFDevType_t *LibNfc_RemDevType);
static uint8_t phLibNfc_ChkDeActType(void *pContext,void *pInfo);

static phLibNfc_State_t phLibNfc_FindTrgtState(void *pContext, phLibNfc_State_t CurrState, phLibNfc_State_t TrgtState,void * pInfo);
static uint8_t phLibNfc_ValidateTransition(phLibNfc_State_t CurrState, phLibNfc_State_t  TgtState);
static NFCSTATUS phLibNfc_StateStatusConversion(phLibNfc_State_t CurrState, phLibNfc_State_t TrgtState, phLibNfc_Event_t TrigEvent);

static NFCSTATUS phLibNfc_StateMachineHandleUsrEvent(void *pContext, phLibNfc_Event_t TrigEvent, void *Param1, void *Param2, void *Param3);
static NFCSTATUS phLibNfc_StateMachineHandleIntEvent(void *pContext, phLibNfc_Event_t TrigEvent, void *Param1, void *Param2, void *Param3);

static NFCSTATUS phLibNfc_DummyFunc(void *pContext, void *Param1, void *Param2, void *Param3);

static phLibNfc_Connector_t phLibNfc_Connector[(phLibNfc_STATE_CONN_MAX - phLibNfc_STATE_MAX - 1)] =
{
    &phLibNfc_ConnIsRfListnerRegisterd,
    &phLibNfc_ConnChkDevType,
    &phLibNfc_ConnChkTgtType,
    &phLibNfc_ChkDeActType,
    &phLibNfc_ChkDiscoveryType
};

/*LibNfc State to State(connectors)*/
static const phLibNfc_State_t phLibNfc_State2State[phLibNfc_STATE_MAX][(phLibNfc_STATE_CONN_MAX - phLibNfc_STATE_MAX - 1) *2] =
{
                                  /*phLibNfc_ConnChkListnerNtf*/                      /*phLibNfc_ConnChkDeviceType*/               /*phLibNfc_ChkTgtType*/                              /*phLibNfc_ConnChkDeactType*/                /*phLibNfc_ConnChkDiscReqType*/
                                /*TRUE*/                     /*FALSE*/                  /*TRUE*/           /*FALSE*/               /*TRUE*/                 /*FALSE*/                      /*TRUE*/                 /*FALSE*/           /*TRUE*/                 /*FALSE*/
    /*phLibNfc_StateIdle*/      {phLibNfc_StateNone,         phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone,     phLibNfc_StateNone,             phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone},
    /*phLibNfc_StateInit*/      {phLibNfc_StateNone,         phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone,     phLibNfc_StateNone,             phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateDiscovery, phLibNfc_StateInit},
    /*phLibNfc_StateReset*/     {phLibNfc_StateNone,         phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone,     phLibNfc_StateNone,             phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone},
    /*phLibNfc_StateDiscovery*/ {phLibNfc_ConnChkDeviceType, phLibNfc_StateDiscovery, phLibNfc_ChkTgtType, phLibNfc_StateDiscovered,phLibNfc_StateRecv,    phLibNfc_StateSEListen,         phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateDiscovery, phLibNfc_StateInit},
    /*phLibNfc_StateDiscovered*/{phLibNfc_StateNone,         phLibNfc_StateNone,      phLibNfc_StateRecv, phLibNfc_StateTransceive,phLibNfc_StateNone,     phLibNfc_StateNone,             phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateDiscovery, phLibNfc_StateInit},
    /*phLibNfc_StateTransceive*/{phLibNfc_StateNone,         phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone,     phLibNfc_StateNone,             phLibNfc_StateDiscovery, phLibNfc_StateInit, phLibNfc_StateDiscovery, phLibNfc_StateInit},
    /*phLibNfc_StateSend*/      {phLibNfc_StateNone,         phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone,     phLibNfc_StateNone,             phLibNfc_StateDiscovery, phLibNfc_StateInit, phLibNfc_StateDiscovery, phLibNfc_StateInit},
    /*phLibNfc_StateRecv*/      {phLibNfc_StateNone,         phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone,     phLibNfc_StateNone,             phLibNfc_StateDiscovery, phLibNfc_StateInit, phLibNfc_StateDiscovery, phLibNfc_StateInit},
    /*phLibNfc_StateSEListen*/  {phLibNfc_StateNone,         phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateRecv,     phLibNfc_StateSEListen,         phLibNfc_StateDiscovery, phLibNfc_StateInit, phLibNfc_StateDiscovery, phLibNfc_StateInit},
    /*phLibNfc_StateDummy*/     {phLibNfc_StateNone,         phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone,     phLibNfc_StateNone,             phLibNfc_StateNone,      phLibNfc_StateNone, phLibNfc_StateNone,      phLibNfc_StateNone},
};

/*State Entry and Exit functions*/
static const phLibNfc_StateFunction_t phLibNfc_StateFptr[phLibNfc_STATE_MAX]=
{
    /*Entry*/                       /*Exit*/
    {NULL,                          NULL}, /*phLibNfc_StateIdle*/
    {NULL,                          NULL},/*phLibNfc_StateInit*/
    {NULL,                          NULL},/*phLibNfc_StateReset*/
    {&phLibNfc_StateDiscoveryEntry, NULL},/*phLibNfc_StateDiscovery*/
    {NULL,                          &phLibNfc_StateDiscoveredExit},/*phLibNfc_StateDiscovered*/
    {NULL,                          &phLibNfc_TranscvExit},/*phLibNfc_StateTransceive*/
    {NULL,                          NULL},/*phLibNfc_StateSend*/
    {NULL,                          NULL},/*phLibNfc_StateRecv*/
    {&phLibNfc_StateSEListenEntry,  NULL},/*phLibNfc_StateSEListen*/
    {NULL,                          NULL},/*phLibNfc_StateDummy*/
};

/*LibNfc State to Event mapping*/
static const phLibNfc_State_t phLibNfc_State2Event[phLibNfc_STATE_MAX][phLibNfc_EVENT_MAX]=
{   /*Event->*/                  /*EventInit*/         /*EventReset*/      /*EventDiscovery*/           /*EventActivate*/          /*EventDeActivate*/        /*EventDeActivateSleep*/        /*EventTransceive*/       /*EventSend*/       /*EventRecv*/     /*EventDummy*/
    /*State*/
    /*phLibNfc_StateIdle*/      {phLibNfc_StateInit, phLibNfc_StateNone,  phLibNfc_StateNone,          phLibNfc_StateNone,         phLibNfc_StateNone,        phLibNfc_StateNone,             phLibNfc_StateNone,       phLibNfc_StateNone, phLibNfc_StateNone, phLibNfc_StateNone   },
    /*phLibNfc_StateInit*/      {phLibNfc_StateNone, phLibNfc_StateReset, phLibNfc_ConnChkDiscReqType, phLibNfc_StateInvalid,      phLibNfc_StateNone,        phLibNfc_StateNone,             phLibNfc_StateNone,       phLibNfc_StateNone, phLibNfc_StateNone, phLibNfc_StateDummy  },
    /*phLibNfc_StateReset*/     {phLibNfc_StateNone, phLibNfc_StateReset, phLibNfc_StateNone,          phLibNfc_StateNone,         phLibNfc_StateNone,        phLibNfc_StateNone,             phLibNfc_StateNone,       phLibNfc_StateNone, phLibNfc_StateNone, phLibNfc_StateDummy  },
    /*phLibNfc_StateDiscovery*/ {phLibNfc_StateInit, phLibNfc_StateReset, phLibNfc_ConnChkDiscReqType, phLibNfc_StateNone,         phLibNfc_StateNone,        phLibNfc_StateNone,             phLibNfc_StateNone,       phLibNfc_StateNone, phLibNfc_StateNone, phLibNfc_StateDummy  },
    /*phLibNfc_StateDiscovered*/{phLibNfc_StateInit, phLibNfc_StateReset, phLibNfc_ConnChkDiscReqType, phLibNfc_ConnChkDeviceType, phLibNfc_StateNone,        phLibNfc_StateNone,             phLibNfc_StateNone,       phLibNfc_StateNone, phLibNfc_StateRecv, phLibNfc_StateDummy  },
    /*phLibNfc_StateTransceive*/{phLibNfc_StateInit, phLibNfc_StateReset, phLibNfc_ConnChkDiscReqType, phLibNfc_StateNone,         phLibNfc_ConnChkDeactType, phLibNfc_StateDiscovered,       phLibNfc_StateTransceive, phLibNfc_StateNone, phLibNfc_StateNone, phLibNfc_StateDummy  },
    /*phLibNfc_StateSend*/      {phLibNfc_StateInit, phLibNfc_StateReset, phLibNfc_ConnChkDiscReqType, phLibNfc_StateNone,         phLibNfc_StateNone,        phLibNfc_StateNone,             phLibNfc_StateNone,       phLibNfc_StateRecv, phLibNfc_StateNone, phLibNfc_StateDummy  },
    /*phLibNfc_StateRecv*/      {phLibNfc_StateNone, phLibNfc_StateReset, phLibNfc_ConnChkDiscReqType, phLibNfc_StateNone,         phLibNfc_StateNone,        phLibNfc_StateNone,             phLibNfc_StateNone,       phLibNfc_StateNone, phLibNfc_StateSend, phLibNfc_StateDummy  },
    /*phLibNfc_StateSEListen*/  {phLibNfc_StateNone, phLibNfc_StateReset, phLibNfc_ConnChkDiscReqType, phLibNfc_ConnChkDeviceType, phLibNfc_StateNone,        phLibNfc_StateNone,             phLibNfc_StateNone,       phLibNfc_StateNone, phLibNfc_StateSend, phLibNfc_StateDummy  },
    /*phLibNfc_StateDummy*/     {phLibNfc_StateNone, phLibNfc_StateNone,  phLibNfc_StateNone,          phLibNfc_StateNone,         phLibNfc_StateNone,        phLibNfc_StateNone,             phLibNfc_StateNone,       phLibNfc_StateNone, phLibNfc_StateNone, phLibNfc_StateNone   },
};

/*Mapping from internal event to state(current state or next state)*/
/*if transition flag is busy then always next state must be used else current state*/
static const phLibNfc_State_t phLibNfc_IntEvent2State[phLibNfc_STATE_MAX][phLibNfc_EVENT_INT_MAX - phLibNfc_EVENT_USER_MAX - 1] =
{
    /*Event->*/                 /*EventReqCompleted*/      /*EventReseted*/     /*EventDeviceDiscovered*/   /*EventDeviceActivated*/    /*EventTimeOut*/            /*EventBoardError*/ /*EventFailed*/             /*EventDeactivated*/       /*EventSEActivated*/     /*EventPCDActivated*/
    /*State*/
    /*phLibNfc_StateIdle*/      {phLibNfc_StateNone,       phLibNfc_StateNone,  phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone, phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,       phLibNfc_StateNone},
    /*phLibNfc_StateInit*/      {phLibNfc_StateInit,       phLibNfc_StateIdle,  phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateIdle, phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,       phLibNfc_StateNone},
    /*phLibNfc_StateReset*/     {phLibNfc_StateIdle,       phLibNfc_StateIdle,  phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone, phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,       phLibNfc_StateNone},
    /*phLibNfc_StateDiscovery*/ {phLibNfc_StateDiscovery,  phLibNfc_StateIdle,  phLibNfc_ConnChkListnerNtf, phLibNfc_ConnChkListnerNtf, phLibNfc_StateNone,         phLibNfc_StateIdle, phLibNfc_StateInit,         phLibNfc_StateNone,         phLibNfc_StateSEListen,   phLibNfc_ConnChkListnerNtf},
    /*phLibNfc_StateDiscovered*/{phLibNfc_StateDiscovered, phLibNfc_StateIdle,  phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateDiscovered,   phLibNfc_StateIdle, phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,       phLibNfc_StateNone},
    /*phLibNfc_StateTransceive*/{phLibNfc_StateTransceive, phLibNfc_StateIdle,  phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateTransceive,   phLibNfc_StateIdle, phLibNfc_StateTransceive,   phLibNfc_StateNone,         phLibNfc_StateNone,       phLibNfc_StateNone},
    /*phLibNfc_StateSend*/      {phLibNfc_StateSend,       phLibNfc_StateIdle,  phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone, phLibNfc_StateNone,         phLibNfc_ConnChkDeactType,  phLibNfc_StateNone,       phLibNfc_StateSEListen},
    /*phLibNfc_StateRecv*/      {phLibNfc_StateRecv,       phLibNfc_StateIdle,  phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone, phLibNfc_StateNone,         phLibNfc_ConnChkDeactType,  phLibNfc_StateNone,       phLibNfc_StateSEListen},
    /*phLibNfc_StateSEListen*/  {phLibNfc_StateSEListen,   phLibNfc_StateIdle,  phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateIdle, phLibNfc_StateNone,         phLibNfc_ConnChkDeactType,  phLibNfc_StateSEListen,   phLibNfc_StateSEListen},
    /*phLibNfc_StateDummy*/     {phLibNfc_StateDummy,      phLibNfc_StateIdle,  phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone, phLibNfc_StateNone,         phLibNfc_StateNone,         phLibNfc_StateNone,       phLibNfc_StateNone},
};

static const phLibNfc_StateTransition_t phLibNfc_StateTransition[phLibNfc_STATE_MAX][phLibNfc_STATE_MAX] =
{
    /*State*/                   /*StateIdle*/               /*StateInit*/                   /*StateReset*/       /*StateDiscovery*/        /*StateDiscovered*/              /*StateTransceive*/              /*StateSend*/        /*StateRecv*/                   /* StateSEListen */           /*StateDummy*/
    /*State*/
    /*phLibNfc_StateIdle*/      {NULL,                      &phLibNfc_Idle2InitTransition,  NULL,                 NULL,                     NULL,                            NULL,                            NULL,                NULL,                           NULL,                         NULL                   },
    /*phLibNfc_StateInit*/      {&phLibNfc_Init2Idle,       &phLibNfc_DummyInit,            &phLibNfc_Init2Reset, &phLibNfc_Init2Discovery, NULL,                            NULL,                            NULL,                NULL,                           NULL,                         &phLibNfc_DummyFunc    },
    /*phLibNfc_StateReset*/     {NULL,                      &phLibNfc_Actv2Reset,           NULL,                 NULL,                     NULL,                            NULL,                            NULL,                NULL,                           NULL,                         &phLibNfc_DummyFunc    },
    /*phLibNfc_StateDiscovery*/ {&phLibNfc_Actv2Idle,       &phLibNfc_Actv2Init,            &phLibNfc_Actv2Reset, &phLibNfc_Actv2Discovery, &phLibNfc_StateDiscoveredEntry,  NULL,                            NULL,                &phLibNfc_StateDiscoveredEntry, &phLibNfc_StateDiscoveredEntry,&phLibNfc_DummyFunc   },/*Only Internal Event changes the state*/
    /*phLibNfc_StateDiscovered*/{&phLibNfc_Actv2Idle,       &phLibNfc_Actv2Init,            &phLibNfc_Actv2Reset, &phLibNfc_Actv2Discovery, NULL,                            &phLibNfc_Discovered2Transceive, NULL,                &phLibNfc_Discovered2Recv,      NULL,                         &phLibNfc_DummyFunc    },
    /*phLibNfc_StateTransceive*/{&phLibNfc_Actv2Idle,       &phLibNfc_Actv2Init,            &phLibNfc_Actv2Reset, &phLibNfc_Actv2Discovery, &phLibNfc_Transceive2Discovered, NULL,                            NULL,                NULL,                           NULL,                         &phLibNfc_DummyFunc    },
    /*phLibNfc_StateSend*/      {&phLibNfc_Actv2Idle,       &phLibNfc_Actv2Init,            &phLibNfc_Actv2Reset, &phLibNfc_Actv2Discovery, &phLibNfc_Send2Discovered,       NULL,                            NULL,                &phLibNfc_Send2Recv,            NULL,                         &phLibNfc_DummyFunc    },
    /*phLibNfc_StateRecv*/      {&phLibNfc_Actv2Idle,       &phLibNfc_Actv2Init,            &phLibNfc_Actv2Reset, &phLibNfc_Actv2Discovery, &phLibNfc_Recv2Discovered,       NULL,                            &phLibNfc_Recv2Send, NULL,                           NULL,                         &phLibNfc_DummyFunc    },
    /*phLibNfc_StateSEListen*/  {&phLibNfc_Actv2Idle,       &phLibNfc_Actv2Init,            &phLibNfc_Actv2Reset, &phLibNfc_LsnAc2Dscv_Def,  NULL,                           NULL,                            &phLibNfc_SEListen2Send, &phLibNfc_SEListen2Recv,    NULL,                         &phLibNfc_DummyFunc    },
    /*phLibNfc_StateDummy*/     {&phLibNfc_Actv2Idle,       NULL,                           NULL,                  NULL,                     NULL,                           NULL,                            NULL,                NULL,                           NULL,                         NULL                   },
};

NFCSTATUS phLibNfc_InitStateMachine(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        pCtx->StateContext.CurrState = phLibNfc_StateIdle;
        pCtx->StateContext.Flag = phLibNfc_StateTrasitionInvalid;
        PH_LOG_LIBNFC_INFO_STR("State machine flag: %!phLibNfc_TransitionFlag!", pCtx->StateContext.Flag);
        pCtx->StateContext.TrigEvent = phLibNfc_EventInvalid;
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_AddDeferredEvent(pphLibNfc_Context_t pCtx,phLibNfc_Event_t TrigEvent, void * Param1, void * Param2, void * Param3)
{
    NFCSTATUS Result = NFCSTATUS_FAILED;
    bool_t duplicateEvent = FALSE;
    phLibNfc_Event_Queue_t * pDeferredEvent;
    uint32_t dwDiscMode = NFC_DISC_CONFIG_DISCOVERY;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_CRIT_EXPECT(NULL != pCtx);
    if (NULL != pCtx)
    {
        pDeferredEvent = pCtx->StateContext.pDeferredEvent;
        while ( pDeferredEvent != NULL )
        {
            if (pDeferredEvent->Event == TrigEvent)
            {
                duplicateEvent = TRUE;
                pDeferredEvent->Param1 = (PVOID)dwDiscMode;
                pDeferredEvent->Param2 = Param2;
                pDeferredEvent->Param3 = Param3;
                Result = NFCSTATUS_SUCCESS;
                break;
            }
            else
            {
                pDeferredEvent = pDeferredEvent->Next;
            }
        }

        if ( ! duplicateEvent )
        {
            phLibNfc_Event_Queue_t * pNewEvent;
            pNewEvent = phOsalNfc_GetMemory(sizeof(*pNewEvent));
            if ( pNewEvent == NULL) {
                PH_LOG_LIBNFC_CRIT_STR("No memory");
                Result = NFCSTATUS_NOT_ENOUGH_MEMORY;
            }
            else
            {
                pNewEvent->Event = TrigEvent;
                pNewEvent->Next = NULL;
                pNewEvent->Param1 = Param1;
                pNewEvent->Param2 = Param2;
                pNewEvent->Param3 = Param3;
                Result = NFCSTATUS_SUCCESS;
            }

            if ( Result == NFCSTATUS_SUCCESS )
            {
                pDeferredEvent = pCtx->StateContext.pDeferredEvent;
                if ( pDeferredEvent == NULL) {
                    pCtx->StateContext.pDeferredEvent = pNewEvent;
                }
                else
                {
                    while ( pDeferredEvent->Next != NULL )
                    {
                        pDeferredEvent = pDeferredEvent->Next;
                    }
                    pDeferredEvent->Next = pNewEvent;
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return Result;
}

void
phLibNfc_ClearAllDeferredEvents(pphLibNfc_Context_t pCtx)
{
    phLibNfc_Event_Queue_t * pDeferredEvent;
    phLibNfc_Event_Queue_t * pDeferredEventNext;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_CRIT_EXPECT(NULL != pCtx);
    if(NULL != pCtx)
    {
        /* Retrieve head node of the deferred event list */
        pDeferredEvent = pCtx->StateContext.pDeferredEvent;
        while(pDeferredEvent != NULL)
        {
            pDeferredEventNext = pDeferredEvent->Next;
            /* Free node */
            phOsalNfc_FreeMemory(pDeferredEvent);
            pDeferredEvent = pDeferredEventNext;
        }
        pCtx->StateContext.pDeferredEvent = NULL;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

void
phLibNfc_DeferredEventHandler(
    _In_ void* pContext
    )
{
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phLibNfc_Event_t nextDeferredEvent ;
    phLibNfc_Event_Queue_t * pDeferredEvent;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if ( (NULL != pCtx) && (NULL != pCtx->StateContext.pDeferredEvent) )
    {
        pDeferredEvent = pCtx->StateContext.pDeferredEvent;
        nextDeferredEvent = pDeferredEvent->Event;
        pCtx->StateContext.pDeferredEvent = pDeferredEvent->Next;
        pDeferredEvent->Event = phLibNfc_EventInvalid; /* Just invalidate the memory contents */

        (void)phLibNfc_StateHandler(pContext,nextDeferredEvent,
            pDeferredEvent->Param1,
            pDeferredEvent->Param2,
            pDeferredEvent->Param3);

        phOsalNfc_FreeMemory(pDeferredEvent);
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
}

/*State must be updated to next state by the event from the lower layer*/
NFCSTATUS phLibNfc_StateHandler(void *pContext,
    phLibNfc_Event_t TrigEvent,
    void *Param1,
    void *Param2,
    void *Param3
    )
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS Result = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if (NULL != pCtx)
    {
        if ((TrigEvent < phLibNfc_EVENT_USER_MAX) &&
            ((pCtx->StateContext.Flag == phLibNfc_StateTransitionComplete) ||
                (pCtx->StateContext.Flag == phLibNfc_StateTrasitionInvalid)))
        {
            Result = phLibNfc_StateMachineHandleUsrEvent(pCtx, TrigEvent, Param1, Param2, Param3);

        }else if ((TrigEvent < phLibNfc_EVENT_USER_MAX) && (pCtx->StateContext.Flag == phLibNfc_StateTrasitionBusy))
        {
            Result = NFCSTATUS_BUSY;
            if (pCtx->StateContext.TrgtState == phLibNfc_StateReset)
            {
                Result = NFCSTATUS_SHUTDOWN;
            }
        }else if (TrigEvent > phLibNfc_EVENT_USER_MAX && TrigEvent < phLibNfc_EVENT_INT_MAX)/*IS INTERNAL EVENT*/
        {
            Result = phLibNfc_StateMachineHandleIntEvent(pCtx, TrigEvent, Param1, Param2, Param3);
            if (Result == NFCSTATUS_PENDING)
            {
                Result = NFCSTATUS_SUCCESS;
                pCtx->StateContext.Flag = phLibNfc_StateTrasitionBusy;
                PH_LOG_LIBNFC_INFO_STR("State machine flag: %!phLibNfc_TransitionFlag!", pCtx->StateContext.Flag);
                pCtx->StateContext.TrgtState = phLibNfc_StateDummy;
            }
        }

        PH_LOG_LIBNFC_INFO_STR("TrigEvent = %!phLibNfc_Event_t!", TrigEvent);
        PH_LOG_LIBNFC_INFO_STR("Current State =  %!phLibNfc_State!", pCtx->StateContext.CurrState);
        PH_LOG_LIBNFC_INFO_STR("Target State =  %!phLibNfc_State!", pCtx->StateContext.TrgtState);

    }else
    {
        Result = NFCSTATUS_INVALID_PARAMETER;
    }

    if ( NFCSTATUS_BUSY == Result ) {
        /*if Busy, add data to a deferred queue */
        if (TrigEvent <= phLibNfc_EventDiscovery) {
            PH_LOG_LIBNFC_CRIT_X32MSG("Extra Event",TrigEvent);
            /* FIXME, handle the return value of phLibNfc_AddDeferredEvent */
            Result = phLibNfc_AddDeferredEvent(pCtx,TrigEvent,Param1,Param2,Param3);
        }
    }
    else {
        if (pCtx != NULL &&
            pCtx->StateContext.pDeferredEvent != NULL &&
            pCtx->StateContext.Flag == phLibNfc_StateTransitionComplete) {
            NFCSTATUS deferRetVal = 0;

            /* generate timer event */
            PH_LOG_LIBNFC_CRIT_STR("Fire Deferred Event");

            deferRetVal = phOsalNfc_QueueDeferredCallback(phLibNfc_DeferredEventHandler,
                                                          pCtx);
            if ( deferRetVal  == NFCSTATUS_SUCCESS ) {
                Result = NFCSTATUS_PENDING;
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Failed to start deferred event handler");
                Result = NFCSTATUS_FAILED;
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return Result;
}

static NFCSTATUS phLibNfc_StateMachineHandleUsrEvent(void *pContext,
                                phLibNfc_Event_t TrigEvent,
                                void *Param1,
                                void *Param2,
                                void *Param3
                                )
{
    pphLibNfc_Context_t pCtx = pContext;
    phLibNfc_State_t CurrState = phLibNfc_StateInvalid;
    phLibNfc_State_t TrgtState = phLibNfc_StateInvalid;
    uint8_t bValidTransition;
    NFCSTATUS Result = 0;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        if((TrigEvent < phLibNfc_EVENT_USER_MAX) &&
            ((pCtx->StateContext.Flag == phLibNfc_StateTransitionComplete) ||
            (pCtx->StateContext.Flag == phLibNfc_StateTrasitionInvalid)))
        {
            CurrState = pCtx->StateContext.CurrState;

            if((CurrState >= phLibNfc_StateIdle) &&
               (CurrState < phLibNfc_STATE_MAX) &&
               (TrigEvent >= phLibNfc_EventInit) &&
               (TrigEvent < phLibNfc_EVENT_MAX))
            {
                TrgtState = phLibNfc_State2Event[CurrState][TrigEvent];
                TrgtState = phLibNfc_FindTrgtState(pCtx, CurrState, TrgtState,Param2);
                if (TrgtState >= phLibNfc_StateNone)
                {
                    /*Get Proper status code*/
                    Result = phLibNfc_StateStatusConversion(pCtx->StateContext.CurrState,
                                                            pCtx->StateContext.TrgtState,
                                                            TrigEvent);
                    /*No Action to be taken Event ocuured can not be handled in this state*/
                    /*set to invalid state*/
                }else if ((TrgtState >= phLibNfc_StateIdle) && (TrgtState < phLibNfc_STATE_MAX))
                {
                    if(NULL != phLibNfc_StateFptr[CurrState].pfExit)
                    {
                        Result = phLibNfc_StateFptr[CurrState].pfExit(pCtx,Param1,Param2,Param3);
                    }
                    bValidTransition = phLibNfc_ValidateTransition(CurrState,TrgtState);
                    if(1 == bValidTransition)
                    {
                        /* Call Transition function */
                        Result = phLibNfc_StateTransition[CurrState][TrgtState](pCtx, Param1, Param2, Param3);
                    }
                    if(NFCSTATUS_PENDING == Result )
                    {
                          pCtx->StateContext.Flag = phLibNfc_StateTrasitionBusy;
                          PH_LOG_LIBNFC_INFO_STR("State machine flag: %!phLibNfc_TransitionFlag!", pCtx->StateContext.Flag);
                          pCtx->StateContext.TrgtState = TrgtState;
                    }
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return Result;
}

static uint8_t phLibNfc_ValidateTransition(phLibNfc_State_t CurrState,
                                           phLibNfc_State_t  TgtState)
{
    uint8_t bStatus = 0;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((phLibNfc_STATE_MAX > CurrState) && (0 <= CurrState) && (phLibNfc_STATE_MAX > TgtState) && (0 <= TgtState))
    {
        if((CurrState != TgtState) ||
            ((phLibNfc_StateDiscovery == CurrState) && (phLibNfc_StateDiscovery == TgtState)) ||
            ((phLibNfc_StateInit == CurrState) && (phLibNfc_StateInit == TgtState)))
        {
            PH_LOG_LIBNFC_INFO_STR("LibNfc CurrState = %!phLibNfc_State!", (uint32_t)CurrState);
            PH_LOG_LIBNFC_INFO_STR("LibNfc TrgtState = %!phLibNfc_State!", (uint32_t)TgtState);

            bStatus = (NULL != phLibNfc_StateTransition[CurrState][TgtState]) ? 1 : 0;

            PH_LOG_LIBNFC_INFO_STR("LibNfc Transition required = %d", bStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return bStatus;
}

static NFCSTATUS phLibNfc_StateMachineHandleIntEvent(void *pContext,
                                phLibNfc_Event_t TrigEvent,
                                void *Param1,
                                void *Param2,
                                void *Param3
                                )
{
    pphLibNfc_Context_t pCtx = pContext;
    phLibNfc_State_t CurrState = phLibNfc_StateInvalid;
    phLibNfc_State_t TrgtState = phLibNfc_StateInvalid;
    NFCSTATUS Result = 0;
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pCtx)
    {
        if(((TrigEvent == phLibNfc_EventDeviceActivated) || (TrigEvent == phLibNfc_EventDeviceDiscovered)\
            || ( TrigEvent == phLibNfc_EventPCDActivated))
            && (phLibNfc_StateTrasitionBusy == pCtx->StateContext.Flag))
        {
            PH_LOG_LIBNFC_CRIT_STR("Received internal event: Activated/Discovered, transition status BUSY");
            Result = NFCSTATUS_FAILED;
        }
        else
        {
            if(TrigEvent > phLibNfc_EVENT_USER_MAX && TrigEvent < phLibNfc_EVENT_INT_MAX)/*IS INTERNAL EVENT*/
            {
                if(pCtx->StateContext.Flag == phLibNfc_StateTrasitionBusy)
                {
                    /*Check for expected internal event*/
                    /*if expected event is same then status is success else....*/
                    TrgtState = phLibNfc_IntEvent2State[pCtx->StateContext.TrgtState][TrigEvent - phLibNfc_EVENT_USER_MAX -1];
                }else if((pCtx->StateContext.Flag == phLibNfc_StateTransitionComplete) ||
                    (pCtx->StateContext.Flag == phLibNfc_StateTrasitionInvalid))
                {
                    CurrState = pCtx->StateContext.CurrState;

                    if(CurrState < phLibNfc_STATE_MAX && CurrState >= phLibNfc_StateIdle)
                    {
                        /*Call Entry function or may needed to have connector*/
                        TrgtState = phLibNfc_IntEvent2State[CurrState][TrigEvent - phLibNfc_EVENT_USER_MAX - 1];
                        /*TrgtState May be connector*/
                        TrgtState = phLibNfc_FindTrgtState(pCtx, CurrState, TrgtState, Param2);

                        if(TrgtState < phLibNfc_STATE_MAX && TrgtState >= phLibNfc_StateIdle)
                        {
                            if (CurrState != TrgtState && /*Event triggered by lower layer needs transition*/
                                (NULL != phLibNfc_StateTransition[CurrState][TrgtState]))
                            {
                                /*execute the transition and event handler function*/
                                /*transition to next*/
                                Result = phLibNfc_StateTransition[CurrState][TrgtState](pCtx, Param1, Param2, Param3);
                            }
                        }
                        else
                        {
                            Result = NFCSTATUS_INVALID_STATE;
                            PH_LOG_LIBNFC_CRIT_STR("Transition can't be executed to TrgtState = %!phLibNfc_State!", TrgtState);
                            /*No Action to be taken Event occurred can not be handled in this state*/
                            /*set to invalid state*/
                        }
                    }
                    else
                    {
                        Result = NFCSTATUS_INVALID_STATE;
                        PH_LOG_LIBNFC_CRIT_STR("Unexpected CurrState = %!phLibNfc_State!", CurrState);
                    }
                }
                if ((TrgtState < phLibNfc_STATE_MAX) && (TrgtState >= phLibNfc_StateIdle))
                {
                    /*Call entry function*/
                    if(NULL != phLibNfc_StateFptr[TrgtState].pfEntry)
                    {
                        Param3 = (void *)&Result;
                        Result = phLibNfc_StateFptr[TrgtState].pfEntry(pContext,Param1,Param2,Param3);
                    }
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("Out of bound value of %!phLibNfc_State!",TrgtState);
                }
                if(phLibNfc_StateDummy == TrgtState ||
                   phLibNfc_StateNone == TrgtState )
                {
                }
                else
                {
                    /*Needs to invoke Callback*/
                    pCtx->StateContext.CurrState = TrgtState;
                }
                pCtx->StateContext.TrgtState = phLibNfc_StateInvalid;
                pCtx->StateContext.Flag = phLibNfc_StateTransitionComplete;
                PH_LOG_LIBNFC_INFO_STR("State machine flag: %!phLibNfc_TransitionFlag!", pCtx->StateContext.Flag);
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return Result;
}
static phLibNfc_State_t phLibNfc_FindTrgtState(void *pContext,
                                               phLibNfc_State_t CurrState,
                                               phLibNfc_State_t TrgtState,
                                               void *pInfo)
{
    pphLibNfc_Context_t pCtx = pContext;
    uint8_t Index = 0;
    NFCSTATUS Result;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        do
        {
            if((TrgtState > phLibNfc_STATE_MAX) && (TrgtState < phLibNfc_STATE_CONN_MAX)) /*Target state is connector*/
            {
                Index = (uint8_t)(TrgtState - phLibNfc_STATE_MAX - 1);
                Result = phLibNfc_Connector[Index](pCtx,pInfo);
                if (0 == Result)
                {
                    TrgtState = phLibNfc_State2State[CurrState][(uint8_t)(Index * 2)];
                }else
                {
                    TrgtState = phLibNfc_State2State[CurrState][((uint8_t)Index * 2) + 1];
                }
            }else if (TrgtState >= phLibNfc_StateNone)
            {
                /*No Action to be taken Event ocuured can not be handled in this state*/
                /*set to invalid state*/
                break;
            }
        }while(TrgtState >= phLibNfc_STATE_MAX);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return TrgtState;
}

static NFCSTATUS phLibNfc_DummyFunc(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pLibContext = (pphLibNfc_Context_t)pContext;
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phLibNfc_DummyInfo_t *pInfo = (phLibNfc_DummyInfo_t *)Param2;
    pphNciNfc_RfDiscConfigParams_t pRfConfParam = NULL;
    uint8_t bNumRtngEntries;
    phLibNfc_Sequence_t *pSetModeSeq = NULL;
    phLibNfc_Handle pSeHandle;
    uint8_t bIndex;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    UNUSED(Param3);
    if((NULL != pLibContext) && (NULL != pInfo))
    {
        switch (pInfo->Evt)
        {
            case phLibNfc_DummyEventSetMode:
            {
                phLibNfc_eSE_ActivationMode ActMode = *(phLibNfc_eSE_ActivationMode *)(pInfo->Params);
                phNciNfc_NfceeModes_t eNfceeMode = PH_NCINFC_NFCEEDISC_UNKNOWN;
                pSeHandle = (phLibNfc_Handle)Param1;
                wStatus = NFCSTATUS_SUCCESS;

                if (phLibNfc_SE_ActModeOn == ActMode)
                {
                    eNfceeMode = PH_NCINFC_EXT_NFCEEMODE_ENABLE; /*Enable SE*/
                }
                else
                {
                    eNfceeMode = PH_NCINFC_EXT_NFCEEMODE_DISABLE; /*Disable SE*/
                }

                if(NULL != (void *)pSeHandle)
                {
                    for (bIndex = 0; bIndex < PHHCINFC_TOTAL_NFCEES; bIndex++)
                    {
                        if (pSeHandle == pLibContext->tSeInfo.tSeList[bIndex].hSecureElement)
                        {
                            pLibContext->sSeContext.pActiveSeInfo = &pLibContext->tSeInfo.tSeList[bIndex];
                            break;
                        }
                    }

                    if(PH_NCINFC_EXT_NFCEEMODE_DISABLE == eNfceeMode)
                    {
                        pSetModeSeq = gphLibNfc_SetSeModeSeq;
                    }
                    else
                    {
                        if(phLibNfc_SE_ActModeOff == pLibContext->sSeContext.pActiveSeInfo->eSE_ActivationMode)
                        {
                            pSetModeSeq = gphLibNfc_SetSeModeSeq;
                        }
                        eNfceeMode = PH_NCINFC_EXT_NFCEEMODE_ENABLE;
                    }

                    pLibContext->sSeContext.eNfceeMode = eNfceeMode;

                    /* If there is no existing sequence to execute, return SUCCESS assuming that
                       mode set and power mode changes are not required */
                    if(NULL != pSetModeSeq)
                    {
                        /* Further information may come from the NFCC */
                        pLibContext->dwHciInitDelay = PHHCINFC_NFCEE_DISCOVERY_DEFAULT_NTF_DELAY;
                        PHLIBNFC_INIT_SEQUENCE(pLibContext,pSetModeSeq);
                        /* Start discover sequence */
                        wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
                    }
                    else
                    {
                        pLibContext->sSeContext.pActiveSeInfo->hSecureElement = pSeHandle;
                        pLibContext->sSeContext.pActiveSeInfo->eSE_ActivationMode =
                            pLibContext->sSeContext.eActivationMode;
                        /* No change in power mode, and NFCEE mode, return SUCCESS */
                        wStatus = NFCSTATUS_SUCCESS;
                    }
                }
            }
            break;
            case phLibNfc_DummyEventSetP2PConfigs:
            {
                pRfConfParam = (pphNciNfc_RfDiscConfigParams_t) pInfo->Params;
                if(NULL != pRfConfParam)
                {
                    wStatus = phNciNfc_SetConfigRfParameters(pLibContext->sHwReference.pNciHandle,
                        pRfConfParam,(pphNciNfc_IfNotificationCb_t)&phLibNfc_P2pConfigParamsCb,
                        (void *)pRfConfParam);
                }
                else
                {
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                }
            }
            break;
            case phLibNfc_DummyEventFelicaChkPresExtn:
            {
                PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_Felica_CheckPresSeq);
                wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,Param3);
                if( NFCSTATUS_PENDING != wStatus )
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            break;
            case phLibNfc_DummyEventIsoDepChkPresExtn:
            {
                PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_IsoDep_CheckPresSeq);
                wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
                if( NFCSTATUS_PENDING != wStatus )
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            break;
            case phLibNfc_DummyEventChkPresMFC:
            {
                /* If Authentication CMD is already stored, then use this command for presence check*/
                if((pLibContext->tMfcInfo.cmd == phNfc_eMifareAuthentA) ||
                    (pLibContext->tMfcInfo.cmd == phNfc_eMifareAuthentB)||
                    (pLibContext->tMfcInfo.cmd == phNfc_eMifareAuthKeyNumA)||
                    (pLibContext->tMfcInfo.cmd == phNfc_eMifareAuthKeyNumB))
                {
                    PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_MFCSendAuthCmdForPresChk);
                }
                else
                {
                    /*Authentication CMD not available, deactivate to Sleep and Select the tag to know
                      the presence */
                    if(pLibContext->bReactivation_Flag == PH_LIBNFC_REACT_ONLYSELECT)
                    {
                        PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_ReActivate_MFCSeq2Select);
                    }
                    else
                    {
                        PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_ReActivate_MFCSeq2);
                    }
                }
                wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
            }
            break;
            case phLibNfc_DummyEventSetRtngCfg:
            {
                if((phLibNfc_StateInit == pLibContext->StateContext.CurrState) &&
                    (phLibNfc_StateTransitionComplete == pLibContext->StateContext.Flag))
                {
                    bNumRtngEntries = *(uint8_t *)pInfo->Params;
                    if((NULL != pLibContext->sSeContext.pRoutingCfgBuffer) &&
                        (0 != bNumRtngEntries))
                    {
                        /* Invoke NCI layer */
                        wStatus = phNciNfc_SetRtngTableConfig(pLibContext->sHwReference.pNciHandle,\
                                                              bNumRtngEntries,\
                                                              pLibContext->sSeContext.pRoutingCfgBuffer,\
                                                              &phLibNfc_ConfigRoutingTableCb,\
                                                              pLibContext);
                    }
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("Routing table can only be configured in Initialized state");
                    wStatus = NFCSTATUS_NOT_ALLOWED;
                }
            }
            break;
            default:
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static uint8_t phLibNfc_CheckCurrentMode(pphLibNfc_LibContext_t pLibCtx)
{
    uint8_t bReturn = PH_LIBNFC_NO_REMOTEDEVICE;
    uint8_t bNumOfDevices = 0;
    phLibNfc_Handle pConnDevice = (phLibNfc_Handle)NULL;
    uint8_t bCount;
    phNfc_eRemDevType_t eRemDevType;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((0 != pLibCtx->dev_cnt) && (NULL != pLibCtx->Connected_handle))
    {
        bNumOfDevices = pLibCtx->dev_cnt;
        pConnDevice = (phLibNfc_Handle)pLibCtx->Connected_handle;
        for(bCount = 0; bCount < bNumOfDevices; bCount++)
        {
            /* Match connected device handle with the list of discovered devices handles*/
            if(pConnDevice == pLibCtx->psRemoteDevList[bCount].hTargetDev)
            {
                eRemDevType = pLibCtx->psRemoteDevList[bCount].psRemoteDevInfo->RemDevType;
                /* Get the current mode based on the type of te remote device we are connected to */
                switch(eRemDevType)
                {
                    case phNfc_eISO14443_A_PCD:
                    case phNfc_eISO14443_B_PCD:
                    case phNfc_eISO14443_BPrime_PCD:
                    case phNfc_eFelica_PCD:
                    case phNfc_eJewel_PCD:
                    case phNfc_eISO15693_PCD:
                    case phNfc_ePCD_DevType:
                    case phNfc_ePICC_DevType:
                    case phNfc_eISO14443_A_PICC:
                    case phNfc_eISO14443_4A_PICC:
                    case phNfc_eISO14443_3A_PICC:
                    case phNfc_eMifare_PICC:
                    case phNfc_eISO14443_B_PICC:
                    case phNfc_eISO14443_4B_PICC:
                    case phNfc_eISO14443_BPrime_PICC:
                    case phNfc_eFelica_PICC:
                    case phNfc_eJewel_PICC:
                    case phNfc_eISO15693_PICC:
                    case phNfc_eKovio_PICC:
                    case phNfc_eNfcIP1_Target:
                        bReturn = PH_LIBNFC_CURRENTMODE_POLL;
                        break;
                    case phNfc_eNfcIP1_Initiator:
                        /* Add new case statements for NFCEE and other listen mode device types */
                        bReturn = PH_LIBNFC_CURRENTMODE_LISTEN;
                        break;
                    default:
                        /* Invalid remote device */
                        bReturn = PH_LIBNFC_INVALID_REMOTEDEVICE;
                        break;
                }
                break;
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return bReturn;
}

static uint8_t phLibNfc_ConnIsRfListnerRegisterd(void *pContext,void *pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t bRetVal = 0x01;
    uint8_t bIndex = 0x00;
    uint8_t bIndex1 = 0x00;
    uint8_t bTagRegNotify = 0;
    pphLibNfc_LibContext_t pLibContext = NULL;
    pphNciNfc_DeviceInfo_t pNciDevInfo = NULL ;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pContext)
    {
        pLibContext = (pphLibNfc_LibContext_t)pContext;
        pNciDevInfo = (pphNciNfc_DeviceInfo_t )pLibContext->pInfo;
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }

    if( (NULL != pLibContext) &&\
        (NULL != pNciDevInfo) && (wStatus == NFCSTATUS_SUCCESS))
    {
        /* Check if the any tag/CE/P2P is registered */
        for(bIndex = 0; bIndex < pNciDevInfo->dwNumberOfDevices ; bIndex++)
        {
            switch(pNciDevInfo->pRemDevList[bIndex]->eRFTechMode)
            {
                case phNciNfc_NFCA_Poll:
                case phNciNfc_NFCA_Kovio_Poll:
                case phNciNfc_NFCA_Active_Poll:
                {
                    wStatus = phLibNfc_ChkRfListnerforNFCAPoll(pContext,\
                                                                pNciDevInfo,\
                                                                bIndex);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        bRetVal = 0x00;
                        bIndex1++;
                        bTagRegNotify = 0x01;
                    }
                    else
                    {
                        bRetVal = 0x01;
                    }
                }
                break;
                case phNciNfc_NFCB_Poll:
                {
                    wStatus = phLibNfc_ChkRfListnerforNFCBPoll(pContext,\
                                                                pNciDevInfo,\
                                                                bIndex);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        bRetVal = 0x00;
                        bIndex1++;
                        bTagRegNotify = 0x01;
                    }
                    else
                    {
                        bRetVal = 0x01;
                    }
                }
                break;
                case phNciNfc_NFCF_Poll:
                case phNciNfc_NFCF_Active_Poll:
                {
                    wStatus = phLibNfc_ChkRfListnerforNFCFPoll(pContext,\
                                                                pNciDevInfo,\
                                                                bIndex);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        bRetVal = 0x00;
                        bIndex1++;
                        bTagRegNotify = 0x01;
                    }
                    else
                    {
                        bRetVal = 0x01;
                    }
                }
                break;
                case phNciNfc_NFCISO15693_Poll:
                {
                    wStatus = phLibNfc_ChkRfListnerforNFCISO15693Poll(pContext,\
                                                                        pNciDevInfo,\
                                                                        bIndex);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        bRetVal = 0x00;
                        bIndex1++;
                        bTagRegNotify = 0x01;
                    }
                    else
                    {
                        bRetVal = 0x01;
                        }
                    }
                    break;
                    case phNciNfc_NFCA_Listen:
                    case phNciNfc_NFCA_Active_Listen:
                    {
                    wStatus = phLibNfc_ChkRfListnerforNFCAListen(pContext,\
                                                                    pNciDevInfo,\
                                                                    bIndex);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        bRetVal = 0x00;
                        bIndex1++;
                        bTagRegNotify = 0x01;
                    }
                    else
                    {
                        bRetVal = 0x01;
                    }
                }
                break;
                case phNciNfc_NFCB_Listen:
                {
                    wStatus = phLibNfc_ChkRfListnerforNFCBListen(pContext,\
                                                                    pNciDevInfo,\
                                                                    bIndex);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        bRetVal = 0x00;
                        bIndex1++;
                        bTagRegNotify = 0x01;
                    }
                    else
                    {
                        bRetVal = 0x01;
                    }
                }
                break;
                case phNciNfc_NFCF_Listen:
                case phNciNfc_NFCF_Active_Listen:
                {
                    wStatus = phLibNfc_ChkRfListnerforNFCFListen(pContext,\
                                                                    pNciDevInfo,\
                                                                    bIndex);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        bRetVal = 0x00;
                        bIndex1++;
                        bTagRegNotify = 0x01;
                    }
                    else
                    {
                        bRetVal = 0x01;
                    }
                }
                break;
                default:
                {
                    bRetVal = 0x01;
                }
                break;
            }
        }
        if(bTagRegNotify)
        {
            for(bIndex = 0; bIndex < pNciDevInfo->dwNumberOfDevices ; bIndex++)
            {
                pLibContext->Disc_handle[bIndex] = pNciDevInfo->pRemDevList[bIndex];
                pLibContext->Map_Handle[bIndex].pNci_RemoteDev_List = pNciDevInfo->pRemDevList[bIndex];
                pLibContext->bDiscovery_Notify_Enable = 0x00;
            }
            bRetVal = 0x00;
            pLibContext->dev_cnt=bIndex;
        }
        else
        {
            pLibContext->bDiscovery_Notify_Enable = 0x01;
        }
    }
    else
    {
        bRetVal = 0x01;
        pLibContext->bDiscovery_Notify_Enable = 0x01;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return bRetVal;
}

static NFCSTATUS phLibNfc_RfListnerRegisterd(void *pContext,uint32_t DevType,uint8_t bSak)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pLibContext)
    {
        switch(DevType)
        {
        case phNfc_eMifare_PICC:
            {
                switch(bSak)
                {
                    case PHLIBNFC_MIFAREUL_SAK:
                    {
                        if(pLibContext->RegNtfType.MifareUL == TRUE)
                        {
                            wStatus = NFCSTATUS_SUCCESS;
                        }
                        else
                        {
                            wStatus = NFCSTATUS_FAILED;
                        }
                    }
                    break;
                    case PHLIBNFC_MFC1K_SAK:
                    case PHLIBNFC_MFC4K_SAK:
                    case PHLIBNFC_MFCMINI_SAK:
                    case PHLIBNFC_MFC1K_WITHSAK1:
                    case PHLIBNFC_MFC1K_WITHSAK88:
                    case PHLIBNFC_MFC4K_WITHSAK98:
                    case PHLIBNFC_MFC4K_WITHSAKB8:
                    case PHLIBNFC_MFC1K_WITHSAK28:
                    case PHLIBNFC_MFC4K_WITHSAK38:
                    {
                        if(pLibContext->RegNtfType.MifareStd == TRUE )
                        {
                            wStatus = NFCSTATUS_SUCCESS;
                        }
                        else
                        {
                            wStatus = NFCSTATUS_FAILED;
                        }
                    }
                    break;
                    default:
                    {
                        wStatus = NFCSTATUS_FAILED;
                    }
                    break;
                }
             }
             break;
         case phNfc_eJewel_PICC:
            {
                if(pLibContext->RegNtfType.Jewel != TRUE )
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            break;
        case phNfc_eISO14443_4A_PICC:
            {
                if(pLibContext->RegNtfType.ISO14443_4A != TRUE)
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            break;
        case phNfc_eISO14443_4B_PICC:
            {
                if(pLibContext->RegNtfType.ISO14443_4B != TRUE)
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            break;
        case phNfc_eNfcIP1_Target:
        case phNfc_eNfcIP1_Initiator:
            {
                if(pLibContext->RegNtfType.NFC != TRUE)
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            break;
        case phNfc_eFelica_PICC:
            {
                if(pLibContext->RegNtfType.Felica != TRUE)
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            break;
        case phNfc_eISO14443_A_PCD:
        case phNfc_eISO14443_B_PCD:
            {
                if(pLibContext->CBInfo.pCeHostNtfCb == NULL)
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            break;
        case phNfc_eISO15693_PICC:
            {
                if(pLibContext->RegNtfType.ISO15693 != TRUE)
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            break;
        case phNfc_eKovio_PICC:
        {
            if (pLibContext->RegNtfType.Kovio != TRUE)
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        break;
        default:
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    else
    {
         wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static uint8_t phLibNfc_ConnChkDevType(void *pContext,void *pInfo)
{
    pphLibNfc_LibContext_t pCtx = (pphLibNfc_LibContext_t)pContext;
    uint8_t bRet = 0;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        /*Device Type could be Listen or Poll which depends on the Technology and Mode*/
        if((pCtx->bTechMode == 1) || (pCtx->bTotalNumDev > 1))
        {
            /*Poll Mode*/
            bRet = 1;
        }else
        {
            /*Listen Mode*/
            bRet = 0;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return bRet;
}

static uint8_t phLibNfc_ConnChkTgtType(void *pContext,void *pInfo)
{
    pphLibNfc_LibContext_t pCtx = (pphLibNfc_LibContext_t)pContext;
    uint8_t bRet = 0;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        /*Check the RF Interface */
        if((pCtx->bRfInterface == 0) || (pCtx->bTotalNumDev > 1))
        {
            /*NFC DEP*/
            bRet = 0;
        }else
        {
            /*ISO DEP*/
            bRet = 1;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return bRet;
}

static uint8_t phLibNfc_ChkDeActType(void *pContext,void *pInfo)
{
    uint8_t bRetVal=0x00;
    pphLibNfc_LibContext_t pLibContext;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    pLibContext=(pphLibNfc_LibContext_t )pContext;

    if((pLibContext->DiscDisconnMode == NFC_DISCONN_STOP_DISCOVERY) ||\
        (pLibContext->DiscDisconnMode == NFC_INTERNAL_STOP_DISCOVERY))
    {
        bRetVal=0x01;/*False case*/
    }
    else if((pLibContext->DiscDisconnMode == NFC_DISCONN_CONTINUE_DISCOVERY) ||
        (pLibContext->DiscDisconnMode == NFC_DISCONN_RESTART_DISCOVERY) ||
        (pLibContext->DiscDisconnMode == NFC_INTERNAL_CONTINUE_DISCOVERY))
    {
        bRetVal=0x00;/*True case*/
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return bRetVal;
}

uint8_t phLibNfc_ChkDiscoveryTypeAndMode(void *DidcMode,void *pInfo, void *TrigEvnt)
{
    uint8_t                     bRetVal = 1;
    pphLibNfc_Context_t         pLibContext = phLibNfc_GetContext();
    phLibNfc_sADD_Cfg_t         *pDiscConfig = pInfo;
    phNfc_eDiscAndDisconnMode_t *pDiscMode = NULL;
    phLibNfc_Event_t            *pTrigEvent = NULL;
    phNfc_sPollDevInfo_t *pPollInfo = NULL;
    pTrigEvent = (phLibNfc_Event_t *)TrigEvnt;
    pDiscMode = (phNfc_eDiscAndDisconnMode_t *)DidcMode;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pDiscConfig)
    {
        pPollInfo = &(pDiscConfig->PollDevInfo.PollCfgInfo);
        if((1 == pPollInfo->EnableIso14443A) ||   \
           (1 == pPollInfo->EnableIso14443B) ||   \
           (1 == pPollInfo->EnableFelica212) ||   \
           (1 == pPollInfo->EnableFelica424) ||   \
           (1 == pPollInfo->EnableIso15693)  ||   \
           (1 == pPollInfo->EnableKovio))
        {
            PH_LOG_LIBNFC_INFO_STR("Poll is enabled");
            bRetVal = 0;
        }
        else if(((phNfc_eDefaultP2PMode != pDiscConfig->NfcIP_Mode) && \
            (phNfc_eInvalidP2PMode != pDiscConfig->NfcIP_Mode) ) && \
            ((phNfc_ePassive106 == (phNfc_ePassive106 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_eP2P_ALL == pDiscConfig->NfcIP_Mode) ||
            (phNfc_ePassive212 == (phNfc_ePassive212 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_eActive106 == (phNfc_eActive106 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_eActive212 == (phNfc_eActive212 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_eActive424 == (phNfc_eActive424 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_ePassive424 == (phNfc_ePassive424 & pDiscConfig->NfcIP_Mode))))
        {
            PH_LOG_LIBNFC_INFO_STR("P2P Initiator is enabled");
            bRetVal = 0;
        }
        else if(0 == pPollInfo->DisableCardEmulation)
        {
            if( (pLibContext->tSeInfo.bSeCount > 0) || (pLibContext->bHceSak > 0) )
            {
                PH_LOG_LIBNFC_INFO_STR("Card emulation is enabled");
                bRetVal = 0;
            }
        }
        else if(0 == pDiscConfig->NfcIP_Tgt_Disable)
        {
            PH_LOG_LIBNFC_INFO_STR("P2P Target mode is enabled");
            bRetVal = 0;
        }
        else
        {
            if((pLibContext->StateContext.CurrState == phLibNfc_StateDiscovery) &&
                (*pTrigEvent == phLibNfc_EventDiscovery) && (*pDiscMode == NFC_DISC_CONFIG_DISCOVERY))
            {
                bRetVal = 0;

            }
            else
            {
                bRetVal = 1;
            }
        }

        if(((bRetVal == 0) && (pLibContext->StateContext.CurrState == phLibNfc_StateDiscovery) &&
            (*pTrigEvent == phLibNfc_EventDiscovery) && (*pDiscMode != NFC_DISC_CONFIG_DISCOVERY)) ||
            ((bRetVal == 1) && (pLibContext->StateContext.CurrState == phLibNfc_StateInit) &&
            (*pTrigEvent == phLibNfc_EventDiscovery) && (*pDiscMode != NFC_DISC_CONFIG_DISCOVERY)) )
        {
            bRetVal = 1;
        }
        else
        {
            bRetVal = 0;
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameters");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return bRetVal;
}
uint8_t phLibNfc_ChkDiscoveryType(void *pContext,void *pInfo)
{
    uint8_t bRetVal = 1;
    phLibNfc_sADD_Cfg_t *pDiscConfig = pInfo;
    phNfc_sPollDevInfo_t *pPollInfo = NULL;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pDiscConfig)
    {
        pPollInfo = &(pDiscConfig->PollDevInfo.PollCfgInfo);
        if((1 == pPollInfo->EnableIso14443A) ||   \
           (1 == pPollInfo->EnableIso14443B) ||   \
           (1 == pPollInfo->EnableFelica212) ||   \
           (1 == pPollInfo->EnableFelica424) ||   \
           (1 == pPollInfo->EnableIso15693)  ||   \
           (1 == pPollInfo->EnableKovio))
        {
            PH_LOG_LIBNFC_INFO_STR("Poll is enabled");
            bRetVal = 0;
        }
        else if(((phNfc_eDefaultP2PMode != pDiscConfig->NfcIP_Mode) && \
            (phNfc_eInvalidP2PMode != pDiscConfig->NfcIP_Mode) ) && \
            ((phNfc_ePassive106 == (phNfc_ePassive106 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_eP2P_ALL == pDiscConfig->NfcIP_Mode) ||
            (phNfc_ePassive212 == (phNfc_ePassive212 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_eActive106 == (phNfc_eActive106 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_eActive212 == (phNfc_eActive212 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_eActive424 == (phNfc_eActive424 & pDiscConfig->NfcIP_Mode)) ||
            (phNfc_ePassive424 == (phNfc_ePassive424 & pDiscConfig->NfcIP_Mode))))
        {
            PH_LOG_LIBNFC_INFO_STR("P2P Initiator is enabled");
            bRetVal = 0;
        }
        else if(0 == pPollInfo->DisableCardEmulation)
        {
            if( (pLibContext->tSeInfo.bSeCount > 0) || (pLibContext->bHceSak > 0) )
            {
                PH_LOG_LIBNFC_INFO_STR("Card emulation is enabled");
                bRetVal = 0;
            }
        }
        else if(0 == pDiscConfig->NfcIP_Tgt_Disable)
        {
            PH_LOG_LIBNFC_INFO_STR("P2P Target mode is enabled");
            bRetVal = 0;
        }
        else
        {
            /* None enabled */
            bRetVal = 1;
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameters");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return bRetVal;
}

static NFCSTATUS phLibNfc_StateStatusConversion(phLibNfc_State_t CurrState,
                                         phLibNfc_State_t TrgtState,
                                         phLibNfc_Event_t TrigEvent
                                         )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    if((phLibNfc_StateInit == CurrState) && (phLibNfc_EventInit == TrigEvent)) /*Re-init requestd*/
    {
        wStatus = NFCSTATUS_ALREADY_INITIALISED;
    }else if(phLibNfc_StateReset == TrgtState)
    {
        wStatus = NFCSTATUS_SHUTDOWN;
    }else
    {
        wStatus = NFCSTATUS_INVALID_STATE;
    }
    return wStatus;
}

NFCSTATUS phLibNfc_StatePrepareShutdown(void *pContext)
{
    pphLibNfc_LibContext_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    if(NULL != pCtx)
    {
        /*Set sequence max to 0 and next to 0xFF*/
        pCtx->SeqMax = 0;
        pCtx->SeqNext = 0xFF;
        if(phLibNfc_StateIdle == pCtx->StateContext.CurrState)
        {
            pCtx->StateContext.CurrState = pCtx->StateContext.TrgtState; /*Just insure that current
                                             it is initialised, even if NFCC is not in init State */
        }
        if(pCtx->StateContext.TrgtState == phLibNfc_StateReset)
        {
            wStatus = NFCSTATUS_BUSY; /*Shutdown is alrady in progress*/
            PH_LOG_LIBNFC_INFO_STR("Shutdown already in progress");
        }else
        {
            pCtx->StateContext.Flag = phLibNfc_StateTransitionComplete;
            PH_LOG_LIBNFC_INFO_STR("State machine flag: %!phLibNfc_TransitionFlag!", pCtx->StateContext.Flag);
            pCtx->StateContext.TrgtState = phLibNfc_StateNone;
            pCtx->SeqMax = 0;
            pCtx->SeqNext = 0;
            pCtx->pSeqHandler = NULL;
            PH_LOG_LIBNFC_INFO_STR("State Machine is ready for shutdown");
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_EnablePriorityDiscDiscon(void *pContext)
{
    pphLibNfc_LibContext_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    if(NULL != pCtx)
    {
        /* Enable Discovery/Disconnect priority only if transceive is in progress */
        if((phLibNfc_StateTransceive == pCtx->StateContext.CurrState) &&
           (phLibNfc_StateTransceive == pCtx->StateContext.TrgtState))
        {
            pCtx->StateContext.Flag = phLibNfc_StateTransitionComplete;
            PH_LOG_LIBNFC_INFO_STR("State machine flag: %!phLibNfc_TransitionFlag!", pCtx->StateContext.Flag);
            pCtx->StateContext.TrgtState = phLibNfc_StateNone;
            pCtx->SeqMax = 0;
            pCtx->SeqNext = 0;
            pCtx->pSeqHandler = NULL;
            PH_LOG_LIBNFC_INFO_STR("Discovery/Disconnect priority raised");
        }
        else
        {
            PH_LOG_LIBNFC_INFO_STR("No change in Discovery/Disconnect priority");
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_EnablePriorityDiscovery(void *pContext)
{
    pphLibNfc_LibContext_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    if(NULL != pCtx)
    {
        /* Enable Discovery priority only if Remte device Receive is in progress */
        if((phLibNfc_StateRecv == pCtx->StateContext.CurrState) &&
           (phLibNfc_StateSend == pCtx->StateContext.TrgtState))
        {
            pCtx->StateContext.Flag = phLibNfc_StateTransitionComplete;
            PH_LOG_LIBNFC_INFO_STR("State machine flag: %!phLibNfc_TransitionFlag!", pCtx->StateContext.Flag);
            pCtx->StateContext.TrgtState = phLibNfc_StateNone;
            pCtx->SeqMax = 0;
            pCtx->SeqNext = 0;
            pCtx->pSeqHandler = NULL;
            PH_LOG_LIBNFC_INFO_STR("Discovery priority raised");
        }
        else
        {
            PH_LOG_LIBNFC_INFO_STR("No change in Discovery priority");
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

phLibNfc_State_t phLibNfc_GetState(void *pContext)
{
    pphLibNfc_LibContext_t pCtx = pContext;
    phLibNfc_State_t TrgtState = phLibNfc_StateInvalid;
    if(NULL != pCtx)
    {
        if(phLibNfc_StateTrasitionBusy == pCtx->StateContext.Flag)
        {
            TrgtState = pCtx->StateContext.TrgtState;
        }else
        {
            TrgtState = pCtx->StateContext.CurrState;
        }
    }else
    {
        TrgtState = phLibNfc_StateInvalid;
    }
    return TrgtState;
}

static
NFCSTATUS phLibNfc_ChkRfListnerforNFCAPoll(void *pContext,\
                                       pphNciNfc_DeviceInfo_t pNciDevInfo,\
                                       uint8_t bIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNfc_eRFDevType_t LibNfc_RemDevType ;
    uint8_t bSak = 0x00;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != pContext)&& (NULL != pNciDevInfo))
    {
        /*For Mifare Clssic Tag,protocol should be Mifare Classic Protocol
          Currently it is giving T2T protocol which is wrong*/
        if((pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == phNciNfc_e_RfProtocolsT2tProtocol) ||
           (pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == phNciNfc_e_RfProtocolsMifCProtocol) )
        {
            /*Technology Mode - NFC A poll and Protocol - T2t or Mifare classic
              Remote Device type - phNfc_eMifare_PICC*/
            LibNfc_RemDevType = phNfc_eMifare_PICC;

            bSak = pNciDevInfo->pRemDevList[bIndex]->tRemoteDevInfo.Iso14443A_Info.Sak;
            wStatus = phLibNfc_RfListnerRegisterd(pContext,\
                                                 ((uint32_t )LibNfc_RemDevType),\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                switch(bSak)
                {
                    case PHLIBNFC_MIFAREUL_SAK:
                    {
                        pLibContext->DiscoverdNtfType.MifareUL = 1;
                    }
                    break;
                    case PHLIBNFC_MFC1K_SAK:
                    case PHLIBNFC_MFC4K_SAK:
                    case PHLIBNFC_MFCMINI_SAK:
                    case PHLIBNFC_MFC1K_WITHSAK1:
                    case PHLIBNFC_MFC1K_WITHSAK88:
                    case PHLIBNFC_MFC4K_WITHSAK98:
                    case PHLIBNFC_MFC4K_WITHSAKB8:
                    case PHLIBNFC_MFC1K_WITHSAK28:
                    case PHLIBNFC_MFC4K_WITHSAK38:
                    {
                        pLibContext->DiscoverdNtfType.MifareStd = 1;
                    }
                    break;
                    default:
                    {
                        wStatus = NFCSTATUS_FAILED;
                    }
                    break;
                }
            }
        }
        else if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == phNciNfc_e_RfProtocolsT1tProtocol)
        {
            /*Technology Mode - NFC A poll and Protocol - T1t
              Remote Device type - phNfc_eJewel_PICC*/
            LibNfc_RemDevType = phNfc_eJewel_PICC;

            wStatus = phLibNfc_RfListnerRegisterd(pContext,\
                                                 ((uint32_t )LibNfc_RemDevType),\
                                                 bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.Jewel = 1;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }

        }
        else if (pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == phNciNfc_e_RfProtocolsKovioProtocol)
        {
            LibNfc_RemDevType = phNfc_eKovio_PICC;
            wStatus = phLibNfc_RfListnerRegisterd(pContext, \
                                                  ((uint32_t )LibNfc_RemDevType), \
                                                  bSak);
            if (wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.Kovio = 1;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == phNciNfc_e_RfProtocolsIsoDepProtocol)
        {
            /*Technology Mode - NFC A poll and Protocol - ISODEP
              Remote Device type - phNfc_eISO14443_4A_PICC*/
            LibNfc_RemDevType = phNfc_eISO14443_4A_PICC;

            wStatus = phLibNfc_RfListnerRegisterd(pContext,\
                                                  ((uint32_t )LibNfc_RemDevType),\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.ISO14443_4A = 1;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == phNciNfc_e_RfProtocolsNfcDepProtocol)
        {
            /*Technology Mode - NFC A poll and Protocol - NFCDEP
              Remote Device type - phNfc_eNfcIP1_Target*/
            LibNfc_RemDevType = phNfc_eNfcIP1_Target;

            wStatus = phLibNfc_RfListnerRegisterd(pContext,\
                                                  ((uint32_t )LibNfc_RemDevType),\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.NFC = 1;
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
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_ChkRfListnerforNFCBPoll(void *pContext,\
                                       pphNciNfc_DeviceInfo_t pNciDevInfo,\
                                       uint8_t bIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNfc_eRFDevType_t LibNfc_RemDevType ;
    uint8_t bSak = 0x00;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext)&& (NULL != pNciDevInfo))
    {
        if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == \
           phNciNfc_e_RfProtocolsIsoDepProtocol)
        {
            /*Technology Mode - NFC B poll and Protocol - ISODEP
              Remote Device type - phNfc_eISO14443_4B_PICC*/
            LibNfc_RemDevType = phNfc_eISO14443_4B_PICC;

            wStatus = phLibNfc_RfListnerRegisterd(pLibContext,\
                                                  ((uint32_t )LibNfc_RemDevType),\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.ISO14443_4B = 1;
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
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}
static
NFCSTATUS phLibNfc_ChkRfListnerforNFCISO15693Poll(void *pContext, \
                                       pphNciNfc_DeviceInfo_t pNciDevInfo, \
                                       uint8_t bIndex )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNfc_eRFDevType_t LibNfc_RemDevType ;
    uint8_t bSak = 0x00;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext)&& (NULL != pNciDevInfo))
    {
        if(pNciDevInfo->pRemDevList[bIndex]->eRFTechMode == \
           phNciNfc_NFCISO15693_Poll)
        {
            /*Technology Mode - NFC ISO15693 poll and Protocol - 15693
              Remote Device type - phNfc_eISO15693_PICC*/
            LibNfc_RemDevType = phNfc_eISO15693_PICC;

            wStatus = phLibNfc_RfListnerRegisterd(pLibContext,\
                                                  ((uint32_t )LibNfc_RemDevType),\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.ISO15693 = 1;
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
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_ChkRfListnerforNFCFPoll(void *pContext,\
                                           pphNciNfc_DeviceInfo_t pNciDevInfo,\
                                           uint8_t bIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNfc_eRFDevType_t LibNfc_RemDevType ;
    uint8_t bSak = 0x00;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext)&& (NULL != pNciDevInfo))
    {
        if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == \
           phNciNfc_e_RfProtocolsNfcDepProtocol)
        {
            /*Technology Mode - NFC F poll and Protocol - NFCDEP
              Remote Device type - phNfc_eNfcIP1_Target*/
            LibNfc_RemDevType = phNfc_eNfcIP1_Target;

            wStatus = phLibNfc_RfListnerRegisterd(pLibContext,\
                                                  ((uint32_t )LibNfc_RemDevType),\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.NFC = 1;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == \
                phNciNfc_e_RfProtocolsT3tProtocol)
        {
            /*Technology Mode - NFC F poll and Protocol - T3T
              Remote Device type - phNfc_eFelica_PICC*/
            LibNfc_RemDevType = phNfc_eFelica_PICC;

            wStatus = phLibNfc_RfListnerRegisterd(pLibContext,\
                                                 ((uint32_t )LibNfc_RemDevType),\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                 pLibContext->DiscoverdNtfType.Felica = 1;
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
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_ChkRfListnerforNFCAListen(void *pContext,\
                                           pphNciNfc_DeviceInfo_t pNciDevInfo,\
                                           uint8_t bIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNfc_eRFDevType_t LibNfc_RemDevType ;
    uint8_t bSak = 0x00;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext)&& (NULL != pNciDevInfo))
    {
        if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == \
           phNciNfc_e_RfProtocolsNfcDepProtocol)
        {
            /*Technology Mode - NFC A Listen and Protocol - NFCDEP
              Remote Device type - phNfc_eNfcIP1_Initiator*/
            LibNfc_RemDevType = phNfc_eNfcIP1_Initiator;
            wStatus = phLibNfc_RfListnerRegisterd(pLibContext,\
                                                  (uint32_t )LibNfc_RemDevType,\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.NFC = 1;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }else if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == phNciNfc_e_RfProtocolsIsoDepProtocol)
        {
            /*Technology Mode - NFC A Listen and Protocol - ISODEP
              Remote Device type - phNfc_eISO14443_A_PCD*/
            LibNfc_RemDevType = phNfc_eISO14443_A_PCD;
            wStatus = phLibNfc_RfListnerRegisterd(pLibContext,\
                                                  (uint32_t)LibNfc_RemDevType,\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.NFC = 1;
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
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_ChkRfListnerforNFCBListen(void *pContext,\
                                             pphNciNfc_DeviceInfo_t pNciDevInfo,\
                                             uint8_t bIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNfc_eRFDevType_t LibNfc_RemDevType ;
    uint8_t bSak = 0x00;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext)&& (NULL != pNciDevInfo))
    {
        if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == \
           phNciNfc_e_RfProtocolsIsoDepProtocol)
        {
            /*Technology Mode - NFC B Listen and Protocol - ISODEP
              Remote Device type - phNfc_eISO14443_B_PCD*/
            LibNfc_RemDevType = phNfc_eISO14443_B_PCD;

            wStatus = phLibNfc_RfListnerRegisterd(pLibContext,\
                                                  (uint32_t)LibNfc_RemDevType,\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.NFC = 1;
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
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_ChkRfListnerforNFCFListen(void *pContext,\
                                             pphNciNfc_DeviceInfo_t pNciDevInfo,\
                                             uint8_t bIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNfc_eRFDevType_t LibNfc_RemDevType ;
    uint8_t bSak = 0x00;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext)&& (NULL != pNciDevInfo))
    {
        if(pNciDevInfo->pRemDevList[bIndex]->eRFProtocol == \
           phNciNfc_e_RfProtocolsNfcDepProtocol)
        {
          /*Technology Mode - NFC F Listen and Protocol - NFCDEP
            Remote Device type - phNfc_eNfcIP1_Initiator*/
            LibNfc_RemDevType = phNfc_eNfcIP1_Initiator;

            wStatus = phLibNfc_RfListnerRegisterd(pLibContext,\
                                                  ((uint32_t )LibNfc_RemDevType),\
                                                  bSak);
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                pLibContext->DiscoverdNtfType.NFC = 1;
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
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MapRemDevType(phNciNfc_RFDevType_t NciNfc_RemDevType, \
                                        phNfc_eRFDevType_t *LibNfc_RemDevType )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != LibNfc_RemDevType)
    {
        /* TODO: Add Remaining types of remote device types */
        switch(NciNfc_RemDevType)
        {
            case phNciNfc_eISO14443_A_PICC:
            {
                *LibNfc_RemDevType = phNfc_eISO14443_A_PICC;
            }
            break;
            case phNciNfc_eISO14443_4A_PICC:
            {
                *LibNfc_RemDevType = phNfc_eISO14443_4A_PICC;
            }
            break;
            case phNciNfc_eISO14443_3A_PICC:
            {
                *LibNfc_RemDevType = phNfc_eISO14443_3A_PICC;
            }
            break;
            case phNciNfc_eMifareUL_PICC:
            case phNciNfc_eMifare1k_PICC:
            case phNciNfc_eMifare4k_PICC:
            {
                *LibNfc_RemDevType = phNfc_eMifare_PICC;
            }
            break;
            case phNciNfc_eISO14443_B_PICC:
            {
                *LibNfc_RemDevType = phNfc_eISO14443_B_PICC;
            }
            break;
            case phNciNfc_eISO14443_4B_PICC:
            {
                *LibNfc_RemDevType = phNfc_eISO14443_4B_PICC;
            }
            break;
            case phNciNfc_eISO14443_BPrime_PICC:
            {
                *LibNfc_RemDevType = phNfc_eISO14443_BPrime_PICC;
            }
            break;
            case phNciNfc_eFelica_PICC:
            {
                *LibNfc_RemDevType = phNfc_eFelica_PICC;
            }
            break;
            case phNciNfc_eJewel_PICC:
            {
                *LibNfc_RemDevType = phNfc_eJewel_PICC;
            }
            break;
            case phNciNfc_eISO15693_PICC:
            {
                *LibNfc_RemDevType = phNfc_eISO15693_PICC;
            }
            break;
            case phNciNfc_eNfcIP1_Target:
            {
                *LibNfc_RemDevType = phNfc_eNfcIP1_Target;
            }
            break;
            case phNciNfc_eNfcIP1_Initiator:
            {
                *LibNfc_RemDevType = phNfc_eNfcIP1_Initiator;
            }
            break;
            case phNciNfc_eKovio_PICC:
            {
                *LibNfc_RemDevType = phNfc_eKovio_PICC;
            }
            break;
            default:
            {
                *LibNfc_RemDevType = phNfc_eInvalid_DevType;
                wStatus = NFCSTATUS_FAILED;
            }
            break;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Init2Idle(void *pContext, void *Param1, void *Param2, void *Param3)
{
    UNUSED(pContext);
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    return NFCSTATUS_SUCCESS;
}
