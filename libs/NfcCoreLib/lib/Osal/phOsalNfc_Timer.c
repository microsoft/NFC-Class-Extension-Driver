/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phOsalNfc_Pch.h"

#include "phOsalNfc_Timer.tmh"

static void phOsalNfc_TimerDeferredCall(void *pParam) 
{
    uint32_t uIndex = (uint32_t)(ULONG_PTR)pParam;
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();

    if (pOsalContext != NULL &&
        pOsalContext->TimerList[uIndex].bFired == TRUE)
    {
        pOsalContext->TimerList[uIndex].pCallback(uIndex + PH_OSAL_TIMER_BASE_ADDRESS, pOsalContext->TimerList[uIndex].pContext);
        pOsalContext->TimerList[uIndex].bFired = FALSE;
    }
}

static void CALLBACK phOsalNfc_TimerCallback(PTP_CALLBACK_INSTANCE pInstance, PVOID pContext, PTP_TIMER pTimer)
{
    uint32_t uIndex = (uint32_t)(ULONG_PTR)pContext;
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();

    UNREFERENCED_PARAMETER(pInstance);
    UNREFERENCED_PARAMETER(pTimer);

    if (pOsalContext != NULL)
    {
        EnterCriticalSection(&pOsalContext->TimerLock);
        
        if (pOsalContext->TimerList[uIndex].pCallback != NULL) {
            pOsalContext->TimerList[uIndex].bFired = TRUE;
            phOsalNfc_PostMsg(PH_OSALNFC_TIMER_MSG, 
                             (UINT_PTR)phOsalNfc_TimerDeferredCall, 
                             (UINT_PTR)uIndex,
                             (UINT_PTR)NULL,
                             (UINT_PTR)NULL);
        }

        LeaveCriticalSection(&pOsalContext->TimerLock);
    }
}

void phOsalNfc_Timer_Init(void)
{
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();
    if (pOsalContext != NULL)
    {
        phOsalNfc_SetMemory((void *)pOsalContext->TimerList,0,sizeof(pOsalContext->TimerList));
        InitializeCriticalSection(&pOsalContext->TimerLock);
    }
}

void phOsalNfc_Timer_DeInit(void)
{
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();
    uint32_t i = 0;

    if (pOsalContext != NULL)
    {
        EnterCriticalSection(&pOsalContext->TimerLock);

        while (i < PH_MAX_OSAL_NUM_TIMERS) 
        {
            if(pOsalContext->TimerList[i].pTimer != NULL)
            {
                PH_LOG_OSAL_INFO_STR("TimerId=%d forced cleanup", i);
                phOsalNfc_Timer_Delete(i + PH_OSAL_TIMER_BASE_ADDRESS);
            }

            i++;
        }

        LeaveCriticalSection(&pOsalContext->TimerLock);
        DeleteCriticalSection(&pOsalContext->TimerLock);
    }
}

uint32_t phOsalNfc_Timer_Create(void)
{
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();
    uint32_t i = 0;

    if (NULL == pOsalContext) {
        return PH_OSALNFC_TIMER_ID_INVALID;
    }

    EnterCriticalSection(&pOsalContext->TimerLock);

    while (i < PH_MAX_OSAL_NUM_TIMERS) 
    {
        /* check whether the timer is free. If free then only it is created */
        if(pOsalContext->TimerList[i].pTimer == NULL)
        {
            pOsalContext->TimerList[i].pTimer = CreateThreadpoolTimer(phOsalNfc_TimerCallback, (void*)i, NULL);
            pOsalContext->TimerList[i].dwThreadId = GetCurrentThreadId();
            break;
        }

        i++;
    }

    LeaveCriticalSection(&pOsalContext->TimerLock);

    if ((i == PH_MAX_OSAL_NUM_TIMERS) || (pOsalContext->TimerList[i].pTimer == NULL))
    {
        return PH_OSALNFC_TIMER_ID_INVALID;
    }

    return (i + PH_OSAL_TIMER_BASE_ADDRESS);
}


/* This starts the timer */ 
NFCSTATUS phOsalNfc_Timer_Start(uint32_t    TimerId,
                          uint32_t     dueTimeMsec, 
                          ppCallBck_t  pCallback,
                          void         *pContext)
{
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();
    uint32_t  uIndex;
    LONGLONG  DueTime;
    uint32_t  uWindow = (dueTimeMsec < 50) ? 0 : (dueTimeMsec / 4);

    if (NULL == pOsalContext ||
        PH_OSALNFC_TIMER_ID_INVALID == TimerId) {
        return PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }

    uIndex = TimerId - PH_OSAL_TIMER_BASE_ADDRESS;
    
    // Convert dueTimeMsec to relative filetime units (100ns)
    DueTime = Int32x32To64(dueTimeMsec, -10000);

    EnterCriticalSection(&pOsalContext->TimerLock);

    SetThreadpoolTimer(pOsalContext->TimerList[uIndex].pTimer, (FILETIME*)&DueTime, 0, uWindow);
    pOsalContext->TimerList[uIndex].pCallback = pCallback;
    pOsalContext->TimerList[uIndex].pContext  = pContext;

    LeaveCriticalSection(&pOsalContext->TimerLock);

    return NFCSTATUS_SUCCESS;
}


NFCSTATUS phOsalNfc_Timer_Stop(uint32_t TimerId)
{
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();
    uint32_t  uIndex;

    if (NULL == pOsalContext ||
        PH_OSALNFC_TIMER_ID_INVALID == TimerId)
    {
        return PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }

    uIndex = TimerId - PH_OSAL_TIMER_BASE_ADDRESS;
    
    SetThreadpoolTimer(pOsalContext->TimerList[uIndex].pTimer, NULL, 0, 0);
    WaitForThreadpoolTimerCallbacks(pOsalContext->TimerList[uIndex].pTimer, TRUE);

    EnterCriticalSection(&pOsalContext->TimerLock);

    pOsalContext->TimerList[uIndex].bFired    = FALSE;
    pOsalContext->TimerList[uIndex].pCallback = NULL;
    pOsalContext->TimerList[uIndex].pContext  = NULL;

    LeaveCriticalSection(&pOsalContext->TimerLock);
    return NFCSTATUS_SUCCESS;
}
    

void phOsalNfc_Timer_Delete(uint32_t TimerId)
{
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();
    uint32_t uIndex;

    //
    // In various places in the code, timers are initialized only
    // after they are first needed.  Despite this fact, they are
    // deleted unconditionally when their context is being deleted
    // Intead of adding an if statement in all places where
    // timers are being deleted, this check is added to prevent 
    // a NULL deref.
    //
    if (NULL == pOsalContext ||
        PH_OSALNFC_TIMER_ID_INVALID == TimerId)
    {
        return;
    }

    uIndex = TimerId - PH_OSAL_TIMER_BASE_ADDRESS;

    EnterCriticalSection(&pOsalContext->TimerLock);

    if(pOsalContext->TimerList[uIndex].pTimer != NULL) 
    {
        SetThreadpoolTimer(pOsalContext->TimerList[uIndex].pTimer, NULL, 0, 0);
        LeaveCriticalSection(&pOsalContext->TimerLock);
        
        WaitForThreadpoolTimerCallbacks(pOsalContext->TimerList[uIndex].pTimer, TRUE);

        EnterCriticalSection(&pOsalContext->TimerLock);

        CloseThreadpoolTimer(pOsalContext->TimerList[uIndex].pTimer);
        pOsalContext->TimerList[uIndex].pTimer    = NULL;
        pOsalContext->TimerList[uIndex].pCallback = NULL;
        pOsalContext->TimerList[uIndex].pContext  = NULL;
        pOsalContext->TimerList[uIndex].bFired    = FALSE;

        uIndex = 0;
        while (uIndex < PH_MAX_OSAL_NUM_TIMERS) 
        {
            if (pOsalContext->TimerList[uIndex].pTimer != NULL)
            {
                break;
            }
            uIndex++;
        }
    }

    LeaveCriticalSection(&pOsalContext->TimerLock);
}
