/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <phOsalNfc.h>
#include <phOsalNfc_Timer.h>

/* Maximum number of timers supported */
#define PH_MAX_OSAL_NUM_TIMERS                  (15)

/* Defines the base address for generating timer id */
#define PH_OSAL_TIMER_BASE_ADDRESS              (100)

/* Maximum message queue size */
#define PH_OSAL_MSG_QUEUE_MAX_SIZE              (16)

typedef struct phOsalNfc_Timer
{
    PTP_TIMER       pTimer;
    ppCallBck_t     pCallback;
    void*           pContext;
    BOOL            bFired;
    DWORD           dwThreadId;
} phOsalNfc_Timer_t;

typedef struct phOsalNfc_MsgQueue
{
    uint32_t        Message;
    UINT_PTR        Param1;
    UINT_PTR        Param2;
    UINT_PTR        Param3;
    UINT_PTR        Param4;
} phOsalNfc_MsgQueue_t;

typedef struct phOsalNfc_Context
{
    HANDLE                          hCallbackThread;
    uint32_t                        dwCallbackThreadID;
    phOsalNfc_MsgFunc_t             pfnCallback;
    void*                           pCallbackContext;
    phOsalNfc_Timer_t               TimerList[PH_MAX_OSAL_NUM_TIMERS];
    CRITICAL_SECTION                TimerLock;
    HANDLE                          hMsgQueueEvent;
    CRITICAL_SECTION                MsgQueueLock;
    uint8_t                         bMsgQueueHead;
    uint8_t                         bMsgQueueTail;
    uint8_t                         bMsgQueueUsed;
    phOsalNfc_MsgQueue_t            MsgQueue[PH_OSAL_MSG_QUEUE_MAX_SIZE];
} phOsalNfc_Context_t;/**< Structure Instance to access variables of Context structure */

extern phOsalNfc_Context_t* phOsalNfc_GetContext();

/**
 * \ingroup grp_osal_nfc
 * \brief Initializes timer related structures
 */
void phOsalNfc_Timer_Init (void);

/**
 * \ingroup grp_osal_nfc
 * \brief Deinitializes timer related structures
 */
void phOsalNfc_Timer_DeInit (void);

/**
 * \ingroup grp_osal_nfc
 * \brief Initializes message queue related structures
 */
NFCSTATUS phOsalNfc_MsgQueue_Init(void);

/**
 * \ingroup grp_osal_nfc
 * \brief Initializes message queue related structures
 */
void phOsalNfc_MsgQueue_DeInit(void);

#ifdef __cplusplus
}
#endif /*  C++ Compilation guard */
