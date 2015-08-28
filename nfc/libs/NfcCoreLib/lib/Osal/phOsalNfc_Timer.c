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

    if (gpphOsalNfc_Context != NULL &&
        gpphOsalNfc_Context->TimerList[uIndex].bFired == TRUE)
    {
        gpphOsalNfc_Context->TimerList[uIndex].pCallback(uIndex + PH_OSAL_TIMER_BASE_ADDRESS, gpphOsalNfc_Context->TimerList[uIndex].pContext);
        gpphOsalNfc_Context->TimerList[uIndex].bFired = FALSE;
    }
}

static void CALLBACK phOsalNfc_TimerCallback(PTP_CALLBACK_INSTANCE pInstance, PVOID pContext, PTP_TIMER pTimer)
{
    uint32_t uIndex = (uint32_t)(ULONG_PTR)pContext;

    UNREFERENCED_PARAMETER(pInstance);
    UNREFERENCED_PARAMETER(pTimer);

    if (gpphOsalNfc_Context != NULL)
    {
        EnterCriticalSection(&gpphOsalNfc_Context->TimerLock);
        
        if (gpphOsalNfc_Context->TimerList[uIndex].pCallback != NULL) {
            gpphOsalNfc_Context->TimerList[uIndex].bFired = TRUE;
            phOsalNfc_PostMsg(PH_OSALNFC_TIMER_MSG, 
                             (UINT_PTR)phOsalNfc_TimerDeferredCall, 
                             (UINT_PTR)uIndex,
                             (UINT_PTR)NULL,
                             (UINT_PTR)NULL);
        }

        LeaveCriticalSection(&gpphOsalNfc_Context->TimerLock);
    }
}

void phOsalNfc_Timer_Init(void)
{
    if (gpphOsalNfc_Context != NULL)
    {
        phOsalNfc_SetMemory((void *)gpphOsalNfc_Context->TimerList,0,sizeof(gpphOsalNfc_Context->TimerList));
        InitializeCriticalSection(&gpphOsalNfc_Context->TimerLock);
    }
}

void phOsalNfc_Timer_DeInit(void)
{
    uint32_t i = 0;

    if (gpphOsalNfc_Context != NULL)
    {
        EnterCriticalSection(&gpphOsalNfc_Context->TimerLock);

        while (i < PH_MAX_OSAL_NUM_TIMERS) 
        {
            if(gpphOsalNfc_Context->TimerList[i].pTimer != NULL)
            {
                PH_LOG_OSAL_INFO_STR("TimerId=%d forced cleanup", i);
                phOsalNfc_Timer_Delete(i + PH_OSAL_TIMER_BASE_ADDRESS);
            }

            i++;
        }

        LeaveCriticalSection(&gpphOsalNfc_Context->TimerLock);
        DeleteCriticalSection(&gpphOsalNfc_Context->TimerLock);
    }
}

uint32_t phOsalNfc_Timer_Create(void)
{
    uint32_t i = 0;

    if (NULL == gpphOsalNfc_Context) {
        return PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }

    EnterCriticalSection(&gpphOsalNfc_Context->TimerLock);

    while (i < PH_MAX_OSAL_NUM_TIMERS) 
    {
        /* check whether the timer is free. If free then only it is created */
        if(gpphOsalNfc_Context->TimerList[i].pTimer == NULL)
        {
            gpphOsalNfc_Context->TimerList[i].pTimer = CreateThreadpoolTimer(phOsalNfc_TimerCallback, (void*)i, NULL);
            gpphOsalNfc_Context->TimerList[i].dwThreadId = GetCurrentThreadId();
            break;
        }

        i++;
    }

    LeaveCriticalSection(&gpphOsalNfc_Context->TimerLock);
    
    if ((i == PH_MAX_OSAL_NUM_TIMERS) || (gpphOsalNfc_Context->TimerList[i].pTimer == NULL))
    {
        return PH_OSALNFC_INVALID_TIMER_ID;
    }
    
    return (i + PH_OSAL_TIMER_BASE_ADDRESS);
}


/* This starts the timer */ 
NFCSTATUS phOsalNfc_Timer_Start(uint32_t    TimerId,
                          uint32_t     dueTimeMsec, 
                          ppCallBck_t  pCallback,
                          void         *pContext)
{
    uint32_t  uIndex;
    LONGLONG  DueTime;
    uint32_t  uWindow = (dueTimeMsec < 50) ? 0 : (dueTimeMsec / 4);

    if (NULL == gpphOsalNfc_Context ||
        PH_OSALNFC_TIMER_ID_INVALID == TimerId) {
        return PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }

    uIndex = TimerId - PH_OSAL_TIMER_BASE_ADDRESS;
    
    // Convert dueTimeMsec to relative filetime units (100ns)
    DueTime = Int32x32To64(dueTimeMsec, -10000);

    EnterCriticalSection(&gpphOsalNfc_Context->TimerLock);

    SetThreadpoolTimer(gpphOsalNfc_Context->TimerList[uIndex].pTimer, (FILETIME*)&DueTime, 0, uWindow);
    gpphOsalNfc_Context->TimerList[uIndex].pCallback = pCallback;
    gpphOsalNfc_Context->TimerList[uIndex].pContext  = pContext;

    LeaveCriticalSection(&gpphOsalNfc_Context->TimerLock);

    return NFCSTATUS_SUCCESS;
}


NFCSTATUS phOsalNfc_Timer_Stop(uint32_t TimerId)
{
    uint32_t  uIndex;

    if (NULL == gpphOsalNfc_Context ||
        PH_OSALNFC_TIMER_ID_INVALID == TimerId)
    {
        return PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }

    uIndex = TimerId - PH_OSAL_TIMER_BASE_ADDRESS;
    
    SetThreadpoolTimer(gpphOsalNfc_Context->TimerList[uIndex].pTimer, NULL, 0, 0);
    WaitForThreadpoolTimerCallbacks(gpphOsalNfc_Context->TimerList[uIndex].pTimer, TRUE);

    EnterCriticalSection(&gpphOsalNfc_Context->TimerLock);

    gpphOsalNfc_Context->TimerList[uIndex].bFired    = FALSE;
    gpphOsalNfc_Context->TimerList[uIndex].pCallback = NULL;
    gpphOsalNfc_Context->TimerList[uIndex].pContext  = NULL;

    LeaveCriticalSection(&gpphOsalNfc_Context->TimerLock);
    return NFCSTATUS_SUCCESS;
}
    

void phOsalNfc_Timer_Delete(uint32_t TimerId)
{
    uint32_t uIndex;

    //
    // In various places in the code, timers are initialized only
    // after they are first needed.  Despite this fact, they are
    // deleted unconditionally when their context is being deleted
    // Intead of adding an if statement in all places where
    // timers are being deleted, this check is added to prevent 
    // a NULL deref.
    //
    if (NULL == gpphOsalNfc_Context ||
        PH_OSALNFC_TIMER_ID_INVALID == TimerId)
    {
        return;
    }

    uIndex = TimerId - PH_OSAL_TIMER_BASE_ADDRESS;

    EnterCriticalSection(&gpphOsalNfc_Context->TimerLock);

    if(gpphOsalNfc_Context->TimerList[uIndex].pTimer != NULL) 
    {
        SetThreadpoolTimer(gpphOsalNfc_Context->TimerList[uIndex].pTimer, NULL, 0, 0);
        LeaveCriticalSection(&gpphOsalNfc_Context->TimerLock);
        
        WaitForThreadpoolTimerCallbacks(gpphOsalNfc_Context->TimerList[uIndex].pTimer, TRUE);

        EnterCriticalSection(&gpphOsalNfc_Context->TimerLock);

        CloseThreadpoolTimer(gpphOsalNfc_Context->TimerList[uIndex].pTimer);
        gpphOsalNfc_Context->TimerList[uIndex].pTimer    = NULL;
        gpphOsalNfc_Context->TimerList[uIndex].pCallback = NULL;
        gpphOsalNfc_Context->TimerList[uIndex].pContext  = NULL;
        gpphOsalNfc_Context->TimerList[uIndex].bFired    = FALSE;

        uIndex = 0;
        while (uIndex < PH_MAX_OSAL_NUM_TIMERS) 
        {
            if (gpphOsalNfc_Context->TimerList[uIndex].pTimer != NULL)
            {
                break;
            }
            uIndex++;
        }
    }

    LeaveCriticalSection(&gpphOsalNfc_Context->TimerLock);
}
